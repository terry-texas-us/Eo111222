#include "Stdafx.h"

#include <algorithm>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoMfStatusBar.h"

IMPLEMENT_DYNAMIC(EoMfStatusBar, CMFCStatusBar)

BEGIN_MESSAGE_MAP(EoMfStatusBar, CMFCStatusBar)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_LBUTTONDBLCLK()
#pragma warning(pop)
ON_EN_KILLFOCUS(1001, OnEditKillFocus)
END_MESSAGE_MAP()

void EoMfStatusBar::OnLButtonDblClk(UINT flags, CPoint point) {
  CRect paneRect;

  GetItemRect(lengthPaneIndex, &paneRect);
  if (paneRect.PtInRect(point)) {
    BeginLengthEdit();
    return;
  }

  GetItemRect(anglePaneIndex, &paneRect);
  if (paneRect.PtInRect(point)) {
    BeginAngleEdit();
    return;
  }

  CMFCStatusBar::OnLButtonDblClk(flags, point);
}

BOOL EoMfStatusBar::PreTranslateMessage(MSG* msg) {
  if (m_editingPane >= 0 && msg->message == WM_KEYDOWN) {
    if (msg->wParam == VK_RETURN) {
      CommitEdit();
      return TRUE;
    }
    if (msg->wParam == VK_ESCAPE) {
      CancelEdit();
      return TRUE;
    }
  }
  return CMFCStatusBar::PreTranslateMessage(msg);
}

void EoMfStatusBar::BeginLengthEdit() {
  if (m_editingPane >= 0) { CancelEdit(); }

  CRect paneRect;
  GetItemRect(lengthPaneIndex, &paneRect);
  paneRect.DeflateRect(1, 1);

  if (!::IsWindow(m_edit.GetSafeHwnd())) {
    m_edit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT, paneRect, this, 1001);
    m_edit.SetFont(GetFont());
  } else {
    m_edit.MoveWindow(&paneRect);
  }

  CString lengthText;
  app.FormatLength(lengthText, std::max(app.GetUnits(), Eo::Units::Engineering), app.DimensionLength());
  lengthText.TrimLeft();
  m_edit.SetWindowTextW(lengthText);

  m_edit.ShowWindow(SW_SHOW);
  m_edit.SetFocus();
  m_edit.SetSel(0, -1);

  m_editingPane = lengthPaneIndex;
}

void EoMfStatusBar::CommitLengthEdit() {
  CString editText;
  m_edit.GetWindowTextW(editText);
  m_edit.ShowWindow(SW_HIDE);
  m_editingPane = -1;

  if (editText.IsEmpty()) { return; }

  wchar_t buffer[64]{};
  wcsncpy_s(buffer, editText.GetString(), _TRUNCATE);
  double newLength = app.ParseLength(app.GetUnits(), buffer);
  app.SetDimensionLength(newLength);

  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) {
    activeView->UpdateStateInformation(AeSysView::DimLen);
  }
}

void EoMfStatusBar::BeginAngleEdit() {
  if (m_editingPane >= 0) { CancelEdit(); }

  CRect paneRect;
  GetItemRect(anglePaneIndex, &paneRect);
  paneRect.DeflateRect(1, 1);

  if (!::IsWindow(m_edit.GetSafeHwnd())) {
    m_edit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT, paneRect, this, 1001);
    m_edit.SetFont(GetFont());
  } else {
    m_edit.MoveWindow(&paneRect);
  }

  // Display current angle in degrees (same format as EoDlgSetAngle)
  CString angleText;
  angleText.Format(L"%.3f", app.DimensionAngle());
  m_edit.SetWindowTextW(angleText);

  m_edit.ShowWindow(SW_SHOW);
  m_edit.SetFocus();
  m_edit.SetSel(0, -1);

  m_editingPane = anglePaneIndex;
}

void EoMfStatusBar::CommitAngleEdit() {
  CString editText;
  m_edit.GetWindowTextW(editText);
  m_edit.ShowWindow(SW_HIDE);
  m_editingPane = -1;

  if (editText.IsEmpty()) { return; }

  // Parse angle in degrees (same as EoDlgSetAngle: DDX_Text with double, range -360..360)
  double newAngle = _wtof(editText.GetString());
  if (newAngle < -360.0) { newAngle = -360.0; }
  if (newAngle > 360.0) { newAngle = 360.0; }
  app.SetDimensionAngle(newAngle);

  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) {
    activeView->UpdateStateInformation(AeSysView::DimAng);
  }
}

void EoMfStatusBar::CommitEdit() {
  if (m_editingPane == lengthPaneIndex) {
    CommitLengthEdit();
  } else if (m_editingPane == anglePaneIndex) {
    CommitAngleEdit();
  }
}

void EoMfStatusBar::CancelEdit() {
  if (m_editingPane < 0) { return; }
  m_edit.ShowWindow(SW_HIDE);
  m_editingPane = -1;
}

void EoMfStatusBar::OnEditKillFocus() { CancelEdit(); }
