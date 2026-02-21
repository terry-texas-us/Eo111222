#include "Stdafx.h"
#if defined(USING_STATE_PATTERN)
#include "AeSysDoc.h"  // If needed for document access
#include "AeSysView.h"
#include "EoGePoint3d.h"
#include "IdleState.h"

void IdleState::OnEnter(AeSysView* context) {
  // Setup: e.g., set default cursor (IDC_CROSS), clear previews
  ATLTRACE2(traceGeneral, 2, L"IdleState::OnEnter\n");
  context->SetModeCursor(0);
  context->m_PreviewGroup.DeletePrimitivesAndRemoveAll();  // Clear any leftovers
                                                           // ModeLineDisplay("Idle Mode");  // If you have mode line UI
}

void IdleState::OnExit([[maybe_unused]] AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"IdleState::OnExit\n");
  // Cleanup: Minimal for idle
}

void IdleState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  ATLTRACE2(traceGeneral, 3, L"IdleState::OnMouseMove - flags: %u, point: (%d, %d)\n", flags, point.x, point.y);
  // Basic handling: e.g., update odometer/position without drawing
  // Existing logic from AeSysView::OnMouseMove (non-mode parts)
  EoGePoint3d cursorPosition = context->GetCursorPosition();
  // UpdateOdometer(cursorPosition);  // If applicable
}

void IdleState::OnDraw(AeSysView* context, CDC* deviceContext) {
  ATLTRACE2(traceGeneral, 2, L"IdleState::OnDraw\n");
  // Idle state also needs to display document content
  auto* document = context->GetDocument();
  document->DisplayAllLayers(context, deviceContext);
  document->DisplayUniquePoints();
}

bool IdleState::OnUpdate([[maybe_unused]] AeSysView* context, [[maybe_unused]] CView* sender,
    [[maybe_unused]] LPARAM hint, [[maybe_unused]] CObject* objectHint) {
  ATLTRACE2(traceGeneral, 2, L"IdleState::OnUpdate\n");
  // Idle-specific if any (e.g., no previews)
  return false;  // Fallback to AeSysView switch for drawing
}
#endif