#include "Stdafx.h"

#include "AeSysView.h"
#include "DimensionModeState.h"

void DimensionModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"DimensionModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousCommand);
  context->RubberBandingDisable();
}

bool DimensionModeState::OnReturn(AeSysView* context) {
  context->OnDimensionModeReturn();
  return true;
}

bool DimensionModeState::OnEscape(AeSysView* context) {
  context->OnDimensionModeEscape();
  return true;
}

void DimensionModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousCommand);
}
