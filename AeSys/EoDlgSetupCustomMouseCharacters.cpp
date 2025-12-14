#include "stdafx.h"

#include "AeSys.h"

#include "EoDlgSetupCustomMouseCharacters.h"

// EoDlgSetupCustomMouseCharacters dialog

IMPLEMENT_DYNAMIC(EoDlgSetupCustomMouseCharacters, CDialog)

EoDlgSetupCustomMouseCharacters::EoDlgSetupCustomMouseCharacters(CWnd* pParent /* = nullptr */) :
	CDialog(EoDlgSetupCustomMouseCharacters::IDD, pParent) {
}
EoDlgSetupCustomMouseCharacters::~EoDlgSetupCustomMouseCharacters() {
}
void EoDlgSetupCustomMouseCharacters::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
}

BOOL EoDlgSetupCustomMouseCharacters::OnInitDialog() {
	CDialog::OnInitDialog();

	SetDlgItemTextW(IDC_LEFT_DOWN, app.CustomLButtonDownCharacters);
	SetDlgItemTextW(IDC_LEFT_UP, app.CustomLButtonUpCharacters);
	SetDlgItemTextW(IDC_RIGHT_DOWN, app.CustomRButtonDownCharacters);
	SetDlgItemTextW(IDC_RIGHT_UP, app.CustomRButtonUpCharacters);

	return TRUE;
}

void EoDlgSetupCustomMouseCharacters::OnOK() {
	GetDlgItemTextW(IDC_LEFT_DOWN, app.CustomLButtonDownCharacters);
	GetDlgItemTextW(IDC_LEFT_UP, app.CustomLButtonUpCharacters);
	GetDlgItemTextW(IDC_RIGHT_DOWN, app.CustomRButtonDownCharacters);
	GetDlgItemTextW(IDC_RIGHT_UP, app.CustomRButtonUpCharacters);

	CDialog::OnOK();
}
