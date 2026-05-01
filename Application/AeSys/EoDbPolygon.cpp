#include "Stdafx.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <vector>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoDxfHatch.h"
#include "EoDxfInterface.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderDevice.h"
#include "EoGsRenderState.h"
#include "Hatch.h"

#ifdef USING_DDE
#include "ddeGItms.h"
#endif

EoDb::PolygonStyle EoDbPolygon::sm_SpecialPolygonStyle = EoDb::PolygonStyle::Special;

std::uint16_t EoDbPolygon::sm_EdgeToEvaluate{};
std::uint16_t EoDbPolygon::sm_Edge{};
int EoDbPolygon::sm_pivotVertex{};

namespace {

typedef struct tagFilAreaEdgLis {
  double yMinExtent;  // minimum y extent of edge
  double yMaxExtent;  // maximum y extent of edge
  double xIntersection;  // x intersection on edge
  union {
    double inverseSlope;  // inverse slope of edge
    double xStepSize;  // change in x for each scanline
  };
} pFilAreaEdgLis;

void DisplayFilAreaHatch(AeSysView* view,
    EoGsRenderDevice* renderDevice,
    EoGeTransformMatrix& transformMatrix,
    const int iSets,
    const int* iPtLstsId,
    const EoGePoint3d* pta) {
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

  // Compute total edge count across all sets for dynamic edge list sizing (1-based indexing)
  const int totalEdgeCapacity = (iSets > 0) ? iPtLstsId[iSets - 1] + 1 : 1;
  std::vector<pFilAreaEdgLis> edg(totalEdgeCapacity, pFilAreaEdgLis{});

  EoGeLine ln;
  EoGeLine lnS;
  EoGeVector3d vEdg;

  const std::int16_t color = Gs::renderState.Color();
  const std::int16_t lineType = Gs::renderState.LineTypeIndex();

  Gs::renderState.SetLineType(renderDevice, 1);

  const int fillStyleIndex = Gs::renderState.PolygonIntStyleId();
  if (fillStyleIndex < 0 || fillStyleIndex >= hatch::maxPatterns || hatch::tableOffset[fillStyleIndex] == 0) {
    Gs::renderState.SetPen(view, renderDevice, color, lineType);
    return;  // Out-of-range or uninitialized hatch table entry — nothing to draw
  }

  int iTblId = hatch::tableOffset[fillStyleIndex];
  const int iHatLns = int(hatch::tableValue[iTblId++]);

  for (int i0 = 0; i0 < iHatLns; i0++) {
    const int iStrsInTable = int(hatch::tableValue[iTblId++]);  // number of strokes stored in table
    const int iStrs = std::min(iStrsInTable, 8);  // local buffer limit
    const double dTotStrLen = hatch::tableValue[iTblId++];  // length of all strokes in line definition
    const double dSinAng = std::sin(hatch::tableValue[iTblId]);  // sine of angle at which line will be drawn
    const double dCosAng = std::cos(hatch::tableValue[iTblId++]);  // cosine of angle at which line will be drawn
    const double dX = hatch::tableValue[iTblId++];  // displacement to origin of initial line
    const double dY = hatch::tableValue[iTblId++];
    double dShift = hatch::tableValue[iTblId++];  // x-axis origin shift between lines
    double dSpac = hatch::tableValue[iTblId++];  // spacing between lines

    // Scan-line algorithm sweeps top-to-bottom; negative spacing from acad.pat
    // indicates opposite tiling direction — normalize to positive and flip shift
    // to preserve the shift-per-step ratio.
    if (dSpac < 0.0) {
      dSpac = -dSpac;
      dShift = -dShift;
    }

    if (Eo::IsGeometricallyZero(dSpac) || Eo::IsGeometricallyZero(dTotStrLen) || iStrs <= 0) {
      // Degenerate spacing, zero total stroke length, or non-positive stroke count would cause invalid looping or
      // modulo-by-zero behavior downstream.
      iTblId += iStrsInTable;  // skip ALL stroke entries to keep table pointer consistent
      continue;
    }

    for (i = 0; i < iStrs; i++) {  // read usable strokes into local buffer
      dStrLen[i] = hatch::tableValue[iTblId++];
    }
    iTblId += (iStrsInTable - iStrs);  // skip any excess strokes beyond buffer capacity

    // Rotate origin on z0 plane so hatch x-axis becomes positive x-axis
    const double dHatOrigX = dX * dCosAng - dY * (-dSinAng);
    const double dHatOrigY = dX * (-dSinAng) + dY * dCosAng;

    // Save matrix before rotation — exact restore eliminates FP drift that
    // accumulates across multi-line patterns (26 matrix multiplies for AR-CONC).
    EoGeTransformMatrix savedMatrix = transformMatrix;

    // Add rotation to matrix which gets current scan lines parallel to x-axis
    transformMatrix *= EoGeTransformMatrix::ZAxisRotation(-dSinAng, dCosAng);
    tmInv = transformMatrix;
    tmInv.Inverse();

    int iActEdgs = 0;
    int iBegPt = 0;

    for (i = 0; i < iSets; i++) {
      if (i != 0) { iBegPt = iPtLstsId[i - 1]; }
      ln.begin = pta[iBegPt];
      ln.begin = transformMatrix * ln.begin;  // Apply transform to get areas first point in z0 plane

      iPts = iPtLstsId[i] - iBegPt;  // Determine number of points in current area
      for (i2 = iBegPt; i2 < (int)iPtLstsId[i]; i2++) {
        ln.end = pta[((i2 - iBegPt + 1) % iPts) + iBegPt];
        ln.end = transformMatrix * ln.end;
        vEdg.x = ln.end.x - ln.begin.x;  // Determine x and y-components of edge
        vEdg.y = ln.end.y - ln.begin.y;
        if (std::abs(vEdg.y)
            > Eo::geometricTolerance * std::sqrt(vEdg.x * vEdg.x + vEdg.y * vEdg.y)) {  // Edge is not horizontal
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
    if (iActEdgs == 0) {
      transformMatrix = savedMatrix;
      continue;  // All edges horizontal in this rotated frame — nothing to scan
    }
    // Determine where first scan position is
    dScan = edg[1].yMaxExtent - fmod((edg[1].yMaxExtent - dHatOrigY), dSpac);
    if (edg[1].yMaxExtent < dScan) { dScan = dScan - dSpac; }
    dSecBeg = dHatOrigX + dShift * (dScan - dHatOrigY) / dSpac;
    // Edge list pointers
    iBegEdg = 1;
    iEndEdg = 1;
    // Determine relative epsilon to be used for extent tests
  l1:
    dEps1 = Eo::geometricTolerance + Eo::geometricTolerance * std::abs(dScan);
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
        if (lnS.begin.x > edg[iCurEdg].xIntersection) { lnS.begin.x -= dTotStrLen; }
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
            if (dCurStrLen < Eo::geometricTolerance) {
              // Zero-length dash (dot) — render a single pixel
              EoGePoint4d ndcPoint(tmInv * lnS.begin);
              view->ModelViewTransformPoint(ndcPoint);
              if (ndcPoint.IsInView()) {
                const auto clientPoint = view->ProjectToClient(ndcPoint);
                renderDevice->SetPixel(clientPoint, pColTbl[color]);
              }
            } else {
              ln = tmInv * lnS;
              ln.Display(view, renderDevice);
            }
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
          ln.Display(view, renderDevice);
        }
        iCurEdg = iCurEdg + 2;
      }
      // Update position of scan line
      dScan -= dSpac;
      dSecBeg -= dShift;
      goto l1;
    }
    transformMatrix = savedMatrix;  // exact restore — no accumulated FP error
  }
  Gs::renderState.SetPen(view, renderDevice, color, lineType);
}

}  // anonymous namespace

