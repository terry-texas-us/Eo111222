#include "Stdafx.h"

#include <algorithm>
#include <cstdarg>

#include "AeSys.h"
#include "AeSysView.h"
#include "EoDlgViewParameters.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "EoGsViewTransform.h"
#include "EoGsViewport.h"
#include "Resource.h"

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

EoDlgViewParameters::EoDlgViewParameters(CWnd* pParent /*=nullptr*/)
	: CDialog(EoDlgViewParameters::IDD, pParent), m_ModelView(0) {
}
EoDlgViewParameters::~EoDlgViewParameters() {
}
void EoDlgViewParameters::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Check(dataExchange, IDC_PERSPECTIVE_PROJECTION, m_PerspectiveProjection);
}

void EoDlgViewParameters::OnBnClickedApply() {
	auto* activeView = AeSysView::GetActiveView();

	EoGsViewport viewport;
	activeView->ModelViewGetViewport(viewport);

	auto* modelView = reinterpret_cast<EoGsViewTransform*>(static_cast<uintptr_t>(m_ModelView));

	wchar_t itemText[32]{};
	EoGePoint3d position;
	GetDlgItemTextW(IDC_POSITION_X, itemText, 32);
	position.x = app.ParseLength(app.GetUnits(), itemText);
	GetDlgItemTextW(IDC_POSITION_Y, itemText, 32);
	position.y = app.ParseLength(app.GetUnits(), itemText);
	GetDlgItemTextW(IDC_POSITION_Z, itemText, 32);
	position.z = app.ParseLength(app.GetUnits(), itemText);
	modelView->SetPosition(position);

	EoGePoint3d target;
	GetDlgItemTextW(IDC_TARGET_X, itemText, 32);
	target.x = app.ParseLength(app.GetUnits(), itemText);
	GetDlgItemTextW(IDC_TARGET_Y, itemText, 32);
	target.y = app.ParseLength(app.GetUnits(), itemText);
	GetDlgItemTextW(IDC_TARGET_Z, itemText, 32);
	target.z = app.ParseLength(app.GetUnits(), itemText);
	modelView->SetTarget(target);

	GetDlgItemTextW(IDC_FRONT_CLIP_DISTANCE, itemText, 32);
	double nearClipDistance = app.ParseLength(app.GetUnits(), itemText);
	GetDlgItemTextW(IDC_BACK_CLIP_DISTANCE, itemText, 32);
	double farClipDistance = app.ParseLength(app.GetUnits(), itemText);

	GetDlgItemTextW(IDC_LENS_LENGTH, itemText, 32);
	double lensLength = app.ParseLength(app.GetUnits(), itemText);

	auto direction = target - position;

	modelView->SetDirection(direction);

	auto viewUp = CrossProduct(direction, EoGeVector3d::positiveUnitZ);
	viewUp = CrossProduct(viewUp, direction);

	if (viewUp.IsNearNull()) {
		viewUp = EoGeVector3d::positiveUnitY;
	}
	else {
		viewUp.Normalize();
	}
	modelView->SetView(position, target, viewUp);
	modelView->SetLensLength(lensLength);
	modelView->SetNearClipDistance(nearClipDistance);
	modelView->SetFarClipDistance(farClipDistance);
	modelView->EnablePerspective(m_PerspectiveProjection == TRUE);
	modelView->SetCenteredWindow(viewport, 0.0, 0.0);

	modelView->BuildTransformMatrix();

	activeView->SetViewTransform(*modelView);
	activeView->InvalidateRect(nullptr, TRUE);

	GetDlgItem(IDC_APPLY)->EnableWindow(FALSE);
}

BOOL EoDlgViewParameters::OnInitDialog() {
	CDialog::OnInitDialog();

    auto* ModelView = reinterpret_cast<EoGsViewTransform*>(static_cast<uintptr_t>(m_ModelView));

	CString Length;
    Eo::Units units = std::max(app.GetUnits(), Eo::Units::Engineering);
	app.FormatLength(Length, units, ModelView->Position().x);
	SetDlgItemTextW(IDC_POSITION_X, Length);
	app.FormatLength(Length, units, ModelView->Position().y);
	SetDlgItemTextW(IDC_POSITION_Y, Length);
	app.FormatLength(Length, units, ModelView->Position().z);
	SetDlgItemTextW(IDC_POSITION_Z, Length);

	app.FormatLength(Length, units, ModelView->Target().x);
	SetDlgItemTextW(IDC_TARGET_X, Length);
	app.FormatLength(Length, units, ModelView->Target().y);
	SetDlgItemTextW(IDC_TARGET_Y, Length);
	app.FormatLength(Length, units, ModelView->Target().z);
	SetDlgItemTextW(IDC_TARGET_Z, Length);

	app.FormatLength(Length, units, ModelView->NearClipDistance());
	SetDlgItemTextW(IDC_FRONT_CLIP_DISTANCE, Length);
	app.FormatLength(Length, units, ModelView->FarClipDistance());
	SetDlgItemTextW(IDC_BACK_CLIP_DISTANCE, Length);

	app.FormatLength(Length, units, ModelView->LensLength());
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
