#include "Stdafx.h"

#include <algorithm>
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
#include "EoGsRenderState.h"
#include "Hatch.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif  // USING_DDE

EoDb::PolygonStyle EoDbPolygon::sm_SpecialPolygonStyle = EoDb::PolygonStyle::Special;

EoUInt16 EoDbPolygon::sm_EdgeToEvaluate{};
EoUInt16 EoDbPolygon::sm_Edge{};
EoUInt16 EoDbPolygon::sm_PivotVertex{};

typedef struct tagFilAreaEdgLis {
  double yMinExtent;     // minimum y extent of edge
  double yMaxExtent;     // maximum y extent of edge
  double xIntersection;  // x intersection on edge
  union {
    double inverseSlope;  // inverse slope of edge
    double xStepSize;     // change in x for each scanline
  };
} pFilAreaEdgLis;

EoDbPolygon::EoDbPolygon()
    : m_hatchOrigin{EoGePoint3d::kOrigin},
      m_positiveX{EoGeVector3d::positiveUnitX},
      m_positiveY{EoGeVector3d::positiveUnitY},
      m_vertices{},
      m_polygonStyle{EoDb::PolygonStyle::Hollow},
      m_fillStyleIndex{1},
      m_numberOfVertices{} {}

EoDbPolygon::EoDbPolygon(EoGePoint3dArray& points) {
  m_color = pstate.Color();
  m_polygonStyle = pstate.PolygonIntStyle();
  m_fillStyleIndex = pstate.PolygonIntStyleId();

  m_hatchOrigin = points[0];

  m_numberOfVertices = EoUInt16(points.GetSize());

  if (m_numberOfVertices >= 3) {
    m_positiveX = EoGeVector3d(points[0], points[1]);
    m_positiveY = EoGeVector3d(points[0], points[2]);
    auto normal = CrossProduct(m_positiveX, m_positiveY);
    normal.Normalize();

    if (normal.z < 0) normal = -normal;

    m_positiveX.Normalize();
    m_positiveX.RotAboutArbAx(normal, hatch::dOffAng);
    m_positiveY = m_positiveX;
    m_positiveY.RotAboutArbAx(normal, Eo::HalfPi);
    m_positiveX *= hatch::dXAxRefVecScal;
    m_positiveY *= hatch::dYAxRefVecScal;

    // Project reference origin to plane

    m_vertices = new EoGePoint3d[m_numberOfVertices];

    for (EoUInt16 w = 0; w < m_numberOfVertices; w++) m_vertices[w] = points[w];
  }
}

EoDbPolygon::EoDbPolygon(EoUInt16 numberOfVertices, EoGePoint3d* pt) {
  m_color = 0;
  m_polygonStyle = EoDb::PolygonStyle::Solid;
  m_fillStyleIndex = 0;
  m_numberOfVertices = numberOfVertices;
  m_hatchOrigin = pt[0];
  m_positiveX = EoGeVector3d::positiveUnitX;
  m_positiveY = EoGeVector3d::positiveUnitY;
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { m_vertices[w] = pt[w]; }
}
EoDbPolygon::EoDbPolygon(EoUInt16 numberOfVertices, EoGePoint3d origin, EoGeVector3d vXAx, EoGeVector3d vYAx,
                         const EoGePoint3d* ppt) {
  m_color = pstate.Color();
  m_polygonStyle = pstate.PolygonIntStyle();
  m_fillStyleIndex = pstate.PolygonIntStyleId();
  m_numberOfVertices = numberOfVertices;
  m_hatchOrigin = origin;
  m_positiveX = vXAx;
  m_positiveY = vYAx;
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { m_vertices[w] = ppt[w]; }
}

