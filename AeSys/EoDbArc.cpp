#include "Stdafx.h"

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbArc.h"
#include "EoDbCircle.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "PrimState.h"

EoDbArc::EoDbArc(const EoGePoint3d& center, double radius, double startAngle, double endAngle,
                 const EoGeVector3d& extrusion)
    : EoDbCircle(pstate.PenColor(), pstate.LineType(), center, radius, extrusion),
      m_startAngle(startAngle),
      m_endAngle(endAngle) {}

EoDbArc::EoDbArc(EoInt16 color, EoInt16 lineTypeIndex, const EoGePoint3d& center, double radius, double startAngle,
                 double endAngle, const EoGeVector3d& extrusion)
    : EoDbCircle(color, lineTypeIndex, center, radius, extrusion), m_startAngle(startAngle), m_endAngle(endAngle) {}

EoDbArc::EoDbArc(const EoDbArc& other)
    : EoDbCircle(other), m_startAngle(other.m_startAngle), m_endAngle(other.m_endAngle) {}

const EoDbArc& EoDbArc::operator=(const EoDbArc& other) {
  if (this != &other) {
    EoDbCircle::operator=(other);
    m_startAngle = other.m_startAngle;
    m_endAngle = other.m_endAngle;
  }
  return *this;
}

void EoDbArc::AddToTreeViewControl(HWND hTree, HTREEITEM hParent) {
  CString label{L"<Arc>"};
  tvAddItem(hTree, hParent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbArc::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbArc(*this);
  return primitive;
}

void EoDbArc::Display(AeSysView* view, CDC* deviceContext) {
  // Simple display: draw as polyline arc using existing polyline helpers similar to ellipse
  auto color = LogicalColor();
  auto lineType = LogicalLineType();
  pstate.SetPen(view, deviceContext, color, lineType);

  // Use a simple fan of points on the circle/arc in model space
  double sweep = m_endAngle - m_startAngle;
  if (fabs(sweep) <= DBL_EPSILON) return;

  int n = std::max(2, int(fabs(Eo::Round(sweep / Eo::TwoPi * 32.0))));
  polyline::BeginLineStrip();
  for (int i = 0; i <= n; ++i) {
    double t = double(i) / double(n);
    double ang = m_startAngle + sweep * t;
    EoGePoint3d pt(m_center.x + m_radius * cos(ang), m_center.y + m_radius * sin(ang), m_center.z);
    polyline::SetVertex(pt);
  }
  polyline::__End(view, deviceContext, lineType);
}

void EoDbArc::AddReportToMessageList(EoGePoint3d) {
  CString str;
  str.Format(L"<Arc> Color: %s Line Type: %s Radius: %f", FormatPenColor().GetString(), FormatLineType().GetString(),
             m_radius);
  app.AddStringToMessageList(str);
}

void EoDbArc::FormatGeometry(CString& str) {
  str += L"Center Point;" + m_center.ToString();
  str += L"Radius;" + CString(std::to_wstring(m_radius).c_str());
  str += L"Start Angle;" + CString(std::to_wstring(m_startAngle).c_str());
  str += L"End Angle;" + CString(std::to_wstring(m_endAngle).c_str());
}

void EoDbArc::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s\tStart;%f\tEnd;%f", FormatPenColor().GetString(), FormatLineType().GetString(),
             m_startAngle, m_endAngle);
}

void EoDbArc::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_center);
}

EoGePoint3d EoDbArc::GetControlPoint() { return m_center; }

void EoDbArc::GetExtents(AeSysView* view, EoGePoint3d& minPt, EoGePoint3d& maxPt, EoGeTransformMatrix& tm) {
  EoGePoint3d pMin(m_center.x - m_radius, m_center.y - m_radius, m_center.z);
  EoGePoint3d pMax(m_center.x + m_radius, m_center.y + m_radius, m_center.z);
  view->ModelTransformPoint(pMin);
  view->ModelTransformPoint(pMax);
  pMin = tm * pMin;
  pMax = tm * pMax;
  minPt = EoGePoint3d::Min(minPt, pMin);
  maxPt = EoGePoint3d::Max(maxPt, pMax);
}

EoGePoint3d EoDbArc::GoToNextControlPoint() { return EoGePoint3d(m_center.x + m_radius, m_center.y, m_center.z); }

bool EoDbArc::Identical(EoDbPrimitive* p) {
  if (!p || !p->Is(EoDb::kEllipsePrimitive)) return false;
  auto* other = static_cast<EoDbArc*>(p);
  return (m_center == other->m_center && fabs(m_radius - other->m_radius) < DBL_EPSILON &&
          fabs(m_startAngle - other->m_startAngle) < DBL_EPSILON && fabs(m_endAngle - other->m_endAngle) < DBL_EPSILON);
}

