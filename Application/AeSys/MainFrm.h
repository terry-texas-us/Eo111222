#pragma once

#include "EoMfOutputDockablePane.h"
#include "EoMfPropertiesDockablePane.h"

class CMainFrame : public CMDIFrameWndEx {
  DECLARE_DYNAMIC(CMainFrame)
 public:
  CMainFrame();
  CMainFrame(const CMainFrame&) = delete;
  CMainFrame& operator=(const CMainFrame&) = delete;
  // Attributes
 private:
  UINT m_applicationLook;
  int m_currentProgress;
  bool m_inProgress;

  // Operations
 public:
  void UpdateMDITabs(BOOL resetMDIChild);

 public:
  BOOL PreCreateWindow(CREATESTRUCT& cs) override;
  BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
                 CWnd* pParentWnd = nullptr, CCreateContext* pContext = nullptr) override;

 public:
  static CMFCToolBarComboBoxButton* GetFindCombo();
  ~CMainFrame() override;

 protected:  // control bar embedded members
  CMFCMenuBar m_menuBar;
  CMFCToolBar m_standardToolBar;
  CMFCStatusBar m_statusBar;
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
  afx_msg LRESULT OnToolbarContextMenu(WPARAM, LPARAM);
  afx_msg void OnApplicationLook(UINT id);
  afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);

  afx_msg LRESULT OnGetTabToolTip(WPARAM wp, LPARAM lp);
  afx_msg void OnTimer(UINT_PTR nIDEvent);
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
  void OnStartProgress();

  CMFCStatusBar& GetStatusBar() { return m_statusBar; }
  EoMfOutputDockablePane& GetOutputPane() { return m_outputPane; }
  EoMfPropertiesDockablePane& GetPropertiesPane() { return m_propertiesPane; }
};