EoDbPolygon::EoDbPolygon(const EoGePoint3d& origin, const EoGeVector3d& xAxis, const EoGeVector3d& yAxis, EoGePoint3dArray& pts) {
  m_color = pstate.Color();
  m_polygonStyle = pstate.PolygonIntStyle();
  m_fillStyleIndex = pstate.PolygonIntStyleId();
  m_numberOfVertices = EoUInt16(pts.GetSize());
  m_hatchOrigin = origin;
  m_positiveX = xAxis;
  m_positiveY = yAxis;
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { m_vertices[w] = pts[w]; }
}

EoDbPolygon::EoDbPolygon(std::int16_t color, EoDb::PolygonStyle style, std::int16_t styleIndex, const EoGePoint3d& origin,
                         const EoGeVector3d& xAxis, const EoGeVector3d& yAxis, EoGePoint3dArray& points)
    : m_hatchOrigin(origin), m_positiveX(xAxis), m_positiveY(yAxis) {
  m_color = color;
  m_polygonStyle = style;
  m_fillStyleIndex = styleIndex;
  m_numberOfVertices = EoUInt16(points.GetSize());
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (EoUInt16 n = 0; n < m_numberOfVertices; n++) { m_vertices[n] = points[n]; }
}

EoDbPolygon::EoDbPolygon(const EoDbPolygon& other) {
  m_color = other.m_color;
  m_polygonStyle = other.m_polygonStyle;
  m_fillStyleIndex = other.m_fillStyleIndex;
  m_hatchOrigin = other.m_hatchOrigin;
  m_positiveX = other.m_positiveX;
  m_positiveY = other.m_positiveY;
  m_numberOfVertices = other.m_numberOfVertices;
  m_vertices = new EoGePoint3d[m_numberOfVertices];
  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { m_vertices[w] = other.m_vertices[w]; }
}

const EoDbPolygon& EoDbPolygon::operator=(const EoDbPolygon& other) {
  m_color = other.m_color;
  m_polygonStyle = other.m_polygonStyle;
  m_fillStyleIndex = other.m_fillStyleIndex;
  m_hatchOrigin = other.m_hatchOrigin;
  m_positiveX = other.m_positiveX;
  m_positiveY = other.m_positiveY;

  if (m_numberOfVertices != other.m_numberOfVertices) {
    m_numberOfVertices = other.m_numberOfVertices;
    delete[] m_vertices;
    m_vertices = new EoGePoint3d[m_numberOfVertices];
  }
  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { m_vertices[w] = other.m_vertices[w]; }

  return (*this);
}

EoDbPolygon::~EoDbPolygon() { delete[] m_vertices; }

void EoDbPolygon::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<Polygon>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbPolygon::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbPolygon(*this);
  return primitive;
}

void EoDbPolygon::Display(AeSysView* view, CDC* deviceContext) {
  std::int16_t color = LogicalColor();

  pstate.SetColor(deviceContext, color);
  EoDb::PolygonStyle polygonStyle =
      sm_SpecialPolygonStyle == EoDb::PolygonStyle::Special ? m_polygonStyle : sm_SpecialPolygonStyle;
  pstate.SetPolygonIntStyle(polygonStyle);  // hollow, solid, pattern, hatch
  pstate.SetPolygonIntStyleId(m_fillStyleIndex);

  int iPtLstsId = m_numberOfVertices;

  if (m_polygonStyle == EoDb::PolygonStyle::Hatch) {
    EoGeTransformMatrix transformMatrix(m_hatchOrigin, m_positiveX, m_positiveY);
    DisplayFilAreaHatch(view, deviceContext, transformMatrix, 1, &iPtLstsId, m_vertices);
  } else {  // Fill area interior style is hollow, solid or pattern
    EoGePoint4dArray PointsArray;

    PointsArray.SetSize(m_numberOfVertices);

    for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { PointsArray[w] = EoGePoint4d(m_vertices[w]); }
    view->ModelViewTransformPoints(PointsArray);
    EoGePoint4d::ClipPolygon(PointsArray);
    Polygon_Display(view, deviceContext, PointsArray);
  }
}

