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

std::uint16_t EoDbPolygon::sm_EdgeToEvaluate{};
std::uint16_t EoDbPolygon::sm_Edge{};
int EoDbPolygon::sm_pivotVertex{};

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
  m_color = renderState.Color();
  m_polygonStyle = renderState.PolygonIntStyle();
  m_fillStyleIndex = renderState.PolygonIntStyleId();

  m_hatchOrigin = points[0];

  m_numberOfVertices = std::uint16_t(points.GetSize());

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

    for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = points[i]; }
  }
}

EoDbPolygon::EoDbPolygon(std::uint16_t numberOfVertices, EoGePoint3d* pt) {
  m_color = 0;
  m_polygonStyle = EoDb::PolygonStyle::Solid;
  m_fillStyleIndex = 0;
  m_numberOfVertices = numberOfVertices;
  m_hatchOrigin = pt[0];
  m_positiveX = EoGeVector3d::positiveUnitX;
  m_positiveY = EoGeVector3d::positiveUnitY;
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = pt[i]; }
}
EoDbPolygon::EoDbPolygon(std::uint16_t numberOfVertices, EoGePoint3d origin, EoGeVector3d vXAx, EoGeVector3d vYAx,
                         const EoGePoint3d* ppt) {
  m_color = renderState.Color();
  m_polygonStyle = renderState.PolygonIntStyle();
  m_fillStyleIndex = renderState.PolygonIntStyleId();
  m_numberOfVertices = numberOfVertices;
  m_hatchOrigin = origin;
  m_positiveX = vXAx;
  m_positiveY = vYAx;
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = ppt[i]; }
}

EoDbPolygon::EoDbPolygon(const EoGePoint3d& origin, const EoGeVector3d& xAxis, const EoGeVector3d& yAxis, EoGePoint3dArray& pts) {
  m_color = renderState.Color();
  m_polygonStyle = renderState.PolygonIntStyle();
  m_fillStyleIndex = renderState.PolygonIntStyleId();
  m_numberOfVertices = std::uint16_t(pts.GetSize());
  m_hatchOrigin = origin;
  m_positiveX = xAxis;
  m_positiveY = yAxis;
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = pts[i]; }
}

