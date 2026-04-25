#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"
#include "EoDlgSetupConstraints.h"
#include "Resource.h"

EoDlgSetupConstraints::EoDlgSetupConstraints(CWnd* pParent /* = nullptr */)
    : CDialog(EoDlgSetupConstraints::IDD, pParent) {}
EoDlgSetupConstraints::EoDlgSetupConstraints(AeSysView* view, CWnd* pParent /* = nullptr */)
    : CDialog(EoDlgSetupConstraints::IDD, pParent), m_ActiveView(view) {}
EoDlgSetupConstraints::~EoDlgSetupConstraints() {}
void EoDlgSetupConstraints::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_USR_GRID_X_INT, m_GridXSnapSpacing);
  DDX_Control(dataExchange, IDC_USR_GRID_Y_INT, m_GridYSnapSpacing);
  DDX_Control(dataExchange, IDC_USR_GRID_Z_INT, m_GridZSnapSpacing);
  DDX_Control(dataExchange, IDC_GRID_DOT_INT_X, m_GridXPointSpacing);
  DDX_Control(dataExchange, IDC_GRID_DOT_INT_Y, m_GridYPointSpacing);
  DDX_Control(dataExchange, IDC_GRID_DOT_INT_Z, m_GridZPointSpacing);
  DDX_Control(dataExchange, IDC_GRID_LN_INT_X, m_GridXLineSpacing);
  DDX_Control(dataExchange, IDC_GRID_LN_INT_Y, m_GridYLineSpacing);
  DDX_Control(dataExchange, IDC_GRID_LN_INT_Z, m_GridZLineSpacing);
  DDX_Control(dataExchange, IDC_GRID_SNAP_ON, m_GridSnapEnableCheckBox);
  DDX_Control(dataExchange, IDC_GRID_PTS_DIS_ON, m_GridDisplayCheckBox);
  DDX_Control(dataExchange, IDC_GRID_LNS_DIS_ON, m_GridLineDisplayCheckBox);
  DDX_Control(dataExchange, IDC_USR_AX_INF_ANG, m_AxisInfluenceAngle);
  DDX_Control(dataExchange, IDC_USR_AX_Z_OFF_ANG, m_AxisZOffsetAngle);
}

BOOL EoDlgSetupConstraints::OnInitDialog() {
  CDialog::OnInitDialog();

  const auto currentUnits = app.GetUnits();

  double x, y, z;

  m_ActiveView->GetGridSnapSpacing(x, y, z);

  CString length;
  app.FormatLength(length, currentUnits, x);
  m_GridXSnapSpacing.SetWindowTextW(length);
  app.FormatLength(length, currentUnits, y);
  m_GridYSnapSpacing.SetWindowTextW(length);
  app.FormatLength(length, currentUnits, z);
  m_GridZSnapSpacing.SetWindowTextW(length);

  m_ActiveView->GetGridPointSpacing(x, y, z);

  app.FormatLength(length, currentUnits, x);
  m_GridXPointSpacing.SetWindowTextW(length);
  app.FormatLength(length, currentUnits, y);
  m_GridYPointSpacing.SetWindowTextW(length);
  app.FormatLength(length, currentUnits, z);
  m_GridZPointSpacing.SetWindowTextW(length);

  m_ActiveView->GetGridLineSpacing(x, y, z);

  app.FormatLength(length, currentUnits, x);
  m_GridXLineSpacing.SetWindowTextW(length);
  app.FormatLength(length, currentUnits, y);
  m_GridYLineSpacing.SetWindowTextW(length);
  app.FormatLength(length, currentUnits, z);
  m_GridZLineSpacing.SetWindowTextW(length);

  m_GridSnapEnableCheckBox.SetCheck(m_ActiveView->GridSnap() ? BST_CHECKED : BST_UNCHECKED);
  m_GridDisplayCheckBox.SetCheck(m_ActiveView->DisplayGridWithPoints() ? BST_CHECKED : BST_UNCHECKED);
  m_GridLineDisplayCheckBox.SetCheck(m_ActiveView->DisplayGridWithLines() ? BST_CHECKED : BST_UNCHECKED);

  CString text;

  text.Format(L"%f", m_ActiveView->AxisConstraintInfluenceAngle());
  m_AxisInfluenceAngle.SetWindowTextW(text);

  text.Format(L"%f", m_ActiveView->AxisConstraintOffsetAngle());
  m_AxisZOffsetAngle.SetWindowTextW(text);

  return TRUE;
}

void EoDlgSetupConstraints::OnOK() {
  const auto currentUnits = app.GetUnits();

  wchar_t windowText[32]{};

  double x, y, z;

  m_GridXSnapSpacing.GetWindowTextW(windowText, 32);
  x = app.ParseLength(currentUnits, windowText);
  m_GridYSnapSpacing.GetWindowTextW(windowText, 32);
  y = app.ParseLength(currentUnits, windowText);
  m_GridZSnapSpacing.GetWindowTextW(windowText, 32);
  z = app.ParseLength(currentUnits, windowText);
  m_ActiveView->SetGridSnapSpacing(x, y, z);

  m_GridXPointSpacing.GetWindowTextW(windowText, 32);
  x = app.ParseLength(currentUnits, windowText);
  m_GridYPointSpacing.GetWindowTextW(windowText, 32);
  y = app.ParseLength(currentUnits, windowText);
  m_GridZPointSpacing.GetWindowTextW(windowText, 32);
  z = app.ParseLength(currentUnits, windowText);
  m_ActiveView->SetGridPointSpacing(x, y, z);

  m_GridXLineSpacing.GetWindowTextW(windowText, 32);
  x = app.ParseLength(currentUnits, windowText);
  m_GridYLineSpacing.GetWindowTextW(windowText, 32);
  y = app.ParseLength(currentUnits, windowText);
  m_GridZLineSpacing.GetWindowTextW(windowText, 32);
  z = app.ParseLength(currentUnits, windowText);
  m_ActiveView->SetGridLineSpacing(x, y, z);

  m_ActiveView->EnableGridSnap(m_GridSnapEnableCheckBox.GetCheck() == BST_CHECKED);
  m_ActiveView->EnableDisplayGridWithPoints(m_GridDisplayCheckBox.GetCheck() == BST_CHECKED);
  m_ActiveView->EnableDisplayGridWithLines(m_GridLineDisplayCheckBox.GetCheck() == BST_CHECKED);

  m_AxisInfluenceAngle.GetWindowTextW(windowText, 32);
  m_ActiveView->SetAxisConstraintInfluenceAngle(_wtof(windowText));
  m_AxisZOffsetAngle.GetWindowTextW(windowText, 32);
  m_ActiveView->SetAxisConstraintOffsetAngle(_wtof(windowText));

  CDialog::OnOK();
}