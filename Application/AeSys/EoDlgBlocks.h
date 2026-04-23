#pragma once

#include "Resource.h"

/// @brief Unified block management dialog — browse, rename, inspect, edit base point, launch block editor.
///
/// Invoked from both File > Manage > Blocks and Tools > Edit Block Definition.
/// The "Edit Block..." button enters the block editor for the selected block.
class EoDlgBlocks : public CDialog {
 public:
  explicit EoDlgBlocks(AeSysDoc* document, CWnd* parent = nullptr);
  EoDlgBlocks(const EoDlgBlocks&) = delete;
  EoDlgBlocks& operator=(const EoDlgBlocks&) = delete;

  ~EoDlgBlocks() override;

  enum { IDD = IDD_BLOCKS };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;

  /// @brief Populates info labels and base point fields for the given block.
  void UpdateSelectionInfo(int listIndex);

  // --- List box ---
  afx_msg void OnLbnSelchangeBlocksList();

  // --- "Edit Block..." button ---
  afx_msg void OnBnClickedEditBlock();
  afx_msg void OnUpdateEditBlock(CCmdUI* cmdUI);

  // --- "Purge Unused" button ---
  afx_msg void OnBnClickedPurge();

  // --- Base point edits ---
  afx_msg void OnEnKillfocusBasePoint();

  // --- In-place rename ---
  void BeginInPlaceRename();
  void CommitRename();
  void CancelRename();

  afx_msg void OnContextMenu(CWnd* window, CPoint point);
  afx_msg void OnEnKillfocusInPlaceEdit();

  BOOL PreTranslateMessage(MSG* message) override;

 private:
  AeSysDoc* m_document{};

  CListBox m_blocksList;
  HWND m_previewWindowHandle{};

  // In-place rename state
  CEdit m_inPlaceEdit;
  int m_renamingIndex{-1};

  DECLARE_MESSAGE_MAP()
};
