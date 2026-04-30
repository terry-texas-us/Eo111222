#pragma once
#include <cstdint>
#include "AeSysState.h"
#include "EoGePoint3d.h"

class AeSysView;

/// @brief Mode state for Draw mode.
/// Owns per-mode interaction state:
///   * m_previousDrawCommand — active draw sub-command op (e.g. ID_OP2 line, ID_OP3 polygon).
///   * m_points              — point accumulator; draw-mode handlers collect click points
///                             here before committing a primitive on Return/RMB.
class DrawModeState : public AeSysState {
 public:
  DrawModeState() = default;
  DrawModeState(const DrawModeState&) = delete;
  DrawModeState& operator=(const DrawModeState&) = delete;
  DrawModeState(DrawModeState&&) = delete;
  DrawModeState& operator=(DrawModeState&&) = delete;

  void OnEnter(AeSysView* context) override;
  void OnExit(AeSysView* context) override;
  void OnRButtonUp(AeSysView* context, UINT nFlags, CPoint point) override;
  void OnMouseMove(AeSysView* context, UINT nFlags, CPoint point) override;
  bool OnReturn(AeSysView* context) override;
  bool OnEscape(AeSysView* context) override;
  [[nodiscard]] UINT GetActiveOp() const noexcept override { return m_previousDrawCommand; }
  [[nodiscard]] bool HandleCommand(AeSysView* context, UINT command) override;

  [[nodiscard]] std::uint16_t PreviousDrawCommand() const noexcept { return m_previousDrawCommand; }
  void SetPreviousDrawCommand(std::uint16_t command) noexcept { m_previousDrawCommand = command; }

  [[nodiscard]] EoGePoint3dArray& Points() noexcept { return m_points; }
  [[nodiscard]] const EoGePoint3dArray& Points() const noexcept { return m_points; }

 private:
  std::uint16_t m_previousDrawCommand{};
  EoGePoint3dArray m_points{};
};
