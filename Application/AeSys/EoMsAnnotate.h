#pragma once
#include <cstdint>
#include "AeSysState.h"
#include "EoGePoint3d.h"

class AeSysView;

/// @brief Mode state for Annotate mode.
/// Owns the per-mode interaction state previously stored on AeSysView:
///   * m_previousOp — the active gesture op (ID_OP2 line, ID_OP3 arrow,
///     ID_OP4 bubble, ID_OP5 hook, ID_OP7 box, ID_OP9 construction line).
///   * m_points     — the point accumulator used across multi-click gestures.
/// View-owned configuration (BubbleRadius, CircleRadius, EndItemType,
/// EndItemSize, NumberOfSides, GapSpaceFactor) intentionally stays on AeSysView;
/// it survives mode switches and is shared with EoDlgAnnotateOptions.
class AnnotateModeState : public AeSysState {
 public:
  AnnotateModeState() = default;
  AnnotateModeState(const AnnotateModeState&) = delete;
  AnnotateModeState& operator=(const AnnotateModeState&) = delete;
  AnnotateModeState(AnnotateModeState&&) = delete;
  AnnotateModeState& operator=(AnnotateModeState&&) = delete;

  void OnExit(AeSysView* context) override;
  void OnRButtonUp(AeSysView* context, UINT nFlags, CPoint point) override;
  void OnMouseMove(AeSysView* context, UINT nFlags, CPoint point) override;
  bool OnEscape(AeSysView* context) override;
  [[nodiscard]] UINT GetActiveOp() const noexcept override { return m_previousOp; }
  [[nodiscard]] bool HandleCommand(AeSysView* context, UINT command) override;

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
