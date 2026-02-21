#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "EoDbGroup.h"
#include "EoDbText.h"
#include "EoDlgModeLetter.h"
#include "EoGeReferenceSystem.h"
#include "EoGsRenderState.h"
#include "Resource.h"

BEGIN_MESSAGE_MAP(EoDlgModeLetter, CDialog)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_SIZE()
#pragma warning(pop)
END_MESSAGE_MAP()

EoGePoint3d EoDlgModeLetter::m_Point = EoGePoint3d::kOrigin;

EoDlgModeLetter::EoDlgModeLetter(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgModeLetter::IDD, pParent) {}
EoDlgModeLetter::~EoDlgModeLetter() {}
void EoDlgModeLetter::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_TEXT, m_TextEditControl);
}
BOOL EoDlgModeLetter::OnInitDialog() {
  CDialog::OnInitDialog();

  m_Point = app.GetCursorPosition();

  return TRUE;
}
void EoDlgModeLetter::OnOK() {
  auto characterCellDefinition = renderState.CharacterCellDefinition();
  EoGeReferenceSystem ReferenceSystem(m_Point, characterCellDefinition);

  const auto& fontDefinition = renderState.FontDefinition();

  if (m_TextEditControl.GetWindowTextLengthW() != 0) {
    CString Text;
    m_TextEditControl.GetWindowTextW(Text);
    m_TextEditControl.SetWindowTextW(L"");

    EoDbText* TextPrimitive = new EoDbText(fontDefinition, ReferenceSystem, Text);
    auto* Group = new EoDbGroup(TextPrimitive);
    AeSysDoc::GetDoc()->AddWorkLayerGroup(Group);
    AeSysDoc::GetDoc()->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
  }
  m_Point = text_GetNewLinePos(fontDefinition, ReferenceSystem, 1.0, 0);
  m_TextEditControl.SetFocus();

  CDialog::OnOK();
}
void EoDlgModeLetter::OnSize(UINT type, int cx, int cy) {
  CDialog::OnSize(type, cx, cy);

  if (::IsWindow(m_TextEditControl.GetSafeHwnd())) { m_TextEditControl.MoveWindow(0, 0, cx, cy, TRUE); }
}
