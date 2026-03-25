#pragma once

#include "EoDbLineTypeTable.h"

// Consider std::vector<std::shared_ptr<EoDbLineType>> for LineTypeTable to modernize

class EoDbLineType;

class EoDlgLineTypesSelection : public CDialogEx {
 public:
  EoDbLineTypeTable m_lineTypes;
  CListCtrl m_lineTypesListControl;
  CListCtrl m_fileLineTypesListControl;

  EoDbLineType* GetSelectedLineType() const { return m_selectedLineType; }
  void SetSelectedLineType(EoDbLineType* lineType) { m_selectedLineType = lineType; }
  [[nodiscard]] bool IsSelectedFromFileList() const noexcept { return m_selectedFromFileList; }

  /// @brief Returns the file-loaded line type table for the caller to merge selected entries.
  EoDbLineTypeTable& FileLineTypes() noexcept { return m_fileLineTypes; }

  EoDlgLineTypesSelection(CWnd* parent = nullptr);
  EoDlgLineTypesSelection(EoDbLineTypeTable& lineTypes, CWnd* pParent = nullptr);

  virtual ~EoDlgLineTypesSelection();

  EoDlgLineTypesSelection(const EoDlgLineTypesSelection&) = delete;
  EoDlgLineTypesSelection& operator=(const EoDlgLineTypesSelection&) = delete;

#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_LINE_TYPES_DIALOG };
#endif

 protected:
  virtual void DoDataExchange(CDataExchange* pDX) override;
  virtual BOOL OnInitDialog() override;
  void OnOK() override;
  BOOL PreTranslateMessage(MSG* message) override;

  afx_msg void OnBnClickedLoadFile();

  DECLARE_MESSAGE_MAP()

 private:
  EoDbLineType* m_selectedLineType{};
  bool m_selectedFromFileList{};
  EoDbLineTypeTable m_fileLineTypes;

  void PopulateList();
  void PopulateFileList();
  afx_msg void OnNMCustomDrawList(NMHDR* pNMHDR, LRESULT* result);
  afx_msg void OnNMCustomDrawFileList(NMHDR* pNMHDR, LRESULT* result);

  /// @brief Shared implementation for NM_CUSTOMDRAW on both list controls.
  void DrawLineTypePreview(CListCtrl& listControl, NMLVCUSTOMDRAW* listViewCustomDraw, LRESULT* result);
};
