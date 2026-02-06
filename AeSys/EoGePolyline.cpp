#include "Stdafx.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "PrimState.h"


namespace { constexpr unsigned int defaultDpi = 96; }

namespace polyline {
EoGePoint4dArray pts_;
bool LoopLine;

void BeginLineLoop() {
  pts_.SetSize(0);
  LoopLine = true;
}

void BeginLineStrip() {
  pts_.SetSize(0);
  LoopLine = false;
}

static bool AnyPointsInView(EoGePoint4dArray& pointsArray) {
  for (int i = 0; i < pointsArray.GetSize(); i++) {
    if (pointsArray[i].IsInView()) { return true; }
  }
  return false;
}

/** @brief Renders a polyline with a specified line type.
 *
 * This function takes a view, device context, an array of 4D points representing the polyline,
 * and a line type definition. It processes the dash pattern defined in the line type and
 * draws the polyline segments accordingly, handling dashes and spaces as specified.
 *
 * @param view Pointer to the AeSysView object for rendering context.
 * @param deviceContext Pointer to the CDC object for drawing operations.
 * @param pointsArray Array of EoGePoint4d representing the vertices of the polyline.
 * @param lineType Pointer to the EoDbLineType object defining the dash pattern.
 */
void __Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray, EoDbLineType* lineType) {
  const auto numberOfDashElements = lineType->GetNumberOfDashes();
  if (numberOfDashElements == 0 || pointsArray.GetSize() < 2) { return; }

  EoGePoint3d modelPoints[2]{};
  CPoint devicePoints[2]{};
  EoGePoint4d viewPoints[2]{};

  size_t dashElementIndex{};
  std::vector<double> dashElements(numberOfDashElements);
  lineType->GetDashElements(dashElements.data());

  const double dpi = static_cast<double>(std::max(defaultDpi, GetDpiForSystem()));
  const double pixelSize = 1.0 / dpi; // Will only be used for dots where dash element length is 0.0

  double dashElementSize = std::max(pixelSize, fabs(dashElements[dashElementIndex]));

  for (int i = 0; i < pointsArray.GetSize() - 1; i++) {
    EoGeVector3d lineAsVector(pointsArray[i], pointsArray[i + 1]);
    modelPoints[0] = pointsArray[i];

    double lineLength = lineAsVector.Length();
    double remainingDistanceToEnd = lineLength;

    while (dashElementSize <= remainingDistanceToEnd + Eo::geometricTolerance) {
      EoGeVector3d dashAsVector(lineAsVector);
      dashAsVector *= dashElementSize / lineLength;
      modelPoints[1] = modelPoints[0] + dashAsVector;
      remainingDistanceToEnd -= dashElementSize;
      if (dashElements[dashElementIndex] >= 0.0) {
        viewPoints[0] = modelPoints[0];
        viewPoints[1] = modelPoints[1];

        view->ModelViewTransformPoints(2, viewPoints);

        if (EoGePoint4d::ClipLine(viewPoints[0], viewPoints[1])) {
          view->DoProjection(devicePoints, 2, &viewPoints[0]);
          deviceContext->Polyline(devicePoints, 2);
        }
      }
      modelPoints[0] = modelPoints[1];
      dashElementIndex = (dashElementIndex + 1) % numberOfDashElements;
      dashElementSize = std::max(pixelSize, fabs(dashElements[dashElementIndex]));
    }
    if (remainingDistanceToEnd > Eo::geometricTolerance) {  // Partial component of dash section must produced
      if (dashElements[dashElementIndex] >= 0.0) {
        modelPoints[1] = pointsArray[i + 1];

        viewPoints[0] = modelPoints[0];
        viewPoints[1] = modelPoints[1];

        view->ModelViewTransformPoints(2, viewPoints);

        if (EoGePoint4d::ClipLine(viewPoints[0], viewPoints[1])) {
          view->DoProjection(devicePoints, 2, &viewPoints[0]);
          deviceContext->Polyline(devicePoints, 2);
        }
      }
    }
    // Length of dash remaining
    dashElementSize -= remainingDistanceToEnd;
  }
}

