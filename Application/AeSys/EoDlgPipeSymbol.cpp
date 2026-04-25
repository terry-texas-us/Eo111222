#include "Stdafx.h"

#include "AeSys.h"
#include "EoDlgPipeSymbol.h"
#include "Resource.h"

EoDlgPipeSymbol::EoDlgPipeSymbol(CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgPipeSymbol::IDD, pParent), m_CurrentPipeSymbolIndex(0) {}
EoDlgPipeSymbol::~EoDlgPipeSymbol() {}
void EoDlgPipeSymbol::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_LIST, m_PipeSymbolsListBoxControl);
}
BOOL EoDlgPipeSymbol::OnInitDialog() {
  CDialog::OnInitDialog();

  const auto names = App::LoadStringResource(IDS_PIPE_SYMBOL_NAMES);
  int position = 0;
  while (position < names.GetLength()) {
    CString namesItem = names.Tokenize(L"\n", position);
    m_PipeSymbolsListBoxControl.AddString(namesItem);
  }
  m_PipeSymbolsListBoxControl.SetCurSel(m_CurrentPipeSymbolIndex);

  return TRUE;
}
void EoDlgPipeSymbol::OnOK() {
  m_CurrentPipeSymbolIndex = m_PipeSymbolsListBoxControl.GetCurSel();

  CDialog::OnOK();
}
