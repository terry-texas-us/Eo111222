#include "stdafx.h"
#include <Windows.h>
#include <algorithm>
#include <atltypes.h>
#include <cmath>
#include <cstdlib>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPoint.h"
#include "EoDbPrimitive.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeVector3d.h"
#include "PrimState.h"

EoDbPoint::EoDbPoint() {
  m_PenColor = 1;
  m_pointStyle = 0;  // single pixel
  m_Point = EoGePoint3d::kOrigin;
  m_NumberOfDatums = 0;
  m_Data = nullptr;
}
EoDbPoint::EoDbPoint(const EoGePoint3d& point) : m_Point(point) {
  m_PenColor = 1;
  m_pointStyle = 0;  // single pixel
  m_NumberOfDatums = 0;
  m_Data = nullptr;
}
EoDbPoint::EoDbPoint(EoInt16 penColor, EoInt16 pointStyle, const EoGePoint3d& point) : m_Point(point) {
  m_PenColor = penColor;
  m_pointStyle = pointStyle;
  m_NumberOfDatums = 0;
  m_Data = nullptr;
}
EoDbPoint::EoDbPoint(EoInt16 penColor, EoInt16 pointStyle, const EoGePoint3d& point, EoUInt16 numberOfDatums, double* data) : m_Point(point) {
  m_PenColor = penColor;
  m_pointStyle = pointStyle;
  m_NumberOfDatums = numberOfDatums;
  m_Data = (numberOfDatums == 0) ? nullptr : new double[numberOfDatums];

  for (EoUInt16 n = 0; n < numberOfDatums; n++) { m_Data[n] = data[n]; }
}
EoDbPoint::EoDbPoint(const EoDbPoint& src) {
  m_PenColor = src.m_PenColor;
  m_pointStyle = src.m_pointStyle;
  m_Point = src.m_Point;
  m_NumberOfDatums = src.m_NumberOfDatums;
  m_Data = (m_NumberOfDatums == 0) ? nullptr : new double[m_NumberOfDatums];

  for (EoUInt16 n = 0; n < m_NumberOfDatums; n++) { m_Data[n] = src.m_Data[n]; }
}
EoDbPoint::~EoDbPoint() {
  if (m_NumberOfDatums != 0) delete[] m_Data;
}
const EoDbPoint& EoDbPoint::operator=(const EoDbPoint& src) {
  m_PenColor = src.m_PenColor;
  m_pointStyle = src.m_pointStyle;
  
  m_Point = src.m_Point;
  if (m_NumberOfDatums != src.m_NumberOfDatums) {
    if (m_NumberOfDatums != 0) delete[] m_Data;

    m_NumberOfDatums = src.m_NumberOfDatums;

    m_Data = (m_NumberOfDatums == 0) ? 0 : new double[m_NumberOfDatums];
  }
  for (EoUInt16 n = 0; n < m_NumberOfDatums; n++) { m_Data[n] = src.m_Data[n]; }
  return (*this);
}
void EoDbPoint::AddToTreeViewControl(HWND hTree, HTREEITEM hParent) { tvAddItem(hTree, hParent, const_cast<LPWSTR>(L"<Point>"), this); }
EoDbPrimitive*& EoDbPoint::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbPoint(*this);
  return (primitive);
}

void EoDbPoint::Display(AeSysView* view, CDC* context) {
  EoInt16 penColor = LogicalPenColor();

  COLORREF hotPenColor = app.PenColorsGetHot(penColor);

  EoGePoint4d pt(m_Point);
  view->ModelViewTransformPoint(pt);

  if (!pt.IsInView()) { return; }

  auto point = view->DoProjection(pt);

  // Compute pixel size for mark
  double markSize = AeSysDoc::GetDoc()->GetPointSize();

  int pixelSize = 8;  // default used if markSize == 0.0
  if (markSize > 0.0) {
    // offset point in world by markSize along X
    EoGePoint3d offset = m_Point + EoGeVector3d(markSize, 0.0, 0.0);
    EoGePoint4d offset4(offset);
    view->ModelTransformPoint(offset4);
    auto offScreen = view->DoProjection(offset4);
    int px = abs(offScreen.x - point.x);
    pixelSize = std::max(1, px);
  } else if (markSize < 0.0) {
    // treat absolute pixels (common convention)
    pixelSize = static_cast<int>(fabs(markSize));
  }

  int i;
  switch (m_pointStyle & 0x0F) {  // Low nibble defines basic shape
    
    case 0:  // single pixel
      context->SetPixel(point, hotPenColor);
      break;

    case 1:  // no visible mark
      break;

    case 2:  // small +
      for (i = -pixelSize; i <= pixelSize; i++) {
        context->SetPixel(point.x + i, point.y, hotPenColor);
        context->SetPixel(point.x, point.y + i, hotPenColor);
      }
      break;
    
    case 4:  // small |
      for (i = -pixelSize; i <= pixelSize; i++) { context->SetPixel(point.x, point.y + i, hotPenColor); }
      break;

    default:  // small X
      for (i = -pixelSize; i <= pixelSize; i++) {
        context->SetPixel(point.x + i, point.y - i, hotPenColor);
        context->SetPixel(point.x + i, point.y + i, hotPenColor);
      }
  }
  if (m_pointStyle & 0x20) {  // bit 5 set, draw a circle around the basic shape
    CPen pen(PS_SOLID, 1, hotPenColor);
    CPen* oldPen = context->SelectObject(&pen);
    auto* oldBrush = static_cast<CBrush*>(context->SelectStockObject(NULL_BRUSH));
    context->Ellipse(point.x - pixelSize, point.y - pixelSize, point.x + pixelSize, point.y + pixelSize);
    context->SelectObject(oldPen);
    context->SelectObject(oldBrush);
  }
  if (m_pointStyle & 0x40) {  // bit 6 set, draw a square around the basic shape
    for (i = -pixelSize; i <= pixelSize; i++) {
      context->SetPixel(point.x + i, point.y - pixelSize, hotPenColor);
      context->SetPixel(point.x + i, point.y + pixelSize, hotPenColor);
      context->SetPixel(point.x - pixelSize, point.y + i, hotPenColor);
      context->SetPixel(point.x + pixelSize, point.y + i, hotPenColor);
    }
  }
}

