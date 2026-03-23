#include "Stdafx.h"

#include <climits>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoDxfEntities.h"
#include "EoDxfInterface.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderState.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif

std::uint16_t EoDbPolyline::sm_EdgeToEvaluate{};
std::uint16_t EoDbPolyline::sm_Edge{};
int EoDbPolyline::sm_pivotVertex{};

EoDbPolyline::EoDbPolyline() { m_flags = 0; }

/** Constructs a closed polyline approximating a regular polygon.
 *
 * @param penColor The pen color for the polyline.
 * @param lineType The line type for the polyline.
 * @param centerPoint The center point of the polygon.
 * @param radius The radius of the circumscribed circle.
 * @param numberOfSides The number of sides for the polygon.
 */
EoDbPolyline::EoDbPolyline(
    std::int16_t penColor, std::int16_t lineType, EoGePoint3d& centerPoint, double radius, int numberOfSides)
    : EoDbPrimitive(penColor, lineType) {
  m_flags = sm_Closed;

  auto* activeView = AeSysView::GetActiveView();

  auto planeNormal = activeView->CameraDirection();
  auto minorAxis = activeView->ViewUp();
  auto majorAxis = minorAxis;
  majorAxis.RotateAboutArbitraryAxis(planeNormal, -Eo::HalfPi);

  majorAxis *= radius;
  minorAxis *= radius;

  polyline::GeneratePointsForNPoly(centerPoint, majorAxis, minorAxis, numberOfSides, m_pts);
}

EoDbPolyline::EoDbPolyline(std::int16_t penColor, std::int16_t lineType, EoGePoint3dArray& pts)
    : EoDbPrimitive(penColor, lineType) {
  m_flags = 0;  // not closed
  m_pts.Copy(pts);
}
EoDbPolyline::EoDbPolyline(EoGePoint3dArray& pts) {
  m_color = renderState.Color();
  m_lineTypeIndex = renderState.LineTypeIndex();

  m_flags = 0;
  m_pts.Copy(pts);
}
EoDbPolyline::EoDbPolyline(const EoDbPolyline& other) {
  m_color = other.m_color;
  m_lineTypeIndex = other.m_lineTypeIndex;
  m_flags = other.m_flags;
  m_pts.Copy(other.m_pts);
  m_bulges = other.m_bulges;
  m_startWidths = other.m_startWidths;
  m_endWidths = other.m_endWidths;
  m_constantWidth = other.m_constantWidth;
}

const EoDbPolyline& EoDbPolyline::operator=(const EoDbPolyline& other) {
  if (this != &other) {
    m_color = other.m_color;
    m_lineTypeIndex = other.m_lineTypeIndex;
    m_flags = other.m_flags;
    m_pts.Copy(other.m_pts);
    m_bulges = other.m_bulges;
    m_startWidths = other.m_startWidths;
    m_endWidths = other.m_endWidths;
    m_constantWidth = other.m_constantWidth;
  }
  return (*this);
}

void EoDbPolyline::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<Polyline>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbPolyline::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbPolyline(*this);
  return primitive;
}

/** Display the polyline on the given device context within the specified view.
 *
 * When the polyline has bulge data, bulged segments are tessellated into arc
 * approximation points before passing to the polyline rendering pipeline.
 * The closing segment (for closed polylines) also respects its bulge value.
 *
 * @param view Pointer to the AeSysView where the polyline will be displayed.
 * @param deviceContext Pointer to the CDC device context for rendering.
 */
