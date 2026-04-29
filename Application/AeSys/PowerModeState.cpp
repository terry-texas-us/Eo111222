#include "Stdafx.h"

#include "AeSysView.h"
#include "PowerModeState.h"

void PowerModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"PowerModeState::OnExit\n");
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
  context->ModeLineUnhighlightOp(m_previousOp);
}

void PowerModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}
