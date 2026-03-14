#include "EoDxfWrite.h"

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfHatch.h"

bool EoDxfWrite::Write3dFace(EoDxf3dFace* face) {
  WriteCodeString(0, L"3DFACE");
  WriteEntity(face);
  WriteCodeString(100, L"AcDbFace");
  WriteCodeDouble(10, face->m_startPoint.x);
  WriteCodeDouble(20, face->m_startPoint.y);
  WriteCodeDouble(30, face->m_startPoint.z);
  WriteCodeDouble(11, face->m_endPoint.x);
  WriteCodeDouble(21, face->m_endPoint.y);
  WriteCodeDouble(31, face->m_endPoint.z);
  WriteCodeDouble(12, face->m_thirdPoint.x);
  WriteCodeDouble(22, face->m_thirdPoint.y);
  WriteCodeDouble(32, face->m_thirdPoint.z);
  WriteCodeDouble(13, face->m_fourthPoint.x);
  WriteCodeDouble(23, face->m_fourthPoint.y);
  WriteCodeDouble(33, face->m_fourthPoint.z);
  WriteCodeInt16(70, face->m_invisibleFlag);
  return m_writeOk;
}

bool EoDxfWrite::WriteArc(EoDxfArc* arc) {
  WriteCodeString(0, L"ARC");
  WriteEntity(arc);
  WriteCodeString(100, L"AcDbCircle");
  WriteCodeDouble(10, arc->m_centerPoint.x);
  WriteCodeDouble(20, arc->m_centerPoint.y);
  if (std::abs(arc->m_centerPoint.z) > EoDxf::geometricTolerance) { WriteCodeDouble(30, arc->m_centerPoint.z); }
  WriteCodeDouble(40, arc->m_radius);
  WriteCodeString(100, L"AcDbArc");
  WriteCodeDouble(50, arc->m_startAngle * EoDxf::RadiansToDegrees);
  WriteCodeDouble(51, arc->m_endAngle * EoDxf::RadiansToDegrees);
  return m_writeOk;
}

bool EoDxfWrite::WriteCircle(EoDxfCircle* circle) {
  WriteCodeString(0, L"CIRCLE");
  WriteEntity(circle);
  WriteCodeString(100, L"AcDbCircle");
  WriteCodeDouble(10, circle->m_centerPoint.x);
  WriteCodeDouble(20, circle->m_centerPoint.y);
  if (std::abs(circle->m_centerPoint.z) > EoDxf::geometricTolerance) { WriteCodeDouble(30, circle->m_centerPoint.z); }
  WriteCodeDouble(40, circle->m_radius);
  return m_writeOk;
}

bool EoDxfWrite::WriteEllipse(EoDxfEllipse* ellipse) {
  // verify axis/ratio and params for full ellipse
  ellipse->CorrectAxis();

  WriteCodeString(0, L"ELLIPSE");
  WriteEntity(ellipse);
  WriteCodeString(100, L"AcDbEllipse");
  WriteCodeDouble(10, ellipse->m_startPoint.x);
  WriteCodeDouble(20, ellipse->m_startPoint.y);
  WriteCodeDouble(30, ellipse->m_startPoint.z);
  WriteCodeDouble(11, ellipse->m_endPoint.x);
  WriteCodeDouble(21, ellipse->m_endPoint.y);
  WriteCodeDouble(31, ellipse->m_endPoint.z);
  WriteCodeDouble(40, ellipse->m_ratio);
  WriteCodeDouble(41, ellipse->m_startParam);
  WriteCodeDouble(42, ellipse->m_endParam);

  return m_writeOk;
}

