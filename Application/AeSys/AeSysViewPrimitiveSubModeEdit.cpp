#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "PickAndDragState.h"
#include "Resource.h"

namespace {
/// Returns the active PickAndDragState from the top of the stack, or nullptr.
PickAndDragState* PickDragState(AeSysView* view) {
  return dynamic_cast<PickAndDragState*>(view->GetCurrentState());
}
}  // namespace

void AeSysView::OnModePrimitiveEdit() {
  // Peek to see if cursor is over a primitive before pushing, so we don't push
  // a dead state when there is nothing to engage.
  const auto cursorPosition = GetCursorPosition();
  auto* group = SelectGroupAndPrimitive(cursorPosition);
  if (group == nullptr) { return; }
  // Disengage so OnEnter can re-engage cleanly via SelectGroupAndPrimitive.
  GetDocument()->InitializeGroupAndPrimitiveEdit();

  PushState(std::make_unique<PickAndDragState>(PickAndDragState::Kind::Primitive));
}

void AeSysView::DoEditPrimitiveCopy() {
  auto* dragState = PickDragState(this);
  if (dragState == nullptr || dragState->SubModeEditPrimitive() == nullptr) { return; }
  auto* document = GetDocument();
  EoDbPrimitive* primitive{};

  dragState->SubModeEditPrimitive()->Copy(primitive);
  dragState->SetSubModeEditPrimitive(primitive);
  dragState->SetSubModeEditGroup(new EoDbGroup(primitive));
  document->AddWorkLayerGroup(dragState->SubModeEditGroup());

  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, dragState->SubModeEditPrimitive());
  dragState->EditSegTransform().Identity();
}

void AeSysView::DoEditPrimitiveEscape() {
  auto* dragState = PickDragState(this);
  if (dragState == nullptr || dragState->SubModeEditPrimitive() == nullptr) { return; }
  auto* document = GetDocument();
  dragState->EditSegTransform().Inverse();

  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, dragState->SubModeEditPrimitive());
  dragState->SubModeEditPrimitive()->Transform(dragState->EditSegTransform());
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, dragState->SubModeEditPrimitive());

  // Pop restores the primary mode via OnExit + the primary mode state below on stack.
  PopState();
}

void AeSysView::DoEditPrimitiveTransform(std::uint16_t operation) {
  auto* dragState = PickDragState(this);
  if (dragState == nullptr || dragState->SubModeEditPrimitive() == nullptr) { return; }
  auto* document = GetDocument();
  EoGeTransformMatrix transformMatrix;

  const EoGeVector3d translateVector(dragState->SubModeEditBeginPoint(), EoGePoint3d::kOrigin);

  transformMatrix.Translate(translateVector);

  if (operation == ID_OP2) {
    transformMatrix *= EditModeRotationTMat();
  } else if (operation == ID_OP3) {
    transformMatrix *= EditModeInvertedRotationTMat();
  } else if (operation == ID_OP6) {
    transformMatrix.Scale(EditModeMirrorScale());
  } else if (operation == ID_OP7) {
    transformMatrix.Scale(EditModeInvertedScaleFactors());
  } else if (operation == ID_OP8) {
    transformMatrix.Scale(EditModeScaleFactors());
  }
  transformMatrix.Translate(-translateVector);

  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, dragState->SubModeEditPrimitive());
  dragState->SubModeEditPrimitive()->Transform(transformMatrix);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, dragState->SubModeEditPrimitive());

  dragState->EditSegTransform() *= transformMatrix;
}

void AeSysView::PreviewPrimitiveEdit() {
  auto* dragState = PickDragState(this);
  if (dragState == nullptr || dragState->SubModeEditPrimitive() == nullptr) { return; }
  auto* document = GetDocument();
  EoGeTransformMatrix transformMatrix;
  dragState->SubModeEditEndPoint() = GetCursorPosition();
  transformMatrix.Translate(EoGeVector3d(dragState->SubModeEditBeginPoint(), dragState->SubModeEditEndPoint()));

  if (app.IsTrapHighlighted() && document->FindTrappedGroup(dragState->SubModeEditGroup()) != nullptr) {
    EoDbPrimitive::SetSpecialColor(app.TrapHighlightColor());
  }
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, dragState->SubModeEditPrimitive());
  dragState->SubModeEditPrimitive()->Transform(transformMatrix);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, dragState->SubModeEditPrimitive());

  EoDbPrimitive::SetSpecialColor(0);

  dragState->EditSegTransform() *= transformMatrix;
  dragState->SubModeEditBeginPoint() = dragState->SubModeEditEndPoint();
}
