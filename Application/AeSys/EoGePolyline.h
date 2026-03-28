#pragma once

/// @file EoGePolyline.h
/// @brief Thin polyline:: namespace wrappers delegating to a global EoGsVertexBuffer.
///
/// Phase 5: The vertex accumulation / rendering / selection state that was previously
/// file-scope mutable data (EoGePoint4dArray pts_, bool LoopLine) now lives inside
/// EoGsVertexBuffer. The polyline:: free functions are retained for call-site
/// compatibility — they delegate to the single global instance.
///
/// Standalone geometry helpers (GeneratePointsForNPoly, TessellateArcSegment) and the
/// explicit-points SelectUsingRectangle overload have no vertex-buffer dependency
/// and remain as pure free functions.

#include <string>
#include <vector>

class AeSysView;
class EoGePoint3d;
class EoGsRenderDevice;

namespace polyline {

// ── Vertex Accumulation (delegate to global EoGsVertexBuffer) ─────────
void BeginLineStrip();
void BeginLineLoop();
void SetVertex(const EoGePoint3d& point);

// ── Rendering ─────────────────────────────────────────────────────────

/// @brief Renders accumulated vertices using the specified linetype via EoGsRenderDevice.
///
/// Replaces the former __End(AeSysView*, CDC*, ...). Rendering calls go through the
/// abstract EoGsRenderDevice interface (MoveTo/LineTo/Polyline), eliminating the
/// direct CDC* dependency from the polyline rendering pipeline.
void End(AeSysView* view, EoGsRenderDevice* renderDevice, std::int16_t lineType,
    const std::wstring& lineTypeName = {});

// ── Selection (delegate to global EoGsVertexBuffer) ───────────────────

/// @brief Determines how many times (if any) a line segment intersects with the polyline.
bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections);
bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, double& dRel, EoGePoint3d& ptProj);
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2);

// ── Standalone Geometry Helpers (no vertex-buffer dependency) ──────────

/// @brief Tests if any segment of an explicit point array is contained within a rectangle.
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2, const EoGePoint3dArray& pts);

/// @brief Determines points necessary to represent an N-Polygon with line segments.
void GeneratePointsForNPoly(EoGePoint3d& centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis,
    int numberOfPoints, EoGePoint3dArray& pts);

/// @brief Tessellates a single bulge arc segment into approximation points.
///
/// Given two consecutive polyline vertices and the bulge value at the first vertex,
/// generates intermediate arc points. The start point is NOT included in the output
/// (the caller has already emitted it); the end point IS included as the last element.
///
/// @param startPoint   The first endpoint of the segment (already emitted by caller).
/// @param endPoint     The second endpoint of the segment.
/// @param bulge        Bulge value at startPoint: tan(θ/4) where θ is the included arc angle.
///                    Positive = CCW arc, negative = CW arc, 0 = straight (returns endPoint only).
/// @param[out] arcPoints  Output vector receiving the tessellated arc points (cleared on entry).
void TessellateArcSegment(
    const EoGePoint3d& startPoint, const EoGePoint3d& endPoint, double bulge, std::vector<EoGePoint3d>& arcPoints);

}  // namespace polyline