bool EoDxfWrite::WriteHatch(EoDxfHatch* hatch) {
  WriteCodeString(0, L"HATCH");
  WriteEntity(hatch);
  WriteCodeString(100, L"AcDbHatch");
  WriteCodeDouble(10, hatch->m_elevationPoint.x);
  WriteCodeDouble(20, hatch->m_elevationPoint.y);
  WriteCodeDouble(30, hatch->m_elevationPoint.z);
  WriteCodeDouble(210, hatch->m_extrusionDirection.x);
  WriteCodeDouble(220, hatch->m_extrusionDirection.y);
  WriteCodeDouble(230, hatch->m_extrusionDirection.z);
  WriteCodeWideString(2, hatch->m_hatchPatternName);
  WriteCodeInt16(70, hatch->m_solidFillFlag);
  WriteCodeInt16(71, hatch->m_associativityFlag);
  hatch->m_numberOfBoundaryPaths = static_cast<int>(hatch->m_hatchLoops.size());
  WriteCodeInt32(91, hatch->m_numberOfBoundaryPaths);
  // write paths data
  for (int i = 0; i < hatch->m_numberOfBoundaryPaths; i++) {
    auto* hatchLoop = hatch->m_hatchLoops.at(i);
      WriteCodeInt32(92, hatchLoop->m_boundaryPathType);
    if ((hatchLoop->m_boundaryPathType & 2) == 2) {
      // polyline boundary path
      if (!hatchLoop->m_entities.empty() && hatchLoop->m_entities.front()->m_entityType == EoDxf::LWPOLYLINE) {
        auto* polyline = static_cast<EoDxfLwPolyline*>(hatchLoop->m_entities.front().get());
        WriteCodeInt16(72, (polyline->m_constantWidth != 0.0) ? 1 : 0);
        WriteCodeInt16(73, polyline->m_polylineFlag);
        WriteCodeInt32(93, static_cast<std::int32_t>(polyline->m_vertices.size()));
        for (const auto& v : polyline->m_vertices) {
          WriteCodeDouble(10, v.x);
          WriteCodeDouble(20, v.y);
          if (v.bulge != 0.0) { WriteCodeDouble(42, v.bulge); }
        }
      }
      WriteCodeInt32(97, 0);
    } else {
      hatchLoop->Update();
      WriteCodeInt32(93, hatchLoop->m_numberOfEdges);
      for (int j = 0; j < hatchLoop->m_numberOfEdges; ++j) {
        switch ((hatchLoop->m_entities.at(j))->m_entityType) {
          case EoDxf::LINE: {
            WriteCodeInt16(72, 1);
            auto* line = static_cast<EoDxfLine*>(hatchLoop->m_entities.at(j).get());
            WriteCodeDouble(10, line->m_startPoint.x);
            WriteCodeDouble(20, line->m_startPoint.y);
            WriteCodeDouble(11, line->m_endPoint.x);
            WriteCodeDouble(21, line->m_endPoint.y);
            break;
          }
          case EoDxf::ARC: {
            WriteCodeInt16(72, 2);
            auto* arc = static_cast<EoDxfArc*>(hatchLoop->m_entities.at(j).get());
            WriteCodeDouble(10, arc->m_centerPoint.x);
            WriteCodeDouble(20, arc->m_centerPoint.y);
            WriteCodeDouble(40, arc->m_radius);
            WriteCodeDouble(50, arc->m_startAngle * EoDxf::RadiansToDegrees);
            WriteCodeDouble(51, arc->m_endAngle * EoDxf::RadiansToDegrees);
            WriteCodeInt16(73, arc->m_isCounterClockwise);
            break;
          }
          case EoDxf::ELLIPSE: {
            WriteCodeInt16(72, 3);
            auto* ellipse = static_cast<EoDxfEllipse*>(hatchLoop->m_entities.at(j).get());
            ellipse->CorrectAxis();
            WriteCodeDouble(10, ellipse->m_startPoint.x);
            WriteCodeDouble(20, ellipse->m_startPoint.y);
            WriteCodeDouble(11, ellipse->m_endPoint.x);
            WriteCodeDouble(21, ellipse->m_endPoint.y);
            WriteCodeDouble(40, ellipse->m_ratio);
            WriteCodeDouble(50, ellipse->m_startParam);
            WriteCodeDouble(51, ellipse->m_endParam);
            WriteCodeInt16(73, ellipse->m_isCounterClockwise);
            break;
          }
          case EoDxf::SPLINE:
            break;
          default:
            break;
        }
      }
      WriteCodeInt32(97, 0);
    }
  }
  WriteCodeInt16(75, hatch->m_hatchStyle);
  WriteCodeInt16(76, hatch->m_hatchPatternType);
  if (!hatch->m_solidFillFlag) {
    WriteCodeDouble(52, hatch->m_hatchPatternAngle);
    WriteCodeDouble(41, hatch->m_hatchPatternScaleOrSpacing);
    WriteCodeInt16(77, hatch->m_hatchPatternDoubleFlag);
    WriteCodeInt16(78, hatch->m_numberOfPatternDefinitionLines);
  }
  WriteCodeInt32(98, 0);

  return m_writeOk;
}

