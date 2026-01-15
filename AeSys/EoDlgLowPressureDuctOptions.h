#pragma once
#include "Resource.h"

class EoDlgLowPressureDuctOptions : public CDialog {
  DECLARE_DYNAMIC(EoDlgLowPressureDuctOptions)

 public:
  EoDlgLowPressureDuctOptions(CWnd* parent = nullptr);
  EoDlgLowPressureDuctOptions(const EoDlgLowPressureDuctOptions&) = delete; 
  EoDlgLowPressureDuctOptions& operator=(const EoDlgLowPressureDuctOptions&) = delete;

  virtual ~EoDlgLowPressureDuctOptions();

  // Dialog Data
  enum { IDD = IDD_DLGPROC_LPD_OPTIONS };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();

 public:
  double m_Width;
  double m_Depth;
  double m_RadiusFactor;
  bool m_GenerateVanes{false};
  int m_Justification;
  bool m_BeginWithTransition{false};

  afx_msg void OnBnClickedOk();
  afx_msg void OnBnClickedGenVanes();
  afx_msg void OnEnChangeWidth();

 protected:
  DECLARE_MESSAGE_MAP()
};