void Polygon_Display(AeSysView* view, EoGsRenderDevice* renderDevice, EoGePoint4dArray& ndcPoints) {
  const int numberOfPoints = static_cast<int>(ndcPoints.GetSize());
  if (numberOfPoints < 2) { return; }

  std::vector<CPoint> clientPoints(static_cast<size_t>(numberOfPoints));

  view->ProjectToClient(clientPoints.data(), ndcPoints);

  if (Gs::renderState.PolygonIntStyle() == EoDb::PolygonStyle::Solid) {
    renderDevice->SelectSolidBrush(pColTbl[Gs::renderState.Color()]);
    renderDevice->Polygon(clientPoints.data(), numberOfPoints);
    renderDevice->RestoreBrush();
  } else if (Gs::renderState.PolygonIntStyle() == EoDb::PolygonStyle::Hollow) {
    renderDevice->SelectNullBrush();
    renderDevice->Polygon(clientPoints.data(), numberOfPoints);
    renderDevice->RestoreBrush();
  } else {
    renderDevice->Polygon(clientPoints.data(), numberOfPoints);
  }
}

namespace {

/** @brief Reverse-maps an AeSys fill style index to its DXF hatch pattern name.
 *
 * This is the inverse of MapHatchPatternNameToIndex in EoDbDxfInterface.cpp.
 * Returns L"SOLID" for solid fills, L"HOLLOW" for index 0 or unrecognized indices.
 *
 * @param fillStyleIndex  The 1-based fill style index from EoDbPolygon.
 * @return The DXF pattern name string.
 */
std::wstring MapFillStyleIndexToPatternName(std::int16_t fillStyleIndex) {
  static const struct {
    std::int16_t index;
    const wchar_t* name;
  } patternTable[] = {{1, L"PEG1"},
      {2, L"PEG2"},
      {3, L"ANGLE"},
      {4, L"ANSI31"},
      {5, L"ANSI32"},
      {6, L"ANSI33"},
      {7, L"ANSI34"},
      {8, L"ANSI35"},
      {9, L"ANSI36"},
      {10, L"ANSI37"},
      {11, L"ANSI38"},
      {12, L"BOX"},
      {13, L"BRICK"},
      {14, L"CLAY"},
      {15, L"CORK"},
      {16, L"CROSS"},
      {17, L"DASH"},
      {18, L"DOLMIT"},
      {19, L"DOTS"},
      {20, L"EARTH"},
      {21, L"ESCHER"},
      {22, L"FLEX"},
      {23, L"GRASS"},
      {24, L"GRATE"},
      {25, L"HEX"},
      {26, L"HONEY"},
      {27, L"HOUND"},
      {28, L"INSUL"},
      {29, L"MUDST"},
      {30, L"NET3"},
      {31, L"PLAST"},
      {32, L"PLASTI"},
      {33, L"SACNCR"},
      {34, L"SQUARE"},
      {35, L"STARS"},
      {36, L"SWAMP"},
      {37, L"TRANS"},
      {38, L"TRIAN"},
      {39, L"ZIGZAG"},
      {40, L"AR-CONC"},
      {41, L"AR-SAND"}};

  for (const auto& entry : patternTable) {
    if (entry.index == fillStyleIndex) { return entry.name; }
  }
  return L"HOLLOW";
}
}  // anonymous namespace

