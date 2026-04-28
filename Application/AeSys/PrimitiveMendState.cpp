#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "PrimitiveMendState.h"
#include "Resource.h"

PrimitiveMendState::~PrimitiveMendState() {
  // Safety: if the state is destroyed without OnExit (e.g. during doc teardown),
  // free the working copy to avoid a leak.
  DeleteCopy();
}

void PrimitiveMendState::DeleteCopy() noexcept {
  delete m_primitiveToMendCopy;
  m_primitiveToMendCopy = nullptr;
}

void PrimitiveMendState::OnEnter(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"PrimitiveMendState::OnEnter\n");

  const EoGePoint3d cursorPosition = context->GetCursorPosition();
  EoGePoint4d ptView(cursorPosition);
  context->ModelViewTransformPoint(ptView);

  m_primitiveToMend = nullptr;
  m_mendPrimitiveBegin = cursorPosition;

  if (context->GroupIsEngaged()) {
    EoGePoint3d ptDet;
    auto* primitive = context->EngagedPrimitive();
    EoDbPolygon::EdgeToEvaluate() = EoDbPolygon::Edge();
    if (primitive->SelectUsingPoint(context, ptView, ptDet)) {
      m_primitiveToMend = primitive;
    }
  }
  if (m_primitiveToMend == nullptr) {
    if (context->SelectGroupAndPrimitive(m_mendPrimitiveBegin) != nullptr) {
      m_primitiveToMend = context->EngagedPrimitive();
    }
  }

  if (m_primitiveToMend == nullptr) {
    // Nothing to mend — OnModePrimitiveMend will not push if IsEngaged() is false.
    return;
  }

  m_primitiveToMend->Copy(m_primitiveToMendCopy);
  m_mendPrimitiveBegin = m_primitiveToMend->SelectAtControlPoint(context, ptView);
  m_mendPrimitiveVertexIndex = 1U << EoDbPrimitive::ControlPointIndex();

  app.SetModeResourceIdentifier(IDR_MEND_MODE);
  app.LoadModeResources(ID_MODE_PRIMITIVE_MEND);
}

void PrimitiveMendState::OnExit([[maybe_unused]] AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"PrimitiveMendState::OnExit\n");
  // The working copy must already have been consumed (Return) or discarded (Escape)
  // by the time OnExit is called.  DeleteCopy is a safety net.
  DeleteCopy();
  // Restore the primary mode UI.
  app.SetModeResourceIdentifier(app.PrimaryModeResourceIdentifier());
  app.LoadModeResources(app.PrimaryMode());
}

void PrimitiveMendState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  if (m_primitiveToMendCopy == nullptr) { return; }

  auto* document = context->GetDocument();
  const auto cursorPosition = context->GetCursorPosition();
  const EoGeVector3d translate{m_mendPrimitiveBegin, cursorPosition};

  // Move the actual document primitive (not the copy) so the UpdateAllViews
  // hints can erase/redraw it correctly in the document.  The copy retains the
  // original snapshot — used only by Escape to restore.
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_primitiveToMend);
  m_primitiveToMend->TranslateUsingMask(translate, m_mendPrimitiveVertexIndex);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, m_primitiveToMend);
  m_mendPrimitiveBegin = cursorPosition;
}

// --- AeSysView dispatch methods (called by OnEditModeReturn / OnEditModeEscape) ---

void AeSysView::MendStateReturn() {
  auto* state = dynamic_cast<PrimitiveMendState*>(GetCurrentState());
  if (state == nullptr || !state->IsEngaged()) { return; }

  // The document primitive is already at the dragged position — just commit.
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, state->PrimitiveToMend());
  state->ConsumeCopy();

  PopState();
}

void AeSysView::MendStateEscape() {
  auto* state = dynamic_cast<PrimitiveMendState*>(GetCurrentState());
  if (state == nullptr || !state->IsEngaged()) { return; }

  // Revert the document primitive to the saved snapshot, then discard copy.
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, state->PrimitiveToMend());
  state->PrimitiveToMend()->Assign(state->PrimitiveToMendCopy());
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, state->PrimitiveToMend());
  state->ConsumeCopy();

  PopState();
}
