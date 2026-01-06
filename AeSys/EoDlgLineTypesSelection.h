#pragma once
#include <Windows.h>
#include <afx.h>
#include <afxcmn.h>
#include <afxwin.h>

#include "afxdialogex.h"

#include "EoDbLineTypeTable.h"

class EoDbLineType;

class EoDlgLineTypesSelection : public CDialogEx {
  DECLARE_DYNAMIC(EoDlgLineTypesSelection)

 public:
  EoDbLineTypeTable m_lineTypes;  // Consider std::vector<std::shared_ptr<EoDbLineType>> for modernization
  CListCtrl m_lineTypesListControl;

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

  DECLARE_MESSAGE_MAP()

 private:
  void PopulateList();
  afx_msg void OnNMCustomDrawList(NMHDR* pNMHDR, LRESULT* result);
};
