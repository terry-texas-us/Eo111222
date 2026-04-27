#include "Stdafx.h"

#include <climits>
#include <cmath>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbLabeledLine.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeReferenceSystem.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderDevice.h"
#include "EoGsRenderState.h"

#ifdef USING_DDE
#include "ddeGItms.h"
#endif

std::uint16_t EoDbLabeledLine::sm_flags{};

EoDbLabeledLine::EoDbLabeledLine(const EoDbLabeledLine& other)
    : EoDbPrimitive(other),
      m_line{other.m_line},
      m_fontDefinition{other.m_fontDefinition},
      m_ReferenceSystem{other.m_ReferenceSystem},
      m_text{other.m_text},
      m_textColor{other.m_textColor} {}

const EoDbLabeledLine& EoDbLabeledLine::operator=(const EoDbLabeledLine& other) {
  if (this == &other) { return (*this); }
  EoDbPrimitive::operator=(other);
  m_line = other.m_line;
  m_textColor = other.m_textColor;
  m_fontDefinition = other.m_fontDefinition;
  m_ReferenceSystem = other.m_ReferenceSystem;
  m_text = other.m_text;

  return (*this);
}

EoDbLabeledLine::EoDbLabeledLine(EoGeLine line,
    const EoDbFontDefinition& fontDefinition,
    const EoGeReferenceSystem& referenceSystem,
    const CString& text,
    std::int16_t textColor)
    : m_line{line},
      m_fontDefinition{fontDefinition},
      m_ReferenceSystem{referenceSystem},
      m_text{text},
      m_textColor{textColor} {}

void EoDbLabeledLine::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  tvAddItem(tree, parent, L"<LabeledLine>", this);
}
EoDbPrimitive*& EoDbLabeledLine::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbLabeledLine(*this);
  return primitive;
}

void EoDbLabeledLine::CutAt2Points(const EoGePoint3d& firstPoint,
    const EoGePoint3d& secondPoint,
    EoDbGroupList* groups,
    EoDbGroupList* newGroups) {
  double dRel[2]{};
  if (!m_line.ComputeParametricRelation(firstPoint, dRel[0])) { return; }
  if (!m_line.ComputeParametricRelation(secondPoint, dRel[1])) { return; }

  EoDbLabeledLine* dimension{};
  if (dRel[0] < Eo::geometricTolerance && dRel[1] >= 1.0 - Eo::geometricTolerance) {
    // Put entire dimension in trap
    dimension = this;
  } else {  // Something gets cut
    dimension = new EoDbLabeledLine(*this);
    if (dRel[0] > Eo::geometricTolerance && dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut section out of middle
      dimension->SetBeginPoint(secondPoint);
      dimension->SetDefaultNote();

      groups->AddTail(new EoDbGroup(dimension));

      dimension = new EoDbLabeledLine(*this);

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

void EoDbLabeledLine::CutAtPoint(const EoGePoint3d& point, EoDbGroup* group) {
  EoGeLine line;

  if (m_line.CutAtPoint(point, line) != 0) {
    auto* labeledLinePrimitive = new EoDbLabeledLine(*this);

    labeledLinePrimitive->m_line = line;
    labeledLinePrimitive->SetDefaultNote();
    group->AddTail(labeledLinePrimitive);
  }
  SetDefaultNote();
}

void EoDbLabeledLine::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  const auto color = LogicalColor();
  Gs::renderState.SetPen(
      view, renderDevice, color, LogicalLineType(), LogicalLineTypeName(), m_lineWeight, m_lineTypeScale);
  m_line.Display(view, renderDevice);

  Gs::renderState.SetColor(renderDevice, m_textColor);

  const auto lineType = Gs::renderState.LineTypeIndex();
  Gs::renderState.SetLineType(renderDevice, 1);

  DisplayText(view, renderDevice, m_fontDefinition, m_ReferenceSystem, m_text);

  Gs::renderState.SetLineType(renderDevice, lineType);
}

void EoDbLabeledLine::AddReportToMessageList(const EoGePoint3d& point) {
  app.AddStringToMessageList(CString(L"<LabeledLine>"));
  EoDbPrimitive::AddReportToMessageList(point);

  const auto length = Length();
  double angle = m_line.AngleFromXAxisXY();

  double dRel{};
  if (!m_line.ComputeParametricRelation(point, dRel)) { return; }

  // Normalize line prior to angle determination
  if (dRel > 0.5) { angle += Eo::Pi; }
  angle = fmod(angle, Eo::TwoPi);

  app.SetEngagedLength(length);
  app.SetEngagedAngle(angle);
#ifdef USING_DDE
  dde::PostAdvise(dde::EngLenInfo);
  dde::PostAdvise(dde::EngAngZInfo);
#endif
}

void EoDbLabeledLine::FormatExtra(CString& str) {
  EoDbPrimitive::FormatExtra(str);
  str += L'\t';
}
void EoDbLabeledLine::FormatGeometry(CString& str) {
  str += L"Begin Point;" + m_line.begin.ToString();
  str += L"End Point;" + m_line.end.ToString();
}
void EoDbLabeledLine::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_line.begin);
  points.Add(m_line.end);
}
// Determination of text extent.
void EoDbLabeledLine::GetBoundingBox(EoGePoint3dArray& ptsBox, double dSpacFac) {
  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_text, dSpacFac, ptsBox);
}
void EoDbLabeledLine::GetExtents(AeSysView* view,
    EoGePoint3d& ptMin,
    EoGePoint3d& ptMax,
    const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3d pt[2] = {m_line.begin, m_line.end};

  for (auto i = 0; i < 2; i++) {
    view->ModelTransformPoint(pt[i]);
    pt[i] = transformMatrix * pt[i];
    ptMin = EoGePoint3d::Min(ptMin, pt[i]);
    ptMax = EoGePoint3d::Max(ptMax, pt[i]);
  }
}

