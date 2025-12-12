#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"

#include "EoDlgSetPastePosition.h"

// EoDlgSetPastePosition dialog

IMPLEMENT_DYNAMIC(EoDlgSetPastePosition, CDialog)

EoDlgSetPastePosition::EoDlgSetPastePosition(CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgSetPastePosition::IDD, pParent) {
}
EoDlgSetPastePosition::~EoDlgSetPastePosition() {
}
void EoDlgSetPastePosition::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
}
void EoDlgSetPastePosition::OnOK() {
	AeSysDoc::GetDoc()->SetTrapPivotPoint(app.GetCursorPosition());

	CDialog::OnOK();
}
