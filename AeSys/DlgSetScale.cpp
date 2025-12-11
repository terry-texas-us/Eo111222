#include "stdafx.h"

#include "DlgSetScale.h"

// CDlgSetScale dialog

IMPLEMENT_DYNAMIC(CDlgSetScale, CDialog)

CDlgSetScale::CDlgSetScale(CWnd* pParent /*=NULL*/) : 
	CDialog(CDlgSetScale::IDD, pParent), m_Scale(0)
{
}
CDlgSetScale::~CDlgSetScale()
{
}
void CDlgSetScale::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SCALE, m_Scale);
	DDV_MinMaxDouble(pDX, m_Scale, .0001, 10000.);
}

BEGIN_MESSAGE_MAP(CDlgSetScale, CDialog)
END_MESSAGE_MAP()
