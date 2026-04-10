#include "Stdafx.h"

#include "EoDlgSheetSetupFormFactor.h"
#include <iterator>

IMPLEMENT_DYNAMIC(EoDlgSheetSetupFormFactor, CDialog)

EoDlgSheetSetupFormFactor::EoDlgSheetSetupFormFactor(CWnd* parent)
    : CDialog(IDD, parent) {}

void EoDlgSheetSetupFormFactor::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
}

BEGIN_MESSAGE_MAP(EoDlgSheetSetupFormFactor, CDialog)
END_MESSAGE_MAP()

BOOL EoDlgSheetSetupFormFactor::OnInitDialog() {
  CDialog::OnInitDialog();

  // Populate the sheet designation combo
  auto* comboBox = static_cast<CComboBox*>(GetDlgItem(IDC_SHEET_DESIGNATION));
  if (comboBox != nullptr) {
    for (const auto& sheetSize : kSheetSizes) {
      comboBox->AddString(sheetSize.label);
    }
    comboBox->SetCurSel(m_designationIndex);
  }

  // Set orientation radio buttons
  CheckRadioButton(IDC_ORIENTATION_LANDSCAPE, IDC_ORIENTATION_PORTRAIT,
      m_isLandscape ? IDC_ORIENTATION_LANDSCAPE : IDC_ORIENTATION_PORTRAIT);

  return TRUE;
}

void EoDlgSheetSetupFormFactor::OnOK() {
  auto* comboBox = static_cast<CComboBox*>(GetDlgItem(IDC_SHEET_DESIGNATION));
  if (comboBox != nullptr) { m_designationIndex = comboBox->GetCurSel(); }

  if (m_designationIndex < 0 || m_designationIndex >= static_cast<int>(std::size(kSheetSizes))) {
    m_designationIndex = 4;  // Default to ARCH E
  }

  m_isLandscape = IsDlgButtonChecked(IDC_ORIENTATION_LANDSCAPE) == BST_CHECKED;

  const auto& size = kSheetSizes[m_designationIndex];
  if (m_isLandscape) {
    m_sheetWidth = size.width;
    m_sheetHeight = size.height;
  } else {
    // Portrait: swap width and height
    m_sheetWidth = size.height;
    m_sheetHeight = size.width;
  }

  CDialog::OnOK();
}
