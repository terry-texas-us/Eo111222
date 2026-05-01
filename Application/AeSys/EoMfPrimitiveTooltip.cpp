#include "Stdafx.h"

#include <vector>

#include "Eo.h"
#include "EoMfPrimitiveTooltip.h"

BEGIN_MESSAGE_MAP(EoMfPrimitiveTooltip, CWnd)
  ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL EoMfPrimitiveTooltip::Create(CWnd* parent) {
  LPCTSTR className = AfxRegisterWndClass(CS_SAVEBITS | CS_DROPSHADOW, LoadCursor(nullptr, IDC_ARROW));
  return CWnd::CreateEx(WS_EX_TOPMOST | WS_EX_NOACTIVATE,
      className,
      nullptr,
      WS_POPUP | WS_BORDER,
      0, 0, 10, 10,
      parent->GetSafeHwnd(),
      nullptr);
}

void EoMfPrimitiveTooltip::Show(const CString& rawExtra, CPoint screenPos) {
  ParseRows(rawExtra);
  if (m_rows.empty()) { return; }
  LayoutAndShow(screenPos);
}

void EoMfPrimitiveTooltip::Hide() {
  if (m_visible) {
    ShowWindow(SW_HIDE);
    m_visible = false;
  }
}

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------

void EoMfPrimitiveTooltip::ParseRows(const CString& rawExtra) {
  m_rows.clear();
  bool firstRow = true;
  int pos = 0;
  while (pos < rawExtra.GetLength()) {
    const int tabPos = rawExtra.Find(L'\t', pos);
    const int pairEnd = (tabPos >= 0) ? tabPos : rawExtra.GetLength();
    CString pair = rawExtra.Mid(pos, pairEnd - pos);
    pair.TrimRight();
    if (!pair.IsEmpty()) {
      const int semi = pair.Find(L';');
      if (semi >= 0) {
        Row row;
        row.label = pair.Left(semi);
        row.value = pair.Mid(semi + 1);
        row.isTitle = firstRow;
        m_rows.push_back(row);
        firstRow = false;
      }
    }
    pos = (tabPos >= 0) ? tabPos + 1 : rawExtra.GetLength();
  }
}

// ---------------------------------------------------------------------------
// Font helpers
// ---------------------------------------------------------------------------

void EoMfPrimitiveTooltip::EnsureFonts([[maybe_unused]] CDC& dc) {
  if (m_fontsCreated) { return; }

  // Match the system menu / status-bar font, then derive bold from it.
  NONCLIENTMETRICSW ncm{};
  ncm.cbSize = sizeof(ncm);
  SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

  LOGFONTW lf = ncm.lfMenuFont;

  // Normal weight
  lf.lfWeight = FW_NORMAL;
  m_normalFont.CreateFontIndirectW(&lf);

  // Bold weight — same face / size
  lf.lfWeight = FW_SEMIBOLD;
  m_boldFont.CreateFontIndirectW(&lf);

  m_fontsCreated = true;
}

// ---------------------------------------------------------------------------
// Layout and show
// ---------------------------------------------------------------------------

