#include "Stdafx.h"

#include "AeSysView.h"
#include "AnnotateModeState.h"

void AnnotateModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"AnnotateModeState::OnExit\n");
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
  context->ModeLineUnhighlightOp(m_previousOp);
}

void AnnotateModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}
