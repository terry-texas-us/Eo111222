#include "Stdafx.h"

#include "EoGsVertexBuffer.h"

#include <algorithm>
#include <cmath>

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderDevice.h"
#include "EoGsRenderState.h"

namespace {
constexpr unsigned int defaultDpi = 96;
}

// ── Vertex Accumulation ───────────────────────────────────────────────

void EoGsVertexBuffer::BeginLineStrip() {
  m_points.SetSize(0);
  m_isLoop = false;
}

void EoGsVertexBuffer::BeginLineLoop() {
  m_points.SetSize(0);
  m_isLoop = true;
}

void EoGsVertexBuffer::SetVertex(const EoGePoint3d& point) {
  EoGePoint4d pt(point);
  m_points.Add(pt);
}

// ── Rendering ─────────────────────────────────────────────────────────

bool EoGsVertexBuffer::AnyPointsInView(EoGePoint4dArray& pointsArray) {
  for (int i = 0; i < pointsArray.GetSize(); i++) {
    if (pointsArray[i].IsInView()) { return true; }
  }
  return false;
}

/** @brief Renders a polyline with a specified line type (dash pattern).
 *
 * Processes the dash/gap pattern from the linetype definition and draws
 * individual line segments via renderDevice->Polyline().
 *
 * @param view          Pointer to the AeSysView for coordinate transforms.
 * @param renderDevice  Abstract rendering device for drawing operations.
 * @param pointsArray   Array of 4D points representing the polyline vertices.
 * @param lineType      Pointer to the EoDbLineType defining the dash pattern.
 */
void EoGsVertexBuffer::DisplayDashPattern(
    AeSysView* view, EoGsRenderDevice* renderDevice, EoGePoint4dArray& pointsArray, EoDbLineType* lineType) {
  const auto numberOfDashElements = lineType->GetNumberOfDashes();
  if (numberOfDashElements == 0 || pointsArray.GetSize() < 2) { return; }

  EoGePoint3d modelPoints[2]{};
  CPoint clientPoints[2]{};
  EoGePoint4d ndcPoints[2]{};

  size_t dashElementIndex{};
  std::vector<double> dashElements(numberOfDashElements);
  lineType->GetDashElements(dashElements.data());

  const double dpi = static_cast<double>(std::max(defaultDpi, GetDpiForSystem()));
  const double pixelSize = 1.0 / dpi;  // Used for dots where dash element length is 0.0

  double dashElementSize = std::max(pixelSize, std::abs(dashElements[dashElementIndex]));

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
          renderDevice->Polyline(clientPoints, 2);
        }
      }
      modelPoints[0] = modelPoints[1];
      dashElementIndex = (dashElementIndex + 1) % numberOfDashElements;
      dashElementSize = std::max(pixelSize, std::abs(dashElements[dashElementIndex]));
    }
    if (remainingDistanceToEnd > Eo::geometricTolerance) {  // Partial component of dash section
      if (dashElements[dashElementIndex] >= 0.0) {
        modelPoints[1] = EoGePoint3d{pointsArray[i + 1]};

        ndcPoints[0] = EoGePoint4d{modelPoints[0]};
        ndcPoints[1] = EoGePoint4d{modelPoints[1]};

        view->ModelViewTransformPoints(2, ndcPoints);

        if (EoGePoint4d::ClipLine(ndcPoints[0], ndcPoints[1])) {
          view->ProjectToClient(clientPoints, 2, &ndcPoints[0]);
          renderDevice->Polyline(clientPoints, 2);
        }
      }
    }
    // Length of dash remaining
    dashElementSize -= remainingDistanceToEnd;
  }
}