void EoDbPolygon::AddReportToMessageList(const EoGePoint3d& point) {
  CString Message(L"<Polygon Edge> ");

  if (sm_Edge > 0 && sm_Edge <= m_numberOfVertices) {
    EoGePoint3d* pBegPt = &m_vertices[sm_Edge - 1];
    EoGePoint3d* pEndPt = &m_vertices[sm_Edge % m_numberOfVertices];

    if (sm_PivotVertex < m_numberOfVertices) {
      pBegPt = &m_vertices[sm_PivotVertex];
      pEndPt = &m_vertices[SwingVertex()];
    }
    double dAng;
    double dLen = EoGeVector3d(*pBegPt, *pEndPt).Length();  // Length of edge

    if (EoGeVector3d(point, *pBegPt).Length() > dLen * 0.5)
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
  str += L"Hatch Origin;" + m_hatchOrigin.ToString();
  str += L"X Axis;" + m_positiveX.ToString();
  str += L"Y Axis;" + m_positiveY.ToString();

  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { str += L"Vertex Point;" + m_vertices[w].ToString(); }
}

CString EoDbPolygon::FormatIntStyle() {
  CString strStyle[] = {L"Hollow", L"Solid", L"Pattern", L"Hatch"};

  CString str = (m_polygonStyle >= EoDb::PolygonStyle::Hollow && m_polygonStyle <= EoDb::PolygonStyle::Hatch)
                    ? strStyle[static_cast<int>(m_polygonStyle)]
                    : const_cast<LPWSTR>(L"Invalid!");

  return str;
}
void EoDbPolygon::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s\tPoints;%d", FormatPenColor().GetString(), FormatLineType().GetString(),
             m_numberOfVertices);
}
EoGePoint3d EoDbPolygon::GetControlPoint() {
  EoUInt16 wBeg = EoUInt16(sm_Edge - 1);
  EoUInt16 wEnd = EoUInt16(sm_Edge % m_numberOfVertices);
  EoGePoint3d pt = EoGeLine(m_vertices[wBeg], m_vertices[wEnd]).Midpoint();
  return pt;
};

EoGePoint3d EoDbPolygon::GoToNextControlPoint() {
  if (sm_PivotVertex >= m_numberOfVertices) {  // have not yet rocked to a vertex
    EoUInt16 wBeg = EoUInt16(sm_Edge - 1);
    EoUInt16 wEnd = EoUInt16(sm_Edge % m_numberOfVertices);

    if (m_vertices[wEnd].x > m_vertices[wBeg].x) {
      sm_PivotVertex = wBeg;
    } else if (m_vertices[wEnd].x < m_vertices[wBeg].x) {
      sm_PivotVertex = wEnd;
    } else if (m_vertices[wEnd].y > m_vertices[wBeg].y) {
      sm_PivotVertex = wBeg;
    } else {
      sm_PivotVertex = wEnd;
    }
  } else if (sm_PivotVertex == 0) {
    if (sm_Edge == 1) {
      sm_PivotVertex = 1;
    } else {
      sm_PivotVertex = EoUInt16(m_numberOfVertices - 1);
    }
  } else if (sm_PivotVertex == EoUInt16(m_numberOfVertices - 1)) {
    if (sm_Edge == m_numberOfVertices) {
      sm_PivotVertex = 0;
    } else {
      sm_PivotVertex--;
    }
  } else {
    if (sm_Edge == sm_PivotVertex) {
      sm_PivotVertex--;
    } else {
      sm_PivotVertex++;
    }
  }
  return (m_vertices[sm_PivotVertex]);
}

bool EoDbPolygon::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) {
  (void)view;
  (void)line;
  return false;
}

