#pragma once

#include "Resource.h"

class EoDlgSetLength : public CDialog {
 public:
  EoDlgSetLength(CWnd* pParent = nullptr);
  EoDlgSetLength(const EoDlgSetLength&) = delete;
  EoDlgSetLength& operator=(const EoDlgSetLength&) = delete;

  virtual ~EoDlgSetLength();

  enum { IDD = IDD_SET_LENGTH };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

 public:
  double m_dLength{0.0};
  CString m_strTitle;
};
