#include "Stdafx.h"

#include "AeSysView.h"
#include "DimensionModeState.h"

void DimensionModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"DimensionModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousCommand);
  context->RubberBandingDisable();
}

void DimensionModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousCommand);
}
