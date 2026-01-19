#include "Stdafx.h"
#include <Windows.h>
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>
#include <algorithm>
#include <atltypes.h>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdio>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "Hatch.h"
#include "PrimState.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif  // USING_DDE

EoInt16 EoDbPolygon::sm_SpecialPolygonStyle = -1;

EoUInt16 EoDbPolygon::sm_EdgeToEvaluate = 0;
EoUInt16 EoDbPolygon::sm_Edge = 0;
EoUInt16 EoDbPolygon::sm_PivotVertex = 0;

typedef struct tagFilAreaEdgLis {
  double dMinY;  // minimum y extent of edge
  double dMaxY;  // maximum y extent of edge
  double dX;     // x intersection on edge
  union {
    double dInvSlope;  // inverse slope of edge
    double dStepSiz;   // change in x for each scanline
  };
} pFilAreaEdgLis;

EoDbPolygon::EoDbPolygon() {
  m_InteriorStyle = EoDb::kHollow;
  m_InteriorStyleIndex = 1;
  m_NumberOfPoints = 0;
  m_Pt = 0;
  m_HatchOrigin = EoGePoint3d::kOrigin;
  m_vPosXAx = EoGeVector3d::kXAxis;
  m_vPosYAx = EoGeVector3d::kYAxis;
}

EoDbPolygon::EoDbPolygon(EoGePoint3dArray& points) {
  m_PenColor = pstate.PenColor();
  m_InteriorStyle = pstate.PolygonIntStyle();
  m_InteriorStyleIndex = pstate.PolygonIntStyleId();

  m_HatchOrigin = points[0];

  m_NumberOfPoints = EoUInt16(points.GetSize());

  if (m_NumberOfPoints >= 3) {
    m_vPosXAx = EoGeVector3d(points[0], points[1]);
    m_vPosYAx = EoGeVector3d(points[0], points[2]);
    auto planeNormal = EoGeCrossProduct(m_vPosXAx, m_vPosYAx);
    planeNormal.Normalize();

    if (planeNormal.z < 0) planeNormal = -planeNormal;

    m_vPosXAx.Normalize();
    m_vPosXAx.RotAboutArbAx(planeNormal, hatch::dOffAng);
    m_vPosYAx = m_vPosXAx;
    m_vPosYAx.RotAboutArbAx(planeNormal, Eo::HalfPi);
    m_vPosXAx *= hatch::dXAxRefVecScal;
    m_vPosYAx *= hatch::dYAxRefVecScal;

    // Project reference origin to plane

    m_Pt = new EoGePoint3d[m_NumberOfPoints];

    for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) m_Pt[w] = points[w];
  }
}
EoDbPolygon::EoDbPolygon(EoUInt16 wPts, EoGePoint3d* pt) {
  m_PenColor = 0;
  m_InteriorStyle = EoDb::kSolid;
  m_InteriorStyleIndex = 0;
  m_NumberOfPoints = wPts;
  m_HatchOrigin = pt[0];
  m_vPosXAx = EoGeVector3d::kXAxis;
  m_vPosYAx = EoGeVector3d::kYAxis;
  m_Pt = new EoGePoint3d[m_NumberOfPoints];

  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) m_Pt[w] = pt[w];
}
EoDbPolygon::EoDbPolygon(EoUInt16 wPts, EoGePoint3d ptOrig, EoGeVector3d vXAx, EoGeVector3d vYAx, const EoGePoint3d* ppt) {
  m_PenColor = pstate.PenColor();
  m_InteriorStyle = pstate.PolygonIntStyle();
  m_InteriorStyleIndex = pstate.PolygonIntStyleId();
  m_NumberOfPoints = wPts;
  m_HatchOrigin = ptOrig;
  m_vPosXAx = vXAx;
  m_vPosYAx = vYAx;
  m_Pt = new EoGePoint3d[m_NumberOfPoints];

  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) m_Pt[w] = ppt[w];
}
EoDbPolygon::EoDbPolygon(EoGePoint3d& origin, EoGeVector3d& xAxis, EoGeVector3d& yAxis, EoGePoint3dArray& pts) {
  m_PenColor = pstate.PenColor();
  m_InteriorStyle = pstate.PolygonIntStyle();
  m_InteriorStyleIndex = pstate.PolygonIntStyleId();
  m_NumberOfPoints = EoUInt16(pts.GetSize());
  m_HatchOrigin = origin;
  m_vPosXAx = xAxis;
  m_vPosYAx = yAxis;
  m_Pt = new EoGePoint3d[m_NumberOfPoints];

  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) m_Pt[w] = pts[w];
}
EoDbPolygon::EoDbPolygon(EoInt16 penColor, EoInt16 style, EoInt16 styleIndex, EoGePoint3d& origin, EoGeVector3d& xAxis, EoGeVector3d& yAxis,
                         EoGePoint3dArray& points)
    : m_HatchOrigin(origin), m_vPosXAx(xAxis), m_vPosYAx(yAxis) {
  m_PenColor = penColor;
  m_InteriorStyle = style;
  m_InteriorStyleIndex = styleIndex;
  m_NumberOfPoints = EoUInt16(points.GetSize());
  m_Pt = new EoGePoint3d[m_NumberOfPoints];

  for (EoUInt16 n = 0; n < m_NumberOfPoints; n++) { m_Pt[n] = points[n]; }
}
EoDbPolygon::EoDbPolygon(const EoDbPolygon& src) {
  m_PenColor = src.m_PenColor;
  m_InteriorStyle = src.m_InteriorStyle;
  m_InteriorStyleIndex = src.m_InteriorStyleIndex;
  m_HatchOrigin = src.m_HatchOrigin;
  m_vPosXAx = src.m_vPosXAx;
  m_vPosYAx = src.m_vPosYAx;
  m_NumberOfPoints = src.m_NumberOfPoints;
  m_Pt = new EoGePoint3d[m_NumberOfPoints];
  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) m_Pt[w] = src.m_Pt[w];
}
const EoDbPolygon& EoDbPolygon::operator=(const EoDbPolygon& src) {
  m_PenColor = src.m_PenColor;
  m_InteriorStyle = src.m_InteriorStyle;
  m_InteriorStyleIndex = src.m_InteriorStyleIndex;
  m_HatchOrigin = src.m_HatchOrigin;
  m_vPosXAx = src.m_vPosXAx;
  m_vPosYAx = src.m_vPosYAx;

  if (m_NumberOfPoints != src.m_NumberOfPoints) {
    m_NumberOfPoints = src.m_NumberOfPoints;
    delete[] m_Pt;
    m_Pt = new EoGePoint3d[m_NumberOfPoints];
  }
  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) m_Pt[w] = src.m_Pt[w];

  return (*this);
}
EoDbPolygon::~EoDbPolygon() { delete[] m_Pt; }

