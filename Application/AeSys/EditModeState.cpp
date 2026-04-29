#include "Stdafx.h"

#include "AeSysView.h"
#include "EditModeState.h"

void EditModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"EditModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousOp);
  context->RubberBandingDisable();
}

void EditModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}
