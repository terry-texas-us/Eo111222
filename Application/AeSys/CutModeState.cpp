#include "Stdafx.h"

#include "AeSysView.h"
#include "CutModeState.h"

void CutModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"CutModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousOp);
  context->RubberBandingDisable();
}

bool CutModeState::OnReturn(AeSysView* context) {
  context->OnCutModeReturn();
  return true;
}

bool CutModeState::OnEscape(AeSysView* context) {
  context->OnCutModeEscape();
  return true;
}

void CutModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}
