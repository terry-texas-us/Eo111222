#include "stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDlgModeRevise.h"

// EoDlgModeRevise dialog
/// <remarks>
///Text related attributes for all notes generated will be same as those of the text last picked.
///Upon exit attributes restored to their entry values.
/// </remarks>

IMPLEMENT_DYNAMIC(EoDlgModeRevise, CDialog)

BEGIN_MESSAGE_MAP(EoDlgModeRevise, CDialog)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_SIZE()
#pragma warning(pop)
END_MESSAGE_MAP()

EoDbFontDefinition EoDlgModeRevise::sm_FontDefinition;
EoGeReferenceSystem EoDlgModeRevise::sm_ReferenceSystem;
EoDbText* EoDlgModeRevise::sm_TextPrimitive;

EoDlgModeRevise::EoDlgModeRevise(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgModeRevise::IDD, pParent) {}
EoDlgModeRevise::~EoDlgModeRevise() {}
void EoDlgModeRevise::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_TEXT, m_TextEditControl);
}
BOOL EoDlgModeRevise::OnInitDialog() {
  CDialog::OnInitDialog();

  sm_TextPrimitive = AeSysView::GetActiveView()->SelectTextUsingPoint(app.GetCursorPosition());
  if (sm_TextPrimitive != 0) {
    sm_TextPrimitive->GetFontDef(sm_FontDefinition);
    sm_TextPrimitive->GetRefSys(sm_ReferenceSystem);
    m_TextEditControl.SetWindowTextW(sm_TextPrimitive->Text());
  } else {
    EndDialog(TRUE);
  }
  return TRUE;
}
void EoDlgModeRevise::OnOK() {
  CString Text;
  m_TextEditControl.GetWindowTextW(Text);

  if (sm_TextPrimitive != 0) {
    AeSysDoc::GetDoc()->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, sm_TextPrimitive);
    sm_TextPrimitive->SetText(Text);
    AeSysDoc::GetDoc()->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, sm_TextPrimitive);
  } else {
    EoDbText* TextPrimitive = new EoDbText(sm_FontDefinition, sm_ReferenceSystem, Text);
    EoDbGroup* Group = new EoDbGroup(TextPrimitive);
    AeSysDoc::GetDoc()->AddWorkLayerGroup(Group);
    AeSysDoc::GetDoc()->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
  }
  sm_ReferenceSystem.SetOrigin(text_GetNewLinePos(sm_FontDefinition, sm_ReferenceSystem, 1.0, 0));

  sm_TextPrimitive = AeSysView::GetActiveView()->SelectTextUsingPoint(sm_ReferenceSystem.Origin());
  if (sm_TextPrimitive != 0) {
    sm_TextPrimitive->GetFontDef(sm_FontDefinition);
    sm_TextPrimitive->GetRefSys(sm_ReferenceSystem);
    m_TextEditControl.SetWindowTextW(sm_TextPrimitive->Text());
  } else
    m_TextEditControl.SetWindowTextW(L"");

  m_TextEditControl.SetFocus();

  CDialog::OnOK();
}
void EoDlgModeRevise::OnSize(UINT type, int cx, int cy) {
  CDialog::OnSize(type, cx, cy);

  if (::IsWindow(m_TextEditControl.GetSafeHwnd())) { m_TextEditControl.MoveWindow(0, 0, cx, cy, TRUE); }
}
