#include "stdafx.h"

#include "DlgSetAngle.h"

// CDlgSetAngle dialog

IMPLEMENT_DYNAMIC(CDlgSetAngle, CDialog)

CDlgSetAngle::CDlgSetAngle(CWnd* pParent /*=NULL*/) :
	CDialog(CDlgSetAngle::IDD, pParent),
	m_dAngle(0)
{
}
CDlgSetAngle::~CDlgSetAngle() 
{
}
void CDlgSetAngle::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ANGLE, m_dAngle);
	DDV_MinMaxDouble(pDX, m_dAngle, - 360., 360.);
}
BOOL CDlgSetAngle::OnInitDialog()
{
	CDialog::OnInitDialog();
	if (!m_strTitle.IsEmpty())
	{
		SetWindowText(m_strTitle);
	}
	return TRUE;
}
BEGIN_MESSAGE_MAP(CDlgSetAngle, CDialog)
END_MESSAGE_MAP()
