#include "stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"

#include "EoDlgSetupConstraints.h"

// EoDlgSetContraints dialog

IMPLEMENT_DYNAMIC(EoDlgSetupConstraints, CDialog)

EoDlgSetupConstraints::EoDlgSetupConstraints(CWnd* pParent /* = NULL */) :
	CDialog(EoDlgSetupConstraints::IDD, pParent) {
}
EoDlgSetupConstraints::EoDlgSetupConstraints(AeSysView* view, CWnd* pParent /* = NULL */) :
	CDialog(EoDlgSetupConstraints::IDD, pParent), m_ActiveView(view) {
}
EoDlgSetupConstraints::~EoDlgSetupConstraints() {
}
void EoDlgSetupConstraints::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Control(dataExchange, IDC_USR_GRID_X_INT, m_GridXSnapSpacing);
	DDX_Control(dataExchange, IDC_USR_GRID_Y_INT, m_GridYSnapSpacing);
	DDX_Control(dataExchange, IDC_USR_GRID_Z_INT, m_GridZSnapSpacing);
	DDX_Control(dataExchange, IDC_GRID_DOT_INT_X, m_GridXPointSpacing);
	DDX_Control(dataExchange, IDC_GRID_DOT_INT_Y, m_GridYPointSpacing);
	DDX_Control(dataExchange, IDC_GRID_DOT_INT_Z, m_GridZPointSpacing);
	DDX_Control(dataExchange, IDC_GRID_LN_INT_X, m_GridXLineSpacing);
	DDX_Control(dataExchange, IDC_GRID_LN_INT_Y, m_GridYLineSpacing);
	DDX_Control(dataExchange, IDC_GRID_LN_INT_Z, m_GridZLineSpacing);
	DDX_Control(dataExchange, IDC_GRID_SNAP_ON, m_GridSnapEnableCheckBox);
	DDX_Control(dataExchange, IDC_GRID_PTS_DIS_ON, m_GridDisplayCheckBox);
	DDX_Control(dataExchange, IDC_GRID_LNS_DIS_ON, m_GridLineDisplayCheckBox);
	DDX_Control(dataExchange, IDC_USR_AX_INF_ANG, m_AxisInfluenceAngle);
	DDX_Control(dataExchange, IDC_USR_AX_Z_OFF_ANG, m_AxisZOffsetAngle);
}

BOOL EoDlgSetupConstraints::OnInitDialog() {
	CDialog::OnInitDialog();

	AeSys::Units CurrentUnits = app.GetUnits();

	double x, y, z;

	m_ActiveView->GetGridSnapSpacing(x, y, z);

	CString Length;
	app.FormatLength(Length, CurrentUnits, x);
	m_GridXSnapSpacing.SetWindowTextW(Length);
	app.FormatLength(Length, CurrentUnits, y);
	m_GridYSnapSpacing.SetWindowTextW(Length);
	app.FormatLength(Length, CurrentUnits, z);
	m_GridZSnapSpacing.SetWindowTextW(Length);

	m_ActiveView->GetGridPointSpacing(x, y, z);

	app.FormatLength(Length, CurrentUnits, x);
	m_GridXPointSpacing.SetWindowTextW(Length);
	app.FormatLength(Length, CurrentUnits, y);
	m_GridYPointSpacing.SetWindowTextW(Length);
	app.FormatLength(Length, CurrentUnits, z);
	m_GridZPointSpacing.SetWindowTextW(Length);

	m_ActiveView->GetGridLineSpacing(x, y, z);

	app.FormatLength(Length, CurrentUnits, x);
	m_GridXLineSpacing.SetWindowTextW(Length);
	app.FormatLength(Length, CurrentUnits, y);
	m_GridYLineSpacing.SetWindowTextW(Length);
	app.FormatLength(Length, CurrentUnits, z);
	m_GridZLineSpacing.SetWindowTextW(Length);

	m_GridSnapEnableCheckBox.SetCheck(m_ActiveView->GridSnap() ? BST_CHECKED : BST_UNCHECKED);
	m_GridDisplayCheckBox.SetCheck(m_ActiveView->DisplayGridWithPoints() ? BST_CHECKED : BST_UNCHECKED);
	m_GridLineDisplayCheckBox.SetCheck(m_ActiveView->DisplayGridWithLines() ? BST_CHECKED : BST_UNCHECKED);

	CString Text;

	Text.Format(L"%f", m_ActiveView->AxisConstraintInfluenceAngle());
	m_AxisInfluenceAngle.SetWindowTextW(Text);

	Text.Format(L"%f", m_ActiveView->AxisConstraintOffsetAngle());
	m_AxisZOffsetAngle.SetWindowTextW(Text);

	return TRUE;
}

void EoDlgSetupConstraints::OnOK() {
	AeSys::Units CurrentUnits = app.GetUnits();

	WCHAR szBuf[32];

	double x, y, z;

	m_GridXSnapSpacing.GetWindowTextW(szBuf, 32);
	x = app.ParseLength(CurrentUnits, szBuf);
	m_GridYSnapSpacing.GetWindowTextW(szBuf, 32);
	y = app.ParseLength(CurrentUnits, szBuf);
	m_GridZSnapSpacing.GetWindowTextW(szBuf, 32);
	z = app.ParseLength(CurrentUnits, szBuf);
	m_ActiveView->SetGridSnapSpacing(x, y, z);

	m_GridXPointSpacing.GetWindowTextW(szBuf, 32);
	x = app.ParseLength(CurrentUnits, szBuf);
	m_GridYPointSpacing.GetWindowTextW(szBuf, 32);
	y = app.ParseLength(CurrentUnits, szBuf);
	m_GridZPointSpacing.GetWindowTextW(szBuf, 32);
	z = app.ParseLength(CurrentUnits, szBuf);
	m_ActiveView->SetGridPointSpacing(x, y, z);

	m_GridXLineSpacing.GetWindowTextW(szBuf, 32);
	x = app.ParseLength(CurrentUnits, szBuf);
	m_GridYLineSpacing.GetWindowTextW(szBuf, 32);
	y = app.ParseLength(CurrentUnits, szBuf);
	m_GridZLineSpacing.GetWindowTextW(szBuf, 32);
	z = app.ParseLength(CurrentUnits, szBuf);
	m_ActiveView->SetGridLineSpacing(x, y, z);

	m_ActiveView->EnableGridSnap(m_GridSnapEnableCheckBox.GetCheck() == BST_CHECKED);
	m_ActiveView->EnableDisplayGridWithPoints(m_GridDisplayCheckBox.GetCheck() == BST_CHECKED);
	m_ActiveView->EnableDisplayGridWithLines(m_GridLineDisplayCheckBox.GetCheck() == BST_CHECKED);

	m_AxisInfluenceAngle.GetWindowTextW(szBuf, 32);
	m_ActiveView->SetAxisConstraintInfluenceAngle(_wtof(szBuf));
	m_AxisZOffsetAngle.GetWindowTextW(szBuf, 32);
	m_ActiveView->SetAxisConstraintOffsetAngle(_wtof(szBuf));

	CDialog::OnOK();
}