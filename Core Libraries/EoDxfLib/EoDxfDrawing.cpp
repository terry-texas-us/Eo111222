#include "EoDxfWrite.h"

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfWriter.h"

bool EoDxfWrite::Write3dFace(EoDxf3dFace* face) {
  m_writer->WriteString(0, "3DFACE");
  WriteEntity(face);
  m_writer->WriteString(100, "AcDbFace");
  m_writer->WriteDouble(10, face->m_firstPoint.x);
  m_writer->WriteDouble(20, face->m_firstPoint.y);
  m_writer->WriteDouble(30, face->m_firstPoint.z);
  m_writer->WriteDouble(11, face->m_secondPoint.x);
  m_writer->WriteDouble(21, face->m_secondPoint.y);
  m_writer->WriteDouble(31, face->m_secondPoint.z);
  m_writer->WriteDouble(12, face->m_thirdPoint.x);
  m_writer->WriteDouble(22, face->m_thirdPoint.y);
  m_writer->WriteDouble(32, face->m_thirdPoint.z);
  m_writer->WriteDouble(13, face->m_fourthPoint.x);
  m_writer->WriteDouble(23, face->m_fourthPoint.y);
  m_writer->WriteDouble(33, face->m_fourthPoint.z);
  m_writer->WriteInt16(70, face->m_invisibleFlag);
  return true;
}

bool EoDxfWrite::WriteArc(EoDxfArc* arc) {
  m_writer->WriteString(0, "ARC");
  WriteEntity(arc);
  m_writer->WriteString(100, "AcDbCircle");
  m_writer->WriteDouble(10, arc->m_firstPoint.x);
  m_writer->WriteDouble(20, arc->m_firstPoint.y);
  if (arc->m_firstPoint.z != 0.0) { m_writer->WriteDouble(30, arc->m_firstPoint.z); }
  m_writer->WriteDouble(40, arc->m_radius);
  m_writer->WriteString(100, "AcDbArc");
  m_writer->WriteDouble(50, arc->m_startAngle * EoDxf::RadiansToDegrees);
  m_writer->WriteDouble(51, arc->m_endAngle * EoDxf::RadiansToDegrees);
  return true;
}

bool EoDxfWrite::WriteCircle(EoDxfCircle* circle) {
  m_writer->WriteString(0, "CIRCLE");
  WriteEntity(circle);
  m_writer->WriteString(100, "AcDbCircle");
  m_writer->WriteDouble(10, circle->m_firstPoint.x);
  m_writer->WriteDouble(20, circle->m_firstPoint.y);
  if (circle->m_firstPoint.z != 0.0) { m_writer->WriteDouble(30, circle->m_firstPoint.z); }
  m_writer->WriteDouble(40, circle->m_radius);
  return true;
}

bool EoDxfWrite::WriteEllipse(EoDxfEllipse* ellipse) {
  // verify axis/ratio and params for full ellipse
  ellipse->CorrectAxis();

  m_writer->WriteString(0, "ELLIPSE");
  WriteEntity(ellipse);
  m_writer->WriteString(100, "AcDbEllipse");
  m_writer->WriteDouble(10, ellipse->m_firstPoint.x);
  m_writer->WriteDouble(20, ellipse->m_firstPoint.y);
  m_writer->WriteDouble(30, ellipse->m_firstPoint.z);
  m_writer->WriteDouble(11, ellipse->m_secondPoint.x);
  m_writer->WriteDouble(21, ellipse->m_secondPoint.y);
  m_writer->WriteDouble(31, ellipse->m_secondPoint.z);
  m_writer->WriteDouble(40, ellipse->m_ratio);
  m_writer->WriteDouble(41, ellipse->m_startParam);
  m_writer->WriteDouble(42, ellipse->m_endParam);

  return true;
}

