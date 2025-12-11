// SelectIsometricViewDialog.cpp : implementation file
//

#include "stdafx.h"
#include "SelectIsometricViewDialog.h"


// SelectIsometricViewDialog dialog

IMPLEMENT_DYNAMIC(SelectIsometricViewDialog, CDialog)

BEGIN_MESSAGE_MAP(SelectIsometricViewDialog, CDialog)
END_MESSAGE_MAP()

SelectIsometricViewDialog::SelectIsometricViewDialog(CWnd* pParent /*=NULL*/)
	: CDialog(SelectIsometricViewDialog::IDD, pParent)
{

}

SelectIsometricViewDialog::~SelectIsometricViewDialog()
{
}

void SelectIsometricViewDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_VIEW_ISO_LEFT, m_LeftRight);
	DDX_Radio(pDX, IDC_VIEW_ISO_FRONT, m_FrontBack);
	DDX_Radio(pDX, IDC_VIEW_ISO_ABOVE, m_AboveUnder);
}

// SelectIsometricViewDialog message handlers

BOOL SelectIsometricViewDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