EoDbPolygon::EoDbPolygon(std::int16_t color, EoDb::PolygonStyle style, std::int16_t styleIndex, const EoGePoint3d& origin,
                         const EoGeVector3d& xAxis, const EoGeVector3d& yAxis, EoGePoint3dArray& points)
    : m_hatchOrigin(origin), m_positiveX(xAxis), m_positiveY(yAxis) {
  m_color = color;
  m_polygonStyle = style;
  m_fillStyleIndex = styleIndex;
  m_numberOfVertices = std::uint16_t(points.GetSize());
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (std::uint16_t n = 0; n < m_numberOfVertices; n++) { m_vertices[n] = points[n]; }
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
  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = other.m_vertices[i]; }
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
  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = other.m_vertices[i]; }

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

  renderState.SetColor(deviceContext, color);
  EoDb::PolygonStyle polygonStyle =
      sm_SpecialPolygonStyle == EoDb::PolygonStyle::Special ? m_polygonStyle : sm_SpecialPolygonStyle;
  renderState.SetPolygonIntStyle(polygonStyle);  // hollow, solid, pattern, hatch
  renderState.SetPolygonIntStyleId(m_fillStyleIndex);

  int iPtLstsId = m_numberOfVertices;

  if (m_polygonStyle == EoDb::PolygonStyle::Hatch) {
    EoGeTransformMatrix transformMatrix(m_hatchOrigin, m_positiveX, m_positiveY);
    DisplayFilAreaHatch(view, deviceContext, transformMatrix, 1, &iPtLstsId, m_vertices);
  } else {  // Fill area interior style is hollow, solid or pattern
    EoGePoint4dArray PointsArray;

    PointsArray.SetSize(m_numberOfVertices);

    for (auto i = 0; i < m_numberOfVertices; i++) { PointsArray[i] = EoGePoint4d(m_vertices[i]); }
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

    if (sm_pivotVertex < m_numberOfVertices) {
      pBegPt = &m_vertices[sm_pivotVertex];
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

  for (auto i = 0; i < m_numberOfVertices; i++) { str += L"Vertex Point;" + m_vertices[i].ToString(); }
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
  std::uint16_t wBeg = std::uint16_t(sm_Edge - 1);
  std::uint16_t wEnd = std::uint16_t(sm_Edge % m_numberOfVertices);
  EoGePoint3d pt = EoGeLine(m_vertices[wBeg], m_vertices[wEnd]).Midpoint();
  return pt;
};

EoGePoint3d EoDbPolygon::GoToNextControlPoint() {
  if (sm_pivotVertex >= m_numberOfVertices) {  // have not yet rocked to a vertex
    std::uint16_t wBeg = std::uint16_t(sm_Edge - 1);
    std::uint16_t wEnd = std::uint16_t(sm_Edge % m_numberOfVertices);

    if (m_vertices[wEnd].x > m_vertices[wBeg].x) {
      sm_pivotVertex = wBeg;
    } else if (m_vertices[wEnd].x < m_vertices[wBeg].x) {
      sm_pivotVertex = wEnd;
    } else if (m_vertices[wEnd].y > m_vertices[wBeg].y) {
      sm_pivotVertex = wBeg;
    } else {
      sm_pivotVertex = wEnd;
    }
  } else if (sm_pivotVertex == 0) {
    if (sm_Edge == 1) {
      sm_pivotVertex = 1;
    } else {
      sm_pivotVertex = std::uint16_t(m_numberOfVertices - 1);
    }
  } else if (sm_pivotVertex == std::uint16_t(m_numberOfVertices - 1)) {
    if (sm_Edge == m_numberOfVertices) {
      sm_pivotVertex = 0;
    } else {
      sm_pivotVertex--;
    }
  } else {
    if (sm_Edge == sm_pivotVertex) {
      sm_pivotVertex--;
    } else {
      sm_pivotVertex++;
    }
  }
  return (m_vertices[sm_pivotVertex]);
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

    for (std::uint16_t w = 1; w <= m_numberOfVertices; w++) {
      EoGePoint4d ptEnd(m_vertices[w % m_numberOfVertices]);
      view->ModelViewTransformPoint(ptEnd);

      EoGeLine Edge(ptBeg, ptEnd);
      if (Edge.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
        ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
        sm_Edge = w;
        sm_pivotVertex = m_numberOfVertices;
        return true;
      }
      ptBeg = ptEnd;
    }
  }
  return false;
}

bool EoDbPolygon::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  EoGePoint3dArray pts;
  for (auto i = 0; i < m_numberOfVertices; i++) { pts.Add(m_vertices[i]); }

  return polyline::SelectUsingRectangle(view, pt1, pt2, pts);
}
void EoDbPolygon::ModifyState() {
  EoDbPrimitive::ModifyState();

  m_polygonStyle = renderState.PolygonIntStyle();
  m_fillStyleIndex = renderState.PolygonIntStyleId();
}
bool EoDbPolygon::PivotOnControlPoint(AeSysView* view, const EoGePoint4d& ptView) {
  if (sm_pivotVertex >= m_numberOfVertices) { return false; }

  // Engaged at a vertex
  EoGePoint4d ptCtrl(m_vertices[sm_pivotVertex]);
  view->ModelViewTransformPoint(ptCtrl);

  if (ptCtrl.DistanceToPointXY(ptView) >= sm_SelectApertureSize) { return false; }

  if (sm_pivotVertex == 0) {
    sm_Edge = std::uint16_t(sm_Edge == 1 ? m_numberOfVertices : 1);
  } else if (sm_pivotVertex == m_numberOfVertices - 1) {
    sm_Edge = std::uint16_t(sm_Edge == m_numberOfVertices ? sm_Edge - 1 : m_numberOfVertices);
  } else if (sm_pivotVertex == sm_Edge) {
    sm_Edge++;
  } else {
    sm_Edge--;
  }
  return true;
}
void EoDbPolygon::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  for (auto i = 0; i < m_numberOfVertices; i++) { points.Add(m_vertices[i]); }
}
// Determines the extent.
void EoDbPolygon::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3d pt;

  for (auto i = 0; i < m_numberOfVertices; i++) {
    pt = m_vertices[i];
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
  for (auto i = 0; i < m_numberOfVertices; i++) {
    EoGePoint4d pt(m_vertices[i]);
    view->ModelViewTransformPoint(pt);

    if (point.DistanceToPointXY(pt) < sm_SelectApertureSize) { return true; }
  }
  return false;
}

