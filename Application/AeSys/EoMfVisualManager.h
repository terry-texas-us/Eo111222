#pragma once

/// @brief Custom visual manager that reads chrome colors from Eo::ColorSchemeColors,
/// enabling full dark/light theme control beyond what CMFCVisualManagerOffice2007 exposes.
class EoMfVisualManager : public CMFCVisualManagerOffice2007 {
  DECLARE_DYNCREATE(EoMfVisualManager)

 public:
  EoMfVisualManager();
  ~EoMfVisualManager() override;
  EoMfVisualManager(const EoMfVisualManager&) = delete;
  EoMfVisualManager& operator=(const EoMfVisualManager&) = delete;

  /// @brief Refreshes all cached brushes/pens from the current Eo::ColorSchemeColors.
  /// Called after a color scheme change so the next paint cycle uses updated colors.
  void RefreshColors();

  // --- Bar / toolbar background ---
  void OnFillBarBackground(CDC* deviceContext,
      CBasePane* bar,
      CRect rectClient,
      CRect rectClip,
      BOOL ncArea = FALSE) override;
  void OnDrawBarGripper(CDC* deviceContext, CRect rectGripper, BOOL isHorz, CBasePane* bar) override;

  // --- Docking pane caption ---
  COLORREF OnDrawPaneCaption(CDC* deviceContext,
      CDockablePane* bar,
      BOOL active,
      CRect rectCaption,
      CRect rectButtons) override;

  // --- Tabs (MDI and docking pane) ---
  void OnEraseTabsArea(CDC* deviceContext, CRect rect, const CMFCBaseTabCtrl* tabWnd) override;
  BOOL OnEraseTabsFrame(CDC* deviceContext, CRect rect, const CMFCBaseTabCtrl* tabWnd) override;
  void OnDrawTab(CDC* deviceContext,
      CRect rectTab,
      int tabIndex,
      BOOL isActive,
      const CMFCBaseTabCtrl* tabWnd) override;
  void OnDrawTabCloseButton(CDC* deviceContext,
      CRect rect,
      const CMFCBaseTabCtrl* tabWnd,
      BOOL isHighlighted,
      BOOL isPressed,
      BOOL isDisabled) override;
  void OnFillTab(CDC* deviceContext,
      CRect rectFill,
      CBrush* brFill,
      int tabIndex,
      BOOL isActive,
      const CMFCBaseTabCtrl* tabWnd) override;
  COLORREF GetTabTextColor(const CMFCBaseTabCtrl* tabWnd, int tabIndex, BOOL isActive) override;

  // --- MDI client area ---
  BOOL OnEraseMDIClientArea(CDC* deviceContext, CRect rectClient) override;
  int GetMDITabsBordersSize() override;

  // --- Toolbar button text ---
  COLORREF GetToolbarButtonTextColor(CMFCToolBarButton* button, CMFCVisualManager::AFX_BUTTON_STATE state) override;

  // --- Toolbar / caption buttons ---
  void OnFillButtonInterior(CDC* deviceContext,
      CMFCToolBarButton* button,
      CRect rect,
      CMFCVisualManager::AFX_BUTTON_STATE state) override;
  void OnDrawButtonBorder(CDC* deviceContext,
      CMFCToolBarButton* button,
      CRect rect,
      CMFCVisualManager::AFX_BUTTON_STATE state) override;
  void OnDrawCaptionButton(CDC* deviceContext,
      CMFCCaptionButton* button,
      BOOL active,
      BOOL isHorz,
      BOOL isMaximized,
      BOOL isDisabled,
      int imageId = -1) override;

  // --- Toolbar combo boxes ---
  void OnDrawComboBorder(CDC* deviceContext,
      CRect rect,
      BOOL isDisabled,
      BOOL isDropped,
      BOOL isHighlighted,
      CMFCToolBarComboBoxButton* button) override;
  void OnDrawComboDropButton(CDC* deviceContext,
      CRect rect,
      BOOL isDisabled,
      BOOL isDropped,
      BOOL isHighlighted,
      CMFCToolBarComboBoxButton* button) override;

  // --- Menu ---
  COLORREF GetMenuItemTextColor(CMFCToolBarMenuButton* button, BOOL isHighlighted, BOOL isDisabled) override;

  // --- Status bar ---
  void OnDrawStatusBarPaneBorder(CDC* deviceContext, CMFCStatusBar* bar, CRect rectPane, UINT id, UINT style) override;

  // --- Separators ---
  void OnDrawSeparator(CDC* deviceContext, CBasePane* bar, CRect rect, BOOL isHorizontal) override;

  // --- Menu ---
  void OnDrawMenuBorder(CDC* deviceContext, CMFCPopupMenu* menu, CRect rect) override;
  void OnFillMenuImageRect(CDC* deviceContext,
      CMFCToolBarButton* button,
      CRect rect,
      CMFCVisualManager::AFX_BUTTON_STATE state) override;
  void OnHighlightMenuItem(CDC* deviceContext,
      CMFCToolBarMenuButton* button,
      CRect rect,
      COLORREF& highlightTextColor) override;
  COLORREF OnDrawMenuLabel(CDC* deviceContext, CRect rect) override;

  // --- Pane borders and dividers ---
  void OnDrawPaneBorder(CDC* deviceContext, CBasePane* bar, CRect& rect) override;
  void OnDrawPaneDivider(CDC* deviceContext, CPaneDivider* divider, CRect rect, BOOL autoHideMode) override;

 private:
  CBrush m_toolbarBackgroundBrush;
  CBrush m_menuBackgroundBrush;
  CBrush m_captionBrush;
  CBrush m_captionActiveBrush;
  CBrush m_tabActiveBrush;
  CBrush m_tabInactiveBrush;
  CBrush m_statusBarBrush;
  CBrush m_menuHighlightBrush;
  CPen m_borderPen;
  CPen m_separatorPen;
  CPen m_menuHighlightBorderPen;
};
