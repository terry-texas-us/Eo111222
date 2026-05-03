#pragma once

/// @brief Single-line command-line edit control hosted in the Output pane "Command" tab.
///
/// Phase 1 scope: backtick activation (handled by the view), Enter dispatches the
/// typed text through EoMfCommandTab::ExecuteCommand, ESC returns focus to the
/// active view. History scrolling, completion, and tokenization arrive in P3+.
class EoCtrlCommandEdit : public CEdit {
 public:
  EoCtrlCommandEdit() = default;
  EoCtrlCommandEdit(const EoCtrlCommandEdit&) = delete;
  EoCtrlCommandEdit& operator=(const EoCtrlCommandEdit&) = delete;
  ~EoCtrlCommandEdit() override = default;

 protected:
  BOOL PreTranslateMessage(MSG* msg) override;
  afx_msg void OnSetFocus(CWnd* oldWnd);
  afx_msg void OnKillFocus(CWnd* newWnd);

  DECLARE_MESSAGE_MAP()
};
