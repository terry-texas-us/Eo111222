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

  auto Names = App::LoadStringResource(IDS_HOME_POINT_SET_NAMES);
  m_HomePointNames.ResetContent();
  int Position = 0;
  while (Position < Names.GetLength()) {
    CString NamesItem = Names.Tokenize(L"\n", Position);
    m_HomePointNames.AddString(NamesItem);
  }
  m_HomePointNames.SetCurSel(9);

  m_CursorPosition = app.GetCursorPosition();

  CString Length;
  app.FormatLength(Length, std::max(app.GetUnits(), Eo::Units::Engineering), m_CursorPosition.x);
  SetDlgItemTextW(IDC_X, Length);
  app.FormatLength(Length, std::max(app.GetUnits(), Eo::Units::Engineering), m_CursorPosition.y);
  SetDlgItemTextW(IDC_Y, Length);
  app.FormatLength(Length, std::max(app.GetUnits(), Eo::Units::Engineering), m_CursorPosition.z);
  SetDlgItemTextW(IDC_Z, Length);

  return TRUE;
}
void EoDlgSetHomePoint::OnOK() {
  wchar_t itemText[32]{};

  Eo::Units CurrentUnits = app.GetUnits();

  m_X.GetWindowTextW(itemText, 32);
  m_CursorPosition.x = app.ParseLength(CurrentUnits, itemText);
  m_Y.GetWindowTextW(itemText, 32);
  m_CursorPosition.y = app.ParseLength(CurrentUnits, itemText);
  m_Z.GetWindowTextW(itemText, 32);
  m_CursorPosition.z = app.ParseLength(CurrentUnits, itemText);

  int NamesItemIndex = m_HomePointNames.GetCurSel();

  if (NamesItemIndex != CB_ERR) {
    switch (NamesItemIndex) {
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
        app.HomePointSave(NamesItemIndex, m_CursorPosition);
    }
    CDialog::OnOK();
  }
}
void EoDlgSetHomePoint::OnCbnEditupdateList() {
  CString NamesItem;
  m_HomePointNames.GetWindowTextW(NamesItem);
  int NamesItemIndex = m_HomePointNames.FindString(-1, NamesItem);

  if (NamesItemIndex != CB_ERR) {
    switch (NamesItemIndex) {
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
        app.HomePointSave(NamesItemIndex, m_CursorPosition);
    }
    CDialog::OnOK();
  }
}
