#include "Stdafx.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "AeSysView.h"
#include "Eo.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsVertexBuffer.h"

namespace {
/// @brief Global vertex buffer instance — replaces the former file-scope pts_/LoopLine.
EoGsVertexBuffer vertexBuffer_;
}  // namespace

namespace polyline {

// ── Thin wrappers delegating to the global EoGsVertexBuffer ───────────

void BeginLineLoop() {
  vertexBuffer_.BeginLineLoop();
}

void BeginLineStrip() {
  vertexBuffer_.BeginLineStrip();
}

void SetVertex(const EoGePoint3d& point) {
  vertexBuffer_.SetVertex(point);
}

void End(AeSysView* view, EoGsRenderDevice* renderDevice, std::int16_t lineType, const std::wstring& lineTypeName) {
  vertexBuffer_.End(view, renderDevice, lineType, lineTypeName);
}

bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  return vertexBuffer_.SelectUsingLine(view, line, intersections);
}

bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, double& dRel, EoGePoint3d& ptProj) {
  return vertexBuffer_.SelectUsingPoint(view, point, dRel, ptProj);
}

bool SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeftPoint, EoGePoint3d upperRightPoint) {
  return vertexBuffer_.SelectUsingRectangle(view, lowerLeftPoint, upperRightPoint);
}

// ── Standalone geometry helpers (no vertex-buffer dependency) ──────────

// Not considering possible closure
bool SelectUsingRectangle(AeSysView* view,
    EoGePoint3d lowerLeftPoint,
    EoGePoint3d upperRightPoint,
    const EoGePoint3dArray& pts) {
  EoGePoint4d begin(pts[0]);
  view->ModelViewTransformPoint(begin);

  for (std::uint16_t w = 1; w < pts.GetSize(); w++) {
    EoGePoint4d end(pts[w]);
    view->ModelViewTransformPoint(end);

    EoGeLine lineSegment(EoGePoint3d{begin}, EoGePoint3d{end});
    if (lineSegment.IsContainedXY(lowerLeftPoint, upperRightPoint)) { return true; }

    begin = end;
  }
  return false;
}

void GeneratePointsForNPoly(EoGePoint3d& centerPoint,
    EoGeVector3d majorAxis,
    EoGeVector3d minorAxis,
    int numberOfPoints,
    EoGePoint3dArray& pts) {
  EoGeTransformMatrix transformMatrix(centerPoint, majorAxis, minorAxis);

  transformMatrix.Inverse();

  // Determine the parameter (angular increment)
  const double angleIncrement = Eo::TwoPi / double(numberOfPoints);
  const double cosIncrement = std::cos(angleIncrement);
  const double sinIncrement = std::sin(angleIncrement);
  pts.SetSize(numberOfPoints);

  pts[0].Set(1.0, 0.0, 0.0);

  for (int i = 0; i < numberOfPoints - 1; i++) {
    pts[i + 1].Set(
        pts[i].x * cosIncrement - pts[i].y * sinIncrement, pts[i].y * cosIncrement + pts[i].x * sinIncrement, 0.0);
  }
  for (int i = 0; i < numberOfPoints; i++) { pts[i] = transformMatrix * pts[i]; }
}