void EoDbPolygon::AddToTreeViewControl(HWND tree, HTREEITEM parent) { 
  CString label{L"<Polygon>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbPolygon::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbPolygon(*this);
  return (primitive);
}

void EoDbPolygon::Display(AeSysView* view, CDC* deviceContext) {
  EoInt16 nPenColor = LogicalPenColor();

  pstate.SetPenColor(deviceContext, nPenColor);
  EoInt16 nPolygonStyle = sm_SpecialPolygonStyle == -1 ? m_InteriorStyle : sm_SpecialPolygonStyle;
  pstate.SetPolygonIntStyle(nPolygonStyle);  // hollow, solid, pattern, hatch
  pstate.SetPolygonIntStyleId(m_InteriorStyleIndex);

  int iPtLstsId = m_NumberOfPoints;

  if (m_InteriorStyle == EoDb::kHatch) {
    EoGeTransformMatrix tm(m_HatchOrigin, m_vPosXAx, m_vPosYAx);
    DisplayFilAreaHatch(view, deviceContext, tm, 1, &iPtLstsId, m_Pt);
  } else {  // Fill area interior style is hollow, solid or pattern
    EoGePoint4dArray PointsArray;

    PointsArray.SetSize(m_NumberOfPoints);

    for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) { PointsArray[w] = EoGePoint4d(m_Pt[w]); }
    view->ModelViewTransformPoints(PointsArray);
    EoGePoint4d::ClipPolygon(PointsArray);
    Polygon_Display(view, deviceContext, PointsArray);
  }
}
void EoDbPolygon::AddReportToMessageList(EoGePoint3d ptPic) {
  CString Message(L"<Polygon Edge> ");

  if (sm_Edge > 0 && sm_Edge <= m_NumberOfPoints) {
    EoGePoint3d* pBegPt = &m_Pt[sm_Edge - 1];
    EoGePoint3d* pEndPt = &m_Pt[sm_Edge % m_NumberOfPoints];

    if (sm_PivotVertex < m_NumberOfPoints) {
      pBegPt = &m_Pt[sm_PivotVertex];
      pEndPt = &m_Pt[SwingVertex()];
    }
    double dAng;
    double dLen = EoGeVector3d(*pBegPt, *pEndPt).Length();  // Length of edge

    if (EoGeVector3d(ptPic, *pBegPt).Length() > dLen * 0.5)
      dAng = EoGeLine(*pEndPt, *pBegPt).AngleFromXAxisXY();
    else
      dAng = EoGeLine(*pBegPt, *pEndPt).AngleFromXAxisXY();

    CString FormattedLength;
    app.FormatLength(FormattedLength, app.GetUnits(), dLen);
    Message.Append(FormattedLength.TrimLeft());
    wchar_t szBuf[24]{};
    swprintf_s(szBuf, 24, L" @ %6.2f degrees", Eo::RadianToDegree(dAng));
    Message.Append(szBuf);
    app.AddStringToMessageList(Message);

    app.SetEngagedLength(dLen);
    app.SetEngagedAngle(dAng);
#if defined(USING_DDE)
    dde::PostAdvise(dde::EngLenInfo);
    dde::PostAdvise(dde::EngAngZInfo);
#endif  // USING_DDE
  }
}
void EoDbPolygon::FormatGeometry(CString& str) {
  str += L"Hatch Origin;" + m_HatchOrigin.ToString();
  str += L"X Axis;" + m_vPosXAx.ToString();
  str += L"Y Axis;" + m_vPosYAx.ToString();

  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) { str += L"Vertex Point;" + m_Pt[w].ToString(); }
}

