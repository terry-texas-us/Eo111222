#pragma once

#include "Resource.h"

/// @brief Modal dialog for managing text styles.
/// Displays a list of all text styles in the document with editable Height and Width Factor columns.
/// Double-click on a Height or Width Factor cell to edit the value in-place.
class EoDlgTextStyleManager : public CDialog {
  DECLARE_DYNAMIC(EoDlgTextStyleManager)

 public:
  explicit EoDlgTextStyleManager(CWnd* parent = nullptr);

  enum { IDD = IDD_TEXT_STYLE_MANAGER };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  BOOL PreTranslateMessage(MSG* msg) override;

  DECLARE_MESSAGE_MAP()
  afx_msg void OnDoubleClickStyleList(NMHDR* notifyMessageHeader, LRESULT* result);
  afx_msg void OnEndEditKillFocus();

 private:
  CListCtrl m_styleList;
  CEdit m_inPlaceEdit;
  int m_editingRow{-1};
  int m_editingColumn{-1};

  void PopulateStyleList();
  void CommitInPlaceEdit();
  void CancelInPlaceEdit();
  void BeginInPlaceEdit(int row, int column);
};
