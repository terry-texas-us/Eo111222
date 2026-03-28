#include "Stdafx.h"

#include <climits>
#include <cmath>
#include <cstdint>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoGsRenderDevice.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbLine.h"
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

EoDbLine::EoDbLine(const EoGePoint3d& begin, const EoGePoint3d& end) : EoDbPrimitive(), m_line{begin, end} {}

EoDbLine* EoDbLine::CreateLine(const EoGePoint3d& begin, const EoGePoint3d& end) { return new EoDbLine(begin, end); }

EoDbLine* EoDbLine::CreateLine(const EoGeLine& line) { return new EoDbLine(line.begin, line.end); }

EoDbLine::EoDbLine(const EoDbLine& other) : EoDbPrimitive(other), m_line{other.m_line} {}

const EoDbLine& EoDbLine::operator=(const EoDbLine& other) {
  if (this != &other) {
    EoDbPrimitive::operator=(other);
    m_line = other.m_line;
  }
  return (*this);
}

void EoDbLine::AddToTreeViewControl(HWND tree, HTREEITEM parent) { tvAddItem(tree, parent, L"<Line>", this); }

EoDbPrimitive*& EoDbLine::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbLine(*this);
  return primitive;
}

/** @brief Cuts a line at two points.
 * @param points Pointer to two points for the line cut.
 * @param groups Group list to receive the original line segments.
 * @param newGroups Group list to receive the new line segments.
 * @note Line segment between two points goes in groups.
 */
void EoDbLine::CutAt2Points(
    const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList* groups, EoDbGroupList* newGroups) {
  EoDbLine* line{};
  double relation[2]{};

  if (!m_line.ComputeParametricRelation(firstPoint, relation[0])) { return; }
  if (!m_line.ComputeParametricRelation(secondPoint, relation[1])) { return; }

  if (relation[0] < Eo::geometricTolerance && relation[1] >= 1.0 - Eo::geometricTolerance) {
    // The two points effectively cover the whole line. No cutting. Put entire line in trap.
    line = this;
  } else {  // Something gets cut
    line = new EoDbLine(*this);
    if (relation[0] > Eo::geometricTolerance &&
        relation[1] < 1.0 - Eo::geometricTolerance) {  // Cut section out of middle
      line->SetBeginPoint(secondPoint);
      groups->AddTail(new EoDbGroup(line));
      line = new EoDbLine(*this);
      line->SetBeginPoint(firstPoint);
      line->SetEndPoint(secondPoint);
      SetEndPoint(firstPoint);
    } else if (relation[1] < 1.0 - Eo::geometricTolerance) {  // Cut in two and place begin section in trap
      line->SetEndPoint(secondPoint);
      SetBeginPoint(secondPoint);
    } else {  // Cut in two and place end section in trap
      line->SetBeginPoint(firstPoint);
      SetEndPoint(firstPoint);
    }
    groups->AddTail(new EoDbGroup(this));
  }
  newGroups->AddTail(new EoDbGroup(line));
}

void EoDbLine::CutAtPoint(const EoGePoint3d& point, EoDbGroup* group) {
  EoGeLine line{};
  if (m_line.CutAtPoint(point, line) != 0) {
    group->AddTail(EoDbLine::CreateLine(line)->WithProperties(m_color, m_lineTypeIndex));
  }
}

void EoDbLine::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  std::int16_t color = LogicalColor();
  std::int16_t lineType = LogicalLineType();
  const auto& lineTypeName = LogicalLineTypeName();

  renderState.SetPen(view, renderDevice, color, lineType, lineTypeName, m_lineWeight, m_lineTypeScale);

  polyline::BeginLineStrip();
  polyline::SetVertex(m_line.begin);
  polyline::SetVertex(m_line.end);
  polyline::End(view, renderDevice, lineType, lineTypeName);
}

void EoDbLine::ExportToDxf(EoDxfInterface* writer) const {
  EoDxfLine line;
  PopulateDxfBaseProperties(&line);
  line.m_startPoint = {m_line.begin.x, m_line.begin.y, m_line.begin.z};
  line.m_endPoint = {m_line.end.x, m_line.end.y, m_line.end.z};
  writer->AddLine(line);
}