CString EoDbPolygon::FormatIntStyle() {
  CString strStyle[] = {L"Hollow", L"Solid", L"Pattern", L"Hatch"};

  CString str = (m_InteriorStyle >= 0 && m_InteriorStyle <= 3) ? strStyle[m_InteriorStyle] : const_cast<LPWSTR>(L"Invalid!");

  return (str);
}
void EoDbPolygon::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s\tPoints;%d", FormatPenColor().GetString(), FormatLineType().GetString(), m_NumberOfPoints);
}
EoGePoint3d EoDbPolygon::GetCtrlPt() {
  EoUInt16 wBeg = EoUInt16(sm_Edge - 1);
  EoUInt16 wEnd = EoUInt16(sm_Edge % m_NumberOfPoints);
  EoGePoint3d pt = EoGeLine(m_Pt[wBeg], m_Pt[wEnd]).Midpoint();
  return (pt);
};

EoGePoint3d EoDbPolygon::GoToNxtCtrlPt() {
  if (sm_PivotVertex >= m_NumberOfPoints) {  // have not yet rocked to a vertex
    EoUInt16 wBeg = EoUInt16(sm_Edge - 1);
    EoUInt16 wEnd = EoUInt16(sm_Edge % m_NumberOfPoints);

    if (m_Pt[wEnd].x > m_Pt[wBeg].x)
      sm_PivotVertex = wBeg;
    else if (m_Pt[wEnd].x < m_Pt[wBeg].x)
      sm_PivotVertex = wEnd;
    else if (m_Pt[wEnd].y > m_Pt[wBeg].y)
      sm_PivotVertex = wBeg;
    else
      sm_PivotVertex = wEnd;
  } else if (sm_PivotVertex == 0) {
    if (sm_Edge == 1)
      sm_PivotVertex = 1;
    else
      sm_PivotVertex = EoUInt16(m_NumberOfPoints - 1);
  } else if (sm_PivotVertex == EoUInt16(m_NumberOfPoints - 1)) {
    if (sm_Edge == m_NumberOfPoints)
      sm_PivotVertex = 0;
    else
      sm_PivotVertex--;
  } else {
    if (sm_Edge == sm_PivotVertex)
      sm_PivotVertex--;
    else
      sm_PivotVertex++;
  }
  return (m_Pt[sm_PivotVertex]);
}