void TessellateArcSegment(const EoGePoint3d& startPoint,
    const EoGePoint3d& endPoint,
    double bulge,
    std::vector<EoGePoint3d>& arcPoints) {
  arcPoints.clear();

  // Zero bulge — straight segment: just emit the end point
  if (Eo::IsGeometricallyZero(bulge)) {
    arcPoints.push_back(endPoint);
    return;
  }

  // Chord vector and length
  const EoGeVector3d chord(startPoint, endPoint);
  const double chordLength = chord.Length();

  if (chordLength < Eo::geometricTolerance) {
    arcPoints.push_back(endPoint);
    return;
  }

  // Included angle: bulge = tan(θ/4), so θ = 4 × atan(|bulge|)
  const double includedAngle = 4.0 * std::atan(std::abs(bulge));

  // Radius from chord length and included angle: r = (chordLength / 2) / sin(θ / 2)
  const double halfAngle = includedAngle / 2.0;
  const double sinHalfAngle = std::sin(halfAngle);

  if (Eo::IsGeometricallyZero(sinHalfAngle)) {
    arcPoints.push_back(endPoint);
    return;
  }
  const double radius = (chordLength / 2.0) / sinHalfAngle;

  // Sagitta: distance from chord midpoint to arc midpoint
  const double sagitta = std::abs(bulge) * chordLength / 2.0;

  // Arc center: offset perpendicular to chord from the chord midpoint.
  // The perpendicular direction depends on the sign of bulge and the arc plane.
  //
  // Determine the arc plane normal. For 2D polylines (LWPOLYLINE) the plane is XY
  // after OCS→WCS transform, but the chord may be in an arbitrary 3D plane if the
  // polyline was extruded. Use the cross product with +Z to get a perpendicular
  // direction in the chord's plane; if the chord is parallel to Z, fall back to +Y.
  EoGeVector3d chordUnit = chord;
  chordUnit /= chordLength;

  EoGeVector3d planeNormal = CrossProduct(chordUnit, EoGeVector3d::positiveUnitZ);
  if (planeNormal.IsNearNull()) { planeNormal = CrossProduct(chordUnit, EoGeVector3d::positiveUnitY); }
  if (planeNormal.IsNearNull()) {
    // Degenerate: chord is a zero-length vector in disguise
    arcPoints.push_back(endPoint);
    return;
  }
  planeNormal.Unitize();

  // perpDir points from chord midpoint toward the arc center.
  // Positive bulge = CCW arc = center is to the left of chord direction.
  // The cross product chordUnit × Z gives a vector pointing to the right,
  // so for positive bulge we negate it.
  EoGeVector3d perpDir = planeNormal;
  if (bulge > 0.0) { perpDir *= -1.0; }

  // Distance from chord midpoint to center = radius - sagitta
  const double centerOffset = radius - sagitta;

  const EoGePoint3d chordMidpoint = EoGePoint3d::Mid(startPoint, endPoint);
  const EoGePoint3d center = chordMidpoint + perpDir * centerOffset;

  // Sweep angle: positive bulge → CCW → positive angle
  //              negative bulge → CW  → negative angle
  double sweepAngle = includedAngle;
  if (bulge < 0.0) { sweepAngle = -sweepAngle; }

  // Adaptive tessellation: scale with included angle, minimum 2 segments per arc
  const int numberOfSegments = std::max(Eo::arcTessellationMinimumSegments,
      static_cast<int>(std::ceil(std::abs(sweepAngle) / Eo::TwoPi * Eo::arcTessellationSegmentsPerFullCircle)));

  arcPoints.reserve(static_cast<size_t>(numberOfSegments));

  const double segmentAngle = sweepAngle / numberOfSegments;
  const double cosIncrement = std::cos(segmentAngle);
  const double sinIncrement = std::sin(segmentAngle);

  // Rotate the direction vector from center to start by incremental angles.
  // This directly produces arc points without requiring a local coordinate frame.
  // The 2D rotation matrix naturally handles CCW (positive angle) and CW (negative angle).
  double directionX = startPoint.x - center.x;
  double directionY = startPoint.y - center.y;

  // Generate intermediate points (skip index 0 = startPoint, include index N = endPoint)
  for (int i = 1; i <= numberOfSegments; ++i) {
    const double rotatedDirectionX = directionX * cosIncrement - directionY * sinIncrement;
    const double rotatedDirectionY = directionX * sinIncrement + directionY * cosIncrement;
    directionX = rotatedDirectionX;
    directionY = rotatedDirectionY;

    EoGePoint3d arcPoint;
    arcPoint.x = center.x + directionX;
    arcPoint.y = center.y + directionY;
    arcPoint.z = startPoint.z;
    arcPoints.push_back(arcPoint);
  }

  // Snap the last point to the exact endpoint to avoid floating-point drift
  if (!arcPoints.empty()) { arcPoints.back() = endPoint; }
}

}  // namespace polyline
