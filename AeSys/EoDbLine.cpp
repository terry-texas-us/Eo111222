#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

#include "ddeGItms.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

EoDbLine::EoDbLine(const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint) {
  m_PenColor = pstate.PenColor();
  m_LineType = pstate.LineType();
  m_ln(beginPoint, endPoint);
}
EoDbLine::EoDbLine(EoGeLine& line) : m_ln(line) {
  m_PenColor = pstate.PenColor();
  m_LineType = pstate.LineType();
}
EoDbLine::EoDbLine(EoInt16 penColor, EoInt16 lineType, EoGeLine line) : EoDbPrimitive(penColor, lineType), m_ln(line) {}
EoDbLine::EoDbLine(EoInt16 penColor, EoInt16 lineType, const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint)
    : EoDbPrimitive(penColor, lineType), m_ln(beginPoint, endPoint) {}
#if defined(USING_ODA)
EoDbLine::EoDbLine(const OdGePoint3d& beginPoint, const OdGePoint3d& endPoint) {
  m_ln.begin(beginPoint.x, beginPoint.y, beginPoint.z);
  m_ln.end(endPoint.x, endPoint.y, endPoint.z);
}
#endif  // USING_ODA
EoDbLine::EoDbLine(const EoDbLine& src) {
  m_PenColor = src.m_PenColor;
  m_LineType = src.m_LineType;
  m_ln = src.m_ln;
}
const EoDbLine& EoDbLine::operator=(const EoDbLine& src) {
  m_PenColor = src.m_PenColor;
  m_LineType = src.m_LineType;
  m_ln = src.m_ln;

  return (*this);
}
void EoDbLine::AddToTreeViewControl(HWND tree, HTREEITEM parent) { tvAddItem(tree, parent, const_cast<LPWSTR>(L"<Line>"), this); }
EoDbPrimitive*& EoDbLine::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbLine(*this);
  return (primitive);
}
void EoDbLine::CutAt2Pts(EoGePoint3d* pt, EoDbGroupList* groups, EoDbGroupList* newGroups) {
  EoDbLine* pLine;
  double dRel[2];

  m_ln.RelOfPtToEndPts(pt[0], dRel[0]);
  m_ln.RelOfPtToEndPts(pt[1], dRel[1]);

  if (dRel[0] <= DBL_EPSILON && dRel[1] >= 1. - DBL_EPSILON) {  // Put entire line in trap
    pLine = this;
  } else {  // Something gets cut
    pLine = new EoDbLine(*this);
    if (dRel[0] > DBL_EPSILON && dRel[1] < 1. - DBL_EPSILON) {  // Cut section out of middle
      pLine->BeginPoint(pt[1]);
      groups->AddTail(new EoDbGroup(pLine));

      pLine = new EoDbLine(*this);
      pLine->BeginPoint(pt[0]);
      pLine->EndPoint(pt[1]);
      EndPoint(pt[0]);
    } else if (dRel[1] < 1. - DBL_EPSILON) {  // Cut in two and place begin section in trap
      pLine->EndPoint(pt[1]);
      BeginPoint(pt[1]);
    } else {  // Cut in two and place end section in trap
      pLine->BeginPoint(pt[0]);
      EndPoint(pt[0]);
    }
    groups->AddTail(new EoDbGroup(this));
  }
  newGroups->AddTail(new EoDbGroup(pLine));
}
void EoDbLine::CutAtPt(EoGePoint3d& pt, EoDbGroup* group) {
  EoGeLine ln;

  if (m_ln.CutAtPt(pt, ln) != 0) group->AddTail(new EoDbLine(m_PenColor, m_LineType, ln));
}
void EoDbLine::Display(AeSysView* view, CDC* deviceContext) {
  EoInt16 PenColor = LogicalPenColor();
  EoInt16 LineType = LogicalLineType();

  pstate.SetPen(view, deviceContext, PenColor, LineType);

  polyline::BeginLineStrip();
  polyline::SetVertex(m_ln.begin);
  polyline::SetVertex(m_ln.end);
  polyline::__End(view, deviceContext, LineType);
}
void EoDbLine::AddReportToMessageList(EoGePoint3d pt) {
  double dLen = Length();
  double AngleInXYPlane = m_ln.AngleFromXAxisXY();

  double dRel;
  m_ln.RelOfPtToEndPts(pt, dRel);

  if (dRel > 0.5) { AngleInXYPlane += PI; }
  AngleInXYPlane = fmod(AngleInXYPlane, TWOPI);

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
             FormattedLength.TrimLeft().GetString(), EoToDegree(m_ln.AngleFromXAxisXY()));
}
void EoDbLine::FormatGeometry(CString& str) {
  str += L"Begin Point;" + m_ln.begin.ToString();
  str += L"End Point;" + m_ln.end.ToString();
}
void EoDbLine::GetAllPts(EoGePoint3dArray& pts) {
  pts.SetSize(0);
  pts.Add(m_ln.begin);
  pts.Add(m_ln.end);
}
void EoDbLine::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  EoGePoint3d pt[2];

  GetPts(pt[0], pt[1]);

  for (EoUInt16 w = 0; w < 2; w++) {
    view->ModelTransformPoint(pt[w]);
    pt[w] = tm * pt[w];
    ptMin = EoGePoint3d::Min(ptMin, pt[w]);
    ptMax = EoGePoint3d::Max(ptMax, pt[w]);
  }
}
EoGePoint3d EoDbLine::GoToNxtCtrlPt() {
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
int EoDbLine::IsWithinArea(EoGePoint3d ptLL, EoGePoint3d ptUR, EoGePoint3d* ptInt) {
  int i;
  int iLoc[2];

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
bool EoDbLine::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptsInt) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_ln.begin);
  polyline::SetVertex(m_ln.end);
  return polyline::SelectUsingLine(view, line, ptsInt);
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
void EoDbLine::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if ((mask & 1) == 1) BeginPoint(BeginPoint() + v);

  if ((mask & 2) == 2) EndPoint(EndPoint() + v);
}
bool EoDbLine::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kLinePrimitive));

  EoDb::Write(file, m_PenColor);
  EoDb::Write(file, m_LineType);
  m_ln.Write(file);

  return true;
}
