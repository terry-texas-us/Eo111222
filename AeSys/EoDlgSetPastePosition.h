#pragma once
#include "Resource.h"

class EoDlgSetPastePosition : public CDialog {
  DECLARE_DYNAMIC(EoDlgSetPastePosition)

 public:
  EoDlgSetPastePosition(CWnd* pParent = nullptr);
  EoDlgSetPastePosition(const EoDlgSetPastePosition&) = delete;
  EoDlgSetPastePosition& operator=(const EoDlgSetPastePosition&) = delete;
    
  virtual ~EoDlgSetPastePosition();

  // Dialog Data
  enum { IDD = IDD_PASTE_POSITION };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual void OnOK();
};
