#include "Stdafx.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbBlockReference.h"
#include "EoDbConic.h"
#include "EoDbDxfInterface.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoDbSpline.h"
#include "EoDbViewport.h"
#include "EoDxfEntities.h"
#include "EoGeLine.h"
#include "EoGeOcsTransform.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

void EoDbDxfInterface::ConvertArcEntity(const EoDxfArc& arc, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Arc entity conversion\n");

  if (arc.m_radius < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Arc entity with non-positive radius (%f) skipped.\n", arc.m_radius);
    return;
  }
  const EoGePoint3d center(arc.m_centerPoint.x, arc.m_centerPoint.y, arc.m_centerPoint.z);

  EoGeVector3d extrusion(arc.m_extrusionDirection.x, arc.m_extrusionDirection.y, arc.m_extrusionDirection.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Unitize();
  }
  // OCS parametric angles pass through unchanged — ComputeArbitraryAxis in CreateRadialArc
  // encodes the OCS→WCS directional flip via the major axis direction.
  const double startAngle = EoDbConic::NormalizeTo2Pi(arc.m_startAngle);
  const double endAngle = EoDbConic::NormalizeTo2Pi(arc.m_endAngle);

  auto* radialArc = EoDbConic::CreateRadialArc(center, extrusion, arc.m_radius, startAngle, endAngle);
  if (radialArc == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Failed to create radial arc.\n");
    return;
  }
  radialArc->SetBaseProperties(&arc, document);
  AddToDocument(radialArc, document, arc.m_space, arc.m_ownerHandle);
}

void EoDbDxfInterface::ConvertCircleEntity(const EoDxfCircle& circle, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Circle entity conversion\n");

  if (circle.m_radius < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Circle entity with non-positive radius (%f) skipped.\n", circle.m_radius);
    return;
  }
  const EoGePoint3d center(circle.m_centerPoint.x, circle.m_centerPoint.y, circle.m_centerPoint.z);
  EoGeVector3d extrusion(circle.m_extrusionDirection.x, circle.m_extrusionDirection.y, circle.m_extrusionDirection.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Unitize();
  }
  auto* conic = EoDbConic::CreateCircle(center, extrusion, circle.m_radius);
  if (conic == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Failed to create circle.\n");
    return;
  }
  conic->SetBaseProperties(&circle, document);
  AddToDocument(conic, document, circle.m_space, circle.m_ownerHandle);
}

/** @brief Converts a DXF ELLIPSE entity
 *
 *  DXF ELLIPSE entities represent both full ellipses and elliptical arcs. The entity is defined
 *  by a center point, a major axis endpoint (relative to center), a minor-to-major axis ratio,
 *  and start/end parameter angles. When start=0 and end=2π, it is a full ellipse.
 *
 *  ## DXF Group Code Summary
 *  | Code | Field | Notes |
 *  |------|-------|-------|
 *  | 10/20/30 | Center point | In WCS (unlike ARC/CIRCLE which use OCS) |
 *  | 11/21/31 | Major axis endpoint | Relative to center, in WCS |
 *  | 40 | Ratio | Minor/major, (0.0, 1.0] |
 *  | 41 | Start parameter | Radians, 0.0 for full ellipse |
 *  | 42 | End parameter | Radians, 2π for full ellipse |
 *  | 210/220/230 | Extrusion direction | WCS — defines ellipse plane normal |
 *
 *  ## Coordinate System
 *  Per the DXF specification, ELLIPSE center and major axis endpoint are in WCS. No OCS→WCS
 *  transform is applied (unlike ARC/CIRCLE). The extrusion direction defines the ellipse plane
 *  normal and is used only by MinorAxis() = Cross(extrusion, majorAxis) × ratio.
 *
 *  @param ellipse The parsed DXF ELLIPSE entity.
 *  @param document The AeSys document receiving the created primitive.
 */