EoDbPolygon::EoDbPolygon()
    : m_hatchOrigin{EoGePoint3d::kOrigin},
      m_positiveX{EoGeVector3d::positiveUnitX},
      m_positiveY{EoGeVector3d::positiveUnitY},
      m_vertices{},
      m_polygonStyle{EoDb::PolygonStyle::Hollow},
      m_fillStyleIndex{1},
      m_numberOfVertices{} {}

EoDbPolygon::EoDbPolygon(EoGePoint3dArray& points) {
  m_color = Gs::renderState.Color();
  m_polygonStyle = Gs::renderState.PolygonIntStyle();
  m_fillStyleIndex = Gs::renderState.PolygonIntStyleId();

  m_numberOfVertices = std::uint16_t(points.GetSize());

  if (m_numberOfVertices == 0) {
    m_hatchOrigin = EoGePoint3d::kOrigin;
    m_positiveX = EoGeVector3d::positiveUnitX;
    m_positiveY = EoGeVector3d::positiveUnitY;
    return;
  }

  m_hatchOrigin = points[0];

  if (m_numberOfVertices >= 3) {
    m_positiveX = EoGeVector3d(points[0], points[1]);
    m_positiveY = EoGeVector3d(points[0], points[2]);
    auto normal = CrossProduct(m_positiveX, m_positiveY);

    if (normal.IsNearNull()) {
      // Degenerate case — first three points are collinear.
      m_positiveX = EoGeVector3d::positiveUnitX;
      m_positiveY = EoGeVector3d::positiveUnitY;
    } else {
      normal.Unitize();

      if (normal.z < 0) { normal = -normal; }

      m_positiveX.Unitize();
      m_positiveX.RotateAboutArbitraryAxis(normal, hatch::dOffAng);
      m_positiveY = m_positiveX;
      m_positiveY.RotateAboutArbitraryAxis(normal, Eo::HalfPi);
      m_positiveX *= hatch::dXAxRefVecScal;
      m_positiveY *= hatch::dYAxRefVecScal;
    }
  } else {
    m_positiveX = EoGeVector3d::positiveUnitX;
    m_positiveY = EoGeVector3d::positiveUnitY;
  }

  // Project reference origin to plane

  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = points[i]; }
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
EoDbPolygon::EoDbPolygon(std::uint16_t numberOfVertices,
    EoGePoint3d origin,
    EoGeVector3d vXAx,
    EoGeVector3d vYAx,
    const EoGePoint3d* ppt) {
  m_color = Gs::renderState.Color();
  m_polygonStyle = Gs::renderState.PolygonIntStyle();
  m_fillStyleIndex = Gs::renderState.PolygonIntStyleId();
  m_numberOfVertices = numberOfVertices;
  m_hatchOrigin = origin;
  m_positiveX = vXAx;
  m_positiveY = vYAx;
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = ppt[i]; }
}

