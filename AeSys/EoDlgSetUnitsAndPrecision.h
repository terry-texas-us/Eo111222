#pragma once
#include "Resource.h"

class EoDlgSetUnitsAndPrecision : public CDialog {
  DECLARE_DYNAMIC(EoDlgSetUnitsAndPrecision)

 public:
  EoDlgSetUnitsAndPrecision(CWnd* pParent = nullptr);

  EoDlgSetUnitsAndPrecision(const EoDlgSetUnitsAndPrecision&) = delete;
  EoDlgSetUnitsAndPrecision& operator=(const EoDlgSetUnitsAndPrecision&) = delete;

  virtual ~EoDlgSetUnitsAndPrecision();

  // Dialog Data
  enum { IDD = IDD_UNITS };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

 public:
  CListBox m_MetricUnitsListBoxControl;
  AeSys::Units m_Units;
  int m_Precision;

  afx_msg void OnBnClickedMetric();

 protected:
  DECLARE_MESSAGE_MAP()
};
