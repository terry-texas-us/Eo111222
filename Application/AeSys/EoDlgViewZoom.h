#pragma once
#include "Resource.h"

class EoDlgViewZoom : public CDialog {
 public:
  EoDlgViewZoom(CWnd* pParent = nullptr);
  EoDlgViewZoom(const EoDlgViewZoom&) = delete;
  EoDlgViewZoom& operator=(const EoDlgViewZoom&) = delete;

  virtual ~EoDlgViewZoom();

  enum { IDD = IDD_VIEW_ZOOM };

  double m_Ratio;

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
};
