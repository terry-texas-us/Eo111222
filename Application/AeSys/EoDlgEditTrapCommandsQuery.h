#pragma once
#include "Resource.h"

class EoDlgEditTrapCommandsQuery : public CDialog {
 public:
  EoDlgEditTrapCommandsQuery(CWnd* parent = nullptr);
  EoDlgEditTrapCommandsQuery(const EoDlgEditTrapCommandsQuery&) = delete;
  EoDlgEditTrapCommandsQuery& operator=(const EoDlgEditTrapCommandsQuery&) = delete;

  virtual ~EoDlgEditTrapCommandsQuery();

  enum { IDD = IDD_EDIT_TRAPCOMMANDS_QUERY };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;

  CTreeCtrl m_GroupTreeViewControl;
  CListCtrl m_GeometryListViewControl;
  CListCtrl m_ExtraListViewControl;

 public:
  void FillExtraList(EoDbPrimitive* primitive);
  void FillGeometryList(EoDbPrimitive* primitive);

  afx_msg void OnTvnSelchangedGroupTree(NMHDR* pNMHDR, LRESULT* pResult);

 protected:
  DECLARE_MESSAGE_MAP()
};
