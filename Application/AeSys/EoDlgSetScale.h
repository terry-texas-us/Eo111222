#pragma once

#include "Resource.h"

class EoDlgSetScale : public CDialog {
 public:
  EoDlgSetScale(CWnd* parent = nullptr);
  EoDlgSetScale(const EoDlgSetScale&) = delete;
  EoDlgSetScale& operator=(const EoDlgSetScale&) = delete;

  virtual ~EoDlgSetScale();

  enum { IDD = IDD_SET_SCALE };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);

 public:
  double m_Scale;
};
