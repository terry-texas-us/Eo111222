#include "Stdafx.h"

#include "Eo.h"
#include "EoDb.h"
#include "EoDbFontDefinition.h"
#include "EoDlgSetupNote.h"
#include "Resource.h"

EoDlgSetupNote::EoDlgSetupNote(CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSetupNote::IDD, pParent), m_height{}, m_expansionFactor{}, m_slantAngle{}, m_rotationAngle{} {}

EoDlgSetupNote::EoDlgSetupNote(EoDbFontDefinition* fontDefinition, CWnd* pParent /*= nullptr*/)
    : CDialog(EoDlgSetupNote::IDD, pParent),
      m_FontDefinition(fontDefinition),
      m_height{},
      m_expansionFactor{},
      m_slantAngle{},
      m_rotationAngle{} {}

EoDlgSetupNote::~EoDlgSetupNote() {}
void EoDlgSetupNote::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Text(dataExchange, IDC_TEXT_HEIGHT, m_height);
  DDX_Text(dataExchange, IDC_TEXT_EXP_FAC, m_expansionFactor);
  DDX_Text(dataExchange, IDC_TEXT_INCLIN, m_slantAngle);
  DDX_Text(dataExchange, IDC_TEXT_ROTATION, m_rotationAngle);
  DDX_Control(dataExchange, IDC_MFCFONTCOMBO, m_MfcFontComboControl);
}

BOOL EoDlgSetupNote::OnInitDialog() {
  CDialog::OnInitDialog();

  m_MfcFontComboControl.AddString(Eo::defaultStrokeFont);
  m_MfcFontComboControl.SelectString(-1, m_FontDefinition->FontName());

  CString spacing;
  spacing.Format(L"%8.4f", m_FontDefinition->CharacterSpacing());
  SetDlgItemTextW(IDC_TEXT_SPACING, spacing);

  CheckRadioButton(IDC_TEXT_ALIGN_HOR_LEFT, IDC_TEXT_ALIGN_HOR_RIGHT,
                   IDC_TEXT_ALIGN_HOR_LEFT + static_cast<int>(m_FontDefinition->HorizontalAlignment()) - 1);
  CheckRadioButton(IDC_TEXT_ALIGN_VER_BOT, IDC_TEXT_ALIGN_VER_TOP,
                   IDC_TEXT_ALIGN_VER_BOT - static_cast<int>(m_FontDefinition->VerticalAlignment()) + 4);
  CheckRadioButton(IDC_PATH_RIGHT, IDC_PATH_DOWN, IDC_PATH_RIGHT + static_cast<int>(m_FontDefinition->Path()));

  return TRUE;
}

void EoDlgSetupNote::OnOK() {
  CString spacing;
  GetDlgItemTextW(IDC_TEXT_SPACING, spacing);
  m_FontDefinition->SetCharacterSpacing(_wtof(spacing));

  auto horizontalAlignment = static_cast<EoDb::HorizontalAlignment>(
      1 - IDC_TEXT_ALIGN_HOR_LEFT + GetCheckedRadioButton(IDC_TEXT_ALIGN_HOR_LEFT, IDC_TEXT_ALIGN_HOR_RIGHT));
  m_FontDefinition->SetHorizontalAlignment(horizontalAlignment);

  auto verticalAlignment = static_cast<EoDb::VerticalAlignment>(
      4 + IDC_TEXT_ALIGN_VER_BOT - GetCheckedRadioButton(IDC_TEXT_ALIGN_VER_BOT, IDC_TEXT_ALIGN_VER_TOP));
  m_FontDefinition->SetVerticalAlignment(verticalAlignment);

  auto path = static_cast<EoDb::Path>(GetCheckedRadioButton(IDC_PATH_RIGHT, IDC_PATH_DOWN) - IDC_PATH_RIGHT);
  m_FontDefinition->SetPath(path);

  int fontsIndex = m_MfcFontComboControl.GetCurSel();
  if (fontsIndex != CB_ERR) {
    CString fontsItemName;
    m_MfcFontComboControl.GetLBText(fontsIndex, fontsItemName);
    m_FontDefinition->SetFontName(fontsItemName);
    auto precision = fontsItemName.CompareNoCase(Eo::defaultStrokeFont) != 0 ? EoDb::Precision::TrueType
                                                                             : EoDb::Precision::StrokeType;
    m_FontDefinition->SetPrecision(precision);
  }
  CDialog::OnOK();
}