void EoDbDxfInterface::ConvertEllipseEntity(const EoDxfEllipse& ellipse, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Ellipse entity conversion (ratio=%.4f, startParam=%.4f, endParam=%.4f)\n",
      ellipse.m_ratio, ellipse.m_startParam, ellipse.m_endParam);

  if (ellipse.m_ratio <= 0.0 || ellipse.m_ratio > 1.0) {
    ATLTRACE2(
        traceGeneral, 1, L"Ellipse entity skipped: invalid ratio (%.6f) — must be in (0.0, 1.0]\n", ellipse.m_ratio);
    return;
  }
  const EoGeVector3d majorAxis(
      ellipse.m_endPointOfMajorAxis.x, ellipse.m_endPointOfMajorAxis.y, ellipse.m_endPointOfMajorAxis.z);
  if (majorAxis.IsNearNull()) {
    ATLTRACE2(traceGeneral, 1, L"Ellipse entity skipped: zero-length major axis\n");
    return;
  }
  EoGeVector3d extrusion(
      ellipse.m_extrusionDirection.x, ellipse.m_extrusionDirection.y, ellipse.m_extrusionDirection.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Unitize();
  }
  const EoGePoint3d center = EoGePoint3d(ellipse.m_centerPoint.x, ellipse.m_centerPoint.y, ellipse.m_centerPoint.z);

  ATLTRACE2(traceGeneral, 3, L"  center=(%.4f, %.4f, %.4f) majorAxis=(%.4f, %.4f, %.4f) majorLen=%.4f\n", center.x,
      center.y, center.z, majorAxis.x, majorAxis.y, majorAxis.z, majorAxis.Length());
  ATLTRACE2(traceGeneral, 3, L"  extrusion=(%.4f, %.4f, %.4f) ratio=%.4f\n", extrusion.x, extrusion.y, extrusion.z,
      ellipse.m_ratio);

  auto* conic =
      EoDbConic::CreateConic(center, extrusion, majorAxis, ellipse.m_ratio, ellipse.m_startParam, ellipse.m_endParam);
  if (conic == nullptr) {
    ATLTRACE2(traceGeneral, 1, L"Ellipse entity skipped: CreateConic returned nullptr\n");
    return;
  }
  conic->SetBaseProperties(&ellipse, document);
  AddToDocument(conic, document, ellipse.m_space, ellipse.m_ownerHandle);

  const bool isFullEllipse = Eo::IsGeometricallyZero(ellipse.m_endParam - ellipse.m_startParam - Eo::TwoPi) ||
      Eo::IsGeometricallyZero(ellipse.m_endParam - ellipse.m_startParam);
  ATLTRACE2(traceGeneral, 2, L"  → EoDbConic %s (majorLen=%.4f, minorLen=%.4f)\n",
      isFullEllipse ? L"Ellipse" : L"EllipticalArc", majorAxis.Length(), majorAxis.Length() * ellipse.m_ratio);
}

EoDbBlockReference* EoDbDxfInterface::ConvertInsertEntity(const EoDxfInsert& blockReference, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Insert entity conversion\n");
  auto insertPrimitive = new EoDbBlockReference();
  insertPrimitive->SetBaseProperties(&blockReference, document);
  insertPrimitive->SetName(CString(blockReference.m_blockName.c_str()));
  insertPrimitive->SetInsertionPoint(blockReference.m_insertionPoint);
  insertPrimitive->SetNormal(EoGeVector3d(blockReference.m_extrusionDirection.x, blockReference.m_extrusionDirection.y,
      blockReference.m_extrusionDirection.z));
  insertPrimitive->SetScaleFactors(
      EoGeVector3d(blockReference.m_xScaleFactor, blockReference.m_yScaleFactor, blockReference.m_zScaleFactor));
  insertPrimitive->SetRotation(blockReference.m_rotationAngle);
  insertPrimitive->SetColumns(blockReference.m_columnCount);
  insertPrimitive->SetColumnSpacing(blockReference.m_columnSpacing);
  insertPrimitive->SetRows(blockReference.m_rowCount);
  insertPrimitive->SetRowSpacing(blockReference.m_rowSpacing);

  // Preserve the extension dictionary handle for dynamic block resolution.
  // ResolveDynamicBlockReferences() uses this after full DXF import to follow
  // the INSERT → DICTIONARY → BLOCKREPRESENTATION → anonymous block chain.
  insertPrimitive->SetExtensionDictionaryHandle(blockReference.m_extensionDictionaryHandle);

  m_currentInsertGroup = AddToDocument(insertPrimitive, document, blockReference.m_space, blockReference.m_ownerHandle);
  return insertPrimitive;
}

void EoDbDxfInterface::ConvertLineEntity(const EoDxfLine& line, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Line entity conversion\n");

  auto linePrimitive = new EoDbLine();
  linePrimitive->SetBaseProperties(&line, document);

  linePrimitive->SetLine(EoGeLine(EoGePoint3d{line.m_startPoint}, EoGePoint3d{line.m_endPoint}));
  AddToDocument(linePrimitive, document, line.m_space, line.m_ownerHandle);
}

