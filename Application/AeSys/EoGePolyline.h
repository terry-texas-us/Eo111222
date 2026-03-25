#pragma once

#include <string>
#include <vector>

class AeSysView;
class EoDbLineType;
class EoGePoint3d;

namespace polyline {
void BeginLineStrip();
void BeginLineLoop();
void __Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray, EoDbLineType* lineType);
void __End(AeSysView* view, CDC* deviceContext, std::int16_t lineType, const std::wstring& lineTypeName = {});
/// <summary>Determines points necessary to represent an N-Polygon with line segments.</summary>
void GeneratePointsForNPoly(EoGePoint3d& centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis,
    int numberOfPoints, EoGePoint3dArray& pts);
/// <summary>Determines how many times (if any), a line segment intersects with polyline.</summary>
bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections);
bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, double& dRel, EoGePoint3d& ptProj);
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2);
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2, const EoGePoint3dArray& pts);
void SetVertex(const EoGePoint3d& point);

/// @brief Tessellates a single bulge arc segment into approximation points.
///
/// Given two consecutive polyline vertices and the bulge value at the first vertex,
/// generates intermediate arc points. The start point is NOT included in the output
/// (the caller has already emitted it); the end point IS included as the last element.
///
/// @param startPoint   The first endpoint of the segment (already emitted by caller).
/// @param endPoint     The second endpoint of the segment.
/// @param bulge        Bulge value at startPoint: tan(θ/4) where θ is the included arc angle.
///                     Positive = CCW arc, negative = CW arc, 0 = straight (returns endPoint only).
/// @param[out] arcPoints  Output vector receiving the tessellated arc points (cleared on entry).
void TessellateArcSegment(
    const EoGePoint3d& startPoint, const EoGePoint3d& endPoint, double bulge, std::vector<EoGePoint3d>& arcPoints);
}  // namespace polyline
