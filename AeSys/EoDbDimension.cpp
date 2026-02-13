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
#include "EoGsRenderState.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif  // USING_DDE

std::uint16_t EoDbDimension::sm_flags{};

EoDbDimension::EoDbDimension(const EoDbDimension& other)
    : EoDbPrimitive(other),
      m_line{other.m_line},
      m_fontDefinition{other.m_fontDefinition},
      m_ReferenceSystem{other.m_ReferenceSystem},
      m_text{other.m_text},
      m_textColor{other.m_textColor} {}

const EoDbDimension& EoDbDimension::operator=(const EoDbDimension& other) {
  if (this == &other) { return (*this); }
  EoDbPrimitive::operator=(other);
  m_line = other.m_line;
  m_textColor = other.m_textColor;
  m_fontDefinition = other.m_fontDefinition;
  m_ReferenceSystem = other.m_ReferenceSystem;
  m_text = other.m_text;

  return (*this);
}

EoDbDimension::EoDbDimension(EoGeLine line, const EoDbFontDefinition& fontDefinition,
    const EoGeReferenceSystem& referenceSystem, const CString& text, std::int16_t textColor)
    : m_line{line},
      m_fontDefinition{fontDefinition},
      m_ReferenceSystem{referenceSystem},
      m_text{text},
      m_textColor{textColor} {}

void EoDbDimension::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<Dim>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}
EoDbPrimitive*& EoDbDimension::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbDimension(*this);
  return primitive;
}

void EoDbDimension::CutAt2Points(
    const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList* groups, EoDbGroupList* newGroups) {
  double dRel[2]{};
  if (!m_line.ComputeParametricRelation(firstPoint, dRel[0])) { return; }
  if (!m_line.ComputeParametricRelation(secondPoint, dRel[1])) { return; }

  EoDbDimension* dimension{};
  if (dRel[0] < Eo::geometricTolerance && dRel[1] >= 1.0 - Eo::geometricTolerance)
    // Put entire dimension in trap
    dimension = this;
  else {  // Something gets cut
    dimension = new EoDbDimension(*this);
    if (dRel[0] > Eo::geometricTolerance && dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut section out of middle
      dimension->SetBeginPoint(secondPoint);
      dimension->SetDefaultNote();

      groups->AddTail(new EoDbGroup(dimension));

      dimension = new EoDbDimension(*this);

      dimension->SetPoints(firstPoint, secondPoint);

      dimension->SetDefaultNote();
      m_line.end = firstPoint;
    } else if (dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut in two and place begin section in trap
      dimension->SetEndPoint(secondPoint);
      dimension->SetDefaultNote();
      m_line.begin = secondPoint;
    } else {  // Cut in two and place end section in trap
      dimension->SetBeginPoint(firstPoint);
      dimension->SetDefaultNote();
      m_line.end = firstPoint;
    }
    SetDefaultNote();
    groups->AddTail(new EoDbGroup(this));
  }
  newGroups->AddTail(new EoDbGroup(dimension));
}

void EoDbDimension::CutAtPoint(const EoGePoint3d& point, EoDbGroup* group) {
  EoGeLine line;

  if (m_line.CutAtPoint(point, line) != 0) {
    EoDbDimension* DimensionPrimitive = new EoDbDimension(*this);

    DimensionPrimitive->m_line = line;
    DimensionPrimitive->SetDefaultNote();
    group->AddTail(DimensionPrimitive);
  }
  SetDefaultNote();
}

void EoDbDimension::Display(AeSysView* view, CDC* deviceContext) {
  std::int16_t color = LogicalColor();
  renderState.SetPen(view, deviceContext, color, LogicalLineType());
  m_line.Display(view, deviceContext);

  renderState.SetColor(deviceContext, m_textColor);

  std::int16_t LineType = renderState.LineTypeIndex();
  renderState.SetLineType(deviceContext, 1);

  DisplayText(view, deviceContext, m_fontDefinition, m_ReferenceSystem, m_text);

  renderState.SetLineType(deviceContext, LineType);
}

