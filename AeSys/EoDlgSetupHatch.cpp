#include "Stdafx.h"

#include "EoDlgSetupHatch.h"
#include "EoGsRenderState.h"
#include "Resource.h"

EoDlgSetupHatch::EoDlgSetupHatch(CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSetupHatch::IDD, pParent), m_HatchXScaleFactor(0), m_HatchYScaleFactor(0), m_HatchRotationAngle(0) {}
EoDlgSetupHatch::~EoDlgSetupHatch() {}
void EoDlgSetupHatch::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Text(dataExchange, IDC_FIL_AREA_HAT_X_SCAL, m_HatchXScaleFactor);
  DDX_Text(dataExchange, IDC_FIL_AREA_HAT_Y_SCAL, m_HatchYScaleFactor);
  DDX_Text(dataExchange, IDC_FIL_AREA_HAT_ROT_ANG, m_HatchRotationAngle);
}
BOOL EoDlgSetupHatch::OnInitDialog() {
  CDialog::OnInitDialog();

  SetDlgItemInt(IDC_FIL_AREA_HAT_ID, static_cast<UINT>(renderState.PolygonIntStyleId()), FALSE);

  return TRUE;
}
void EoDlgSetupHatch::OnOK() {
  renderState.SetPolygonIntStyleId(std::int16_t(GetDlgItemInt(IDC_FIL_AREA_HAT_ID, 0, FALSE)));

  CDialog::OnOK();
}
