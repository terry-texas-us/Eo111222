#include "stdafx.h"

#include "EoDlgSetupNote.h"

// EoDlgSetupNote dialog

IMPLEMENT_DYNAMIC(EoDlgSetupNote, CDialog)

EoDlgSetupNote::EoDlgSetupNote(CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSetupNote::IDD, pParent),
      m_TextHeight(0),
      m_TextExpansionFactor(0),
      m_CharacterSlantAngle(0),
      m_TextRotationAngle(0) {}

EoDlgSetupNote::EoDlgSetupNote(EoDbFontDefinition* fontDefinition, CWnd* pParent /*= nullptr*/)
    : CDialog(EoDlgSetupNote::IDD, pParent),
      m_FontDefinition(fontDefinition),
      m_TextHeight(0),
      m_TextExpansionFactor(0),
      m_CharacterSlantAngle(0),
      m_TextRotationAngle(0) {}

EoDlgSetupNote::~EoDlgSetupNote() {}
void EoDlgSetupNote::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Text(dataExchange, IDC_TEXT_HEIGHT, m_TextHeight);
  DDX_Text(dataExchange, IDC_TEXT_EXP_FAC, m_TextExpansionFactor);
  DDX_Text(dataExchange, IDC_TEXT_INCLIN, m_CharacterSlantAngle);
  DDX_Text(dataExchange, IDC_TEXT_ROTATION, m_TextRotationAngle);
  DDX_Control(dataExchange, IDC_MFCFONTCOMBO, m_MfcFontComboControl);
}

BOOL EoDlgSetupNote::OnInitDialog() {
  CDialog::OnInitDialog();

  m_MfcFontComboControl.AddString(L"Simplex.psf");
  m_MfcFontComboControl.SelectString(-1, m_FontDefinition->FontName());

  CString Spacing;
  Spacing.Format(L"%8.4f", m_FontDefinition->CharacterSpacing());
  SetDlgItemTextW(IDC_TEXT_SPACING, Spacing);

  CheckRadioButton(IDC_TEXT_ALIGN_HOR_LEFT, IDC_TEXT_ALIGN_HOR_RIGHT,
                   IDC_TEXT_ALIGN_HOR_LEFT + m_FontDefinition->HorizontalAlignment() - 1);
  CheckRadioButton(IDC_TEXT_ALIGN_VER_BOT, IDC_TEXT_ALIGN_VER_TOP,
                   IDC_TEXT_ALIGN_VER_BOT - m_FontDefinition->VerticalAlignment() + 4);
  CheckRadioButton(IDC_PATH_RIGHT, IDC_PATH_DOWN, IDC_PATH_RIGHT + m_FontDefinition->Path());

  return TRUE;
}
void EoDlgSetupNote::OnOK() {
  CString Spacing;
  GetDlgItemTextW(IDC_TEXT_SPACING, Spacing);
  m_FontDefinition->CharacterSpacing(_wtof(Spacing));

  EoUInt16 HorizontalAlignment =
      EoUInt16(1 - IDC_TEXT_ALIGN_HOR_LEFT + GetCheckedRadioButton(IDC_TEXT_ALIGN_HOR_LEFT, IDC_TEXT_ALIGN_HOR_RIGHT));
  m_FontDefinition->HorizontalAlignment(HorizontalAlignment);

  EoUInt16 VerticalAlignment =
      EoUInt16(4 + IDC_TEXT_ALIGN_VER_BOT - GetCheckedRadioButton(IDC_TEXT_ALIGN_VER_BOT, IDC_TEXT_ALIGN_VER_TOP));
  m_FontDefinition->VerticalAlignment(VerticalAlignment);

  EoUInt16 Path = EoUInt16(GetCheckedRadioButton(IDC_PATH_RIGHT, IDC_PATH_DOWN) - IDC_PATH_RIGHT);
  m_FontDefinition->Path(Path);

  int FontsIndex = m_MfcFontComboControl.GetCurSel();
  if (FontsIndex != CB_ERR) {
    CString FontsItemName;
    m_MfcFontComboControl.GetLBText(FontsIndex, FontsItemName);
    m_FontDefinition->FontName(FontsItemName);
    EoUInt16 Precision =
        EoUInt16(FontsItemName.CompareNoCase(L"Simplex.psf") != 0 ? EoDb::kEoTrueType : EoDb::kStrokeType);
    m_FontDefinition->Precision(Precision);
  }

  CDialog::OnOK();
}
