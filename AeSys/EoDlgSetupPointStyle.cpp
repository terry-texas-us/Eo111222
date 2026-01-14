#include "stdafx.h"
#include <Windows.h>

#include "afxdialogex.h"
#include <afx.h>
#include <afxdd_.h>
#include <afxwin.h>

#include "EoDlgSetupPointStyle.h"
#include "Resource.h"

IMPLEMENT_DYNAMIC(CDlgSetPointStyle, CDialogEx)

CDlgSetPointStyle::CDlgSetPointStyle(CWnd* parent /*=nullptr*/) : CDialogEx(IDD_SET_POINT_STYLE, parent) {}

CDlgSetPointStyle::~CDlgSetPointStyle() {}

void CDlgSetPointStyle::DoDataExchange(CDataExchange* pDX) {
  CDialog::DoDataExchange(pDX);

  // When loading (pDX->m_bSaveAndValidate == FALSE) break out the parts of the
  // stored flag into the UI-backed variables before calling DDX_ helpers.
  if (!pDX->m_bSaveAndValidate) {
    m_radioPoint = m_pointStyle & 0x0F;          // radio index stored in low bits
    m_checkCircle = (m_pointStyle & 0x20) != 0;  // 0x20 -> check1
    m_checkSquare = (m_pointStyle & 0x40) != 0;  // 0x40 -> check2
  }
  // Transfer radio and checkbox states between controls and UI vars
  DDX_Radio(pDX, IDC_RADIO_POINT_STYLE_0, m_radioPoint);
  DDX_Check(pDX, IDC_CHECK_CIRCLE, m_checkCircle);
  DDX_Check(pDX, IDC_CHECK_SQUARE, m_checkSquare);

  // new: DDX for mark size numeric edit control (assumes IDC_EDIT1)
  DDX_Text(pDX, IDC_EDIT1, m_pointSize);
  if (pDX->m_bSaveAndValidate) {
    DDV_MinMaxDouble(pDX, m_pointSize, -10000.0, 10000.0);  // arbitrary sane range
  }

  // When saving, combine UI-backed values back into the single flags integer.
  if (pDX->m_bSaveAndValidate) { m_pointStyle = (m_radioPoint & 0x0F) | (m_checkCircle ? 0x20 : 0) | (m_checkSquare ? 0x40 : 0); }
}
BOOL CDlgSetPointStyle::OnInitDialog() {
  CDialog::OnInitDialog();

  // initialize controls based on m_pointStyle if needed
  UpdateData(FALSE);

  return TRUE;  // return TRUE unless you set the focus to a control
}

void CDlgSetPointStyle::OnOK() {
  // Fetch values from controls
  if (!UpdateData(TRUE)) {
    // failed to get data from controls
    return;
  }

  // Validate: for example, accept only 0..4 (five radio styles).
  const int maxStyle = 4;
  int radioOnly = m_pointStyle & 0x0F;
  if (radioOnly < 0 || radioOnly > maxStyle) {
    AfxMessageBox(L"Please select a valid point style.", MB_ICONWARNING);
    return;  // don't close dialog
  }

  // If validation passes, proceed with default OK behavior
  CDialog::OnOK();
}
BEGIN_MESSAGE_MAP(CDlgSetPointStyle, CDialog)
END_MESSAGE_MAP()
