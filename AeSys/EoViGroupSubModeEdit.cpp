#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "Resource.h"

void AeSysView::OnModeGroupEdit() {
  InitializeGroupAndPrimitiveEdit();

  m_SubModeEditBeginPoint = GetCursorPosition();

  EoDbGroup* Group = SelectGroupAndPrimitive(m_SubModeEditBeginPoint);

  if (Group != 0) {
    m_SubModeEditGroup = Group;
    app.LoadModeResources(ID_MODE_GROUP_EDIT);
  }
}
void AeSysView::DoEditGroupCopy() {
  auto* Document = GetDocument();
  if (m_SubModeEditGroup != 0) {
    EoDbGroup* Group = new EoDbGroup(*m_SubModeEditGroup);

    Document->AddWorkLayerGroup(Group);
    m_SubModeEditGroup = Group;

    Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);
    m_tmEditSeg.Identity();
  }
}
void AeSysView::DoEditGroupEscape() {
  if (m_SubModeEditGroup != 0) {
    auto* document = GetDocument();
    m_tmEditSeg.Inverse();

    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);
    m_SubModeEditGroup->Transform(m_tmEditSeg);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);

    InitializeGroupAndPrimitiveEdit();

    app.LoadModeResources(app.PrimaryMode());
  }
}
void AeSysView::DoEditGroupTransform(EoUInt16 operation) {
  auto* Document = GetDocument();
  if (m_SubModeEditGroup != 0) {
    EoGeTransformMatrix tm;

    EoGeVector3d TranslateVector(m_SubModeEditBeginPoint, EoGePoint3d::kOrigin);

    tm.Translate(TranslateVector);

    if (operation == ID_OP2) {
      tm *= EditModeRotationTMat();
    } else if (operation == ID_OP3) {
      tm *= EditModeInvertedRotationTMat();
    } else if (operation == ID_OP6) {
      tm.Scale(EditModeMirrorScale());
    } else if (operation == ID_OP7) {
      tm.Scale(EditModeInvertedScaleFactors());
    } else if (operation == ID_OP8) {
      tm.Scale(EditModeScaleFactors());
    }
    tm.Translate(-TranslateVector);

    Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);
    m_SubModeEditGroup->Transform(tm);
    Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);

    m_tmEditSeg *= tm;
  }
}
void AeSysView::PreviewGroupEdit() {
  auto* Document = GetDocument();
  if (m_SubModeEditGroup != 0) {
    EoGeTransformMatrix tm;
    m_SubModeEditEndPoint = GetCursorPosition();
    tm.Translate(EoGeVector3d(m_SubModeEditBeginPoint, m_SubModeEditEndPoint));

    if (app.IsTrapHighlighted() && Document->FindTrappedGroup(m_SubModeEditGroup) != 0) {
      EoDbPrimitive::SetSpecialPenColorIndex(app.TrapHighlightColor());
    }
    Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);
    m_SubModeEditGroup->Transform(tm);
    Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_SubModeEditGroup);

    EoDbPrimitive::SetSpecialPenColorIndex(0);

    m_tmEditSeg *= tm;

    m_SubModeEditBeginPoint = m_SubModeEditEndPoint;
  }
}
void AeSysDoc::InitializeGroupAndPrimitiveEdit() {
  auto position = GetFirstViewPosition();
  while (position != 0) {
    AeSysView* View = (AeSysView*)GetNextView(position);
    View->InitializeGroupAndPrimitiveEdit();
  }
}
void AeSysView::InitializeGroupAndPrimitiveEdit() {
  m_SubModeEditBeginPoint = EoGePoint3d::kOrigin;
  m_SubModeEditEndPoint = m_SubModeEditBeginPoint;

  m_SubModeEditGroup = 0;
  m_SubModeEditPrimitive = 0;

  m_tmEditSeg.Identity();
}