EoGePoint3d EoDbPolygon::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_controlPointIndex = SHRT_MAX;
  double dApert = sm_SelectApertureSize;

  sm_pivotVertex = m_numberOfVertices;

  for (auto i = 0; i < m_numberOfVertices; i++) {
    EoGePoint4d pt(m_vertices[i]);
    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dApert) {
      sm_controlPointIndex = i;
      dApert = dDis;

      sm_Edge = std::uint16_t(i + 1);
      sm_pivotVertex = i;
    }
  }
  return (sm_controlPointIndex == SHRT_MAX) ? EoGePoint3d::kOrigin : m_vertices[sm_controlPointIndex];
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
  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = transformMatrix * m_vertices[i]; }
}

void EoDbPolygon::Translate(const EoGeVector3d& v) {
  m_hatchOrigin += v;
  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] += v; }
}

void EoDbPolygon::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  // nothing done to hatch coordinate origin

  for (auto i = 0; i < m_numberOfVertices; i++) {
    if (((mask >> i) & 1UL) == 1) { m_vertices[i] += v; }
  }
}

bool EoDbPolygon::Write(CFile& file) {
  EoDb::Write(file, std::uint16_t(EoDb::kPolygonPrimitive));
  EoDb::Write(file, m_color);
  // note polygon style stuffed up into unused line type on io
  EoDb::Write(file, static_cast<std::int16_t>(m_polygonStyle));
  EoDb::Write(file, m_fillStyleIndex);
  EoDb::Write(file, m_numberOfVertices);
  m_hatchOrigin.Write(file);
  m_positiveX.Write(file);
  m_positiveY.Write(file);

  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i].Write(file); }

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

  std::int16_t color = renderState.Color();
  std::int16_t lineType = renderState.LineTypeIndex();

  renderState.SetLineType(deviceContext, 1);

  int iTblId = hatch::tableOffset[renderState.PolygonIntStyleId()];
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
  renderState.SetPen(view, deviceContext, color, lineType);
}

std::uint16_t EoDbPolygon::SwingVertex() const {
  std::uint16_t swingVertex;

  if (sm_pivotVertex == 0) {
    swingVertex = std::uint16_t(sm_Edge == 1 ? 1 : m_numberOfVertices - 1);
  } else if (sm_pivotVertex == std::uint16_t(m_numberOfVertices - 1)) {
    swingVertex = std::uint16_t(sm_Edge == m_numberOfVertices ? 0 : sm_pivotVertex - 1);
  } else {
    swingVertex = std::uint16_t(sm_Edge == sm_pivotVertex ? sm_pivotVertex - 1 : sm_pivotVertex + 1);
  }
  return swingVertex;
}

void Polygon_Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray) {
  int iPts = static_cast<int>(pointsArray.GetSize());
  if (iPts < 2) { return; }

  auto* pnt = new CPoint[static_cast<size_t>(iPts)];

  view->DoProjection(pnt, pointsArray);

  if (renderState.PolygonIntStyle() == EoDb::PolygonStyle::Solid) {
    CBrush brush(pColTbl[renderState.Color()]);
    auto* oldBrush = deviceContext->SelectObject(&brush);
    deviceContext->Polygon(pnt, iPts);
    deviceContext->SelectObject(oldBrush);
  } else if (renderState.PolygonIntStyle() == EoDb::PolygonStyle::Hollow) {
    auto* oldBrush = (CBrush*)deviceContext->SelectStockObject(NULL_BRUSH);
    deviceContext->Polygon(pnt, iPts);
    deviceContext->SelectObject(oldBrush);
  } else {
    deviceContext->Polygon(pnt, iPts);
  }
  delete[] pnt;
}