#pragma once

#include "afxdialogex.h"
#include <afxwin.h>

#include "Resource.h"

class CDlgSetPointStyle : public CDialogEx {
  DECLARE_DYNAMIC(CDlgSetPointStyle)

 public:
  CDlgSetPointStyle(CWnd* pParent = nullptr);  // standard constructor
  virtual ~CDlgSetPointStyle() override;

  CDlgSetPointStyle(const CDlgSetPointStyle&) = delete;
  CDlgSetPointStyle& operator=(const CDlgSetPointStyle&) = delete;

  // Dialog Data
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_SET_POINT_STYLE };
#endif

  // Full flag value (radio index in low bits plus checkbox flags)

  int m_pointStyle{0};  // current point style (default simple pixel)

  int m_radioPoint{0};        // radio button selection
  BOOL m_checkCircle{FALSE};  // corresponds to first 0x32 circle
  BOOL m_checkSquare{FALSE};  // corresponds to second 0x64 box

  double m_pointSize{0.0};  // mark size: positive=world units, negative=pixels

 protected:
  void DoDataExchange(CDataExchange* pDX) override;  // DDX/DDV support

 protected:
  BOOL OnInitDialog() override;
  void OnOK() override;

  DECLARE_MESSAGE_MAP()
};
