#include "Stdafx.h"

#include <algorithm>

#include "AeSys.h"
#include "Eo.h"
#include "EoDlgSetLength.h"

EoDlgSetLength::EoDlgSetLength(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgSetLength::IDD, pParent) {}

EoDlgSetLength::~EoDlgSetLength() {}

void EoDlgSetLength::DoDataExchange(CDataExchange* dataExchange) { CDialog::DoDataExchange(dataExchange); }

BOOL EoDlgSetLength::OnInitDialog() {
  CDialog::OnInitDialog();
  if (!m_title.IsEmpty()) { SetWindowTextW(m_title); }
  CString length;
  app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Engineering), m_length);
  SetDlgItemTextW(IDC_DISTANCE, length);
  return TRUE;
}

void EoDlgSetLength::OnOK() {
  wchar_t itemText[32]{};

  GetDlgItemTextW(IDC_DISTANCE, itemText, 32);
  m_length = app.ParseLength(app.GetUnits(), itemText);

  CDialog::OnOK();
}
