#pragma once

// EoDlgSetText dialog

class EoDlgSetText : public CDialog {
  DECLARE_DYNAMIC(EoDlgSetText)

 public:
  EoDlgSetText(CWnd* pParent = nullptr);
  EoDlgSetText(const EoDlgSetText&) = delete;
  EoDlgSetText& operator=(const EoDlgSetText&) = delete;

  virtual ~EoDlgSetText();

  // Dialog Data
  enum { IDD = IDD_SET_TEXT };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();

 public:
  CString m_sText;
  CString m_strTitle;
};
