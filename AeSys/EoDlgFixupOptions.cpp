#include "stdafx.h"

#include "EoDlgFixupOptions.h"

// EoDlgFixupOptions dialog

IMPLEMENT_DYNAMIC(EoDlgFixupOptions, CDialog)

EoDlgFixupOptions::EoDlgFixupOptions(CWnd* pParent /*=nullptr*/) :
	CDialog(EoDlgFixupOptions::IDD, pParent) {
}
EoDlgFixupOptions::~EoDlgFixupOptions() {
}
void EoDlgFixupOptions::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Text(dataExchange, IDC_FIX_AX_TOL, m_FixupAxisTolerance);
	DDX_Text(dataExchange, IDC_FIX_SIZ, m_FixupModeCornerSize);
}
