#pragma once

// EoDlgViewZoom dialog

class EoDlgViewZoom : public CDialog {
  DECLARE_DYNAMIC(EoDlgViewZoom)

 public:
  EoDlgViewZoom(CWnd* pParent = nullptr);
  EoDlgViewZoom(const EoDlgViewZoom&) = delete;
  EoDlgViewZoom& operator=(const EoDlgViewZoom&) = delete;

  virtual ~EoDlgViewZoom();

  // Dialog Data
  enum { IDD = IDD_VIEW_ZOOM };

  float m_Ratio;

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
};
