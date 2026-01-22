#include "Stdafx.h"
#include <Windows.h>
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <string>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
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

EoDbCircle::EoDbCircle(const EoGePoint3d& center, double radius, const EoGeVector3d& extrusion)
    : EoDbPrimitive(pstate.PenColor(), pstate.LineType()), m_center(center), m_radius(radius), m_extrusion(extrusion) {}

EoDbCircle::EoDbCircle(EoInt16 color, EoInt16 lineTypeIndex, const EoGePoint3d& center, double radius,
                       const EoGeVector3d& extrusion)
    : EoDbPrimitive(color, lineTypeIndex), m_center(center), m_radius(radius), m_extrusion(extrusion) {}

EoDbCircle::EoDbCircle(const EoDbCircle& other)
    : EoDbPrimitive(other), m_center(other.m_center), m_radius(other.m_radius), m_extrusion(other.m_extrusion) {}

const EoDbCircle& EoDbCircle::operator=(const EoDbCircle& other) {
  if (this != &other) {
    EoDbPrimitive::operator=(other);
    m_center = other.m_center;
    m_radius = other.m_radius;
    m_extrusion = other.m_extrusion;
  }
  return *this;
}

void EoDbCircle::AddToTreeViewControl(HWND hTree, HTREEITEM hParent) {
  CString label{L"<Circle>"};
  tvAddItem(hTree, hParent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbCircle::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbCircle(*this);
  return primitive;
}

void EoDbCircle::Display(AeSysView* view, CDC* deviceContext) {
  auto color = LogicalColor();
  auto lineType = LogicalLineType();
  pstate.SetPen(view, deviceContext, color, lineType);

  int n = 32;
  polyline::BeginLineStrip();
  for (int i = 0; i <= n; ++i) {
    double t = double(i) / double(n);
    double ang = Eo::TwoPi * t;
    EoGePoint3d pt(m_center.x + m_radius * cos(ang), m_center.y + m_radius * sin(ang), m_center.z);
    polyline::SetVertex(pt);
  }
  polyline::__End(view, deviceContext, lineType);
}

void EoDbCircle::AddReportToMessageList(EoGePoint3d) {
  CString str;
  str.Format(L"<Circle> Color: %s Line Type: %s Radius: %f", FormatPenColor().GetString(), FormatLineType().GetString(),
             m_radius);
  app.AddStringToMessageList(str);
}

void EoDbCircle::FormatGeometry(CString& str) {
  str += L"Center Point;" + m_center.ToString();
  str += L"Radius;" + CString(std::to_wstring(m_radius).c_str());
}

void EoDbCircle::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s", FormatPenColor().GetString(), FormatLineType().GetString());
}

void EoDbCircle::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_center);
}

EoGePoint3d EoDbCircle::GetControlPoint() { return m_center; }

void EoDbCircle::GetExtents(AeSysView* view, EoGePoint3d& minPt, EoGePoint3d& maxPt, EoGeTransformMatrix& tm) {
  EoGePoint3d pMin(m_center.x - m_radius, m_center.y - m_radius, m_center.z);
  EoGePoint3d pMax(m_center.x + m_radius, m_center.y + m_radius, m_center.z);
  view->ModelTransformPoint(pMin);
  view->ModelTransformPoint(pMax);
  pMin = tm * pMin;
  pMax = tm * pMax;
  minPt = EoGePoint3d::Min(minPt, pMin);
  maxPt = EoGePoint3d::Max(maxPt, pMax);
}

EoGePoint3d EoDbCircle::GoToNextControlPoint() { return EoGePoint3d(m_center.x + m_radius, m_center.y, m_center.z); }

bool EoDbCircle::Identical(EoDbPrimitive* p) {
  if (!p || !p->Is(EoDb::kCirclePrimitive)) { return false; }
  auto* other = static_cast<EoDbCircle*>(p);
  return (m_center == other->m_center && fabs(m_radius - other->m_radius) < DBL_EPSILON &&
          m_extrusion == other->m_extrusion);
}

bool EoDbCircle::Is(EoUInt16 type) { return type == EoDb::kEllipsePrimitive; }

bool EoDbCircle::IsInView(AeSysView* view) {
  EoGePoint3d p = m_center;
  view->ModelTransformPoint(p);
  // rudimentary test: check if bounding box intersects view frustum (not implemented fully)
  return true;
}

bool EoDbCircle::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d ptBeg(m_center.x + m_radius, m_center.y + m_radius, m_center.z, 1.0);
  view->ModelViewTransformPoint(ptBeg);
  if (point.DistanceToPointXY(ptBeg) < sm_SelectApertureSize) return true;
  EoGePoint4d ptEnd(m_center.x + m_radius, m_center.y + m_radius, m_center.z, 1.0);
  view->ModelViewTransformPoint(ptEnd);
  if (point.DistanceToPointXY(ptEnd) < sm_SelectApertureSize) return true;
  return false;
}

EoGePoint3d EoDbCircle::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_ControlPointIndex = USHRT_MAX;
  double dAPert = sm_SelectApertureSize;
  EoGePoint3d pts[2]{
      EoGePoint3d(m_center.x + m_radius * cos(0.0), m_center.y + m_radius * sin(0.0), m_center.z),
      EoGePoint3d(m_center.x + m_radius * cos(Eo::TwoPi), m_center.y + m_radius * sin(Eo::TwoPi), m_center.z)};
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

bool EoDbCircle::SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray& /* intersections */) {
  return false;
}

bool EoDbCircle::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& outPt) {
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

bool EoDbCircle::SelectUsingRectangle(AeSysView* /* view */, EoGePoint3d a, EoGePoint3d b) {
  // bounding box test
  double minx = std::min(a.x, b.x);
  double maxx = std::max(a.x, b.x);
  double miny = std::min(a.y, b.y);
  double maxy = std::max(a.y, b.y);
  if (m_center.x + m_radius < minx || m_center.x - m_radius > maxx) return false;
  if (m_center.y + m_radius < miny || m_center.y - m_radius > maxy) return false;
  return true;
}

void EoDbCircle::Transform(EoGeTransformMatrix& tm) { m_center = tm * m_center; }

void EoDbCircle::Translate(EoGeVector3d translate) { m_center += translate; }

void EoDbCircle::TranslateUsingMask(EoGeVector3d translate, const DWORD mask) {
  if (mask != 0) m_center += translate;
}

bool EoDbCircle::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kEllipsePrimitive));
  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  m_center.Write(file);
  EoDb::Write(file, m_radius);
  // write extrusion
  m_extrusion.Write(file);
  return true;
}

void EoDbCircle::Write(CFile& /* file */, EoUInt8* /* buffer */) {
  // Stub: provide binary write if required by older code paths
}

void EoDbCircle::CutAtPt(EoGePoint3d& /* point */, EoDbGroup* /* group */) {}

void EoDbCircle::CutAt2Pts(EoGePoint3d* /* points */, EoDbGroupList* /* groups */, EoDbGroupList* /* newGroups */) {
  // Stub
}

int EoDbCircle::IsWithinArea(EoGePoint3d /* lowerLeftCorner */, EoGePoint3d /* upperRightCorner */,
                             EoGePoint3d* intersectingPoints) {
  // Stub: implement proper intersection logic if needed
  intersectingPoints = nullptr;
  return 0;
}