void EoDbDxfInterface::ConvertLWPolylineEntity(const EoDxfLwPolyline& polyline, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"LWPolyline entity conversion\n");

  auto* polylinePrimitive = new EoDbPolyline();
  polylinePrimitive->SetBaseProperties(&polyline, document);

  const auto numVerts = static_cast<std::uint16_t>(polyline.m_vertices.size());
  polylinePrimitive->SetNumberOfVertices(numVerts);

  for (std::uint16_t index = 0; index < numVerts; ++index) {
    polylinePrimitive->SetVertexFromLwVertex(index, polyline.m_vertices[index]);
  }

  if (polyline.m_polylineFlag & 0x01) { polylinePrimitive->SetClosed(true); }
  if (polyline.m_polylineFlag & 0x80) { polylinePrimitive->SetPlinegen(true); }

  // Store constant width for round-trip fidelity
  polylinePrimitive->SetConstantWidth(polyline.m_constantWidth);

  // Populate per-vertex bulge values when any vertex has a non-zero bulge
  const bool hasAnyBulge = std::any_of(polyline.m_vertices.begin(), polyline.m_vertices.end(),
      [](const EoDxfPolylineVertex2d& vertex) noexcept { return vertex.bulge != 0.0; });
  if (hasAnyBulge) {
    std::vector<double> bulges(numVerts);
    for (std::uint16_t index = 0; index < numVerts; ++index) { bulges[index] = polyline.m_vertices[index].bulge; }
    polylinePrimitive->SetBulges(std::move(bulges));
  }

  // Populate per-vertex width values: use per-vertex widths if present, else expand constant width.
  // DXF convention: per-vertex width 0 means "use the constant width" when a constant width is set.
  // When any vertex has an explicit non-zero width, we still fill zero-width vertices with the
  // constant width as fallback (mixed usage).
  const bool hasAnyPerVertexWidth = std::any_of(polyline.m_vertices.begin(), polyline.m_vertices.end(),
      [](const EoDxfPolylineVertex2d& vertex) { return vertex.stawidth != 0.0 || vertex.endwidth != 0.0; });
  if (hasAnyPerVertexWidth) {
    std::vector<double> startWidths(numVerts);
    std::vector<double> endWidths(numVerts);
    for (std::uint16_t index = 0; index < numVerts; ++index) {
      const auto& vertex = polyline.m_vertices[index];
      startWidths[index] = (Eo::IsGeometricallyNonZero(vertex.stawidth)) ? vertex.stawidth : polyline.m_constantWidth;
      endWidths[index] = (Eo::IsGeometricallyNonZero(vertex.endwidth)) ? vertex.endwidth : polyline.m_constantWidth;
    }
    polylinePrimitive->SetWidths(std::move(startWidths), std::move(endWidths));
  } else if (Eo::IsGeometricallyNonZero(polyline.m_constantWidth)) {
    // Expand constant width into per-vertex start/end widths
    std::vector<double> startWidths(numVerts, polyline.m_constantWidth);
    std::vector<double> endWidths(numVerts, polyline.m_constantWidth);
    polylinePrimitive->SetWidths(std::move(startWidths), std::move(endWidths));
  }

  AddToDocument(polylinePrimitive, document, polyline.m_space, polyline.m_ownerHandle);
}

void EoDbDxfInterface::ConvertPolyline3DEntity(const EoDxfPolyline& polyline, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Polyline 3D entity conversion (%zu vertices)\n", polyline.m_vertices.size());

  if (polyline.m_vertices.empty()) { return; }

  auto* polylinePrimitive = new EoDbPolyline();
  polylinePrimitive->SetBaseProperties(&polyline, document);

  const auto numVerts = static_cast<std::uint16_t>(polyline.m_vertices.size());
  polylinePrimitive->SetNumberOfVertices(numVerts);

  for (std::uint16_t index = 0; index < numVerts; ++index) {
    const auto& vertex = polyline.m_vertices[index];
    // 3D polyline vertices are truly 3D — no OCS/elevation mapping needed
    polylinePrimitive->SetVertex(
        index, EoGePoint3d{vertex->m_locationPoint.x, vertex->m_locationPoint.y, vertex->m_locationPoint.z});
  }

  if (polyline.m_polylineFlag & 0x01) { polylinePrimitive->SetClosed(true); }
  if (polyline.m_polylineFlag & 0x80) { polylinePrimitive->SetPlinegen(true); }
  polylinePrimitive->Set3D(true);

  AddToDocument(polylinePrimitive, document, polyline.m_space, polyline.m_ownerHandle);
}