void EoDbDimension::AddReportToMessageList(const EoGePoint3d& point) {
  CString str;
  str.Format(L"<Dim> Color: %s Line Type: %s", FormatPenColor().GetString(), FormatLineType().GetString());
  app.AddStringToMessageList(str);

  double length = Length();
  double angle = m_line.AngleFromXAxisXY();

  double dRel{};
  if (!m_line.ComputeParametricRelation(point, dRel)) { return; }

  // Normalize line prior to angle determination
  if (dRel > 0.5) { angle += Eo::Pi; }
  angle = fmod(angle, Eo::TwoPi);

  app.SetEngagedLength(length);
  app.SetEngagedAngle(angle);
#if defined(USING_DDE)
  dde::PostAdvise(dde::EngLenInfo);
  dde::PostAdvise(dde::EngAngZInfo);
#endif  // USING_DDE
}

void EoDbDimension::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s", FormatPenColor().GetString(), FormatLineType().GetString());
}
void EoDbDimension::FormatGeometry(CString& str) {
  str += L"Begin Point;" + m_line.begin.ToString();
  str += L"End Point;" + m_line.end.ToString();
}
void EoDbDimension::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_line.begin);
  points.Add(m_line.end);
}
// Determination of text extent.
void EoDbDimension::GetBoundingBox(EoGePoint3dArray& ptsBox, double dSpacFac) {
  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_text.GetLength(), dSpacFac, ptsBox);
}
void EoDbDimension::GetExtents(
    AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3d pt[2] = {m_line.begin, m_line.end};

  for (auto i = 0; i < 2; i++) {
    view->ModelTransformPoint(pt[i]);
    pt[i] = transformMatrix * pt[i];
    ptMin = EoGePoint3d::Min(ptMin, pt[i]);
    ptMax = EoGePoint3d::Max(ptMax, pt[i]);
  }
}

EoGePoint3d EoDbDimension::GoToNextControlPoint() {
  if (sm_controlPointIndex == 0) {
    sm_controlPointIndex = 1;
  } else if (sm_controlPointIndex == 1) {
    sm_controlPointIndex = 0;
  } else {  // Initial rock .. jump to point at lower left or down if vertical
    EoGePoint3d ptBeg = m_line.begin;
    EoGePoint3d ptEnd = m_line.end;

    if (ptEnd.x > ptBeg.x) {
      sm_controlPointIndex = 0;
    } else if (ptEnd.x < ptBeg.x) {
      sm_controlPointIndex = 1;
    } else if (ptEnd.y > ptBeg.y) {
      sm_controlPointIndex = 0;
    } else {
      sm_controlPointIndex = 1;
    }
  }
  return (sm_controlPointIndex == 0 ? m_line.begin : m_line.end);
}
bool EoDbDimension::IsInView(AeSysView* view) {
  EoGePoint4d pt[] = {EoGePoint4d(m_line.begin), EoGePoint4d(m_line.end)};

  view->ModelViewTransformPoints(2, &pt[0]);

  return (EoGePoint4d::ClipLine(pt[0], pt[1]));
}
bool EoDbDimension::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d pt;

  for (auto i = 0; i < 2; i++) {
    pt = EoGePoint4d(m_line[i]);
    view->ModelViewTransformPoint(pt);

    if (point.DistanceToPointXY(pt) < sm_SelectApertureSize) { return true; }
  }
  return false;
}

void EoDbDimension::ModifyState() {
  if ((sm_flags & 0x0001) != 0) { EoDbPrimitive::ModifyState(); }

  if ((sm_flags & 0x0002) != 0) { m_fontDefinition = renderState.FontDefinition(); }
}

double EoDbDimension::RelOfPt(const EoGePoint3d& point) {
  double relation{};
  (void)m_line.ComputeParametricRelation(point, relation);
  return relation;
}

