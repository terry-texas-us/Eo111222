#pragma once

class EoDbLayer;

/// @brief Toolbar combo box for layer selection with integrated leading icon and owner-draw state icons.
/// The icon area on the left opens the layer manager dialog on click.
/// Each dropdown item shows On/Off, Freeze, Lock icons, a color swatch,
/// and the layer name. Clicking on icons in the dropdown toggles layer states.
/// The closed combo shows the current work layer with its state icons.
class EoCtrlLayerComboBox : public CMFCToolBarComboBoxButton {
  DECLARE_SERIAL(EoCtrlLayerComboBox)

 public:
  /// @brief Width of the leading icon area (pixels). The CComboBox HWND is offset rightward by this amount.
  static constexpr int kIconAreaWidth = 28;

  EoCtrlLayerComboBox();
  EoCtrlLayerComboBox(const EoCtrlLayerComboBox&) = delete;
  EoCtrlLayerComboBox& operator=(const EoCtrlLayerComboBox&) = delete;

  /// @brief Populates the dropdown with layers from the active document.
  void PopulateItems();

  /// @brief Synchronizes the combo selection to match the given layer name.
  void SetCurrentLayer(const CString& layerName);

  /** @brief Draws the layer manage icon bitmap centered in the combo's leading icon area.
   *
   * @param deviceContext The device context to draw on.
   * @param iconRectangle The rectangle defining the icon area to draw within.
   */
  static void DrawLayerIcon(CDC* deviceContext, const CRect& iconRectangle);

 protected:
  CComboBox* CreateCombo(CWnd* parentWindow, const CRect& rect) override;
  BOOL NotifyCommand(int notifyCode) override;
  void Serialize(CArchive& ar) override;
  void OnDraw(CDC* deviceContext, const CRect& rect, CMFCToolBarImages* images, BOOL isHorz = TRUE,
      BOOL isCustomizeMode = FALSE, BOOL isHighlighted = FALSE, BOOL drawBorder = TRUE,
      BOOL grayDisabledButtons = TRUE) override;
  void OnMove() override;
  BOOL OnClick(CWnd* parentWindow, BOOL delay = TRUE) override;

 private:
  /// @brief Rebuilds the item list from the document's layer table.
  void BuildItemList();

  /// @brief Handles the CBN_SELCHANGE notification — switches the work layer.
  void OnSelectionChanged();

  /// @brief Opens the layer manager dialog.
  void OpenLayerManager();

  /// @brief True while a deferred PopulateItems is queued via PostMessage.
  /// Prevents repeated posting and suppresses content rendering until the list is fresh.
  bool m_rebuildPending{false};
};

/// @brief Owner-draw combo box that renders layer state icons and color swatches.
/// Each item shows: [On/Off] [Freeze] [Lock] [Color] LayerName.
/// Clicking on icon areas in the dropdown toggles the corresponding layer state.
class EoCtrlLayerOwnerDrawCombo : public CComboBox {
 public:
  void MeasureItem(LPMEASUREITEMSTRUCT measureItemStruct) override;
  void DrawItem(LPDRAWITEMSTRUCT drawItemStruct) override;

  /// @brief Back-pointer to the owning toolbar button, set in CreateCombo.
  /// Used by OnRebuildLayers to call PopulateItems() outside the paint handler.
  EoCtrlLayerComboBox* m_ownerButton{nullptr};

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
  afx_msg LRESULT OnRebuildLayers(WPARAM wParam, LPARAM lParam);

 private:
  CBrush m_dropdownBackgroundBrush;
  bool m_listboxSubclassed{false};
};
