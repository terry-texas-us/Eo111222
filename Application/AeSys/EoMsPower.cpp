#include "Stdafx.h"

#include "AeSysView.h"
#include "EoDbConic.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoGsRenderState.h"
#include "EoMsPower.h"
#include "Resource.h"

void PowerModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"PowerModeState::OnExit\n");
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
  context->ModeLineUnhighlightOp(m_previousOp);
}

void PowerModeState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  if (m_previousOp != ID_OP2) { return; }
  if (m_points.IsEmpty() || m_points[0] == context->GetCursorPosition()) { return; }

  const EoDbHandleSuppressionScope suppressHandles;
  auto cursorPosition = context->GetCursorPosition();

  context->PreviewGroup().DeletePrimitivesAndRemoveAll();

  EoDbConic* circle{};
  const auto* group = context->SelectCircleUsingPoint(cursorPosition, 0.02, circle);
  if (group != nullptr) {
    const double radius = circle->Radius();
    cursorPosition = circle->Center();
    cursorPosition = cursorPosition.ProjectToward(m_points[0], radius);
  } else {
    cursorPosition = context->SnapPointToAxis(m_points[0], cursorPosition);
  }
  const auto pt1 = m_points[0].ProjectToward(cursorPosition, m_previousRadius);
  context->PreviewGroup().AddTail(EoDbLine::CreateLine(pt1, cursorPosition)->WithProperties(Gs::renderState));
  context->InvalidateOverlay();
}

bool PowerModeState::OnReturn(AeSysView* context) {
  context->OnPowerModeReturn();
  return true;
}

bool PowerModeState::OnEscape(AeSysView* context) {
  context->OnPowerModeEscape();
  return true;
}

void PowerModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}

void PowerModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  // RMB commits the current conductor run (same as Enter).
  OnReturn(context);
}

bool PowerModeState::BuildContextMenu([[maybe_unused]] AeSysView* context, CMenu& menu) {
  if (m_previousOp == 0) { return false; }
  menu.AppendMenu(MF_STRING, ID_POWER_MODE_RETURN, L"&Commit Run\tEnter");
  menu.AppendMenu(MF_SEPARATOR);
  menu.AppendMenu(MF_STRING, ID_POWER_MODE_ESCAPE, L"C&ancel\tEsc");
  return true;
}

bool PowerModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToPowerCommand[] = {
      0,                     // ID_OP0
      0,                     // ID_OP1
      ID_POWER_MODE_CIRCUIT, // ID_OP2
      0,                     // ID_OP3
      ID_POWER_MODE_GROUND,  // ID_OP4
      ID_POWER_MODE_HOT,     // ID_OP5
      ID_POWER_MODE_SWITCH,  // ID_OP6
      ID_POWER_MODE_NEUTRAL, // ID_OP7
      ID_POWER_MODE_HOME,    // ID_OP8
      0,                     // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToPowerCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToPowerCommand[opIndex]);
    return true;
  }
  return false;
}