bool EoDxfWrite::WriteLine(EoDxfLine* line) {
  WriteCodeString(0, L"LINE");
  WriteEntity(line);
  WriteCodeString(100, L"AcDbLine");
  WriteCodeDouble(10, line->m_startPoint.x);
  WriteCodeDouble(20, line->m_startPoint.y);
  if (line->m_startPoint.z != 0.0 || line->m_endPoint.z != 0.0) {
    WriteCodeDouble(30, line->m_startPoint.z);
    WriteCodeDouble(11, line->m_endPoint.x);
    WriteCodeDouble(21, line->m_endPoint.y);
    WriteCodeDouble(31, line->m_endPoint.z);
  } else {
    WriteCodeDouble(11, line->m_endPoint.x);
    WriteCodeDouble(21, line->m_endPoint.y);
  }
  return m_writeOk;
}

bool EoDxfWrite::WritePoint(EoDxfPoint* point) {
  WriteCodeString(0, L"POINT");
  WriteEntity(point);
  WriteCodeString(100, L"AcDbPoint");
  WriteCodeDouble(10, point->m_firstPoint.x);
  WriteCodeDouble(20, point->m_firstPoint.y);
  if (point->m_firstPoint.z != 0.0) { WriteCodeDouble(30, point->m_firstPoint.z); }
  return m_writeOk;
}

bool EoDxfWrite::WriteRay(EoDxfRay* ray) {
  WriteCodeString(0, L"RAY");
  WriteEntity(ray);
  WriteCodeString(100, L"AcDbRay");
  WriteCodeDouble(10, ray->m_startPoint.x);
  WriteCodeDouble(20, ray->m_startPoint.y);
  if (std::fabs(ray->m_startPoint.z) > EoDxf::geometricTolerance) { WriteCodeDouble(30, ray->m_startPoint.z); }
  auto direction = ray->m_endPoint;
  direction.Unitize();
  WriteCodeDouble(11, direction.x);
  WriteCodeDouble(21, direction.y);
  if (std::fabs(direction.z) > EoDxf::geometricTolerance) { WriteCodeDouble(31, direction.z); }
  return m_writeOk;
}

bool EoDxfWrite::WriteSolid(EoDxfSolid* solid) {
  WriteCodeString(0, L"SOLID");
  WriteEntity(solid);
  WriteCodeString(100, L"AcDbTrace");  // SOLID shares the same subclass as TRACE
  WriteCodeDouble(10, solid->m_startPoint.x);
  WriteCodeDouble(20, solid->m_startPoint.y);
  WriteCodeDouble(30, solid->m_startPoint.z);
  WriteCodeDouble(11, solid->m_endPoint.x);
  WriteCodeDouble(21, solid->m_endPoint.y);
  WriteCodeDouble(31, solid->m_endPoint.z);
  WriteCodeDouble(12, solid->m_thirdPoint.x);
  WriteCodeDouble(22, solid->m_thirdPoint.y);
  WriteCodeDouble(32, solid->m_thirdPoint.z);
  WriteCodeDouble(13, solid->m_fourthPoint.x);
  WriteCodeDouble(23, solid->m_fourthPoint.y);
  WriteCodeDouble(33, solid->m_fourthPoint.z);
  return m_writeOk;
}