EoGePoint3d EoDbLabeledLine::GoToNextControlPoint() {
  if (sm_controlPointIndex == 0) {
    sm_controlPointIndex = 1;
  } else if (sm_controlPointIndex == 1) {
    sm_controlPointIndex = 0;
  } else {  // Initial rock .. jump to point at lower left or down if vertical
    const EoGePoint3d ptBeg = m_line.begin;
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
bool EoDbLabeledLine::IsInView(AeSysView* view) {
  EoGePoint4d pt[] = {EoGePoint4d(m_line.begin), EoGePoint4d(m_line.end)};

  view->ModelViewTransformPoints(2, &pt[0]);

  return (EoGePoint4d::ClipLine(pt[0], pt[1]));
}
bool EoDbLabeledLine::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d pt;

  for (auto i = 0; i < 2; i++) {
    pt = EoGePoint4d(m_line[i]);
    view->ModelViewTransformPoint(pt);

    if (point.DistanceToPointXY(pt) < sm_SelectApertureSize) { return true; }
  }
  return false;
}

void EoDbLabeledLine::ModifyState() {
  if ((sm_flags & 0x0001) != 0) { EoDbPrimitive::ModifyState(); }

  if ((sm_flags & 0x0002) != 0) { m_fontDefinition = Gs::renderState.FontDefinition(); }
}

double EoDbLabeledLine::RelOfPt(const EoGePoint3d& point) {
  double relation{};
  (void)m_line.ComputeParametricRelation(point, relation);
  return relation;
}

EoGePoint3d EoDbLabeledLine::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
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
bool EoDbLabeledLine::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  sm_flags &= ~0x0003;

  EoGePoint4d pt[4]{};
  pt[0] = EoGePoint4d(m_line.begin);
  pt[1] = EoGePoint4d(m_line.end);
  view->ModelViewTransformPoints(2, &pt[0]);

  EoGeLine ln;
  ln.begin = EoGePoint3d{pt[0]};
  ln.end = EoGePoint3d{pt[1]};
  if (ln.IsSelectedByPointXY(EoGePoint3d{point}, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
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
    if (EoGeLine(EoGePoint3d{pt[n]}, EoGePoint3d{pt[(n + 1) % 4]}).DirRelOfPt(EoGePoint3d{point}) < 0) { return false; }
  }
  ptProj = EoGePoint3d{point};
  sm_flags |= 0x0002;
  return true;
}
bool EoDbLabeledLine::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_line.begin);
  polyline::SetVertex(m_line.end);

  return polyline::SelectUsingLine(view, line, intersections);
}
bool EoDbLabeledLine::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
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

