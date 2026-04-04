#pragma once

#include "Resource.h"

/// @brief Toolbar combo box for text style selection.
/// Shows all text styles from the document's text style table.
/// Selecting a style applies its properties to the render state.
class EoCtrlTextStyleComboBox : public CMFCToolBarComboBoxButton {
  DECLARE_SERIAL(EoCtrlTextStyleComboBox)

 public:
  EoCtrlTextStyleComboBox();
  EoCtrlTextStyleComboBox(const EoCtrlTextStyleComboBox&) = delete;
  EoCtrlTextStyleComboBox& operator=(const EoCtrlTextStyleComboBox&) = delete;

  /// @brief Populates the dropdown with text style names from the active document.
  void PopulateItems();

  /// @brief Synchronizes the combo selection to match the given text style name (case-insensitive).
  void SetCurrentTextStyle(const std::wstring& textStyleName);

 protected:
  CComboBox* CreateCombo(CWnd* parentWindow, const CRect& rect) override;
  BOOL NotifyCommand(int notifyCode) override;
  void Serialize(CArchive& ar) override;
  void OnDraw(CDC* deviceContext, const CRect& rect, CMFCToolBarImages* images, BOOL isHorz = TRUE,
      BOOL isCustomizeMode = FALSE, BOOL isHighlighted = FALSE, BOOL drawBorder = TRUE,
      BOOL grayDisabledButtons = TRUE) override;

 private:
  /// @brief Rebuilds the item list from the document's text style table.
  void BuildItemList();

  /// @brief Handles the CBN_SELCHANGE notification — applies the selected text style.
  void OnSelectionChanged();
};

/// @brief Theme-aware combo box for the text style toolbar.
/// Custom-paints the closed combo and dropdown list for dark/light scheme consistency.
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
