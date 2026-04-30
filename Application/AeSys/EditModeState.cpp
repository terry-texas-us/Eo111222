#include "Stdafx.h"

#include "AeSysView.h"
#include "EditModeState.h"

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