bool EoDbPolygon::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  if (sm_EdgeToEvaluate > 0 && sm_EdgeToEvaluate <= m_numberOfVertices) {  // Evaluate specified edge of polygon
    EoGePoint4d ptBeg(m_vertices[sm_EdgeToEvaluate - 1]);
    EoGePoint4d ptEnd(m_vertices[sm_EdgeToEvaluate % m_numberOfVertices]);

    view->ModelViewTransformPoint(ptBeg);
    view->ModelViewTransformPoint(ptEnd);

    EoGeLine Edge(ptBeg, ptEnd);
    if (Edge.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
      ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
      return true;
    }
  } else {  // Evaluate entire polygon
    EoGePoint4d ptBeg(m_vertices[0]);
    view->ModelViewTransformPoint(ptBeg);

    for (EoUInt16 w = 1; w <= m_numberOfVertices; w++) {
      EoGePoint4d ptEnd(m_vertices[w % m_numberOfVertices]);
      view->ModelViewTransformPoint(ptEnd);

      EoGeLine Edge(ptBeg, ptEnd);
      if (Edge.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
        ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
        sm_Edge = w;
        sm_PivotVertex = m_numberOfVertices;
        return true;
      }
      ptBeg = ptEnd;
    }
  }
  return false;
}

bool EoDbPolygon::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  EoGePoint3dArray pts;
  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { pts.Add(m_vertices[w]); }

  return polyline::SelectUsingRectangle(view, pt1, pt2, pts);
}
void EoDbPolygon::ModifyState() {
  EoDbPrimitive::ModifyState();

  m_polygonStyle = pstate.PolygonIntStyle();
  m_fillStyleIndex = pstate.PolygonIntStyleId();
}
bool EoDbPolygon::PivotOnControlPoint(AeSysView* view, const EoGePoint4d& ptView) {
  if (sm_PivotVertex >= m_numberOfVertices) { return false; }

  // Engaged at a vertex
  EoGePoint4d ptCtrl(m_vertices[sm_PivotVertex]);
  view->ModelViewTransformPoint(ptCtrl);

  if (ptCtrl.DistanceToPointXY(ptView) >= sm_SelectApertureSize) { return false; }

  if (sm_PivotVertex == 0) {
    sm_Edge = EoUInt16(sm_Edge == 1 ? m_numberOfVertices : 1);
  } else if (sm_PivotVertex == m_numberOfVertices - 1) {
    sm_Edge = EoUInt16(sm_Edge == m_numberOfVertices ? sm_Edge - 1 : m_numberOfVertices);
  } else if (sm_PivotVertex == sm_Edge) {
    sm_Edge++;
  } else {
    sm_Edge--;
  }
  return true;
}
void EoDbPolygon::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { points.Add(m_vertices[w]); }
}
// Determines the extent.
void EoDbPolygon::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3d pt;

  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) {
    pt = m_vertices[w];
    view->ModelTransformPoint(pt);
    pt = transformMatrix * pt;
    ptMin = EoGePoint3d::Min(ptMin, pt);
    ptMax = EoGePoint3d::Max(ptMax, pt);
  }
}

bool EoDbPolygon::IsInView(AeSysView* view) {
  EoGePoint4d pt[2]{};

  pt[0] = m_vertices[0];
  view->ModelViewTransformPoint(pt[0]);

  for (int i = m_numberOfVertices - 1; i >= 0; i--) {
    pt[1] = m_vertices[i];
    view->ModelViewTransformPoint(pt[1]);

    if (EoGePoint4d::ClipLine(pt[0], pt[1])) { return true; }
    pt[0] = pt[1];
  }
  return false;
}

bool EoDbPolygon::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) {
    EoGePoint4d pt(m_vertices[w]);
    view->ModelViewTransformPoint(pt);

    if (point.DistanceToPointXY(pt) < sm_SelectApertureSize) { return true; }
  }
  return false;
}

