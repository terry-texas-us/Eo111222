#pragma once

#include "Resource.h"

class EoDlgSetLength : public CDialog {
 public:
  EoDlgSetLength(CWnd* pParent = nullptr);

  EoDlgSetLength(const EoDlgSetLength&) = delete;

  EoDlgSetLength& operator=(const EoDlgSetLength&) = delete;

  virtual ~EoDlgSetLength();

  enum { IDD = IDD_SET_LENGTH };

  [[nodiscard]] double Length() const noexcept { return m_length; }
  void SetLength(double length) { m_length = length; }

  [[nodiscard]] const CString& Title() const noexcept { return m_title; }
  void SetTitle(const CString& title) { m_title = title; }

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

 private:
  double m_length{};
  CString m_title{};
};
