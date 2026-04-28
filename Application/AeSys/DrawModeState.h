#pragma once
#include <cstdint>
#include "AeSysState.h"

class AeSysView;

/// @brief Mode state for Draw mode (Phases 2A–2C).
/// Phase 2C: owns m_previousDrawCommand — the active draw sub-command op (e.g. ID_OP2
/// for line, ID_OP3 for polygon). All draw handlers access it through DrawModeState*
/// rather than the former file-local static in AeSysViewDrawMode.cpp.
/// pts remains on AeSysView (shared by other modes not yet migrated).
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

 private:
  std::uint16_t m_previousDrawCommand{};
};
