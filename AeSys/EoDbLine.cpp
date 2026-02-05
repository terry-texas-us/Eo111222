#include "Stdafx.h"

#include <climits>
#include <cmath>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "PrimState.h"
#include "drw_base.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif  // USING_DDE

EoDbLine::EoDbLine(const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"EoDbLine(points) CTOR: this=%p\n", this);
  m_color = pstate.PenColor();
  m_lineTypeIndex = pstate.LineType();
  m_ln(beginPoint, endPoint);
}
EoDbLine::EoDbLine(EoGeLine& line) : m_ln(line) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"EoDbLine(EoGeLine) CTOR: this=%p\n", this);
  m_color = pstate.PenColor();
  m_lineTypeIndex = pstate.LineType();
}
EoDbLine::EoDbLine(EoInt16 color, EoInt16 lineType, EoGeLine line) : EoDbPrimitive(color, lineType), m_ln(line) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"EoDbLine(color,lt,line) CTOR: this=%p\n", this);
}

EoDbLine::EoDbLine(EoInt16 color, EoInt16 lineType, const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint)
    : EoDbPrimitive(color, lineType), m_ln(beginPoint, endPoint) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"EoDbLine(color,lt,pts) CTOR: this=%p\n", this);
}

EoDbLine::EoDbLine(const DRW_Coord& beginPoint, const DRW_Coord& endPoint) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"EoDbLine(DRW_Coord) CTOR: this=%p\n", this);
  m_ln.begin(beginPoint.x, beginPoint.y, beginPoint.z);
  m_ln.end(endPoint.x, endPoint.y, endPoint.z);
}

EoDbLine::EoDbLine(const EoDbLine& other) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"EoDbLine COPY CTOR: this=%p, from=%p\n", this, &other);
  m_color = other.m_color;
  m_lineTypeIndex = other.m_lineTypeIndex;
  m_ln = other.m_ln;
}

const EoDbLine& EoDbLine::operator=(const EoDbLine& other) {
  m_color = other.m_color;
  m_lineTypeIndex = other.m_lineTypeIndex;
  m_ln = other.m_ln;

  return (*this);
}
void EoDbLine::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<Line>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbLine::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbLine(*this);
  return (primitive);
}

/** @brief Cuts a line at two points.
 * @param points Pointer to two points for the line cut.
 * @param groups Group list to receive the original line segments.
 * @param newGroups Group list to receive the new line segments.
 * @note Line segment between two points goes in groups.
 */
void EoDbLine::CutAt2Points(const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList* groups,
                         EoDbGroupList* newGroups) {
  EoDbLine* line{};
  double dRel[2]{};

  m_ln.RelOfPtToEndPts(firstPoint, dRel[0]);
  m_ln.RelOfPtToEndPts(secondPoint, dRel[1]);

  if (dRel[0] < Eo::geometricTolerance && dRel[1] >= 1.0 - Eo::geometricTolerance) {
    // The two points effectively cover the whole line. No cutting. Put entire line in trap.
    line = this;
  } else {  // Something gets cut
    line = new EoDbLine(*this);
    if (dRel[0] > Eo::geometricTolerance && dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut section out of middle
      line->BeginPoint(secondPoint);
      groups->AddTail(new EoDbGroup(line));
      line = new EoDbLine(*this);
      line->BeginPoint(firstPoint);
      line->EndPoint(secondPoint);
      EndPoint(firstPoint);
    } else if (dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut in two and place begin section in trap
      line->EndPoint(secondPoint);
      BeginPoint(secondPoint);
    } else {  // Cut in two and place end section in trap
      line->BeginPoint(firstPoint);
      EndPoint(firstPoint);
    }
    groups->AddTail(new EoDbGroup(this));
  }
  newGroups->AddTail(new EoDbGroup(line));
}

void EoDbLine::CutAtPoint(EoGePoint3d& point, EoDbGroup* group) {
  EoGeLine line;

  if (m_ln.CutAtPt(point, line) != 0) { group->AddTail(new EoDbLine(m_color, m_lineTypeIndex, line)); }
}

