#pragma once
#include "Resource.h"

class EoDlgFixupOptions : public CDialog {
  DECLARE_DYNAMIC(EoDlgFixupOptions)

 public:
  EoDlgFixupOptions(CWnd* parent = nullptr);
  EoDlgFixupOptions(const EoDlgFixupOptions& other) = delete;
  EoDlgFixupOptions& operator=(const EoDlgFixupOptions& other) = delete;

  virtual ~EoDlgFixupOptions();

  // Dialog Data
  enum { IDD = IDD_FIXUP_OPTIONS };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);

 public:
  double m_FixupAxisTolerance{0.0};
  double m_FixupModeCornerSize{0.25};
};
