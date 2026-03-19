#include <memory>
#include <string>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfHatch.h"
#include "EoDxfReader.h"
#include "EoDxfSpline.h"

void EoDxfHatch::AddLine() {
  ClearEntities();
  if (m_hatchLoop) {
    auto entity = std::make_unique<EoDxfLine>();
    m_line = entity.get();
    m_hatchLoop->m_entities.push_back(std::move(entity));
  }
}

void EoDxfHatch::AddArc() {
  ClearEntities();
  if (m_hatchLoop) {
    auto entity = std::make_unique<EoDxfArc>();
    m_arc = entity.get();
    m_hatchLoop->m_entities.push_back(std::move(entity));
  }
}

void EoDxfHatch::AddEllipse() {
  ClearEntities();
  if (m_hatchLoop) {
    auto entity = std::make_unique<EoDxfEllipse>();
    m_ellipse = entity.get();
    m_hatchLoop->m_entities.push_back(std::move(entity));
  }
}

void EoDxfHatch::AddSpline() {
  ClearEntities();
  if (m_hatchLoop) {
    m_point = nullptr;
    auto entity = std::make_unique<EoDxfSpline>();
    m_spline = entity.get();
    m_hatchLoop->m_entities.push_back(std::move(entity));
  }
}

void EoDxfHatch::ClearEntities() noexcept {
  m_point = nullptr;
  m_line = nullptr;
  m_polyline = nullptr;
  m_arc = nullptr;
  m_ellipse = nullptr;
  m_spline = nullptr;
  m_polylineVertex = nullptr;
  // Entity-level state (elevation, seed points) must NOT be reset here.
  // ClearEntities() is called when switching between edge types within a
  // single HATCH entity.  Resetting m_isElevationPointParsed would cause
  // subsequent code 10/20 values to route to the elevation point instead
  // of to the new edge entity's coordinates.
}

