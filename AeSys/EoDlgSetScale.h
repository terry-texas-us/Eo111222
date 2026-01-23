#pragma once

#include "Resource.h"

class EoDlgSetScale : public CDialog {
  DECLARE_DYNAMIC(EoDlgSetScale)

 public:
  EoDlgSetScale(CWnd* parent = nullptr);
  EoDlgSetScale(const EoDlgSetScale&) = delete;
  EoDlgSetScale& operator=(const EoDlgSetScale&) = delete;

  virtual ~EoDlgSetScale();

  // Dialog Data
  enum { IDD = IDD_SET_SCALE };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);

 public:
  double m_Scale;
};
