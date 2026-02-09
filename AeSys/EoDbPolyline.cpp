#include "Stdafx.h"

#include <climits>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "PrimState.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif  // USING_DDE

EoUInt16 EoDbPolyline::sm_EdgeToEvaluate = 0;
EoUInt16 EoDbPolyline::sm_Edge = 0;
EoUInt16 EoDbPolyline::sm_PivotVertex = 0;

EoDbPolyline::EoDbPolyline() { m_flags = 0; }

/** Constructs a closed polyline approximating a regular polygon.
 *
 * @param penColor The pen color for the polyline.
 * @param lineType The line type for the polyline.
 * @param centerPoint The center point of the polygon.
 * @param radius The radius of the circumscribed circle.
 * @param numberOfSides The number of sides for the polygon.
 */
EoDbPolyline::EoDbPolyline(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, double radius,
                           int numberOfSides)
    : EoDbPrimitive(penColor, lineType) {
  m_flags = 0x0010;  // polyline is closed

  auto* activeView = AeSysView::GetActiveView();

  auto planeNormal = activeView->CameraDirection();
  auto minorAxis = activeView->ViewUp();
  auto majorAxis = minorAxis;
  majorAxis.RotAboutArbAx(planeNormal, -Eo::HalfPi);

  majorAxis *= radius;
  minorAxis *= radius;

  polyline::GeneratePointsForNPoly(centerPoint, majorAxis, minorAxis, numberOfSides, m_pts);
}

EoDbPolyline::EoDbPolyline(EoInt16 penColor, EoInt16 lineType, EoGePoint3dArray& pts)
    : EoDbPrimitive(penColor, lineType) {
  m_flags = 0;  // not closed
  m_pts.Copy(pts);
}
EoDbPolyline::EoDbPolyline(EoGePoint3dArray& pts) {
  m_color = pstate.Color();
  m_lineTypeIndex = pstate.LineType();

  m_flags = 0;
  m_pts.Copy(pts);
}
EoDbPolyline::EoDbPolyline(const EoDbPolyline& other) {
  m_color = other.m_color;
  m_lineTypeIndex = other.m_lineTypeIndex;
  m_flags = other.m_flags;
  m_pts.Copy(other.m_pts);
}

const EoDbPolyline& EoDbPolyline::operator=(const EoDbPolyline& other) {
  m_color = other.m_color;
  m_lineTypeIndex = other.m_lineTypeIndex;
  m_flags = other.m_flags;
  m_pts.Copy(other.m_pts);
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
 * @param view Pointer to the AeSysView where the polyline will be displayed.
 * @param deviceContext Pointer to the CDC device context for rendering.
 */
void EoDbPolyline::Display(AeSysView* view, CDC* deviceContext) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"EoDbPolyline::Display(%p, %p)\n", view, deviceContext);

  EoInt16 color = LogicalColor();
  EoInt16 lineType = LogicalLineType();

  pstate.SetPen(view, deviceContext, color, lineType);

  if (IsLooped()) {
    polyline::BeginLineLoop();
  } else {
    polyline::BeginLineStrip();
  }
  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) { polyline::SetVertex(m_pts[w]); }
  polyline::__End(view, deviceContext, lineType);
}

