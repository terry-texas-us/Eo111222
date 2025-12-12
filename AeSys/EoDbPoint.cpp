#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

EoDbPoint::EoDbPoint() {
  m_PenColor = 1;
  m_PointStyle = 1;
  m_Point = EoGePoint3d::kOrigin;
  m_NumberOfDatums = 0;
  m_Data = 0;
}
EoDbPoint::EoDbPoint(const EoGePoint3d& point) : m_Point(point) {
  m_PenColor = 1;
  m_PointStyle = 1;
  m_NumberOfDatums = 0;
  m_Data = 0;
}
EoDbPoint::EoDbPoint(EoInt16 penColor, EoInt16 pointStyle, const EoGePoint3d& point) : m_Point(point) {
  m_PenColor = penColor;
  m_PointStyle = pointStyle;
  m_NumberOfDatums = 0;
  m_Data = 0;
}
EoDbPoint::EoDbPoint(EoInt16 penColor, EoInt16 pointStyle, const EoGePoint3d& point, EoUInt16 numberOfDatums,
                     double* data)
    : m_Point(point) {
  m_PenColor = penColor;
  m_PointStyle = pointStyle;
  m_NumberOfDatums = numberOfDatums;
  m_Data = (numberOfDatums == 0) ? 0 : new double[numberOfDatums];

  for (EoUInt16 n = 0; n < numberOfDatums; n++) { m_Data[n] = data[n]; }
}
EoDbPoint::EoDbPoint(const EoDbPoint& src) {
  m_PenColor = src.m_PenColor;
  m_PointStyle = src.m_PointStyle;
  m_Point = src.m_Point;
  m_NumberOfDatums = src.m_NumberOfDatums;
  m_Data = (m_NumberOfDatums == 0) ? 0 : new double[m_NumberOfDatums];

  for (EoUInt16 n = 0; n < m_NumberOfDatums; n++) { m_Data[n] = src.m_Data[n]; }
}
EoDbPoint::~EoDbPoint() {
  if (m_NumberOfDatums != 0) delete[] m_Data;
}
const EoDbPoint& EoDbPoint::operator=(const EoDbPoint& src) {
  m_PenColor = src.m_PenColor;
  m_PointStyle = src.m_PointStyle;
  ;
  m_Point = src.m_Point;
  if (m_NumberOfDatums != src.m_NumberOfDatums) {
    if (m_NumberOfDatums != 0) delete[] m_Data;

    m_NumberOfDatums = src.m_NumberOfDatums;

    m_Data = (m_NumberOfDatums == 0) ? 0 : new double[m_NumberOfDatums];
  }
  for (EoUInt16 n = 0; n < m_NumberOfDatums; n++) { m_Data[n] = src.m_Data[n]; }
  return (*this);
}
void EoDbPoint::AddToTreeViewControl(HWND hTree, HTREEITEM hParent) { tvAddItem(hTree, hParent, L"<Point>", this); }
EoDbPrimitive*& EoDbPoint::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbPoint(*this);
  return (primitive);
}
void EoDbPoint::Display(AeSysView* view, CDC* deviceContext) {
  EoInt16 PenColor = LogicalPenColor();

  COLORREF HotPenColor = app.PenColorsGetHot(PenColor);

  EoGePoint4d pt(m_Point);
  view->ModelViewTransformPoint(pt);

  if (pt.IsInView()) {
    CPoint pnt = view->DoProjection(pt);

    int i;
    switch (m_PointStyle) {
      case 0:  // 3 pixel plus
        for (i = -1; i <= 1; i++) {
          deviceContext->SetPixel(pnt.x + i, pnt.y, HotPenColor);
          deviceContext->SetPixel(pnt.x, pnt.y + i, HotPenColor);
        }
        break;

      case 1:  // 5 pixel plus
        for (i = -2; i <= 2; i++) {
          deviceContext->SetPixel(pnt.x + i, pnt.y, HotPenColor);
          deviceContext->SetPixel(pnt.x, pnt.y + i, HotPenColor);
        }
        break;

      case 2:  // 9 pixel plus
        for (i = -4; i <= 4; i++) {
          deviceContext->SetPixel(pnt.x + i, pnt.y, HotPenColor);
          deviceContext->SetPixel(pnt.x, pnt.y + i, HotPenColor);
        }
        break;

      case 3:  // 9 pixel cross
        for (i = -4; i <= 4; i++) {
          deviceContext->SetPixel(pnt.x + i, pnt.y - i, HotPenColor);
          deviceContext->SetPixel(pnt.x + i, pnt.y + i, HotPenColor);
        }
        break;

      default:  // 5 pixel square
        for (int Row = -2; Row <= 2; Row++) {
          for (int Col = -2; Col <= 2; Col++) {
            if (abs(Row) == 2 || abs(Col) == 2) { deviceContext->SetPixel(pnt.x + Col, pnt.y + Row, HotPenColor); }
          }
        }
    }
  }
}
void EoDbPoint::AddReportToMessageList(EoGePoint3d) {
  CString str;
  str.Format(L"<Point> Color: %s Line Type: %s", FormatPenColor().GetString(), FormatLineType().GetString());
  app.AddStringToMessageList(str);
}
void EoDbPoint::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%d", FormatPenColor().GetString(), m_PointStyle);
}
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
  m_PointStyle = pstate.PointStyle();
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
  EoDb::Write(file, m_PointStyle);
  m_Point.Write(file);
  EoDb::Write(file, m_NumberOfDatums);
  for (EoUInt16 w = 0; w < m_NumberOfDatums; w++) EoDb::Write(file, m_Data[w]);

  return true;
}
