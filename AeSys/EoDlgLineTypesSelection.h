#pragma once

#include "EoDbLineTypeTable.h"

// Consider std::vector<std::shared_ptr<EoDbLineType>> for LineTypeTable to modernize

class EoDbLineType;

class EoDlgLineTypesSelection : public CDialogEx {
  DECLARE_DYNAMIC(EoDlgLineTypesSelection)

 public:
  EoDbLineTypeTable m_lineTypes;
  CListCtrl m_lineTypesListControl;

  // Getter and setter for m_selectedLineType
  EoDbLineType* GetSelectedLineType() const { return m_selectedLineType; }
  void SetSelectedLineType(EoDbLineType* lineType) { m_selectedLineType = lineType; }

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

  afx_msg void OnSize(UINT type, int x, int y);

  DECLARE_MESSAGE_MAP()

 private:
  EoDbLineType* m_selectedLineType{};

   void PopulateList();
  afx_msg void OnNMCustomDrawList(NMHDR* pNMHDR, LRESULT* result);

  public:
   afx_msg void OnBnClickedOk();
};
