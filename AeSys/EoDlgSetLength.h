#pragma once
#include <Windows.h>
#include <afx.h>
#include <afxstr.h>

#include "Resource.h"

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
  double m_dLength{0.0};
  CString m_strTitle;
};