bool EoDbPolygon::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  if (sm_EdgeToEvaluate > 0 && sm_EdgeToEvaluate <= m_NumberOfPoints) {  // Evaluate specified edge of polygon
    EoGePoint4d ptBeg(m_Pt[sm_EdgeToEvaluate - 1]);
    EoGePoint4d ptEnd(m_Pt[sm_EdgeToEvaluate % m_NumberOfPoints]);

    view->ModelViewTransformPoint(ptBeg);
    view->ModelViewTransformPoint(ptEnd);

    EoGeLine Edge(ptBeg, ptEnd);
    if (Edge.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
      ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
      return true;
    }
  } else {  // Evaluate entire polygon
    EoGePoint4d ptBeg(m_Pt[0]);
    view->ModelViewTransformPoint(ptBeg);

    for (EoUInt16 w = 1; w <= m_NumberOfPoints; w++) {
      EoGePoint4d ptEnd(m_Pt[w % m_NumberOfPoints]);
      view->ModelViewTransformPoint(ptEnd);

      EoGeLine Edge(ptBeg, ptEnd);
      if (Edge.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
        ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
        sm_Edge = w;
        sm_PivotVertex = m_NumberOfPoints;
        return true;
      }
      ptBeg = ptEnd;
    }
  }
  return false;
}

bool EoDbPolygon::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  EoGePoint3dArray pts;
  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) pts.Add(m_Pt[w]);

  return polyline::SelectUsingRectangle(view, pt1, pt2, pts);
}
void EoDbPolygon::ModifyState() {
  EoDbPrimitive::ModifyState();

  m_InteriorStyle = pstate.PolygonIntStyle();
  m_InteriorStyleIndex = pstate.PolygonIntStyleId();
}
bool EoDbPolygon::PvtOnCtrlPt(AeSysView* view, const EoGePoint4d& ptView) {
  if (sm_PivotVertex >= m_NumberOfPoints)
    // Not engaged at a vertex
    return false;

  EoGePoint4d ptCtrl(m_Pt[sm_PivotVertex]);
  view->ModelViewTransformPoint(ptCtrl);

  if (ptCtrl.DistanceToPointXY(ptView) >= sm_SelectApertureSize)
    // Not on proper vertex
    return false;

  if (sm_PivotVertex == 0)
    sm_Edge = EoUInt16(sm_Edge == 1 ? m_NumberOfPoints : 1);
  else if (sm_PivotVertex == m_NumberOfPoints - 1)
    sm_Edge = EoUInt16(sm_Edge == m_NumberOfPoints ? sm_Edge - 1 : m_NumberOfPoints);
  else if (sm_PivotVertex == sm_Edge)
    sm_Edge++;
  else
    sm_Edge--;

  return true;
}
void EoDbPolygon::GetAllPts(EoGePoint3dArray& pts) {
  pts.SetSize(0);
  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) pts.Add(m_Pt[w]);
}
// Determines the extent.
void EoDbPolygon::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  EoGePoint3d pt;

  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) {
    pt = m_Pt[w];
    view->ModelTransformPoint(pt);
    pt = tm * pt;
    ptMin = EoGePoint3d::Min(ptMin, pt);
    ptMax = EoGePoint3d::Max(ptMax, pt);
  }
}

bool EoDbPolygon::IsInView(AeSysView* view) {
  EoGePoint4d pt[2];

  pt[0] = m_Pt[0];
  view->ModelViewTransformPoint(pt[0]);

  for (int i = m_NumberOfPoints - 1; i >= 0; i--) {
    pt[1] = m_Pt[i];
    view->ModelViewTransformPoint(pt[1]);

    if (EoGePoint4d::ClipLine(pt[0], pt[1])) return true;
    pt[0] = pt[1];
  }
  return false;
}

bool EoDbPolygon::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) {
    EoGePoint4d pt(m_Pt[w]);
    view->ModelViewTransformPoint(pt);

    if (point.DistanceToPointXY(pt) < sm_SelectApertureSize) return true;
  }
  return false;
}

