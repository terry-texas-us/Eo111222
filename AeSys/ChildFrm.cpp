#include "Stdafx.h"

#include "AeSys.h"
#include "ChildFrm.h"
#include "EoApOptions.h"

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_TIMER()
#pragma warning(pop)
END_MESSAGE_MAP()

CChildFrame::CChildFrame() {}
CChildFrame::~CChildFrame() {}

void CChildFrame::ActivateFrame(int nCmdShow) {
  ATLTRACE2(traceGeneral, 3, L"CChildFrame::ActivateFrame(%i)\n", nCmdShow);
  CMDIChildWndEx::ActivateFrame(nCmdShow);
  ATLTRACE2(traceGeneral, 3, L"CChildFrame::ActivateFrame() - Leaving\n");
}
BOOL CChildFrame::DestroyWindow() {
  ATLTRACE2(traceGeneral, 3, L"CChildFrame::DestroyWindow() - Entering\n");

  auto* deviceContext = GetDC();

  // Stock objects are never left "current" so it is safe to delete whatever the old object is

  deviceContext->SelectStockObject(BLACK_PEN)->DeleteObject();
  deviceContext->SelectStockObject(WHITE_BRUSH)->DeleteObject();

  return CMDIChildWndEx::DestroyWindow();
}
BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs) {
  if (!CMDIChildWndEx::PreCreateWindow(cs)) return FALSE;

  if (app.m_Options.m_tabsStyle != EoApOptions::None) { cs.style &= ~WS_SYSMENU; }
  return TRUE;
}

void CChildFrame::OnTimer(UINT_PTR nIDEvent) {
  ATLTRACE2(traceGeneral, 1, L"CChildFrame::OnTimer() - Entering\n");

  CMDIChildWndEx::OnTimer(nIDEvent);
}
