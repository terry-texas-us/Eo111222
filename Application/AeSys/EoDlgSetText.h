#pragma once
#include "Resource.h"

class EoDlgSetText : public CDialog {
 public:
  EoDlgSetText(CWnd* pParent = nullptr);
  EoDlgSetText(const EoDlgSetText&) = delete;
  EoDlgSetText& operator=(const EoDlgSetText&) = delete;

  virtual ~EoDlgSetText();

  enum { IDD = IDD_SET_TEXT };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();

 public:
  CString m_sText;
  CString m_strTitle;
};