void EoDbPoint::AddReportToMessageList(EoGePoint3d) {
  CString str;
  str.Format(L"<Point> Color: %s Line Type: %s", FormatPenColor().GetString(), FormatLineType().GetString());
  app.AddStringToMessageList(str);
}
void EoDbPoint::FormatExtra(CString& str) { str.Format(L"Color;%s\tStyle;%d", FormatPenColor().GetString(), m_pointStyle); }
void EoDbPoint::FormatGeometry(CString& str) { str += L"Point;" + m_Point.ToString(); }
EoGePoint3d EoDbPoint::GetCtrlPt() { return (m_Point); }
void EoDbPoint::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  EoGePoint3d pt(m_Point);

  view->ModelTransformPoint(pt);
  pt = tm * pt;
  ptMin = EoGePoint3d::Min(ptMin, pt);
  ptMax = EoGePoint3d::Max(ptMax, pt);
}
bool EoDbPoint::IsInView(AeSysView* view) {
  EoGePoint4d pt(m_Point);

  view->ModelViewTransformPoint(pt);

  return (pt.IsInView());
}
EoGePoint3d EoDbPoint::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d pt(m_Point);
  view->ModelViewTransformPoint(pt);

  sm_ControlPointIndex = (point.DistanceToPointXY(pt) < sm_SelectApertureSize) ? 0U : USHRT_MAX;
  return (sm_ControlPointIndex == 0) ? m_Point : EoGePoint3d::kOrigin;
}
bool EoDbPoint::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  EoGePoint4d pt(m_Point);

  view->ModelViewTransformPoint(pt);

  ptProj = pt;

  return (point.DistanceToPointXY(pt) <= view->SelectApertureSize()) ? true : false;
}
bool EoDbPoint::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  EoGePoint4d pt(m_Point);
  view->ModelViewTransformPoint(pt);

  return ((pt.x >= pt1.x && pt.x <= pt2.x && pt.y >= pt1.y && pt.y <= pt2.y) ? true : false);
}
bool EoDbPoint::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d pt(m_Point);
  view->ModelViewTransformPoint(pt);

  return ((point.DistanceToPointXY(pt) < sm_SelectApertureSize) ? true : false);
}
void EoDbPoint::ModifyState() {
  EoDbPrimitive::ModifyState();
  m_pointStyle = pstate.PointStyle();
}
void EoDbPoint::SetDat(EoUInt16 wDats, double* dDat) {
  if (m_NumberOfDatums != wDats) {
    if (m_NumberOfDatums != 0) { delete[] m_Data; }
    m_NumberOfDatums = wDats;
    m_Data = (m_NumberOfDatums == 0) ? 0 : new double[m_NumberOfDatums];
  }
  for (EoUInt16 w = 0; w < m_NumberOfDatums; w++) { m_Data[w] = dDat[w]; }
}
void EoDbPoint::Transform(EoGeTransformMatrix& tm) { m_Point = tm * m_Point; }
void EoDbPoint::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if (mask != 0) m_Point += v;
}
bool EoDbPoint::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kPointPrimitive));
  EoDb::Write(file, m_PenColor);
  EoDb::Write(file, m_pointStyle);
  m_Point.Write(file);
  EoDb::Write(file, m_NumberOfDatums);
  for (EoUInt16 w = 0; w < m_NumberOfDatums; w++) EoDb::Write(file, m_Data[w]);

  return true;
}
