#include "Stdafx.h"

#include "AeSysView.h"
#include "FixupModeState.h"

void FixupModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"FixupModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousCommand);
  context->RubberBandingDisable();
}

void FixupModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousCommand);
}
