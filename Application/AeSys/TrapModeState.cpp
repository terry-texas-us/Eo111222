#include "Stdafx.h"

#include "AeSysView.h"
#include "TrapModeState.h"

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
