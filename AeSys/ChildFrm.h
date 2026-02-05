#pragma once

class CChildFrame : public CMDIChildWndEx {
  DECLARE_DYNCREATE(CChildFrame)
 public:
  CChildFrame();
  CChildFrame(const CChildFrame&) = delete;
  CChildFrame& operator=(const CChildFrame&) = delete;

 public:
  BOOL PreCreateWindow(CREATESTRUCT& cs) override;

 public:
  ~CChildFrame() override;

 protected:
  DECLARE_MESSAGE_MAP()
 public:
  void ActivateFrame(int nCmdShow = -1) override;
  BOOL DestroyWindow() override;
  afx_msg void OnTimer(UINT_PTR nIDEvent);
};
