#include "Stdafx.h"

#include <climits>
#include <cmath>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbDimension.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeReferenceSystem.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "PrimState.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif  // USING_DDE

EoUInt16 EoDbDimension::sm_wFlags = 0;

EoDbDimension::EoDbDimension(EoInt16 color, EoInt16 lineType, EoGeLine line) : m_ln(line) {
  m_color = color;
  m_lineTypeIndex = lineType;

  m_nTextPenColor = 5;
  pstate.GetFontDef(m_fontDefinition);
  SetDefaultNote();
}
EoDbDimension::EoDbDimension(EoInt16 color, EoInt16 lineType, EoGeLine line, EoInt16 textPenColor,
                             const EoDbFontDefinition& fontDefinition, const EoGeReferenceSystem& referenceSystem,
                             const CString& text)
    : m_ln(line), m_fontDefinition(fontDefinition), m_ReferenceSystem(referenceSystem), m_strText(text) {
  m_color = color;
  m_lineTypeIndex = lineType;
  m_nTextPenColor = textPenColor;
}
EoDbDimension::EoDbDimension(const EoDbDimension& src) {
  m_color = src.m_color;
  m_lineTypeIndex = src.m_lineTypeIndex;
  m_ln = src.m_ln;

  m_nTextPenColor = src.m_nTextPenColor;
  m_fontDefinition = src.m_fontDefinition;
  m_ReferenceSystem = src.m_ReferenceSystem;
  m_strText = src.m_strText;
}
const EoDbDimension& EoDbDimension::operator=(const EoDbDimension& src) {
  m_color = src.m_color;
  m_lineTypeIndex = src.m_lineTypeIndex;
  m_ln = src.m_ln;

  m_nTextPenColor = src.m_nTextPenColor;
  m_fontDefinition = src.m_fontDefinition;
  m_ReferenceSystem = src.m_ReferenceSystem;
  m_strText = src.m_strText;

  return (*this);
}
void EoDbDimension::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<Dim>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}
EoDbPrimitive*& EoDbDimension::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbDimension(*this);
  return (primitive);
}

void EoDbDimension::CutAt2Points(const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList* groups, EoDbGroupList* newGroups) {
  EoDbDimension* pDim;
  double dRel[2]{};

  m_ln.RelOfPtToEndPts(firstPoint, dRel[0]);
  m_ln.RelOfPtToEndPts(secondPoint, dRel[1]);

  if (dRel[0] < Eo::geometricTolerance && dRel[1] >= 1.0 - Eo::geometricTolerance)
    // Put entire dimension in trap
    pDim = this;
  else {  // Something gets cut
    pDim = new EoDbDimension(*this);
    if (dRel[0] > Eo::geometricTolerance && dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut section out of middle
      pDim->BeginPoint(secondPoint);
      pDim->SetDefaultNote();

      groups->AddTail(new EoDbGroup(pDim));

      pDim = new EoDbDimension(*this);
      pDim->BeginPoint(firstPoint);
      pDim->EndPoint(secondPoint);
      pDim->SetDefaultNote();
      m_ln.end = firstPoint;
    } else if (dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut in two and place begin section in trap
      pDim->EndPoint(secondPoint);
      pDim->SetDefaultNote();
      m_ln.begin = secondPoint;
    } else {  // Cut in two and place end section in trap
      pDim->BeginPoint(firstPoint);
      pDim->SetDefaultNote();
      m_ln.end = firstPoint;
    }
    SetDefaultNote();
    groups->AddTail(new EoDbGroup(this));
  }
  newGroups->AddTail(new EoDbGroup(pDim));
}

