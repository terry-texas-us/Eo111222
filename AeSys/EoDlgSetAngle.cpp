#include "stdafx.h"

#include "EoDlgSetAngle.h"

// EoDlgSetAngle dialog

IMPLEMENT_DYNAMIC(EoDlgSetAngle, CDialog)

EoDlgSetAngle::EoDlgSetAngle(CWnd* pParent /*=nullptr*/) :
	CDialog(EoDlgSetAngle::IDD, pParent), m_dAngle(0) {
}
EoDlgSetAngle::~EoDlgSetAngle() {
}
void EoDlgSetAngle::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Text(dataExchange, IDC_ANGLE, m_dAngle);
	DDV_MinMaxDouble(dataExchange, m_dAngle, - 360.0, 360.0);
}
BOOL EoDlgSetAngle::OnInitDialog() {
	CDialog::OnInitDialog();
	if (!m_strTitle.IsEmpty()) {
		SetWindowTextW(m_strTitle);
	}
	return TRUE;
}
