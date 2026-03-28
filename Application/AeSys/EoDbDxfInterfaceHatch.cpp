#include "Stdafx.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbDxfInterface.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoDxfEntities.h"
#include "EoGeOcsTransform.h"
#include "EoGePoint3d.h"
#include "EoGePolyline.h"
#include "EoGeVector3d.h"

/** @brief Maps a DXF hatch pattern name to the AeSys fill style index.
 *
 * Pattern names are matched case-insensitively against the built-in set loaded
 * from DefaultSet.txt.  Returns 0 (no match) when the name is unrecognized,
 * which causes DisplayFilAreaHatch to early-return harmlessly.
 *
 * @param patternName  The DXF pattern name (group code 2).
 * @return The 1-based fill style index, or 0 if no match.
 */
static std::int16_t MapHatchPatternNameToIndex(const std::wstring& patternName) {
  // Table mirrors DefaultSet.txt load order (1-based indices 1–39)
  static const struct {
    const wchar_t* name;
    std::int16_t index;
  } patternTable[] = {{L"PEG1", 1}, {L"PEG2", 2}, {L"ANGLE", 3}, {L"ANSI31", 4}, {L"ANSI32", 5}, {L"ANSI33", 6},
      {L"ANSI34", 7}, {L"ANSI35", 8}, {L"ANSI36", 9}, {L"ANSI37", 10}, {L"ANSI38", 11}, {L"BOX", 12}, {L"BRICK", 13},
      {L"CLAY", 14}, {L"CORK", 15}, {L"CROSS", 16}, {L"DASH", 17}, {L"DOLMIT", 18}, {L"DOTS", 19}, {L"EARTH", 20},
      {L"ESCHER", 21}, {L"FLEX", 22}, {L"GRASS", 23}, {L"GRATE", 24}, {L"HEX", 25}, {L"HONEY", 26}, {L"HOUND", 27},
      {L"INSUL", 28}, {L"MUDST", 29}, {L"NET3", 30}, {L"PLAST", 31}, {L"PLASTI", 32}, {L"SACNCR", 33}, {L"SQUARE", 34},
      {L"STARS", 35}, {L"SWAMP", 36}, {L"TRANS", 37}, {L"TRIAN", 38}, {L"ZIGZAG", 39}, {L"AR-CONC", 40},
      {L"AR-SAND", 41}};

  for (const auto& entry : patternTable) {
    if (_wcsicmp(patternName.c_str(), entry.name) == 0) { return entry.index; }
  }
  return 0;  // Unrecognized pattern — caller falls back to Hollow
}

/** @brief Converts a DXF HATCH entity to one or more EoDbPolygon primitives.
 *
 * Each hatch boundary loop becomes a separate EoDbPolygon. Polyline boundaries
 * are tessellated (bulge arcs expanded); edge-type boundaries chain line, arc,
 * and ellipse edges into a closed vertex array. Spline edges are logged and skipped.
 *
 * Solid-fill hatches map to PolygonStyle::Solid. Pattern hatches map to
 * PolygonStyle::Hatch with reference vectors derived from the DXF pattern angle
 * and scale. Hollow hatches (solidFillFlag == 0 with pattern name "SOLID" absent)
 * map to PolygonStyle::Hollow.
 *
 * Boundary points are in OCS; when the extrusion normal differs from positive Z,
 * the OCS-to-WCS transform is applied to all points and reference vectors.
 *
 * Island loops (neither external nor outermost) are rendered as Hollow polygons
 * when the outer hatch is solid-filled, creating the visual "hole" effect.
 *
 * @param hatch  The parsed DXF HATCH entity.
 * @param document  The AeSysDoc receiving the converted primitives.
 */
