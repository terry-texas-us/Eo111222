#include "Stdafx.h"

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
  m_editModeRotationAngles.Get(editOptions.m_EditModeRotationAngleX, editOptions.m_EditModeRotationAngleY,
                               editOptions.m_EditModeRotationAngleZ);

  if (editOptions.DoModal() == IDOK) {
    m_editModeRotationAngles.Set(editOptions.m_EditModeRotationAngleX, editOptions.m_EditModeRotationAngleY,
                                 editOptions.m_EditModeRotationAngleZ);
    m_EditModeScale.Set(editOptions.m_EditModeScaleX, editOptions.m_EditModeScaleY, editOptions.m_EditModeScaleZ);
  }
}

void AeSysView::OnEditModePivot() {
  auto* document = GetDocument();

  auto cursorPosition = GetCursorPosition();
  document->SetTrapPivotPoint(cursorPosition);
}

void AeSysView::OnEditModeRotccw() {
  auto* document = GetDocument();

  EoGeTransformMatrix Matrix;
  Matrix = Matrix.BuildRotationTransformMatrix(EditModeRotationAngles());
  EoGeVector3d TranslateVector(document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

  EoGeTransformMatrix transformMatrix;
  transformMatrix.Translate(TranslateVector);
  transformMatrix *= Matrix;
  transformMatrix.Translate(-TranslateVector);
  document->TransformTrappedGroups(transformMatrix);
}

void AeSysView::OnEditModeRotcw() {
  auto* document = GetDocument();

  EoGeTransformMatrix Matrix;
  Matrix = Matrix.BuildRotationTransformMatrix(EditModeRotationAngles());
  Matrix.Inverse();
  EoGeVector3d TranslateVector(document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

  EoGeTransformMatrix transformMatrix;
  transformMatrix.Translate(TranslateVector);
  transformMatrix *= Matrix;
  transformMatrix.Translate(-TranslateVector);
  document->TransformTrappedGroups(transformMatrix);
}

void AeSysView::OnEditModeMove() {
  auto* document = GetDocument();

  auto cursorPosition = GetCursorPosition();
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
  auto cursorPosition = GetCursorPosition();
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
  EoGeVector3d TranslateVector(document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

  transformMatrix.Translate(TranslateVector);
  transformMatrix.Scale(EditModeMirrorScale());
  transformMatrix.Translate(-TranslateVector);
  document->TransformTrappedGroups(transformMatrix);
}

void AeSysView::OnEditModeReduce() {
  auto* document = GetDocument();

  EoGeTransformMatrix transformMatrix;
  EoGeVector3d TranslateVector(document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

  transformMatrix.Translate(TranslateVector);
  transformMatrix.Scale(EditModeInvertedScaleFactors());
  transformMatrix.Translate(-TranslateVector);
  document->TransformTrappedGroups(transformMatrix);
}

void AeSysView::OnEditModeEnlarge() {
  auto* document = GetDocument();

  EoGeTransformMatrix transformMatrix;
  EoGeVector3d TranslateVector(document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

  transformMatrix.Translate(TranslateVector);
  transformMatrix.Scale(EditModeScaleFactors());
  transformMatrix.Translate(-TranslateVector);
  document->TransformTrappedGroups(transformMatrix);
}

void AeSysView::OnEditModeReturn() {
  // TODO: Add your command handler code here
}

void AeSysView::OnEditModeEscape() {
  if (m_PreviousOp == ID_OP4 || m_PreviousOp == ID_OP5) {
    ModeLineUnhighlightOp(m_PreviousOp);
    RubberBandingDisable();
  }
}
