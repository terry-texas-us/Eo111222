#include "Stdafx.h"
#if defined(USING_STATE_PATTERN)
#include "AeSysDoc.h"  // If needed for document access
#include "AeSysView.h"
#include "EoGePoint3d.h"
#include "IdleState.h"

void IdleState::OnEnter(AeSysView* context) {
  // Setup: e.g., set default cursor (IDC_CROSS), clear previews
  context->SetModeCursor(0);
  context->m_PreviewGroup.DeletePrimitivesAndRemoveAll();  // Clear any leftovers
                                                           // ModeLineDisplay("Idle Mode");  // If you have mode line UI
}

void IdleState::OnExit([[maybe_unused]] AeSysView* context) {
  // Cleanup: Minimal for idle
}

void IdleState::OnMouseMove(
    [[maybe_unused]] AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  // Basic handling: e.g., update odometer/position without drawing
  // Existing logic from AeSysView::OnMouseMove (non-mode parts)
  EoGePoint3d cursorPosition = context->GetCursorPosition();
  // UpdateOdometer(cursorPosition);  // If applicable
}

void IdleState::OnDraw(AeSysView* context, CDC* deviceContext) {
  // Idle state also needs to display document content
  auto* document = context->GetDocument();
  document->DisplayAllLayers(context, deviceContext);
  document->DisplayUniquePoints();
}

bool IdleState::OnUpdate([[maybe_unused]] AeSysView* context, [[maybe_unused]] CView* sender, [[maybe_unused]] LPARAM hint,
    [[maybe_unused]] CObject* objectHint) {
  // Idle-specific if any (e.g., no previews)
  return false;  // Fallback to AeSysView switch for drawing
}
#endif