EoDbPolygon::EoDbPolygon(const EoGePoint3d& origin,
    const EoGeVector3d& xAxis,
    const EoGeVector3d& yAxis,
    EoGePoint3dArray& pts) {
  m_color = Gs::renderState.Color();
  m_polygonStyle = Gs::renderState.PolygonIntStyle();
  m_fillStyleIndex = Gs::renderState.PolygonIntStyleId();
  m_numberOfVertices = std::uint16_t(pts.GetSize());
  m_hatchOrigin = origin;
  m_positiveX = xAxis;
  m_positiveY = yAxis;
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = pts[i]; }
}

EoDbPolygon::EoDbPolygon(std::int16_t color,
    EoDb::PolygonStyle style,
    std::int16_t styleIndex,
    const EoGePoint3d& origin,
    const EoGeVector3d& xAxis,
    const EoGeVector3d& yAxis,
    EoGePoint3dArray& points)
    : m_hatchOrigin(origin), m_positiveX(xAxis), m_positiveY(yAxis) {
  m_color = color;
  m_polygonStyle = style;
  m_fillStyleIndex = styleIndex;
  m_numberOfVertices = std::uint16_t(points.GetSize());
  m_vertices = new EoGePoint3d[m_numberOfVertices];

  for (std::uint16_t n = 0; n < m_numberOfVertices; n++) { m_vertices[n] = points[n]; }
}

EoDbPolygon::EoDbPolygon(const EoDbPolygon& other) : EoDbPrimitive(other) {
  m_polygonStyle = other.m_polygonStyle;
  m_fillStyleIndex = other.m_fillStyleIndex;
  m_hatchOrigin = other.m_hatchOrigin;
  m_positiveX = other.m_positiveX;
  m_positiveY = other.m_positiveY;
  m_numberOfVertices = other.m_numberOfVertices;
  m_vertices = new EoGePoint3d[m_numberOfVertices];
  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i] = other.m_vertices[i]; }
  m_hatchPatternDoubleFlag = other.m_hatchPatternDoubleFlag;
  m_patternDefinitionLines = other.m_patternDefinitionLines;
}

const EoDbPolygon& EoDbPolygon::operator=(const EoDbPolygon& other) {
  if (this == &other) { return *this; }

  EoDbPrimitive::operator=(other);

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

  m_hatchPatternDoubleFlag = other.m_hatchPatternDoubleFlag;
  m_patternDefinitionLines = other.m_patternDefinitionLines;

  return *this;
}

EoDbPolygon::~EoDbPolygon() {
  delete[] m_vertices;
}

void EoDbPolygon::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  tvAddItem(tree, parent, L"<Polygon>", this);
}

EoDbPrimitive*& EoDbPolygon::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbPolygon(*this);
  return primitive;
}

