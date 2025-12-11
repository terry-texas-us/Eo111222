#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDlgEditOptions.h"

void AeSysView::OnEditModeOptions() {
	EoDlgEditOptions Dialog(this);
	Dialog.m_EditModeScaleX = m_EditModeScale.x;
	Dialog.m_EditModeScaleY = m_EditModeScale.y;
	Dialog.m_EditModeScaleZ = m_EditModeScale.z;
	Dialog.m_EditModeRotationAngleX = m_EditModeRotationAngles.x;
	Dialog.m_EditModeRotationAngleY = m_EditModeRotationAngles.y;
	Dialog.m_EditModeRotationAngleZ = m_EditModeRotationAngles.z;

	if (Dialog.DoModal() == IDOK) {
		m_EditModeRotationAngles.x = Dialog.m_EditModeRotationAngleX;
		m_EditModeRotationAngles.y = Dialog.m_EditModeRotationAngleY;
		m_EditModeRotationAngles.z = Dialog.m_EditModeRotationAngleZ;
		m_EditModeScale.x = Dialog.m_EditModeScaleX;
		m_EditModeScale.y = Dialog.m_EditModeScaleY;
		m_EditModeScale.z = Dialog.m_EditModeScaleZ;
	}
}

void AeSysView::OnEditModePivot() {
	AeSysDoc* Document = GetDocument();

	EoGePoint3d pt = GetCursorPosition();
	Document->SetTrapPivotPoint(pt);
	// pSetSegPos(pTRAP_PVT_MRK_ID, pt);
}

void AeSysView::OnEditModeRotccw() {
	AeSysDoc* Document = GetDocument();

	EoGeTransformMatrix Matrix;
	Matrix = Matrix.BuildRotationTransformMatrix(EditModeRotationAngles());
	EoGeVector3d TranslateVector(Document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

	EoGeTransformMatrix tm;
	tm.Translate(TranslateVector);
	tm *= Matrix;
	tm.Translate(- TranslateVector);
	Document->TransformTrappedGroups(tm);
}

void AeSysView::OnEditModeRotcw() {
	AeSysDoc* Document = GetDocument();

	EoGeTransformMatrix Matrix;
	Matrix = Matrix.BuildRotationTransformMatrix(EditModeRotationAngles());
	Matrix.Inverse();
	EoGeVector3d TranslateVector(Document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

	EoGeTransformMatrix tm;
	tm.Translate(TranslateVector);
	tm *= Matrix;
	tm.Translate(- TranslateVector);
	Document->TransformTrappedGroups(tm);
}

void AeSysView::OnEditModeMove() {
	AeSysDoc* Document = GetDocument();

	EoGePoint3d pt = GetCursorPosition();
	if (m_PreviousOp != ID_OP4) {
		m_PreviousOp = ModeLineHighlightOp(ID_OP4);
		RubberBandingStartAtEnable(pt, Lines);
	}
	else {
		EoGeTransformMatrix tm;
		tm.Translate(EoGeVector3d(Document->GetTrapPivotPoint(), pt));

		ModeLineUnhighlightOp(m_PreviousOp);
		RubberBandingDisable();
		Document->TransformTrappedGroups(tm);
	}
	Document->SetTrapPivotPoint(pt);
	// pSetSegPos(pTRAP_PVT_MRK_ID, pt);
}

void AeSysView::OnEditModeCopy() {
	AeSysDoc* Document = GetDocument();

	EoGePoint3d pt = GetCursorPosition();
	if (m_PreviousOp != ID_OP5) {
		m_PreviousOp = ModeLineHighlightOp(ID_OP5);
		RubberBandingStartAtEnable(pt, Lines);
	}
	else {
		ModeLineUnhighlightOp(m_PreviousOp);
		RubberBandingDisable();
		Document->CopyTrappedGroups(EoGeVector3d(Document->GetTrapPivotPoint(), pt));
	}
	Document->SetTrapPivotPoint(pt);
	// pSetSegPos(pTRAP_PVT_MRK_ID, pt);
}

void AeSysView::OnEditModeFlip() {
	AeSysDoc* Document = GetDocument();

	EoGeTransformMatrix tm;
	EoGeVector3d TranslateVector(Document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

	tm.Translate(TranslateVector);
	tm.Scale(EditModeMirrorScale());
	tm.Translate(- TranslateVector);
	Document->TransformTrappedGroups(tm);
}

void AeSysView::OnEditModeReduce() {
	AeSysDoc* Document = GetDocument();

	EoGeTransformMatrix tm;
	EoGeVector3d TranslateVector(Document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

	tm.Translate(TranslateVector);
	tm.Scale(EditModeInvertedScaleFactors());
	tm.Translate(- TranslateVector);
	Document->TransformTrappedGroups(tm);
}

void AeSysView::OnEditModeEnlarge() {
	AeSysDoc* Document = GetDocument();

	EoGeTransformMatrix tm;
	EoGeVector3d TranslateVector(Document->GetTrapPivotPoint(), EoGePoint3d::kOrigin);

	tm.Translate(TranslateVector);
	tm.Scale(EditModeScaleFactors());
	tm.Translate(- TranslateVector);
	Document->TransformTrappedGroups(tm);
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
