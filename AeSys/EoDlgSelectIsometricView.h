#pragma once
#include "Resource.h"

class EoDlgSelectIsometricView : public CDialog {
 public:
  EoDlgSelectIsometricView(CWnd* pParent = nullptr);
  EoDlgSelectIsometricView(const EoDlgSelectIsometricView&) = delete;	
  EoDlgSelectIsometricView& operator=(const EoDlgSelectIsometricView&) = delete;

  virtual ~EoDlgSelectIsometricView();

  enum { IDD = IDD_SELECT_ISOMETRIC_VIEW };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();

 public:
  int m_LeftRight{};
  int m_FrontBack{};
  int m_AboveUnder{};
};