EoGePoint3d EoDbPolygon::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_ControlPointIndex = USHRT_MAX;
  double dApert = sm_SelectApertureSize;

  sm_PivotVertex = m_numberOfVertices;

  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) {
    EoGePoint4d pt(m_vertices[w]);
    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dApert) {
      sm_ControlPointIndex = w;
      dApert = dDis;

      sm_Edge = EoUInt16(w + 1);
      sm_PivotVertex = w;
    }
  }
  return (sm_ControlPointIndex == USHRT_MAX) ? EoGePoint3d::kOrigin : m_vertices[sm_ControlPointIndex];
}
void EoDbPolygon::SetHatRefVecs(double dOffAng, double dXScal, double dYScal) {
  m_positiveX = EoGeVector3d(m_vertices[0], m_vertices[1]);
  m_positiveY = EoGeVector3d(m_vertices[0], m_vertices[2]);

  auto normal = CrossProduct(m_positiveX, m_positiveY);
  normal.Normalize();

  if (normal.z < 0) normal = -normal;

  m_positiveX.Normalize();
  m_positiveX.RotAboutArbAx(normal, dOffAng);
  m_positiveY = m_positiveX;
  m_positiveY.RotAboutArbAx(normal, Eo::HalfPi);
  m_positiveX *= dXScal;
  m_positiveY *= dYScal;
}

void EoDbPolygon::Transform(const EoGeTransformMatrix& transformMatrix) {
  m_hatchOrigin = transformMatrix * m_hatchOrigin;
  m_positiveX = transformMatrix * m_positiveX;
  m_positiveY = transformMatrix * m_positiveY;
  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { m_vertices[w] = transformMatrix * m_vertices[w]; }
}

void EoDbPolygon::Translate(const EoGeVector3d& v) {
  m_hatchOrigin += v;
  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { m_vertices[w] += v; }
}

void EoDbPolygon::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  // nothing done to hatch coordinate origin

  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) {
    if (((mask >> w) & 1UL) == 1) { m_vertices[w] += v; }
  }
}