void EoDbDimension::CutAtPoint(EoGePoint3d& pointt, EoDbGroup* group) {
  EoGeLine ln;

  if (m_ln.CutAtPt(pointt, ln) != 0) {
    EoDbDimension* DimensionPrimitive = new EoDbDimension(*this);

    DimensionPrimitive->m_ln = ln;
    DimensionPrimitive->SetDefaultNote();
    group->AddTail(DimensionPrimitive);
  }
  SetDefaultNote();
}
void EoDbDimension::Display(AeSysView* view, CDC* deviceContext) {
  EoInt16 color = LogicalColor();
  pstate.SetPen(view, deviceContext, color, LogicalLineType());
  m_ln.Display(view, deviceContext);

  pstate.SetColor(deviceContext, m_nTextPenColor);

  EoInt16 LineType = pstate.LineType();
  pstate.SetLineType(deviceContext, 1);

  DisplayText(view, deviceContext, m_fontDefinition, m_ReferenceSystem, m_strText);

  pstate.SetLineType(deviceContext, LineType);
}
void EoDbDimension::AddReportToMessageList(EoGePoint3d pt) {
  CString str;
  str.Format(L"<Dim> Color: %s Line Type: %s", FormatPenColor().GetString(), FormatLineType().GetString());
  app.AddStringToMessageList(str);

  double dLen = Length();
  double dAng = m_ln.AngleFromXAxisXY();

  double dRel;
  m_ln.RelOfPtToEndPts(pt, dRel);

  if (dRel > 0.5)
    // Normalize line prior to angle determination
    dAng += Eo::Pi;

  dAng = fmod(dAng, Eo::TwoPi);
  app.SetEngagedLength(dLen);
  app.SetEngagedAngle(dAng);
#if defined(USING_DDE)
  dde::PostAdvise(dde::EngLenInfo);
  dde::PostAdvise(dde::EngAngZInfo);
#endif  // USING_DDE
}
void EoDbDimension::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s", FormatPenColor().GetString(), FormatLineType().GetString());
}
void EoDbDimension::FormatGeometry(CString& str) {
  str += L"Begin Point;" + m_ln.begin.ToString();
  str += L"End Point;" + m_ln.end.ToString();
}
void EoDbDimension::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_ln.begin);
  points.Add(m_ln.end);
}
// Determination of text extent.
void EoDbDimension::GetBoundingBox(EoGePoint3dArray& ptsBox, double dSpacFac) {
  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_strText.GetLength(), dSpacFac, ptsBox);
}
void EoDbDimension::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  EoGePoint3d pt[2] = {m_ln.begin, m_ln.end};

  for (EoUInt16 w = 0; w < 2; w++) {
    view->ModelTransformPoint(pt[w]);
    pt[w] = tm * pt[w];
    ptMin = EoGePoint3d::Min(ptMin, pt[w]);
    ptMax = EoGePoint3d::Max(ptMax, pt[w]);
  }
}
EoGePoint3d EoDbDimension::GoToNextControlPoint() {
  if (sm_ControlPointIndex == 0)
    sm_ControlPointIndex = 1;
  else if (sm_ControlPointIndex == 1)
    sm_ControlPointIndex = 0;
  else {  // Initial rock .. jump to point at lower left or down if vertical
    EoGePoint3d ptBeg = m_ln.begin;
    EoGePoint3d ptEnd = m_ln.end;

    if (ptEnd.x > ptBeg.x)
      sm_ControlPointIndex = 0;
    else if (ptEnd.x < ptBeg.x)
      sm_ControlPointIndex = 1;
    else if (ptEnd.y > ptBeg.y)
      sm_ControlPointIndex = 0;
    else
      sm_ControlPointIndex = 1;
  }
  return (sm_ControlPointIndex == 0 ? m_ln.begin : m_ln.end);
}
bool EoDbDimension::IsInView(AeSysView* view) {
  EoGePoint4d pt[] = {EoGePoint4d(m_ln.begin), EoGePoint4d(m_ln.end)};

  view->ModelViewTransformPoints(2, &pt[0]);

  return (EoGePoint4d::ClipLine(pt[0], pt[1]));
}
bool EoDbDimension::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d pt;

  for (EoUInt16 w = 0; w < 2; w++) {
    pt = EoGePoint4d(m_ln[w]);
    view->ModelViewTransformPoint(pt);

    if (point.DistanceToPointXY(pt) < sm_SelectApertureSize) return true;
  }
  return false;
}
void EoDbDimension::ModifyState() {
  if ((sm_wFlags & 0x0001) != 0) EoDbPrimitive::ModifyState();

  if ((sm_wFlags & 0x0002) != 0) pstate.GetFontDef(m_fontDefinition);
}
double EoDbDimension::RelOfPt(EoGePoint3d pt) {
  double dRel;
  m_ln.RelOfPtToEndPts(pt, dRel);
  return dRel;
}
EoGePoint3d EoDbDimension::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_ControlPointIndex = USHRT_MAX;

  double dAPert = sm_SelectApertureSize;

  for (EoUInt16 w = 0; w < 2; w++) {
    EoGePoint4d pt(m_ln[w]);
    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dAPert) {
      sm_ControlPointIndex = w;
      dAPert = dDis;
    }
  }
  return (sm_ControlPointIndex == USHRT_MAX) ? EoGePoint3d::kOrigin : m_ln[sm_ControlPointIndex];
}
bool EoDbDimension::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  sm_wFlags &= ~0x0003;

  EoGePoint4d pt[4]{};
  pt[0] = EoGePoint4d(m_ln.begin);
  pt[1] = EoGePoint4d(m_ln.end);
  view->ModelViewTransformPoints(2, &pt[0]);

  EoGeLine ln;
  ln.begin = pt[0];
  ln.end = pt[1];
  if (ln.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
    ptProj.z = ln.begin.z + sm_RelationshipOfPoint * (ln.end.z - ln.begin.z);
    sm_wFlags |= 0x0001;
    return true;
  }
  EoGePoint3dArray ptsExt;
  GetBoundingBox(ptsExt, 0.0);

  pt[0] = EoGePoint4d(ptsExt[0]);
  pt[1] = EoGePoint4d(ptsExt[1]);
  pt[2] = EoGePoint4d(ptsExt[2]);
  pt[3] = EoGePoint4d(ptsExt[3]);
  view->ModelViewTransformPoints(4, pt);

  for (size_t n = 0; n < 4; n++) {
    if (EoGeLine(pt[n], pt[(n + 1) % 4]).DirRelOfPt(point) < 0) return false;
  }
  ptProj = point;
  sm_wFlags |= 0x0002;
  return true;
}
bool EoDbDimension::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_ln.begin);
  polyline::SetVertex(m_ln.end);

  return polyline::SelectUsingLine(view, line, intersections);
}
bool EoDbDimension::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_ln.begin);
  polyline::SetVertex(m_ln.end);

  if (!polyline::SelectUsingRectangle(view, pt1, pt2)) {
    EoGePoint3dArray pts;
    GetBoundingBox(pts, 0.0);
    return polyline::SelectUsingRectangle(view, pt1, pt2, pts);
  }
  return true;
}
void EoDbDimension::SetDefaultNote() {
  auto* activeView = AeSysView::GetActiveView();

  m_ReferenceSystem.SetOrigin(m_ln.Midpoint());
  double dAng = 0.;
  wchar_t cText0 = m_strText[0];
  if (cText0 != 'R' && cText0 != 'D') {
    dAng = m_ln.AngleFromXAxisXY();
    double dDis = 0.075;
    if (dAng > Eo::HalfPi + Eo::Radian && dAng < Eo::TwoPi - Eo::HalfPi + Eo::Radian) {
      dAng -= Eo::Pi;
      dDis = -dDis;
    }
    EoGePoint3d ptOrigin;
    EoGeLine(m_ReferenceSystem.Origin(), m_ln.end).ProjPtFrom_xy(0.0, dDis, &ptOrigin);
    m_ReferenceSystem.SetOrigin(ptOrigin);
  }
  EoGeVector3d vPlnNorm = activeView->CameraDirection();

  EoGeVector3d vRefXAx;
  EoGeVector3d vRefYAx;

  vRefYAx = activeView->ViewUp();
  vRefYAx.RotAboutArbAx(vPlnNorm, dAng);
  vRefYAx *= 0.1;
  vRefXAx = vRefYAx;
  vRefXAx.RotAboutArbAx(vPlnNorm, -Eo::HalfPi);
  vRefXAx *= 0.6;

  m_ReferenceSystem.SetXDirection(vRefXAx);
  m_ReferenceSystem.SetYDirection(vRefYAx);

  AeSys::Units Units = app.GetUnits();
  if (Units == AeSys::kArchitectural) { Units = AeSys::kArchitecturalS; }
  app.FormatLength(m_strText, Units, m_ln.Length());
  m_strText.TrimLeft();
  if (cText0 == 'R' || cText0 == 'D') { m_strText = cText0 + m_strText; }
}
void EoDbDimension::Transform(EoGeTransformMatrix& tm) {
  if ((sm_wFlags & 0x0001) != 0) {
    m_ln.begin = tm * m_ln.begin;
    m_ln.end = tm * m_ln.end;
  }
  if ((sm_wFlags & 0x0002) != 0) { m_ReferenceSystem.Transform(tm); }
}
void EoDbDimension::Translate(EoGeVector3d v) {
  m_ln += v;
  m_ReferenceSystem.SetOrigin(m_ReferenceSystem.Origin() + v);
}
void EoDbDimension::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if ((mask & 1) == 1) m_ln.begin += v;

  if ((mask & 2) == 2) m_ln.end += v;

  SetDefaultNote();
}
bool EoDbDimension::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kDimensionPrimitive));

  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  m_ln.Write(file);

  EoDb::Write(file, m_nTextPenColor);
  m_fontDefinition.Write(file);
  m_ReferenceSystem.Write(file);
  EoDb::Write(file, m_strText);

  return true;
}
