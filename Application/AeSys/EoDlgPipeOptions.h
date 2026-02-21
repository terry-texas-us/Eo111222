#pragma once
#include "Resource.h"

/// @brief Dialog class for configuring pipe-related options.
class EoDlgPipeOptions : public CDialog {
 public:
  EoDlgPipeOptions(CWnd* parent = nullptr);
  EoDlgPipeOptions(const EoDlgPipeOptions&) = delete;
  EoDlgPipeOptions& operator=(const EoDlgPipeOptions&) = delete;

  virtual ~EoDlgPipeOptions();

  enum { IDD = IDD_PIPE_OPTIONS };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

 public:
  double m_PipeTicSize;
  double m_PipeRiseDropRadius;
};
