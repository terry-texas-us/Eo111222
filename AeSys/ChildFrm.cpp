#include "stdafx.h"
#include "ChildFrm.h"
#include "AeSys.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
ON_WM_TIMER()
END_MESSAGE_MAP()

// Construction/destruction

CChildFrame::CChildFrame() {}
CChildFrame::~CChildFrame() {}

// Overrides

void CChildFrame::ActivateFrame(int nCmdShow) {
  ATLTRACE2(atlTraceGeneral, 1, L"CChildFrame::ActivateFrame(%i)\n", nCmdShow);
  CMDIChildWndEx::ActivateFrame(nCmdShow);
  ATLTRACE2(atlTraceGeneral, 1, L"CChildFrame::ActivateFrame() - Leaving\n");
}
BOOL CChildFrame::DestroyWindow() {
  ATLTRACE2(atlTraceGeneral, 1, L"CChildFrame::DestroyWindow() - Entering\n");

  CDC* DeviceContext = GetDC();

  // Stock objects are never left "current" so it is safe to delete whatever the old object is

  DeviceContext->SelectStockObject(BLACK_PEN)->DeleteObject();
  DeviceContext->SelectStockObject(WHITE_BRUSH)->DeleteObject();

  return CMDIChildWndEx::DestroyWindow();
}
BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs) {
  if (!CMDIChildWndEx::PreCreateWindow(cs)) return FALSE;

  if (app.m_Options.m_nTabsStyle != EoApOptions::None) { cs.style &= ~WS_SYSMENU; }
  return TRUE;
}

#ifdef _DEBUG  // Diagnostic overrides
void CChildFrame::AssertValid() const { CMDIChildWndEx::AssertValid(); }
void CChildFrame::Dump(CDumpContext& dc) const { CMDIChildWndEx::Dump(dc); }
#endif  //_DEBUG
void CChildFrame::OnTimer(UINT_PTR nIDEvent) {
  ATLTRACE2(atlTraceGeneral, 1, L"CChildFrame::OnTimer() - Entering\n");

  CMDIChildWndEx::OnTimer(nIDEvent);
}