bool EoDxfWrite::WriteHatch(EoDxfHatch* hatch) {
  m_writer->WriteString(0, "HATCH");
  WriteEntity(hatch);
  m_writer->WriteString(100, "AcDbHatch");
  m_writer->WriteDouble(10, hatch->m_firstPoint.x);
  m_writer->WriteDouble(20, hatch->m_firstPoint.y);
  m_writer->WriteDouble(30, hatch->m_firstPoint.z);
  m_writer->WriteDouble(210, hatch->m_extrusionDirection.x);
  m_writer->WriteDouble(220, hatch->m_extrusionDirection.y);
  m_writer->WriteDouble(230, hatch->m_extrusionDirection.z);
  m_writer->WriteString(2, hatch->m_hatchPatternName);
  m_writer->WriteInt16(70, hatch->m_solidFillFlag);
  m_writer->WriteInt16(71, hatch->m_associativityFlag);
  hatch->m_numberOfBoundaryPaths = static_cast<int>(hatch->m_hatchLoops.size());
  m_writer->WriteInt32(91, hatch->m_numberOfBoundaryPaths);
  // write paths data
  for (int i = 0; i < hatch->m_numberOfBoundaryPaths; i++) {
    auto* hatchLoop = hatch->m_hatchLoops.at(i);
    m_writer->WriteInt32(92, hatchLoop->m_boundaryPathType);
    if ((hatchLoop->m_boundaryPathType & 2) == 2) {
      // polyline boundary path
      if (!hatchLoop->m_entities.empty() && hatchLoop->m_entities.front()->m_entityType == EoDxf::LWPOLYLINE) {
        auto* polyline = static_cast<EoDxfLwPolyline*>(hatchLoop->m_entities.front().get());
        m_writer->WriteInt16(72, (polyline->m_constantWidth != 0.0) ? 1 : 0);
        m_writer->WriteInt16(73, polyline->m_polylineFlag);
        m_writer->WriteInt32(93, static_cast<int>(polyline->m_vertices.size()));
        for (const auto& v : polyline->m_vertices) {
          m_writer->WriteDouble(10, v.x);
          m_writer->WriteDouble(20, v.y);
          if (v.bulge != 0.0) { m_writer->WriteDouble(42, v.bulge); }
        }
      }
      m_writer->WriteInt32(97, 0);
    } else {
      hatchLoop->Update();
      m_writer->WriteInt32(93, hatchLoop->m_numberOfEdges);
      for (int j = 0; j < hatchLoop->m_numberOfEdges; ++j) {
        switch ((hatchLoop->m_entities.at(j))->m_entityType) {
          case EoDxf::LINE: {
            m_writer->WriteInt16(72, 1);
            auto* line = static_cast<EoDxfLine*>(hatchLoop->m_entities.at(j).get());
            m_writer->WriteDouble(10, line->m_firstPoint.x);
            m_writer->WriteDouble(20, line->m_firstPoint.y);
            m_writer->WriteDouble(11, line->m_secondPoint.x);
            m_writer->WriteDouble(21, line->m_secondPoint.y);
            break;
          }
          case EoDxf::ARC: {
            m_writer->WriteInt16(72, 2);
            auto* arc = static_cast<EoDxfArc*>(hatchLoop->m_entities.at(j).get());
            m_writer->WriteDouble(10, arc->m_firstPoint.x);
            m_writer->WriteDouble(20, arc->m_firstPoint.y);
            m_writer->WriteDouble(40, arc->m_radius);
            m_writer->WriteDouble(50, arc->m_startAngle * EoDxf::RadiansToDegrees);
            m_writer->WriteDouble(51, arc->m_endAngle * EoDxf::RadiansToDegrees);
            m_writer->WriteInt16(73, arc->m_isCounterClockwise);
            break;
          }
          case EoDxf::ELLIPSE: {
            m_writer->WriteInt16(72, 3);
            auto* ellipse = static_cast<EoDxfEllipse*>(hatchLoop->m_entities.at(j).get());
            ellipse->CorrectAxis();
            m_writer->WriteDouble(10, ellipse->m_firstPoint.x);
            m_writer->WriteDouble(20, ellipse->m_firstPoint.y);
            m_writer->WriteDouble(11, ellipse->m_secondPoint.x);
            m_writer->WriteDouble(21, ellipse->m_secondPoint.y);
            m_writer->WriteDouble(40, ellipse->m_ratio);
            m_writer->WriteDouble(50, ellipse->m_startParam);
            m_writer->WriteDouble(51, ellipse->m_endParam);
            m_writer->WriteInt16(73, ellipse->m_isCounterClockwise);
            break;
          }
          case EoDxf::SPLINE:
            break;
          default:
            break;
        }
      }
      m_writer->WriteInt32(97, 0);
    }
  }
  m_writer->WriteInt16(75, hatch->m_hatchStyle);
  m_writer->WriteInt16(76, hatch->m_hatchPatternType);
  if (!hatch->m_solidFillFlag) {
    m_writer->WriteDouble(52, hatch->m_hatchPatternAngle);
    m_writer->WriteDouble(41, hatch->m_hatchPatternScaleOrSpacing);
    m_writer->WriteInt16(77, hatch->m_hatchPatternDoubleFlag);
    m_writer->WriteInt16(78, hatch->m_numberOfPatternDefinitionLines);
  }
  m_writer->WriteInt32(98, 0);

  return true;
}