void EoDbDxfInterface::ConvertPolyline2DEntity(const EoDxfPolyline& polyline, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Polyline 2D entity conversion (%zu vertices)\n", polyline.m_vertices.size());

  if (polyline.m_vertices.empty()) { return; }

  // For curve-fit (0x02) and spline-fit (0x04) polylines, keep all vertices including generated
  // points. The curve-fit/spline-fit structure is lost, but the rendered geometry is preserved.

  auto* polylinePrimitive = new EoDbPolyline();
  polylinePrimitive->SetBaseProperties(&polyline, document);

  const auto numVerts = static_cast<std::uint16_t>(polyline.m_vertices.size());
  polylinePrimitive->SetNumberOfVertices(numVerts);

  // 2D POLYLINE: vertex x,y are in OCS; z is the polyline elevation
  const double elevation = polyline.m_polylineElevation.z;

  // Resolve extrusion direction for OCS → WCS transform
  EoGeVector3d extrusionDirection{
      polyline.m_extrusionDirection.x, polyline.m_extrusionDirection.y, polyline.m_extrusionDirection.z};
  if (extrusionDirection.IsNearNull()) {
    extrusionDirection = EoGeVector3d::positiveUnitZ;
  } else {
    extrusionDirection.Unitize();
  }
  const bool needsOcsTransform = Eo::IsGeometricallyNonZero(extrusionDirection.x) ||
      Eo::IsGeometricallyNonZero(extrusionDirection.y) || Eo::IsGeometricallyNonZero(extrusionDirection.z - 1.0);
  const EoGeOcsTransform transformOcs{extrusionDirection};

  for (std::uint16_t index = 0; index < numVerts; ++index) {
    const auto& vertex = polyline.m_vertices[index];
    auto point = EoGePoint3d{vertex->m_locationPoint.x, vertex->m_locationPoint.y, elevation};
    if (needsOcsTransform) { point = transformOcs * point; }
    polylinePrimitive->SetVertex(index, point);
  }

  if (polyline.m_polylineFlag & 0x01) { polylinePrimitive->SetClosed(true); }
  if (polyline.m_polylineFlag & 0x80) { polylinePrimitive->SetPlinegen(true); }

  // Populate per-vertex bulge values when any vertex has a non-zero bulge
  const bool hasAnyBulge = std::any_of(polyline.m_vertices.begin(), polyline.m_vertices.end(),
      [](const EoDxfVertex* vertex) { return vertex->m_bulge != 0.0; });
  if (hasAnyBulge) {
    std::vector<double> bulges(numVerts);
    for (std::uint16_t index = 0; index < numVerts; ++index) { bulges[index] = polyline.m_vertices[index]->m_bulge; }
    polylinePrimitive->SetBulges(std::move(bulges));
  }

  // Populate per-vertex width values: use per-vertex widths if present, else expand default widths.
  // DXF convention: per-vertex width 0 means "use default width" when default widths are set.
  const bool hasAnyPerVertexWidth = std::any_of(polyline.m_vertices.begin(), polyline.m_vertices.end(),
      [](const EoDxfVertex* vertex) { return vertex->m_startingWidth != 0.0 || vertex->m_endingWidth != 0.0; });
  if (hasAnyPerVertexWidth) {
    std::vector<double> startWidths(numVerts);
    std::vector<double> endWidths(numVerts);
    for (std::uint16_t index = 0; index < numVerts; ++index) {
      const auto* vertex = polyline.m_vertices[index];
      startWidths[index] = (Eo::IsGeometricallyNonZero(vertex->m_startingWidth)) ? vertex->m_startingWidth
                                                                                  : polyline.m_defaultStartWidth;
      endWidths[index] =
          (Eo::IsGeometricallyNonZero(vertex->m_endingWidth)) ? vertex->m_endingWidth : polyline.m_defaultEndWidth;
    }
    polylinePrimitive->SetWidths(std::move(startWidths), std::move(endWidths));
  } else if (Eo::IsGeometricallyNonZero(polyline.m_defaultStartWidth) ||
      Eo::IsGeometricallyNonZero(polyline.m_defaultEndWidth)) {
    // Expand default widths into per-vertex start/end widths
    std::vector<double> startWidths(numVerts, polyline.m_defaultStartWidth);
    std::vector<double> endWidths(numVerts, polyline.m_defaultEndWidth);
    polylinePrimitive->SetWidths(std::move(startWidths), std::move(endWidths));
  }

  AddToDocument(polylinePrimitive, document, polyline.m_space, polyline.m_ownerHandle);
}

