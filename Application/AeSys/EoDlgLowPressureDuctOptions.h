#pragma once
#include "Resource.h"

class EoDlgLowPressureDuctOptions : public CDialog {
 public:
  EoDlgLowPressureDuctOptions(CWnd* parent = nullptr);
  EoDlgLowPressureDuctOptions(const EoDlgLowPressureDuctOptions&) = delete; 
  EoDlgLowPressureDuctOptions& operator=(const EoDlgLowPressureDuctOptions&) = delete;

  virtual ~EoDlgLowPressureDuctOptions();

  enum { IDD = IDD_DLGPROC_LPD_OPTIONS };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;

 public:
  double m_Width;
  double m_Depth;
  double m_RadiusFactor;
  bool m_GenerateVanes{};
  int m_Justification;
  bool m_BeginWithTransition{};

  afx_msg void OnBnClickedOk();
  afx_msg void OnBnClickedGenVanes();
  afx_msg void OnEnChangeWidth();

 protected:
  DECLARE_MESSAGE_MAP()
};