void EoDbLabeledLine::SetDefaultNote() {
  const auto* activeView = AeSysView::GetActiveView();

  m_ReferenceSystem.SetOrigin(m_line.Midpoint());
  double angle{};
  const wchar_t cText0 = m_text[0];
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
  const auto cameraDirection = activeView->CameraDirection();

  EoGeVector3d yDirection = activeView->ViewUp();
  yDirection.RotateAboutArbitraryAxis(cameraDirection, angle);
  yDirection *= 0.1;

  EoGeVector3d xDirection = yDirection;
  xDirection.RotateAboutArbitraryAxis(cameraDirection, -Eo::HalfPi);
  xDirection *= Eo::defaultCharacterCellAspectRatio;

  m_ReferenceSystem.SetXDirection(xDirection);
  m_ReferenceSystem.SetYDirection(yDirection);

  Eo::Units units = app.GetUnits();
  if (units == Eo::Units::Architectural) { units = Eo::Units::ArchitecturalS; }
  app.FormatLength(m_text, units, m_line.Length());
  m_text.TrimLeft();
  if (cText0 == 'R' || cText0 == 'D') { m_text = cText0 + m_text; }
}

void EoDbLabeledLine::SetBeginPoint(const EoGePoint3d& begin) {
  m_line.begin = begin;
}
void EoDbLabeledLine::SetEndPoint(const EoGePoint3d& end) {
  m_line.end = end;
}
void EoDbLabeledLine::SetPoints(const EoGePoint3d& begin, const EoGePoint3d& end) {
  m_line.begin = begin;
  m_line.end = end;
}

void EoDbLabeledLine::Transform(const EoGeTransformMatrix& transformMatrix) {
  if ((sm_flags & 0x0001) != 0) {
    m_line.begin = transformMatrix * m_line.begin;
    m_line.end = transformMatrix * m_line.end;
  }
  if ((sm_flags & 0x0002) != 0) { m_ReferenceSystem.Transform(transformMatrix); }
}

void EoDbLabeledLine::Translate(const EoGeVector3d& v) {
  m_line += v;
  m_ReferenceSystem.SetOrigin(m_ReferenceSystem.Origin() + v);
}
void EoDbLabeledLine::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if ((mask & 1) == 1) { m_line.begin += v; }

  if ((mask & 2) == 2) { m_line.end += v; }

  SetDefaultNote();
}
EoDbLabeledLine* EoDbLabeledLine::ReadFromPeg(CFile& file) {
  const auto color = EoDb::ReadInt16(file);
  const auto lineType = EoDb::ReadInt16(file);
  const auto beginPoint = EoDb::ReadPoint3d(file);
  const auto endPoint = EoDb::ReadPoint3d(file);
  const auto textColor = EoDb::ReadInt16(file);
  EoDbFontDefinition fontDefinition;
  fontDefinition.Read(file);
  EoGeReferenceSystem referenceSystem;
  referenceSystem.Read(file);
  CString text;
  EoDb::Read(file, text);

  auto* dimension =
      new EoDbLabeledLine(EoGeLine(beginPoint, endPoint), fontDefinition, referenceSystem, text, textColor);
  dimension->SetColor(color);
  dimension->SetLineTypeName(EoDbLineTypeTable::LegacyLineTypeName(lineType));
  return dimension;
}

bool EoDbLabeledLine::Write(CFile& file) {
  EoDb::WriteUInt16(file, std::uint16_t(EoDb::kDimensionPrimitive));

  EoDb::WriteInt16(file, m_color);
  EoDb::WriteInt16(file, EoDbLineTypeTable::LegacyLineTypeIndex(m_lineType));
  m_line.Write(file);

  EoDb::WriteInt16(file, m_textColor);
  m_fontDefinition.Write(file);
  m_ReferenceSystem.Write(file);
  EoDb::Write(file, m_text);

  return true;
}
