#include "stdafx.h"

#include "EoDlgSetText.h"

// EoDlgSetText dialog

IMPLEMENT_DYNAMIC(EoDlgSetText, CDialog)

EoDlgSetText::EoDlgSetText(CWnd* pParent /*=nullptr*/) :
	CDialog(EoDlgSetText::IDD, pParent) {
}
EoDlgSetText::~EoDlgSetText() {
}
void EoDlgSetText::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Text(dataExchange, IDC_TEXT, m_sText);
}
BOOL EoDlgSetText::OnInitDialog() {
	if (!m_strTitle.IsEmpty()) {
		SetWindowTextW(m_strTitle);
	}
	CDialog::OnInitDialog();
	return TRUE;
}

