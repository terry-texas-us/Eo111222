#pragma once

// EoDlgPipeSymbol dialog

class EoDlgPipeSymbol : public CDialog {
  DECLARE_DYNAMIC(EoDlgPipeSymbol)

 public:
  EoDlgPipeSymbol(CWnd* pParent = nullptr);
  EoDlgPipeSymbol(const EoDlgPipeSymbol&) = delete;
  EoDlgPipeSymbol& operator=(const EoDlgPipeSymbol&) = delete;

  virtual ~EoDlgPipeSymbol();

  // Dialog Data
  enum { IDD = IDD_PIPE_SYMBOL };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

 public:
  CListBox m_PipeSymbolsListBoxControl;
  int m_CurrentPipeSymbolIndex;
};
