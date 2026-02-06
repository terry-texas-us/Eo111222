#pragma once
#include "Resource.h"

class EoDlgSetupCustomMouseCharacters : public CDialog {
 public:
  EoDlgSetupCustomMouseCharacters(CWnd* pParent = nullptr);
  EoDlgSetupCustomMouseCharacters(const EoDlgSetupCustomMouseCharacters&) = delete;
  EoDlgSetupCustomMouseCharacters& operator=(const EoDlgSetupCustomMouseCharacters&) = delete;

  virtual ~EoDlgSetupCustomMouseCharacters();

  enum { IDD = IDD_MOUSEKEYS };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();
};