void EoDbPolyline::Display(AeSysView* view, CDC* deviceContext) {
  ATLTRACE2(traceGeneral, 3, L"EoDbPolyline::Display(%p, %p)\n", view, deviceContext);

  const auto numberOfVertices = m_pts.GetSize();
  if (numberOfVertices < 2) { return; }

  std::int16_t color = LogicalColor();
  std::int16_t lineType = LogicalLineType();

  renderState.SetPen(view, deviceContext, color, lineType);

  if (IsClosed()) {
    polyline::BeginLineLoop();
  } else {
    polyline::BeginLineStrip();
  }

  if (HasBulge()) {
    std::vector<EoGePoint3d> arcPoints;

    // Emit the first vertex
    polyline::SetVertex(m_pts[0]);

    // Emit intermediate and segment-end vertices for each edge
    for (INT_PTR i = 0; i < numberOfVertices - 1; ++i) {
      const double bulge = (static_cast<size_t>(i) < m_bulges.size()) ? m_bulges[static_cast<size_t>(i)] : 0.0;
      polyline::TessellateArcSegment(m_pts[i], m_pts[i + 1], bulge, arcPoints);
      for (const auto& point : arcPoints) { polyline::SetVertex(point); }
    }

    // For closed polylines, the closing segment (last → first) also carries a bulge
    if (IsClosed()) {
      const double closingBulge = (static_cast<size_t>(numberOfVertices - 1) < m_bulges.size())
                                      ? m_bulges[static_cast<size_t>(numberOfVertices - 1)]
                                      : 0.0;
      if (std::abs(closingBulge) >= Eo::geometricTolerance) {
        polyline::TessellateArcSegment(m_pts[numberOfVertices - 1], m_pts[0], closingBulge, arcPoints);
        // Emit all but the last point (which is m_pts[0], already handled by BeginLineLoop)
        for (size_t j = 0; j + 1 < arcPoints.size(); ++j) { polyline::SetVertex(arcPoints[j]); }
      }
    }
  } else {
    for (auto i = 0; i < numberOfVertices; i++) { polyline::SetVertex(m_pts[i]); }
  }
  polyline::__End(view, deviceContext, lineType);
}

void EoDbPolyline::ExportToDxf(EoDxfInterface* writer) const {
  const auto numberOfVertices = static_cast<size_t>(m_pts.GetSize());
  if (numberOfVertices == 0) { return; }

  if (Is3D()) {
    // 3D polyline → heavy POLYLINE with AcDb3dPolyline subclass and AcDb3dPolylineVertex vertices
    EoDxfPolyline dxfPolyline;
    PopulateDxfBaseProperties(&dxfPolyline);

    std::int16_t polylineFlag = 0x08;  // 3D polyline
    if (IsClosed()) { polylineFlag |= 0x01; }
    if (HasPlinegen()) { polylineFlag |= 0x80; }
    dxfPolyline.m_polylineFlag = polylineFlag;

    for (size_t i = 0; i < numberOfVertices; ++i) {
      const auto& point = m_pts[static_cast<INT_PTR>(i)];
      EoDxfVertex vertex{point.x, point.y, point.z, 0.0};
      vertex.m_vertexFlags = 0x20;  // AcDb3dPolylineVertex
      dxfPolyline.addVertex(vertex);
    }

    writer->AddPolyline(dxfPolyline);
    return;
  }

  EoDxfLwPolyline lwPolyline;
  PopulateDxfBaseProperties(&lwPolyline);

  lwPolyline.m_numberOfVertices = static_cast<std::int32_t>(numberOfVertices);

  std::int16_t polylineFlag = 0;
  if (IsClosed()) { polylineFlag |= 0x01; }
  if (HasPlinegen()) { polylineFlag |= 0x80; }
  lwPolyline.m_polylineFlag = polylineFlag;
  lwPolyline.m_constantWidth = m_constantWidth;
  lwPolyline.m_elevation = m_pts[0].z;

  lwPolyline.m_vertices.reserve(numberOfVertices);
  for (size_t i = 0; i < numberOfVertices; ++i) {
    EoDxfPolylineVertex2d vertex;
    vertex.x = m_pts[static_cast<INT_PTR>(i)].x;
    vertex.y = m_pts[static_cast<INT_PTR>(i)].y;
    vertex.z = m_pts[static_cast<INT_PTR>(i)].z;

    if (i < m_bulges.size()) { vertex.bulge = m_bulges[i]; }
    if (i < m_startWidths.size()) { vertex.stawidth = m_startWidths[i]; }
    if (i < m_endWidths.size()) { vertex.endwidth = m_endWidths[i]; }

    lwPolyline.m_vertices.push_back(vertex);
  }

  writer->AddLWPolyline(lwPolyline);
}

