#pragma once

#include "EoGeReferenceSystem.h"

/// @file EoDlgModeRevise.h
/// @brief Modal dialog for revising text primitives in the active view.
///
/// Provides a simple editor dialog that locates a text primitive under the current
/// cursor position, presents its string in a single-line `CEdit` control for user
/// editing, and commits changes back into the document model (or creates a new
/// text primitive if none is found).
///
class EoDlgModeRevise : public CDialog {
  DECLARE_DYNAMIC(EoDlgModeRevise)

 public:
  EoDlgModeRevise(CWnd* parent = nullptr);
  EoDlgModeRevise(const EoDlgModeRevise&) = delete;
  EoDlgModeRevise& operator=(const EoDlgModeRevise&) = delete;

  virtual ~EoDlgModeRevise();

  // Dialog Data
  enum { IDD = IDD_ADD_NOTE };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  static EoDbFontDefinition sm_FontDefinition;
  static EoGeReferenceSystem sm_ReferenceSystem;
  static EoDbText* sm_TextPrimitive;

 public:
  CEdit m_TextEditControl;

  afx_msg void OnSize(UINT nType, int cx, int cy);

 protected:
  DECLARE_MESSAGE_MAP()
};
