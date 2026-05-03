#include "Stdafx.h"

#include "EoCtrlCommandEdit.h"
#include "EoMfCommandTab.h"
#include "MainFrm.h"

BEGIN_MESSAGE_MAP(EoCtrlCommandEdit, CEdit)
ON_WM_SETFOCUS()
ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void EoCtrlCommandEdit::OnSetFocus(CWnd* oldWnd) {
  CEdit::OnSetFocus(oldWnd);
  auto* mainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if (mainFrame != nullptr) { mainFrame->SetCommandLineActive(true); }
}

void EoCtrlCommandEdit::OnKillFocus(CWnd* newWnd) {
  CEdit::OnKillFocus(newWnd);
  auto* mainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if (mainFrame != nullptr) { mainFrame->SetCommandLineActive(false); }
}

BOOL EoCtrlCommandEdit::PreTranslateMessage(MSG* msg) {
  if (msg->message == WM_KEYDOWN) {
    switch (msg->wParam) {
      case VK_RETURN: {
        CString text;
        GetWindowTextW(text);
        SetWindowTextW(L"");
        auto* tab = DYNAMIC_DOWNCAST(EoMfCommandTab, GetParent());
        if (tab != nullptr) {
          if (text.IsEmpty()) {
            // Bare Enter: repeat the last submitted command (AutoCAD muscle memory).
            const auto lastCmd = tab->LastSubmittedCommand();
            if (!lastCmd.empty()) { tab->ExecuteCommand(lastCmd); }
          } else {
            tab->ExecuteCommand(static_cast<LPCWSTR>(text));
          }
        }
        return TRUE;
      }
      case VK_TAB: {
        CString text;
        GetWindowTextW(text);
        auto* tab = DYNAMIC_DOWNCAST(EoMfCommandTab, GetParent());
        if (tab != nullptr) { tab->TryTabComplete(static_cast<LPCWSTR>(text)); }
        return TRUE;
      }
      case VK_UP: {
        auto* tab = DYNAMIC_DOWNCAST(EoMfCommandTab, GetParent());
        if (tab != nullptr) {
          const auto recalled = tab->RecallPreviousCommand();
          if (!recalled.empty()) {
            SetWindowTextW(recalled.c_str());
            SetSel(static_cast<int>(recalled.size()), static_cast<int>(recalled.size()));
          }
        }
        return TRUE;
      }
      case VK_DOWN: {
        auto* tab = DYNAMIC_DOWNCAST(EoMfCommandTab, GetParent());
        if (tab != nullptr) {
          const auto recalled = tab->RecallNextCommand();
          SetWindowTextW(recalled.c_str());
          SetSel(static_cast<int>(recalled.size()), static_cast<int>(recalled.size()));
        }
        return TRUE;
      }
      case VK_ESCAPE: {
        CString text;
        GetWindowTextW(text);
        if (!text.IsEmpty()) {
          SetWindowTextW(L"");
        } else {
          // Return focus to the active MDI view.
          auto* mdiFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, AfxGetMainWnd());
          if (mdiFrame != nullptr) {
            auto* mdiChild = mdiFrame->MDIGetActive();
            if (mdiChild != nullptr) {
              auto* view = mdiChild->GetActiveView();
              if (view != nullptr) { view->SetFocus(); }
            }
          }
        }
        return TRUE;
      }
      default:
        break;
    }
  }
  return CEdit::PreTranslateMessage(msg);
}