void EoDbPolyline::AddReportToMessageList(const EoGePoint3d& point) {
  auto numberOfVertices = std::uint16_t(m_pts.GetSize());
  if (sm_Edge == 0 || sm_Edge > numberOfVertices) { return; }
  int beginVertexIndex = static_cast<int>(sm_Edge) - 1;

  EoGePoint3d begin = m_pts[beginVertexIndex];
  EoGePoint3d end = m_pts[sm_Edge % numberOfVertices];

  if (sm_pivotVertex < numberOfVertices) {
    begin = m_pts[sm_pivotVertex];
    end = m_pts[SwingVertex()];
  }

  double edgeLength;
  double angleInXYPlane;

  // Check if this edge has a non-zero bulge
  const double bulge =
      (HasBulge() && static_cast<size_t>(beginVertexIndex) < m_bulges.size())
          ? m_bulges[static_cast<size_t>(beginVertexIndex)]
          : 0.0;

  if (std::abs(bulge) >= Eo::geometricTolerance) {
    // Arc segment: compute arc length = radius × |includedAngle|
    const double chordLength = EoGeVector3d(begin, end).Length();
    const double includedAngle = 4.0 * std::atan(std::abs(bulge));
    const double sinHalfAngle = std::sin(includedAngle / 2.0);
    if (std::abs(sinHalfAngle) > Eo::geometricTolerance) {
      const double radius = (chordLength / 2.0) / sinHalfAngle;
      edgeLength = radius * includedAngle;
    } else {
      edgeLength = chordLength;
    }
    // Report the chord direction angle (start→end baseline)
    angleInXYPlane = EoGeLine(begin, end).AngleFromXAxisXY();
  } else {
    edgeLength = EoGeVector3d(begin, end).Length();
    if (EoGeVector3d(point, begin).Length() > edgeLength * 0.5) {
      angleInXYPlane = EoGeLine(end, begin).AngleFromXAxisXY();
    } else {
      angleInXYPlane = EoGeLine(begin, end).AngleFromXAxisXY();
    }
  }

  CString lengthAsString;
  CString angleAsString;
  app.FormatLength(lengthAsString, app.GetUnits(), edgeLength);
  app.FormatAngle(angleAsString, angleInXYPlane, 8, 3);

  CString edgeType = (std::abs(bulge) >= Eo::geometricTolerance) ? L"Polyline Arc" : L"Polyline Edge";
  app.AddStringToMessageList(edgeType);
  EoDbPrimitive::AddReportToMessageList(point);
    
  CString message;
  message.Format(L"  %s @ %s", lengthAsString.TrimLeft().GetString(), angleAsString.GetString());
  app.AddStringToMessageList(message);

  app.SetEngagedLength(edgeLength);
  app.SetEngagedAngle(angleInXYPlane);

#if defined(USING_DDE)
  dde::PostAdvise(dde::EngLenInfo);
  dde::PostAdvise(dde::EngAngZInfo);
#endif
}
void EoDbPolyline::FormatGeometry(CString& str) {
  for (auto i = 0; i < m_pts.GetSize(); i++) {
    str += L"Point;" + m_pts[i].ToString();
    if (HasBulge() && i < static_cast<INT_PTR>(m_bulges.size())) {
      CString bulgeStr;
      bulgeStr.Format(L"\tBulge;%.6f", m_bulges[static_cast<size_t>(i)]);
      str += bulgeStr;
    }
  }
}

void EoDbPolyline::FormatExtra(CString& str) {
  EoDbPrimitive::FormatExtra(str);
  str.AppendFormat(L"\tPoints;%d\tClosed;%s\tBulge;%s\tWidth;%s", static_cast<int>(m_pts.GetSize()),
      IsClosed() ? L"Yes" : L"No", HasBulge() ? L"Yes" : L"No", HasWidth() ? L"Yes" : L"No");
  str += L'\t';
}

void EoDbPolyline::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Copy(m_pts);
}

EoGePoint3d EoDbPolyline::GetControlPoint() {
  int wPts = static_cast<int>(m_pts.GetSize());
  std::uint16_t wBeg = std::uint16_t(sm_Edge - 1);
  std::uint16_t wEnd = std::uint16_t(sm_Edge % wPts);
  EoGePoint3d pt = EoGeLine(m_pts[wBeg], m_pts[wEnd]).Midpoint();
  return pt;
}
void EoDbPolyline::GetExtents(
    AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3dArray tessellatedPoints;
  BuildTessellatedPoints(tessellatedPoints);

  for (auto i = 0; i < tessellatedPoints.GetSize(); i++) {
    EoGePoint3d pt = tessellatedPoints[i];
    view->ModelTransformPoint(pt);
    pt = transformMatrix * pt;
    ptMin = EoGePoint3d::Min(ptMin, pt);
    ptMax = EoGePoint3d::Max(ptMax, pt);
  }
}

