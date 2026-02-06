#pragma once
#include "Resource.h"

class EoDlgModeLetter : public CDialog {
 public:
  EoDlgModeLetter(CWnd* pParent = nullptr);
  EoDlgModeLetter(const EoDlgModeLetter&) = delete;
  EoDlgModeLetter& operator=(const EoDlgModeLetter&) = delete;

  virtual ~EoDlgModeLetter();

  enum { IDD = IDD_ADD_NOTE };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  static EoGePoint3d m_Point;

 public:
  CEdit m_TextEditControl;
  /// <summary> Effectively resizes the edit control to use the entire client area of the dialog.</summary>
  /// <remarks> OnSize can be called before OnInitialUpdate so check is made for valid control window.</remarks>
  afx_msg void OnSize(UINT nType, int cx, int cy);

 protected:
  DECLARE_MESSAGE_MAP()
};
