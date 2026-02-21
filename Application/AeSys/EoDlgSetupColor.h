#pragma once
#include "EoCtrlColorsButton.h"
#include "Resource.h"

class EoDlgSetupColor : public CDialog {
 public:
  EoDlgSetupColor(CWnd* pParent = nullptr);
  EoDlgSetupColor(const EoDlgSetupColor&) = delete;
  EoDlgSetupColor& operator=(const EoDlgSetupColor&) = delete;

  virtual ~EoDlgSetupColor();

  enum { IDD = IDD_SETUP_COLOR };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  void OnOK() override;
  BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

  EoCtrlColorsButton m_EvenColorsButton;
  EoCtrlColorsButton m_OddColorsButton;
  EoCtrlColorsButton m_NamedColorsButton;
  EoCtrlColorsButton m_GraysButton;
  EoCtrlColorsButton m_SelectionButton;

  CEdit m_ColorEditControl;

  void DrawSelectionInformation(std::uint16_t index);

 public:
  std::uint16_t m_ColorIndex;

  afx_msg void OnBnClickedByblockButton();
  afx_msg void OnBnClickedBylayerButton();
  afx_msg void OnClickedEvenColors();
  afx_msg void OnClickedGrays();
  afx_msg void OnClickedNamedColors();
  afx_msg void OnClickedOddColors();
  afx_msg void OnChangeColorEdit();

 protected:
  DECLARE_MESSAGE_MAP()
};
