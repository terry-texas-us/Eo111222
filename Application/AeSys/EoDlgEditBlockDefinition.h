#pragma once

#include "Resource.h"

class AeSysDoc;

class EoDlgEditBlockDefinition : public CDialog {
 public:
  EoDlgEditBlockDefinition(AeSysDoc* document, CWnd* parent = nullptr);
  EoDlgEditBlockDefinition(const EoDlgEditBlockDefinition&) = delete;
  EoDlgEditBlockDefinition& operator=(const EoDlgEditBlockDefinition&) = delete;

  ~EoDlgEditBlockDefinition() override;

  enum { IDD = IDD_EDIT_BLOCK_DEFINITION };

  /// @brief The name of the block selected for editing. Empty if cancelled.
  CString m_selectedBlockName;

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  void OnOK() override;

  AeSysDoc* m_document{};
  CListBox m_blocksListBoxControl;

  afx_msg void OnLbnSelchangeBlocksList();

  DECLARE_MESSAGE_MAP()
};
