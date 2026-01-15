#pragma once
#include "Resource.h"

class EoDlgDrawOptions : public CDialog {
  DECLARE_DYNAMIC(EoDlgDrawOptions)

 public:
  EoDlgDrawOptions(CWnd* parent = nullptr);
  EoDlgDrawOptions(const EoDlgDrawOptions&) = delete;
  EoDlgDrawOptions& operator=(const EoDlgDrawOptions&) = delete;

  virtual ~EoDlgDrawOptions();

  enum { IDD = IDD_DRAW_OPTIONS };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  void OnOK() override;

 public:
  afx_msg void OnBnClickedPen();
  afx_msg void OnBnClickedLine();
  afx_msg void OnBnClickedText();
  afx_msg void OnBnClickedFill();
  afx_msg void OnBnClickedConstraints();

 protected:
  DECLARE_MESSAGE_MAP()
};
