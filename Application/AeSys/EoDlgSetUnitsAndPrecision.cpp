#include "Stdafx.h"

#include "AeSys.h"
#include "Eo.h"
#include "EoDlgSetUnitsAndPrecision.h"
#include "Resource.h"

BEGIN_MESSAGE_MAP(EoDlgSetUnitsAndPrecision, CDialog)
ON_BN_CLICKED(IDC_METRIC, &EoDlgSetUnitsAndPrecision::OnBnClickedMetric)
END_MESSAGE_MAP()

EoDlgSetUnitsAndPrecision::EoDlgSetUnitsAndPrecision(CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSetUnitsAndPrecision::IDD, pParent), m_Units(Eo::Units::Inches), m_Precision(8) {}

EoDlgSetUnitsAndPrecision::~EoDlgSetUnitsAndPrecision() {}
void EoDlgSetUnitsAndPrecision::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_METRIC_UNITS, m_MetricUnitsListBoxControl);
  DDX_Text(dataExchange, IDC_PRECISION, m_Precision);
}
BOOL EoDlgSetUnitsAndPrecision::OnInitDialog() {
  CDialog::OnInitDialog();

  int CheckButtonId = std::min(IDC_ARCHITECTURAL + static_cast<int>(m_Units), IDC_METRIC);
  CheckRadioButton(IDC_ARCHITECTURAL, IDC_METRIC, CheckButtonId);

  auto MetricUnits = App::LoadStringResource(IDS_METRIC_UNITS);
  int Position = 0;
  while (Position < MetricUnits.GetLength()) {
    CString UnitsItem = MetricUnits.Tokenize(L"\n", Position);
    m_MetricUnitsListBoxControl.AddString(UnitsItem);
  }
  if (CheckButtonId == IDC_METRIC) {
    m_MetricUnitsListBoxControl.SetCurSel(static_cast<int>(m_Units) - static_cast<int>(Eo::Units::Meters));
  }
  return TRUE;
}
void EoDlgSetUnitsAndPrecision::OnOK() {
  switch (GetCheckedRadioButton(IDC_ARCHITECTURAL, IDC_METRIC)) {
    case IDC_ARCHITECTURAL:
      m_Units = Eo::Units::Architectural;
      break;
    case IDC_ENGINEERING:
      m_Units = Eo::Units::Engineering;
      break;
    case IDC_FEET:
      m_Units = Eo::Units::Feet;
      break;
    case IDC_INCHES:
      m_Units = Eo::Units::Inches;
      break;
    default:
      switch (m_MetricUnitsListBoxControl.GetCurSel()) {
        case 0:
          m_Units = Eo::Units::Meters;
          break;
        case 1:
          m_Units = Eo::Units::Millimeters;
          break;
        case 2:
          m_Units = Eo::Units::Centimeters;
          break;
        case 3:
          m_Units = Eo::Units::Decimeters;
          break;
        default:
          m_Units = Eo::Units::Kilometers;
      }
  }
  CDialog::OnOK();
}
void EoDlgSetUnitsAndPrecision::OnBnClickedMetric() {
  m_MetricUnitsListBoxControl.SetCurSel(static_cast<int>(Eo::Units::Centimeters) - static_cast<int>(Eo::Units::Meters));
}