EoGePoint3d EoDbPolygon::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_ControlPointIndex = USHRT_MAX;
  double dApert = sm_SelectApertureSize;

  sm_PivotVertex = m_NumberOfPoints;

  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) {
    EoGePoint4d pt(m_Pt[w]);
    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dApert) {
      sm_ControlPointIndex = w;
      dApert = dDis;

      sm_Edge = EoUInt16(w + 1);
      sm_PivotVertex = w;
    }
  }
  return (sm_ControlPointIndex == USHRT_MAX) ? EoGePoint3d::kOrigin : m_Pt[sm_ControlPointIndex];
}
void EoDbPolygon::SetHatRefVecs(double dOffAng, double dXScal, double dYScal) {
  m_vPosXAx = EoGeVector3d(m_Pt[0], m_Pt[1]);
  m_vPosYAx = EoGeVector3d(m_Pt[0], m_Pt[2]);

  EoGeVector3d vPlnNorm = EoGeCrossProduct(m_vPosXAx, m_vPosYAx);
  vPlnNorm.Normalize();

  if (vPlnNorm.z < 0) vPlnNorm = -vPlnNorm;

  m_vPosXAx.Normalize();
  m_vPosXAx.RotAboutArbAx(vPlnNorm, dOffAng);
  m_vPosYAx = m_vPosXAx;
  m_vPosYAx.RotAboutArbAx(vPlnNorm, Eo::HalfPi);
  m_vPosXAx *= dXScal;
  m_vPosYAx *= dYScal;
}

void EoDbPolygon::Transform(EoGeTransformMatrix& tm) {
  m_HatchOrigin = tm * m_HatchOrigin;
  m_vPosXAx = tm * m_vPosXAx;
  m_vPosYAx = tm * m_vPosYAx;
  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) m_Pt[w] = tm * m_Pt[w];
}

void EoDbPolygon::Translate(EoGeVector3d v) {
  m_HatchOrigin += v;
  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) m_Pt[w] += v;
}

void EoDbPolygon::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  // nothing done to hatch coordinate origin

  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) {
    if (((mask >> w) & 1UL) == 1) { m_Pt[w] += v; }
  }
}

