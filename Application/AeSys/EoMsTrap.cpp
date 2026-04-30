#include "Stdafx.h"

#include "AeSysView.h"
#include "Resource.h"
#include "EoMsTrap.h"

void TrapModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"TrapModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousOp);
  context->RubberBandingDisable();
}

bool TrapModeState::OnEscape(AeSysView* context) {
  // Cancel any in-progress two-click stitch/field gesture.
  if (m_previousOp != 0) {
    context->RubberBandingDisable();
    UnhighlightOp(context);
  }
  return true;
}

void TrapModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}

void TrapModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  OnEscape(context);
}

bool TrapModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToTrapCommand[] = {
      0,                   // ID_OP0
      ID_TRAP_MODE_POINT,  // ID_OP1
      ID_TRAP_MODE_STITCH, // ID_OP2
      0,                   // ID_OP3
      ID_TRAP_MODE_FIELD,  // ID_OP4
      ID_TRAP_MODE_LAST,   // ID_OP5
      ID_TRAP_MODE_ENGAGE, // ID_OP6
      0,                   // ID_OP7
      ID_TRAP_MODE_MENU,   // ID_OP8
      ID_TRAP_MODE_MODIFY, // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToTrapCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToTrapCommand[opIndex]);
    return true;
  }
  return false;
}
