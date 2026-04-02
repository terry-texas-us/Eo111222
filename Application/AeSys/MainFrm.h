#pragma once

#include <cstdint>

#include "EoMfOutputDockablePane.h"
#include "EoMfPropertiesDockablePane.h"
#include "EoMfStatusBar.h"

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
  BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
      CWnd* pParentWnd = nullptr, CCreateContext* pContext = nullptr) override;

 public:
  ~CMainFrame() override;

 protected:  // control bar embedded members
  CMFCMenuBar m_menuBar;
  CMFCToolBar m_standardToolBar;
  CMFCToolBar m_renderPropertiesToolBar;  // Properties toolbar (Color, future LineStyle/LineWeight)
  EoMfStatusBar m_statusBar;
  EoMfOutputDockablePane m_outputPane;
  EoMfPropertiesDockablePane m_propertiesPane;
  CMFCToolBarImages m_userImages;
  bool m_useHighDpiToolbar{false};  // True when 32x32 icons are loaded (DPI > 144); false for 24x24

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
  afx_msg LRESULT OnToolbarContextMenu(WPARAM, LPARAM);

  afx_msg LRESULT OnGetTabToolTip(WPARAM wp, LPARAM lp);
  afx_msg LRESULT OnToolbarReset(WPARAM toolbarResourceId, LPARAM lparam);
  afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);

  DECLARE_MESSAGE_MAP()

  BOOL OnShowPopupMenu(CMFCPopupMenu* pMenuPopup) override;
  BOOL OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL bDrop) override;

  /** @brief Creates the dockable panes: Output and Properties.
   *  @return TRUE if successful, FALSE otherwise.
   */
  BOOL CreateDockablePanes();
  void SetDockablePanesIcons(bool highColorMode);

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

  /// @brief Synchronizes the toolbar pen-color combo box with the current ACI index.
  void SyncColorCombo(std::int16_t aciIndex);

  /// @brief Synchronizes the toolbar line-type combo box with the current line type.
  void SyncLineTypeCombo(std::int16_t lineTypeIndex, const std::wstring& lineTypeName);
};