void EoDbLine::AddReportToMessageList(const EoGePoint3d& point) {
  double length = Length();
  double angle = m_line.AngleFromXAxisXY();

  double relation{};
  if (!m_line.ComputeParametricRelation(point, relation)) { return; }

  if (relation > 0.5) { angle += Eo::Pi; }
  angle = fmod(angle, Eo::TwoPi);

  CString lengthAsString;
  CString angleAsString;
  app.FormatLength(lengthAsString, app.GetUnits(), length);
  app.FormatAngle(angleAsString, angle, 8, 3);

  app.AddStringToMessageList(L"<Line>");
  EoDbPrimitive::AddReportToMessageList(point);
  CString message;
  message.Format(L"  %s @ %s", lengthAsString.TrimLeft().GetString(), angleAsString.GetString());
  app.AddStringToMessageList(message);

  app.SetEngagedLength(length);
  app.SetEngagedAngle(angle);

#if defined(USING_DDE)
  dde::PostAdvise(dde::EngLenInfo);
  dde::PostAdvise(dde::EngAngZInfo);
#endif
}

void EoDbLine::FormatExtra(CString& str) {
  EoDbPrimitive::FormatExtra(str);
  CString formattedLength;
  app.FormatLength(formattedLength, app.GetUnits(), Length());
  str.AppendFormat(L"\tLength;%s\tZ-Angle;%f", formattedLength.TrimLeft().GetString(),
      Eo::RadianToDegree(m_line.AngleFromXAxisXY()));
  str += L'\t';
}

void EoDbLine::FormatGeometry(CString& str) {
  str += L"Begin Point;" + m_line.begin.ToString();
  str += L"End Point;" + m_line.end.ToString();
}
void EoDbLine::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_line.begin);
  points.Add(m_line.end);
}

void EoDbLine::GetExtents(
    AeSysView* view, EoGePoint3d& minPoint, EoGePoint3d& maxPoint, const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3d points[2]{m_line.begin, m_line.end};

  for (auto i = 0; i < 2; i++) {
    view->ModelTransformPoint(points[i]);
    points[i] = transformMatrix * points[i];
    minPoint = EoGePoint3d::Min(minPoint, points[i]);
    maxPoint = EoGePoint3d::Max(maxPoint, points[i]);
  }
}

EoGePoint3d EoDbLine::GoToNextControlPoint() {
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

bool EoDbLine::Identical(EoDbPrimitive* primitive) { return m_line == static_cast<EoDbLine*>(primitive)->Line(); }
bool EoDbLine::IsInView(AeSysView* view) {
  EoGePoint4d pt[] = {EoGePoint4d(m_line.begin), EoGePoint4d(m_line.end)};
  view->ModelViewTransformPoints(2, &pt[0]);

  return (EoGePoint4d::ClipLine(pt[0], pt[1]));
}

bool EoDbLine::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d ndcPoint;

  ndcPoint = EoGePoint4d{m_line.begin};
  view->ModelViewTransformPoint(ndcPoint);

  if (point.DistanceToPointXY(ndcPoint) < sm_SelectApertureSize) { return true; }

  ndcPoint = EoGePoint4d{m_line.end};
  view->ModelViewTransformPoint(ndcPoint);

  if (point.DistanceToPointXY(ndcPoint) < sm_SelectApertureSize) { return true; }

  return false;
}

/** @brief Clips line to area defined by two points using Cohen-Sutherland algorithm.
 * @param lowerLeft Lower left point of clipping rectangle.
 * @param upperRight Upper right point of clipping rectangle.
 * @param clippedPoints Array of two points to receive clipped line endpoints.
 * @return
 *		0 - line is completely outside area
 *		2 - line is completely or partially within area (clippedPoints contains clipped line)
 */
