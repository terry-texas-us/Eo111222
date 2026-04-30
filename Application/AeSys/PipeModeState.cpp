#include "Stdafx.h"

#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "EoGsRenderState.h"
#include "EoPipeGeometry.h"
#include "PipeModeState.h"

void PipeModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"PipeModeState::OnExit\n");
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
  context->ModeLineUnhighlightOp(m_previousOp);
}

void PipeModeState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  if (m_points.IsEmpty()) { return; }

  const EoDbHandleSuppressionScope suppressHandles;
  auto cursorPosition = context->GetCursorPosition();

  switch (m_previousOp) {
    case ID_OP2:
    case ID_OP3:
      if (m_points[0] == cursorPosition) { return; }
      cursorPosition = context->SnapPointToAxis(m_points[0], cursorPosition);
      break;

    case ID_OP4:
    case ID_OP5:
    case ID_OP9:
      cursorPosition = context->SnapPointToAxis(m_points[0], cursorPosition);
      break;

    default:
      return;
  }

  // End type: plain line (OP2) has no end fitting; all others preview with an elbow/fitting tick (OP3).
  const int endType = (m_previousOp == ID_OP2) ? ID_OP2 : ID_OP3;
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  Pipe::GenerateLineWithFittings(m_previousOp, m_points[0], endType, cursorPosition,
      context->PipeRiseDropRadius(), context->PipeTicSize(), &context->PreviewGroup());
  context->InvalidateOverlay();
}

bool PipeModeState::OnReturn(AeSysView* context) {
  context->OnPipeModeReturn();
  return true;
}

bool PipeModeState::OnEscape(AeSysView* context) {
  context->OnPipeModeEscape();
  return true;
}

void PipeModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}

