#include "Stdafx.h"

#include "AeSysView.h"
#include "TrapModeState.h"

void TrapModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"TrapModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousOp);
  context->RubberBandingDisable();
}

void TrapModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}