void EoDbPolygon::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  const auto color = LogicalColor();

  Gs::renderState.SetColor(renderDevice, color);
  const EoDb::PolygonStyle polygonStyle =
      sm_SpecialPolygonStyle == EoDb::PolygonStyle::Special ? m_polygonStyle : sm_SpecialPolygonStyle;
  Gs::renderState.SetPolygonIntStyle(polygonStyle);  // hollow, solid, pattern, hatch
  Gs::renderState.SetPolygonIntStyleId(m_fillStyleIndex);

  const int iPtLstsId = m_numberOfVertices;

  if (m_polygonStyle == EoDb::PolygonStyle::Hatch) {
    EoGeTransformMatrix transformMatrix(m_hatchOrigin, m_positiveX, m_positiveY);
    DisplayFilAreaHatch(view, renderDevice, transformMatrix, 1, &iPtLstsId, m_vertices);
  } else {  // Fill area interior style is hollow, solid or pattern
    EoGePoint4dArray vertices;

    vertices.SetSize(m_numberOfVertices);

    for (auto i = 0; i < m_numberOfVertices; i++) { vertices[i] = EoGePoint4d(m_vertices[i]); }
    view->ModelViewTransformPoints(vertices);
    EoGePoint4d::ClipPolygon(vertices);
    Polygon_Display(view, renderDevice, vertices);
  }
}

void EoDbPolygon::ExportToDxf(EoDxfInterface* writer) const {
  if (m_numberOfVertices < 3) { return; }

  EoDxfHatch hatch;
  PopulateDxfBaseProperties(&hatch);

  // Elevation point carries the Z coordinate of the hatch plane
  hatch.m_elevationPoint = {0.0, 0.0, m_hatchOrigin.z};

  // Map EoDbPolygon style to DXF hatch properties
  switch (m_polygonStyle) {
    case EoDb::PolygonStyle::Solid:
      hatch.m_solidFillFlag = 1;
      hatch.m_hatchPatternName = L"SOLID";
      hatch.m_hatchPatternType = 1;  // predefined
      break;
    case EoDb::PolygonStyle::Hatch: {
      hatch.m_solidFillFlag = 0;
      hatch.m_hatchPatternName = MapFillStyleIndexToPatternName(m_fillStyleIndex);
      hatch.m_hatchPatternType = 1;  // predefined

      // Recover pattern angle and scale from the reference vectors.
      // ConvertHatchEntity encodes: xAxis = {cos(angle)*scale, sin(angle)*scale, 0}
      const double scale = m_positiveX.Length();
      if (scale > Eo::geometricTolerance) {
        hatch.m_hatchPatternScaleOrSpacing = scale;
        hatch.m_hatchPatternAngle = Eo::RadianToDegree(std::atan2(m_positiveX.y, m_positiveX.x));
      }
      break;
    }
    case EoDb::PolygonStyle::Hollow:
      [[fallthrough]];
    default:
      hatch.m_solidFillFlag = 0;
      hatch.m_hatchPatternName = L"HOLLOW";
      hatch.m_hatchPatternType = 1;  // predefined
      break;
  }

  hatch.m_hatchStyle = 0;  // normal
  hatch.m_associativityFlag = 0;  // non-associative

  // Passthrough: restore DXF pattern definition lines and double flag for non-solid hatches
  hatch.m_hatchPatternDoubleFlag = m_hatchPatternDoubleFlag;
  hatch.m_patternDefinitionLines = m_patternDefinitionLines;

  // Build a single polyline boundary loop from the polygon vertices
  auto* hatchLoop = new EoDxfHatchLoop(0x01 | 0x02);  // external + polyline

  auto polyline = std::make_unique<EoDxfLwPolyline>();
  polyline->m_polylineFlag = 1;  // closed
  polyline->m_numberOfVertices = static_cast<std::int32_t>(m_numberOfVertices);
  polyline->m_vertices.reserve(m_numberOfVertices);
  for (std::uint16_t i = 0; i < m_numberOfVertices; ++i) {
    polyline->m_vertices.emplace_back(m_vertices[i].x, m_vertices[i].y);
  }

  hatchLoop->m_entities.push_back(std::move(polyline));
  hatch.AppendLoop(hatchLoop);

  writer->AddHatch(hatch);
}

