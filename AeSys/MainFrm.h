#pragma once

#include "EoMfOutputDockablePane.h"
#include "EoMfPropertiesDockablePane.h"

const int nStatusIcon = 0;
const int nStatusInfo = 1;
const int nStatusProgress = 2;
const int nStatusOp0 = 3;

class CMainFrame : public CMDIFrameWndEx {
  DECLARE_DYNAMIC(CMainFrame)
 public:
  CMainFrame();
  CMainFrame(const CMainFrame&) = delete;
  CMainFrame& operator=(const CMainFrame&) = delete;
  // Attributes
 private:
  UINT m_ApplicationLook;
  int m_CurrentProgress;
  bool m_InProgress;

  // Operations
 public:
  void UpdateMDITabs(BOOL resetMDIChild);

  // Overrides
 public:
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
  virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
                         CWnd* pParentWnd = nullptr, CCreateContext* pContext = nullptr);

  // Implementation
 public:
  static CMFCToolBarComboBoxButton* GetFindCombo(void);
  virtual ~CMainFrame();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

 protected:  // control bar embedded members
  CMFCMenuBar m_MenuBar;
  CMFCToolBar m_StandardToolBar;
  CMFCStatusBar m_StatusBar;
  EoMfOutputDockablePane m_OutputPane;
  EoMfPropertiesDockablePane m_PropertiesPane;
  CMFCToolBarImages m_UserImages;

 protected:
  afx_msg int OnCreate(LPCREATESTRUCT createStruct);
  afx_msg void OnDestroy();
  afx_msg void OnWindowManager();
  afx_msg void OnMdiTabbed();
  afx_msg void OnUpdateMdiTabbed(CCmdUI* pCmdUI);
  afx_msg void OnViewCustomize(void);
  afx_msg void OnViewFullScreen(void);
  afx_msg LRESULT OnToolbarContextMenu(WPARAM, LPARAM);
  afx_msg void OnApplicationLook(UINT id);
  afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);

  afx_msg LRESULT OnGetTabToolTip(WPARAM wp, LPARAM lp);
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg LRESULT OnToolbarReset(WPARAM toolbarResourceId, LPARAM lparam);
  afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);

  DECLARE_MESSAGE_MAP()

  virtual BOOL OnShowPopupMenu(CMFCPopupMenu* pMenuPopup);
  virtual BOOL OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL bDrop);

  /// <summary></summary>
  /// <remarks>
  // CBRS_FLOAT_MULTI allows panes to float together in a single window. By default, panes only float individually.
  /// </remarks>
  BOOL CreateDockablePanes();
  void SetDockablePanesIcons(bool highColorMode);

 public:
  CString GetPaneText(int index);
  void SetPaneInfo(int index, UINT newId, UINT nStyle, int width);
  BOOL SetPaneText(int index, LPCWSTR newText);
  void SetPaneStyle(int index, UINT style);
  void SetPaneTextColor(int index, COLORREF textColor = COLORREF(-1));
  void OnStartProgress(void);

  CMFCStatusBar& GetStatusBar(void) { return m_StatusBar; }
  EoMfOutputDockablePane& GetOutputPane(void) { return m_OutputPane; }
  EoMfPropertiesDockablePane& GetPropertiesPane(void) { return m_PropertiesPane; }
};