void EoDbPolyline::AddReportToMessageList(EoGePoint3d ptPic) {
  auto numberOfVertices = EoUInt16(m_pts.GetSize());
  if (sm_Edge == 0 || sm_Edge > numberOfVertices) { return; }

  int beginVertexIndex = static_cast<int>(sm_Edge) - 1;

  EoGePoint3d begin = m_pts[beginVertexIndex];
  EoGePoint3d end = m_pts[sm_Edge % numberOfVertices];

  if (sm_PivotVertex < numberOfVertices) {
    begin = m_pts[sm_PivotVertex];
    end = m_pts[SwingVertex()];
  }
  double angleInXYPlane;
  double edgeLength = EoGeVector3d(begin, end).Length();

  if (EoGeVector3d(ptPic, begin).Length() > edgeLength * 0.5) {
    angleInXYPlane = EoGeLine(end, begin).AngleFromXAxisXY();
  } else {
    angleInXYPlane = EoGeLine(begin, end).AngleFromXAxisXY();
  }
  CString lengthAsString;
  CString angleAsString;
  app.FormatLength(lengthAsString, app.GetUnits(), edgeLength);
  app.FormatAngle(angleAsString, angleInXYPlane, 8, 3);

  CString message;
  message.Format(L"<Polyline Edge> Color: %s Line Type: %s \u2022 %s @ %s", FormatPenColor().GetString(),
                 FormatLineType().GetString(), lengthAsString.TrimLeft().GetString(), angleAsString.GetString());
  app.AddStringToMessageList(message);

  app.SetEngagedLength(edgeLength);
  app.SetEngagedAngle(angleInXYPlane);

#if defined(USING_DDE)
  dde::PostAdvise(dde::EngLenInfo);
  dde::PostAdvise(dde::EngAngZInfo);
#endif  // USING_DDE
}
void EoDbPolyline::FormatGeometry(CString& str) {
  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) { str += L"Point;" + m_pts[w].ToString(); }
}
void EoDbPolyline::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s\tPoints;%d", FormatPenColor().GetString(), FormatLineType().GetString(),
             static_cast<int>(m_pts.GetSize()));
}

void EoDbPolyline::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Copy(m_pts);
}

EoGePoint3d EoDbPolyline::GetControlPoint() {
  EoUInt16 wPts = EoUInt16(m_pts.GetSize());
  EoUInt16 wBeg = EoUInt16(sm_Edge - 1);
  EoUInt16 wEnd = EoUInt16(sm_Edge % wPts);
  EoGePoint3d pt = EoGeLine(m_pts[wBeg], m_pts[wEnd]).Midpoint();
  return pt;
}
void EoDbPolyline::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3d pt;

  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) {
    pt = m_pts[w];
    view->ModelTransformPoint(pt);
    pt = transformMatrix * pt;
    ptMin = EoGePoint3d::Min(ptMin, pt);
    ptMax = EoGePoint3d::Max(ptMax, pt);
  }
}
EoGePoint3d EoDbPolyline::GoToNextControlPoint() {
  EoUInt16 wPts = EoUInt16(m_pts.GetSize());

  if (sm_PivotVertex >= wPts) {  // have not yet rocked to a vertex
    EoUInt16 wBeg = EoUInt16(sm_Edge - 1);
    EoUInt16 wEnd = EoUInt16(sm_Edge % wPts);

    if (m_pts[wEnd].x > m_pts[wBeg].x)
      sm_PivotVertex = wBeg;
    else if (m_pts[wEnd].x < m_pts[wBeg].x)
      sm_PivotVertex = wEnd;
    else if (m_pts[wEnd].y > m_pts[wBeg].y)
      sm_PivotVertex = wBeg;
    else
      sm_PivotVertex = wEnd;
  } else if (sm_PivotVertex == 0) {
    if (sm_Edge == 1)
      sm_PivotVertex = 1;
    else
      sm_PivotVertex = wPts - 1U;
  } else if (sm_PivotVertex == wPts - 1) {
    if (sm_Edge == wPts)
      sm_PivotVertex = 0;
    else
      sm_PivotVertex--;
  } else {
    if (sm_Edge == sm_PivotVertex)
      sm_PivotVertex--;
    else
      sm_PivotVertex++;
  }
  return (m_pts[sm_PivotVertex]);
}
bool EoDbPolyline::IsInView(AeSysView* view) {
  EoGePoint4d pt[2]{};

  pt[0] = m_pts[0];
  view->ModelViewTransformPoint(pt[0]);

  for (EoUInt16 w = 1; w < m_pts.GetSize(); w++) {
    pt[1] = m_pts[w];
    view->ModelViewTransformPoint(pt[1]);

    if (EoGePoint4d::ClipLine(pt[0], pt[1])) { return true; }

    pt[0] = pt[1];
  }
  return false;
}

bool EoDbPolyline::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  (void)view;
  (void)point;
  return false;
}

