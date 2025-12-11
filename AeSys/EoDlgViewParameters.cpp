#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"
#include "EoDlgViewParameters.h"

// EoDlgViewParameters dialog

IMPLEMENT_DYNAMIC(EoDlgViewParameters, CDialog)

BEGIN_MESSAGE_MAP(EoDlgViewParameters, CDialog)
	ON_BN_CLICKED(IDC_APPLY, &EoDlgViewParameters::OnBnClickedApply)
	ON_EN_CHANGE(IDC_POSITION_X, &EoDlgViewParameters::OnEnChangePositionX)
	ON_EN_CHANGE(IDC_POSITION_Y, &EoDlgViewParameters::OnEnChangePositionY)
	ON_EN_CHANGE(IDC_POSITION_Z, &EoDlgViewParameters::OnEnChangePositionZ)
	ON_EN_CHANGE(IDC_TARGET_X, &EoDlgViewParameters::OnEnChangeTargetX)
	ON_EN_CHANGE(IDC_TARGET_Y, &EoDlgViewParameters::OnEnChangeTargetY)
	ON_EN_CHANGE(IDC_TARGET_Z, &EoDlgViewParameters::OnEnChangeTargetZ)
	ON_EN_CHANGE(IDC_FRONT_CLIP_DISTANCE, &EoDlgViewParameters::OnEnChangeFrontClipDistance)
	ON_EN_CHANGE(IDC_BACK_CLIP_DISTANCE, &EoDlgViewParameters::OnEnChangeBackClipDistance)
	ON_EN_CHANGE(IDC_LENS_LENGTH, &EoDlgViewParameters::OnEnChangeLensLength)
	ON_BN_CLICKED(IDC_PERSPECTIVE_PROJECTION, &EoDlgViewParameters::OnBnClickedPerspectiveProjection)
END_MESSAGE_MAP()

EoDlgViewParameters::EoDlgViewParameters(CWnd* pParent /*=NULL*/)
	: CDialog(EoDlgViewParameters::IDD, pParent), m_ModelView(0) {
}
EoDlgViewParameters::~EoDlgViewParameters() {
}
void EoDlgViewParameters::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_PERSPECTIVE_PROJECTION, m_PerspectiveProjection);
}
// EoDlgViewParameters message handlers