bool EoDxfWrite::WriteLine(EoDxfLine* line) {
  m_writer->WriteString(0, "LINE");
  WriteEntity(line);
  m_writer->WriteString(100, "AcDbLine");
  m_writer->WriteDouble(10, line->m_firstPoint.x);
  m_writer->WriteDouble(20, line->m_firstPoint.y);
  if (line->m_firstPoint.z != 0.0 || line->m_secondPoint.z != 0.0) {
    m_writer->WriteDouble(30, line->m_firstPoint.z);
    m_writer->WriteDouble(11, line->m_secondPoint.x);
    m_writer->WriteDouble(21, line->m_secondPoint.y);
    m_writer->WriteDouble(31, line->m_secondPoint.z);
  } else {
    m_writer->WriteDouble(11, line->m_secondPoint.x);
    m_writer->WriteDouble(21, line->m_secondPoint.y);
  }
  return true;
}

bool EoDxfWrite::WritePoint(EoDxfPoint* point) {
  m_writer->WriteString(0, "POINT");
  WriteEntity(point);
  m_writer->WriteString(100, "AcDbPoint");
  m_writer->WriteDouble(10, point->m_firstPoint.x);
  m_writer->WriteDouble(20, point->m_firstPoint.y);
  if (point->m_firstPoint.z != 0.0) { m_writer->WriteDouble(30, point->m_firstPoint.z); }
  return true;
}

bool EoDxfWrite::WriteRay(EoDxfRay* ray) {
  m_writer->WriteString(0, "RAY");
  WriteEntity(ray);
  m_writer->WriteString(100, "AcDbRay");
  m_writer->WriteDouble(10, ray->m_firstPoint.x);
  m_writer->WriteDouble(20, ray->m_firstPoint.y);
  if (std::fabs(ray->m_firstPoint.z) > EoDxf::geometricTolerance) { m_writer->WriteDouble(30, ray->m_firstPoint.z); }
  auto direction = ray->m_secondPoint;
  direction.Unitize();
  m_writer->WriteDouble(11, direction.x);
  m_writer->WriteDouble(21, direction.y);
  if (std::fabs(direction.z) > EoDxf::geometricTolerance) { m_writer->WriteDouble(31, direction.z); }
  return true;
}

bool EoDxfWrite::WriteSolid(EoDxfSolid* solid) {
  m_writer->WriteString(0, "SOLID");
  WriteEntity(solid);
  m_writer->WriteString(100, "AcDbTrace");  // SOLID shares the same subclass as TRACE
  m_writer->WriteDouble(10, solid->m_firstPoint.x);
  m_writer->WriteDouble(20, solid->m_firstPoint.y);
  m_writer->WriteDouble(30, solid->m_firstPoint.z);
  m_writer->WriteDouble(11, solid->m_secondPoint.x);
  m_writer->WriteDouble(21, solid->m_secondPoint.y);
  m_writer->WriteDouble(31, solid->m_secondPoint.z);
  m_writer->WriteDouble(12, solid->m_thirdPoint.x);
  m_writer->WriteDouble(22, solid->m_thirdPoint.y);
  m_writer->WriteDouble(32, solid->m_thirdPoint.z);
  m_writer->WriteDouble(13, solid->m_fourthPoint.x);
  m_writer->WriteDouble(23, solid->m_fourthPoint.y);
  m_writer->WriteDouble(33, solid->m_fourthPoint.z);
  return true;
}

