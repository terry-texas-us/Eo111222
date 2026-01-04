#pragma once
#include <Windows.h>
#include <afx.h>
#include <afxcmn.h>
#include <afxwin.h>

#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "Resource.h"

class EoDlgSetupLineType : public CDialog {
  DECLARE_DYNAMIC(EoDlgSetupLineType)

 public:
  EoDbLineType* m_LineType{nullptr};
  EoDbLineTypeTable* m_LineTypeTable{nullptr};
  CListCtrl m_LineTypesListControl;

  EoDlgSetupLineType(CWnd* parent = nullptr);
  EoDlgSetupLineType(EoDbLineTypeTable* lineTypeTable, CWnd* parent = nullptr);

  EoDlgSetupLineType(const EoDlgSetupLineType&) = delete;
  EoDlgSetupLineType& operator=(const EoDlgSetupLineType&) = delete;

  ~EoDlgSetupLineType() override;

  enum { IDD = IDD_SETUP_LINETYPE };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  void OnOK() override;

  afx_msg void OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT drawItemStruct);

 private:
  DECLARE_MESSAGE_MAP()
};
