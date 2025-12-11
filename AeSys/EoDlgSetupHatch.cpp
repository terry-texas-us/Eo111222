#include "stdafx.h"
#include "AeSys.h"

#include "EoDlgSetupHatch.h"

// EoDlgSetupHatch dialog

IMPLEMENT_DYNAMIC(EoDlgSetupHatch, CDialog)

BEGIN_MESSAGE_MAP(EoDlgSetupHatch, CDialog)
END_MESSAGE_MAP()

EoDlgSetupHatch::EoDlgSetupHatch(CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgSetupHatch::IDD, pParent), m_HatchXScaleFactor(0), m_HatchYScaleFactor(0), m_HatchRotationAngle(0) {
}
EoDlgSetupHatch::~EoDlgSetupHatch() {
}
void EoDlgSetupHatch::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FIL_AREA_HAT_X_SCAL, m_HatchXScaleFactor);
	DDX_Text(pDX, IDC_FIL_AREA_HAT_Y_SCAL, m_HatchYScaleFactor);
	DDX_Text(pDX, IDC_FIL_AREA_HAT_ROT_ANG, m_HatchRotationAngle);
}
BOOL EoDlgSetupHatch::OnInitDialog() {
	CDialog::OnInitDialog();

	SetDlgItemInt(IDC_FIL_AREA_HAT_ID, pstate.PolygonIntStyleId(), FALSE);

	return TRUE;
}
void EoDlgSetupHatch::OnOK() {
	pstate.SetPolygonIntStyleId(EoInt16(GetDlgItemInt(IDC_FIL_AREA_HAT_ID, 0, FALSE)));

	CDialog::OnOK();
}
