#pragma once
#include "Resource.h"

class EoDlgSetupConstraints : public CDialog {
  DECLARE_DYNAMIC(EoDlgSetupConstraints)

 public:
  EoDlgSetupConstraints(CWnd* parent = nullptr);
  EoDlgSetupConstraints(AeSysView* view, CWnd* parent = nullptr);
  EoDlgSetupConstraints(const EoDlgSetupConstraints&) = delete;
  EoDlgSetupConstraints& operator=(const EoDlgSetupConstraints&) = delete;

  virtual ~EoDlgSetupConstraints();

  // Dialog Data
  enum { IDD = IDD_SETUP_CONSTRAINTS_GRID };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  AeSysView* m_ActiveView{};

  CEdit m_GridXSnapSpacing;
  CEdit m_GridYSnapSpacing;
  CEdit m_GridZSnapSpacing;
  CEdit m_GridXPointSpacing;
  CEdit m_GridYPointSpacing;
  CEdit m_GridZPointSpacing;
  CEdit m_GridXLineSpacing;
  CEdit m_GridYLineSpacing;
  CEdit m_GridZLineSpacing;

  CButton m_GridSnapEnableCheckBox;
  CButton m_GridDisplayCheckBox;
  CButton m_GridLineDisplayCheckBox;

  CEdit m_AxisInfluenceAngle;
  CEdit m_AxisZOffsetAngle;
};
