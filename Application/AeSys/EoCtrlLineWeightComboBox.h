#pragma once

#include <cstdint>

#include "EoDxfLineWeights.h"
#include "Resource.h"

/// @brief Owner-draw toolbar combo box for DXF line weight selection.
/// Shows By Layer, By Block, Default, and the 24 standard DXF line weights with
/// thickness-proportional line previews.
class EoCtrlLineWeightComboBox : public CMFCToolBarComboBoxButton {
  DECLARE_SERIAL(EoCtrlLineWeightComboBox)

 public:
  EoCtrlLineWeightComboBox();
  EoCtrlLineWeightComboBox(const EoCtrlLineWeightComboBox&) = delete;
  EoCtrlLineWeightComboBox& operator=(const EoCtrlLineWeightComboBox&) = delete;

  /// @brief Populates the dropdown with default items.
  void PopulateItems();

  /// @brief Synchronizes the combo selection to match the given line weight enum value.
  void SetCurrentLineWeight(EoDxfLineWeights::LineWeight lineWeight);

  /// @brief Returns the display name for a given LineWeight enum value (e.g. "0.25 mm", "ByLayer").
  static CString LineWeightToName(EoDxfLineWeights::LineWeight lineWeight);

  /// @brief Draws a thickness-proportional line preview into the given rectangle.
  static void DrawWeightPreview(CDC* deviceContext, const CRect& rect,
      EoDxfLineWeights::LineWeight lineWeight, COLORREF lineColor);

 protected:
  CComboBox* CreateCombo(CWnd* parentWindow, const CRect& rect) override;
  BOOL NotifyCommand(int notifyCode) override;
  void Serialize(CArchive& ar) override;
  void OnDraw(CDC* deviceContext, const CRect& rect, CMFCToolBarImages* images, BOOL isHorz = TRUE,
      BOOL isCustomizeMode = FALSE, BOOL isHighlighted = FALSE, BOOL drawBorder = TRUE,
      BOOL grayDisabledButtons = TRUE) override;

 private:
  /// @brief Rebuilds the item list: ByLayer, ByBlock, Default, then 24 standard weights.
  void BuildItemList();

  /// @brief Handles the CBN_SELCHANGE notification — applies the selected line weight.
  void OnSelectionChanged();
};

/// @brief Owner-draw combo box control that draws thickness-proportional line previews.
/// Custom-paints the entire closed combo for a flat, theme-consistent look.
class EoCtrlLineWeightOwnerDrawCombo : public CComboBox {
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
  void DrawDropdownArrow(CDC* deviceContext, const CRect& rect, COLORREF arrowColor);
};
