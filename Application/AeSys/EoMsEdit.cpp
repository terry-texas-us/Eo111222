#include "Stdafx.h"

#include "AeSysView.h"
#include "EoMsEdit.h"
#include "Resource.h"

void EditModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"EditModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousOp);
  context->RubberBandingDisable();
}

bool EditModeState::OnReturn(AeSysView* context) {
  // Commit (or abandon) any in-progress two-click move/copy gesture.
  if (m_previousOp == ID_OP4 || m_previousOp == ID_OP5) {
    context->RubberBandingDisable();
    UnhighlightOp(context);
    return true;
  }
  return false;
}

bool EditModeState::OnEscape(AeSysView* context) {
  // Cancel any in-progress two-click move/copy gesture.
  if (m_previousOp == ID_OP4 || m_previousOp == ID_OP5) {
    context->RubberBandingDisable();
    UnhighlightOp(context);
    return true;
  }
  return false;
}

void EditModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}

void EditModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  OnEscape(context);
}

bool EditModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToEditCommand[] = {
      0,                    // ID_OP0
      ID_EDIT_MODE_PIVOT,   // ID_OP1
      ID_EDIT_MODE_ROTCCW,  // ID_OP2
      ID_EDIT_MODE_ROTCW,   // ID_OP3
      ID_EDIT_MODE_MOVE,    // ID_OP4
      ID_EDIT_MODE_COPY,    // ID_OP5
      ID_EDIT_MODE_FLIP,    // ID_OP6
      ID_EDIT_MODE_REDUCE,  // ID_OP7
      ID_EDIT_MODE_ENLARGE, // ID_OP8
      0,                    // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToEditCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToEditCommand[opIndex]);
    return true;
  }
  return false;
}
