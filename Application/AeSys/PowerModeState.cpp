#include "Stdafx.h"

#include "AeSysView.h"
#include "EoDbConic.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoGsRenderState.h"
#include "PowerModeState.h"

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

