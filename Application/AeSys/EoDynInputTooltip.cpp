#include "stdafx.h"
#include "EoDynInputTooltip.h"
#include "Eo.h"

// clang-format off
BEGIN_MESSAGE_MAP(EoDynInputTooltip, CWnd)
  ON_WM_PAINT()
END_MESSAGE_MAP()
// clang-format on

BOOL EoDynInputTooltip::Create(CWnd* ownerView) {
  LPCTSTR className = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
  const DWORD exStyle = WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
  if (!CWnd::CreateEx(exStyle, className, L"", WS_POPUP, CRect(0, 0, 10, 10), ownerView, 0)) {
    return FALSE;
  }
  SetLayeredWindowAttributes(0, 210, LWA_ALPHA);  // ~82% opacity
  EnsureFont();
  return TRUE;
}

void EoDynInputTooltip::EnsureFont() {
  if (m_font.GetSafeHandle() != nullptr) { return; }
  // Base on system message-box font, increase point size by 2 for readability.
  NONCLIENTMETRICS ncm{};
  ncm.cbSize = sizeof(ncm);
  ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
  LOGFONT lf = ncm.lfMessageFont;
  // lfHeight is negative device units; make it 2pt larger (subtract ~3px at 96dpi).
  if (lf.lfHeight < 0) { lf.lfHeight -= 3; } else { lf.lfHeight += 3; }
  m_font.CreateFontIndirect(&lf);
}

void EoDynInputTooltip::Show(CPoint cursorScreen,
    const EoGePoint3d& worldPos,
    const EoGePoint3d& anchorWorld,
    const wchar_t* prompt) {
  m_promptRow = (prompt != nullptr && prompt[0] != L'\0') ? CString(prompt) : CString(L"Specify point");

  const bool hasAnchor = !(anchorWorld == EoGePoint3d{});
  if (hasAnchor) {
    const double dx = worldPos.x - anchorWorld.x;
    const double dy = worldPos.y - anchorWorld.y;
    const double dist = std::sqrt(dx * dx + dy * dy);
    double angleDeg = std::atan2(dy, dx) * (180.0 / Eo::Pi);
    if (angleDeg < 0.0) { angleDeg += 360.0; }
    m_valueRow.Format(L"%.4f  >  %.3f\xb0", dist, angleDeg);
  } else {
    m_valueRow.Format(L"X  %.4f      Y  %.4f", worldPos.x, worldPos.y);
  }
  PositionAndShow(cursorScreen);
}

void EoDynInputTooltip::PositionAndShow(CPoint cursorScreen) {
  EnsureFont();
  CDC* dc = GetDC();
  if (dc == nullptr) { return; }
  auto* prevFont = dc->SelectObject(&m_font);
  const CSize szPrompt = dc->GetTextExtent(m_promptRow);
  const CSize szValue  = dc->GetTextExtent(m_valueRow);
  dc->SelectObject(prevFont);
  ReleaseDC(dc);

  const int lineH = szPrompt.cy + 2 * kPadY;
  const int w = 2 * kPadX + std::max<int>(szPrompt.cx, szValue.cx);
  const int h = 2 * lineH + 2;

  int x = cursorScreen.x + kOffsetX;
  int y = cursorScreen.y + kOffsetY;
  if (x + w > ::GetSystemMetrics(SM_CXSCREEN)) { x = cursorScreen.x - w - kOffsetX; }
  if (y + h > ::GetSystemMetrics(SM_CYSCREEN)) { y = cursorScreen.y - h - kOffsetY; }

  SetWindowPos(&wndTopMost, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  m_visible = true;
  Invalidate(FALSE);
}

void EoDynInputTooltip::Hide() {
  if (m_visible) {
    ShowWindow(SW_HIDE);
    m_visible = false;
  }
}

void EoDynInputTooltip::OnPaint() {
  CPaintDC dc(this);
  CRect clientRect;
  GetClientRect(clientRect);

  const COLORREF bgColor     = RGB(30, 30, 34);
  const COLORREF promptColor = RGB(200, 210, 240);  // slightly blue-tinted
  const COLORREF valueColor  = RGB(230, 230, 220);
  const COLORREF borderColor = RGB(60, 100, 160);

  dc.FillSolidRect(clientRect, bgColor);
  dc.Draw3dRect(clientRect, borderColor, borderColor);

  auto* prevFont = dc.SelectObject(&m_font);
  dc.SetBkMode(TRANSPARENT);

  const int lineH = dc.GetTextExtent(L"Wg", 2).cy + 2 * kPadY;
  int y = 1 + kPadY;

  dc.SetTextColor(promptColor);
  dc.TextOut(kPadX, y, m_promptRow);
  y += lineH;

  dc.SetTextColor(valueColor);
  dc.TextOut(kPadX, y, m_valueRow);

  dc.SelectObject(prevFont);
}
