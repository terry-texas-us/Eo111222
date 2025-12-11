#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

#include "EoDlgAnnotateOptions.h"

// EoDlgAnnotateOptions dialog

IMPLEMENT_DYNAMIC(EoDlgAnnotateOptions, CDialog)

BEGIN_MESSAGE_MAP(EoDlgAnnotateOptions, CDialog)
END_MESSAGE_MAP()

EoDlgAnnotateOptions::EoDlgAnnotateOptions(CWnd* pParent /* = NULL */) : 
CDialog(EoDlgAnnotateOptions::IDD, pParent)
{
}
EoDlgAnnotateOptions::EoDlgAnnotateOptions(AeSysView* view, CWnd* pParent /* = NULL */) :
CDialog(EoDlgAnnotateOptions::IDD, pParent), m_ActiveView(view)
{
}
EoDlgAnnotateOptions::~EoDlgAnnotateOptions()
{
}
void EoDlgAnnotateOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ANN_ARR_TYP, m_EndItemTypeComboBox);
}
BOOL EoDlgAnnotateOptions::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetDlgItemTextW(IDC_ANN_DEF_TXT, m_ActiveView->DefaultText());

	CString Text;
	Text.Format(L"%8.4f", m_ActiveView->GapSpaceFactor());	
	SetDlgItemTextW(IDC_ANN_GAP_SPACE_FAC, Text);
	Text.Format(L"%8.4f", 	m_ActiveView->CircleRadius());
	SetDlgItemTextW(IDC_ANN_HOOK_RAD, Text);

	m_EndItemTypeComboBox.SetCurSel(m_ActiveView->EndItemType() - 1);

	Text.Format(L"%8.4f", 	m_ActiveView->EndItemSize());
	SetDlgItemTextW(IDC_ANN_ARR_SIZ, Text);
	Text.Format(L"%8.4f", 	m_ActiveView->BubbleRadius());
	SetDlgItemTextW(IDC_ANN_BUB_RAD, Text);
	SetDlgItemInt(IDC_ANN_BUB_FACETS, m_ActiveView->NumberOfSides(), FALSE);

	return TRUE;
}
void EoDlgAnnotateOptions::OnOK()
{
	CString Text;
	GetDlgItemTextW(IDC_ANN_DEF_TXT, Text);
	m_ActiveView->SetDefaultText(Text);

	GetDlgItemTextW(IDC_ANN_GAP_SPACE_FAC, Text);
	m_ActiveView->GapSpaceFactor(_wtof(Text));
	GetDlgItemTextW(IDC_ANN_HOOK_RAD, Text);
	m_ActiveView->CircleRadius(_wtof(Text));

	m_ActiveView->SetEndItemType(m_EndItemTypeComboBox.GetCurSel() + 1);

	GetDlgItemTextW(IDC_ANN_ARR_SIZ, Text);
	m_ActiveView->EndItemSize(_wtof(Text));

	GetDlgItemTextW(IDC_ANN_BUB_RAD, Text);
	m_ActiveView->BubbleRadius(_wtof(Text));
	m_ActiveView->NumberOfSides(GetDlgItemInt(IDC_ANN_BUB_FACETS, 0, FALSE));

	CDialog::OnOK();
}
