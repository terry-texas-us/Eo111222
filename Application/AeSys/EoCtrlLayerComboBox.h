#pragma once

#include <cstdint>

#include "Resource.h"

class EoDbLayer;

/// @brief Toolbar combo box for layer selection with owner-draw state icons.
/// Each dropdown item shows On/Off, Freeze, Lock icons, a color swatch,
/// and the layer name. Clicking on icons in the dropdown toggles layer states.
/// The closed combo shows the current work layer with its state icons.
class EoCtrlLayerComboBox : public CMFCToolBarComboBoxButton {
  DECLARE_SERIAL(EoCtrlLayerComboBox)

 public:
  EoCtrlLayerComboBox();
  EoCtrlLayerComboBox(const EoCtrlLayerComboBox&) = delete;
  EoCtrlLayerComboBox& operator=(const EoCtrlLayerComboBox&) = delete;

  /// @brief Populates the dropdown with layers from the active document.
  void PopulateItems();

  /// @brief Synchronizes the combo selection to match the given layer name.
  void SetCurrentLayer(const CString& layerName);

 protected:
  CComboBox* CreateCombo(CWnd* parentWindow, const CRect& rect) override;
  BOOL NotifyCommand(int notifyCode) override;
  void Serialize(CArchive& ar) override;
  void OnDraw(CDC* deviceContext, const CRect& rect, CMFCToolBarImages* images, BOOL isHorz = TRUE,
      BOOL isCustomizeMode = FALSE, BOOL isHighlighted = FALSE, BOOL drawBorder = TRUE,
      BOOL grayDisabledButtons = TRUE) override;

 private:
  /// @brief Rebuilds the item list from the document's layer table.
  void BuildItemList();

  /// @brief Handles the CBN_SELCHANGE notification — switches the work layer.
  void OnSelectionChanged();
};

/// @brief Owner-draw combo box that renders layer state icons and color swatches.
/// Each item shows: [On/Off] [Freeze] [Lock] [Color] LayerName.
/// Clicking on icon areas in the dropdown toggles the corresponding layer state.
class EoCtrlLayerOwnerDrawCombo : public CComboBox {
 public:
  void MeasureItem(LPMEASUREITEMSTRUCT measureItemStruct) override;
  void DrawItem(LPDRAWITEMSTRUCT drawItemStruct) override;

  /// @brief Icon layout constants (pixel offsets within each item).
  /// Icons are rendered at native 24×24 with 24px step.
  static constexpr int kIconSize = 24;
  static constexpr int kIconStep = 24;
  static constexpr int kOnOffX = 2;
  static constexpr int kFreezeX = kOnOffX + kIconStep;
  static constexpr int kLockX = kFreezeX + kIconStep;
  static constexpr int kColorX = kLockX + kIconStep;
  static constexpr int kColorWidth = 20;
  static constexpr int kNameX = kColorX + kColorWidth + 6;

 protected:
  DECLARE_MESSAGE_MAP()
  afx_msg HBRUSH OnCtlColor(CDC* deviceContext, CWnd* control, UINT ctlColor);
  afx_msg void OnNcCalcSize(BOOL calcValidRects, NCCALCSIZE_PARAMS* params);
  afx_msg void OnPaint();
  afx_msg void OnNcPaint();
  afx_msg BOOL OnEraseBkgnd(CDC* deviceContext);

 private:
  CBrush m_dropdownBackgroundBrush;
  bool m_listboxSubclassed{false};

  /// @brief Draws a DPI-scaled downward-pointing triangle centered in the given rectangle.
  void DrawDropdownArrow(CDC* deviceContext, const CRect& rect, COLORREF arrowColor);
};
