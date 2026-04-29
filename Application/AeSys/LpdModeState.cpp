#include "Stdafx.h"

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "LpdModeState.h"

void LpdModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"LpdModeState::OnExit\n");
  ResetSequence(context);
}

void LpdModeState::ResetSequence(AeSysView* context) {
  if (context == nullptr) { return; }

  // Drop any preview geometry.
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();

  // If the most recently committed section was hidden behind preview, redraw it.
  if (!m_originalPreviousGroupDisplayed && m_originalPreviousGroup != nullptr) {
    auto* document = context->GetDocument();
    if (document != nullptr) {
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_originalPreviousGroup);
    }
  }
  m_originalPreviousGroupDisplayed = true;
  m_originalPreviousGroup = nullptr;

  // Tear down rubber-banding so the next sequence starts clean.
  context->RubberBandingDisable();

  // Unhighlight the active op pane and reset the op id (passed by reference).
  context->ModeLineUnhighlightOp(m_previousOp);
  m_previousOp = 0;

  // Reset sequence flags / selection pointers.
  m_continueSection = false;
  m_endCapGroup = nullptr;
  m_endCapPoint = nullptr;
  m_endCapLocation = 0;
}
