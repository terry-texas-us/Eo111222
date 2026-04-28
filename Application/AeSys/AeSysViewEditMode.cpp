#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDlgEditOptions.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "Resource.h"

void AeSysView::OnEditModeOptions() {
  EoDlgEditOptions editOptions(this);
  m_EditModeScale.Get(editOptions.m_EditModeScaleX, editOptions.m_EditModeScaleY, editOptions.m_EditModeScaleZ);
  m_editModeRotationAngles.Get(
      editOptions.m_EditModeRotationAngleX, editOptions.m_EditModeRotationAngleY, editOptions.m_EditModeRotationAngleZ);

  if (editOptions.DoModal() == IDOK) {
    m_editModeRotationAngles.Set(editOptions.m_EditModeRotationAngleX,
        editOptions.m_EditModeRotationAngleY,
        editOptions.m_EditModeRotationAngleZ);
    m_EditModeScale.Set(editOptions.m_EditModeScaleX, editOptions.m_EditModeScaleY, editOptions.m_EditModeScaleZ);
  }
}

void AeSysView::OnEditModePivot() {
  auto* document = GetDocument();

  const auto cursorPosition = GetCursorPosition();
  document->SetTrapPivotPoint(cursorPosition);
}

void AeSysView::OnEditModeRotccw() {
  auto* document = GetDocument();

  const auto matrix = EoGeTransformMatrix::BuildRotationTransformMatrix(EditModeRotationAngles());
  const EoGeVector3d translateVector(document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

  EoGeTransformMatrix transformMatrix;
  transformMatrix.Translate(translateVector);
  transformMatrix *= matrix;
  transformMatrix.Translate(-translateVector);
  document->TransformTrappedGroups(transformMatrix);
}

void AeSysView::OnEditModeRotcw() {
  auto* document = GetDocument();

  auto matrix = EoGeTransformMatrix::BuildRotationTransformMatrix(EditModeRotationAngles());
  matrix.Inverse();
  const EoGeVector3d translateVector(document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

  EoGeTransformMatrix transformMatrix;
  transformMatrix.Translate(translateVector);
  transformMatrix *= matrix;
  transformMatrix.Translate(-translateVector);
  document->TransformTrappedGroups(transformMatrix);
}

void AeSysView::OnEditModeMove() {
  auto* document = GetDocument();

  const auto cursorPosition = GetCursorPosition();
  if (m_PreviousOp != ID_OP4) {
    m_PreviousOp = ModeLineHighlightOp(ID_OP4);
    RubberBandingStartAtEnable(cursorPosition, Lines);
  } else {
    EoGeTransformMatrix transformMatrix;
    transformMatrix.Translate(EoGeVector3d(document->GetTrapPivotPoint(), cursorPosition));

    ModeLineUnhighlightOp(m_PreviousOp);
    RubberBandingDisable();
    document->TransformTrappedGroups(transformMatrix);
  }
  document->SetTrapPivotPoint(cursorPosition);
  // pSetSegPos(pTRAP_PVT_MRK_ID, pt);
}

void AeSysView::OnEditModeCopy() {
  auto* document = GetDocument();
  const auto cursorPosition = GetCursorPosition();
  if (m_PreviousOp != ID_OP5) {
    m_PreviousOp = ModeLineHighlightOp(ID_OP5);
    RubberBandingStartAtEnable(cursorPosition, Lines);
  } else {
    ModeLineUnhighlightOp(m_PreviousOp);
    RubberBandingDisable();
    document->CopyTrappedGroups(EoGeVector3d(document->GetTrapPivotPoint(), cursorPosition));
  }
  document->SetTrapPivotPoint(cursorPosition);
}

void AeSysView::OnEditModeFlip() {
  auto* document = GetDocument();

  EoGeTransformMatrix transformMatrix;
  const EoGeVector3d translateVector{document->GetTrapPivotPoint(), EoGePoint3d::kOrigin};

  transformMatrix.Translate(translateVector);
  transformMatrix.Scale(EditModeMirrorScale());
  transformMatrix.Translate(-translateVector);
  document->TransformTrappedGroups(transformMatrix);
}

void AeSysView::OnEditModeReduce() {
  auto* document = GetDocument();

  EoGeTransformMatrix transformMatrix;
  const EoGeVector3d translateVector{document->GetTrapPivotPoint(), EoGePoint3d::kOrigin};

  transformMatrix.Translate(translateVector);
  transformMatrix.Scale(EditModeInvertedScaleFactors());
  transformMatrix.Translate(-translateVector);
  document->TransformTrappedGroups(transformMatrix);
}

void AeSysView::OnEditModeEnlarge() {
  auto* document = GetDocument();

  EoGeTransformMatrix transformMatrix;
  const EoGeVector3d translateVector{document->GetTrapPivotPoint(), EoGePoint3d::kOrigin};

  transformMatrix.Translate(translateVector);
  transformMatrix.Scale(EditModeScaleFactors());
  transformMatrix.Translate(-translateVector);
  document->TransformTrappedGroups(transformMatrix);
}

void AeSysView::OnEditModeReturn() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
    case ID_MODE_GROUP_EDIT:
      // Commit the drag — geometry stays where it is, just pop back to primary mode.
      PopState();
      break;

    case ID_MODE_PRIMITIVE_MEND:
      MendStateReturn();
      break;
  }
}

void AeSysView::OnEditModeEscape() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveEscape();
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupEscape();
      break;

    case ID_MODE_PRIMITIVE_MEND:
      MendStateEscape();
      break;

    default:
      if (m_PreviousOp == ID_OP4 || m_PreviousOp == ID_OP5) {
        ModeLineUnhighlightOp(m_PreviousOp);
        RubberBandingDisable();
      }
      break;
  }
}
