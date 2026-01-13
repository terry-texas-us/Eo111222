#pragma once

class EoDlgAnnotateOptions : public CDialog {
  DECLARE_DYNAMIC(EoDlgAnnotateOptions)

 public:
  EoDlgAnnotateOptions(CWnd* parent = nullptr);
  EoDlgAnnotateOptions(AeSysView* view, CWnd* parent = nullptr);
  EoDlgAnnotateOptions(const EoDlgAnnotateOptions&) = delete;
  EoDlgAnnotateOptions& operator=(const EoDlgAnnotateOptions&) = delete;

  virtual ~EoDlgAnnotateOptions();

  enum { IDD = IDD_ANNOTATE_OPTIONS };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  AeSysView* m_ActiveView{nullptr};

 public:
  CComboBox m_EndItemTypeComboBox;

  double m_GapSpaceFactor;
  double m_CircleRadius;
  double m_EndItemSize;
  double m_BubbleRadius;
  int m_NumberOfSides;
  CString m_DefaultText;
};
