#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"
#include "PickAndDragState.h"
#include "Resource.h"

void PickAndDragState::OnEnter(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"PickAndDragState::OnEnter (kind=%d)\n", static_cast<int>(m_kind));
  m_subModeEditBeginPoint = context->GetCursorPosition();
  m_tmEditSeg.Identity();

  const int modeId = (m_kind == Kind::Primitive) ? ID_MODE_PRIMITIVE_EDIT : ID_MODE_GROUP_EDIT;

  auto* group = context->SelectGroupAndPrimitive(m_subModeEditBeginPoint);
  if (group == nullptr) {
    // Nothing under cursor — pop ourselves immediately by scheduling a pop.
    // We cannot call PopState here (we are inside PushState→OnEnter), so mark
    // the group pointers null; the caller (OnModePrimitiveEdit/OnModeGroupEdit)
    // checks and does not push if group == nullptr.  This branch is unreachable
    // if callers guard correctly, but kept for safety.
    return;
  }
  m_subModeEditGroup = group;
  if (m_kind == Kind::Primitive) {
    m_subModeEditPrimitive = context->EngagedPrimitive();
  }
  app.SetModeResourceIdentifier(IDR_PICK_AND_DRAG_MODE);
  app.LoadModeResources(modeId);
}

void PickAndDragState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"PickAndDragState::OnExit\n");
  context->InitializeGroupAndPrimitiveEdit();
  // Restore the primary mode UI. If a primary-mode state (e.g. DrawModeState) is below
  // on the stack, its OnEnter will immediately override this — that is intentional.
  app.SetModeResourceIdentifier(app.PrimaryModeResourceIdentifier());
  app.LoadModeResources(app.PrimaryMode());
}

void PickAndDragState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  if (m_kind == Kind::Primitive) {
    context->PreviewPrimitiveEdit();
  } else {
    context->PreviewGroupEdit();
  }
}
