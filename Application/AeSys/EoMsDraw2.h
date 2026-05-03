#pragma once
#include <cstdint>
#include "AeSysState.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"

class AeSysView;
class EoDbGroup;
class EoDbLine;

/// @brief Mode state for Draw2 (parallel-wall) mode.
/// Owns the per-mode interaction state previously stored on AeSysView:
///   * m_previousOp                — the active gesture op (ID_OP1 join, ID_OP2 wall).
///   * m_previousPoint             — anchor point of the current wall segment.
///   * m_continuingCorner          — true when chaining wall segments to clean a corner.
///   * m_currentReferenceLine, m_previousReferenceLine,
///     m_currentLeftLine, m_currentRightLine
///                                 — geometry for parallel/corner construction.
///   * m_assemblyGroup, m_beginSectionGroup, m_endSectionGroup,
///     m_beginSectionLinePrimitive, m_endSectionLinePrimitive
///                                 — gesture references into the document.
/// View-owned configuration (m_distanceBetweenLines, m_centerLineEccentricity)
/// intentionally stays on AeSysView; it survives mode switches and is shared
/// with EoDlgSetLength via OnDraw2ModeOptions.
class Draw2ModeState : public AeSysState {
 public:
  Draw2ModeState() = default;
  Draw2ModeState(const Draw2ModeState&) = delete;
  Draw2ModeState& operator=(const Draw2ModeState&) = delete;
  Draw2ModeState(Draw2ModeState&&) = delete;
  Draw2ModeState& operator=(Draw2ModeState&&) = delete;

  void OnExit(AeSysView* context) override;
  void OnRButtonUp(AeSysView* context, UINT nFlags, CPoint point) override;
  void OnMouseMove(AeSysView* context, UINT nFlags, CPoint point) override;
  bool OnReturn(AeSysView* context) override;
  bool OnEscape(AeSysView* context) override;
  [[nodiscard]] UINT GetActiveOp() const noexcept override { return m_previousOp; }
  [[nodiscard]] bool HandleCommand(AeSysView* context, UINT command) override;
  [[nodiscard]] const wchar_t* PromptString() const noexcept override;
  [[nodiscard]] const wchar_t* ModeLabel() const noexcept override { return L"Draw2"; }
  [[nodiscard]] bool BuildContextMenu(AeSysView* context, CMenu& menu) override;

  void UnhighlightOp(AeSysView* context);

  [[nodiscard]] std::uint16_t PreviousOp() const noexcept { return m_previousOp; }
  [[nodiscard]] std::uint16_t& PreviousOpRef() noexcept { return m_previousOp; }
  void SetPreviousOp(std::uint16_t op) noexcept { m_previousOp = op; }

  [[nodiscard]] EoGePoint3d& PreviousPointRef() noexcept { return m_previousPoint; }
  [[nodiscard]] const EoGePoint3d& PreviousPoint() const noexcept { return m_previousPoint; }
  void SetPreviousPoint(const EoGePoint3d& point) noexcept { m_previousPoint = point; }

  [[nodiscard]] bool& ContinuingCornerRef() noexcept { return m_continuingCorner; }
  [[nodiscard]] bool ContinuingCorner() const noexcept { return m_continuingCorner; }

  [[nodiscard]] EoGeLine& CurrentReferenceLineRef() noexcept { return m_currentReferenceLine; }
  [[nodiscard]] EoGeLine& PreviousReferenceLineRef() noexcept { return m_previousReferenceLine; }
  [[nodiscard]] EoGeLine& CurrentLeftLineRef() noexcept { return m_currentLeftLine; }
  [[nodiscard]] EoGeLine& CurrentRightLineRef() noexcept { return m_currentRightLine; }

  [[nodiscard]] EoDbGroup*& AssemblyGroupRef() noexcept { return m_assemblyGroup; }
  [[nodiscard]] EoDbGroup*& BeginSectionGroupRef() noexcept { return m_beginSectionGroup; }
  [[nodiscard]] EoDbGroup*& EndSectionGroupRef() noexcept { return m_endSectionGroup; }
  [[nodiscard]] EoDbLine*& BeginSectionLinePrimitiveRef() noexcept { return m_beginSectionLinePrimitive; }
  [[nodiscard]] EoDbLine*& EndSectionLinePrimitiveRef() noexcept { return m_endSectionLinePrimitive; }

 private:
  std::uint16_t m_previousOp{};
  EoGePoint3d m_previousPoint{};
  bool m_continuingCorner{};
  EoGeLine m_currentReferenceLine{};
  EoGeLine m_previousReferenceLine{};
  EoGeLine m_currentLeftLine{};
  EoGeLine m_currentRightLine{};
  EoDbGroup* m_assemblyGroup{};
  EoDbGroup* m_beginSectionGroup{};
  EoDbGroup* m_endSectionGroup{};
  EoDbLine* m_beginSectionLinePrimitive{};
  EoDbLine* m_endSectionLinePrimitive{};
};
