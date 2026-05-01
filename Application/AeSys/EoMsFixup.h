#pragma once

#include "AeSysState.h"
#include "EoGeLine.h"

class EoDbGroup;
class EoDbPrimitive;

/// @brief Per-mode scratch state for Fixup mode (reference/mend/chamfer/fillet/square/parallel).
///
/// Owns the previously-file-local trio of group/primitive/line slots: previous (last
/// committed selection), reference (anchor for parallel/relative gestures), and current
/// (current selection being worked on). The `m_previousCommand` field tracks the active
/// gesture op (ID_OP1 reference, ID_OP2 mend, ID_OP3 chamfer, ID_OP4 fillet).
///
/// Lifetime is tied to Fixup mode: state is pushed on entry and popped by
/// `PopAllModeStates()` when the user switches modes. Return/Escape clear the
/// reference selection and unhighlight the current op without popping.
class FixupModeState final : public AeSysState {
 public:
  void OnExit(AeSysView* context) override;
  bool OnReturn(AeSysView* context) override;
  bool OnEscape(AeSysView* context) override;
  void OnRButtonUp(AeSysView* context, UINT flags, CPoint point) override;
  [[nodiscard]] UINT GetActiveOp() const noexcept override { return m_previousCommand; }
  [[nodiscard]] bool HandleCommand(AeSysView* context, UINT command) override;
  [[nodiscard]] bool BuildContextMenu(AeSysView* context, CMenu& menu) override;

  /// Resets `m_previousCommand` to 0 via reference so `ModeLineUnhighlightOp`
  /// can clear it. Used by `OnFixupModeReturn` / `OnFixupModeEscape`.
  void UnhighlightOp(AeSysView* context);

  [[nodiscard]] std::uint16_t PreviousCommand() const noexcept { return m_previousCommand; }
  [[nodiscard]] std::uint16_t& PreviousCommandRef() noexcept { return m_previousCommand; }
  void SetPreviousCommand(std::uint16_t command) noexcept { m_previousCommand = command; }

  [[nodiscard]] EoDbGroup*& PreviousGroupRef() noexcept { return m_previousGroup; }
  [[nodiscard]] EoDbPrimitive*& PreviousPrimitiveRef() noexcept { return m_previousPrimitive; }
  [[nodiscard]] EoGeLine& PreviousLineRef() noexcept { return m_previousLine; }

  [[nodiscard]] EoDbGroup*& ReferenceGroupRef() noexcept { return m_referenceGroup; }
  [[nodiscard]] EoDbPrimitive*& ReferencePrimitiveRef() noexcept { return m_referencePrimitive; }
  [[nodiscard]] EoGeLine& ReferenceLineRef() noexcept { return m_referenceLine; }

  [[nodiscard]] EoDbGroup*& CurrentGroupRef() noexcept { return m_currentGroup; }
  [[nodiscard]] EoDbPrimitive*& CurrentPrimitiveRef() noexcept { return m_currentPrimitive; }
  [[nodiscard]] EoGeLine& CurrentLineRef() noexcept { return m_currentLine; }

 private:
  std::uint16_t m_previousCommand{};

  EoDbGroup* m_previousGroup{};
  EoDbPrimitive* m_previousPrimitive{};
  EoGeLine m_previousLine{};

  EoDbGroup* m_referenceGroup{};
  EoDbPrimitive* m_referencePrimitive{};
  EoGeLine m_referenceLine{};

  EoDbGroup* m_currentGroup{};
  EoDbPrimitive* m_currentPrimitive{};
  EoGeLine m_currentLine{};
};
