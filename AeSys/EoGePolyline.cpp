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
#include "EoGsRenderState.h"


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
  CPoint clientPoints[2]{};
  EoGePoint4d ndcPoints[2]{};

  size_t dashElementIndex{};
  std::vector<double> dashElements(numberOfDashElements);
  lineType->GetDashElements(dashElements.data());

  const double dpi = static_cast<double>(std::max(defaultDpi, GetDpiForSystem()));
  const double pixelSize = 1.0 / dpi; // Will only be used for dots where dash element length is 0.0

  double dashElementSize = std::max(pixelSize, fabs(dashElements[dashElementIndex]));

  for (int i = 0; i < pointsArray.GetSize() - 1; i++) {
    EoGeVector3d lineAsVector(EoGePoint3d{pointsArray[i]}, EoGePoint3d{pointsArray[i + 1]});
    modelPoints[0] = EoGePoint3d{pointsArray[i]};

    double lineLength = lineAsVector.Length();
    double remainingDistanceToEnd = lineLength;

    while (dashElementSize <= remainingDistanceToEnd + Eo::geometricTolerance) {
      EoGeVector3d dashAsVector(lineAsVector);
      dashAsVector *= dashElementSize / lineLength;
      modelPoints[1] = modelPoints[0] + dashAsVector;
      remainingDistanceToEnd -= dashElementSize;
      if (dashElements[dashElementIndex] >= 0.0) {
        ndcPoints[0] = EoGePoint4d{modelPoints[0]};
        ndcPoints[1] = EoGePoint4d{modelPoints[1]};

        view->ModelViewTransformPoints(2, ndcPoints);

        if (EoGePoint4d::ClipLine(ndcPoints[0], ndcPoints[1])) {
          view->ProjectToClient(clientPoints, 2, &ndcPoints[0]);
          deviceContext->Polyline(clientPoints, 2);
        }
      }
      modelPoints[0] = modelPoints[1];
      dashElementIndex = (dashElementIndex + 1) % numberOfDashElements;
      dashElementSize = std::max(pixelSize, fabs(dashElements[dashElementIndex]));
    }
    if (remainingDistanceToEnd > Eo::geometricTolerance) {  // Partial component of dash section must produced
      if (dashElements[dashElementIndex] >= 0.0) {
        modelPoints[1] = EoGePoint3d{pointsArray[i + 1]};

        ndcPoints[0] = EoGePoint4d{modelPoints[0]};
        ndcPoints[1] = EoGePoint4d{modelPoints[1]};

        view->ModelViewTransformPoints(2, ndcPoints);

        if (EoGePoint4d::ClipLine(ndcPoints[0], ndcPoints[1])) {
          view->ProjectToClient(clientPoints, 2, &ndcPoints[0]);
          deviceContext->Polyline(clientPoints, 2);
        }
      }
    }
    // Length of dash remaining
    dashElementSize -= remainingDistanceToEnd;
  }
}

void __End(AeSysView* view, CDC* deviceContext, std::int16_t lineTypeIndex) {
  if (EoDbPrimitive::IsSupportedTyp(lineTypeIndex)) {
    auto size = pts_.GetSize();
    if (size > 1) {
      view->ModelViewTransformPoints(pts_);

      if (AnyPointsInView(pts_)) {
        CPoint clientPoint;
        clientPoint = view->ProjectToClient(pts_[0]);
        deviceContext->MoveTo(clientPoint);

        for (INT_PTR i = 1; i < size; i++) {
          clientPoint = view->ProjectToClient(pts_[i]);
          deviceContext->LineTo(clientPoint);
        }
        if (LoopLine) {
          clientPoint = view->ProjectToClient(pts_[0]);
          deviceContext->LineTo(clientPoint);
        }
        return;
      }
    }
  } else {
    auto* lineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();

    EoDbLineType* lineType;
    if (!lineTypeTable->LookupUsingLegacyIndex(static_cast<std::uint16_t>(lineTypeIndex), lineType)) { return; }
    renderState.SetLineType(deviceContext, 1);
    __Display(view, deviceContext, pts_, lineType);
    renderState.SetLineType(deviceContext, lineTypeIndex);
  }
}

