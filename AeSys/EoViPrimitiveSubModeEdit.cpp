#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "Resource.h"

void AeSysView::OnModePrimitiveEdit() {
  InitializeGroupAndPrimitiveEdit();

  m_SubModeEditBeginPoint = GetCursorPosition();

  EoDbGroup* Group = SelectGroupAndPrimitive(m_SubModeEditBeginPoint);

  if (Group != 0) {
    m_SubModeEditGroup = Group;
    m_SubModeEditPrimitive = EngagedPrimitive();
    app.LoadModeResources(ID_MODE_PRIMITIVE_EDIT);
  }
}
void AeSysView::DoEditPrimitiveCopy() {
  auto* document = GetDocument();
  if (m_SubModeEditPrimitive != 0) {
    EoDbPrimitive* Primitive;

    m_SubModeEditPrimitive->Copy(Primitive);
    m_SubModeEditPrimitive = Primitive;
    m_SubModeEditGroup = new EoDbGroup(m_SubModeEditPrimitive);
    document->AddWorkLayerGroup(m_SubModeEditGroup);

    document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);
    m_tmEditSeg.Identity();
  }
}
void AeSysView::DoEditPrimitiveEscape() {
  auto* document = GetDocument();
  if (m_SubModeEditPrimitive != 0) {
    m_tmEditSeg.Inverse();

    document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);
    m_SubModeEditPrimitive->Transform(m_tmEditSeg);
    document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);

    InitializeGroupAndPrimitiveEdit();

    app.LoadModeResources(app.PrimaryMode());
  }
}
void AeSysView::DoEditPrimitiveTransform(EoUInt16 operation) {
  if (m_SubModeEditPrimitive != 0) {
    auto* document = GetDocument();
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

    document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);
    m_SubModeEditPrimitive->Transform(tm);
    document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);

    m_tmEditSeg *= tm;
  }
}
void AeSysView::PreviewPrimitiveEdit() {
  if (m_SubModeEditPrimitive != 0) {
    auto* document = GetDocument();
    EoGeTransformMatrix tm;
    m_SubModeEditEndPoint = GetCursorPosition();
    tm.Translate(EoGeVector3d(m_SubModeEditBeginPoint, m_SubModeEditEndPoint));

    if (app.IsTrapHighlighted() && document->FindTrappedGroup(m_SubModeEditGroup) != 0)
      EoDbPrimitive::SetSpecialPenColorIndex(app.TrapHighlightColor());

    document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);
    m_SubModeEditPrimitive->Transform(tm);
    document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_SubModeEditPrimitive);

    EoDbPrimitive::SetSpecialPenColorIndex(0);

    m_tmEditSeg *= tm;

    m_SubModeEditBeginPoint = m_SubModeEditEndPoint;
  }
}
