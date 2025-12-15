#include "stdafx.h"

#include "EoDlgSetScale.h"

// EoDlgSetScale dialog

IMPLEMENT_DYNAMIC(EoDlgSetScale, CDialog)

EoDlgSetScale::EoDlgSetScale(CWnd* pParent /*=nullptr*/) :
	CDialog(EoDlgSetScale::IDD, pParent), m_Scale(0) {
}
EoDlgSetScale::~EoDlgSetScale() {
}
void EoDlgSetScale::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Text(dataExchange, IDC_SCALE, m_Scale);
	DDV_MinMaxDouble(dataExchange, m_Scale, 0.0001, 10000.);
}

