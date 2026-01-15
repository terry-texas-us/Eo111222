#pragma once
#include "Resource.h"

class EoDlgSetHomePoint : public CDialog {
  DECLARE_DYNAMIC(EoDlgSetHomePoint)

 public:
  EoDlgSetHomePoint(CWnd* pParent = nullptr);
  EoDlgSetHomePoint(AeSysView* activeView, CWnd* pParent = nullptr);
  EoDlgSetHomePoint(const EoDlgSetHomePoint&) = delete;
  EoDlgSetHomePoint& operator=(const EoDlgSetHomePoint&) = delete;

  virtual ~EoDlgSetHomePoint();

  // Dialog Data
  enum { IDD = IDD_HOME_POINT_EDIT };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  AeSysView* m_ActiveView{nullptr};
  static EoGePoint3d m_CursorPosition;

 public:
  CComboBox m_HomePointNames;
  CEdit m_X;
  CEdit m_Y;
  CEdit m_Z;

  afx_msg void OnCbnEditupdateList();

 protected:
  DECLARE_MESSAGE_MAP()
};