bool EoDxfWrite::WriteTrace(EoDxfTrace* trace) {
  WriteCodeString(0, L"TRACE");
  WriteEntity(trace);
  WriteCodeString(100, L"AcDbTrace");
  WriteCodeDouble(10, trace->m_startPoint.x);
  WriteCodeDouble(20, trace->m_startPoint.y);
  WriteCodeDouble(30, trace->m_startPoint.z);
  WriteCodeDouble(11, trace->m_endPoint.x);
  WriteCodeDouble(21, trace->m_endPoint.y);
  WriteCodeDouble(31, trace->m_endPoint.z);
  WriteCodeDouble(12, trace->m_thirdPoint.x);
  WriteCodeDouble(22, trace->m_thirdPoint.y);
  WriteCodeDouble(32, trace->m_thirdPoint.z);
  WriteCodeDouble(13, trace->m_fourthPoint.x);
  WriteCodeDouble(23, trace->m_fourthPoint.y);
  WriteCodeDouble(33, trace->m_fourthPoint.z);
  return m_writeOk;
}

bool EoDxfWrite::WriteXline(EoDxfXline* xline) {
  WriteCodeString(0, L"XLINE");
  WriteEntity(xline);
  WriteCodeString(100, L"AcDbXline");
  WriteCodeDouble(10, xline->m_startPoint.x);
  WriteCodeDouble(20, xline->m_startPoint.y);
  if (std::fabs(xline->m_startPoint.z) > EoDxf::geometricTolerance) {
    WriteCodeDouble(30, xline->m_startPoint.z);
  }
  auto direction = xline->m_endPoint;
  direction.Unitize();
  WriteCodeDouble(11, direction.x);
  WriteCodeDouble(21, direction.y);
  if (std::fabs(direction.z) > EoDxf::geometricTolerance) { WriteCodeDouble(31, direction.z); }
  return m_writeOk;
}

bool EoDxfWrite::WriteLWPolyline(EoDxfLwPolyline* polyline) {
  WriteCodeString(0, L"LWPOLYLINE");
  WriteEntity(polyline);
  WriteCodeString(100, L"AcDbPolyline");

  // Modern container – no more raw pointers
  polyline->m_numberOfVertices = static_cast<int>(polyline->m_vertices.size());
  WriteCodeInt32(90, polyline->m_numberOfVertices);
  WriteCodeInt16(70, polyline->m_polylineFlag);
  WriteCodeDouble(43, polyline->m_constantWidth);
  if (polyline->m_elevation != 0.0) { WriteCodeDouble(38, polyline->m_elevation); }
  if (polyline->m_thickness != 0.0) { WriteCodeDouble(39, polyline->m_thickness); }
  for (const auto& vertex : polyline->m_vertices) {
    WriteCodeDouble(10, vertex.x);
    WriteCodeDouble(20, vertex.y);
    if (vertex.stawidth != 0.0) { WriteCodeDouble(40, vertex.stawidth); }
    if (vertex.endwidth != 0.0) { WriteCodeDouble(41, vertex.endwidth); }
    if (vertex.bulge != 0.0) { WriteCodeDouble(42, vertex.bulge); }
  }

  return m_writeOk;
}