bool EoDbPolyline::PivotOnControlPoint(AeSysView* view, const EoGePoint4d& ptView) {
  EoUInt16 wPts = EoUInt16(m_pts.GetSize());

  if (sm_PivotVertex >= wPts)
    // Not engaged at a vertex
    return false;

  EoGePoint4d ptCtrl(m_pts[sm_PivotVertex]);
  view->ModelViewTransformPoint(ptCtrl);

  if (ptCtrl.DistanceToPointXY(ptView) >= sm_SelectApertureSize)
    // Not on proper vertex
    return false;

  if (sm_PivotVertex == 0)
    sm_Edge = EoUInt16(sm_Edge == 1 ? wPts : 1);
  else if (sm_PivotVertex == wPts - 1)
    sm_Edge = EoUInt16(sm_Edge == wPts ? sm_Edge - 1 : wPts);
  else if (sm_PivotVertex == sm_Edge)
    sm_Edge++;
  else
    sm_Edge--;

  return true;
}
EoGePoint3d EoDbPolyline::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoUInt16 wPts = EoUInt16(m_pts.GetSize());

  sm_ControlPointIndex = USHRT_MAX;
  double dApert = sm_SelectApertureSize;

  sm_PivotVertex = wPts;

  for (EoUInt16 w = 0; w < wPts; w++) {
    EoGePoint4d pt(m_pts[w]);
    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dApert) {
      sm_ControlPointIndex = w;
      dApert = dDis;

      sm_Edge = EoUInt16(w + 1);
      sm_PivotVertex = w;
    }
  }
  return (sm_ControlPointIndex == USHRT_MAX) ? EoGePoint3d::kOrigin : m_pts[sm_ControlPointIndex];
}

bool EoDbPolyline::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) {
  (void)view;
  (void)line;
  return false;
}

bool EoDbPolyline::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  EoUInt16 wPts = EoUInt16(m_pts.GetSize());
  if (sm_EdgeToEvaluate > 0 && sm_EdgeToEvaluate <= wPts) {  // Evaluate specified edge of polyline
    EoGePoint4d ptBeg(m_pts[sm_EdgeToEvaluate - 1]);
    EoGePoint4d ptEnd(m_pts[sm_EdgeToEvaluate % wPts]);

    view->ModelViewTransformPoint(ptBeg);
    view->ModelViewTransformPoint(ptEnd);

    EoGeLine LineSegment(ptBeg, ptEnd);
    if (LineSegment.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
      ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
      return true;
    }
  } else {  // Evaluate entire polyline
    EoUInt16 wEdges = EoUInt16(m_pts.GetSize());
    if (!IsLooped()) wEdges--;

    EoGePoint4d ptBeg(m_pts[0]);
    view->ModelViewTransformPoint(ptBeg);

    for (EoUInt16 w = 1; w <= wEdges; w++) {
      EoGePoint4d ptEnd(m_pts[w % wPts]);
      view->ModelViewTransformPoint(ptEnd);

      EoGeLine LineSegment(ptBeg, ptEnd);
      if (LineSegment.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
        ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
        sm_Edge = w;
        sm_PivotVertex = wPts;
        return true;
      }
      ptBeg = ptEnd;
    }
  }
  return false;
}
bool EoDbPolyline::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  return polyline::SelectUsingRectangle(view, pt1, pt2, m_pts);
}
void EoDbPolyline::Transform(const EoGeTransformMatrix& transformMatrix) {
  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) m_pts[w] = transformMatrix * m_pts[w];
}
void EoDbPolyline::Translate(const EoGeVector3d& v) {
  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) m_pts[w] += v;
}
void EoDbPolyline::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++)
    if (((mask >> w) & 1UL) == 1) m_pts[w] += v;
}
bool EoDbPolyline::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kPolylinePrimitive));
  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  EoDb::Write(file, EoUInt16(m_pts.GetSize()));

  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) m_pts[w].Write(file);

  return true;
}
EoUInt16 EoDbPolyline::SwingVertex() {
  EoUInt16 wPts = EoUInt16(m_pts.GetSize());

  EoUInt16 wSwingVertex;

  if (sm_PivotVertex == 0)
    wSwingVertex = EoUInt16(sm_Edge == 1 ? 1 : wPts - 1);
  else if (sm_PivotVertex == EoUInt16(wPts - 1))
    wSwingVertex = EoUInt16(sm_Edge == wPts ? 0 : sm_PivotVertex - 1);
  else
    wSwingVertex = EoUInt16(sm_Edge == sm_PivotVertex ? sm_PivotVertex - 1 : sm_PivotVertex + 1);

  return wSwingVertex;
}
