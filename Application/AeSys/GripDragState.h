#pragma once
#include "AeSysState.h"
#include "EoGePoint3d.h"

class AeSysView;
class EoDbGroup;
class EoDbPrimitive;

/// @brief Transient grip-drag sub-mode state, pushed on top of any primary mode state
/// when the user LMB-clicks on a grip square drawn on a trapped primitive's control point.
///
/// Behaviour mirrors PrimitiveMendState: OnMouseMove drags the selected control point
/// live via TranslateUsingMask; Return/Enter commits; Escape reverts via the saved copy.
/// Both paths pop cleanly back to the primary mode below on the stack.
///
/// Endpoint snap: while dragging, OnMouseMove scans all visible primitives for a control
/// point within kSnapRadiusPx device pixels. When found, the translation target is snapped
/// to that exact world-space position instead of the raw cursor. A cyan triangle marker is
/// drawn at the snap target during the drag (see AeSysViewRender::DrawGripMarkersD2D).
class GripDragState : public AeSysState {
 public:
  /// Snap radius in device pixels used during endpoint-snap scan.
  static constexpr int kSnapRadiusPx{12};

  /// @param group         The document group that owns the primitive (non-owning ptr).
  /// @param primitive     The primitive whose control point is being dragged (non-owning ptr).
  /// @param controlPointMask  The vertex mask for TranslateUsingMask (1 << sm_controlPointIndex).
  /// @param anchorPoint   The world-space position of the grip that was clicked.
  GripDragState(EoDbGroup* group,
      EoDbPrimitive* primitive,
      DWORD controlPointMask,
      const EoGePoint3d& anchorPoint) noexcept
      : m_group{group},
        m_primitive{primitive},
        m_controlPointMask{controlPointMask},
        m_anchorPoint{anchorPoint} {}

  GripDragState(const GripDragState&) = delete;
  GripDragState& operator=(const GripDragState&) = delete;
  GripDragState(GripDragState&&) = delete;
  GripDragState& operator=(GripDragState&&) = delete;
  ~GripDragState() override;

  void OnEnter(AeSysView* context) override;
  void OnExit(AeSysView* context) override;
  bool HandleKeypad(AeSysView* context, UINT nChar, UINT nRepCnt, UINT nFlags) override;
  void OnLButtonDown(AeSysView* context, UINT nFlags, CPoint point) override;
  void OnRButtonUp(AeSysView* context, UINT nFlags, CPoint point) override;
  void OnMouseMove(AeSysView* context, UINT flags, CPoint point) override;
  bool OnReturn(AeSysView* context) override;
  bool OnEscape(AeSysView* context) override;
  [[nodiscard]] bool ShouldBlockCommand(UINT commandId) const noexcept override;

  [[nodiscard]] EoDbGroup* Group() const noexcept { return m_group; }
  [[nodiscard]] EoDbPrimitive* Primitive() const noexcept { return m_primitive; }
  /// World-space snap target set by the most recent OnMouseMove snap scan.
  /// Valid only when m_isSnapped is true; exposed so the render path can draw the marker.
  [[nodiscard]] const EoGePoint3d& SnapTarget() const noexcept { return m_snapTarget; }
  [[nodiscard]] bool IsSnapped() const noexcept { return m_isSnapped; }

 private:
  EoDbGroup* m_group{};             // non-owning
  EoDbPrimitive* m_primitive{};     // non-owning
  EoDbPrimitive* m_originalCopy{};  // owning snapshot for Escape
  DWORD m_controlPointMask{};
  EoGePoint3d m_anchorPoint{};      // original grip world position — never mutated
  EoGePoint3d m_snapTarget{};       // world-space snap candidate from last mouse move
  bool m_isSnapped{false};          // true when cursor is within kSnapRadiusPx of a snap point

  void DeleteCopy() noexcept;
};
