#include "stdafx.h"
#include "EoDlgSelectIsometricView.h"
#include "Resource.h"

IMPLEMENT_DYNAMIC(EoDlgSelectIsometricView, CDialog)

EoDlgSelectIsometricView::EoDlgSelectIsometricView(CWnd* pParent /*=nullptr*/)
	: CDialog(EoDlgSelectIsometricView::IDD, pParent) {

}

EoDlgSelectIsometricView::~EoDlgSelectIsometricView() {
}

void EoDlgSelectIsometricView::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Radio(dataExchange, IDC_VIEW_ISO_LEFT, m_LeftRight);
	DDX_Radio(dataExchange, IDC_VIEW_ISO_FRONT, m_FrontBack);
	DDX_Radio(dataExchange, IDC_VIEW_ISO_ABOVE, m_AboveUnder);
}

// EoDlgSelectIsometricView message handlers

BOOL EoDlgSelectIsometricView::OnInitDialog() {
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