void EoDlgViewParameters::OnBnClickedApply() {
	AeSysView* ActiveView = AeSysView::GetActiveView();

	EoGsViewport Viewport;
	ActiveView->ModelViewGetViewport(Viewport);

	EoGsViewTransform* ModelView = (EoGsViewTransform*) m_ModelView;

	WCHAR szBuf[32];
	EoGePoint3d Position;
	GetDlgItemTextW(IDC_POSITION_X, (LPWSTR) szBuf, 32);
	Position.x = app.ParseLength(app.GetUnits(), szBuf);
	GetDlgItemTextW(IDC_POSITION_Y, (LPWSTR) szBuf, 32);
	Position.y = app.ParseLength(app.GetUnits(), szBuf);
	GetDlgItemTextW(IDC_POSITION_Z, (LPWSTR) szBuf, 32);
	Position.z = app.ParseLength(app.GetUnits(), szBuf);
	ModelView->SetPosition(Position);

	EoGePoint3d Target;
	GetDlgItemTextW(IDC_TARGET_X, (LPWSTR) szBuf, 32);
	Target.x = app.ParseLength(app.GetUnits(), szBuf);
	GetDlgItemTextW(IDC_TARGET_Y, (LPWSTR) szBuf, 32);
	Target.y = app.ParseLength(app.GetUnits(), szBuf);
	GetDlgItemTextW(IDC_TARGET_Z, (LPWSTR) szBuf, 32);
	Target.z = app.ParseLength(app.GetUnits(), szBuf);
	ModelView->SetTarget(Target);

	GetDlgItemTextW(IDC_FRONT_CLIP_DISTANCE, (LPWSTR) szBuf, 32);
	float NearClipDistance = static_cast<float>(app.ParseLength(app.GetUnits(), szBuf));
	GetDlgItemTextW(IDC_BACK_CLIP_DISTANCE, (LPWSTR) szBuf, 32);
	float FarClipDistance = static_cast<float>(app.ParseLength(app.GetUnits(), szBuf));

	GetDlgItemTextW(IDC_LENS_LENGTH, (LPWSTR) szBuf, 32);
	float LensLength = static_cast<float>(app.ParseLength(app.GetUnits(), szBuf));

	EoGeVector3d Direction = Position - Target;

	ModelView->SetDirection(Direction);

	EoGeVector3d ViewUp = EoGeCrossProduct(Direction, EoGeVector3d::kZAxis);
	ViewUp = EoGeCrossProduct(ViewUp, Direction);

	if (ViewUp.IsNearNull()) {
		ViewUp = EoGeVector3d::kYAxis;
	}
	else {
		ViewUp.Normalize();
	}
	ModelView->SetView(Position, Target, ViewUp);
	ModelView->SetLensLength(LensLength);
	ModelView->SetNearClipDistance(NearClipDistance);
	ModelView->SetFarClipDistance(FarClipDistance);
	ModelView->EnablePerspective(m_PerspectiveProjection == TRUE);
	ModelView->SetCenteredWindow(Viewport, 0.0f, 0.0f);

	ModelView->BuildTransformMatrix();

	ActiveView->SetViewTransform(*ModelView);
	ActiveView->InvalidateRect(NULL, TRUE);

	GetDlgItem(IDC_APPLY)->EnableWindow(FALSE);
}
BOOL EoDlgViewParameters::OnInitDialog() {
	CDialog::OnInitDialog();

	EoGsViewTransform* ModelView = (EoGsViewTransform*) m_ModelView;

	CString Length;
	AeSys::Units Units = max(app.GetUnits(), AeSys::kEngineering);
	app.FormatLength(Length, Units, ModelView->Position().x, 16, 8);
	SetDlgItemTextW(IDC_POSITION_X, Length);
	app.FormatLength(Length, Units, ModelView->Position().y, 16, 8);
	SetDlgItemTextW(IDC_POSITION_Y, Length);
	app.FormatLength(Length, Units, ModelView->Position().z, 16, 8);
	SetDlgItemTextW(IDC_POSITION_Z, Length);

	app.FormatLength(Length, Units, ModelView->Target().x, 16, 8);
	SetDlgItemTextW(IDC_TARGET_X, Length);
	app.FormatLength(Length, Units, ModelView->Target().y, 16, 8);
	SetDlgItemTextW(IDC_TARGET_Y, Length);
	app.FormatLength(Length, Units, ModelView->Target().z, 16, 8);
	SetDlgItemTextW(IDC_TARGET_Z, Length);

	app.FormatLength(Length, Units, ModelView->NearClipDistance(), 16, 8);
	SetDlgItemTextW(IDC_FRONT_CLIP_DISTANCE, Length);
	app.FormatLength(Length, Units, ModelView->FarClipDistance(), 16, 8);
	SetDlgItemTextW(IDC_BACK_CLIP_DISTANCE, Length);

	app.FormatLength(Length, Units, ModelView->LensLength(), 16, 8);
	SetDlgItemTextW(IDC_LENS_LENGTH, Length);

	GetDlgItem(IDC_APPLY)->EnableWindow(FALSE);

	return TRUE;
}
void EoDlgViewParameters::OnOK() {
	OnBnClickedApply();

	CDialog::OnOK();
}
void EoDlgViewParameters::OnEnChangePositionX() {
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void EoDlgViewParameters::OnEnChangePositionY() {
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void EoDlgViewParameters::OnEnChangePositionZ() {
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void EoDlgViewParameters::OnEnChangeTargetX() {
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void EoDlgViewParameters::OnEnChangeTargetY() {
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void EoDlgViewParameters::OnEnChangeTargetZ() {
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void EoDlgViewParameters::OnEnChangeFrontClipDistance() {
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void EoDlgViewParameters::OnEnChangeBackClipDistance() {
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void EoDlgViewParameters::OnEnChangeLensLength() {
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void EoDlgViewParameters::OnBnClickedPerspectiveProjection() {
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
