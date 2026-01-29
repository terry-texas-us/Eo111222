#pragma once

#include <cstdarg>

#include "Resource.h"

class EoDlgViewParameters : public CDialog {
  DECLARE_DYNAMIC(EoDlgViewParameters)

 public:
  EoDlgViewParameters(CWnd* parent = nullptr);
  EoDlgViewParameters(const EoDlgViewParameters&) = delete;
  EoDlgViewParameters& operator=(const EoDlgViewParameters&) = delete;

  virtual ~EoDlgViewParameters();

  // Dialog Data
  enum { IDD = IDD_VIEW_PARAMETERS };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  void OnOK() override;

 public:
  BOOL m_PerspectiveProjection{};
  uintptr_t m_ModelView;
  /** @brief Handler for the Apply button click event.
   *
   * This function retrieves the current view parameters from the dialog controls,
   * updates the active view's model view with the new parameters, and refreshes the view.
   */
  afx_msg void OnBnClickedApply();
  afx_msg void OnEnChangePositionX();
  afx_msg void OnEnChangePositionY();
  afx_msg void OnEnChangePositionZ();
  afx_msg void OnEnChangeTargetX();
  afx_msg void OnEnChangeTargetY();
  afx_msg void OnEnChangeTargetZ();
  afx_msg void OnEnChangeFrontClipDistance();
  afx_msg void OnEnChangeBackClipDistance();
  afx_msg void OnEnChangeLensLength();
  afx_msg void OnBnClickedPerspectiveProjection();

 protected:
  DECLARE_MESSAGE_MAP()
};
