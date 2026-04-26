#pragma once

/// @brief Custom status bar with edit-in-place support for the Length, Angle, Scale, and Zoom panes.
/// Double-clicking a supported pane shows an inline CEdit control for direct entry.
/// Enter commits, Escape/focus-loss cancels.
class EoMfStatusBar : public CMFCStatusBar {
  DECLARE_DYNAMIC(EoMfStatusBar)

 public:
  EoMfStatusBar() = default;
  EoMfStatusBar(const EoMfStatusBar&) = delete;
  EoMfStatusBar& operator=(const EoMfStatusBar&) = delete;

  /// @brief Pane index for the dimension length display.
  static constexpr int lengthPaneIndex = 1;

  /// @brief Pane index for the dimension angle display.
  static constexpr int anglePaneIndex = 2;

  /// @brief Pane index for the world scale display.
  static constexpr int scalePaneIndex = 13;

  /// @brief Pane index for the zoom ratio display.
  static constexpr int zoomPaneIndex = 14;

  /// @brief Returns true if an inline edit control is currently active.
  [[nodiscard]] bool IsEditing() const noexcept { return m_editingPane >= 0; }

 protected:
  afx_msg void OnLButtonDblClk(UINT flags, CPoint point);
  afx_msg void OnEditKillFocus();
  BOOL PreTranslateMessage(MSG* msg) override;

  DECLARE_MESSAGE_MAP()

 private:
  /// @brief Shows an inline edit control over the Length pane, pre-filled with the current value.
  void BeginLengthEdit();

  /// @brief Parses the edit text, applies the new dimension length, and hides the edit control.
  void CommitLengthEdit();

  /// @brief Shows an inline edit control over the Angle pane, pre-filled with the current value.
  void BeginAngleEdit();

  /// @brief Parses the edit text, applies the new dimension angle, and hides the edit control.
  void CommitAngleEdit();

  /// @brief Shows an inline edit control over the Scale pane, pre-filled with the current value.
  void BeginScaleEdit();

  /// @brief Parses the edit text, applies the new world scale, and hides the edit control.
  void CommitScaleEdit();

  /// @brief Shows an inline edit control over the Zoom pane, pre-filled with the current value.
  void BeginZoomEdit();

  /// @brief Parses the edit text, applies the new zoom ratio, and hides the edit control.
  void CommitZoomEdit();

  /// @brief Commits the active edit (dispatches to the appropriate pane commit).
  void CommitEdit();

  /// @brief Hides the edit control without applying changes.
  void CancelEdit();

  CEdit m_edit;  ///< Shared inline edit control for pane editing.
  int m_editingPane{-1};  ///< Pane index currently being edited, or -1 if not editing.
};
