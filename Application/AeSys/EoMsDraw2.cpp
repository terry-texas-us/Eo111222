#include "Stdafx.h"

#include "AeSysView.h"
#include "EoMsDraw2.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGsRenderState.h"
#include "Resource.h"

void Draw2ModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"Draw2ModeState::OnExit\n");
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
  context->ModeLineUnhighlightOp(m_previousOp);
}

void Draw2ModeState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  if (m_previousOp == 0) { return; }  // No gesture in progress — nothing to preview.

  const EoDbHandleSuppressionScope suppressHandles;
  auto cursorPosition = context->GetCursorPosition();
  cursorPosition = context->SnapPointToAxis(m_previousPoint, cursorPosition);

  EoGeLine previewLines[2]{};
  EoGeLine line(m_previousPoint, cursorPosition);
  if (!line.GetParallels(context->DistanceBetweenLines(), context->CenterLineEccentricity(), previewLines[0], previewLines[1])) {
    return;
  }

  context->PreviewGroup().DeletePrimitivesAndRemoveAll();

  const EoGeLine beginCap{previewLines[0].begin, previewLines[1].begin};
  context->PreviewGroup().AddTail(EoDbLine::CreateLine(beginCap)->WithProperties(Gs::renderState));
  context->PreviewGroup().AddTail(EoDbLine::CreateLine(previewLines[0])->WithProperties(Gs::renderState));
  context->PreviewGroup().AddTail(EoDbLine::CreateLine(previewLines[1])->WithProperties(Gs::renderState));
  const EoGeLine endCap{previewLines[1].end, previewLines[0].end};
  context->PreviewGroup().AddTail(EoDbLine::CreateLine(endCap)->WithProperties(Gs::renderState));

  context->InvalidateOverlay();
}

bool Draw2ModeState::OnReturn(AeSysView* context) {
  context->OnDraw2ModeReturn();
  return true;
}

bool Draw2ModeState::OnEscape(AeSysView* context) {
  context->OnDraw2ModeEscape();
  return true;
}

void Draw2ModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  OnReturn(context);
}

bool Draw2ModeState::BuildContextMenu([[maybe_unused]] AeSysView* context, CMenu& menu) {
  if (m_previousOp == 0) { return false; }
  menu.AppendMenu(MF_STRING, ID_DRAW2_MODE_RETURN, L"&Commit Segment\tEnter");
  menu.AppendMenu(MF_SEPARATOR);
  menu.AppendMenu(MF_STRING, ID_DRAW2_MODE_ESCAPE, L"C&ancel\tEsc");
  return true;
}

bool Draw2ModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToDraw2Command[] = {
      0,                  // ID_OP0
      ID_DRAW2_MODE_JOIN, // ID_OP1
      ID_DRAW2_MODE_WALL, // ID_OP2
      0, 0, 0, 0, 0, 0, 0 // ID_OP3..ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToDraw2Command[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToDraw2Command[opIndex]);
    return true;
  }
  return false;
}

void Draw2ModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}

const wchar_t* Draw2ModeState::PromptString() const noexcept {
  switch (m_previousOp) {
    case ID_DRAW2_MODE_WALL: return L"Specify next wall segment end point";
    case ID_DRAW2_MODE_JOIN: return L"Select wall to join";
    default:                 return L"Draw2 (parallel wall) -- choose sub-command";
  }
}