void EoDbDxfInterface::ConvertPointEntity(const EoDxfPoint& point, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Point entity conversion\n");

  auto pointPrimitive = new EoDbPoint();
  pointPrimitive->SetBaseProperties(&point, document);
  pointPrimitive->SetPoint(point.m_pointLocation.x, point.m_pointLocation.y, point.m_pointLocation.z);
  AddToDocument(pointPrimitive, document, point.m_space, point.m_ownerHandle);
}

/** @brief Converts a DXF SPLINE entity to an EoDbSpline primitive in the AeSys document.
 *
 *  DXF SPLINE entities (AutoCAD 13+) define B-splines with degree, knot vector, control points,
 *  and optional fit points and weight values. AeSys `EoDbSpline` stores only control points and
 *  uses `GenPts(3, m_pts)` to tessellate a uniform cubic B-spline at render time.
 *
 *  ## Mapping Strategy
 *  - **Control points present**: Use them directly. The control polygon defines the spline shape.
 *    `EoDbSpline::GenPts` applies its own uniform knot vector with order 3 (quadratic), so the
 *    curve is an approximation of the original DXF spline when the DXF degree differs from 2 or
 *    the knot vector is non-uniform. This is acceptable for display-quality rendering.
 *  - **Fit points only (no control points)**: Use fit points as control points. The resulting
 *    B-spline will approximate the fit points rather than interpolating them exactly.
 *  - **Closed splines** (flag bit 0x01): Not specially handled; the control point polygon is used
 *    as-is. AutoCAD wraps control points for closed splines, so the parsed points already encode
 *    closure.
 *
 *  ## Limitations
 *  - Rational splines (NURBS with non-unit weights) are rendered as non-rational approximations.
 *
 *  ## Coordinate System
 *  - DXF SPLINE control/fit points are in WCS (same as ELLIPSE). `ApplyExtrusion()` is a no-op.
 *  - No OCS → WCS transform is needed or applied.
 *
 *  @param spline The parsed DXF spline entity.
 *  @param document The AeSys document receiving the created primitive.
 */
void EoDbDxfInterface::ConvertSplineEntity(const EoDxfSpline& spline, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2,
      L"Spline entity conversion (degree=%d, controlPts=%d, fitPts=%d, knots=%d, flags=0x%04X)\n",
      spline.m_degreeOfTheSplineCurve, spline.m_numberOfControlPoints, spline.m_numberOfFitPoints,
      spline.m_numberOfKnots, spline.m_splineFlag);

  // Determine which point set to use: control points preferred, fit points as fallback
  const auto& controlPoints = spline.m_controlPoints;
  const auto& fitPoints = spline.m_fitPoints;

  const bool haveControlPoints = !controlPoints.empty();
  const bool haveFitPoints = !fitPoints.empty();

  if (!haveControlPoints && !haveFitPoints) {
    ATLTRACE2(traceGeneral, 1, L"Spline entity skipped: no control points and no fit points\n");
    return;
  }

  // Select point source
  const auto& sourcePoints = haveControlPoints ? controlPoints : fitPoints;
  const auto pointCount = static_cast<std::uint16_t>(sourcePoints.size());

  if (pointCount < 2) {
    ATLTRACE2(traceGeneral, 1, L"Spline entity skipped: fewer than 2 points (%d)\n", pointCount);
    return;
  }

  if (!haveControlPoints) {
    ATLTRACE2(
        traceGeneral, 2, L"  Spline: using %d fit points as control points (no control points defined)\n", pointCount);
  }

  // DXF SPLINE control/fit points are in WCS (same as ELLIPSE) — no OCS transform needed.
  EoGePoint3dArray points;
  points.SetSize(pointCount);

  for (std::uint16_t i = 0; i < pointCount; ++i) {
    const auto* sourcePoint = sourcePoints[i];
    points[i] = EoGePoint3d{sourcePoint->x, sourcePoint->y, sourcePoint->z};
  }

  auto* splinePrimitive = new EoDbSpline(points);
  splinePrimitive->SetBaseProperties(&spline, document);

  // Preserve DXF spline properties for faithful round-trip export and correct-degree rendering.
  splinePrimitive->SetDegree(spline.m_degreeOfTheSplineCurve > 0 ? spline.m_degreeOfTheSplineCurve
                                                                   : static_cast<std::int16_t>(3));
  splinePrimitive->SetFlags(spline.m_splineFlag);

  // Import knot vector when control points are used (knots are meaningless with fit-point fallback).
  if (haveControlPoints && !spline.m_knotValues.empty()) {
    splinePrimitive->SetKnots(std::vector<double>(spline.m_knotValues.begin(), spline.m_knotValues.end()));
  }

  // Import weights for rational splines (NURBS).
  if (haveControlPoints && spline.IsRational() && !spline.m_weightValues.empty()) {
    splinePrimitive->SetWeights(std::vector<double>(spline.m_weightValues.begin(), spline.m_weightValues.end()));
  }

  AddToDocument(splinePrimitive, document, spline.m_space, spline.m_ownerHandle);

  ATLTRACE2(traceGeneral, 2, L"  Spline \u2192 EoDbSpline (%d pts, degree=%d, flags=0x%04X, knots=%zu)\n",
      pointCount, splinePrimitive->Degree(), splinePrimitive->Flags(), splinePrimitive->Knots().size());
}

