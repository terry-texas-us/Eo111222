#pragma once
#include "Resource.h"

class EoDlgSetupNote : public CDialog {
 public:
  EoDlgSetupNote(CWnd* pParent = nullptr);
  EoDlgSetupNote(EoDbFontDefinition* fontDefinition, CWnd* pParent = nullptr);
  EoDlgSetupNote(const EoDlgSetupNote&) = delete;
  EoDlgSetupNote& operator=(const EoDlgSetupNote&) = delete;

  virtual ~EoDlgSetupNote();

  enum { IDD = IDD_SETUP_NOTE };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

 public:
  EoDbFontDefinition* m_FontDefinition{};

  CMFCFontComboBox m_MfcFontComboControl;

  double m_height;
  double m_expansionFactor;
  double m_slantAngle;
  double m_rotationAngle;
};
