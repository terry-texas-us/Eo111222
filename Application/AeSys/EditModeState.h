#pragma once
#include <cstdint>
#include "AeSysState.h"

class AeSysView;

/// @brief Mode state for Edit mode (Phase 2B-2).
/// Owns the per-mode interaction state previously stored on AeSysView:
///   * m_previousOp — the active two-click move (ID_OP4) or copy (ID_OP5) gesture op.
/// Rotate / Flip / Reduce / Enlarge are atomic commands and do not need state.
/// PRIMITIVE_EDIT / GROUP_EDIT / PRIMITIVE_MEND sub-modes are handled by their own
/// dedicated state classes (PickAndDragState / PrimitiveMendState) which pop this state
/// when entering, and rely on app.PrimaryMode() to restore the edit UI on exit.
class EditModeState : public AeSysState {
 public:
  EditModeState() = default;
  EditModeState(const EditModeState&) = delete;
  EditModeState& operator=(const EditModeState&) = delete;
  EditModeState(EditModeState&&) = delete;
  EditModeState& operator=(EditModeState&&) = delete;

  void OnExit(AeSysView* context) override;

  /// Unhighlights the active op pane (if any) via the view's status bar API.
  /// Passes m_previousOp by reference so it is also reset to 0 by ModeLineUnhighlightOp.
  void UnhighlightOp(AeSysView* context);

  [[nodiscard]] std::uint16_t PreviousOp() const noexcept { return m_previousOp; }
  void SetPreviousOp(std::uint16_t op) noexcept { m_previousOp = op; }

 private:
  std::uint16_t m_previousOp{};
};
