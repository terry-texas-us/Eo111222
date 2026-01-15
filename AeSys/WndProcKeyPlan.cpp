#include "Stdafx.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDlgActiveViewKeyplan.h"

CPoint pnt;

namespace {
bool leftButtonDownInKeyplanRectangle{false};

void WndProcKeyPlanOnDraw(HWND hwnd) {
  PAINTSTRUCT ps;

  CDC dc;
  dc.Attach(BeginPaint(hwnd, &ps));

  CDC dcMem;
  dcMem.CreateCompatibleDC(nullptr);

  dcMem.SelectObject(CBitmap::FromHandle(EoDlgActiveViewKeyplan::m_hbmKeyplan));
  BITMAP bitmap;
  ::GetObject(EoDlgActiveViewKeyplan::m_hbmKeyplan, sizeof(BITMAP), (LPTSTR)&bitmap);

  dc.BitBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, &dcMem, 0, 0, SRCCOPY);

  CBrush* pBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
  dc.Rectangle(0, 0, bitmap.bmWidth, bitmap.bmHeight);

  //Note: Need to use the CWnd associated with Keyplan and not the active app view

  auto* activeView = AeSysView::GetActiveView();

  EoGePoint3d Target = activeView->CameraTarget();

  double UMin = Target.x + activeView->UMin();
  double UMax = Target.x + activeView->UMax();
  double VMin = Target.y + activeView->VMin();
  double VMax = Target.y + activeView->VMax();

  double UMinOverview = Target.x + activeView->OverviewUMin();
  double VMinOverview = Target.y + activeView->OverviewVMin();

  CRect rc;
  rc.left = Eo::Round((UMin - UMinOverview) / activeView->OverviewUExt() * bitmap.bmWidth);
  rc.right = Eo::Round((UMax - UMinOverview) / activeView->OverviewUExt() * bitmap.bmWidth);
  rc.top = Eo::Round((1.0 - (VMax - VMinOverview) / activeView->OverviewVExt()) * bitmap.bmHeight);
  rc.bottom = Eo::Round((1.0 - (VMin - VMinOverview) / activeView->OverviewVExt()) * bitmap.bmHeight);

  int DrawMode = dc.SetROP2(R2_XORPEN);

  // Show current window as light gray rectangle with no outline
  dc.SelectStockObject(LTGRAY_BRUSH);
  CPen* pPen = (CPen*)dc.SelectStockObject(NULL_PEN);
  dc.Rectangle(rc.left, rc.top, rc.right, rc.bottom);

