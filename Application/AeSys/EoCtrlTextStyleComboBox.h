#pragma once

#include "Resource.h"

/// @brief Toolbar combo box for text style selection with integrated leading icon.
/// The icon area on the left opens the text style manager dialog on click.
/// Shows all text styles from the document's text style table.
/// Selecting a style applies its properties to the render state.
class EoCtrlTextStyleComboBox : public CMFCToolBarComboBoxButton {
  DECLARE_SERIAL(EoCtrlTextStyleComboBox)

 public:
  /// @brief Width of the leading icon area (pixels). The CComboBox HWND is offset rightward by this amount.
  static constexpr int kIconAreaWidth = 28;

  EoCtrlTextStyleComboBox();
  EoCtrlTextStyleComboBox(const EoCtrlTextStyleComboBox&) = delete;
  EoCtrlTextStyleComboBox& operator=(const EoCtrlTextStyleComboBox&) = delete;

  /// @brief Populates the dropdown with text style names from the active document.
  void PopulateItems();

  /// @brief Synchronizes the combo selection to match the given text style name (case-insensitive).
  void SetCurrentTextStyle(const std::wstring& textStyleName);

  /// @brief Draws the text style icon bitmap at the given position.
  static void DrawTextStyleIcon(CDC* deviceContext, const CRect& iconRect, COLORREF textColor);

 protected:
  CComboBox* CreateCombo(CWnd* parentWindow, const CRect& rect) override;
  BOOL NotifyCommand(int notifyCode) override;
  void Serialize(CArchive& ar) override;
  void OnDraw(CDC* deviceContext,
      const CRect& rect,
      CMFCToolBarImages* images,
      BOOL isHorz = TRUE,
      BOOL isCustomizeMode = FALSE,
      BOOL isHighlighted = FALSE,
      BOOL drawBorder = TRUE,
      BOOL grayDisabledButtons = TRUE) override;
  void OnMove() override;
  BOOL OnClick(CWnd* parentWindow, BOOL delay = TRUE) override;

 private:
  /// @brief Rebuilds the item list from the document's text style table.
  void BuildItemList();

  /// @brief Handles the CBN_SELCHANGE notification — applies the selected text style.
  void OnSelectionChanged();

  /// @brief Opens the text style manager dialog and refreshes the combo.
  void OpenTextStyleManager();
};

/// @brief Theme-aware combo box for the text style toolbar.
/// Custom-paints the closed combo and dropdown list with leading icon area.
class EoCtrlTextStyleThemedCombo : public CComboBox {
 protected:
  DECLARE_MESSAGE_MAP()
  afx_msg HBRUSH OnCtlColor(CDC* deviceContext, CWnd* control, UINT ctlColor);
  afx_msg void OnNcCalcSize(BOOL calcValidRects, NCCALCSIZE_PARAMS* params);
  afx_msg void OnPaint();
  afx_msg void OnNcPaint();
  afx_msg BOOL OnEraseBkgnd(CDC* deviceContext);

 private:
  CBrush m_dropdownBackgroundBrush;
};
