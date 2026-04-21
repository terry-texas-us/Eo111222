#pragma once

#include <cstdint>

#include "Resource.h"

class EoDbLineType;

/// @brief Owner-draw toolbar combo box for line type selection.
/// Shows By Layer, By Block, Continuous, document-loaded line types with dash pattern previews,
/// and "Load Line Types..." which opens the EoDlgLineTypesSelection dialog.
class EoCtrlLineTypeComboBox : public CMFCToolBarComboBoxButton {
  DECLARE_SERIAL(EoCtrlLineTypeComboBox)

 public:
  EoCtrlLineTypeComboBox();
  EoCtrlLineTypeComboBox(const EoCtrlLineTypeComboBox&) = delete;
  EoCtrlLineTypeComboBox& operator=(const EoCtrlLineTypeComboBox&) = delete;

  /// @brief Item data sentinel for "Load Line Types..." entry.
  static constexpr DWORD_PTR kLoadLineTypes = static_cast<DWORD_PTR>(-1);

  /// @brief Populates the dropdown with default items plus document line types.
  void PopulateItems();

  /// @brief Synchronizes the combo selection to match the given line type index and name.
  void SetCurrentLineType(std::int16_t lineTypeIndex, const std::wstring& lineTypeName);

  /// @brief Draws a dash pattern preview into the given rectangle.
  /// @param lineType If null, draws a continuous line (solid).
  static void DrawDashPreview(CDC* deviceContext, const CRect& rect, const EoDbLineType* lineType,
      COLORREF lineColor);

 protected:
  CComboBox* CreateCombo(CWnd* parentWindow, const CRect& rect) override;
  BOOL NotifyCommand(int notifyCode) override;
  void Serialize(CArchive& ar) override;
  void OnDraw(CDC* deviceContext, const CRect& rect, CMFCToolBarImages* images, BOOL isHorz = TRUE,
      BOOL isCustomizeMode = FALSE, BOOL isHighlighted = FALSE, BOOL drawBorder = TRUE,
      BOOL grayDisabledButtons = TRUE) override;

 private:
  /// @brief Rebuilds the item list: ByLayer, ByBlock, document line types, "Load Line Types...".
  void BuildItemList();

  /// @brief Handles the CBN_SELCHANGE notification — applies the selected line type or opens dialog.
  void OnSelectionChanged();
};

/// @brief Owner-draw combo box control that draws dash pattern previews next to item text.
/// Custom-paints the entire closed combo for a flat, theme-consistent look.
class EoCtrlLineTypeOwnerDrawCombo : public CComboBox {
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
