#include "Stdafx.h"

#include "AeSysView.h"
#include "EoMsDimension.h"
#include "Resource.h"

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

void DimensionModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  OnEscape(context);
}

bool DimensionModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToDimensionCommand[] = {
      0,                          // ID_OP0
      ID_DIMENSION_MODE_ARROW,    // ID_OP1
      ID_DIMENSION_MODE_LINE,     // ID_OP2
      ID_DIMENSION_MODE_DLINE,    // ID_OP3
      ID_DIMENSION_MODE_DLINE2,   // ID_OP4
      ID_DIMENSION_MODE_EXTEN,    // ID_OP5
      ID_DIMENSION_MODE_RADIUS,   // ID_OP6
      ID_DIMENSION_MODE_DIAMETER, // ID_OP7
      ID_DIMENSION_MODE_ANGLE,    // ID_OP8
      ID_DIMENSION_MODE_CONVERT,  // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToDimensionCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToDimensionCommand[opIndex]);
    return true;
  }
  return false;
}
