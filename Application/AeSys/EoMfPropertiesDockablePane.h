#pragma once

class EoMfPropertiesMFCToolBar : public CMFCToolBar {
 public:
  EoMfPropertiesMFCToolBar() = default;
  EoMfPropertiesMFCToolBar(const EoMfPropertiesMFCToolBar&) = delete;
  EoMfPropertiesMFCToolBar& operator=(const EoMfPropertiesMFCToolBar&) = delete;

  virtual void OnUpdateCmdUI(CFrameWnd* target, BOOL disableIfNoHndler) {
    (void)target;
    CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), disableIfNoHndler);
  }
};

class EoMfPropertiesDockablePane : public CDockablePane {
 public:  // Construction
  EoMfPropertiesDockablePane();
  EoMfPropertiesDockablePane(const EoMfPropertiesDockablePane&) = delete;
  EoMfPropertiesDockablePane& operator=(const EoMfPropertiesDockablePane&) = delete;

 protected:  // Attributes
  CMFCPropertyGridCtrl m_PropertyGrid;
  CFont m_PropertyGridFont;
  EoMfPropertiesMFCToolBar m_PropertiesToolBar;

  enum PropertyDataTag { kActiveViewScale = 100, kWorkGroupCount = 200, kTrapGroupCount = 201 };

 public:  // Overrides
 public:  // Implementation
  virtual ~EoMfPropertiesDockablePane();

 protected:
  afx_msg int OnCreate(LPCREATESTRUCT createStruct);
  afx_msg void OnSetFocus(CWnd* oldWindow);
  afx_msg void OnSettingChange(UINT uFlags, LPCWSTR lpszSection);
  afx_msg void OnSize(UINT type, int cx, int cy);

  afx_msg LRESULT OnPropertyChanged(WPARAM, LPARAM);

  afx_msg void OnExpandAllProperties();
  afx_msg void OnProperties1();
  afx_msg void OnSortProperties();
  afx_msg void OnUpdateExpandAllProperties(CCmdUI* pCmdUI);
  afx_msg void OnUpdateProperties1(CCmdUI* pCmdUI);
  afx_msg void OnUpdateSortProperties(CCmdUI* pCmdUI);

  DECLARE_MESSAGE_MAP()

 protected:  // Operations
  void AdjustLayout() override;
  void InitializePropertyGrid();
  void SetPropertyGridFont();

 public:  // Operations
  CMFCPropertyGridCtrl& GetPropertyGridCtrl() { return m_PropertyGrid; }
  CMFCPropertyGridProperty& GetActiveViewScaleProperty() { return *m_PropertyGrid.FindItemByData(kActiveViewScale); }

  /// @brief Updates the Document Statistics group with current work and trap group counts.
  void UpdateDocumentStatistics();

  /// @brief Applies the active color scheme to the property grid.
  void ApplyColorScheme();
};
