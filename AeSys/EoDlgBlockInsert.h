#pragma once
#include <Windows.h>
#include <afx.h>

#include "EoGePoint3d.h"
#include "Resource.h"

class EoDlgBlockInsert : public CDialog {
  DECLARE_DYNAMIC(EoDlgBlockInsert)

 public:
  EoDlgBlockInsert(CWnd* parent = nullptr);
  EoDlgBlockInsert(AeSysDoc* document, CWnd* parent = nullptr);
  EoDlgBlockInsert(const EoDlgBlockInsert&) = delete;
  EoDlgBlockInsert& operator=(const EoDlgBlockInsert&) = delete;

  virtual ~EoDlgBlockInsert();

  enum { IDD = IDD_INSERT_BLOCK };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  static EoGePoint3d InsertionPoint;
  AeSysDoc* m_Document{nullptr};

 public:
  CListBox m_BlocksListBoxControl;
  afx_msg void OnLbnSelchangeBlocksList();
  afx_msg void OnBnClickedPurge();

 protected:
  DECLARE_MESSAGE_MAP()
};
