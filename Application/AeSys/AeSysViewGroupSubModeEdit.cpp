#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "PickAndDragState.h"
#include "Resource.h"

namespace {
PickAndDragState* PickDragState(AeSysView* view) {
  return dynamic_cast<PickAndDragState*>(view->GetCurrentState());
}
}  // namespace

void AeSysView::OnModeGroupEdit() {
  const auto cursorPosition = GetCursorPosition();
  auto* group = SelectGroupAndPrimitive(cursorPosition);
  if (group == nullptr) { return; }
  GetDocument()->InitializeGroupAndPrimitiveEdit();

  PushState(std::make_unique<PickAndDragState>(PickAndDragState::Kind::Group));
}

void AeSysView::DoEditGroupCopy() {
  auto* dragState = PickDragState(this);
  if (dragState == nullptr || dragState->SubModeEditGroup() == nullptr) { return; }
  auto* document = GetDocument();
  auto* group = new EoDbGroup(*dragState->SubModeEditGroup());

  document->AddWorkLayerGroup(group);
  dragState->SetSubModeEditGroup(group);

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, dragState->SubModeEditGroup());
  dragState->EditSegTransform().Identity();
}

void AeSysView::DoEditGroupEscape() {
  auto* dragState = PickDragState(this);
  if (dragState == nullptr || dragState->SubModeEditGroup() == nullptr) { return; }
  auto* document = GetDocument();
  dragState->EditSegTransform().Inverse();

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, dragState->SubModeEditGroup());
  dragState->SubModeEditGroup()->Transform(dragState->EditSegTransform());
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, dragState->SubModeEditGroup());

  PopState();
}

void AeSysView::DoEditGroupTransform(std::uint16_t operation) {
  auto* dragState = PickDragState(this);
  if (dragState == nullptr || dragState->SubModeEditGroup() == nullptr) { return; }
  auto* document = GetDocument();
  EoGeTransformMatrix transformMatrix;

  const EoGeVector3d translateVector{dragState->SubModeEditBeginPoint(), EoGePoint3d::kOrigin};

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

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, dragState->SubModeEditGroup());
  dragState->SubModeEditGroup()->Transform(transformMatrix);
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, dragState->SubModeEditGroup());

  dragState->EditSegTransform() *= transformMatrix;
}

void AeSysView::PreviewGroupEdit() {
  auto* dragState = PickDragState(this);
  if (dragState == nullptr || dragState->SubModeEditGroup() == nullptr) { return; }
  auto* document = GetDocument();
  EoGeTransformMatrix transformMatrix;
  dragState->SubModeEditEndPoint() = GetCursorPosition();
  transformMatrix.Translate(EoGeVector3d(dragState->SubModeEditBeginPoint(), dragState->SubModeEditEndPoint()));

  if (app.IsTrapHighlighted() && document->FindTrappedGroup(dragState->SubModeEditGroup()) != nullptr) {
    EoDbPrimitive::SetSpecialColor(app.TrapHighlightColor());
  }
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, dragState->SubModeEditGroup());
  dragState->SubModeEditGroup()->Transform(transformMatrix);
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, dragState->SubModeEditGroup());

  EoDbPrimitive::SetSpecialColor(0);

  dragState->EditSegTransform() *= transformMatrix;
  dragState->SubModeEditBeginPoint() = dragState->SubModeEditEndPoint();
}

void AeSysDoc::InitializeGroupAndPrimitiveEdit() const {
  auto position = GetFirstViewPosition();
  while (position != nullptr) {
    const auto view = static_cast<AeSysView*>(GetNextView(position));
    view->InitializeGroupAndPrimitiveEdit();
  }
}

void AeSysView::InitializeGroupAndPrimitiveEdit() {
  // State is now owned by PickAndDragState on the stack.
  // This method is retained as a no-op called by PickAndDragState::OnExit
  // and by document-level InitializeGroupAndPrimitiveEdit broadcasts.
}
