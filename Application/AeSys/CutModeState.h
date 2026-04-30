#pragma once

#include "AeSysState.h"
#include "EoGePoint3d.h"

/// @brief Per-mode scratch state for Cut mode (Slice/Field/Clip gestures).
///
/// Owns:
///   * m_previousOp   — the active gesture op (ID_OP2 slice, ID_OP4 field,
///                      ID_OP7 clip), or 0 when idle.
///   * m_previousPos  — the first-click anchor for the active two-click gesture.
///
/// Cut mode itself is not push/pop driven by individual gestures; the state
/// lives for the duration of the mode and is popped by `PopAllModeStates()`
/// when the user switches to a different mode.
class CutModeState final : public AeSysState {
 public:
  void OnExit(AeSysView* context) override;
  bool OnReturn(AeSysView* context) override;
  bool OnEscape(AeSysView* context) override;

  /// Resets `m_previousOp` to 0 via reference so `ModeLineUnhighlightOp`
  /// can clear it. Used by `OnCutModeReturn` / `OnCutModeEscape` to abort
  /// the current gesture without popping the mode.
  void UnhighlightOp(AeSysView* context);

  [[nodiscard]] std::uint16_t PreviousOp() const noexcept { return m_previousOp; }
  [[nodiscard]] std::uint16_t& PreviousOpRef() noexcept { return m_previousOp; }
  void SetPreviousOp(std::uint16_t op) noexcept { m_previousOp = op; }

  [[nodiscard]] const EoGePoint3d& PreviousPosition() const noexcept { return m_previousPos; }
  [[nodiscard]] EoGePoint3d& PreviousPositionRef() noexcept { return m_previousPos; }
  void SetPreviousPosition(const EoGePoint3d& position) noexcept { m_previousPos = position; }

 private:
  std::uint16_t m_previousOp{};
  EoGePoint3d m_previousPos{};
};
