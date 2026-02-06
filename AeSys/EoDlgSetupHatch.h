#pragma once
#include "Resource.h"

class EoDlgSetupHatch : public CDialog {
 public:
  EoDlgSetupHatch(CWnd* pParent = nullptr);
  EoDlgSetupHatch(const EoDlgSetupHatch&) = delete;
  EoDlgSetupHatch& operator=(const EoDlgSetupHatch&) = delete;
  
  virtual ~EoDlgSetupHatch();

  enum { IDD = IDD_SETUP_HATCH };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);

 public:
  virtual BOOL OnInitDialog();
  virtual void OnOK();
  double m_HatchXScaleFactor;
  double m_HatchYScaleFactor;
  double m_HatchRotationAngle;
};