EoGePoint3d EoDbPolyline::GoToNextControlPoint() {
  int wPts = static_cast<int>(m_pts.GetSize());

  if (sm_pivotVertex >= wPts) {  // have not yet rocked to a vertex
    std::uint16_t wBeg = std::uint16_t(sm_Edge - 1);
    std::uint16_t wEnd = std::uint16_t(sm_Edge % wPts);

    if (m_pts[wEnd].x > m_pts[wBeg].x) {
      sm_pivotVertex = wBeg;
    } else if (m_pts[wEnd].x < m_pts[wBeg].x) {
      sm_pivotVertex = wEnd;
    } else if (m_pts[wEnd].y > m_pts[wBeg].y) {
      sm_pivotVertex = wBeg;
    } else {
      sm_pivotVertex = wEnd;
    }
  } else if (sm_pivotVertex == 0) {
    if (sm_Edge == 1) {
      sm_pivotVertex = 1;
    } else {
      sm_pivotVertex = wPts - 1;
    }
  } else if (sm_pivotVertex == wPts - 1) {
    if (sm_Edge == wPts) {
      sm_pivotVertex = 0;
    } else {
      sm_pivotVertex--;
    }
  } else {
    if (sm_Edge == sm_pivotVertex) {
      sm_pivotVertex--;
    } else {
      sm_pivotVertex++;
    }
  }
  return (m_pts[sm_pivotVertex]);
}

bool EoDbPolyline::IsInView(AeSysView* view) {
  EoGePoint3dArray tessellatedPoints;
  BuildTessellatedPoints(tessellatedPoints);

  if (tessellatedPoints.GetSize() < 2) { return false; }

  EoGePoint4d pt[2]{};

  pt[0] = EoGePoint4d{tessellatedPoints[0]};
  view->ModelViewTransformPoint(pt[0]);

  for (INT_PTR i = 1; i < tessellatedPoints.GetSize(); i++) {
    pt[1] = EoGePoint4d{tessellatedPoints[i]};
    view->ModelViewTransformPoint(pt[1]);

    if (EoGePoint4d::ClipLine(pt[0], pt[1])) { return true; }

    pt[0] = pt[1];
  }
  return false;
}

bool EoDbPolyline::IsPointOnControlPoint([[maybe_unused]] AeSysView* view, [[maybe_unused]] const EoGePoint4d& point) {
  return false;
}

bool EoDbPolyline::PivotOnControlPoint(AeSysView* view, const EoGePoint4d& ptView) {
  std::uint16_t wPts = std::uint16_t(m_pts.GetSize());

  if (sm_pivotVertex >= wPts) {
    // Not engaged at a vertex
    return false;
  }

  EoGePoint4d ptCtrl(m_pts[sm_pivotVertex]);
  view->ModelViewTransformPoint(ptCtrl);

  if (ptCtrl.DistanceToPointXY(ptView) >= sm_SelectApertureSize) {
    // Not on proper vertex
    return false;
  }

  if (sm_pivotVertex == 0) {
    sm_Edge = std::uint16_t(sm_Edge == 1 ? wPts : 1);
  } else if (sm_pivotVertex == wPts - 1) {
    sm_Edge = std::uint16_t(sm_Edge == wPts ? sm_Edge - 1 : wPts);
  } else if (sm_pivotVertex == sm_Edge) {
    sm_Edge++;
  } else {
    sm_Edge--;
  }

  return true;
}
EoGePoint3d EoDbPolyline::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  std::uint16_t wPts = std::uint16_t(m_pts.GetSize());

  sm_controlPointIndex = SHRT_MAX;
  double dApert = sm_SelectApertureSize;

  sm_pivotVertex = wPts;

  for (auto i = 0; i < wPts; i++) {
    EoGePoint4d pt(m_pts[i]);
    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dApert) {
      sm_controlPointIndex = i;
      dApert = dDis;

      sm_Edge = std::uint16_t(i + 1);
      sm_pivotVertex = i;
    }
  }
  return (sm_controlPointIndex == SHRT_MAX) ? EoGePoint3d::kOrigin : m_pts[sm_controlPointIndex];
}

