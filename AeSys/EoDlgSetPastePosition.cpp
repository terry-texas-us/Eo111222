#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "EoDlgSetPastePosition.h"

EoDlgSetPastePosition::EoDlgSetPastePosition(CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSetPastePosition::IDD, pParent) {}

EoDlgSetPastePosition::~EoDlgSetPastePosition() {}

void EoDlgSetPastePosition::DoDataExchange(CDataExchange* dataExchange) { CDialog::DoDataExchange(dataExchange); }

void EoDlgSetPastePosition::OnOK() {
  AeSysDoc::GetDoc()->SetTrapPivotPoint(app.GetCursorPosition());

  CDialog::OnOK();
}
