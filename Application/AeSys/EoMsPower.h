#pragma once
#include <cstdint>
#include "AeSysState.h"
#include "EoGePoint3d.h"

class AeSysView;

/// @brief Mode state for Power mode.
/// Owns the per-mode interaction state previously stored on AeSysView and
/// in two function-local statics:
///   * m_previousOp           — the active gesture op (ID_OP2 circuit, etc.).
///   * m_points               — point accumulator for multi-click circuit gestures.
///   * m_powerArrow           — set when home-run arrow geometry is staged.
///   * m_powerConductor       — set when a conductor stamp is staged.
///   * m_circuitEndPoint      — anchor end point on the circuit being followed.
///   * m_previousRadius       — last circle radius used to project line endpoints.
///   * m_pointOnCircuitHome   — replaces the static in OnPowerModeHome.
///   * m_pointOnCircuitConductor — replaces the static in DoPowerModeConductor.
/// View-owned configuration (m_PowerConductorSpacing) stays on AeSysView.
class PowerModeState : public AeSysState {
 public:
  PowerModeState() = default;
  PowerModeState(const PowerModeState&) = delete;
  PowerModeState& operator=(const PowerModeState&) = delete;
  PowerModeState(PowerModeState&&) = delete;
  PowerModeState& operator=(PowerModeState&&) = delete;

  void OnExit(AeSysView* context) override;
  void OnMouseMove(AeSysView* context, UINT nFlags, CPoint point) override;
  bool OnReturn(AeSysView* context) override;
  bool OnEscape(AeSysView* context) override;
  void OnRButtonUp(AeSysView* context, UINT flags, CPoint point) override;
  [[nodiscard]] UINT GetActiveOp() const noexcept override { return m_previousOp; }
  [[nodiscard]] bool HandleCommand(AeSysView* context, UINT command) override;
  [[nodiscard]] const wchar_t* PromptString() const noexcept override;
  [[nodiscard]] const wchar_t* ModeLabel() const noexcept override { return L"Power"; }
  [[nodiscard]] bool BuildContextMenu(AeSysView* context, CMenu& menu) override;

  void UnhighlightOp(AeSysView* context);

  [[nodiscard]] std::uint16_t PreviousOp() const noexcept { return m_previousOp; }
  [[nodiscard]] std::uint16_t& PreviousOpRef() noexcept { return m_previousOp; }
  void SetPreviousOp(std::uint16_t op) noexcept { m_previousOp = op; }

  [[nodiscard]] EoGePoint3dArray& Points() noexcept { return m_points; }
  [[nodiscard]] const EoGePoint3dArray& Points() const noexcept { return m_points; }

  [[nodiscard]] bool& PowerArrowRef() noexcept { return m_powerArrow; }
  [[nodiscard]] bool& PowerConductorRef() noexcept { return m_powerConductor; }
  [[nodiscard]] EoGePoint3d& CircuitEndPointRef() noexcept { return m_circuitEndPoint; }
  [[nodiscard]] double& PreviousRadiusRef() noexcept { return m_previousRadius; }
  [[nodiscard]] EoGePoint3d& PointOnCircuitHomeRef() noexcept { return m_pointOnCircuitHome; }
  [[nodiscard]] EoGePoint3d& PointOnCircuitConductorRef() noexcept { return m_pointOnCircuitConductor; }

 private:
  std::uint16_t m_previousOp{};
  EoGePoint3dArray m_points{};
  bool m_powerArrow{};
  bool m_powerConductor{};
  EoGePoint3d m_circuitEndPoint{};
  double m_previousRadius{};
  EoGePoint3d m_pointOnCircuitHome{};
  EoGePoint3d m_pointOnCircuitConductor{};
};