void EoDxfHatch::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10: {
      double val = reader.GetDouble();

      if (!m_isElevationPointParsed) {
        m_elevationPoint.x = val;  // always 0.0
      } else if (m_isReadingSeedPoints && m_seedPointsRemaining > 0) {
        // ← SEED POINT PATH (never reaches AddLine etc.)
        auto entity = std::make_unique<EoDxfPoint>();
        m_currentSeedPoint = entity.get();
        m_seedPoints.push_back(std::move(entity));
        m_currentSeedPoint->m_pointLocation.x = val;
      } else if (m_polyline) {
        if (!m_polylineVertex) { m_polylineVertex = &m_polyline->AddVertex(); }
        m_polylineVertex->x = val;
      } else if (m_line) {
        m_line->m_startPoint.x = val;
      } else if (m_arc) {
        m_arc->m_centerPoint.x = val;
      } else if (m_ellipse) {
        m_ellipse->m_centerPoint.x = val;
      } else if (m_spline) {
        // TODO: spline control point routing
      }
      break;
    }
    case 20: {
      double val = reader.GetDouble();

      if (!m_isElevationPointParsed) {
        m_elevationPoint.y = val;  // always 0.0
      } else if (m_currentSeedPoint) {
        // ← SEED POINT PATH
        m_currentSeedPoint->m_pointLocation.y = val;
        m_currentSeedPoint = nullptr;  // pair complete
        --m_seedPointsRemaining;
      } else if (m_polylineVertex) {
        m_polylineVertex->y = val;
        m_polylineVertex = nullptr;
      } else if (m_line) {
        m_line->m_startPoint.y = val;
      } else if (m_arc) {
        m_arc->m_centerPoint.y = val;
      } else if (m_ellipse) {
        m_ellipse->m_centerPoint.y = val;
      } else if (m_spline) {
        // TODO: spline control point routing
      }
      break;
    }
    case 30:
      m_elevationPoint.z = reader.GetDouble();  // elevation value for the hatch
      m_isElevationPointParsed = true;
      break;

      // --------------------------------------------------
      // SEED POINTS (after all boundary paths)
      // --------------------------------------------------
    case 98: {
      m_seedPointsRemaining = reader.GetInt32();
      m_seedPoints.clear();
      m_isReadingSeedPoints = (m_seedPointsRemaining > 0);
      m_currentSeedPoint = nullptr;
      break;
    }
    case 2:
      m_hatchPatternName = reader.GetWideString();
      break;
    case 70:
      m_solidFillFlag = reader.GetInt16();
      break;
    case 71:
      m_associativityFlag = reader.GetInt16();
      break;
    case 11:
      if (m_line) {
        m_line->m_endPoint.x = reader.GetDouble();
      } else if (m_ellipse) {
        m_ellipse->m_endPointOfMajorAxis.x = reader.GetDouble();
      }
      break;
    case 21:
      if (m_line) {
        m_line->m_endPoint.y = reader.GetDouble();
      } else if (m_ellipse) {
        m_ellipse->m_endPointOfMajorAxis.y = reader.GetDouble();
      }
      break;
    case 40:
      if (m_arc) {
        m_arc->m_radius = reader.GetDouble();
      } else if (m_ellipse) {
        m_ellipse->m_ratio = reader.GetDouble();
      } else if (m_spline) {
        // Add routing for the active m_spline data groups
      }
      break;
    case 41:
      m_hatchPatternScaleOrSpacing = reader.GetDouble();
      break;
    case 42:
      if (m_polylineVertex) { m_polylineVertex->bulge = reader.GetDouble(); }
      break;
    case 50:
      if (m_arc) {
        m_arc->m_startAngle = reader.GetDouble() * EoDxf::DegreesToRadians;
      } else if (m_ellipse) {
        m_ellipse->m_startParam = reader.GetDouble();
      }
      break;
    case 51:
      if (m_arc) {
        m_arc->m_endAngle = reader.GetDouble() * EoDxf::DegreesToRadians;
      } else if (m_ellipse) {
        m_ellipse->m_endParam = reader.GetDouble();
      }
      break;
    case 52:
      m_hatchPatternAngle = reader.GetDouble();
      break;
    case 73:
      if (m_arc) {
        m_arc->m_isCounterClockwise = reader.GetInt16();
      } else if (m_ellipse) {
        m_ellipse->m_isCounterClockwise = reader.GetInt16();
      } else if (m_polyline) {
        m_polyline->m_polylineFlag = reader.GetInt16();
      }
      break;
    case 75:
      m_hatchStyle = reader.GetInt16();
      break;
    case 76:
      m_hatchPatternType = reader.GetInt16();
      break;
    case 77:
      m_hatchPatternDoubleFlag = reader.GetInt16();
      break;
    case 78:
      m_numberOfPatternDefinitionLines = reader.GetInt16();
      break;

    case 91:
      m_numberOfBoundaryPaths = reader.GetInt32();
      m_hatchLoops.reserve(m_numberOfBoundaryPaths);
      break;

    // Hatch boundary path data groups
    case 72: {
      int edgeType = reader.GetInt16();
      if (m_hatchLoop) {
        switch (edgeType) {
          case 1:
            AddLine();
            break;
          case 2:
            AddArc();
            break;
          case 3:
            AddEllipse();
            break;
          case 4:
            AddSpline();
            break;
            // case for polyline boundaries usually handled via 92/93
        }
      }
      break;
    }
    case 92: {
      auto boundaryPathType = reader.GetInt32();
      m_hatchLoop = new EoDxfHatchLoop(boundaryPathType);
      m_hatchLoops.push_back(m_hatchLoop);
      if (boundaryPathType & 2) {  // polyline
        m_isPolyline = true;
        ClearEntities();
        auto entity = std::make_unique<EoDxfLwPolyline>();
        m_polyline = entity.get();
        m_hatchLoop->m_entities.push_back(std::move(entity));
      } else {
        m_isPolyline = false;
      }
      break;
    }
    case 93:
      if (m_polyline) {
        m_polyline->m_numberOfVertices = reader.GetInt32();
        m_polyline->m_vertices.reserve(m_polyline->m_numberOfVertices);  // modern reserve on new container
      } else if (m_hatchLoop) {
        m_hatchLoop->m_numberOfEdges = reader.GetInt32();
      }
      break;

    // Spline-specific data groups (94, 95, 96) are stored on the active m_spline if it exists, otherwise ignored
    case 94:
      if (m_spline) { m_spline->m_degreeOfTheSplineCurve = reader.GetInt16(); }
      break;
    case 95:
      if (m_spline) {
        m_spline->m_numberOfKnots = reader.GetInt16();
        m_spline->m_knotValues.reserve(m_spline->m_numberOfKnots);
      }
      break;
    case 96:
      if (m_spline) { m_spline->m_numberOfControlPoints = reader.GetInt16(); }
      break;

    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}