bool EoDxfWrite::WriteTrace(EoDxfTrace* trace) {
  m_writer->WriteString(0, "TRACE");
  WriteEntity(trace);
  m_writer->WriteString(100, "AcDbTrace");
  m_writer->WriteDouble(10, trace->m_firstPoint.x);
  m_writer->WriteDouble(20, trace->m_firstPoint.y);
  m_writer->WriteDouble(30, trace->m_firstPoint.z);
  m_writer->WriteDouble(11, trace->m_secondPoint.x);
  m_writer->WriteDouble(21, trace->m_secondPoint.y);
  m_writer->WriteDouble(31, trace->m_secondPoint.z);
  m_writer->WriteDouble(12, trace->m_thirdPoint.x);
  m_writer->WriteDouble(22, trace->m_thirdPoint.y);
  m_writer->WriteDouble(32, trace->m_thirdPoint.z);
  m_writer->WriteDouble(13, trace->m_fourthPoint.x);
  m_writer->WriteDouble(23, trace->m_fourthPoint.y);
  m_writer->WriteDouble(33, trace->m_fourthPoint.z);
  return true;
}

bool EoDxfWrite::WriteXline(EoDxfXline* xline) {
  m_writer->WriteString(0, "XLINE");
  WriteEntity(xline);
  m_writer->WriteString(100, "AcDbXline");
  m_writer->WriteDouble(10, xline->m_firstPoint.x);
  m_writer->WriteDouble(20, xline->m_firstPoint.y);
  if (std::fabs(xline->m_firstPoint.z) > EoDxf::geometricTolerance) {
    m_writer->WriteDouble(30, xline->m_firstPoint.z);
  }
  auto direction = xline->m_secondPoint;
  direction.Unitize();
  m_writer->WriteDouble(11, direction.x);
  m_writer->WriteDouble(21, direction.y);
  if (std::fabs(direction.z) > EoDxf::geometricTolerance) { m_writer->WriteDouble(31, direction.z); }
  return true;
}

bool EoDxfWrite::WriteLWPolyline(EoDxfLwPolyline* polyline) {
  m_writer->WriteString(0, "LWPOLYLINE");
  WriteEntity(polyline);
  m_writer->WriteString(100, "AcDbPolyline");

  // Modern container – no more raw pointers
  polyline->m_numberOfVertices = static_cast<int>(polyline->m_vertices.size());
  m_writer->WriteInt32(90, polyline->m_numberOfVertices);
  m_writer->WriteInt16(70, polyline->m_polylineFlag);
  m_writer->WriteDouble(43, polyline->m_constantWidth);
  if (polyline->m_elevation != 0.0) { m_writer->WriteDouble(38, polyline->m_elevation); }
  if (polyline->m_thickness != 0.0) { m_writer->WriteDouble(39, polyline->m_thickness); }
  for (const auto& vertex : polyline->m_vertices) {
    m_writer->WriteDouble(10, vertex.x);
    m_writer->WriteDouble(20, vertex.y);
    if (vertex.stawidth != 0.0) { m_writer->WriteDouble(40, vertex.stawidth); }
    if (vertex.endwidth != 0.0) { m_writer->WriteDouble(41, vertex.endwidth); }
    if (vertex.bulge != 0.0) { m_writer->WriteDouble(42, vertex.bulge); }
  }

  return true;
}

