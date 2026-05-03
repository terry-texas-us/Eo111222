#include "Stdafx.h"

#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "EoGsRenderState.h"
#include "EoPipeGeometry.h"
#include "EoMsPipe.h"
#include "Resource.h"

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

void PipeModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  // RMB commits the current segment (same as Enter) — pipe runs are chain-committed.
  OnReturn(context);
}

bool PipeModeState::BuildContextMenu([[maybe_unused]] AeSysView* context, CMenu& menu) {
  if (m_previousOp == 0) { return false; }
  menu.AppendMenu(MF_STRING, ID_PIPE_MODE_RETURN, L"&Commit Segment\tEnter");
  menu.AppendMenu(MF_SEPARATOR);
  menu.AppendMenu(MF_STRING, ID_PIPE_MODE_ESCAPE, L"C&ancel\tEsc");
  return true;
}

bool PipeModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToPipeCommand[] = {
      0,                    // ID_OP0
      0,                    // ID_OP1
      ID_PIPE_MODE_LINE,    // ID_OP2
      ID_PIPE_MODE_FITTING, // ID_OP3
      ID_PIPE_MODE_DROP,    // ID_OP4
      ID_PIPE_MODE_RISE,    // ID_OP5
      0,                    // ID_OP6
      0,                    // ID_OP7
      ID_PIPE_MODE_SYMBOL,  // ID_OP8
      ID_PIPE_MODE_WYE,     // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToPipeCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToPipeCommand[opIndex]);
    return true;
  }
  return false;
}

const wchar_t* PipeModeState::PromptString() const noexcept {
  switch (m_previousOp) {
    case ID_PIPE_MODE_LINE:    return L"Specify next pipe segment end point";
    case ID_PIPE_MODE_FITTING: return L"Specify fitting location";
    case ID_PIPE_MODE_DROP:    return L"Specify drop location";
    case ID_PIPE_MODE_RISE:    return L"Specify rise location";
    case ID_PIPE_MODE_WYE:     return L"Specify wye location";
    default:                   return L"Pipe -- choose sub-command (LINE FITTING DROP...)";
  }
}

