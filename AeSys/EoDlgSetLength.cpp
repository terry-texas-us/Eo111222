#include "Stdafx.h"

#include "AeSys.h"
#include "EoDlgSetLength.h"

IMPLEMENT_DYNAMIC(EoDlgSetLength, CDialog)

EoDlgSetLength::EoDlgSetLength(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgSetLength::IDD, pParent) {}
EoDlgSetLength::~EoDlgSetLength() {}
void EoDlgSetLength::DoDataExchange(CDataExchange* dataExchange) { CDialog::DoDataExchange(dataExchange); }
BOOL EoDlgSetLength::OnInitDialog() {
  CDialog::OnInitDialog();
  if (!m_strTitle.IsEmpty()) { SetWindowTextW(m_strTitle); }
  CString Length;
  app.FormatLength(Length, std::max(app.GetUnits(), Eo::Units::Engineering), m_dLength);
  SetDlgItemTextW(IDC_DISTANCE, Length);
  return TRUE;
}
void EoDlgSetLength::OnOK() {
  wchar_t itemText[32]{};

  GetDlgItemTextW(IDC_DISTANCE, itemText, 32);
  m_dLength = app.ParseLength(app.GetUnits(), itemText);

  CDialog::OnOK();
}