bool EoDbPolyline::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  const auto numberOfVertices = m_pts.GetSize();
  if (numberOfVertices < 2) { return false; }

  std::uint16_t wEdges = std::uint16_t(numberOfVertices);
  if (!IsClosed()) { wEdges--; }
  std::uint16_t wPts = std::uint16_t(numberOfVertices);

  if (HasBulge()) {
    std::vector<EoGePoint3d> arcPoints;

    for (std::uint16_t w = 1; w <= wEdges; w++) {
      const INT_PTR edgeStart = w - 1;
      const INT_PTR edgeEnd = w % wPts;
      const double bulge =
          (static_cast<size_t>(edgeStart) < m_bulges.size()) ? m_bulges[static_cast<size_t>(edgeStart)] : 0.0;

      polyline::TessellateArcSegment(m_pts[edgeStart], m_pts[edgeEnd], bulge, arcPoints);

      EoGePoint4d ptBeg(m_pts[edgeStart]);
      view->ModelViewTransformPoint(ptBeg);

      for (const auto& arcPoint : arcPoints) {
        EoGePoint4d ptEnd(arcPoint);
        view->ModelViewTransformPoint(ptEnd);

        EoGePoint3d intersection;
        if (EoGeLine::Intersection_xy(line, EoGeLine(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd}), intersection)) {
          double relation{};

          if (line.ComputeParametricRelation(intersection, relation)
              && relation >= -Eo::geometricTolerance && relation <= 1.0 + Eo::geometricTolerance) {
            if (EoGeLine(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd}).ComputeParametricRelation(intersection, relation)
                && relation >= -Eo::geometricTolerance && relation <= 1.0 + Eo::geometricTolerance) {
              intersection.z = ptBeg.z + relation * (ptEnd.z - ptBeg.z);
              intersections.Add(intersection);
            }
          }
        }
        ptBeg = ptEnd;
      }
    }
  } else {
    EoGePoint4d ptBeg(m_pts[0]);
    view->ModelViewTransformPoint(ptBeg);

    for (std::uint16_t w = 1; w <= wEdges; w++) {
      EoGePoint4d ptEnd(m_pts[w % wPts]);
      view->ModelViewTransformPoint(ptEnd);

      EoGePoint3d intersection;
      if (EoGeLine::Intersection_xy(line, EoGeLine(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd}), intersection)) {
        double relation{};

        if (line.ComputeParametricRelation(intersection, relation)
            && relation >= -Eo::geometricTolerance && relation <= 1.0 + Eo::geometricTolerance) {
          if (EoGeLine(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd}).ComputeParametricRelation(intersection, relation)
              && relation >= -Eo::geometricTolerance && relation <= 1.0 + Eo::geometricTolerance) {
            intersection.z = ptBeg.z + relation * (ptEnd.z - ptBeg.z);
            intersections.Add(intersection);
          }
        }
      }
      ptBeg = ptEnd;
    }
  }
  return !intersections.IsEmpty();
}

