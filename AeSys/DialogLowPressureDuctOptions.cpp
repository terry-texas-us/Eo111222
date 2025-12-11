#include "stdafx.h"
#include "AeSys.h"

#include "DialogLowPressureDuctOptions.h"

// DialogLowPressureDuctOptions dialog

IMPLEMENT_DYNAMIC(DialogLowPressureDuctOptions, CDialog)

BEGIN_MESSAGE_MAP(DialogLowPressureDuctOptions, CDialog)
	ON_BN_CLICKED(IDOK, &DialogLowPressureDuctOptions::OnBnClickedOk)
	ON_BN_CLICKED(IDC_GEN_VANES, &DialogLowPressureDuctOptions::OnBnClickedGenVanes)
END_MESSAGE_MAP()

DialogLowPressureDuctOptions::DialogLowPressureDuctOptions(CWnd* pParent /*=NULL*/)
	: CDialog(DialogLowPressureDuctOptions::IDD, pParent)
	, m_Width(0)
	, m_Depth(0)
	, m_RadiusFactor(0)
	, m_Justification(0)
{
}
DialogLowPressureDuctOptions::~DialogLowPressureDuctOptions()
{
}
void DialogLowPressureDuctOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_RADIUS_FACTOR, m_RadiusFactor);
}
BOOL DialogLowPressureDuctOptions::OnInitDialog()
{
	WCHAR szBuf[32];
	
	CDialog::OnInitDialog();
	
	AeSys::Units Units = max(app.GetUnits(), AeSys::kInches);
	app.FormatLength(szBuf, 32, Units, m_Width, 12, 2);
	SetDlgItemTextW(IDC_WIDTH, szBuf);
	app.FormatLength(szBuf, 32, Units, m_Depth, 12, 2);
	SetDlgItemTextW(IDC_DEPTH, szBuf);
	CheckRadioButton(IDC_LEFT, IDC_RIGHT, IDC_CENTER + m_Justification);
	CheckDlgButton(IDC_GEN_VANES, m_GenerateVanes ? 1 : 0);
	CheckDlgButton(IDC_BEGINWITHTRANSITION, m_BeginWithTransition ? 1 : 0);
	return TRUE;
}

// DialogLowPressureDuctOptions message handlers

void DialogLowPressureDuctOptions::OnBnClickedOk()
{
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
void DialogLowPressureDuctOptions::OnBnClickedGenVanes()
{
	// TODO: Add your control notification handler code here
}
