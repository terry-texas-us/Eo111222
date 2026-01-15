#include "Stdafx.h"
#include "EoDlgPipeOptions.h"
#include "Resource.h"

IMPLEMENT_DYNAMIC(EoDlgPipeOptions, CDialog)

EoDlgPipeOptions::EoDlgPipeOptions(CWnd* pParent /*=nullptr*/) :
	CDialog(EoDlgPipeOptions::IDD, pParent), m_PipeTicSize(0), m_PipeRiseDropRadius(0) {
}
EoDlgPipeOptions::~EoDlgPipeOptions() {
}
void EoDlgPipeOptions::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Text(dataExchange, IDC_TIC_SIZE, m_PipeTicSize);
	DDX_Text(dataExchange, IDC_RISEDROP_RADIUS, m_PipeRiseDropRadius);
}
BOOL EoDlgPipeOptions::OnInitDialog() {
	CDialog::OnInitDialog();

	return TRUE;
}
void EoDlgPipeOptions::OnOK() {
	CDialog::OnOK();
}