bool EoDxfWrite::WritePolyline(EoDxfPolyline* polyline) {
  WriteCodeString(0, L"POLYLINE");
  WriteEntity(polyline);

  const bool is3dPolyline = (polyline->m_polylineFlag & (8 | 16 | 64)) != 0;
  WriteCodeString(100, is3dPolyline ? L"AcDb3dPolyline" : L"AcDb2dPolyline");

  // Group code 66, `entities follow flag` is optional in AC1015, ignored in AC1018 and later)
  if (m_version == EoDxf::Version::AC1014) { WriteCodeInt16(66, 1); }

  WriteCodeDouble(10, 0.0);
  WriteCodeDouble(20, 0.0);
  WriteCodeDouble(30, polyline->m_firstPoint.z);
  if (polyline->m_thickness != 0) { WriteCodeDouble(39, polyline->m_thickness); }
  WriteCodeInt16(70, polyline->m_polylineFlag);
  if (polyline->m_defaultStartWidth != 0) { WriteCodeDouble(40, polyline->m_defaultStartWidth); }
  if (polyline->m_defaultEndWidth != 0) { WriteCodeDouble(41, polyline->m_defaultEndWidth); }
  if ((polyline->m_polylineFlag & 16) || (polyline->m_polylineFlag & 32)) {
    WriteCodeInt16(71, polyline->m_polygonMeshVertexCountM);
    WriteCodeInt16(72, polyline->m_polygonMeshVertexCountN);
  }
  if (polyline->m_smoothSurfaceDensityM != 0) { WriteCodeInt16(73, polyline->m_smoothSurfaceDensityM); }
  if (polyline->m_smoothSurfaceDensityN != 0) { WriteCodeInt16(74, polyline->m_smoothSurfaceDensityN); }
  if (polyline->m_curvesAndSmoothSurfaceType != 0) { WriteCodeInt16(75, polyline->m_curvesAndSmoothSurfaceType); }
  auto extrusionDirection = polyline->m_extrusionDirection;
  if (extrusionDirection.x > EoDxf::geometricTolerance || extrusionDirection.y < -EoDxf::geometricTolerance ||
      std::abs(extrusionDirection.z - 1.0) > EoDxf::geometricTolerance) {
    WriteCodeDouble(210, extrusionDirection.x);
    WriteCodeDouble(220, extrusionDirection.y);
    WriteCodeDouble(230, extrusionDirection.z);
  }

  // VERTEX entities
  const auto polylineHandle = polyline->m_handle;
  for (auto* vertex : polyline->m_vertices) {
    WriteCodeString(0, L"VERTEX");
    // WriteEntity assigns a new handle and writes AcDbEntity subclass
    WriteEntity(vertex);

    // Base subclass marker required before the specific subclass
    if (vertex->m_vertexFlags & 128) {
      WriteCodeString(100, L"AcDbPolyFaceMeshVertex");
    } else if (vertex->m_vertexFlags & 64) {
      WriteCodeString(100, L"AcDbPolygonMeshVertex");
    } else if (vertex->m_vertexFlags & 32) {
      WriteCodeString(100, L"AcDb3dPolylineVertex");
    } else {
      WriteCodeString(100, L"AcDb2dVertex");
    }

    if ((vertex->m_vertexFlags & 128) != 0 && (vertex->m_vertexFlags & 64) == 0) {
      // Polyface face records (flag 128 without flag 64): dummy coordinates
      WriteCodeDouble(10, 0.0);
      WriteCodeDouble(20, 0.0);
      WriteCodeDouble(30, 0.0);
    } else {
      WriteCodeDouble(10, vertex->m_locationPoint.x);
      WriteCodeDouble(20, vertex->m_locationPoint.y);
      WriteCodeDouble(30, vertex->m_locationPoint.z);
    }
    if (vertex->m_startingWidth != 0) { WriteCodeDouble(40, vertex->m_startingWidth); }
    if (vertex->m_endingWidth != 0) { WriteCodeDouble(41, vertex->m_endingWidth); }
    if (vertex->m_bulge != 0.0) { WriteCodeDouble(42, vertex->m_bulge); }
    if (vertex->m_vertexFlags != 0) { WriteCodeInt16(70, vertex->m_vertexFlags); }
    if ((vertex->m_vertexFlags & 2) != 0) { WriteCodeDouble(50, vertex->m_curveFitTangentDirection); }

    if ((vertex->m_vertexFlags & 128) != 0) {
      if (vertex->m_polyfaceMeshVertexIndex1 != 0) { WriteCodeInt16(71, vertex->m_polyfaceMeshVertexIndex1); }
      if (vertex->m_polyfaceMeshVertexIndex2 != 0) { WriteCodeInt16(72, vertex->m_polyfaceMeshVertexIndex2); }
      if (vertex->m_polyfaceMeshVertexIndex3 != 0) { WriteCodeInt16(73, vertex->m_polyfaceMeshVertexIndex3); }
      if (vertex->m_polyfaceMeshVertexIndex4 != 0) { WriteCodeInt16(74, vertex->m_polyfaceMeshVertexIndex4); }

      if ((vertex->m_vertexFlags & 64) == 0 && vertex->m_identifier != 0) {
        WriteCodeInt32(91, vertex->m_identifier);
      }
    }
  }

  // SEQEND entity
  WriteCodeString(0, L"SEQEND");

  EoDxfSeqEnd seqEnd{*polyline};
  seqEnd.m_handle = ++m_entityCount;
  WriteCodeString(5, ToHexString(seqEnd.m_handle));
  WriteCodeString(330, ToHexString(polylineHandle));
  WriteCodeString(100, L"AcDbEntity");
  WriteCodeWideString(8, seqEnd.m_layer);

  return m_writeOk;
}