void EoDbLine::Display(AeSysView* view, CDC* deviceContext) {
  EoInt16 color = LogicalColor();
  EoInt16 lineType = LogicalLineType();

  pstate.SetPen(view, deviceContext, color, lineType);

  polyline::BeginLineStrip();
  polyline::SetVertex(m_ln.begin);
  polyline::SetVertex(m_ln.end);
  polyline::__End(view, deviceContext, lineType);
}
void EoDbLine::AddReportToMessageList(EoGePoint3d pt) {
  double dLen = Length();
  double AngleInXYPlane = m_ln.AngleFromXAxisXY();

  double dRel;
  m_ln.RelOfPtToEndPts(pt, dRel);

  if (dRel > 0.5) { AngleInXYPlane += Eo::Pi; }
  AngleInXYPlane = fmod(AngleInXYPlane, Eo::TwoPi);

  CString LengthAsString;
  CString AngleAsString;
  app.FormatLength(LengthAsString, app.GetUnits(), dLen);
  app.FormatAngle(AngleAsString, AngleInXYPlane, 8, 3);

  CString Message;
  Message.Format(L"<Line> Color: %s Line Type: %s \u2022 %s @ %s", FormatPenColor().GetString(),
                 FormatLineType().GetString(), LengthAsString.TrimLeft().GetString(), AngleAsString.GetString());
  app.AddStringToMessageList(Message);

  app.SetEngagedLength(dLen);
  app.SetEngagedAngle(AngleInXYPlane);

#if defined(USING_DDE)
  dde::PostAdvise(dde::EngLenInfo);
  dde::PostAdvise(dde::EngAngZInfo);
#endif  // USING_DDE
}
void EoDbLine::FormatExtra(CString& str) {
  CString FormattedLength;
  app.FormatLength(FormattedLength, app.GetUnits(), Length());

  str.Format(L"Color;%s\tStyle;%s\tLength;%s\tZ-Angle;%f", FormatPenColor().GetString(), FormatLineType().GetString(),
             FormattedLength.TrimLeft().GetString(), Eo::RadianToDegree(m_ln.AngleFromXAxisXY()));
}
void EoDbLine::FormatGeometry(CString& str) {
  str += L"Begin Point;" + m_ln.begin.ToString();
  str += L"End Point;" + m_ln.end.ToString();
}
void EoDbLine::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_ln.begin);
  points.Add(m_ln.end);
}
void EoDbLine::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  EoGePoint3d pt[2]{};

  GetPts(pt[0], pt[1]);

  for (EoUInt16 w = 0; w < 2; w++) {
    view->ModelTransformPoint(pt[w]);
    pt[w] = tm * pt[w];
    ptMin = EoGePoint3d::Min(ptMin, pt[w]);
    ptMax = EoGePoint3d::Max(ptMax, pt[w]);
  }
}
EoGePoint3d EoDbLine::GoToNextControlPoint() {
  if (sm_ControlPointIndex == 0)
    sm_ControlPointIndex = 1;
  else if (sm_ControlPointIndex == 1)
    sm_ControlPointIndex = 0;
  else {  // Initial rock .. jump to point at lower left or down if vertical
    EoGePoint3d ptBeg = BeginPoint();
    EoGePoint3d ptEnd = EndPoint();

    if (ptEnd.x > ptBeg.x)
      sm_ControlPointIndex = 0;
    else if (ptEnd.x < ptBeg.x)
      sm_ControlPointIndex = 1;
    else if (ptEnd.y > ptBeg.y)
      sm_ControlPointIndex = 0;
    else
      sm_ControlPointIndex = 1;
  }
  return (sm_ControlPointIndex == 0 ? BeginPoint() : EndPoint());
}
bool EoDbLine::Identical(EoDbPrimitive* primitive) { return m_ln == static_cast<EoDbLine*>(primitive)->Ln(); }
bool EoDbLine::IsInView(AeSysView* view) {
  EoGePoint4d pt[] = {EoGePoint4d(m_ln.begin), EoGePoint4d(m_ln.end)};
  view->ModelViewTransformPoints(2, &pt[0]);

  return (EoGePoint4d::ClipLine(pt[0], pt[1]));
}
bool EoDbLine::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d pt;

  pt = m_ln.begin;
  view->ModelViewTransformPoint(pt);

  if (point.DistanceToPointXY(pt) < sm_SelectApertureSize) return true;

  pt = m_ln.end;
  view->ModelViewTransformPoint(pt);

  if (point.DistanceToPointXY(pt) < sm_SelectApertureSize) return true;

  return false;
}

/** @brief Clips line to area defined by two points.
 * @param ptLL Lower left point of area.
 * @param ptUR Upper right point of area.
 * @param ptInt Array of two points to receive clipped line.
 * @return
 *		0 - line is completely outside area
 *		2 - line is completely or partially within area (ptInt contains clipped line)
 */