void EoDbDxfInterface::ConvertHatchEntity(const EoDxfHatch& hatch, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Hatch entity conversion - Pattern: %s, Loops: %d, Solid: %d\n",
      hatch.m_hatchPatternName.c_str(), static_cast<int>(hatch.HatchLoops().size()), hatch.m_solidFillFlag);

  if (hatch.HatchLoops().empty()) { return; }

  // Build OCS→WCS transform from the entity's extrusion direction
  const EoGeVector3d extrusionNormal{
      hatch.m_extrusionDirection.x, hatch.m_extrusionDirection.y, hatch.m_extrusionDirection.z};
  const EoGeOcsTransform ocsToWcs = EoGeOcsTransform::CreateOcsToWcs(extrusionNormal);
  const bool needsOcsTransform = !ocsToWcs.IsWorldCoordinateSystem();

  // Determine polygon style from DXF hatch properties
  EoDb::PolygonStyle polygonStyle = EoDb::PolygonStyle::Hollow;
  std::int16_t fillStyleIndex = 0;

  if (hatch.m_solidFillFlag == 1) {
    polygonStyle = EoDb::PolygonStyle::Solid;
  } else if (hatch.m_hatchPatternName == L"HOLLOW" || hatch.m_hatchPatternName.empty()) {
    polygonStyle = EoDb::PolygonStyle::Hollow;
  } else {
    // Pattern hatch — map DXF name to AeSys fill style index
    polygonStyle = EoDb::PolygonStyle::Hatch;
    fillStyleIndex = MapHatchPatternNameToIndex(hatch.m_hatchPatternName);
    if (fillStyleIndex == 0) {
      ATLTRACE2(traceGeneral, 1, L"  Unrecognized hatch pattern name \"%s\" — falling back to Hollow\n",
          hatch.m_hatchPatternName.c_str());
      polygonStyle = EoDb::PolygonStyle::Hollow;
    }
  }

  if (hatch.m_hatchPatternDoubleFlag != 0) {
    ATLTRACE2(traceGeneral, 1, L"  Hatch pattern double flag set — second 90° pass not implemented in PEG V1\n");
  }

  // Hatch origin from the elevation point (OCS origin for the hatch plane)
  EoGePoint3d hatchOrigin{hatch.m_elevationPoint.x, hatch.m_elevationPoint.y, hatch.m_elevationPoint.z};

  // Build reference vectors from pattern angle and scale
  // For solid fills, use identity-like reference vectors (unit X and Y)
  EoGeVector3d xAxis = EoGeVector3d::positiveUnitX;
  EoGeVector3d yAxis = EoGeVector3d::positiveUnitY;

  if (polygonStyle == EoDb::PolygonStyle::Hatch && hatch.m_hatchPatternScaleOrSpacing > Eo::geometricTolerance) {
    const double scale = hatch.m_hatchPatternScaleOrSpacing;
    const double angle = hatch.m_hatchPatternAngle * Eo::Radian;  // DXF pattern angle is in degrees
    const double cosAngle = std::cos(angle);
    const double sinAngle = std::sin(angle);
    xAxis = EoGeVector3d{cosAngle * scale, sinAngle * scale, 0.0};
    yAxis = EoGeVector3d{-sinAngle * scale, cosAngle * scale, 0.0};
  }

  // Transform hatch origin from OCS to WCS when extrusion is non-default.
  // Pattern reference vectors (xAxis, yAxis) are NOT transformed — they define
  // the visual pattern orientation in the rendering plane. Transforming them
  // through OCS would mirror the pattern for negative-Z extrusion because the
  // OCS X-axis reverses direction (e.g., [0,0,-1] → OCS X = [-1,0,0]).
  // AutoCAD renders hatch patterns with the same visual orientation regardless
  // of extrusion direction; preserving untransformed vectors matches this behavior.
  if (needsOcsTransform) { hatchOrigin = ocsToWcs * hatchOrigin; }

  int loopIndex = 0;
  for (const auto* hatchLoop : hatch.HatchLoops()) {
    ++loopIndex;

    if (hatchLoop->m_entities.empty()) {
      ATLTRACE2(traceGeneral, 1, L"  Loop %d: empty entity list, skipping\n", loopIndex);
      continue;
    }

    // Determine per-loop polygon style: island boundaries become Hollow when the
    // outer hatch is solid-filled, creating the visual "hole" effect.
    const bool isIslandLoop =
        (hatchLoop->m_boundaryPathType & 0x01) == 0 && (hatchLoop->m_boundaryPathType & 0x10) == 0;
    EoDb::PolygonStyle loopPolygonStyle = polygonStyle;
    if (isIslandLoop && polygonStyle == EoDb::PolygonStyle::Solid) {
      loopPolygonStyle = EoDb::PolygonStyle::Hollow;
      ATLTRACE2(traceGeneral, 2, L"  Loop %d: island boundary (type 0x%X) — rendered as Hollow\n", loopIndex,
          hatchLoop->m_boundaryPathType);
    } else if (isIslandLoop) {
      ATLTRACE2(traceGeneral, 2, L"  Loop %d: island boundary (type 0x%X) — converted as independent polygon\n",
          loopIndex, hatchLoop->m_boundaryPathType);
    }

    EoGePoint3dArray boundaryPoints;

    if (hatchLoop->m_boundaryPathType & 2) {
      // ── Polyline boundary ────────────────────────────────
      const auto* polylineEntity = (hatchLoop->m_entities.front()->m_entityType == EoDxf::LWPOLYLINE)
          ? static_cast<const EoDxfLwPolyline*>(hatchLoop->m_entities.front().get())
          : nullptr;
      if (polylineEntity == nullptr || polylineEntity->m_vertices.empty()) {
        ATLTRACE2(traceGeneral, 1, L"  Loop %d: polyline boundary with no vertices, skipping\n", loopIndex);
        continue;
      }

      const auto& vertices = polylineEntity->m_vertices;
      const auto vertexCount = vertices.size();
      const double elevation = hatch.m_elevationPoint.z;

      // Check if any vertex has a non-zero bulge
      const bool hasAnyBulge = std::any_of(vertices.begin(), vertices.end(),
          [](const EoDxfPolylineVertex2d& vertex) noexcept { return Eo::IsGeometricallyNonZero(vertex.bulge); });

      if (hasAnyBulge) {
        // Tessellate bulge arcs into straight segments
        std::vector<EoGePoint3d> arcPoints;
        boundaryPoints.Add(EoGePoint3d{vertices[0].x, vertices[0].y, elevation});

        for (size_t i = 0; i < vertexCount - 1; ++i) {
          const EoGePoint3d startPt{vertices[i].x, vertices[i].y, elevation};
          const EoGePoint3d endPt{vertices[i + 1].x, vertices[i + 1].y, elevation};
          polyline::TessellateArcSegment(startPt, endPt, vertices[i].bulge, arcPoints);
          for (const auto& arcPoint : arcPoints) { boundaryPoints.Add(arcPoint); }
        }

        // Handle closing segment if polyline is closed
        if (polylineEntity->m_polylineFlag & 0x01) {
          const EoGePoint3d startPt{vertices[vertexCount - 1].x, vertices[vertexCount - 1].y, elevation};
          const EoGePoint3d endPt{vertices[0].x, vertices[0].y, elevation};
          const double closingBulge = vertices[vertexCount - 1].bulge;
          if (Eo::IsGeometricallyNonZero(closingBulge)) {
            polyline::TessellateArcSegment(startPt, endPt, closingBulge, arcPoints);
            // Exclude last point (it duplicates the first vertex)
            for (size_t j = 0; j + 1 < arcPoints.size(); ++j) { boundaryPoints.Add(arcPoints[j]); }
          }
        }
      } else {
        // No bulges — straight segments only
        for (size_t i = 0; i < vertexCount; ++i) {
          boundaryPoints.Add(EoGePoint3d{vertices[i].x, vertices[i].y, elevation});
        }
      }

      ATLTRACE2(traceGeneral, 2, L"  Loop %d: polyline boundary → %d tessellated vertices\n", loopIndex,
          static_cast<int>(boundaryPoints.GetSize()));
    } else {
      // ── Edge-type boundary ───────────────────────────────
      const double elevation = hatch.m_elevationPoint.z;

      for (const auto& edgeEntity : hatchLoop->m_entities) {
        switch (edgeEntity->m_entityType) {
          case EoDxf::LINE: {
            const auto* line = static_cast<const EoDxfLine*>(edgeEntity.get());
            // Add start point of first edge; subsequent edges share endpoints
            if (boundaryPoints.IsEmpty()) {
              boundaryPoints.Add(EoGePoint3d{line->m_startPoint.x, line->m_startPoint.y, elevation});
            }
            boundaryPoints.Add(EoGePoint3d{line->m_endPoint.x, line->m_endPoint.y, elevation});
            break;
          }
          case EoDxf::ARC: {
            const auto* arc = static_cast<const EoDxfArc*>(edgeEntity.get());
            const double centerX = arc->m_centerPoint.x;
            const double centerY = arc->m_centerPoint.y;
            const double radius = arc->m_radius;
            double startAngle = arc->m_startAngle;
            double endAngle = arc->m_endAngle;

            // Determine sweep direction from CCW flag
            double sweepAngle;
            if (arc->m_isCounterClockwise) {
              sweepAngle = endAngle - startAngle;
              if (sweepAngle <= 0.0) { sweepAngle += Eo::TwoPi; }
            } else {
              sweepAngle = endAngle - startAngle;
              if (sweepAngle >= 0.0) { sweepAngle -= Eo::TwoPi; }
            }

            // Adaptive tessellation
            const int numberOfSegments = std::max(Eo::arcTessellationMinimumSegments,
                static_cast<int>(
                    std::ceil(std::abs(sweepAngle) / Eo::TwoPi * Eo::arcTessellationSegmentsPerFullCircle)));

            // Add start point if this is the first edge
            if (boundaryPoints.IsEmpty()) {
              boundaryPoints.Add(EoGePoint3d{
                  centerX + radius * std::cos(startAngle), centerY + radius * std::sin(startAngle), elevation});
            }

            const double angleStep = sweepAngle / numberOfSegments;
            for (int seg = 1; seg <= numberOfSegments; ++seg) {
              const double angle = startAngle + angleStep * seg;
              boundaryPoints.Add(
                  EoGePoint3d{centerX + radius * std::cos(angle), centerY + radius * std::sin(angle), elevation});
            }

            ATLTRACE2(traceGeneral, 3, L"    Arc edge: center=(%.4f,%.4f) r=%.4f → %d segments\n", centerX, centerY,
                radius, numberOfSegments);
            break;
          }
          case EoDxf::ELLIPSE: {
            const auto* ellipse = static_cast<const EoDxfEllipse*>(edgeEntity.get());
            const double cx = ellipse->m_centerPoint.x;
            const double cy = ellipse->m_centerPoint.y;
            const double majorX = ellipse->m_endPointOfMajorAxis.x;
            const double majorY = ellipse->m_endPointOfMajorAxis.y;
            const double ratio = ellipse->m_ratio;
            double startParam = ellipse->m_startParam;
            double endParam = ellipse->m_endParam;

            // Minor axis perpendicular to major axis in 2D
            const double minorX = -majorY * ratio;
            const double minorY = majorX * ratio;

            // Determine sweep
            double sweepParam;
            if (ellipse->m_isCounterClockwise) {
              sweepParam = endParam - startParam;
              if (sweepParam <= 0.0) { sweepParam += Eo::TwoPi; }
            } else {
              sweepParam = endParam - startParam;
              if (sweepParam >= 0.0) { sweepParam -= Eo::TwoPi; }
            }

            const int numberOfSegments = std::max(Eo::arcTessellationMinimumSegments,
                static_cast<int>(
                    std::ceil(std::abs(sweepParam) / Eo::TwoPi * Eo::arcTessellationSegmentsPerFullCircle)));

            if (boundaryPoints.IsEmpty()) {
              const double cosStart = std::cos(startParam);
              const double sinStart = std::sin(startParam);
              boundaryPoints.Add(EoGePoint3d{
                  cx + majorX * cosStart + minorX * sinStart, cy + majorY * cosStart + minorY * sinStart, elevation});
            }

            const double paramStep = sweepParam / numberOfSegments;
            for (int seg = 1; seg <= numberOfSegments; ++seg) {
              const double param = startParam + paramStep * seg;
              const double cosParam = std::cos(param);
              const double sinParam = std::sin(param);
              boundaryPoints.Add(EoGePoint3d{
                  cx + majorX * cosParam + minorX * sinParam, cy + majorY * cosParam + minorY * sinParam, elevation});
            }

            ATLTRACE2(traceGeneral, 3, L"    Ellipse edge: center=(%.4f,%.4f) ratio=%.4f → %d segments\n", cx, cy,
                ratio, numberOfSegments);
            break;
          }
          case EoDxf::SPLINE:
            ATLTRACE2(traceGeneral, 1, L"    Spline edge in hatch boundary — skipped (not supported in PEG V1)\n");
            break;
          default:
            ATLTRACE2(traceGeneral, 1, L"    Unknown edge type %d in hatch boundary — skipped\n",
                static_cast<int>(edgeEntity->m_entityType));
            break;
        }
      }

      ATLTRACE2(traceGeneral, 2, L"  Loop %d: edge boundary → %d tessellated vertices\n", loopIndex,
          static_cast<int>(boundaryPoints.GetSize()));
    }

    // Need at least 3 points for a valid polygon
    if (boundaryPoints.GetSize() < 3) {
      ATLTRACE2(traceGeneral, 1, L"  Loop %d: insufficient vertices (%d), skipping\n", loopIndex,
          static_cast<int>(boundaryPoints.GetSize()));
      continue;
    }

    // Remove duplicate closing vertex if present (EoDbPolygon is implicitly closed)
    const auto lastIndex = boundaryPoints.GetSize() - 1;
    const auto& firstPt = boundaryPoints[0];
    const auto& lastPt = boundaryPoints[lastIndex];
    if (Eo::IsGeometricallyZero(firstPt.x - lastPt.x) && Eo::IsGeometricallyZero(firstPt.y - lastPt.y) &&
        Eo::IsGeometricallyZero(firstPt.z - lastPt.z)) {
      boundaryPoints.SetSize(lastIndex);
    }

    if (boundaryPoints.GetSize() < 3) {
      ATLTRACE2(traceGeneral, 1, L"  Loop %d: degenerate after dedup (%d vertices), skipping\n", loopIndex,
          static_cast<int>(boundaryPoints.GetSize()));
      continue;
    }

    // Transform boundary points from OCS to WCS when extrusion is non-default
    if (needsOcsTransform) {
      for (INT_PTR i = 0; i < boundaryPoints.GetSize(); ++i) { boundaryPoints[i] = ocsToWcs * boundaryPoints[i]; }
    }

    auto* polygon = new EoDbPolygon(static_cast<std::int16_t>(hatch.m_color), loopPolygonStyle, fillStyleIndex,
        hatchOrigin, xAxis, yAxis, boundaryPoints);

    // Passthrough: preserve DXF pattern definition lines and double flag for round-trip export
    polygon->SetHatchPatternDoubleFlag(hatch.m_hatchPatternDoubleFlag);
    polygon->SetPatternDefinitionLines(hatch.m_patternDefinitionLines);

    // Set layer and line type from the DXF entity for correct document placement
    polygon->SetBaseProperties(&hatch, document);

    AddToDocument(polygon, document, hatch.m_space);

    ATLTRACE2(traceGeneral, 2, L"  Loop %d: created EoDbPolygon (%s, %d vertices)\n", loopIndex,
        loopPolygonStyle == EoDb::PolygonStyle::Solid       ? L"Solid"
            : loopPolygonStyle == EoDb::PolygonStyle::Hatch ? L"Hatch"
                                                             : L"Hollow",
        static_cast<int>(boundaryPoints.GetSize()));
  }
}
