#include "stdafx.h"

#include "DlgSetLength.h"

// CDlgSetLength dialog

IMPLEMENT_DYNAMIC(CDlgSetLength, CDialog)

CDlgSetLength::CDlgSetLength(CWnd* pParent /*=NULL*/) :
	CDialog(CDlgSetLength::IDD, pParent)
{
}
CDlgSetLength::~CDlgSetLength()
{
}
void CDlgSetLength::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}
BOOL CDlgSetLength::OnInitDialog()
{
	TCHAR szBuf[32];
	
	CDialog::OnInitDialog();
	if (!m_strTitle.IsEmpty()) 
	{
		SetWindowText(m_strTitle);
	}
	app.FormatLength(szBuf, 32, max(app.GetUnits(), AeSys::kEngineering), m_dLength, 16, 8);
	SetDlgItemText(IDC_DISTANCE, szBuf);
	return TRUE;
}
void CDlgSetLength::OnOK()
{
	TCHAR szBuf[32];
	
	GetDlgItemText(IDC_DISTANCE, (LPTSTR) szBuf, 32);
	m_dLength = app.ParseLength(app.GetUnits(), szBuf);
	
	CDialog::OnOK();
}
BEGIN_MESSAGE_MAP(CDlgSetLength, CDialog)
END_MESSAGE_MAP()