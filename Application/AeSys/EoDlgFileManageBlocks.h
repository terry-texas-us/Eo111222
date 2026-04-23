#pragma once

#include "Resource.h"

class EoDlgFileManageBlocks : public CDialog {
 public:
  EoDlgFileManageBlocks(CWnd* parent = nullptr);
  EoDlgFileManageBlocks(AeSysDoc* document, CWnd* parent = nullptr);
  EoDlgFileManageBlocks(const EoDlgFileManageBlocks&) = delete;
  EoDlgFileManageBlocks& operator=(const EoDlgFileManageBlocks&) = delete;

  virtual ~EoDlgFileManageBlocks();

  enum { IDD = IDD_FILE_MANAGE_BLOCKS };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;

 public:
  AeSysDoc* m_document{};

  CListBox m_blocksList;
  CStatic m_references;

  HWND m_previewWindowHandle{};

  /** @brief Handler for the block list selection change event.
   * 
   * This method is called when the user changes the selection in the block list control. It retrieves the currently
   * selected block and updates the reference count display and the block preview accordingly.
   */
  afx_msg void OnLbnSelchangeBlocksList();
  afx_msg void OnContextMenu(CWnd* window, CPoint point);

 protected:
  /// @brief Begins F2-style in-place rename for the currently selected list item.
  void BeginInPlaceRename();

  /// @brief Commits the in-place rename: validates, calls RenameBlock, refreshes the list.
  void CommitRename();

  /// @brief Cancels an active in-place rename without making changes.
  void CancelRename();

  CEdit m_inPlaceEdit;
  int m_renamingIndex{-1};

  afx_msg void OnEnKillfocusInPlaceEdit();
  afx_msg void OnEnReturnInPlaceEdit();

  BOOL PreTranslateMessage(MSG* message) override;

  DECLARE_MESSAGE_MAP()
};
