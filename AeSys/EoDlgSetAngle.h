#pragma once

#include "Resource.h"

class EoDlgSetAngle : public CDialog {
  DECLARE_DYNAMIC(EoDlgSetAngle)

 public:
  EoDlgSetAngle(CWnd* pParent = nullptr);
  EoDlgSetAngle(const EoDlgSetAngle&) = delete;
  EoDlgSetAngle& operator=(const EoDlgSetAngle&) = delete;

  virtual ~EoDlgSetAngle();

  // Dialog Data
  enum { IDD = IDD_SET_ANGLE };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();

 public:
  double m_dAngle;
  CString m_strTitle;
};
