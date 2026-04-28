#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"
#include "DrawModeState.h"

void DrawModeState::OnEnter(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"DrawModeState::OnEnter\n");
  m_previousDrawCommand = 0;
  // Restore full draw-mode UI when re-entered after a sub-mode pop (e.g. PickAndDragState).
  app.SetModeResourceIdentifier(IDR_DRAW_MODE);
  app.LoadModeResources(ID_MODE_DRAW, context);
}

void DrawModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"DrawModeState::OnExit\n");
  // Single cleanup point for all draw-mode exit paths.
  context->ModeLineUnhighlightOp(m_previousDrawCommand);
  m_previousDrawCommand = 0;
  context->ClearPoints();
  context->RubberBandingDisable();
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
}