  // Show defining window as hollow rectangle with dark gray outline
  dc.SelectStockObject(NULL_BRUSH);
  CPen penGray(PS_SOLID, 2, RGB(0x80, 0x80, 0x80));
  dc.SelectObject(&penGray);
  dc.Rectangle(EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top,
               EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

  // Restore device context
  dc.SelectObject(pPen);
  dc.SelectObject(pBrush);
  dc.SetROP2(DrawMode);

  dc.Detach();

  EndPaint(hwnd, &ps);
}

void WndProcKeyPlanOnMouseMove(HWND hwnd, WPARAM nParam, LPARAM lParam) {
  if (LOWORD(nParam) == MK_LBUTTON) {
    CPoint pntCur;

    pntCur.x = LOWORD(lParam);
    pntCur.y = HIWORD(lParam);

    CDC dcKeyPlan;
    HDC hDCKeyplan = ::GetDC(hwnd);
    int DrawMode = ::SetROP2(hDCKeyplan, R2_XORPEN);

    // Show defining window as hollow rectangle with dark gray outline
    HBRUSH hBrush = (HBRUSH)::SelectObject(hDCKeyplan, ::GetStockObject(NULL_BRUSH));
    HPEN hPenGray = ::CreatePen(PS_SOLID, 2, RGB(0x80, 0x80, 0x80));
    HPEN hPen = (HPEN)::SelectObject(hDCKeyplan, hPenGray);
    ::Rectangle(hDCKeyplan, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top,
                EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

    if (leftButtonDownInKeyplanRectangle) {
      OffsetRect(&EoDlgActiveViewKeyplan::m_rcWnd, (pntCur.x - pnt.x), (pntCur.y - pnt.y));
    } else {
      if (pntCur.x > EoDlgActiveViewKeyplan::m_rcWnd.right)
        EoDlgActiveViewKeyplan::m_rcWnd.right += (pntCur.x - pnt.x);
      else if (pntCur.x < EoDlgActiveViewKeyplan::m_rcWnd.left)
        EoDlgActiveViewKeyplan::m_rcWnd.left += (pntCur.x - pnt.x);
      if (pntCur.y > EoDlgActiveViewKeyplan::m_rcWnd.bottom)
        EoDlgActiveViewKeyplan::m_rcWnd.bottom += (pntCur.y - pnt.y);
      else if (pntCur.y < EoDlgActiveViewKeyplan::m_rcWnd.top)
        EoDlgActiveViewKeyplan::m_rcWnd.top += (pntCur.y - pnt.y);
      ::SendMessage(::GetParent(hwnd), WM_COMMAND, (WPARAM)::GetWindowLong(hwnd, GWL_ID), (LPARAM)hwnd);
    }
    pnt = pntCur;
    ::Rectangle(hDCKeyplan, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top,
                EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

    ::SelectObject(hDCKeyplan, hPen);
    ::SelectObject(hDCKeyplan, hBrush);
    ::DeleteObject(hPenGray);
    ::SetROP2(hDCKeyplan, DrawMode);
    ::ReleaseDC(hwnd, hDCKeyplan);
  }
}

void WndProcKeyPlanOnNewRatio(HWND hwnd, LPARAM lParam) {
  double Ratio = *(double*)(LPDWORD)lParam;

  auto* activeView = AeSysView::GetActiveView();

  EoGePoint3d Target = activeView->CameraTarget();

  double UExtent = activeView->WidthInInches() / Ratio;
  double UMin = Target.x - (UExtent * 0.5);
  double UMax = UMin + UExtent;
  double VExtent = activeView->HeightInInches() / Ratio;
  double VMin = Target.y - (VExtent * 0.5);
  double VMax = VMin + VExtent;

  HDC hDCKeyplan = ::GetDC(hwnd);
  int DrawMode = ::SetROP2(hDCKeyplan, R2_XORPEN);

  HBRUSH hBrush = (HBRUSH)::SelectObject(hDCKeyplan, ::GetStockObject(NULL_BRUSH));
  HPEN hPenGray = ::CreatePen(PS_SOLID, 2, RGB(0x80, 0x80, 0x80));
  HPEN hPen = (HPEN)::SelectObject(hDCKeyplan, hPenGray);
  ::Rectangle(hDCKeyplan, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top,
              EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

  CDC dcMem;
  dcMem.CreateCompatibleDC(nullptr);

  dcMem.SelectObject(CBitmap::FromHandle(EoDlgActiveViewKeyplan::m_hbmKeyplan));
  BITMAP bitmap;
  ::GetObject(EoDlgActiveViewKeyplan::m_hbmKeyplan, sizeof(BITMAP), (LPSTR)&bitmap);

  double dUMinOverview = Target.x + activeView->OverviewUMin();
  double dVMinOverview = Target.y + activeView->OverviewVMin();

  EoDlgActiveViewKeyplan::m_rcWnd.left =
      Eo::Round((UMin - dUMinOverview) / activeView->OverviewUExt() * bitmap.bmWidth);
  EoDlgActiveViewKeyplan::m_rcWnd.top =
      Eo::Round((1.0 - (VMax - dVMinOverview) / activeView->OverviewVExt()) * bitmap.bmHeight);
  EoDlgActiveViewKeyplan::m_rcWnd.right =
      Eo::Round((UMax - dUMinOverview) / activeView->OverviewUExt() * bitmap.bmWidth);
  EoDlgActiveViewKeyplan::m_rcWnd.bottom =
      Eo::Round((1.0 - (VMin - dVMinOverview) / activeView->OverviewVExt()) * bitmap.bmHeight);

  ::Rectangle(hDCKeyplan, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top,
              EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);
  ::SelectObject(hDCKeyplan, hPen);
  ::SelectObject(hDCKeyplan, hBrush);
  ::DeleteObject(hPenGray);

  ::SetROP2(hDCKeyplan, DrawMode);
  ::ReleaseDC(hwnd, hDCKeyplan);
}
}  // namespace

LRESULT CALLBACK WndProcKeyPlan(HWND hwnd, UINT message, WPARAM nParam, LPARAM lParam) {
  switch (message) {
    case WM_USER_ON_NEW_RATIO:
      WndProcKeyPlanOnNewRatio(hwnd, lParam);
      break;

    case WM_PAINT:
      WndProcKeyPlanOnDraw(hwnd);
      return (FALSE);

    case WM_LBUTTONDOWN:
      SetFocus(hwnd);
      pnt.x = LOWORD(lParam);
      pnt.y = HIWORD(lParam);
      leftButtonDownInKeyplanRectangle = ::PtInRect(&EoDlgActiveViewKeyplan::m_rcWnd, pnt) != 0;
      return (FALSE);

    case WM_MOUSEMOVE:
      WndProcKeyPlanOnMouseMove(hwnd, nParam, lParam);
      return (FALSE);
  }
  return DefWindowProc(hwnd, message, nParam, lParam);
}

ATOM WINAPI RegisterKeyPlanWindowClass(HINSTANCE instance) {
  WNDCLASS Class{};
  Class.style = CS_HREDRAW | CS_VREDRAW;
  Class.lpfnWndProc = WndProcKeyPlan;
  Class.cbClsExtra = 0;
  Class.cbWndExtra = 0;
  Class.hInstance = instance;
  Class.hIcon = 0;
  Class.hCursor = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_CROSS, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
  Class.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
  Class.lpszMenuName = 0;
  Class.lpszClassName = L"KeyPlanWindow";

  return ::RegisterClass(&Class);
}