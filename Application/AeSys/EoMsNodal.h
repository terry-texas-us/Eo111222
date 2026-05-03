#pragma once

#include "AeSysState.h"
#include "EoGePoint3d.h"

/// @brief Per-mode scratch state for Nodal mode.
///
/// Owns:
///   * m_previousCommand        — the active gesture op (ID_OP3 area, ID_OP4 move,
///                                ID_OP5 copy, ID_OP6 to-line, ID_OP7 to-polygon),
///                                or 0 when idle.
///   * m_previousCursorPosition — the first-click anchor for two-click gestures.
///   * m_points                 — the legacy `pts` accumulator previously owned by
///                                AeSysView; used by point/line/area selection (as a
///                                temporary scratch buffer for `GetAllPoints`) and by
///                                move/copy/to-polygon (as the persistent gesture
///                                anchor list).
class NodalModeState final : public AeSysState {
 public:
  void OnExit(AeSysView* context) override;
  void OnMouseMove(AeSysView* context, UINT nFlags, CPoint point) override;
  bool OnReturn(AeSysView* context) override;
  bool OnEscape(AeSysView* context) override;
  void OnRButtonUp(AeSysView* context, UINT flags, CPoint point) override;
  [[nodiscard]] UINT GetActiveOp() const noexcept override { return m_previousCommand; }
  [[nodiscard]] bool HandleCommand(AeSysView* context, UINT command) override;
  [[nodiscard]] const wchar_t* PromptString() const noexcept override;
  [[nodiscard]] const wchar_t* ModeLabel() const noexcept override { return L"Nodal"; }
  [[nodiscard]] bool BuildContextMenu(AeSysView* context, CMenu& menu) override;

  void UnhighlightOp(AeSysView* context);

  [[nodiscard]] std::uint16_t PreviousCommand() const noexcept { return m_previousCommand; }
  [[nodiscard]] std::uint16_t& PreviousCommandRef() noexcept { return m_previousCommand; }
  void SetPreviousCommand(std::uint16_t command) noexcept { m_previousCommand = command; }

  [[nodiscard]] const EoGePoint3d& PreviousCursorPosition() const noexcept { return m_previousCursorPosition; }
  [[nodiscard]] EoGePoint3d& PreviousCursorPositionRef() noexcept { return m_previousCursorPosition; }
  void SetPreviousCursorPosition(const EoGePoint3d& position) noexcept { m_previousCursorPosition = position; }

  [[nodiscard]] EoGePoint3dArray& Points() noexcept { return m_points; }

 private:
  std::uint16_t m_previousCommand{};
  EoGePoint3d m_previousCursorPosition{};
  EoGePoint3dArray m_points{};
};
