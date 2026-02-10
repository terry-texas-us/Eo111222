#include "Stdafx.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbPoint.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderState.h"

EoDbPoint::EoDbPoint() {
  m_color = 1;
  m_pointStyle = 0;  // single pixel
  m_Point = EoGePoint3d::kOrigin;
  m_NumberOfDatums = 0;
  m_Data = nullptr;
}
EoDbPoint::EoDbPoint(const EoGePoint3d& point) : m_Point(point) {
  m_color = 1;
  m_pointStyle = 0;  // single pixel
  m_NumberOfDatums = 0;
  m_Data = nullptr;
}
EoDbPoint::EoDbPoint(std::int16_t penColor, std::int16_t pointStyle, const EoGePoint3d& point) : m_Point(point) {
  m_color = penColor;
  m_pointStyle = pointStyle;
  m_NumberOfDatums = 0;
  m_Data = nullptr;
}
EoDbPoint::EoDbPoint(std::int16_t penColor, std::int16_t pointStyle, const EoGePoint3d& point, std::uint16_t numberOfDatums,
                     double* data)
    : m_Point(point) {
  m_color = penColor;
  m_pointStyle = pointStyle;
  m_NumberOfDatums = numberOfDatums;
  m_Data = (numberOfDatums == 0) ? nullptr : new double[numberOfDatums];

  for (std::uint16_t n = 0; n < numberOfDatums; n++) { m_Data[n] = data[n]; }
}
EoDbPoint::EoDbPoint(const EoDbPoint& src) {
  m_color = src.m_color;
  m_pointStyle = src.m_pointStyle;
  m_Point = src.m_Point;
  m_NumberOfDatums = src.m_NumberOfDatums;
  m_Data = (m_NumberOfDatums == 0) ? nullptr : new double[m_NumberOfDatums];

  for (std::uint16_t n = 0; n < m_NumberOfDatums; n++) { m_Data[n] = src.m_Data[n]; }
}
EoDbPoint::~EoDbPoint() {
  if (m_NumberOfDatums != 0) delete[] m_Data;
}
const EoDbPoint& EoDbPoint::operator=(const EoDbPoint& src) {
  m_color = src.m_color;
  m_pointStyle = src.m_pointStyle;

  m_Point = src.m_Point;
  if (m_NumberOfDatums != src.m_NumberOfDatums) {
    if (m_NumberOfDatums != 0) delete[] m_Data;

    m_NumberOfDatums = src.m_NumberOfDatums;

    m_Data = (m_NumberOfDatums == 0) ? 0 : new double[m_NumberOfDatums];
  }
  for (std::uint16_t n = 0; n < m_NumberOfDatums; n++) { m_Data[n] = src.m_Data[n]; }
  return (*this);
}
void EoDbPoint::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<Point>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbPoint::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbPoint(*this);
  return primitive;
}

void EoDbPoint::Display(AeSysView* view, CDC* context) {
  std::int16_t color = LogicalColor();

  COLORREF hotPenColor = app.PenColorsGetHot(color);

  EoGePoint4d pt(m_Point);
  view->ModelViewTransformPoint(pt);

  if (!pt.IsInView()) { return; }

  auto point = view->DoProjection(pt);

  // Compute pixel size for point
  double pointSize = AeSysDoc::GetDoc()->GetPointSize();

  int pixelSize = 8;  // default used if markSize == 0.0
  if (pointSize > 0.0) {
    auto dpi = static_cast<double>(GetDpiForSystem());
    pixelSize = static_cast<int>(std::max(2.0, (pointSize * dpi)) / 2.0);
  } else if (pointSize < 0.0) {
    // treat absolute pixels (common convention)
    pixelSize = static_cast<int>(std::max(2.0, fabs(pointSize)) / 2.0);
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

void EoDbPoint::AddReportToMessageList(const EoGePoint3d&) {
  CString str;
  str.Format(L"<Point> Color: %s Line Type: %s", FormatPenColor().GetString(), FormatLineType().GetString());
  app.AddStringToMessageList(str);
}
void EoDbPoint::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%d", FormatPenColor().GetString(), m_pointStyle);
}
void EoDbPoint::FormatGeometry(CString& str) { str += L"Point;" + m_Point.ToString(); }
EoGePoint3d EoDbPoint::GetControlPoint() { return m_Point; }

void EoDbPoint::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_Point);
}

void EoDbPoint::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3d pt(m_Point);

  view->ModelTransformPoint(pt);
  pt = transformMatrix * pt;
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

  sm_controlPointIndex = (point.DistanceToPointXY(pt) < sm_SelectApertureSize) ? 0 : SHRT_MAX;
  return (sm_controlPointIndex == 0) ? m_Point : EoGePoint3d::kOrigin;
}

bool EoDbPoint::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) {
  (void)view;
  (void)line;
  return false;
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
  m_pointStyle = renderState.PointStyle();
}
void EoDbPoint::SetDat(std::uint16_t wDats, double* dDat) {
  if (m_NumberOfDatums != wDats) {
    if (m_NumberOfDatums != 0) { delete[] m_Data; }
    m_NumberOfDatums = wDats;
    m_Data = (m_NumberOfDatums == 0) ? 0 : new double[m_NumberOfDatums];
  }
  for (auto i = 0; i < m_NumberOfDatums; i++) { m_Data[i] = dDat[i]; }
}

void EoDbPoint::SetPoint(double x, double y, double z) {
  m_Point.x = x;
  m_Point.y = y;
  m_Point.z = z;
}

void EoDbPoint::Transform(const EoGeTransformMatrix& transformMatrix) { m_Point = transformMatrix * m_Point; }
void EoDbPoint::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if (mask != 0) m_Point += v;
}
bool EoDbPoint::Write(CFile& file) {
  EoDb::Write(file, std::uint16_t(EoDb::kPointPrimitive));
  EoDb::Write(file, m_color);
  EoDb::Write(file, m_pointStyle);
  m_Point.Write(file);
  EoDb::Write(file, m_NumberOfDatums);
  for (auto i = 0; i < m_NumberOfDatums; i++) { EoDb::Write(file, m_Data[i]); }

  return true;
}