int EoDbLine::IsWithinArea(const EoGePoint3d& lowerLeft, const EoGePoint3d& upperRight, EoGePoint3d* ptInt) {
  int i;
  int iLoc[2]{};

  ptInt[0] = m_line.begin;
  ptInt[1] = m_line.end;

  for (i = 0; i < 2; i++) { iLoc[i] = ptInt[i].RelationshipToRectangle(lowerLeft, upperRight); }
  while (iLoc[0] != 0 || iLoc[1] != 0) {
    if ((iLoc[0] & iLoc[1]) != 0) { return 0; }

    i = (iLoc[0] != 0) ? 0 : 1;
    if ((iLoc[i] & 1) != 0) {  // Clip against top
      ptInt[i].x = ptInt[i].x + (ptInt[1].x - ptInt[0].x) * (upperRight.y - ptInt[i].y) / (ptInt[1].y - ptInt[0].y);
      ptInt[i].y = upperRight.y;
    } else if ((iLoc[i] & 2) != 0) {  // Clip against bottom
      ptInt[i].x = ptInt[i].x + (ptInt[1].x - ptInt[0].x) * (lowerLeft.y - ptInt[i].y) / (ptInt[1].y - ptInt[0].y);
      ptInt[i].y = lowerLeft.y;
    } else if ((iLoc[i] & 4) != 0) {  // Clip against right
      ptInt[i].y = ptInt[i].y + (ptInt[1].y - ptInt[0].y) * (upperRight.x - ptInt[i].x) / (ptInt[1].x - ptInt[0].x);
      ptInt[i].x = upperRight.x;
    } else if ((iLoc[i] & 8) != 0) {  // Clip against left
      ptInt[i].y = ptInt[i].y + (ptInt[1].y - ptInt[0].y) * (lowerLeft.x - ptInt[i].x) / (ptInt[1].x - ptInt[0].x);
      ptInt[i].x = lowerLeft.x;
    }
    iLoc[i] = ptInt[i].RelationshipToRectangle(lowerLeft, upperRight);
  }
  return (2);
}

double EoDbLine::RelOfPt(const EoGePoint3d& point) {
  double relation{};
  (void)m_line.ComputeParametricRelation(point, relation);
  return relation;
}

EoGePoint3d EoDbLine::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_controlPointIndex = SHRT_MAX;

  double dApert = sm_SelectApertureSize;

  for (auto i = 0; i < 2; i++) {
    EoGePoint4d pt(m_line[i]);
    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dApert) {
      sm_controlPointIndex = i;
      dApert = dDis;
    }
  }
  return (sm_controlPointIndex == SHRT_MAX) ? EoGePoint3d::kOrigin : m_line[sm_controlPointIndex];
}
/** @brief Evaluates whether a line intersects line.
 * @param view Current view.
 * @param line Line to test against.
 * @param intersections Array to receive intersection points.
 * @return true if intersection occurs, false otherwise.
 */
bool EoDbLine::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_line.begin);
  polyline::SetVertex(m_line.end);
  return polyline::SelectUsingLine(view, line, intersections);
}
bool EoDbLine::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_line.begin);
  polyline::SetVertex(m_line.end);
  return polyline::SelectUsingPoint(view, point, sm_RelationshipOfPoint, ptProj);
}
bool EoDbLine::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  polyline::BeginLineStrip();
  polyline::SetVertex(m_line.begin);
  polyline::SetVertex(m_line.end);
  return polyline::SelectUsingRectangle(view, pt1, pt2);
}
void EoDbLine::Square(AeSysView* view) {
  auto ptBeg = view->SnapPointToGrid(m_line.begin);
  auto ptEnd = view->SnapPointToGrid(m_line.end);

  auto pt = EoGeLine(ptBeg, ptEnd).Midpoint();
  double dLen = EoGeVector3d(ptBeg, ptEnd).Length();
  ptEnd = view->SnapPointToAxis(pt, ptEnd);
  SetBeginPoint(ptEnd.ProjectToward(pt, dLen));
  SetEndPoint(ptEnd);
}

void EoDbLine::Transform(const EoGeTransformMatrix& transformMatrix) {
  SetBeginPoint(transformMatrix * m_line.begin);
  SetEndPoint(transformMatrix * m_line.end);
}

void EoDbLine::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if ((mask & 1) == 1) { SetBeginPoint(m_line.begin + v); }

  if ((mask & 2) == 2) { SetEndPoint(m_line.end + v); }
}
EoDbLine* EoDbLine::ReadFromPeg(CFile& file) {
  auto color = EoDb::ReadInt16(file);
  auto lineTypeIndex = EoDb::ReadInt16(file);
  auto begin = EoDb::ReadPoint3d(file);
  auto end = EoDb::ReadPoint3d(file);

  auto* line = new EoDbLine(begin, end);
  line->SetColor(color);
  line->SetLineTypeIndex(lineTypeIndex);
  return line;
}

bool EoDbLine::Write(CFile& file) {
  EoDb::WriteUInt16(file, std::uint16_t(EoDb::kLinePrimitive));

  EoDb::WriteInt16(file, m_color);
  EoDb::WriteInt16(file, m_lineTypeIndex);
  m_line.Write(file);

  return true;
}
