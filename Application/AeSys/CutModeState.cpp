#include "Stdafx.h"

#include "AeSysView.h"
#include "CutModeState.h"

void CutModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"CutModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousOp);
  context->RubberBandingDisable();
}

void CutModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}
