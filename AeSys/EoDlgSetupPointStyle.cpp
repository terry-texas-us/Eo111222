#include "stdafx.h"
#include <Windows.h>

#include "afxdialogex.h"
#include <afx.h>
#include <afxdd_.h>
#include <afxwin.h>

#include "EoDlgSetupPointStyle.h"
#include "Resource.h"

CDlgSetPointStyle::CDlgSetPointStyle(CWnd* parent /*=nullptr*/) : CDialogEx(IDD_SET_POINT_STYLE, parent) {}

CDlgSetPointStyle::~CDlgSetPointStyle() {}

void CDlgSetPointStyle::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);

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
  DDX_Text(pDX, IDC_EDIT_POINT_SIZE, m_pointSize);
  if (pDX->m_bSaveAndValidate) { DDV_MinMaxDouble(pDX, m_pointSize, -288.0, 4.0); }
  // When saving, combine UI-backed values back into the single flags integer.
  if (pDX->m_bSaveAndValidate) { m_pointStyle = (m_radioPoint & 0x0F) | (m_checkCircle ? 0x20 : 0) | (m_checkSquare ? 0x40 : 0); }
}
BOOL CDlgSetPointStyle::OnInitDialog() {
  CDialogEx::OnInitDialog();
  UpdateData(FALSE);
  return TRUE;  // return TRUE unless you set the focus to a control
}

void CDlgSetPointStyle::OnOK() {
  if (!UpdateData(TRUE)) { return; }

  // Validate: for example, accept only 0..4 (five radio styles).
  const int maxStyle = 4;
  int radioOnly = m_pointStyle & 0x0F;
  if (radioOnly < 0 || radioOnly > maxStyle) {
    AfxMessageBox(L"Please select a valid point style.", MB_ICONWARNING);
    return;  // don't close dialog
  }
  CDialogEx::OnOK();
}