bool EoDbPolyline::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  std::uint16_t wPts = std::uint16_t(m_pts.GetSize());
  if (sm_EdgeToEvaluate > 0 && sm_EdgeToEvaluate <= wPts) {  // Evaluate specified edge of polyline
    const INT_PTR edgeStart = sm_EdgeToEvaluate - 1;
    const INT_PTR edgeEnd = sm_EdgeToEvaluate % wPts;

    if (HasBulge()) {
      // Tessellate the single specified edge and test each sub-segment
      const double bulge =
          (static_cast<size_t>(edgeStart) < m_bulges.size()) ? m_bulges[static_cast<size_t>(edgeStart)] : 0.0;
      std::vector<EoGePoint3d> arcPoints;
      polyline::TessellateArcSegment(m_pts[edgeStart], m_pts[edgeEnd], bulge, arcPoints);

      EoGePoint4d ptBeg(m_pts[edgeStart]);
      view->ModelViewTransformPoint(ptBeg);

      for (const auto& arcPoint : arcPoints) {
        EoGePoint4d ptEnd(arcPoint);
        view->ModelViewTransformPoint(ptEnd);

        EoGeLine lineSegment(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd});
        if (lineSegment.IsSelectedByPointXY(
                EoGePoint3d{point}, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
          ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
          return true;
        }
        ptBeg = ptEnd;
      }
    } else {
      EoGePoint4d ptBeg(m_pts[edgeStart]);
      EoGePoint4d ptEnd(m_pts[edgeEnd]);

      view->ModelViewTransformPoint(ptBeg);
      view->ModelViewTransformPoint(ptEnd);

      EoGeLine lineSegment(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd});
      if (lineSegment.IsSelectedByPointXY(
              EoGePoint3d{point}, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
        ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
        return true;
      }
    }
  } else {  // Evaluate entire polyline
    std::uint16_t wEdges = std::uint16_t(m_pts.GetSize());
    if (!IsClosed()) { wEdges--; }

    if (HasBulge()) {
      std::vector<EoGePoint3d> arcPoints;

      for (std::uint16_t w = 1; w <= wEdges; w++) {
        const INT_PTR edgeStart = w - 1;
        const INT_PTR edgeEnd = w % wPts;
        const double bulge =
            (static_cast<size_t>(edgeStart) < m_bulges.size()) ? m_bulges[static_cast<size_t>(edgeStart)] : 0.0;

        polyline::TessellateArcSegment(m_pts[edgeStart], m_pts[edgeEnd], bulge, arcPoints);

        EoGePoint4d ptBeg(m_pts[edgeStart]);
        view->ModelViewTransformPoint(ptBeg);

        for (const auto& arcPoint : arcPoints) {
          EoGePoint4d ptEnd(arcPoint);
          view->ModelViewTransformPoint(ptEnd);

          EoGeLine lineSegment(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd});
          if (lineSegment.IsSelectedByPointXY(
                  EoGePoint3d{point}, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
            ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
            sm_Edge = w;
            sm_pivotVertex = wPts;
            return true;
          }
          ptBeg = ptEnd;
        }
      }
    } else {
      EoGePoint4d ptBeg(m_pts[0]);
      view->ModelViewTransformPoint(ptBeg);

      for (std::uint16_t w = 1; w <= wEdges; w++) {
        EoGePoint4d ptEnd(m_pts[w % wPts]);
        view->ModelViewTransformPoint(ptEnd);

        EoGeLine lineSegment(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd});
        if (lineSegment.IsSelectedByPointXY(
                EoGePoint3d{point}, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
          ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
          sm_Edge = w;
          sm_pivotVertex = wPts;
          return true;
        }
        ptBeg = ptEnd;
      }
    }
  }
  return false;
}
bool EoDbPolyline::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  EoGePoint3dArray tessellatedPoints;
  BuildTessellatedPoints(tessellatedPoints);
  return polyline::SelectUsingRectangle(view, pt1, pt2, tessellatedPoints);
}
void EoDbPolyline::Transform(const EoGeTransformMatrix& transformMatrix) {
  for (auto i = 0; i < m_pts.GetSize(); i++) { m_pts[i] = transformMatrix * m_pts[i]; }
}
void EoDbPolyline::Translate(const EoGeVector3d& v) {
  for (auto i = 0; i < m_pts.GetSize(); i++) { m_pts[i] += v; }
}
void EoDbPolyline::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  for (auto i = 0; i < m_pts.GetSize(); i++) {
    if (((mask >> i) & 1UL) == 1) { m_pts[i] += v; }
  }
}
EoDbPolyline* EoDbPolyline::ReadFromPeg(CFile& file) {
  auto color = EoDb::ReadInt16(file);
  auto lineType = EoDb::ReadInt16(file);
  auto flags = static_cast<std::int16_t>(EoDb::ReadUInt16(file));
  auto constantWidth = EoDb::ReadDouble(file);
  auto numberOfVertices = EoDb::ReadUInt16(file);

  EoGePoint3dArray points;
  points.SetSize(numberOfVertices);

  for (std::uint16_t n = 0; n < numberOfVertices; n++) { points[n] = EoDb::ReadPoint3d(file); }

  auto* polyline = new EoDbPolyline(color, lineType, points);
  polyline->m_flags = flags;
  polyline->m_constantWidth = constantWidth;

  if (flags & sm_HasBulge) {
    std::vector<double> bulges(numberOfVertices);
    for (std::uint16_t n = 0; n < numberOfVertices; n++) { bulges[n] = EoDb::ReadDouble(file); }
    polyline->SetBulges(std::move(bulges));
  }
  if (flags & sm_HasWidth) {
    std::vector<double> startWidths(numberOfVertices);
    std::vector<double> endWidths(numberOfVertices);
    for (std::uint16_t n = 0; n < numberOfVertices; n++) { startWidths[n] = EoDb::ReadDouble(file); }
    for (std::uint16_t n = 0; n < numberOfVertices; n++) { endWidths[n] = EoDb::ReadDouble(file); }
    polyline->SetWidths(std::move(startWidths), std::move(endWidths));
  }

  return polyline;
}

