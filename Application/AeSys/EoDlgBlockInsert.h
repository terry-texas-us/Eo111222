#pragma once

#include "EoGePoint3d.h"
#include "Resource.h"

class EoDlgBlockInsert : public CDialog {
 public:
  EoDlgBlockInsert([[maybe_unused]] CWnd* parent = nullptr);
  EoDlgBlockInsert(AeSysDoc* document, [[maybe_unused]] CWnd* parent = nullptr);
  EoDlgBlockInsert(const EoDlgBlockInsert&) = delete;
  EoDlgBlockInsert& operator=(const EoDlgBlockInsert&) = delete;

  virtual ~EoDlgBlockInsert();

  enum { IDD = IDD_INSERT_BLOCK };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;

  /** * @brief Initializes the block insertion dialog by populating the block list and setting up the preview.
   *
   * This method retrieves the current cursor position to use as the insertion point for the block reference.
   * It then iterates through the blocks in the document, filtering out anonymous blocks, system blocks, and
   * model/paper space blocks, and adds the valid block names to the list box control. The first block is selected
   * by default. If there are no blocks in the document, the preview is cleared; otherwise, it displays a preview
   * of the first block. Finally, the dialog is centered on the main application window.
   *
   * @return TRUE to indicate that the dialog should set focus to the first control; FALSE if focus has been set
   * manually.
   */
  BOOL OnInitDialog() override;
  void OnOK() override;

  static EoGePoint3d InsertionPoint;
  AeSysDoc* m_document{};

 public:
  CListBox m_blocksListBoxControl;
  afx_msg void OnLbnSelchangeBlocksList();
  afx_msg void OnBnClickedPurge();

 protected:
  DECLARE_MESSAGE_MAP()
};