int EoDbLine::IsWithinArea(EoGePoint3d ptLL, EoGePoint3d ptUR, EoGePoint3d* ptInt) {
  int i;
  int iLoc[2]{};

  GetPts(ptInt[0], ptInt[1]);

  for (i = 0; i < 2; i++) { iLoc[i] = ptInt[i].RelationshipToRectangle(ptLL, ptUR); }
  while (iLoc[0] != 0 || iLoc[1] != 0) {
    if ((iLoc[0] & iLoc[1]) != 0) return 0;

    i = (iLoc[0] != 0) ? 0 : 1;
    if ((iLoc[i] & 1) != 0) {  // Clip against top
      ptInt[i].x = ptInt[i].x + (ptInt[1].x - ptInt[0].x) * (ptUR.y - ptInt[i].y) / (ptInt[1].y - ptInt[0].y);
      ptInt[i].y = ptUR.y;
    } else if ((iLoc[i] & 2) != 0) {  // Clip against bottom
      ptInt[i].x = ptInt[i].x + (ptInt[1].x - ptInt[0].x) * (ptLL.y - ptInt[i].y) / (ptInt[1].y - ptInt[0].y);
      ptInt[i].y = ptLL.y;
    } else if ((iLoc[i] & 4) != 0) {  // Clip against right
      ptInt[i].y = ptInt[i].y + (ptInt[1].y - ptInt[0].y) * (ptUR.x - ptInt[i].x) / (ptInt[1].x - ptInt[0].x);
      ptInt[i].x = ptUR.x;
    } else if ((iLoc[i] & 8) != 0) {  // Clip against left
      ptInt[i].y = ptInt[i].y + (ptInt[1].y - ptInt[0].y) * (ptLL.x - ptInt[i].x) / (ptInt[1].x - ptInt[0].x);
      ptInt[i].x = ptLL.x;
    }
    iLoc[i] = ptInt[i].RelationshipToRectangle(ptLL, ptUR);
  }
  return (2);
}
double EoDbLine::RelOfPt(EoGePoint3d pt) {
  double dRel;
  m_ln.RelOfPtToEndPts(pt, dRel);
  return dRel;
}
EoGePoint3d EoDbLine::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_ControlPointIndex = USHRT_MAX;

  double dApert = sm_SelectApertureSize;

  for (EoUInt16 w = 0; w < 2; w++) {
    EoGePoint4d pt(m_ln[w]);
    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dApert) {
      sm_ControlPointIndex = w;
      dApert = dDis;
    }
  }
  return (sm_ControlPointIndex == USHRT_MAX) ? EoGePoint3d::kOrigin : m_ln[sm_ControlPointIndex];
}
/** @brief Evaluates whether a line intersects line.
 * @param view Current view.
 * @param line Line to test against.
 * @param intersections Array to receive intersection points.
 * @return true if intersection occurs, false otherwise.
 */
bool EoDbLine::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_ln.begin);
  polyline::SetVertex(m_ln.end);
  return polyline::SelectUsingLine(view, line, intersections);
}
bool EoDbLine::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_ln.begin);
  polyline::SetVertex(m_ln.end);
  return polyline::SelectUsingPoint(view, point, sm_RelationshipOfPoint, ptProj);
}
bool EoDbLine::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_ln.begin);
  polyline::SetVertex(m_ln.end);
  return polyline::SelectUsingRectangle(view, pt1, pt2);
}
void EoDbLine::Square(AeSysView* view) {
  EoGePoint3d ptBeg = view->SnapPointToGrid(BeginPoint());
  EoGePoint3d ptEnd = view->SnapPointToGrid(EndPoint());

  EoGePoint3d pt = EoGeLine(ptBeg, ptEnd).Midpoint();
  double dLen = EoGeVector3d(ptBeg, ptEnd).Length();
  ptEnd = view->SnapPointToAxis(pt, ptEnd);
  BeginPoint(ptEnd.ProjectToward(pt, dLen));
  EndPoint(ptEnd);
}

void EoDbLine::Transform(EoGeTransformMatrix& transformMatrix) {
  BeginPoint(transformMatrix * BeginPoint());
  EndPoint(transformMatrix * EndPoint());
}

void EoDbLine::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if ((mask & 1) == 1) BeginPoint(BeginPoint() + v);

  if ((mask & 2) == 2) EndPoint(EndPoint() + v);
}
bool EoDbLine::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kLinePrimitive));

  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  m_ln.Write(file);

  return true;
}
