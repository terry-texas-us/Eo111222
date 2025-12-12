#pragma once

class CChildFrame : public CMDIChildWndEx {
  DECLARE_DYNCREATE(CChildFrame)
 public:
  CChildFrame();
  CChildFrame(const CChildFrame&) = delete;
  CChildFrame& operator=(const CChildFrame&) = delete;

 public:
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

 public:
  virtual ~CChildFrame();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

 protected:
  DECLARE_MESSAGE_MAP()
 public:
  virtual void ActivateFrame(int nCmdShow = -1);
  virtual BOOL DestroyWindow();
  afx_msg void OnTimer(UINT_PTR nIDEvent);
};
