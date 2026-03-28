#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "Resource.h"

void AeSysView::OnModePrimitiveEdit() {
  InitializeGroupAndPrimitiveEdit();

  m_SubModeEditBeginPoint = GetCursorPosition();

  auto* group = SelectGroupAndPrimitive(m_SubModeEditBeginPoint);

  if (group != nullptr) {
    m_SubModeEditGroup = group;
    m_SubModeEditPrimitive = EngagedPrimitive();
    app.LoadModeResources(ID_MODE_PRIMITIVE_EDIT);
  }
}

void AeSysView::DoEditPrimitiveCopy() {
  auto* document = GetDocument();
  if (m_SubModeEditPrimitive == nullptr) { return; }
  EoDbPrimitive* primitive{};

  m_SubModeEditPrimitive->Copy(primitive);
  m_SubModeEditPrimitive = primitive;
  m_SubModeEditGroup = new EoDbGroup(m_SubModeEditPrimitive);
  document->AddWorkLayerGroup(m_SubModeEditGroup);

  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);
  m_tmEditSeg.Identity();
}

void AeSysView::DoEditPrimitiveEscape() {
  auto* document = GetDocument();
  if (m_SubModeEditPrimitive == nullptr) { return; }
  m_tmEditSeg.Inverse();

  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);
  m_SubModeEditPrimitive->Transform(m_tmEditSeg);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);

  InitializeGroupAndPrimitiveEdit();

  app.LoadModeResources(app.PrimaryMode());
}

void AeSysView::DoEditPrimitiveTransform(std::uint16_t operation) {
  if (m_SubModeEditPrimitive == nullptr) { return; }
  auto* document = GetDocument();
  EoGeTransformMatrix transformMatrix;

  EoGeVector3d TranslateVector(m_SubModeEditBeginPoint, EoGePoint3d::kOrigin);

  transformMatrix.Translate(TranslateVector);

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
  transformMatrix.Translate(-TranslateVector);

  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);
  m_SubModeEditPrimitive->Transform(transformMatrix);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);

  m_tmEditSeg *= transformMatrix;
}

void AeSysView::PreviewPrimitiveEdit() {
  if (m_SubModeEditPrimitive == nullptr) { return; }
  auto* document = GetDocument();
  EoGeTransformMatrix transformMatrix;
  m_SubModeEditEndPoint = GetCursorPosition();
  transformMatrix.Translate(EoGeVector3d(m_SubModeEditBeginPoint, m_SubModeEditEndPoint));

  if (app.IsTrapHighlighted() && document->FindTrappedGroup(m_SubModeEditGroup) != 0) {
    EoDbPrimitive::SetSpecialColor(app.TrapHighlightColor());
  }
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);
  m_SubModeEditPrimitive->Transform(transformMatrix);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);

  EoDbPrimitive::SetSpecialColor(0);

  m_tmEditSeg *= transformMatrix;

  m_SubModeEditBeginPoint = m_SubModeEditEndPoint;
}
