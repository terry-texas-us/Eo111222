#pragma once
#include <cstdint>
#include "AeSysState.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "Section.h"

class AeSysView;
class EoDbGroup;
class EoDbPoint;

/// @brief Mode state for Low Pressure Duct (rectangular) mode.
/// Owns the per-mode interaction state previously stored on AeSysView:
///   * m_previousOp                       — active gesture op (ID_OP2 means a duct
///                                          sequence is in progress).
///   * m_previousPoint                    — anchor for the next section / preview.
///   * m_previousSection / m_currentSection — duct cross-sections being run.
///   * m_previousReferenceLine /
///     m_currentReferenceLine             — center-line geometry for the run.
///   * m_continueSection                  — true while an open section is awaiting
///                                          continuation (elbow / transition).
///   * m_originalPreviousGroup /
///     m_originalPreviousGroupDisplayed   — last committed section, used to redraw
///                                          when the preview supersedes it.
///   * m_endCapGroup / m_endCapPoint /
///     m_endCapLocation                   — selection state from "Join".
///
/// View-owned configuration (DuctSeamSize, DuctTapSize, GenerateTurningVanes,
/// DuctJustification, TransitionSlope, InsideRadiusFactor, ElbowType,
/// BeginWithTransition, centerLineEccentricity) intentionally stays on AeSysView;
/// it survives mode switches and is shared with EoDlgLowPressureDuctOptions.
class LpdModeState : public AeSysState {
 public:
  LpdModeState() = default;
  LpdModeState(const LpdModeState&) = delete;
  LpdModeState& operator=(const LpdModeState&) = delete;
  LpdModeState(LpdModeState&&) = delete;
  LpdModeState& operator=(LpdModeState&&) = delete;

  void OnExit(AeSysView* context) override;
  void OnRButtonUp(AeSysView* context, UINT nFlags, CPoint point) override;
  void OnMouseMove(AeSysView* context, UINT nFlags, CPoint point) override;
  bool OnReturn(AeSysView* context) override;
  bool OnEscape(AeSysView* context) override;
  [[nodiscard]] UINT GetActiveOp() const noexcept override { return m_previousOp; }
  [[nodiscard]] const wchar_t* PromptString() const noexcept override;
  [[nodiscard]] const wchar_t* ModeLabel() const noexcept override { return L"Duct"; }
  [[nodiscard]] bool HandleCommand(AeSysView* context, UINT command) override;
  [[nodiscard]] bool BuildContextMenu(AeSysView* context, CMenu& menu) override;

  /// Unhighlights the active op pane (if any) and resets sequence state to idle.
  /// Safe to call repeatedly — clears preview, restores any hidden original
  /// section, nulls stale selection pointers, and disables rubber-banding.
  void ResetSequence(AeSysView* context);

  [[nodiscard]] std::uint16_t PreviousOp() const noexcept { return m_previousOp; }
  [[nodiscard]] std::uint16_t& PreviousOpRef() noexcept { return m_previousOp; }
  void SetPreviousOp(std::uint16_t op) noexcept { m_previousOp = op; }

  [[nodiscard]] const EoGePoint3d& PreviousPoint() const noexcept { return m_previousPoint; }
  [[nodiscard]] EoGePoint3d& PreviousPointRef() noexcept { return m_previousPoint; }
  void SetPreviousPoint(const EoGePoint3d& point) noexcept { m_previousPoint = point; }

  [[nodiscard]] Section& PreviousSectionRef() noexcept { return m_previousSection; }
  [[nodiscard]] Section& CurrentSectionRef() noexcept { return m_currentSection; }

  [[nodiscard]] EoGeLine& PreviousReferenceLineRef() noexcept { return m_previousReferenceLine; }
  [[nodiscard]] EoGeLine& CurrentReferenceLineRef() noexcept { return m_currentReferenceLine; }

  [[nodiscard]] bool ContinueSection() const noexcept { return m_continueSection; }
  void SetContinueSection(bool value) noexcept { m_continueSection = value; }

  [[nodiscard]] bool OriginalPreviousGroupDisplayed() const noexcept {
    return m_originalPreviousGroupDisplayed;
  }
  void SetOriginalPreviousGroupDisplayed(bool value) noexcept {
    m_originalPreviousGroupDisplayed = value;
  }
  [[nodiscard]] EoDbGroup* OriginalPreviousGroup() const noexcept { return m_originalPreviousGroup; }
  void SetOriginalPreviousGroup(EoDbGroup* group) noexcept { m_originalPreviousGroup = group; }

  [[nodiscard]] EoDbGroup* EndCapGroup() const noexcept { return m_endCapGroup; }
  void SetEndCapGroup(EoDbGroup* group) noexcept { m_endCapGroup = group; }

  [[nodiscard]] EoDbPoint* EndCapPoint() const noexcept { return m_endCapPoint; }
  [[nodiscard]] EoDbPoint*& EndCapPointRef() noexcept { return m_endCapPoint; }
  void SetEndCapPoint(EoDbPoint* point) noexcept { m_endCapPoint = point; }

  [[nodiscard]] int EndCapLocation() const noexcept { return m_endCapLocation; }
  void SetEndCapLocation(int value) noexcept { m_endCapLocation = value; }

 private:
  std::uint16_t m_previousOp{0};
  EoGePoint3d m_previousPoint{};

  Section m_previousSection{0.125, 0.0625, Section::Rectangular};
  Section m_currentSection{0.125, 0.0625, Section::Rectangular};

  EoGeLine m_previousReferenceLine{};
  EoGeLine m_currentReferenceLine{};

  bool m_continueSection{false};
  bool m_originalPreviousGroupDisplayed{true};
  EoDbGroup* m_originalPreviousGroup{nullptr};

  EoDbGroup* m_endCapGroup{nullptr};
  EoDbPoint* m_endCapPoint{nullptr};
  int m_endCapLocation{0};
};
