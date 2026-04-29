#pragma once
#include <cstdint>
#include "AeSysState.h"
#include "EoGePoint3d.h"

class AeSysView;

/// @brief Mode state for Draw mode (Phases 2A–2C, 2B-1).
/// Phase 2C: owns m_previousDrawCommand — the active draw sub-command op (e.g. ID_OP2
/// for line, ID_OP3 for polygon). All draw handlers access it through DrawModeState*
/// rather than the former file-local static in AeSysViewDrawMode.cpp.
/// Phase 2B-1: owns m_points — the per-mode point accumulator that draw-mode handlers
/// use to collect click points before committing a primitive on Return/click. Other
/// modes (Nodal, Pipe, Draw2) still use the shared AeSysView::pts scratch buffer
/// pending their own state migration (Phase 2B-2+).
class DrawModeState : public AeSysState {
 public:
  DrawModeState() = default;
  DrawModeState(const DrawModeState&) = delete;
  DrawModeState& operator=(const DrawModeState&) = delete;
  DrawModeState(DrawModeState&&) = delete;
  DrawModeState& operator=(DrawModeState&&) = delete;

  void OnEnter(AeSysView* context) override;
  void OnExit(AeSysView* context) override;

  [[nodiscard]] std::uint16_t PreviousDrawCommand() const noexcept { return m_previousDrawCommand; }
  void SetPreviousDrawCommand(std::uint16_t command) noexcept { m_previousDrawCommand = command; }

  [[nodiscard]] EoGePoint3dArray& Points() noexcept { return m_points; }
  [[nodiscard]] const EoGePoint3dArray& Points() const noexcept { return m_points; }

 private:
  std::uint16_t m_previousDrawCommand{};
  EoGePoint3dArray m_points{};
};
