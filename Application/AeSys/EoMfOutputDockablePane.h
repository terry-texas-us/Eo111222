#pragma once
#include <string>

class EoMfOutputListBox : public CListBox {
 public:  // Construction
  EoMfOutputListBox();
  EoMfOutputListBox(const EoMfOutputListBox&) = delete;
  EoMfOutputListBox& operator=(const EoMfOutputListBox&) = delete;

 public:  // Implementation
  virtual ~EoMfOutputListBox();

 protected:
  afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
  afx_msg void OnEditCopy();
  afx_msg void OnEditClear();
  afx_msg void OnViewOutput();

  DECLARE_MESSAGE_MAP()
};

class EoMfOutputDockablePane : public CDockablePane {
 public:  // Construction
  EoMfOutputDockablePane();
  EoMfOutputDockablePane(const EoMfOutputDockablePane&) = delete;
  EoMfOutputDockablePane& operator=(const EoMfOutputDockablePane&) = delete;

 protected:  // Attributes
  CFont m_Font;
  CMFCTabCtrl m_wndTabs;

  EoMfOutputListBox m_OutputMessagesList;
  EoMfOutputListBox m_OutputReportsList;

 public:  // Implementation
  virtual ~EoMfOutputDockablePane();
  void ModifyCaption(const CString& string) { SetWindowTextW(string); }
  void AddStringToMessageList(const CString& string) { m_OutputMessagesList.AddString(string); }
  void AddStringToMessageList(const std::wstring& message) { m_OutputMessagesList.AddString(message.c_str()); }
  void AddStringToReportsList(const CString& string) { m_OutputReportsList.AddString(string); }

 protected:
  afx_msg int OnCreate(LPCREATESTRUCT createStruct);
  afx_msg void OnSize(UINT type, int cx, int cy);

  DECLARE_MESSAGE_MAP()
};
