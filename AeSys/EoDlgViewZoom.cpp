#include "stdafx.h"
#include "EoDlgViewZoom.h"

// EoDlgViewZoom dialog

IMPLEMENT_DYNAMIC(EoDlgViewZoom, CDialog)

EoDlgViewZoom::EoDlgViewZoom(CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgViewZoom::IDD, pParent), m_Ratio(0) {
}
EoDlgViewZoom::~EoDlgViewZoom() {
}
void EoDlgViewZoom::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Text(dataExchange, IDC_RATIO, m_Ratio);
	DDV_MinMaxFloat(dataExchange, m_Ratio, 0.001f, 999.0f);
}
BOOL EoDlgViewZoom::OnInitDialog() {
	m_Ratio = static_cast<float>(EoRound(m_Ratio, 3));
	int Precision = (m_Ratio >= 1.) ? 3 - int(log10(m_Ratio)) - 1 : 3;
	CString FormatSpecification;
	FormatSpecification.Format(L"%%8.%if", Precision);
	CString RatioAsString;
	RatioAsString.Format(FormatSpecification, m_Ratio);
	m_Ratio = static_cast<float>(_wtof(RatioAsString));

	CDialog::OnInitDialog();
	return TRUE;
}
// EoDlgViewZoom message handlers


