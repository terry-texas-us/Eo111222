#pragma once
#include "Resource.h"

class EoDlgSelectIsometricView : public CDialog {
  DECLARE_DYNAMIC(EoDlgSelectIsometricView)

 public:
  EoDlgSelectIsometricView(CWnd* pParent = nullptr);
  EoDlgSelectIsometricView(const EoDlgSelectIsometricView&) = delete;	
  EoDlgSelectIsometricView& operator=(const EoDlgSelectIsometricView&) = delete;

  virtual ~EoDlgSelectIsometricView();

  // Dialog Data
  enum { IDD = IDD_SELECT_ISOMETRIC_VIEW };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();

 public:
  int m_LeftRight{0};
  int m_FrontBack{0};
  int m_AboveUnder{0};
};