bool EoDxfWrite::WriteSpline(EoDxfSpline* spline) {
  WriteCodeString(0, L"SPLINE");
  WriteEntity(spline);
  WriteCodeString(100, L"AcDbSpline");

  WriteCodeDouble(210, spline->m_normalVector.x);
  WriteCodeDouble(220, spline->m_normalVector.y);
  WriteCodeDouble(230, spline->m_normalVector.z);

  WriteCodeInt16(70, spline->m_splineFlag);
  WriteCodeInt16(71, spline->m_degreeOfTheSplineCurve);

  WriteCodeInt16(72, spline->m_numberOfKnots);
  WriteCodeInt16(73, spline->m_numberOfControlPoints);
  WriteCodeInt16(74, spline->m_numberOfFitPoints);

  WriteCodeDouble(42, spline->m_knotTolerance);
  WriteCodeDouble(43, spline->m_controlPointTolerance);
  WriteCodeDouble(44, spline->m_fitTolerance);

  // Start tangent (12, 22, 32) — write when spline flag bit 1 is set (tangent defined)
  if ((spline->m_splineFlag & 1) != 0) {
    WriteCodeDouble(12, spline->m_startTangent.x);
    WriteCodeDouble(22, spline->m_startTangent.y);
    WriteCodeDouble(32, spline->m_startTangent.z);
  }

  // End tangent (13, 23, 33)
  if ((spline->m_splineFlag & 1) != 0) {
    WriteCodeDouble(13, spline->m_endTangent.x);
    WriteCodeDouble(23, spline->m_endTangent.y);
    WriteCodeDouble(33, spline->m_endTangent.z);
  }

  for (int i = 0; i < spline->m_numberOfKnots; i++) { WriteCodeDouble(40, spline->m_knotValues.at(i)); }

  for (int i = 0; i < spline->m_numberOfControlPoints; i++) {
    auto* controlPoint = spline->m_controlPoints.at(i);
    WriteCodeDouble(10, controlPoint->x);
    WriteCodeDouble(20, controlPoint->y);
    WriteCodeDouble(30, controlPoint->z);
  }

  // Fit points (11, 21, 31)
  for (std::int16_t i = 0; i < spline->m_numberOfFitPoints; ++i) {
    const auto* fitPoint = spline->m_fitPoints[static_cast<size_t>(i)];
    WriteCodeDouble(11, fitPoint->x);
    WriteCodeDouble(21, fitPoint->y);
    WriteCodeDouble(31, fitPoint->z);
  }
  return m_writeOk;
}
