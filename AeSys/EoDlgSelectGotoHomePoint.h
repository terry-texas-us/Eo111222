#pragma once

// EoDlgSelectGotHomePoint dialog

class EoDlgSelectGotoHomePoint : public CDialog {
  DECLARE_DYNAMIC(EoDlgSelectGotoHomePoint)

 public:
  EoDlgSelectGotoHomePoint(CWnd* pParent = nullptr);
  EoDlgSelectGotoHomePoint(AeSysView* currentView, CWnd* pParent = nullptr);
  EoDlgSelectGotoHomePoint(const EoDlgSelectGotoHomePoint&) = delete;
  EoDlgSelectGotoHomePoint& operator=(const EoDlgSelectGotoHomePoint&) = delete;

  virtual ~EoDlgSelectGotoHomePoint();

  // Dialog Data
  enum { IDD = IDD_HOME_POINT_GO };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  AeSysView* m_ActiveView;

 public:
  CComboBox m_HomePointNames;
  CEdit m_X;
  CEdit m_Y;
  CEdit m_Z;

  afx_msg void OnCbnEditupdateList();
  afx_msg void OnCbnSelchangeList();

 protected:
  DECLARE_MESSAGE_MAP()
};
