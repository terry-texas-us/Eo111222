#include "Stdafx.h"

#include "AeSysView.h"
#include "Draw2ModeState.h"

void Draw2ModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"Draw2ModeState::OnExit\n");
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
  context->ModeLineUnhighlightOp(m_previousOp);
}

void Draw2ModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}
