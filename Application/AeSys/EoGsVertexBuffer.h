#pragma once

/// @file EoGsVertexBuffer.h
/// @brief Retained vertex buffer — object-oriented replacement for polyline:: namespace state.
///
/// Phase 5 of the Direct2D migration plan. Encapsulates the file-scope mutable
/// EoGePoint4dArray + bool LoopLine that previously lived in polyline:: namespace
/// (OpenGL heritage). Rendering methods use EoGsRenderDevice* instead of CDC*,
/// eliminating the GDI dependency from the polyline rendering pipeline.
///
/// A single global instance is accessed via polyline::VertexBuffer() during the
/// transitional period. Phase 8 (batch geometry submission) will introduce
/// per-batch or per-primitive vertex buffers.

#include "EoGePoint4d.h"

#include <cstdint>
#include <string>

class AeSysView;
class EoDbLineType;
class EoGeLine;
class EoGsRenderDevice;

/// @brief Retained vertex buffer for polyline rendering and selection.
///
/// Vertices are accumulated via BeginLineStrip/Loop() + SetVertex() pairs,
/// then either rendered via End() or queried via SelectUsing*() methods.
/// The rendering path uses EoGsRenderDevice* for all drawing calls;
/// renderState (still CDC*-based) is accessed via GetCDC() transitionally.
class EoGsVertexBuffer {
 public:
  EoGsVertexBuffer() = default;

  // ── Vertex Accumulation ─────────────────────────────────────────────

  /// @brief Starts a new line strip (open polyline). Clears accumulated vertices.
  void BeginLineStrip();

  /// @brief Starts a new line loop (closed polyline). Clears accumulated vertices.
  void BeginLineLoop();

  /// @brief Appends a 3D vertex (converted to 4D homogeneous internally).
  void SetVertex(const EoGePoint3d& point);

  // ── Rendering ───────────────────────────────────────────────────────

  /// @brief Renders accumulated vertices using the specified linetype.
  ///
  /// Dispatches to DisplayDashPattern() for named/indexed dash patterns,
  /// or to solid-line rendering via EoGsRenderDevice::MoveTo/LineTo.
  /// @param view      View for model-view transform and projection.
  /// @param renderDevice  Abstract rendering device (GDI or D2D backend).
  /// @param lineType  Internal linetype index.
  /// @param lineTypeName  Linetype name for name-based lookup (empty = use index).
  void End(AeSysView* view, EoGsRenderDevice* renderDevice, std::int16_t lineType,
      const std::wstring& lineTypeName = {});

  // ── Selection (operate on accumulated vertices) ─────────────────────

  /// @brief Tests polyline segments for intersection with a line.
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections);

  /// @brief Tests if a point selects the polyline (nearest-point test).
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, double& dRel, EoGePoint3d& ptProj);

  /// @brief Tests if any polyline segment is contained within a rectangle.
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2);

  // ── Accessors ───────────────────────────────────────────────────────

  /// @brief Read-only access to the accumulated 4D points.
  [[nodiscard]] const EoGePoint4dArray& Points() const noexcept { return m_points; }

 private:
  EoGePoint4dArray m_points;
  bool m_isLoop{false};

  /// @brief Checks if any point in the array is within the view frustum.
  static bool AnyPointsInView(EoGePoint4dArray& pointsArray);

  /// @brief Renders a polyline with a dash-pattern linetype.
  ///
  /// Processes the dash/gap pattern from the linetype definition and draws
  /// individual line segments via renderDevice->Polyline().
  void DisplayDashPattern(
      AeSysView* view, EoGsRenderDevice* renderDevice, EoGePoint4dArray& pointsArray, EoDbLineType* lineType);
};
