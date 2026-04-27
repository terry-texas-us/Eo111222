#include "Stdafx.h"

#include <algorithm>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDlgSetHomePoint.h"
#include "Resource.h"

BEGIN_MESSAGE_MAP(EoDlgSetHomePoint, CDialog)
ON_CBN_EDITUPDATE(IDC_LIST, &EoDlgSetHomePoint::OnCbnEditupdateList)
END_MESSAGE_MAP()

EoGePoint3d EoDlgSetHomePoint::m_CursorPosition = EoGePoint3d::kOrigin;

EoDlgSetHomePoint::EoDlgSetHomePoint(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgSetHomePoint::IDD, pParent) {}
EoDlgSetHomePoint::EoDlgSetHomePoint(AeSysView* activeView, CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSetHomePoint::IDD, pParent), m_ActiveView(activeView) {}
EoDlgSetHomePoint::~EoDlgSetHomePoint() {}
void EoDlgSetHomePoint::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_LIST, m_HomePointNames);
  DDX_Control(dataExchange, IDC_X, m_X);
  DDX_Control(dataExchange, IDC_Y, m_Y);
  DDX_Control(dataExchange, IDC_Z, m_Z);
}
BOOL EoDlgSetHomePoint::OnInitDialog() {
  CDialog::OnInitDialog();

  const auto names = App::LoadStringResource(IDS_HOME_POINT_SET_NAMES);
  m_HomePointNames.ResetContent();
  int position{};
  while (position < names.GetLength()) {
    CString namesItem = names.Tokenize(L"\n", position);
    m_HomePointNames.AddString(namesItem);
  }
  m_HomePointNames.SetCurSel(9);

  m_CursorPosition = app.GetCursorPosition();

  CString length;
  app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Engineering), m_CursorPosition.x);
  SetDlgItemTextW(IDC_X, length);
  app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Engineering), m_CursorPosition.y);
  SetDlgItemTextW(IDC_Y, length);
  app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Engineering), m_CursorPosition.z);
  SetDlgItemTextW(IDC_Z, length);

  return TRUE;
}
void EoDlgSetHomePoint::OnOK() {
  wchar_t itemText[32]{};

  const auto currentUnits = app.GetUnits();

  m_X.GetWindowTextW(itemText, 32);
  m_CursorPosition.x = app.ParseLength(currentUnits, itemText);
  m_Y.GetWindowTextW(itemText, 32);
  m_CursorPosition.y = app.ParseLength(currentUnits, itemText);
  m_Z.GetWindowTextW(itemText, 32);
  m_CursorPosition.z = app.ParseLength(currentUnits, itemText);

  const int namesItemIndex = m_HomePointNames.GetCurSel();

  if (namesItemIndex != CB_ERR) {
    switch (namesItemIndex) {
      case 9:
        m_ActiveView->GridOrign(m_CursorPosition);
        break;
      case 10:
        AeSysDoc::GetDoc()->SetTrapPivotPoint(m_CursorPosition);
        break;
      case 11:
        m_ActiveView->SetCameraTarget(m_CursorPosition);
        break;
      default:
        app.HomePointSave(namesItemIndex, m_CursorPosition);
    }
    CDialog::OnOK();
  }
}
void EoDlgSetHomePoint::OnCbnEditupdateList() {
  CString namesItem;
  m_HomePointNames.GetWindowTextW(namesItem);
  const int namesItemIndex = m_HomePointNames.FindString(-1, namesItem);

  if (namesItemIndex != CB_ERR) {
    switch (namesItemIndex) {
      case 9:
        m_ActiveView->GridOrign(m_CursorPosition);
        break;
      case 10:
        AeSysDoc::GetDoc()->SetTrapPivotPoint(m_CursorPosition);
        break;
      case 11:
        m_ActiveView->SetCameraTarget(m_CursorPosition);
        break;
      default:
        app.HomePointSave(namesItemIndex, m_CursorPosition);
    }
    CDialog::OnOK();
  }
}
