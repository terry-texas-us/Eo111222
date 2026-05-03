#pragma once

#include <string>
#include <vector>

#include "EoCtrlCommandEdit.h"
#include "EoMfOutputDockablePane.h"

/// @brief Composite child window hosting the "Command" tab inside EoMfOutputDockablePane.
///
/// Layout: a read-only history list box fills the client area above a single-line
/// EoCtrlCommandEdit docked at the bottom. Enter in the edit dispatches the text
/// through ExecuteCommand, which uses EoCommandRegistry to resolve verb -> mode/op
/// WM_COMMAND ids and routes through the main frame. HELP / ? lists registered
/// commands. Phase 3 will add coordinate-token injection.
class EoMfCommandTab : public CWnd {
  DECLARE_DYNAMIC(EoMfCommandTab)

 public:
  EoMfCommandTab() = default;
  EoMfCommandTab(const EoMfCommandTab&) = delete;
  EoMfCommandTab& operator=(const EoMfCommandTab&) = delete;
  ~EoMfCommandTab() override = default;

  /// @brief Creates the child controls inside this composite window.
  BOOL Create(const RECT& rect, CWnd* parent, UINT id);

  /// @brief Sets focus to the command edit control.
  void FocusCommandEdit();

  /// @brief Appends a line to the history list and scrolls to the bottom.
  void AppendHistory(const std::wstring& line);

  /// @brief Dispatches a typed command line. Phase 1 handles a small hardcoded set.
  void ExecuteCommand(const std::wstring& commandLine);

  /// @brief Returns the previous submitted command line for Up-arrow recall.
  [[nodiscard]] std::wstring RecallPreviousCommand();

  /// @brief Returns the next submitted command line for Down-arrow recall, or
  ///        an empty string when navigating past the most recent entry.
  [[nodiscard]] std::wstring RecallNextCommand();

  /// @brief Attempts Tab completion on the current partial text. Replaces the
  ///        edit text with the first match whose upper-case form starts with the
  ///        current text; cycles through matches on successive Tab presses.
  ///        Resets the cycle when the text changes between Tab presses.
  void TryTabComplete(const std::wstring& partial);

  /// @brief Appends the active mode's PromptString (if any) as a hint line.
  void ShowModePrompt();

  /// @brief Applies the active color scheme to the history list.
  void ApplyColorScheme();

  /// @brief Returns the most recently submitted command line, or empty if none.
  [[nodiscard]] std::wstring LastSubmittedCommand() const {
    if (m_submittedHistory.empty()) { return {}; }
    return m_submittedHistory.back();
  }

 protected:
  EoMfOutputListBox m_history;
  EoCtrlCommandEdit m_edit;
  CFont m_font;

  /// @brief Submitted command-line strings, oldest first. Used for Up/Down recall.
  std::vector<std::wstring> m_submittedHistory;

  /// @brief Recall cursor. Equals m_submittedHistory.size() when not navigating.
  std::size_t m_recallIndex{0};

  /// @brief Tab-completion state: the sorted candidate list and cycle index.
  std::vector<std::wstring> m_tabCandidates;
  std::size_t m_tabIndex{0};
  std::wstring m_tabPrefix;

  static constexpr int kEditHeight = 22;
  static constexpr int kGap = 2;

  afx_msg int OnCreate(LPCREATESTRUCT createStruct);
  afx_msg void OnSize(UINT type, int cx, int cy);
  afx_msg void OnSetFocus(CWnd* oldWnd);

  DECLARE_MESSAGE_MAP()
};