void EoGsVertexBuffer::End(
    AeSysView* view, EoGsRenderDevice* renderDevice, std::int16_t lineTypeIndex, const std::wstring& lineTypeName) {
  // Transitional: renderState still uses CDC* for pen style changes
  auto* deviceContext = renderDevice->GetCDC();

  // Try name-based lookup first for non-Continuous linetypes
  if (!lineTypeName.empty() && _wcsicmp(lineTypeName.c_str(), L"CONTINUOUS") != 0
      && _wcsicmp(lineTypeName.c_str(), L"ByLayer") != 0 && _wcsicmp(lineTypeName.c_str(), L"ByBlock") != 0) {
    auto* lineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();
    EoDbLineType* lineType{};
    if (lineTypeTable->Lookup(CString(lineTypeName.c_str()), lineType) && lineType->GetNumberOfDashes() > 0) {
      renderState.SetLineType(deviceContext, 1);
      DisplayDashPattern(view, renderDevice, m_points, lineType);
      renderState.SetLineType(deviceContext, lineTypeIndex);
      return;
    }
  }

  // Continuous or unresolved name — use solid pen for stock types
  if (lineTypeIndex == 0 || lineTypeIndex == 1 || EoDbPrimitive::IsSupportedTyp(lineTypeIndex)) {
    auto size = m_points.GetSize();
    if (size > 1) {
      view->ModelViewTransformPoints(m_points);

      if (AnyPointsInView(m_points)) {
        CPoint clientPoint;
        clientPoint = view->ProjectToClient(m_points[0]);
        renderDevice->MoveTo(clientPoint.x, clientPoint.y);

        for (INT_PTR i = 1; i < size; i++) {
          clientPoint = view->ProjectToClient(m_points[i]);
          renderDevice->LineTo(clientPoint.x, clientPoint.y);
        }
        if (m_isLoop) {
          clientPoint = view->ProjectToClient(m_points[0]);
          renderDevice->LineTo(clientPoint.x, clientPoint.y);
        }
        return;
      }
    }
  } else {
    // Legacy index fallback for V1 PEG files without linetype names
    auto* lineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();

    EoDbLineType* lineType{};
    if (!lineTypeTable->LookupUsingLegacyIndex(static_cast<std::uint16_t>(lineTypeIndex), lineType)) { return; }
    renderState.SetLineType(deviceContext, 1);
    DisplayDashPattern(view, renderDevice, m_points, lineType);
    renderState.SetLineType(deviceContext, lineTypeIndex);
  }
}

// ── Selection ─────────────────────────────────────────────────────────

bool EoGsVertexBuffer::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  EoGePoint4d begin(m_points[0]);
  EoGePoint4d end;

  view->ModelViewTransformPoint(begin);

  for (std::uint16_t w = 1; w < m_points.GetSize(); w++) {
    end = EoGePoint4d(m_points[w]);
    view->ModelViewTransformPoint(end);

    EoGePoint3d intersection;
    if (EoGeLine::Intersection_xy(line, EoGeLine(EoGePoint3d{begin}, EoGePoint3d{end}), intersection)) {
      double relation{};

      if (!line.ComputeParametricRelation(intersection, relation)) {
        begin = end;
        continue;
      }

      if (relation >= -Eo::geometricTolerance && relation <= 1.0 + Eo::geometricTolerance) {
        if (!EoGeLine(EoGePoint3d{begin}, EoGePoint3d{end}).ComputeParametricRelation(intersection, relation)) {
          begin = end;
          continue;
        }

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

bool EoGsVertexBuffer::SelectUsingPoint(AeSysView* view, EoGePoint4d point, double& dRel, EoGePoint3d& ptProj) {
  bool result{};

  EoGePoint4d begin(m_points[0]);
  view->ModelViewTransformPoint(begin);

  for (int i = 1; i < (int)m_points.GetSize(); i++) {
    EoGePoint4d end = EoGePoint4d(m_points[i]);
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

bool EoGsVertexBuffer::SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeftPoint, EoGePoint3d upperRightPoint) {
  EoGePoint4d begin(m_points[0]);
  view->ModelViewTransformPoint(begin);

  for (std::uint16_t w = 1; w < m_points.GetSize(); w++) {
    EoGePoint4d end(m_points[w]);
    view->ModelViewTransformPoint(end);

    EoGeLine LineSegment(EoGePoint3d{begin}, EoGePoint3d{end});
    if (LineSegment.IsContainedXY(lowerLeftPoint, upperRightPoint)) { return true; }

    begin = end;
  }
  return false;
}
