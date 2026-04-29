#include "Stdafx.h"

#include "AeSysView.h"
#include "PipeModeState.h"

void PipeModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"PipeModeState::OnExit\n");
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
  context->ModeLineUnhighlightOp(m_previousOp);
}

void PipeModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}
