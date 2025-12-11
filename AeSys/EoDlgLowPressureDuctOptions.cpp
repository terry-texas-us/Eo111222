#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

#include "EoDlgLowPressureDuctOptions.h"

// EoDlgLowPressureDuctOptions dialog

IMPLEMENT_DYNAMIC(EoDlgLowPressureDuctOptions, CDialog)

BEGIN_MESSAGE_MAP(EoDlgLowPressureDuctOptions, CDialog)
	ON_BN_CLICKED(IDOK, &EoDlgLowPressureDuctOptions::OnBnClickedOk)
	ON_BN_CLICKED(IDC_GEN_VANES, &EoDlgLowPressureDuctOptions::OnBnClickedGenVanes)
END_MESSAGE_MAP()

EoDlgLowPressureDuctOptions::EoDlgLowPressureDuctOptions(CWnd* pParent /*=NULL*/)
	: CDialog(EoDlgLowPressureDuctOptions::IDD, pParent)
	, m_Width(0), m_Depth(0), m_RadiusFactor(0), m_Justification(0) {
}
EoDlgLowPressureDuctOptions::~EoDlgLowPressureDuctOptions() {
}
void EoDlgLowPressureDuctOptions::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_RADIUS_FACTOR, m_RadiusFactor);
}
BOOL EoDlgLowPressureDuctOptions::OnInitDialog() {
	CDialog::OnInitDialog();

	AeSys::Units Units = max(app.GetUnits(), AeSys::kInches);

	CString Length;
	app.FormatLength(Length, Units, m_Width, 12, 3);
	SetDlgItemTextW(IDC_WIDTH, Length);
	app.FormatLength(Length, Units, m_Depth, 12, 3);
	SetDlgItemTextW(IDC_DEPTH, Length);
	CheckRadioButton(IDC_LEFT, IDC_RIGHT, IDC_CENTER + m_Justification);
	CheckDlgButton(IDC_GEN_VANES, m_GenerateVanes ? 1 : 0);
	CheckDlgButton(IDC_BEGINWITHTRANSITION, m_BeginWithTransition ? 1 : 0);
	return TRUE;
}

// EoDlgLowPressureDuctOptions message handlers

void EoDlgLowPressureDuctOptions::OnBnClickedOk() {
	WCHAR szBuf[32];

	GetDlgItemTextW(IDC_WIDTH, szBuf, 32);
	m_Width = app.ParseLength(app.GetUnits(), szBuf);
	GetDlgItemTextW(IDC_DEPTH, szBuf, 32);
	m_Depth = app.ParseLength(app.GetUnits(), szBuf);
	m_Justification = GetCheckedRadioButton(IDC_LEFT, IDC_RIGHT) - IDC_CENTER;
	m_GenerateVanes = IsDlgButtonChecked(IDC_GEN_VANES) == 0 ? false : true;
	m_BeginWithTransition = IsDlgButtonChecked(IDC_BEGINWITHTRANSITION) == 0 ? false : true;

	OnOK();
}
void EoDlgLowPressureDuctOptions::OnBnClickedGenVanes() {
	// TODO: Add your control notification handler code here
}