bool EoDbPolygon::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kPolygonPrimitive));
  EoDb::Write(file, m_PenColor);
  EoDb::Write(file, m_InteriorStyle);  // note polygon style stuffed up into unused line type on io
  EoDb::Write(file, m_InteriorStyleIndex);
  EoDb::Write(file, m_NumberOfPoints);
  m_HatchOrigin.Write(file);
  m_vPosXAx.Write(file);
  m_vPosYAx.Write(file);

  for (EoUInt16 w = 0; w < m_NumberOfPoints; w++) m_Pt[w].Write(file);

  return true;
}
void DisplayFilAreaHatch(AeSysView* view, CDC* deviceContext, EoGeTransformMatrix& tm, const int iSets, const int* iPtLstsId, EoGePoint3d* pta) {
  double dCurStrLen;
  double dEps1;
  double dMaxY;
  double dRemDisToEdg;
  double dScan;
  double dSecBeg;
  double dStrLen[8]{};
  int i;
  int i2;
  int iBegEdg;
  int iCurEdg;
  int iEndEdg;
  int iPts;
  int iStrId;

  EoGeTransformMatrix tmInv;

  pFilAreaEdgLis edg[65]{};

  EoGeLine ln;
  EoGeLine lnS;
  EoGeVector3d vEdg;

  EoInt16 nPenColor = pstate.PenColor();
  EoInt16 LineType = pstate.LineType();

  pstate.SetLineType(deviceContext, 1);

  int iTblId = hatch::iTableOffset[pstate.PolygonIntStyleId()];
  int iHatLns = int(hatch::fTableValue[iTblId++]);

  for (int i0 = 0; i0 < iHatLns; i0++) {
    int iStrs = int(hatch::fTableValue[iTblId++]);     // number of strokes in line definition
    double dTotStrLen = hatch::fTableValue[iTblId++];  // length of all strokes in line definition
    double dSinAng = sin(hatch::fTableValue[iTblId]);  // angle at which line will be drawn
    double dCosAng = cos(hatch::fTableValue[iTblId++]);
    double dX = hatch::fTableValue[iTblId++];  // displacement to origin of initial line
    double dY = hatch::fTableValue[iTblId++];
    double dShift = hatch::fTableValue[iTblId++];  // x-axis origin shift between lines
    double dSpac = hatch::fTableValue[iTblId++];   // spacing between lines

    for (i = 0; i < iStrs; i++)  // length of each stoke in line definition
      dStrLen[i] = hatch::fTableValue[iTblId++];

    // Rotate origin on z0 plane so hatch x-axis becomes positive x-axis
    double dHatOrigX = dX * dCosAng - dY * (-dSinAng);
    double dHatOrigY = dX * (-dSinAng) + dY * dCosAng;

    // Add rotation to matrix which gets current scan lines parallel to x-axis
    EoGeTransformMatrix tmRotZ;
    tm *= tmRotZ.ZAxisRotation(-dSinAng, dCosAng);
    tmInv = tm;
    tmInv.Inverse();

    int iActEdgs = 0;
    int iBegPt = 0;

    for (i = 0; i < iSets; i++) {
      if (i != 0) iBegPt = iPtLstsId[i - 1];
      ln.begin = pta[iBegPt];
      ln.begin = tm * ln.begin;  // Apply transform to get areas first point in z0 plane

      iPts = iPtLstsId[i] - iBegPt;  // Determine number of points in current area
      for (i2 = iBegPt; i2 < (int)iPtLstsId[i]; i2++) {
        ln.end = pta[((i2 - iBegPt + 1) % iPts) + iBegPt];
        ln.end = tm * ln.end;
        vEdg.x = ln.end.x - ln.begin.x;  // Determine x and y-components of edge
        vEdg.y = ln.end.y - ln.begin.y;
        if (fabs(vEdg.y) > DBL_EPSILON * sqrt(vEdg.x * vEdg.x + vEdg.y * vEdg.y)) {  // Edge is not horizontal
          dMaxY = std::max(ln.begin.y, ln.end.y);
          iCurEdg = iActEdgs + 1;
          // Find correct insertion point for edge in edge list using ymax as sort key
          while (iCurEdg != 1 && edg[iCurEdg - 1].dMaxY < dMaxY) {
            edg[iCurEdg] = edg[iCurEdg - 1];  // Move entry down
            iCurEdg--;
          }
          // Insert information about new edge
          edg[iCurEdg].dMaxY = dMaxY;
          edg[iCurEdg].dInvSlope = vEdg.x / vEdg.y;
          if (ln.begin.y > ln.end.y) {
            edg[iCurEdg].dMinY = ln.end.y;
            edg[iCurEdg].dX = ln.begin.x;
          } else {
            edg[iCurEdg].dMinY = ln.begin.y;
            edg[iCurEdg].dX = ln.end.x;
          }
          iActEdgs++;  // Increment count of active edges in edge list
        }
        ln.begin = ln.end;
      }
    }
    // Determine where first scan position is
    dScan = edg[1].dMaxY - fmod((edg[1].dMaxY - dHatOrigY), dSpac);
    if (edg[1].dMaxY < dScan) dScan = dScan - dSpac;
    dSecBeg = dHatOrigX + dShift * (dScan - dHatOrigY) / dSpac;
    // Edge list pointers
    iBegEdg = 1;
    iEndEdg = 1;
    // Determine relative epsilon to be used for extent tests
  l1:
    dEps1 = DBL_EPSILON + DBL_EPSILON * fabs(dScan);
    while (iEndEdg <= iActEdgs && edg[iEndEdg].dMaxY >= dScan - dEps1) {
      // Set x intersection back to last scanline
      edg[iEndEdg].dX += edg[iEndEdg].dInvSlope * (dSpac + dScan - edg[iEndEdg].dMaxY);
      // Determine the change in x per scan
      edg[iEndEdg].dStepSiz = -edg[iEndEdg].dInvSlope * dSpac;
      iEndEdg++;
    }
    for (i = iBegEdg; i < iEndEdg; i++) {
      iCurEdg = i;
      if (edg[i].dMinY < dScan - dEps1) {  // Edge y-extent overlaps current scan . determine intersections
        edg[i].dX += edg[i].dStepSiz;
        while (iCurEdg > iBegEdg && edg[iCurEdg].dX < edg[iCurEdg - 1].dX) {
          edg[0] = edg[iCurEdg];
          edg[iCurEdg] = edg[iCurEdg - 1];
          edg[iCurEdg - 1] = edg[0];
          iCurEdg--;
        }
      } else {  // Edge y-extent does not overlap current scan. remove edge from active edge list
        iBegEdg++;
        while (iCurEdg >= iBegEdg) {
          edg[iCurEdg] = edg[iCurEdg - 1];
          iCurEdg--;
        }
      }
    }
    if (iEndEdg != iBegEdg) {  // At least one pair of edge intersections .. generate scan lines for each pair
      iCurEdg = iBegEdg;
      lnS.begin.y = dScan;
      lnS.end.y = dScan;
      for (i = 1; i <= (iEndEdg - iBegEdg) / 2; i++) {
        lnS.begin.x = edg[iCurEdg].dX - fmod((edg[iCurEdg].dX - dSecBeg), dTotStrLen);
        if (lnS.begin.x > edg[iCurEdg].dX) lnS.begin.x -= dTotStrLen;
        iStrId = 0;
        dRemDisToEdg = edg[iCurEdg].dX - lnS.begin.x;
        dCurStrLen = dStrLen[iStrId];
        while (dCurStrLen <= dRemDisToEdg + DBL_EPSILON) {
          lnS.begin.x += dCurStrLen;
          dRemDisToEdg -= dCurStrLen;
          iStrId = (iStrId + 1) % iStrs;
          dCurStrLen = dStrLen[iStrId];
        }
        lnS.begin.x = edg[iCurEdg].dX;
        dCurStrLen -= dRemDisToEdg;
        dRemDisToEdg = edg[iCurEdg + 1].dX - edg[iCurEdg].dX;
        while (dCurStrLen <= dRemDisToEdg + DBL_EPSILON) {
          lnS.end.x = lnS.begin.x + dCurStrLen;
          if ((iStrId & 1) == 0) {
            ln = tmInv * lnS;
            ln.Display(view, deviceContext);
          }
          dRemDisToEdg -= dCurStrLen;
          iStrId = (iStrId + 1) % iStrs;
          dCurStrLen = dStrLen[iStrId];
          lnS.begin.x = lnS.end.x;
        }
        if (dRemDisToEdg > DBL_EPSILON && (iStrId & 1) == 0) {
          // Partial component of dash section must produced
          lnS.end.x = edg[iCurEdg + 1].dX;
          ln = tmInv * lnS;
          ln.Display(view, deviceContext);
        }
        iCurEdg = iCurEdg + 2;
      }
      // Update position of scan line
      dScan -= dSpac;
      dSecBeg -= dShift;
      goto l1;
    }
    tm *= tmRotZ.ZAxisRotation(dSinAng, dCosAng);
  }
  pstate.SetPen(view, deviceContext, nPenColor, LineType);
}

