#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "Resource.h"

namespace {
constexpr int statusOp0{3};
}

/// @brief Retrieves the DPI (dots per inch) for a window. The function tries to call GetDpiForWindow from user32.dll if available; if not available or the window handle is invalid, it falls back to the screen DPI obtained from the device context, defaulting to 96 if detection fails.
/// @param hwnd Handle to the window whose DPI should be retrieved. If nullptr or not a valid window, the function falls back to the primary screen DPI.
/// @return The DPI value (horizontal dots per inch) as a UINT for the specified window or screen. Returns 96 if the DPI cannot be determined.
static UINT GetWindowDpi(HWND hwnd) {
  UINT dpi = 96;
  HMODULE user32Module = ::GetModuleHandleW(L"user32.dll");
  if (user32Module != nullptr) {
    typedef UINT(WINAPI * GetDpiForWindow_t)(HWND);
    auto GetDpiForWindow =
        reinterpret_cast<GetDpiForWindow_t>(reinterpret_cast<void*>(::GetProcAddress(user32Module, "GetDpiForWindow")));
    if (GetDpiForWindow != nullptr && hwnd != nullptr && ::IsWindow(hwnd)) {
      dpi = GetDpiForWindow(hwnd);
    } else {
      HDC screenContext = ::GetDC(nullptr);
      if (screenContext != nullptr) {
        dpi = static_cast<UINT>(::GetDeviceCaps(screenContext, LOGPIXELSX));
        ::ReleaseDC(nullptr, screenContext);
      }
    }
  }
  return dpi;
}

static int ScaleHeightForDpi(HWND hwnd, int baseHeight) {
  if (baseHeight <= 0) { return 0; }
  UINT dpi = GetWindowDpi(hwnd);
  int scaled = MulDiv(baseHeight, static_cast<int>(dpi), 96);
  return (scaled > 0) ? scaled : 0;
}

/// @brief Scales a base width value according to the DPI of a given window.
/// @param hwnd Handle to the window whose DPI is used for scaling.
/// @param baseWidth The base width in pixels at the standard 96 DPI. If <= 0, the function returns 0.
/// @return The width scaled to the window's DPI (using 96 DPI as the baseline), or 0 if baseWidth <= 0 or the computed scaled value is not positive.
static int ScaleWidthForDpi(HWND hwnd, int baseWidth) {
  if (baseWidth <= 0) { return 0; }
  UINT dpi = GetWindowDpi(hwnd);
  int scaled = MulDiv(baseWidth, static_cast<int>(dpi), 96);
  return (scaled > 0) ? scaled : 0;
}

/// @brief Draws a line of text into a pane area of the given view using the provided device context and font.
/// @param context Pointer to the device context (CDC) used for drawing. If nullptr, the function returns immediately.
/// @param view Pointer to the view (CPegView) whose client rectangle is used to compute the pane area. If nullptr, the function returns immediately.
/// @param paneIndex Zero-based index of the pane in which to draw the text; used to compute the left and right boundaries for the pane.
/// @param paneText The text to draw (CString).
/// @param font Font (CFont) to select into the device context for drawing the text.
/// @param textColor COLORREF specifying the text color to use while drawing.
static void DrawPaneTextInView(CDC* context, AeSysView* view, int paneIndex, const CString& paneText, int font,
                               COLORREF textColor) {
  if (context == nullptr || view == nullptr) { return; }

  auto* oldFont = context->SelectObject(static_cast<CFont*>(context->SelectStockObject(font)));
  auto oldTextAlignment = context->SetTextAlign(TA_LEFT | TA_TOP);
  auto oldTextColor = context->SetTextColor(textColor);
  auto oldBackgroundColor = context->SetBkColor(~App::ViewTextColor() & 0x00ffffff);

  TEXTMETRIC metrics;
  context->GetTextMetrics(&metrics);

  CRect clientArea;
  view->GetClientRect(&clientArea);

  int paneWidth = clientArea.Width() / 10;

  int left = paneIndex * paneWidth;
  int top = clientArea.bottom - metrics.tmHeight;
  int right = (paneIndex + 1) * paneWidth;
  int bottom = clientArea.bottom;
  CRect rc(left, top, right, bottom);

  auto paneTextLength = static_cast<UINT>(paneText.GetLength());
  context->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, paneText, paneTextLength, nullptr);

  context->SetBkColor(oldBackgroundColor);
  context->SetTextColor(oldTextColor);
  context->SetTextAlign(oldTextAlignment);
  context->SelectObject(oldFont);
}

void AeSysView::ModeLineDisplay() {
  if (app.CurrentMode() == 0) { return; }
  m_OpHighlighted = 0;

  auto modeInformation = App::LoadStringResource(UINT(app.CurrentMode()));

  CString paneText;

  auto* context = GetDC();

  for (int i = 0; i < 10; i++) {
    AfxExtractSubString(paneText, modeInformation, i + 1, '\n');

    auto textExtent = context->GetTextExtent(paneText);

    GetStatusBar().SetPaneInfo(::statusOp0 + i, static_cast<UINT>(ID_OP0 + i), SBPS_NORMAL, textExtent.cx);
    GetStatusBar().SetPaneText(::statusOp0 + i, paneText);
    GetStatusBar().SetTipText(::statusOp0 + i, L"Mode Command Tip Text");

    if (app.ModeInformationOverView()) {
      DrawPaneTextInView(context, GetActiveView(), i, paneText, DEFAULT_GUI_FONT, App::ViewTextColor());
    }
  }
  ReleaseDC(context);
}

EoUInt16 AeSysView::ModeLineHighlightOp(EoUInt16 command) {
  ModeLineUnhighlightOp(m_OpHighlighted);

  m_OpHighlighted = command;

  if (command == 0) { return 0; }
  int paneIndex = ::statusOp0 + m_OpHighlighted - ID_OP0;

  GetStatusBar().SetPaneTextColor(paneIndex, Eo::colorRed);

  if (app.ModeInformationOverView()) {
    CString paneText = GetStatusBar().GetPaneText(paneIndex);
    auto* deviceContext = GetDC();
    DrawPaneTextInView(deviceContext, GetActiveView(), paneIndex - ::statusOp0, paneText, DEFAULT_GUI_FONT,
                       Eo::colorRed);
    ReleaseDC(deviceContext);
  }
  return command;
}

void AeSysView::ModeLineUnhighlightOp(EoUInt16& command) {
  if (command == 0 || m_OpHighlighted == 0) { return; }
  int paneIndex = ::statusOp0 + m_OpHighlighted - ID_OP0;

  GetStatusBar().SetPaneTextColor(paneIndex);

  if (app.ModeInformationOverView()) {
    CString paneText = GetStatusBar().GetPaneText(paneIndex);
    CDC* context = GetDC();
    DrawPaneTextInView(context, GetActiveView(), paneIndex - ::statusOp0, paneText, DEFAULT_GUI_FONT,
                       App::ViewTextColor());
    ReleaseDC(context);
  }
  command = 0;
  m_OpHighlighted = 0;
}
