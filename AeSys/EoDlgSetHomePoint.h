#pragma once
#include "Resource.h"

class EoDlgSetHomePoint : public CDialog {
 public:
  EoDlgSetHomePoint(CWnd* pParent = nullptr);
  EoDlgSetHomePoint(AeSysView* activeView, CWnd* pParent = nullptr);
  EoDlgSetHomePoint(const EoDlgSetHomePoint&) = delete;
  EoDlgSetHomePoint& operator=(const EoDlgSetHomePoint&) = delete;

  virtual ~EoDlgSetHomePoint();

  enum { IDD = IDD_HOME_POINT_EDIT };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  void OnOK() override;

  AeSysView* m_ActiveView{};
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