void __End(AeSysView* view, CDC* deviceContext, EoInt16 lineTypeIndex) {
  if (EoDbPrimitive::IsSupportedTyp(lineTypeIndex)) {
    INT_PTR size = pts_.GetSize();
    if (size > 1) {
      view->ModelViewTransformPoints(pts_);

      if (AnyPointsInView(pts_)) {
        CPoint point;
        point = view->DoProjection(pts_[0]);
        deviceContext->MoveTo(point);

        for (INT_PTR i = 1; i < size; i++) {
          point = view->DoProjection(pts_[i]);
          deviceContext->LineTo(point);
        }
        if (LoopLine) {
          point = view->DoProjection(pts_[0]);
          deviceContext->LineTo(point);
        }
        return;
      }
    }
  } else {
    EoDbLineTypeTable* LineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();

    EoDbLineType* LineType;
    if (!LineTypeTable->LookupUsingLegacyIndex(static_cast<EoUInt16>(lineTypeIndex), LineType)) { return; }
    pstate.SetLineType(deviceContext, 1);
    __Display(view, deviceContext, pts_, LineType);
    pstate.SetLineType(deviceContext, lineTypeIndex);
  }
}
void GeneratePointsForNPoly(EoGePoint3d& centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis,
                            int numberOfPoints, EoGePoint3dArray& pts) {
  EoGeTransformMatrix tm(centerPoint, majorAxis, minorAxis);

  tm.Inverse();

  // Determine the parameter (angular increment)
  double AngleIncrement = Eo::TwoPi / double(numberOfPoints);
  double CosIncrement = cos(AngleIncrement);
  double SinIncrement = sin(AngleIncrement);
  pts.SetSize(numberOfPoints);

  pts[0](1.0, 0.0, 0.0);

  for (int i = 0; i < numberOfPoints - 1; i++) {
    pts[i + 1](pts[i].x * CosIncrement - pts[i].y * SinIncrement, pts[i].y * CosIncrement + pts[i].x * SinIncrement,
               0.0);
  }
  for (int i = 0; i < numberOfPoints; i++) { pts[i] = tm * pts[i]; }
}

/** @brief Selects polyline segments intersecting with a given line.
 *
 * This function checks each segment of the polyline defined by the points in `pts_`
 * to see if it intersects with the provided line. If an intersection is found,
 * the intersection point is calculated and added to the `intersections` array.
 *
 * @param view Pointer to the AeSysView object for coordinate transformations.
 * @param line The line to check for intersections with the polyline segments.
 * @param intersections Output array that will contain the intersection points.
 * @return True if any intersection points were found; otherwise, false.
 */
bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  EoGePoint4d ptBeg(pts_[0]);
  EoGePoint4d ptEnd;

  view->ModelViewTransformPoint(ptBeg);

  for (EoUInt16 w = 1; w < pts_.GetSize(); w++) {
    ptEnd = EoGePoint4d(pts_[w]);
    view->ModelViewTransformPoint(ptEnd);

    EoGePoint3d ptInt;
    if (EoGeLine::Intersection_xy(line, EoGeLine(ptBeg, ptEnd), ptInt)) {
      double dRel;
      line.RelOfPtToEndPts(ptInt, dRel);
      if (dRel >= -Eo::geometricTolerance && dRel <= 1.0 + Eo::geometricTolerance) {
        EoGeLine(ptBeg, ptEnd).RelOfPtToEndPts(ptInt, dRel);
        if (dRel >= -Eo::geometricTolerance && dRel <= 1.0 + Eo::geometricTolerance) {
          ptInt.z = ptBeg.z + dRel * (ptEnd.z - ptBeg.z);
          intersections.Add(ptInt);
        }
      }
    }
    ptBeg = ptEnd;
  }
  return (!intersections.IsEmpty());
}

bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, double& dRel, EoGePoint3d& ptProj) {
  bool bResult = false;

  EoGePoint4d ptBeg(pts_[0]);
  view->ModelViewTransformPoint(ptBeg);

  for (int i = 1; i < (int)pts_.GetSize(); i++) {
    EoGePoint4d ptEnd = EoGePoint4d(pts_[i]);
    view->ModelViewTransformPoint(ptEnd);
    EoGeLine LineSegment(ptBeg, ptEnd);
    if (LineSegment.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &dRel)) {
      ptProj.z = ptBeg.z + dRel * (ptEnd.z - ptBeg.z);
      bResult = true;
      break;
    }
    ptBeg = ptEnd;
  }
  return (bResult);
}
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeftPoint, EoGePoint3d upperRightPoint) {
  EoGePoint4d ptBeg(pts_[0]);
  view->ModelViewTransformPoint(ptBeg);

  for (EoUInt16 w = 1; w < pts_.GetSize(); w++) {
    EoGePoint4d ptEnd(pts_[w]);
    view->ModelViewTransformPoint(ptEnd);

    EoGeLine LineSegment(ptBeg, ptEnd);
    if (LineSegment.IsContainedXY(lowerLeftPoint, upperRightPoint)) { return true; }

    ptBeg = ptEnd;
  }
  return false;
}
// Not considering possible closure
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeftPoint, EoGePoint3d upperRightPoint,
                          const EoGePoint3dArray& pts) {
  EoGePoint4d ptBeg(pts[0]);
  view->ModelViewTransformPoint(ptBeg);

  for (EoUInt16 w = 1; w < pts.GetSize(); w++) {
    EoGePoint4d ptEnd(pts[w]);
    view->ModelViewTransformPoint(ptEnd);

    EoGeLine LineSegment(ptBeg, ptEnd);
    if (LineSegment.IsContainedXY(lowerLeftPoint, upperRightPoint)) { return true; }

    ptBeg = ptEnd;
  }
  return false;
}
void SetVertex(const EoGePoint3d& point) {
  EoGePoint4d Point4(point);
  pts_.Add(Point4);
}

}  // namespace polyline
