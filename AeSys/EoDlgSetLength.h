#pragma once

// EoDlgSetLength dialog

class EoDlgSetLength : public CDialog {
  DECLARE_DYNAMIC(EoDlgSetLength)

 public:
  EoDlgSetLength(CWnd* pParent = nullptr);
  EoDlgSetLength(const EoDlgSetLength&) = delete;
  EoDlgSetLength& operator=(const EoDlgSetLength&) = delete;

  virtual ~EoDlgSetLength();

  // Dialog Data
  enum { IDD = IDD_SET_LENGTH };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

 public:
  double m_dLength;
  CString m_strTitle;
};
