#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "Resource.h"

void AeSysView::OnModeGroupEdit() {
  InitializeGroupAndPrimitiveEdit();

  m_SubModeEditBeginPoint = GetCursorPosition();

  auto* group = SelectGroupAndPrimitive(m_SubModeEditBeginPoint);

  if (group != nullptr) {
    m_SubModeEditGroup = group;
    app.LoadModeResources(ID_MODE_GROUP_EDIT);
  }
}

void AeSysView::DoEditGroupCopy() {
  auto* document = GetDocument();
  if (m_SubModeEditGroup == nullptr) { return; }
  auto* group = new EoDbGroup(*m_SubModeEditGroup);

  document->AddWorkLayerGroup(group);
  m_SubModeEditGroup = group;

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);
  m_tmEditSeg.Identity();
}

void AeSysView::DoEditGroupEscape() {
  if (m_SubModeEditGroup == nullptr) { return; }
  auto* document = GetDocument();
  m_tmEditSeg.Inverse();

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);
  m_SubModeEditGroup->Transform(m_tmEditSeg);
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);

  InitializeGroupAndPrimitiveEdit();

  app.LoadModeResources(app.PrimaryMode());
}

void AeSysView::DoEditGroupTransform(std::uint16_t operation) {
  auto* document = GetDocument();
  if (m_SubModeEditGroup == nullptr) { return; }
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

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);
  m_SubModeEditGroup->Transform(transformMatrix);
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);

  m_tmEditSeg *= transformMatrix;
}

void AeSysView::PreviewGroupEdit() {
  auto* document = GetDocument();
  if (m_SubModeEditGroup == nullptr) { return; }
  EoGeTransformMatrix transformMatrix;
  m_SubModeEditEndPoint = GetCursorPosition();
  transformMatrix.Translate(EoGeVector3d(m_SubModeEditBeginPoint, m_SubModeEditEndPoint));

  if (app.IsTrapHighlighted() && document->FindTrappedGroup(m_SubModeEditGroup) != 0) {
    EoDbPrimitive::SetSpecialColor(app.TrapHighlightColor());
  }
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);
  m_SubModeEditGroup->Transform(transformMatrix);
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);

  EoDbPrimitive::SetSpecialColor(0);

  m_tmEditSeg *= transformMatrix;

  m_SubModeEditBeginPoint = m_SubModeEditEndPoint;
}

void AeSysDoc::InitializeGroupAndPrimitiveEdit() {
  auto position = GetFirstViewPosition();
  while (position != nullptr) {
    AeSysView* View = (AeSysView*)GetNextView(position);
    View->InitializeGroupAndPrimitiveEdit();
  }
}
void AeSysView::InitializeGroupAndPrimitiveEdit() {
  m_SubModeEditBeginPoint = EoGePoint3d::kOrigin;
  m_SubModeEditEndPoint = m_SubModeEditBeginPoint;

  m_SubModeEditGroup = nullptr;
  m_SubModeEditPrimitive = nullptr;

  m_tmEditSeg.Identity();
}