void EoDbDxfInterface::ConvertViewportEntity(const EoDxfViewport& viewport, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Viewport entity conversion (id=%d, status=%d)\n", viewport.m_viewportId,
      viewport.m_viewportStatus);

  auto* viewportPrimitive = new EoDbViewport();
  viewportPrimitive->SetBaseProperties(&viewport, document);

  // Paper-space geometry
  viewportPrimitive->SetCenterPoint(
      EoGePoint3d{viewport.m_centerPoint.x, viewport.m_centerPoint.y, viewport.m_centerPoint.z});
  viewportPrimitive->SetWidth(viewport.m_width);
  viewportPrimitive->SetHeight(viewport.m_height);

  // Viewport identity
  viewportPrimitive->SetViewportStatus(viewport.m_viewportStatus);
  viewportPrimitive->SetViewportId(viewport.m_viewportId);

  // Model-space view parameters (round-trip preservation)
  viewportPrimitive->SetViewCenter(EoGePoint3d{viewport.m_viewCenter.x, viewport.m_viewCenter.y, 0.0});
  viewportPrimitive->SetSnapBasePoint(EoGePoint3d{viewport.m_snapBasePoint.x, viewport.m_snapBasePoint.y, 0.0});
  viewportPrimitive->SetSnapSpacing(EoGePoint3d{viewport.m_snapSpacing.x, viewport.m_snapSpacing.y, 0.0});
  viewportPrimitive->SetGridSpacing(EoGePoint3d{viewport.m_gridSpacing.x, viewport.m_gridSpacing.y, 0.0});
  viewportPrimitive->SetViewDirection(
      EoGePoint3d{viewport.m_viewDirection.x, viewport.m_viewDirection.y, viewport.m_viewDirection.z});
  viewportPrimitive->SetViewTargetPoint(
      EoGePoint3d{viewport.m_viewTargetPoint.x, viewport.m_viewTargetPoint.y, viewport.m_viewTargetPoint.z});
  viewportPrimitive->SetLensLength(viewport.m_lensLength);
  viewportPrimitive->SetFrontClipPlane(viewport.m_frontClipPlane);
  viewportPrimitive->SetBackClipPlane(viewport.m_backClipPlane);
  viewportPrimitive->SetViewHeight(viewport.m_viewHeight);
  viewportPrimitive->SetSnapAngle(viewport.m_snapAngle);
  viewportPrimitive->SetTwistAngle(viewport.m_twistAngle);

  AddToDocument(viewportPrimitive, document, viewport.m_space, viewport.m_ownerHandle);

  ATLTRACE2(traceGeneral, 2, L"  Viewport id=%d size=(%.2f x %.2f) center=(%.2f, %.2f)\n",
      viewport.m_viewportId, viewport.m_width, viewport.m_height, viewport.m_centerPoint.x, viewport.m_centerPoint.y);
}
