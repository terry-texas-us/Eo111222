#pragma once
#include "Resource.h"

class EoDlgEditOptions : public CDialog {

 public:
  EoDlgEditOptions(CWnd* parent = nullptr);
  EoDlgEditOptions(AeSysView* view, CWnd* parent = nullptr);
  EoDlgEditOptions(const EoDlgEditOptions&) = delete;
  EoDlgEditOptions& operator=(const EoDlgEditOptions&) = delete;

  virtual ~EoDlgEditOptions();

  enum { IDD = IDD_EDIT_OPTIONS };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  AeSysView* m_ActiveView{nullptr};

 public:
  CEdit m_RotationXEditControl;
  CEdit m_RotationYEditControl;
  CEdit m_RotationZEditControl;
  CEdit m_SizingXEditControl;
  CEdit m_SizingYEditControl;
  CEdit m_SizingZEditControl;
  CButton m_MirrorXCheckControl;
  CButton m_MirrorYCheckControl;
  CButton m_MirrorZCheckControl;

  afx_msg void OnEditOpRotation();
  afx_msg void OnEditOpMirroring();
  afx_msg void OnEditOpSizing();
  afx_msg void OnBnClickedEditOpMirX();
  afx_msg void OnBnClickedEditOpMirY();
  afx_msg void OnBnClickedEditOpMirZ();

 protected:
  DECLARE_MESSAGE_MAP()
 public:
  double m_EditModeScaleX{1.0};
  double m_EditModeScaleY{1.0};
  double m_EditModeScaleZ{1.0};
  double m_EditModeRotationAngleX{0.0};
  double m_EditModeRotationAngleY{0.0};
  double m_EditModeRotationAngleZ{0.0};
};
