#include "Stdafx.h"

#include "EoDlgAttributePrompt.h"

IMPLEMENT_DYNAMIC(EoDlgAttributePrompt, CDialog)

EoDlgAttributePrompt::EoDlgAttributePrompt(CWnd* parent)
    : CDialog(IDD, parent) {}

void EoDlgAttributePrompt::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
}

BEGIN_MESSAGE_MAP(EoDlgAttributePrompt, CDialog)
END_MESSAGE_MAP()

BOOL EoDlgAttributePrompt::OnInitDialog() {
  CDialog::OnInitDialog();

  SetDlgItemText(IDC_ATTRIB_BLOCK_NAME, m_blockName);
  SetDlgItemText(IDC_ATTRIB_TAG, m_tagName);
  SetDlgItemText(IDC_ATTRIB_PROMPT, m_promptString.IsEmpty() ? m_tagName : m_promptString);
  SetDlgItemText(IDC_ATTRIB_VALUE, m_defaultValue);

  // Select all text in the value edit so the user can type immediately
  auto* editControl = static_cast<CEdit*>(GetDlgItem(IDC_ATTRIB_VALUE));
  if (editControl != nullptr) {
    editControl->SetSel(0, -1);
    editControl->SetFocus();
  }

  return FALSE;  // We set focus explicitly
}

void EoDlgAttributePrompt::OnOK() {
  GetDlgItemText(IDC_ATTRIB_VALUE, m_enteredValue);
  CDialog::OnOK();
}
