#pragma once
#include "Resource.h"

class EoDlgActiveViewKeyplan : public CDialog {
  DECLARE_DYNAMIC(EoDlgActiveViewKeyplan)

 public:
  EoDlgActiveViewKeyplan(CWnd* pParent = nullptr);
  EoDlgActiveViewKeyplan(AeSysView* view, CWnd* pParent = nullptr);
  EoDlgActiveViewKeyplan(const EoDlgActiveViewKeyplan&) = delete;
  EoDlgActiveViewKeyplan& operator=(const EoDlgActiveViewKeyplan&) = delete;

  virtual ~EoDlgActiveViewKeyplan();

  enum { IDD = IDD_ACTIVE_VIEW_KEYPLAN };

  static HBITMAP m_hbmKeyplan;
  static CRect m_rcWnd;

  double m_dRatio;

 protected:
  static bool bKeyplan;

  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  void OnOK() override;

  AeSysView* m_ActiveView{};
  void Refresh();

 public:
  afx_msg void OnBnClickedRecall();
  afx_msg void OnBnClickedSave();
  afx_msg void OnEnKillfocusRatio();

 protected:
  DECLARE_MESSAGE_MAP()
};

// User defined message sent by EoDlgActiveViewKeyplan to WndProcKeyPlan when a new zoom ratio is set using an edit control (LPARAM is the new value of ratio)
#define WM_USER_ON_NEW_RATIO WM_USER + 1
