#include "Stdafx.h"

#include <climits>
#include <cmath>
#include <cstdint>

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
#include "EoGsRenderState.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif  // USING_DDE

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

void EoDbLine::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<Line>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

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
  double dRel[2]{};

  m_line.RelOfPtToEndPts(firstPoint, dRel[0]);
  m_line.RelOfPtToEndPts(secondPoint, dRel[1]);

  if (dRel[0] < Eo::geometricTolerance && dRel[1] >= 1.0 - Eo::geometricTolerance) {
    // The two points effectively cover the whole line. No cutting. Put entire line in trap.
    line = this;
  } else {  // Something gets cut
    line = new EoDbLine(*this);
    if (dRel[0] > Eo::geometricTolerance && dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut section out of middle
      line->SetBeginPoint(secondPoint);
      groups->AddTail(new EoDbGroup(line));
      line = new EoDbLine(*this);
      line->SetBeginPoint(firstPoint);
      line->SetEndPoint(secondPoint);
      SetEndPoint(firstPoint);
    } else if (dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut in two and place begin section in trap
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

void EoDbLine::Display(AeSysView* view, CDC* deviceContext) {
  std::int16_t color = LogicalColor();
  std::int16_t lineType = LogicalLineType();

  renderState.SetPen(view, deviceContext, color, lineType);

  polyline::BeginLineStrip();
  polyline::SetVertex(m_line.begin);
  polyline::SetVertex(m_line.end);
  polyline::__End(view, deviceContext, lineType);
}
void EoDbLine::AddReportToMessageList(const EoGePoint3d& point) {
  double dLen = Length();
  double AngleInXYPlane = m_line.AngleFromXAxisXY();

  double dRel;
  m_line.RelOfPtToEndPts(point, dRel);

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
      FormattedLength.TrimLeft().GetString(), Eo::RadianToDegree(m_line.AngleFromXAxisXY()));
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
  EoGePoint4d pt;

  pt = m_line.begin;
  view->ModelViewTransformPoint(pt);

  if (point.DistanceToPointXY(pt) < sm_SelectApertureSize) { return true; }

  pt = m_line.end;
  view->ModelViewTransformPoint(pt);

  if (point.DistanceToPointXY(pt) < sm_SelectApertureSize) { return true; }

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
  double dRel;
  m_line.RelOfPtToEndPts(point, dRel);
  return dRel;
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
  EoGePoint3d ptBeg = view->SnapPointToGrid(m_line.begin);
  EoGePoint3d ptEnd = view->SnapPointToGrid(m_line.end);

  EoGePoint3d pt = EoGeLine(ptBeg, ptEnd).Midpoint();
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
bool EoDbLine::Write(CFile& file) {
  EoDb::Write(file, std::uint16_t(EoDb::kLinePrimitive));

  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  m_line.Write(file);

  return true;
}
