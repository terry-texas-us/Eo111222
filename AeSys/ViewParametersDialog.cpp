#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"
#include "ViewParametersDialog.h"

// ViewParametersDialog dialog

IMPLEMENT_DYNAMIC(ViewParametersDialog, CDialog)

BEGIN_MESSAGE_MAP(ViewParametersDialog, CDialog)
	ON_BN_CLICKED(IDC_APPLY, &ViewParametersDialog::OnBnClickedApply)
	ON_EN_CHANGE(IDC_POSITION_X, &ViewParametersDialog::OnEnChangePositionX)
	ON_EN_CHANGE(IDC_POSITION_Y, &ViewParametersDialog::OnEnChangePositionY)
	ON_EN_CHANGE(IDC_POSITION_Z, &ViewParametersDialog::OnEnChangePositionZ)
	ON_EN_CHANGE(IDC_TARGET_X, &ViewParametersDialog::OnEnChangeTargetX)
	ON_EN_CHANGE(IDC_TARGET_Y, &ViewParametersDialog::OnEnChangeTargetY)
	ON_EN_CHANGE(IDC_TARGET_Z, &ViewParametersDialog::OnEnChangeTargetZ)
	ON_EN_CHANGE(IDC_FRONT_CLIP_DISTANCE, &ViewParametersDialog::OnEnChangeFrontClipDistance)
	ON_EN_CHANGE(IDC_BACK_CLIP_DISTANCE, &ViewParametersDialog::OnEnChangeBackClipDistance)
	ON_EN_CHANGE(IDC_LENS_LENGTH, &ViewParametersDialog::OnEnChangeLensLength)
	ON_BN_CLICKED(IDC_PERSPECTIVE_PROJECTION, &ViewParametersDialog::OnBnClickedPerspectiveProjection)
END_MESSAGE_MAP()

ViewParametersDialog::ViewParametersDialog(CWnd* pParent /*=NULL*/)
	: CDialog(ViewParametersDialog::IDD, pParent)
	, m_ModelView(0)
{
}
ViewParametersDialog::~ViewParametersDialog()
{
}
void ViewParametersDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_PERSPECTIVE_PROJECTION, m_PerspectiveProjection);
}
// ViewParametersDialog message handlers

void ViewParametersDialog::OnBnClickedApply()
{
	AeSysView* ActiveView = AeSysView::GetActiveView();

	CViewport Viewport;
	ActiveView->ModelViewGetViewport(Viewport);

	CModelView* ModelView = (CModelView*) m_ModelView;
	
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
	double FrontClipDistance = app.ParseLength(app.GetUnits(), szBuf);
	GetDlgItemTextW(IDC_BACK_CLIP_DISTANCE, (LPWSTR) szBuf, 32);
	double BackClipDistance = app.ParseLength(app.GetUnits(), szBuf);
	
	GetDlgItemTextW(IDC_LENS_LENGTH, (LPWSTR) szBuf, 32);
	double LensLength = app.ParseLength(app.GetUnits(), szBuf);

	EoGeVector3d Direction = Position - Target;

	ModelView->SetDirection(Direction);
		
	EoGeVector3d ViewUp = EoGeVector3d::kZAxis.CrossProduct(Direction);

	if (ViewUp.IsNull(DBL_EPSILON))
	{
		ViewUp = EoGeVector3d::kYAxis;
	}
	else
	{
		ViewUp.Normalize();
	}
	ModelView->SetView(Position, Target, ViewUp);
	ModelView->SetLensLength(LensLength);  
	ModelView->SetFrontClipDistance(FrontClipDistance); 
	ModelView->SetBackClipDistance(BackClipDistance);  
	ModelView->SetPerspectiveEnabled(m_PerspectiveProjection);
	ModelView->SetCenteredWindow(Viewport, 0., 0.);
		
	ModelView->BuildTransformMatrix();
	
	ActiveView->ModelViewSetActive(*ModelView);
	ActiveView->InvalidateRect(NULL, TRUE);

	GetDlgItem(IDC_APPLY)->EnableWindow(FALSE);
}
BOOL ViewParametersDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CModelView* ModelView = (CModelView*) m_ModelView;

	WCHAR szBuf[32];
	app.FormatLength(szBuf, 32, max(app.GetUnits(), AeSys::kEngineering), ModelView->Position().x, 16, 8);
	SetDlgItemTextW(IDC_POSITION_X, szBuf);
	app.FormatLength(szBuf, 32, max(app.GetUnits(), AeSys::kEngineering), ModelView->Position().y, 16, 8);
	SetDlgItemTextW(IDC_POSITION_Y, szBuf);
	app.FormatLength(szBuf, 32, max(app.GetUnits(), AeSys::kEngineering), ModelView->Position().z, 16, 8);
	SetDlgItemTextW(IDC_POSITION_Z, szBuf);

	app.FormatLength(szBuf, 32, max(app.GetUnits(), AeSys::kEngineering), ModelView->Target().x, 16, 8);
	SetDlgItemTextW(IDC_TARGET_X, szBuf);
	app.FormatLength(szBuf, 32, max(app.GetUnits(), AeSys::kEngineering), ModelView->Target().y, 16, 8);
	SetDlgItemTextW(IDC_TARGET_Y, szBuf);
	app.FormatLength(szBuf, 32, max(app.GetUnits(), AeSys::kEngineering), ModelView->Target().z, 16, 8);
	SetDlgItemTextW(IDC_TARGET_Z, szBuf);

	app.FormatLength(szBuf, 32, max(app.GetUnits(), AeSys::kEngineering), ModelView->FrontClipDistance(), 16, 8);
	SetDlgItemTextW(IDC_FRONT_CLIP_DISTANCE, szBuf);
	app.FormatLength(szBuf, 32, max(app.GetUnits(), AeSys::kEngineering), ModelView->BackClipDistance(), 16, 8);
	SetDlgItemTextW(IDC_BACK_CLIP_DISTANCE, szBuf);

	app.FormatLength(szBuf, 32, max(app.GetUnits(), AeSys::kEngineering), ModelView->LensLength(), 16, 8);
	SetDlgItemTextW(IDC_LENS_LENGTH, szBuf);

	GetDlgItem(IDC_APPLY)->EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
void ViewParametersDialog::OnOK()
{
	OnBnClickedApply();

	CDialog::OnOK();
}
void ViewParametersDialog::OnEnChangePositionX()
{
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void ViewParametersDialog::OnEnChangePositionY()
{
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void ViewParametersDialog::OnEnChangePositionZ()
{
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void ViewParametersDialog::OnEnChangeTargetX()
{
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void ViewParametersDialog::OnEnChangeTargetY()
{
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void ViewParametersDialog::OnEnChangeTargetZ()
{
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void ViewParametersDialog::OnEnChangeFrontClipDistance()
{
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void ViewParametersDialog::OnEnChangeBackClipDistance()
{
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void ViewParametersDialog::OnEnChangeLensLength()
{
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
void ViewParametersDialog::OnBnClickedPerspectiveProjection()
{
	GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}
