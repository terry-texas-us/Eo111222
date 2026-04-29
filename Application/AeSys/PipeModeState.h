#pragma once
#include <cstdint>
#include "AeSysState.h"
#include "EoGePoint3d.h"

class AeSysView;

/// @brief Mode state for Pipe mode (Phase 2B-3).
/// Owns the per-mode interaction state previously stored on AeSysView:
///   * m_previousOp — the active gesture op (ID_OP2 line, ID_OP3 fitting,
///     ID_OP4 drop, ID_OP5 rise, ID_OP9 wye).
///   * m_points     — the point accumulator used across multi-click gestures.
/// View-owned configuration (m_PipeTicSize, m_PipeRiseDropRadius,
/// m_CurrentPipeSymbolIndex) intentionally stays on AeSysView; it survives
/// mode switches and is shared with EoDlgPipeOptions / EoDlgPipeSymbol.
class PipeModeState : public AeSysState {
 public:
  PipeModeState() = default;
  PipeModeState(const PipeModeState&) = delete;
  PipeModeState& operator=(const PipeModeState&) = delete;
  PipeModeState(PipeModeState&&) = delete;
  PipeModeState& operator=(PipeModeState&&) = delete;

  void OnExit(AeSysView* context) override;

  /// Unhighlights the active op pane (if any) via the view's status bar API.
  /// Passes m_previousOp by reference so it is also reset to 0 by ModeLineUnhighlightOp.
  void UnhighlightOp(AeSysView* context);

  [[nodiscard]] std::uint16_t PreviousOp() const noexcept { return m_previousOp; }
  [[nodiscard]] std::uint16_t& PreviousOpRef() noexcept { return m_previousOp; }
  void SetPreviousOp(std::uint16_t op) noexcept { m_previousOp = op; }

  [[nodiscard]] EoGePoint3dArray& Points() noexcept { return m_points; }
  [[nodiscard]] const EoGePoint3dArray& Points() const noexcept { return m_points; }

 private:
  std::uint16_t m_previousOp{};
  EoGePoint3dArray m_points{};
};
