#include "Stdafx.h"

#include "Eo.h"
#include "EoDlgViewZoom.h"
#include "Resource.h"

EoDlgViewZoom::EoDlgViewZoom(CWnd* pParent /*=nullptr*/) :
	CDialog(EoDlgViewZoom::IDD, pParent), m_Ratio(0) {
}
EoDlgViewZoom::~EoDlgViewZoom() {
}
void EoDlgViewZoom::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Text(dataExchange, IDC_RATIO, m_Ratio);
	DDV_MinMaxDouble(dataExchange, m_Ratio, 0.001, 999.0);
}
BOOL EoDlgViewZoom::OnInitDialog() {
  double factor = std::pow(10.0, 3);
  m_Ratio = std::round(m_Ratio * factor) / factor;

	CDialog::OnInitDialog();
	return TRUE;
}
