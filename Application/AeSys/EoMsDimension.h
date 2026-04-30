#pragma once

#include "AeSysState.h"
#include "EoGePoint3d.h"

/// @brief Per-mode scratch state for Dimension mode.
///
/// Owns the previously-file-local `PreviousDimensionCommand` / `PreviousDimensionCursorPosition`
/// statics, which track the active dimension gesture (line, dline, dline2, exten, radius,
/// diameter, angle, convert) and its anchor point.
class DimensionModeState final : public AeSysState {
 public:
  void OnExit(AeSysView* context) override;
  bool OnReturn(AeSysView* context) override;
  bool OnEscape(AeSysView* context) override;
  void OnRButtonUp(AeSysView* context, UINT flags, CPoint point) override;
  [[nodiscard]] UINT GetActiveOp() const noexcept override { return m_previousCommand; }
  [[nodiscard]] bool HandleCommand(AeSysView* context, UINT command) override;

  /// Resets `m_previousCommand` to 0 via reference so `ModeLineUnhighlightOp`
  /// can clear it. Used to abort the current gesture without popping the mode.
  void UnhighlightOp(AeSysView* context);

  [[nodiscard]] std::uint16_t PreviousCommand() const noexcept { return m_previousCommand; }
  [[nodiscard]] std::uint16_t& PreviousCommandRef() noexcept { return m_previousCommand; }
  void SetPreviousCommand(std::uint16_t command) noexcept { m_previousCommand = command; }

  [[nodiscard]] const EoGePoint3d& PreviousCursorPosition() const noexcept { return m_previousCursorPosition; }
  [[nodiscard]] EoGePoint3d& PreviousCursorPositionRef() noexcept { return m_previousCursorPosition; }
  void SetPreviousCursorPosition(const EoGePoint3d& position) noexcept { m_previousCursorPosition = position; }

 private:
  std::uint16_t m_previousCommand{};
  EoGePoint3d m_previousCursorPosition{};
};