void EoMfPrimitiveTooltip::LayoutAndShow(CPoint screenPos) {
  CWindowDC screenDC(nullptr);
  EnsureFonts(screenDC);

  // Measure all rows to determine window size.
  CFont* prevFont = screenDC.SelectObject(&m_boldFont);

  const int dpi = GetDpiForSystem();
  const int padX = MulDiv(kPadX, dpi, 96);
  const int padY = MulDiv(kPadY, dpi, 96);
  const int rowGap = MulDiv(kRowGap, dpi, 96);
  const int titleGap = MulDiv(kTitleGap, dpi, 96);
  const int colGap = MulDiv(12, dpi, 96);  // gap between label and value columns

  int maxLabelW = 0;
  int maxValueW = 0;
  int rowHeight = 0;

  for (auto& row : m_rows) {
    screenDC.SelectObject(&m_boldFont);
    const CSize labelSz = screenDC.GetTextExtent(row.label);
    screenDC.SelectObject(&m_normalFont);
    const CSize valueSz = screenDC.GetTextExtent(row.value);
    maxLabelW = std::max(maxLabelW, static_cast<int>(labelSz.cx));
    maxValueW = std::max(maxValueW, static_cast<int>(valueSz.cx));
    rowHeight = std::max(rowHeight, static_cast<int>(std::max(labelSz.cy, valueSz.cy)));
  }
  screenDC.SelectObject(prevFont);

  const int rowCount = static_cast<int>(m_rows.size());
  const int totalHeight = padY * 2
      + rowCount * rowHeight
      + (rowCount - 1) * rowGap
      + titleGap;  // extra space below the title row
  const int totalWidth = padX * 2 + maxLabelW + colGap + maxValueW;

  // Keep the popup within the nearest monitor.
  const HMONITOR monitor = MonitorFromPoint(screenPos, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi{};
  mi.cbSize = sizeof(mi);
  GetMonitorInfoW(monitor, &mi);
  const RECT& workArea = mi.rcWork;

  int x = screenPos.x + MulDiv(16, dpi, 96);
  int y = screenPos.y + MulDiv(16, dpi, 96);
  if (x + totalWidth > workArea.right) { x = screenPos.x - totalWidth - MulDiv(4, dpi, 96); }
  if (y + totalHeight > workArea.bottom) { y = screenPos.y - totalHeight - MulDiv(4, dpi, 96); }

  SetWindowPos(&wndTopMost, x, y, totalWidth, totalHeight, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  m_visible = true;
  Invalidate(FALSE);
  UpdateWindow();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void EoMfPrimitiveTooltip::OnPaint() {
  CPaintDC dc(this);
  EnsureFonts(dc);

  CRect clientRect;
  GetClientRect(&clientRect);

  const int dpi = GetDpiForSystem();
  const int padX = MulDiv(kPadX, dpi, 96);
  const int padY = MulDiv(kPadY, dpi, 96);
  const int rowGap = MulDiv(kRowGap, dpi, 96);
  const int titleGap = MulDiv(kTitleGap, dpi, 96);
  const int colGap = MulDiv(12, dpi, 96);

  // Background fill — use pane background from chrome palette.
  dc.FillSolidRect(clientRect, Eo::chromeColors.paneBackground);

  // Measure label column width.
  CFont* prevFont = dc.SelectObject(&m_boldFont);
  int labelColW = 0;
  for (auto& row : m_rows) {
    dc.SelectObject(&m_boldFont);
    labelColW = std::max(labelColW, static_cast<int>(dc.GetTextExtent(row.label).cx));
  }
  dc.SelectObject(prevFont);

  dc.SetBkMode(TRANSPARENT);

  int y = padY;
  for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
    const auto& row = m_rows[static_cast<size_t>(i)];

    // Measure row height.
    dc.SelectObject(&m_boldFont);
    const int labelH = static_cast<int>(dc.GetTextExtent(row.label).cy);
    dc.SelectObject(&m_normalFont);
    const int valueH = static_cast<int>(dc.GetTextExtent(row.value).cy);
    const int rowH = std::max(labelH, valueH);

    if (row.isTitle) {
      // Title row: accent blue background strip, white text both columns.
      CRect titleRect(0, y - MulDiv(3, dpi, 96), clientRect.right, y + rowH + MulDiv(3, dpi, 96));
      dc.FillSolidRect(titleRect, Eo::chromeColors.captionActiveBackground);

      dc.SetTextColor(Eo::chromeColors.captionActiveText);
      dc.SelectObject(&m_boldFont);
      dc.TextOutW(padX, y, row.label);
      dc.SelectObject(&m_normalFont);
      dc.TextOutW(padX + labelColW + colGap, y, row.value);

      y += rowH + rowGap + titleGap;
    } else {
      // Normal row: bold dark label, normal value.
      dc.SetTextColor(Eo::chromeColors.paneGroupText);
      dc.SelectObject(&m_boldFont);
      dc.TextOutW(padX, y, row.label);

      dc.SetTextColor(Eo::chromeColors.paneText);
      dc.SelectObject(&m_normalFont);
      dc.TextOutW(padX + labelColW + colGap, y, row.value);

      y += rowH + rowGap;
    }
  }

  // Thin border line using the pane border color.
  CBrush borderBrush(Eo::chromeColors.borderColor);
  dc.FrameRect(clientRect, &borderBrush);
}