bool EoDxfWrite::WritePolyline(EoDxfPolyline* polyline) {
  m_writer->WriteString(0, "POLYLINE");
  WriteEntity(polyline);

  const bool is3dPolyline = (polyline->m_polylineFlag & (8 | 16 | 64)) != 0;
  m_writer->WriteString(100, is3dPolyline ? "AcDb3dPolyline" : "AcDb2dPolyline");

  // Group code 66, `entities follow flag` is optional in AC1015, ignored in AC1018 and later)
  if (m_version == EoDxf::Version::AC1014) { m_writer->WriteInt16(66, 1); }

  m_writer->WriteDouble(10, 0.0);
  m_writer->WriteDouble(20, 0.0);
  m_writer->WriteDouble(30, polyline->m_firstPoint.z);
  if (polyline->m_thickness != 0) { m_writer->WriteDouble(39, polyline->m_thickness); }
  m_writer->WriteInt16(70, polyline->m_polylineFlag);
  if (polyline->m_defaultStartWidth != 0) { m_writer->WriteDouble(40, polyline->m_defaultStartWidth); }
  if (polyline->m_defaultEndWidth != 0) { m_writer->WriteDouble(41, polyline->m_defaultEndWidth); }
  if ((polyline->m_polylineFlag & 16) || (polyline->m_polylineFlag & 32)) {
    m_writer->WriteInt16(71, polyline->m_polygonMeshVertexCountM);
    m_writer->WriteInt16(72, polyline->m_polygonMeshVertexCountN);
  }
  if (polyline->m_smoothSurfaceDensityM != 0) { m_writer->WriteInt16(73, polyline->m_smoothSurfaceDensityM); }
  if (polyline->m_smoothSurfaceDensityN != 0) { m_writer->WriteInt16(74, polyline->m_smoothSurfaceDensityN); }
  if (polyline->m_curvesAndSmoothSurfaceType != 0) { m_writer->WriteInt16(75, polyline->m_curvesAndSmoothSurfaceType); }
  auto extrusionDirection = polyline->m_extrusionDirection;
  if (extrusionDirection.x > EoDxf::geometricTolerance || extrusionDirection.y < -EoDxf::geometricTolerance ||
      std::abs(extrusionDirection.z - 1.0) > EoDxf::geometricTolerance) {
    m_writer->WriteDouble(210, extrusionDirection.x);
    m_writer->WriteDouble(220, extrusionDirection.y);
    m_writer->WriteDouble(230, extrusionDirection.z);
  }

  // VERTEX entities
  const auto polylineHandle = polyline->m_handle;
  for (auto* vertex : polyline->m_vertices) {
    m_writer->WriteString(0, "VERTEX");
    // WriteEntity assigns a new handle and writes AcDbEntity subclass
    WriteEntity(vertex);

    // Base subclass marker required before the specific subclass
    if (vertex->m_vertexFlags & 128) {
      m_writer->WriteString(100, "AcDbPolyFaceMeshVertex");
    } else if (vertex->m_vertexFlags & 64) {
      m_writer->WriteString(100, "AcDbPolygonMeshVertex");
    } else if (vertex->m_vertexFlags & 32) {
      m_writer->WriteString(100, "AcDb3dPolylineVertex");
    } else {
      m_writer->WriteString(100, "AcDb2dVertex");
    }

    if ((vertex->m_vertexFlags & 128) != 0 && (vertex->m_vertexFlags & 64) == 0) {
      // Polyface face records (flag 128 without flag 64): dummy coordinates
      m_writer->WriteDouble(10, 0.0);
      m_writer->WriteDouble(20, 0.0);
      m_writer->WriteDouble(30, 0.0);
    } else {
      m_writer->WriteDouble(10, vertex->m_firstPoint.x);
      m_writer->WriteDouble(20, vertex->m_firstPoint.y);
      m_writer->WriteDouble(30, vertex->m_firstPoint.z);
    }
    if (vertex->m_startingWidth != 0) { m_writer->WriteDouble(40, vertex->m_startingWidth); }
    if (vertex->m_endingWidth != 0) { m_writer->WriteDouble(41, vertex->m_endingWidth); }
    if (vertex->m_bulge != 0.0) { m_writer->WriteDouble(42, vertex->m_bulge); }
    if (vertex->m_vertexFlags != 0) { m_writer->WriteInt16(70, vertex->m_vertexFlags); }
    if ((vertex->m_vertexFlags & 2) != 0) { m_writer->WriteDouble(50, vertex->m_curveFitTangentDirection); }

    if ((vertex->m_vertexFlags & 128) != 0) {
      if (vertex->m_polyfaceMeshVertexIndex1 != 0) { m_writer->WriteInt16(71, vertex->m_polyfaceMeshVertexIndex1); }
      if (vertex->m_polyfaceMeshVertexIndex2 != 0) { m_writer->WriteInt16(72, vertex->m_polyfaceMeshVertexIndex2); }
      if (vertex->m_polyfaceMeshVertexIndex3 != 0) { m_writer->WriteInt16(73, vertex->m_polyfaceMeshVertexIndex3); }
      if (vertex->m_polyfaceMeshVertexIndex4 != 0) { m_writer->WriteInt16(74, vertex->m_polyfaceMeshVertexIndex4); }

      if ((vertex->m_vertexFlags & 64) == 0 && vertex->m_identifier != 0) {
        m_writer->WriteInt32(91, vertex->m_identifier);
      }
    }
  }

  // SEQEND entity
  m_writer->WriteString(0, "SEQEND");

  EoDxfSeqEnd seqEnd{*polyline};
  seqEnd.m_handle = ++m_entityCount;
  m_writer->WriteString(5, ToHexString(seqEnd.m_handle));
  m_writer->WriteString(330, ToHexString(polylineHandle));
  m_writer->WriteString(100, "AcDbEntity");
  m_writer->WriteUtf8String(8, seqEnd.m_layer);

  return true;
}

