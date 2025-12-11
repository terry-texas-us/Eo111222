#include "stdafx.h"

#include "DlgSetText.h"

// CDlgSetText dialog

IMPLEMENT_DYNAMIC(CDlgSetText, CDialog)

CDlgSetText::CDlgSetText(CWnd* pParent /*=NULL*/) : 
	CDialog(CDlgSetText::IDD, pParent) 
{
}
CDlgSetText::~CDlgSetText() 
{
}
void CDlgSetText::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_TEXT, m_sText);
}
BOOL CDlgSetText::OnInitDialog(void)
{
	if (!m_strTitle.IsEmpty())
	{
		SetWindowText(m_strTitle);
	}
	CDialog::OnInitDialog();
	return TRUE;
}
BEGIN_MESSAGE_MAP(CDlgSetText, CDialog)
END_MESSAGE_MAP()

// CDlgSetText message handlers

