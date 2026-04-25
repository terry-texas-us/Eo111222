#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDlgSelectGotoHomePoint.h"
#include "Resource.h"

BEGIN_MESSAGE_MAP(EoDlgSelectGotoHomePoint, CDialog)
ON_CBN_EDITUPDATE(IDC_LIST, &EoDlgSelectGotoHomePoint::OnCbnEditupdateList)
ON_CBN_SELCHANGE(IDC_LIST, &EoDlgSelectGotoHomePoint::OnCbnSelchangeList)
END_MESSAGE_MAP()

EoDlgSelectGotoHomePoint::EoDlgSelectGotoHomePoint(CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSelectGotoHomePoint::IDD, pParent) {}
EoDlgSelectGotoHomePoint::EoDlgSelectGotoHomePoint(AeSysView* activeView, CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSelectGotoHomePoint::IDD, pParent), m_ActiveView(activeView) {}
EoDlgSelectGotoHomePoint::~EoDlgSelectGotoHomePoint() {}
void EoDlgSelectGotoHomePoint::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_LIST, m_HomePointNames);
  DDX_Control(dataExchange, IDC_X, m_X);
  DDX_Control(dataExchange, IDC_Y, m_Y);
  DDX_Control(dataExchange, IDC_Z, m_Z);
}
BOOL EoDlgSelectGotoHomePoint::OnInitDialog() {
  CDialog::OnInitDialog();

  const auto names = App::LoadStringResource(IDS_HOME_POINT_GO_NAMES);
  m_HomePointNames.ResetContent();
  int position = 0;
  while (position < names.GetLength()) {
    CString namesItem = names.Tokenize(L"\n", position);
    m_HomePointNames.AddString(namesItem);
  }
  m_HomePointNames.SetCurSel(9);

  const auto origin = m_ActiveView->GridOrign();

  CString length;

  app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Engineering), origin.x);
  SetDlgItemTextW(IDC_X, length);
  app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Engineering), origin.y);
  SetDlgItemTextW(IDC_Y, length);
  app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Engineering), origin.z);
  SetDlgItemTextW(IDC_Z, length);

  return TRUE;
}
void EoDlgSelectGotoHomePoint::OnOK() { CDialog::OnOK(); }
void EoDlgSelectGotoHomePoint::OnCbnEditupdateList() {
  CString namesItem;
  m_HomePointNames.GetWindowTextW(namesItem);

  int namesItemIndex = m_HomePointNames.FindString(-1, namesItem);

  if (namesItemIndex != CB_ERR) {
    switch (namesItemIndex) {
      case 9:
        m_ActiveView->SetCursorPosition(m_ActiveView->GridOrign());
        break;
      case 10:
        m_ActiveView->SetCursorPosition(AeSysDoc::GetDoc()->GetTrapPivotPoint());
        break;
      case 11:
        m_ActiveView->SetCursorPosition(m_ActiveView->CameraTarget());
        break;
      case 12:
        m_ActiveView->SetCursorPosition(EoGePoint3d::kOrigin);
        break;
      default:
        m_ActiveView->SetCursorPosition(app.HomePointGet(namesItemIndex));
    }
    CDialog::OnOK();
  }
}
void EoDlgSelectGotoHomePoint::OnCbnSelchangeList() {
  int namesItemIndex = m_HomePointNames.GetCurSel();

  if (namesItemIndex != CB_ERR) {
    EoGePoint3d point;
    switch (namesItemIndex) {
      case 9:
        point = m_ActiveView->GridOrign();
        break;
      case 10:
        point = AeSysDoc::GetDoc()->GetTrapPivotPoint();
        break;
      case 11:
        point = m_ActiveView->CameraTarget();
        break;
      case 12:
        point = EoGePoint3d::kOrigin;
        break;
      default:
        point = app.HomePointGet(namesItemIndex);
    }
    CString length;
    app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Engineering), point.x);
    SetDlgItemTextW(IDC_X, length);
    app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Engineering), point.y);
    SetDlgItemTextW(IDC_Y, length);
    app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Engineering), point.z);
    SetDlgItemTextW(IDC_Z, length);
  }
}