void GeneratePointsForNPoly(EoGePoint3d& centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis,
                            int numberOfPoints, EoGePoint3dArray& pts) {
  EoGeTransformMatrix transformMatrix(centerPoint, majorAxis, minorAxis);

  transformMatrix.Inverse();

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
  for (int i = 0; i < numberOfPoints; i++) { pts[i] = transformMatrix * pts[i]; }
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
 * @return true if any intersection points were found; otherwise, false.
 */
bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  EoGePoint4d begin(pts_[0]);
  EoGePoint4d end;

  view->ModelViewTransformPoint(begin);

  for (std::uint16_t w = 1; w < pts_.GetSize(); w++) {
    end = EoGePoint4d(pts_[w]);
    view->ModelViewTransformPoint(end);

    EoGePoint3d intersection;
    if (EoGeLine::Intersection_xy(line, EoGeLine(EoGePoint3d{begin}, EoGePoint3d{end}), intersection)) {
      double relation{};
      
      if (!line.ComputeParametricRelation(intersection, relation)) { continue; }
      
      if (relation >= -Eo::geometricTolerance && relation <= 1.0 + Eo::geometricTolerance) {
        if (!EoGeLine(EoGePoint3d{begin}, EoGePoint3d{end}).ComputeParametricRelation(intersection, relation)) { continue; }
        
        if (relation >= -Eo::geometricTolerance && relation <= 1.0 + Eo::geometricTolerance) {
          intersection.z = begin.z + relation * (end.z - begin.z);
          intersections.Add(intersection);
        }
      }
    }
    begin = end;
  }
  return (!intersections.IsEmpty());
}

bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, double& dRel, EoGePoint3d& ptProj) {
  bool result{};

  EoGePoint4d begin(pts_[0]);
  view->ModelViewTransformPoint(begin);

  for (int i = 1; i < (int)pts_.GetSize(); i++) {
    EoGePoint4d end = EoGePoint4d(pts_[i]);
    view->ModelViewTransformPoint(end);
    EoGeLine LineSegment(EoGePoint3d{begin}, EoGePoint3d{end});
    if (LineSegment.IsSelectedByPointXY(EoGePoint3d{point}, view->SelectApertureSize(), ptProj, &dRel)) {
      ptProj.z = begin.z + dRel * (end.z - begin.z);
      result = true;
      break;
    }
    begin = end;
  }
  return result;
}

bool SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeftPoint, EoGePoint3d upperRightPoint) {
  EoGePoint4d begin(pts_[0]);
  view->ModelViewTransformPoint(begin);

  for (std::uint16_t w = 1; w < pts_.GetSize(); w++) {
    EoGePoint4d end(pts_[w]);
    view->ModelViewTransformPoint(end);

    EoGeLine LineSegment(EoGePoint3d{begin}, EoGePoint3d{end});
    if (LineSegment.IsContainedXY(lowerLeftPoint, upperRightPoint)) { return true; }

    begin = end;
  }
  return false;
}

// Not considering possible closure
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeftPoint, EoGePoint3d upperRightPoint,
                          const EoGePoint3dArray& pts) {
  EoGePoint4d begin(pts[0]);
  view->ModelViewTransformPoint(begin);

  for (std::uint16_t w = 1; w < pts.GetSize(); w++) {
    EoGePoint4d end(pts[w]);
    view->ModelViewTransformPoint(end);

    EoGeLine LineSegment(EoGePoint3d{begin}, EoGePoint3d{end});
    if (LineSegment.IsContainedXY(lowerLeftPoint, upperRightPoint)) { return true; }

    begin = end;
  }
  return false;
}

void SetVertex(const EoGePoint3d& point) {
  EoGePoint4d Point4(point);
  pts_.Add(Point4);
}

}  // namespace polyline
