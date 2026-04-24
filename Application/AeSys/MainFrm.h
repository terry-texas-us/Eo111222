#pragma once

#include <cstdint>

#include "EoDxfLineWeights.h"
#include "EoMfOutputDockablePane.h"
#include "EoMfPropertiesDockablePane.h"
#include "EoMfStatusBar.h"

/// @brief Toolbar subclass that skips button state serialization.
/// MFC's CMFCToolBar::SaveState/LoadState persists button images, combo items,
/// and custom button state as opaque binary blobs in the registry. Any mismatch
/// between saved and runtime state (schema change, DPI change, metric override)
/// causes cascading corruption — lost icons, text-only labels, duplicated items.
/// This subclass returns TRUE from both methods so MFC considers state handled
/// but no registry I/O occurs. Buttons are rebuilt from resources and OnToolbarReset
/// on every launch. Docking position is unaffected (managed by CDockingManager).
class EoMfStatelessToolBar : public CMFCToolBar {
 public:
  BOOL LoadState(LPCTSTR /*lpszProfileName*/ = nullptr, int /*nIndex*/ = -1, UINT /*uiID*/ = (UINT)-1) override {
    return TRUE;
  }
  BOOL SaveState(LPCTSTR /*lpszProfileName*/ = nullptr, int /*nIndex*/ = -1, UINT /*uiID*/ = (UINT)-1) override {
    return TRUE;
  }

  /// @brief Sets button and image sizes for both regular and locked modes.
  /// CMFCToolBar::SetSizes() updates only m_sizeButton.
  /// CMFCToolBar::GetButtonSize() returns m_sizeButtonLocked when m_bLocked is TRUE
  /// (set by LoadToolBar with bLocked=TRUE), so SetSizes() alone has no effect on
  /// locked toolbars. This method updates both so the size takes effect regardless
  /// of locked state. bDontAdjust=TRUE defers layout to the caller's RecalcLayout.
  void SetSizesAll(const CSize& buttonSize, const CSize& imageSize) {
    SetSizes(buttonSize, imageSize);
    SetLockedSizes(buttonSize, imageSize, TRUE);
  }
};

class CMainFrame : public CMDIFrameWndEx {
  DECLARE_DYNAMIC(CMainFrame)
 public:
  CMainFrame();
  CMainFrame(const CMainFrame&) = delete;
  CMainFrame& operator=(const CMainFrame&) = delete;

  // Operations
 public:
  void UpdateMDITabs(BOOL resetMDIChild);

 public:
  BOOL PreCreateWindow(CREATESTRUCT& cs) override;
  BOOL LoadFrame(UINT resourceId, DWORD defaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
      CWnd* parentWnd = nullptr, CCreateContext* createContext = nullptr) override;

 public:
  ~CMainFrame() override;

 protected:  // control bar embedded members
  CMFCMenuBar m_menuBar;
  CMFCToolBar m_standardToolBar;  // Standard toolbar (no custom controls — plain CMFCToolBar)
  EoMfStatelessToolBar m_renderPropertiesToolBar;  // Properties toolbar (Color, LineType, LineWeight)
  EoMfStatelessToolBar m_layerPropertiesToolBar;  // Layer toolbar (Layer Manager button, Layer combo)
  EoMfStatelessToolBar m_stylesToolBar;  // Styles toolbar (Text Style)
  EoMfStatusBar m_statusBar;
  EoMfOutputDockablePane m_outputPane;
  EoMfPropertiesDockablePane m_propertiesPane;
  CMFCToolBarImages m_userImages;

 protected:
  afx_msg int OnCreate(LPCREATESTRUCT createStruct);
  afx_msg void OnDestroy();
  afx_msg void OnWindowManager();
  afx_msg void OnMdiTabbed();
  afx_msg void OnUpdateMdiTabbed(CCmdUI* pCmdUI);
  afx_msg void OnViewCustomize();
  afx_msg void OnViewFullScreen();
  afx_msg void OnUpdatePenColorCombo(CCmdUI* pCmdUI);
  afx_msg void OnUpdateLineTypeCombo(CCmdUI* pCmdUI);
  afx_msg void OnUpdateLineWeightCombo(CCmdUI* pCmdUI);
  afx_msg void OnUpdateTextStyleCombo(CCmdUI* pCmdUI);
  afx_msg void OnUpdateLayerCombo(CCmdUI* pCmdUI);
  afx_msg LRESULT OnToolbarContextMenu(WPARAM, LPARAM);

  afx_msg LRESULT OnGetTabToolTip(WPARAM wp, LPARAM lp);
  afx_msg LRESULT OnToolbarReset(WPARAM toolbarResourceId, LPARAM lparam);
  afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM name);

  DECLARE_MESSAGE_MAP()

  BOOL OnShowPopupMenu(CMFCPopupMenu* pMenuPopup) override;
  BOOL OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL bDrop) override;

  /** @brief Creates the dockable panes: Output and Properties.
   *  @return TRUE if successful, FALSE otherwise.
   */
  BOOL CreateDockablePanes();
  void SetDockablePanesIcons();

  /// @brief Measures the combo HWND closed height and re-applies SetSizes on all
  /// application toolbars so icon-only and combo-containing toolbars report identical
  /// row heights. Must be called after any operation that can reset button sizes
  /// (LoadBitmap, LoadToolBar, docking state restore).
  void AdjustToolbarSizesToMatchCombos();

 public:
  CString GetPaneText(int index);
  void SetPaneInfo(int index, UINT newId, UINT nStyle, int width);
  BOOL SetPaneText(int index, LPCWSTR newText);
  void SetPaneStyle(int index, UINT style);
  void SetPaneTextColor(int index, COLORREF textColor = COLORREF(-1));
  void SetPaneBackgroundColor(int index, COLORREF backgroundColor = COLORREF(-1));

  EoMfStatusBar& GetStatusBar() { return m_statusBar; }
  EoMfOutputDockablePane& GetOutputPane() { return m_outputPane; }
  EoMfPropertiesDockablePane& GetPropertiesPane() { return m_propertiesPane; }

  /// @brief Propagates the active color scheme to docking panes and chrome.
  void ApplyColorScheme();

  /// @brief Ensures all application toolbars are visible after docking state restore.
  /// Safety net against corrupted or stale registry docking state blobs.
  void EnsureToolbarsVisible();

  /// @brief Synchronizes the toolbar pen-color combo box with the current ACI index.
  void SyncColorCombo(std::int16_t aciIndex);

  /// @brief Synchronizes the toolbar line-type combo box with the current line type.
  void SyncLineTypeCombo(std::int16_t lineTypeIndex, const std::wstring& lineTypeName);

  /// @brief Synchronizes the toolbar line-weight combo box with the current line weight.
  void SyncLineWeightCombo(EoDxfLineWeights::LineWeight lineWeight);

  /// @brief Synchronizes the toolbar text-style combo box with the given text style name.
  void SyncTextStyleCombo(const std::wstring& textStyleName);

  /// @brief Synchronizes the toolbar layer combo box with the current work layer.
  void SyncLayerCombo(const CString& layerName);
};
