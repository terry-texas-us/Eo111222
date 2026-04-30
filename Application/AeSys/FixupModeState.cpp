#include "Stdafx.h"

#include "AeSysView.h"
#include "FixupModeState.h"

void FixupModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"FixupModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousCommand);
  context->RubberBandingDisable();
}

bool FixupModeState::OnReturn(AeSysView* context) {
  context->OnFixupModeReturn();
  return true;
}

bool FixupModeState::OnEscape(AeSysView* context) {
  context->OnFixupModeEscape();
  return true;
}

void FixupModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousCommand);
}
