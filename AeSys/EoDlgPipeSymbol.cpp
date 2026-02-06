#include "Stdafx.h"

#include "AeSys.h"
#include "EoDlgPipeSymbol.h"
#include "Resource.h"

EoDlgPipeSymbol::EoDlgPipeSymbol(CWnd* pParent /*=nullptr*/) :
	CDialog(EoDlgPipeSymbol::IDD, pParent), m_CurrentPipeSymbolIndex(0) {
}
EoDlgPipeSymbol::~EoDlgPipeSymbol() {
}
void EoDlgPipeSymbol::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Control(dataExchange, IDC_LIST, m_PipeSymbolsListBoxControl);
}
BOOL EoDlgPipeSymbol::OnInitDialog() {
	CDialog::OnInitDialog();

	auto Names = App::LoadStringResource(IDS_PIPE_SYMBOL_NAMES);
	int Position = 0;
	while (Position < Names.GetLength()) {
		CString NamesItem = Names.Tokenize(L"\n", Position);
		m_PipeSymbolsListBoxControl.AddString(NamesItem);
	}
	m_PipeSymbolsListBoxControl.SetCurSel(m_CurrentPipeSymbolIndex);

	return TRUE;
}
void EoDlgPipeSymbol::OnOK() {
	m_CurrentPipeSymbolIndex = m_PipeSymbolsListBoxControl.GetCurSel();

	CDialog::OnOK();
}