EoUInt16 EoDbPolygon::SwingVertex() const {
  EoUInt16 wSwingVertex;

  if (sm_PivotVertex == 0)
    wSwingVertex = EoUInt16(sm_Edge == 1 ? 1 : m_NumberOfPoints - 1);
  else if (sm_PivotVertex == EoUInt16(m_NumberOfPoints - 1))
    wSwingVertex = EoUInt16(sm_Edge == m_NumberOfPoints ? 0 : sm_PivotVertex - 1);
  else
    wSwingVertex = EoUInt16(sm_Edge == sm_PivotVertex ? sm_PivotVertex - 1 : sm_PivotVertex + 1);

  return (wSwingVertex);
}
void Polygon_Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray) {
  int iPts = static_cast<int>(pointsArray.GetSize());
  if (iPts >= 2) {
    auto* pnt = new CPoint[static_cast<size_t>(iPts)];

    view->DoProjection(pnt, pointsArray);

    if (pstate.PolygonIntStyle() == EoDb::kSolid) {
      CBrush brush(pColTbl[pstate.PenColor()]);
      CBrush* pBrushOld = deviceContext->SelectObject(&brush);
      deviceContext->Polygon(pnt, iPts);
      deviceContext->SelectObject(pBrushOld);
    } else if (pstate.PolygonIntStyle() == EoDb::kHollow) {
      CBrush* pBrushOld = (CBrush*)deviceContext->SelectStockObject(NULL_BRUSH);
      deviceContext->Polygon(pnt, iPts);
      deviceContext->SelectObject(pBrushOld);
    } else {
      deviceContext->Polygon(pnt, iPts);
    }
    delete[] pnt;
  }
}