bool EoDxfWrite::WriteSpline(EoDxfSpline* spline) {
  m_writer->WriteString(0, "SPLINE");
  WriteEntity(spline);
  m_writer->WriteString(100, "AcDbSpline");

  m_writer->WriteDouble(210, spline->m_normalVector.x);
  m_writer->WriteDouble(220, spline->m_normalVector.y);
  m_writer->WriteDouble(230, spline->m_normalVector.z);

  m_writer->WriteInt16(70, spline->m_splineFlag);
  m_writer->WriteInt16(71, spline->m_degreeOfTheSplineCurve);

  m_writer->WriteInt16(72, spline->m_numberOfKnots);
  m_writer->WriteInt16(73, spline->m_numberOfControlPoints);
  m_writer->WriteInt16(74, spline->m_numberOfFitPoints);

  m_writer->WriteDouble(42, spline->m_knotTolerance);
  m_writer->WriteDouble(43, spline->m_controlPointTolerance);
  m_writer->WriteDouble(44, spline->m_fitTolerance);

  // Start tangent (12, 22, 32) — write when spline flag bit 1 is set (tangent defined)
  if ((spline->m_splineFlag & 1) != 0) {
    m_writer->WriteDouble(12, spline->m_startTangent.x);
    m_writer->WriteDouble(22, spline->m_startTangent.y);
    m_writer->WriteDouble(32, spline->m_startTangent.z);
  }

  // End tangent (13, 23, 33)
  if ((spline->m_splineFlag & 1) != 0) {
    m_writer->WriteDouble(13, spline->m_endTangent.x);
    m_writer->WriteDouble(23, spline->m_endTangent.y);
    m_writer->WriteDouble(33, spline->m_endTangent.z);
  }

  for (int i = 0; i < spline->m_numberOfKnots; i++) { m_writer->WriteDouble(40, spline->m_knotValues.at(i)); }

  for (int i = 0; i < spline->m_numberOfControlPoints; i++) {
    auto* controlPoint = spline->m_controlPoints.at(i);
    m_writer->WriteDouble(10, controlPoint->x);
    m_writer->WriteDouble(20, controlPoint->y);
    m_writer->WriteDouble(30, controlPoint->z);
  }

  // Fit points (11, 21, 31)
  for (std::int32_t i = 0; i < spline->m_numberOfFitPoints; ++i) {
    const auto* fitPoint = spline->m_fitPoints[static_cast<size_t>(i)];
    m_writer->WriteDouble(11, fitPoint->x);
    m_writer->WriteDouble(21, fitPoint->y);
    m_writer->WriteDouble(31, fitPoint->z);
  }
  return true;
}
