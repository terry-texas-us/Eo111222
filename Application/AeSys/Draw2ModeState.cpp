#include "Stdafx.h"

#include "AeSysView.h"
#include "Draw2ModeState.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGsRenderState.h"

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

void Draw2ModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}