void EoDbPolygon::AddReportToMessageList(const EoGePoint3d& point) {
  CString message(L"<Polygon Edge> ");

  if (sm_Edge > 0 && sm_Edge <= m_numberOfVertices) {
    EoGePoint3d* pBegPt = &m_vertices[sm_Edge - 1];
    EoGePoint3d* pEndPt = &m_vertices[sm_Edge % m_numberOfVertices];

    if (sm_pivotVertex < m_numberOfVertices) {
      pBegPt = &m_vertices[sm_pivotVertex];
      pEndPt = &m_vertices[SwingVertex()];
    }
    double dAng;
    const double dLen = EoGeVector3d(*pBegPt, *pEndPt).Length();  // Length of edge

    if (EoGeVector3d(point, *pBegPt).Length() > dLen * 0.5) {
      dAng = EoGeLine(*pEndPt, *pBegPt).AngleFromXAxisXY();
    } else {
      dAng = EoGeLine(*pBegPt, *pEndPt).AngleFromXAxisXY();
    }

    CString formattedLength;
    app.FormatLength(formattedLength, app.GetUnits(), dLen);
    message.Append(formattedLength.TrimLeft());
    wchar_t szBuf[24]{};
    swprintf_s(szBuf, 24, L" @ %6.2f degrees", Eo::RadianToDegree(dAng));
    message.Append(szBuf);
    app.AddStringToMessageList(message);
    EoDbPrimitive::AddReportToMessageList(point);

    app.SetEngagedLength(dLen);
    app.SetEngagedAngle(dAng);
#ifdef USING_DDE
    dde::PostAdvise(dde::EngLenInfo);
    dde::PostAdvise(dde::EngAngZInfo);
#endif
  }
}

void EoDbPolygon::FormatGeometry(CString& str) {
  str += L"Hatch Origin;" + m_hatchOrigin.ToString();
  str += L"X Axis;" + m_positiveX.ToString();
  str += L"Y Axis;" + m_positiveY.ToString();

  for (auto i = 0; i < m_numberOfVertices; i++) { str += L"Vertex Point;" + m_vertices[i].ToString(); }
}

CString EoDbPolygon::FormatIntStyle() {
  const CString strStyle[] = {L"Hollow", L"Solid", L"Pattern", L"Hatch"};

  const CString str = (m_polygonStyle >= EoDb::PolygonStyle::Hollow && m_polygonStyle <= EoDb::PolygonStyle::Hatch)
      ? strStyle[static_cast<int>(m_polygonStyle)]
      : CString(L"Invalid!");

  return str;
}
void EoDbPolygon::FormatExtra(CString& str) {
  EoDbPrimitive::FormatExtra(str);
  str.AppendFormat(L"Points;%d", m_numberOfVertices);
  str += L'\t';
}
EoGePoint3d EoDbPolygon::GetControlPoint() {
  const auto wBeg = static_cast<std::uint16_t>(sm_Edge - 1);
  const auto wEnd = static_cast<std::uint16_t>(sm_Edge % m_numberOfVertices);
  const auto pt = EoGeLine(m_vertices[wBeg], m_vertices[wEnd]).Midpoint();
  return pt;
};

