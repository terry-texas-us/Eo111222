#pragma once
#include "Resource.h"

class EoDlgPipeSymbol : public CDialog {
 public:
  EoDlgPipeSymbol(CWnd* pParent = nullptr);
  EoDlgPipeSymbol(const EoDlgPipeSymbol&) = delete;
  EoDlgPipeSymbol& operator=(const EoDlgPipeSymbol&) = delete;

  virtual ~EoDlgPipeSymbol();

  enum { IDD = IDD_PIPE_SYMBOL };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

 public:
  CListBox m_PipeSymbolsListBoxControl;
  int m_CurrentPipeSymbolIndex;
};