EoDbPolyline* EoDbPolyline::ReadFromCSplinePeg(CFile& file) {
  auto color = EoDb::ReadInt16(file);
  auto lineType = EoDb::ReadInt16(file);

  file.Seek(sizeof(std::uint16_t), CFile::current);
  auto numberOfPoints = EoDb::ReadUInt16(file);
  file.Seek(sizeof(std::uint16_t), CFile::current);
  file.Seek(sizeof(EoGeVector3d), CFile::current);
  file.Seek(sizeof(EoGeVector3d), CFile::current);

  EoGePoint3dArray points;
  points.SetSize(numberOfPoints);
  for (std::uint16_t n = 0; n < numberOfPoints; n++) { points[n] = EoDb::ReadPoint3d(file); }

  return new EoDbPolyline(color, lineType, points);
}

bool EoDbPolyline::Write(CFile& file) {
  EoDb::WriteUInt16(file, std::uint16_t(EoDb::kPolylinePrimitive));
  EoDb::WriteInt16(file, m_color);
  EoDb::WriteInt16(file, m_lineTypeIndex);
  EoDb::WriteUInt16(file, static_cast<std::uint16_t>(m_flags));
  EoDb::WriteDouble(file, m_constantWidth);
  const auto numberOfVertices = static_cast<std::uint16_t>(m_pts.GetSize());
  EoDb::WriteUInt16(file, numberOfVertices);

  for (auto i = 0; i < m_pts.GetSize(); i++) { m_pts[i].Write(file); }

  if (HasBulge()) {
    for (size_t i = 0; i < m_bulges.size(); i++) { EoDb::WriteDouble(file, m_bulges[i]); }
  }
  if (HasWidth()) {
    for (size_t i = 0; i < m_startWidths.size(); i++) { EoDb::WriteDouble(file, m_startWidths[i]); }
    for (size_t i = 0; i < m_endWidths.size(); i++) { EoDb::WriteDouble(file, m_endWidths[i]); }
  }

  return true;
}

void EoDbPolyline::BuildTessellatedPoints(EoGePoint3dArray& tessellatedPoints) const {
  const auto numberOfVertices = m_pts.GetSize();
  if (numberOfVertices < 2) {
    tessellatedPoints.SetSize(0);
    if (numberOfVertices == 1) { tessellatedPoints.Add(m_pts[0]); }
    return;
  }

  // Estimate capacity: each vertex plus potential arc points
  tessellatedPoints.SetSize(0);

  if (HasBulge()) {
    std::vector<EoGePoint3d> arcPoints;

    tessellatedPoints.Add(m_pts[0]);

    for (INT_PTR i = 0; i < numberOfVertices - 1; ++i) {
      const double bulge = (static_cast<size_t>(i) < m_bulges.size()) ? m_bulges[static_cast<size_t>(i)] : 0.0;
      polyline::TessellateArcSegment(m_pts[i], m_pts[i + 1], bulge, arcPoints);
      for (const auto& point : arcPoints) { tessellatedPoints.Add(point); }
    }

    if (IsClosed()) {
      const double closingBulge = (static_cast<size_t>(numberOfVertices - 1) < m_bulges.size())
                                      ? m_bulges[static_cast<size_t>(numberOfVertices - 1)]
                                      : 0.0;
      polyline::TessellateArcSegment(m_pts[numberOfVertices - 1], m_pts[0], closingBulge, arcPoints);
      for (const auto& point : arcPoints) { tessellatedPoints.Add(point); }
    }
  } else {
    for (auto i = 0; i < numberOfVertices; i++) { tessellatedPoints.Add(m_pts[i]); }
    if (IsClosed()) { tessellatedPoints.Add(m_pts[0]); }
  }
}

std::uint16_t EoDbPolyline::SwingVertex() {
  std::uint16_t wPts = std::uint16_t(m_pts.GetSize());

  std::uint16_t wSwingVertex;

  if (sm_pivotVertex == 0) {
    wSwingVertex = std::uint16_t(sm_Edge == 1 ? 1 : wPts - 1);
  } else if (sm_pivotVertex == std::uint16_t(wPts - 1)) {
    wSwingVertex = std::uint16_t(sm_Edge == wPts ? 0 : sm_pivotVertex - 1);
  } else {
    wSwingVertex = std::uint16_t(sm_Edge == sm_pivotVertex ? sm_pivotVertex - 1 : sm_pivotVertex + 1);
  }

  return wSwingVertex;
}
