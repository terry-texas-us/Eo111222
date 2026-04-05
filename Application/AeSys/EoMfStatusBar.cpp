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

  GetItemRect(scalePaneIndex, &paneRect);
  if (paneRect.PtInRect(point)) {
    BeginScaleEdit();
    return;
  }

  GetItemRect(zoomPaneIndex, &paneRect);
  if (paneRect.PtInRect(point)) {
    BeginZoomEdit();
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
  m_editingPane = -1;
  m_edit.ShowWindow(SW_HIDE);

  if (editText.IsEmpty()) { return; }

  wchar_t buffer[64]{};
  wcsncpy_s(buffer, editText.GetString(), _TRUNCATE);
  double newLength = app.ParseLength(app.GetUnits(), buffer);
  app.SetDimensionLength(newLength);

  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) { activeView->UpdateStateInformation(AeSysView::DimLen); }
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
  m_editingPane = -1;
  m_edit.ShowWindow(SW_HIDE);

  if (editText.IsEmpty()) { return; }

  wchar_t* endPtr{};
  double newAngle = wcstod(editText.GetString(), &endPtr);
  if (endPtr == editText.GetString()) { return; }  // No valid conversion

  if (newAngle < -360.0) { newAngle = -360.0; }
  if (newAngle > 360.0) { newAngle = 360.0; }
  app.SetDimensionAngle(newAngle);

  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) { activeView->UpdateStateInformation(AeSysView::DimAng); }
}

void EoMfStatusBar::BeginScaleEdit() {
  if (m_editingPane >= 0) { CancelEdit(); }

  CRect paneRect;
  GetItemRect(scalePaneIndex, &paneRect);
  paneRect.DeflateRect(1, 1);

  if (!::IsWindow(m_edit.GetSafeHwnd())) {
    m_edit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT, paneRect, this, 1001);
    m_edit.SetFont(GetFont());
  } else {
    m_edit.MoveWindow(&paneRect);
  }

  auto* activeView = AeSysView::GetActiveView();
  double worldScale = (activeView != nullptr) ? activeView->GetWorldScale() : 1.0;
  CString scaleText;
  scaleText.Format(L"%.2f", worldScale);
  m_edit.SetWindowTextW(scaleText);

  m_edit.ShowWindow(SW_SHOW);
  m_edit.SetFocus();
  m_edit.SetSel(0, -1);

  m_editingPane = scalePaneIndex;
}

void EoMfStatusBar::CommitScaleEdit() {
  CString editText;
  m_edit.GetWindowTextW(editText);
  m_editingPane = -1;
  m_edit.ShowWindow(SW_HIDE);

  if (editText.IsEmpty()) { return; }

  double newScale = _wtof(editText.GetString());
  if (newScale <= 0.0) { return; }

  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) { activeView->SetWorldScale(newScale); }
}

void EoMfStatusBar::BeginZoomEdit() {
  if (m_editingPane >= 0) { CancelEdit(); }

  CRect paneRect;
  GetItemRect(zoomPaneIndex, &paneRect);
  paneRect.DeflateRect(1, 1);

  if (!::IsWindow(m_edit.GetSafeHwnd())) {
    m_edit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT, paneRect, this, 1001);
    m_edit.SetFont(GetFont());
  } else {
    m_edit.MoveWindow(&paneRect);
  }

  auto* activeView = AeSysView::GetActiveView();
  double zoomRatio = 1.0;
  if (activeView != nullptr) {
    double uExtent = activeView->UExtent();
    if (uExtent > 0.0) { zoomRatio = activeView->WidthInInches() / uExtent; }
  }
  CString zoomText;
  zoomText.Format(L"%.3f", zoomRatio);
  m_edit.SetWindowTextW(zoomText);

  m_edit.ShowWindow(SW_SHOW);
  m_edit.SetFocus();
  m_edit.SetSel(0, -1);

  m_editingPane = zoomPaneIndex;
}

void EoMfStatusBar::CommitZoomEdit() {
  CString editText;
  m_edit.GetWindowTextW(editText);
  m_editingPane = -1;
  m_edit.ShowWindow(SW_HIDE);

  if (editText.IsEmpty()) { return; }

  const auto newZoomRatio = _wtof(editText.GetString());
  if (newZoomRatio <= Eo::geometricTolerance) { return; }

  auto* activeView = AeSysView::GetActiveView();
  if (activeView == nullptr) { return; }
  const auto widthInInches = activeView->WidthInInches();
  const auto uExtent = activeView->UExtent();

  if (widthInInches <= Eo::geometricTolerance || uExtent <= Eo::geometricTolerance) { return; }
  /** Only apply the new zoom ratio if it results in a significant change to the view window.
   * This prevents unnecessary updates when the user enters a value very close to the current zoom ratio.
   */
  const auto newUExtent = widthInInches / newZoomRatio;
  const auto aspectRatio = activeView->VExtent() / uExtent;
  const auto newVExtent = newUExtent * aspectRatio;
  const auto centerU = (activeView->UMin() + activeView->UMax()) * 0.5;
  const auto centerV = (activeView->VMin() + activeView->VMax()) * 0.5;
  activeView->SetViewWindow(
      centerU - newUExtent * 0.5, centerV - newVExtent * 0.5, centerU + newUExtent * 0.5, centerV + newVExtent * 0.5);
  activeView->InvalidateScene();
  activeView->UpdateStateInformation(AeSysView::WndRatio);
}

void EoMfStatusBar::CommitEdit() {
  if (m_editingPane == lengthPaneIndex) {
    CommitLengthEdit();
  } else if (m_editingPane == anglePaneIndex) {
    CommitAngleEdit();
  } else if (m_editingPane == scalePaneIndex) {
    CommitScaleEdit();
  } else if (m_editingPane == zoomPaneIndex) {
    CommitZoomEdit();
  }
}

void EoMfStatusBar::CancelEdit() {
  if (m_editingPane < 0) { return; }
  m_edit.ShowWindow(SW_HIDE);
  m_editingPane = -1;
}

void EoMfStatusBar::OnEditKillFocus() { CancelEdit(); }
