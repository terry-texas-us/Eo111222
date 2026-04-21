#pragma once

#include <cstdint>

/// @brief Owner-draw toolbar combo box for ACI pen color selection.
/// Shows By Layer, By Block, the 7 named ACI colors with swatches, and "More Colors...".
/// "More Colors..." opens the full EoDlgSetupColor dialog.
class EoCtrlColorComboBox : public CMFCToolBarComboBoxButton {
  DECLARE_SERIAL(EoCtrlColorComboBox)

 public:
  EoCtrlColorComboBox();
  EoCtrlColorComboBox(const EoCtrlColorComboBox&) = delete;
  EoCtrlColorComboBox& operator=(const EoCtrlColorComboBox&) = delete;

  /// @brief Item data sentinel for "More Colors..." entry.
  static constexpr DWORD_PTR kMoreColors = static_cast<DWORD_PTR>(-1);

  /// @brief Populates the dropdown with default ACI items.
  void PopulateItems();

  /// @brief Synchronizes the combo selection to match the given ACI color index.
  /// If the color is not one of the quick-pick items, displays it as a numeric string.
  void SetCurrentColor(std::int16_t aciIndex);

  /// @brief Returns the COLORREF for a given ACI index (uses Eo::ColorPalette).
  static COLORREF AciToColorRef(std::int16_t aciIndex);

  /// @brief Returns the display name for a given ACI index.
  static CString AciToName(std::int16_t aciIndex);

 protected:
  CComboBox* CreateCombo(CWnd* parentWindow, const CRect& rect) override;
  BOOL NotifyCommand(int notifyCode) override;
  void Serialize(CArchive& ar) override;
  void OnDraw(CDC* deviceContext, const CRect& rect, CMFCToolBarImages* images, BOOL isHorz = TRUE,
      BOOL isCustomizeMode = FALSE, BOOL isHighlighted = FALSE, BOOL drawBorder = TRUE,
      BOOL grayDisabledButtons = TRUE) override;

 private:
  /// @brief Builds the item list with standard colors and an optional custom ACI entry.
  /// Custom entries (ACI 8–255) appear before "More Colors..." with their index as text.
  void BuildItemList(std::int16_t activeAciIndex);

  /// @brief Handles the CBN_SELCHANGE notification — applies the selected color or opens dialog.
  void OnSelectionChanged();
};

/// @brief Owner-draw combo box control that draws color swatches next to item text.
/// Custom-paints the entire closed combo (border, dropdown arrow) for a flat, theme-consistent look.
/// Handles WM_CTLCOLORLISTBOX to paint the dropdown background with the current color scheme.
class EoCtrlColorOwnerDrawCombo : public CComboBox {
 public:
  void MeasureItem(LPMEASUREITEMSTRUCT measureItemStruct) override;
  void DrawItem(LPDRAWITEMSTRUCT drawItemStruct) override;

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
