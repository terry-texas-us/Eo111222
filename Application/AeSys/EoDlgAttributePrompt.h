#pragma once

#include "Resource.h"

/// @brief Modal dialog that prompts the user for a single attribute value during interactive block insertion.
class EoDlgAttributePrompt : public CDialog {
  DECLARE_DYNAMIC(EoDlgAttributePrompt)

 public:
  explicit EoDlgAttributePrompt(CWnd* parent = nullptr);

  enum { IDD = IDD_ATTRIBUTE_PROMPT };

  // Input fields — set by caller before DoModal()
  CString m_blockName;
  CString m_tagName;
  CString m_promptString;
  CString m_defaultValue;

  // Output field — read by caller after DoModal() returns IDOK
  CString m_enteredValue;

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  void OnOK() override;

  DECLARE_MESSAGE_MAP()
};
