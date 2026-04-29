#pragma once
#include <cstdint>
#include "AeSysState.h"
#include "EoGePoint3d.h"

class AeSysView;

/// @brief Mode state for Trap / Trapr modes (Phase 2B-2).
/// Owns the per-mode interaction state previously stored on AeSysView:
///   * m_previousOp     — the active stitch/field gesture op (ID_OP2 line, ID_OP4 rectangle).
///   * m_previousPoint  — the anchor point captured on the first click of a two-click gesture.
/// Both Trap (add groups) and Trapr (remove groups) variants share this state object;
/// toggling between them via OnTrapCommandsAddGroups -> OnModeTrap pops and re-pushes it,
/// which is acceptable because no in-progress gesture survives a mode switch anyway.
class TrapModeState : public AeSysState {
 public:
  TrapModeState() = default;
  TrapModeState(const TrapModeState&) = delete;
  TrapModeState& operator=(const TrapModeState&) = delete;
  TrapModeState(TrapModeState&&) = delete;
  TrapModeState& operator=(TrapModeState&&) = delete;

  void OnExit(AeSysView* context) override;

  /// Unhighlights the active op pane (if any) via the view's status bar API.
  /// Passes m_previousOp by reference so it is also reset to 0 by ModeLineUnhighlightOp.
  void UnhighlightOp(AeSysView* context);

  [[nodiscard]] std::uint16_t PreviousOp() const noexcept { return m_previousOp; }
  void SetPreviousOp(std::uint16_t op) noexcept { m_previousOp = op; }

  [[nodiscard]] const EoGePoint3d& PreviousPoint() const noexcept { return m_previousPoint; }
  void SetPreviousPoint(const EoGePoint3d& point) noexcept { m_previousPoint = point; }

 private:
  std::uint16_t m_previousOp{};
  EoGePoint3d m_previousPoint{};
};