bool EoDbArc::Is(EoUInt16 type) { return type == EoDb::kEllipsePrimitive; }

bool EoDbArc::IsInView(AeSysView* view) {
  EoGePoint3d p = m_center;
  view->ModelTransformPoint(p);
  // rudimentary test: check if bounding box intersects view frustum (not implemented fully)
  return true;
}

bool EoDbArc::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d ptBeg(m_center.x + m_radius * cos(m_startAngle), m_center.y + m_radius * sin(m_startAngle), m_center.z,
                    1.0);
  view->ModelViewTransformPoint(ptBeg);
  if (point.DistanceToPointXY(ptBeg) < sm_SelectApertureSize) return true;
  EoGePoint4d ptEnd(m_center.x + m_radius * cos(m_endAngle), m_center.y + m_radius * sin(m_endAngle), m_center.z, 1.0);
  view->ModelViewTransformPoint(ptEnd);
  if (point.DistanceToPointXY(ptEnd) < sm_SelectApertureSize) return true;
  return false;
}

EoGePoint3d EoDbArc::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_ControlPointIndex = USHRT_MAX;
  double dAPert = sm_SelectApertureSize;
  EoGePoint3d pts[2]{
      EoGePoint3d(m_center.x + m_radius * cos(m_startAngle), m_center.y + m_radius * sin(m_startAngle), m_center.z),
      EoGePoint3d(m_center.x + m_radius * cos(m_endAngle), m_center.y + m_radius * sin(m_endAngle), m_center.z)};
  for (EoUInt16 w = 0; w < 2; ++w) {
    EoGePoint4d pt(pts[w]);
    view->ModelViewTransformPoint(pt);
    double dDis = point.DistanceToPointXY(pt);
    if (dDis < dAPert) {
      sm_ControlPointIndex = w;
      dAPert = dDis;
    }
  }
  return (sm_ControlPointIndex == USHRT_MAX) ? EoGePoint3d::kOrigin : pts[sm_ControlPointIndex];
}

bool EoDbArc::SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray& /* intersections */) {
  return false;
}

bool EoDbArc::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& outPt) {
  // project point to XY and test distance to circle
  EoGePoint4d p = point;
  view->ModelViewTransformPoint(p);
  double d = sqrt((p.x - m_center.x) * (p.x - m_center.x) + (p.y - m_center.y) * (p.y - m_center.y));
  if (fabs(d - m_radius) < sm_SelectApertureSize) {
    outPt = EoGePoint3d(p.x, p.y, m_center.z);
    return true;
  }
  return false;
}

bool EoDbArc::SelectUsingRectangle(AeSysView* /* view */, EoGePoint3d a, EoGePoint3d b) {
  // bounding box test
  double minx = std::min(a.x, b.x);
  double maxx = std::max(a.x, b.x);
  double miny = std::min(a.y, b.y);
  double maxy = std::max(a.y, b.y);
  if (m_center.x + m_radius < minx || m_center.x - m_radius > maxx) return false;
  if (m_center.y + m_radius < miny || m_center.y - m_radius > maxy) return false;
  return true;
}

void EoDbArc::Transform(EoGeTransformMatrix& tm) { m_center = tm * m_center; }

void EoDbArc::Translate(EoGeVector3d translate) { m_center += translate; }

void EoDbArc::TranslateUsingMask(EoGeVector3d translate, const DWORD mask) {
  if (mask != 0) m_center += translate;
}

bool EoDbArc::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kEllipsePrimitive));
  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  m_center.Write(file);
  EoDb::Write(file, m_radius);
  EoDb::Write(file, m_startAngle);
  EoDb::Write(file, m_endAngle);
  // write extrusion
  m_extrusion.Write(file);
  return true;
}

void EoDbArc::Write(CFile& /* file */, EoUInt8* /* buffer */) {
  // Stub: provide binary write if required by older code paths
}

void EoDbArc::CutAtPt(EoGePoint3d& /* point */, EoDbGroup* /* group */) {}

void EoDbArc::CutAt2Pts(EoGePoint3d* /* points */, EoDbGroupList* /* groups */, EoDbGroupList* /* newGroups */) {
  // Stub
}

int EoDbArc::IsWithinArea(EoGePoint3d /* lowerLeftCorner */, EoGePoint3d /* upperRightCorner */,
                          EoGePoint3d* intersectingPoints) {
  // Stub: implement proper intersection logic if needed
  intersectingPoints = nullptr;
  return 0;
}
