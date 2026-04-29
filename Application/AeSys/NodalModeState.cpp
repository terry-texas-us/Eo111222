#include "Stdafx.h"

#include "AeSysView.h"
#include "NodalModeState.h"

void NodalModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"NodalModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousCommand);
  context->RubberBandingDisable();
  m_points.RemoveAll();
}

void NodalModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousCommand);
}