EoGePoint3d EoDbPolygon::GoToNextControlPoint() {
  if (sm_pivotVertex >= m_numberOfVertices) {  // have not yet rocked to a vertex
    const auto wBeg = static_cast<std::uint16_t>(sm_Edge - 1);
    const auto wEnd = static_cast<std::uint16_t>(sm_Edge % m_numberOfVertices);

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

bool EoDbPolygon::SelectUsingLine([[maybe_unused]] AeSysView* view,
    [[maybe_unused]] EoGeLine line,
    [[maybe_unused]] EoGePoint3dArray&) {
  return false;
}

bool EoDbPolygon::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  if (sm_EdgeToEvaluate > 0 && sm_EdgeToEvaluate <= m_numberOfVertices) {  // Evaluate specified edge of polygon
    EoGePoint4d ptBeg(m_vertices[sm_EdgeToEvaluate - 1]);
    EoGePoint4d ptEnd(m_vertices[sm_EdgeToEvaluate % m_numberOfVertices]);

    view->ModelViewTransformPoint(ptBeg);
    view->ModelViewTransformPoint(ptEnd);

    EoGeLine edge(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd});
    if (edge.IsSelectedByPointXY(EoGePoint3d{point}, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
      ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
      return true;
    }
  } else {  // Evaluate entire polygon
    EoGePoint4d ptBeg(m_vertices[0]);
    view->ModelViewTransformPoint(ptBeg);

    for (std::uint16_t w = 1; w <= m_numberOfVertices; w++) {
      EoGePoint4d ptEnd(m_vertices[w % m_numberOfVertices]);
      view->ModelViewTransformPoint(ptEnd);

      EoGeLine edge(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd});
      if (edge.IsSelectedByPointXY(EoGePoint3d{point}, view->SelectApertureSize(), ptProj, &sm_RelationshipOfPoint)) {
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

bool EoDbPolygon::IsWhollyContainedByRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  EoGePoint3dArray pts;
  for (auto i = 0; i < m_numberOfVertices; i++) { pts.Add(m_vertices[i]); }
  return polyline::IsWhollyContainedByRectangle(view, pt1, pt2, pts);
}
void EoDbPolygon::ModifyState() {
  EoDbPrimitive::ModifyState();

  m_polygonStyle = Gs::renderState.PolygonIntStyle();
  m_fillStyleIndex = Gs::renderState.PolygonIntStyleId();
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
void EoDbPolygon::GetExtents(AeSysView* view,
    EoGePoint3d& ptMin,
    EoGePoint3d& ptMax,
    const EoGeTransformMatrix& transformMatrix) {
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
  EoGePoint4d ndcPoints[2]{};

  ndcPoints[0] = EoGePoint4d{m_vertices[0]};
  view->ModelViewTransformPoint(ndcPoints[0]);

  for (int i = m_numberOfVertices - 1; i >= 0; i--) {
    ndcPoints[1] = EoGePoint4d{m_vertices[i]};
    view->ModelViewTransformPoint(ndcPoints[1]);
    if (EoGePoint4d::ClipLine(ndcPoints[0], ndcPoints[1])) { return true; }
    ndcPoints[0] = ndcPoints[1];
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

    const double dDis = point.DistanceToPointXY(pt);

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
  normal.Unitize();

  if (normal.z < 0) { normal = -normal; }

  m_positiveX.Unitize();
  m_positiveX.RotateAboutArbitraryAxis(normal, dOffAng);
  m_positiveY = m_positiveX;
  m_positiveY.RotateAboutArbitraryAxis(normal, Eo::HalfPi);
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
  // Note: DWORD mask limits selective translation to the first 32 vertices

  for (auto i = 0; i < m_numberOfVertices; i++) {
    if (((mask >> i) & 1UL) == 1) { m_vertices[i] += v; }
  }
}

EoDbPolygon* EoDbPolygon::ReadFromPeg(CFile& file) {
  const auto penColor = EoDb::ReadInt16(file);
  const auto polygonStyle = static_cast<EoDb::PolygonStyle>(EoDb::ReadInt16(file));
  const auto interiorStyleIndex = EoDb::ReadInt16(file);
  const auto numberOfPoints = EoDb::ReadUInt16(file);
  const auto hatchOrigin = EoDb::ReadPoint3d(file);
  const auto hatchXAxis = EoDb::ReadVector3d(file);
  const auto hatchYAxis = EoDb::ReadVector3d(file);

  EoGePoint3dArray points;
  points.SetSize(numberOfPoints);
  for (std::uint16_t n = 0; n < numberOfPoints; n++) { points[n] = EoDb::ReadPoint3d(file); }
  return new EoDbPolygon(penColor, polygonStyle, interiorStyleIndex, hatchOrigin, hatchXAxis, hatchYAxis, points);
}

bool EoDbPolygon::Write(CFile& file) {
  EoDb::WriteUInt16(file, std::uint16_t(EoDb::kPolygonPrimitive));
  EoDb::WriteInt16(file, m_color);
  // note polygon style stuffed up into unused line type on io
  EoDb::WriteInt16(file, static_cast<std::int16_t>(m_polygonStyle));
  EoDb::WriteInt16(file, m_fillStyleIndex);
  EoDb::WriteUInt16(file, m_numberOfVertices);
  m_hatchOrigin.Write(file);
  m_positiveX.Write(file);
  m_positiveY.Write(file);

  for (auto i = 0; i < m_numberOfVertices; i++) { m_vertices[i].Write(file); }

  return true;
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