EoGePoint3d EoDbDimension::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_controlPointIndex = SHRT_MAX;

  double dAPert = sm_SelectApertureSize;

  for (auto i = 0; i < 2; i++) {
    EoGePoint4d pt(m_line[i]);
    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dAPert) {
      sm_controlPointIndex = i;
      dAPert = dDis;
    }
  }
  return (sm_controlPointIndex == SHRT_MAX) ? EoGePoint3d::kOrigin : m_line[sm_controlPointIndex];
}
bool EoDbDimension::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  sm_flags &= ~0x0003;

  EoGePoint4d pt[4]{};
  pt[0] = EoGePoint4d(m_line.begin);
  pt[1] = EoGePoint4d(m_line.end);
  view->ModelViewTransformPoints(2, &pt[0]);

  EoGeLine ln;
  ln.begin = pt[0];
  ln.end = pt[1];
  if (ln.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
    ptProj.z = ln.begin.z + sm_RelationshipOfPoint * (ln.end.z - ln.begin.z);
    sm_flags |= 0x0001;
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
    if (EoGeLine(pt[n], pt[(n + 1) % 4]).DirRelOfPt(point) < 0) { return false; }
  }
  ptProj = point;
  sm_flags |= 0x0002;
  return true;
}
bool EoDbDimension::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_line.begin);
  polyline::SetVertex(m_line.end);

  return polyline::SelectUsingLine(view, line, intersections);
}
bool EoDbDimension::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_line.begin);
  polyline::SetVertex(m_line.end);

  if (!polyline::SelectUsingRectangle(view, pt1, pt2)) {
    EoGePoint3dArray pts;
    GetBoundingBox(pts, 0.0);
    return polyline::SelectUsingRectangle(view, pt1, pt2, pts);
  }
  return true;
}

void EoDbDimension::SetDefaultNote() {
  auto* activeView = AeSysView::GetActiveView();

  m_ReferenceSystem.SetOrigin(m_line.Midpoint());
  double angle{};
  wchar_t cText0 = m_text[0];
  if (cText0 != 'R' && cText0 != 'D') {
    angle = m_line.AngleFromXAxisXY();
    double dDis = 0.075;
    if (angle > Eo::HalfPi + Eo::Radian && angle < Eo::TwoPi - Eo::HalfPi + Eo::Radian) {
      angle -= Eo::Pi;
      dDis = -dDis;
    }
    EoGePoint3d ptOrigin{};
    EoGeLine(m_ReferenceSystem.Origin(), m_line.end).ProjPtFrom_xy(0.0, dDis, &ptOrigin);
    m_ReferenceSystem.SetOrigin(ptOrigin);
  }
  auto cameraDirection = activeView->CameraDirection();

  EoGeVector3d yDirection = activeView->ViewUp();
  yDirection.RotAboutArbAx(cameraDirection, angle);
  yDirection *= 0.1;

  EoGeVector3d xDirection = yDirection;
  xDirection.RotAboutArbAx(cameraDirection, -Eo::HalfPi);
  xDirection *= Eo::defaultCharacterCellAspectRatio;

  m_ReferenceSystem.SetXDirection(xDirection);
  m_ReferenceSystem.SetYDirection(yDirection);

  Eo::Units units = app.GetUnits();
  if (units == Eo::Units::Architectural) { units = Eo::Units::ArchitecturalS; }
  app.FormatLength(m_text, units, m_line.Length());
  m_text.TrimLeft();
  if (cText0 == 'R' || cText0 == 'D') { m_text = cText0 + m_text; }
}

void EoDbDimension::SetBeginPoint(const EoGePoint3d& begin) { m_line.begin = begin; }
void EoDbDimension::SetEndPoint(const EoGePoint3d& end) { m_line.end = end; }
void EoDbDimension::SetPoints(const EoGePoint3d& begin, const EoGePoint3d& end) {
  m_line.begin = begin;
  m_line.end = end;
}

void EoDbDimension::Transform(const EoGeTransformMatrix& transformMatrix) {
  if ((sm_flags & 0x0001) != 0) {
    m_line.begin = transformMatrix * m_line.begin;
    m_line.end = transformMatrix * m_line.end;
  }
  if ((sm_flags & 0x0002) != 0) { m_ReferenceSystem.Transform(transformMatrix); }
}

void EoDbDimension::Translate(const EoGeVector3d& v) {
  m_line += v;
  m_ReferenceSystem.SetOrigin(m_ReferenceSystem.Origin() + v);
}
void EoDbDimension::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if ((mask & 1) == 1) m_line.begin += v;

  if ((mask & 2) == 2) m_line.end += v;

  SetDefaultNote();
}
bool EoDbDimension::Write(CFile& file) {
  EoDb::Write(file, std::uint16_t(EoDb::kDimensionPrimitive));

  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  m_line.Write(file);

  EoDb::Write(file, m_textColor);
  m_fontDefinition.Write(file);
  m_ReferenceSystem.Write(file);
  EoDb::Write(file, m_text);

  return true;
}