bool EoDbPolygon::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kPolygonPrimitive));
  EoDb::Write(file, m_color);
  // note polygon style stuffed up into unused line type on io
  EoDb::Write(file, static_cast<std::int16_t>(m_polygonStyle));
  EoDb::Write(file, m_fillStyleIndex);
  EoDb::Write(file, m_numberOfVertices);
  m_hatchOrigin.Write(file);
  m_positiveX.Write(file);
  m_positiveY.Write(file);

  for (EoUInt16 w = 0; w < m_numberOfVertices; w++) { m_vertices[w].Write(file); }

  return true;
}
void DisplayFilAreaHatch(AeSysView* view, CDC* deviceContext, EoGeTransformMatrix& transformMatrix, const int iSets,
                         const int* iPtLstsId, EoGePoint3d* pta) {
  double dCurStrLen{};
  double dEps1{};
  double dMaxY{};
  double dRemDisToEdg{};
  double dScan{};
  double dSecBeg{};
  double dStrLen[8]{};
  int i{};
  int i2{};
  int iBegEdg{};
  int iCurEdg{};
  int iEndEdg{};
  int iPts{};
  int iStrId{};

  EoGeTransformMatrix tmInv;

  pFilAreaEdgLis edg[65]{};

  EoGeLine ln;
  EoGeLine lnS;
  EoGeVector3d vEdg;

  std::int16_t color = pstate.Color();
  std::int16_t lineType = pstate.LineTypeIndex();

  pstate.SetLineType(deviceContext, 1);

  int iTblId = hatch::tableOffset[pstate.PolygonIntStyleId()];
  int iHatLns = int(hatch::tableValue[iTblId++]);

  for (int i0 = 0; i0 < iHatLns; i0++) {
    int iStrs = int(hatch::tableValue[iTblId++]);       // number of strokes in line definition
    double dTotStrLen = hatch::tableValue[iTblId++];    // length of all strokes in line definition
    double dSinAng = sin(hatch::tableValue[iTblId]);    // sine of angle at which line will be drawn
    double dCosAng = cos(hatch::tableValue[iTblId++]);  // cosine of angle at which line will be drawn
    double dX = hatch::tableValue[iTblId++];            // displacement to origin of initial line
    double dY = hatch::tableValue[iTblId++];
    double dShift = hatch::tableValue[iTblId++];  // x-axis origin shift between lines
    double dSpac = hatch::tableValue[iTblId++];   // spacing between lines

    for (i = 0; i < iStrs; i++)  // length of each stoke in line definition
      dStrLen[i] = hatch::tableValue[iTblId++];

    // Rotate origin on z0 plane so hatch x-axis becomes positive x-axis
    double dHatOrigX = dX * dCosAng - dY * (-dSinAng);
    double dHatOrigY = dX * (-dSinAng) + dY * dCosAng;

    // Add rotation to matrix which gets current scan lines parallel to x-axis
    transformMatrix *= EoGeTransformMatrix::ZAxisRotation(-dSinAng, dCosAng);
    tmInv = transformMatrix;
    tmInv.Inverse();

    int iActEdgs = 0;
    int iBegPt = 0;

    for (i = 0; i < iSets; i++) {
      if (i != 0) iBegPt = iPtLstsId[i - 1];
      ln.begin = pta[iBegPt];
      ln.begin = transformMatrix * ln.begin;  // Apply transform to get areas first point in z0 plane

      iPts = iPtLstsId[i] - iBegPt;  // Determine number of points in current area
      for (i2 = iBegPt; i2 < (int)iPtLstsId[i]; i2++) {
        ln.end = pta[((i2 - iBegPt + 1) % iPts) + iBegPt];
        ln.end = transformMatrix * ln.end;
        vEdg.x = ln.end.x - ln.begin.x;  // Determine x and y-components of edge
        vEdg.y = ln.end.y - ln.begin.y;
        if (fabs(vEdg.y) >
            Eo::geometricTolerance * sqrt(vEdg.x * vEdg.x + vEdg.y * vEdg.y)) {  // Edge is not horizontal
          dMaxY = std::max(ln.begin.y, ln.end.y);
          iCurEdg = iActEdgs + 1;
          // Find correct insertion point for edge in edge list using ymax as sort key
          while (iCurEdg != 1 && edg[iCurEdg - 1].yMaxExtent < dMaxY) {
            edg[iCurEdg] = edg[iCurEdg - 1];  // Move entry down
            iCurEdg--;
          }
          // Insert information about new edge
          edg[iCurEdg].yMaxExtent = dMaxY;
          edg[iCurEdg].inverseSlope = vEdg.x / vEdg.y;
          if (ln.begin.y > ln.end.y) {
            edg[iCurEdg].yMinExtent = ln.end.y;
            edg[iCurEdg].xIntersection = ln.begin.x;
          } else {
            edg[iCurEdg].yMinExtent = ln.begin.y;
            edg[iCurEdg].xIntersection = ln.end.x;
          }
          iActEdgs++;  // Increment count of active edges in edge list
        }
        ln.begin = ln.end;
      }
    }
    // Determine where first scan position is
    dScan = edg[1].yMaxExtent - fmod((edg[1].yMaxExtent - dHatOrigY), dSpac);
    if (edg[1].yMaxExtent < dScan) dScan = dScan - dSpac;
    dSecBeg = dHatOrigX + dShift * (dScan - dHatOrigY) / dSpac;
    // Edge list pointers
    iBegEdg = 1;
    iEndEdg = 1;
    // Determine relative epsilon to be used for extent tests
  l1:
    dEps1 = Eo::geometricTolerance + Eo::geometricTolerance * fabs(dScan);
    while (iEndEdg <= iActEdgs && edg[iEndEdg].yMaxExtent >= dScan - dEps1) {
      // Set x intersection back to last scanline
      edg[iEndEdg].xIntersection += edg[iEndEdg].inverseSlope * (dSpac + dScan - edg[iEndEdg].yMaxExtent);
      // Determine the change in x per scan
      edg[iEndEdg].xStepSize = -edg[iEndEdg].inverseSlope * dSpac;
      iEndEdg++;
    }
    for (i = iBegEdg; i < iEndEdg; i++) {
      iCurEdg = i;
      if (edg[i].yMinExtent < dScan - dEps1) {  // Edge y-extent overlaps current scan . determine intersections
        edg[i].xIntersection += edg[i].xStepSize;
        while (iCurEdg > iBegEdg && edg[iCurEdg].xIntersection < edg[iCurEdg - 1].xIntersection) {
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
        lnS.begin.x = edg[iCurEdg].xIntersection - fmod((edg[iCurEdg].xIntersection - dSecBeg), dTotStrLen);
        if (lnS.begin.x > edg[iCurEdg].xIntersection) lnS.begin.x -= dTotStrLen;
        iStrId = 0;
        dRemDisToEdg = edg[iCurEdg].xIntersection - lnS.begin.x;
        dCurStrLen = dStrLen[iStrId];
        while (dCurStrLen <= dRemDisToEdg + Eo::geometricTolerance) {
          lnS.begin.x += dCurStrLen;
          dRemDisToEdg -= dCurStrLen;
          iStrId = (iStrId + 1) % iStrs;
          dCurStrLen = dStrLen[iStrId];
        }
        lnS.begin.x = edg[iCurEdg].xIntersection;
        dCurStrLen -= dRemDisToEdg;
        dRemDisToEdg = edg[iCurEdg + 1].xIntersection - edg[iCurEdg].xIntersection;
        while (dCurStrLen <= dRemDisToEdg + Eo::geometricTolerance) {
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
        if (dRemDisToEdg > Eo::geometricTolerance && (iStrId & 1) == 0) {
          // Partial component of dash section must produced
          lnS.end.x = edg[iCurEdg + 1].xIntersection;
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
    transformMatrix *= EoGeTransformMatrix::ZAxisRotation(dSinAng, dCosAng);
  }
  pstate.SetPen(view, deviceContext, color, lineType);
}

EoUInt16 EoDbPolygon::SwingVertex() const {
  EoUInt16 swingVertex;

  if (sm_PivotVertex == 0) {
    swingVertex = EoUInt16(sm_Edge == 1 ? 1 : m_numberOfVertices - 1);
  } else if (sm_PivotVertex == EoUInt16(m_numberOfVertices - 1)) {
    swingVertex = EoUInt16(sm_Edge == m_numberOfVertices ? 0 : sm_PivotVertex - 1);
  } else {
    swingVertex = EoUInt16(sm_Edge == sm_PivotVertex ? sm_PivotVertex - 1 : sm_PivotVertex + 1);
  }
  return swingVertex;
}

void Polygon_Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray) {
  int iPts = static_cast<int>(pointsArray.GetSize());
  if (iPts < 2) { return; }

  auto* pnt = new CPoint[static_cast<size_t>(iPts)];

  view->DoProjection(pnt, pointsArray);

  if (pstate.PolygonIntStyle() == EoDb::PolygonStyle::Solid) {
    CBrush brush(pColTbl[pstate.Color()]);
    auto* oldBrush = deviceContext->SelectObject(&brush);
    deviceContext->Polygon(pnt, iPts);
    deviceContext->SelectObject(oldBrush);
  } else if (pstate.PolygonIntStyle() == EoDb::PolygonStyle::Hollow) {
    auto* oldBrush = (CBrush*)deviceContext->SelectStockObject(NULL_BRUSH);
    deviceContext->Polygon(pnt, iPts);
    deviceContext->SelectObject(oldBrush);
  } else {
    deviceContext->Polygon(pnt, iPts);
  }
  delete[] pnt;
}