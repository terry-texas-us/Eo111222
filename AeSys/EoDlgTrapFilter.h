#pragma once

// EoDlgTrapFilter dialog

class EoDlgTrapFilter : public CDialog {
  DECLARE_DYNAMIC(EoDlgTrapFilter)

 public:
  EoDlgTrapFilter(CWnd* pParent = nullptr);
  EoDlgTrapFilter(AeSysDoc* document, CWnd* pParent = nullptr);
  EoDlgTrapFilter(const EoDlgTrapFilter&) = delete;
  EoDlgTrapFilter& operator=(const EoDlgTrapFilter&) = delete;

  virtual ~EoDlgTrapFilter();

  // Dialog Data
  enum { IDD = IDD_TRAP_FILTER };

  AeSysDoc* m_Document{nullptr};

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

 public:
  CComboBox m_FilterLineComboBoxControl;
  CListBox m_FilterPrimitiveTypeListBoxControl;

  void FilterByColor(EoInt16 colorIndex);
  void FilterByLineType(int lineType);
  void FilterByPrimitiveType(const EoDb::PrimitiveTypes primitiveType);
};
