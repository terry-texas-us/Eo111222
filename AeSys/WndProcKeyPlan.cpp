#include "Stdafx.h"

#include <cassert>

#include "AeSysView.h"
#include "Eo.h"
#include "EoDlgActiveViewKeyplan.h"

namespace {
CPoint point{};
bool leftButtonDownInKeyplanRectangle{};

void WndProcKeyPlanOnDraw(HWND hwnd) {
  PAINTSTRUCT paint;

  CDC deviceContext{};
  deviceContext.Attach(BeginPaint(hwnd, &paint));

  CDC memoryContext;
  memoryContext.CreateCompatibleDC(nullptr);

  memoryContext.SelectObject(CBitmap::FromHandle(EoDlgActiveViewKeyplan::m_hbmKeyplan));
  BITMAP bitmap{};
  ::GetObjectW(EoDlgActiveViewKeyplan::m_hbmKeyplan, sizeof(BITMAP), (LPTSTR)&bitmap);

  deviceContext.BitBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, &memoryContext, 0, 0, SRCCOPY);

  auto* brush = deviceContext.SelectStockObject(NULL_BRUSH);
  deviceContext.Rectangle(0, 0, bitmap.bmWidth, bitmap.bmHeight);

  /// @todo Need to use the CWnd associated with Keyplan and not the active app view

  auto* activeView = AeSysView::GetActiveView();

  auto cameraTarget = activeView->CameraTarget();

  double uMin = cameraTarget.x + activeView->UMin();
  double uMax = cameraTarget.x + activeView->UMax();
  double vMin = cameraTarget.y + activeView->VMin();
  double vMax = cameraTarget.y + activeView->VMax();

  double uMinOverview = cameraTarget.x + activeView->OverviewUMin();
  double vMinOverview = cameraTarget.y + activeView->OverviewVMin();

  CRect rectangle;
  rectangle.left = Eo::Round((uMin - uMinOverview) / activeView->OverviewUExt() * bitmap.bmWidth);
  rectangle.right = Eo::Round((uMax - uMinOverview) / activeView->OverviewUExt() * bitmap.bmWidth);
  rectangle.top = Eo::Round((1.0 - (vMax - vMinOverview) / activeView->OverviewVExt()) * bitmap.bmHeight);
  rectangle.bottom = Eo::Round((1.0 - (vMin - vMinOverview) / activeView->OverviewVExt()) * bitmap.bmHeight);

  auto drawMode = deviceContext.SetROP2(R2_XORPEN);

  // Show current window as light gray rectangle with no outline
  deviceContext.SelectStockObject(LTGRAY_BRUSH);
  auto* pen = deviceContext.SelectStockObject(NULL_PEN);
  deviceContext.Rectangle(rectangle.left, rectangle.top, rectangle.right, rectangle.bottom);

  // Show defining window as hollow rectangle with dark gray outline
  deviceContext.SelectStockObject(NULL_BRUSH);
  CPen grayPen(PS_SOLID, 2, RGB(0x80, 0x80, 0x80));
  deviceContext.SelectObject(&grayPen);
  deviceContext.Rectangle(EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top,
                          EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

  // Restore device context
  deviceContext.SelectObject(pen);
  deviceContext.SelectObject(brush);
  deviceContext.SetROP2(drawMode);

  deviceContext.Detach();

  EndPaint(hwnd, &paint);
}

void WndProcKeyPlanOnMouseMove(HWND hwnd, WPARAM nParam, LPARAM lParam) {
  if (LOWORD(nParam) != MK_LBUTTON) { return; }
  CPoint currentPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  auto deviceContextHandle = ::GetDC(hwnd);
  auto drawMode = ::SetROP2(deviceContextHandle, R2_XORPEN);

  // Show defining window as hollow rectangle with dark gray outline
  HBRUSH brush = (HBRUSH)::SelectObject(deviceContextHandle, ::GetStockObject(NULL_BRUSH));
  HPEN grayPen = ::CreatePen(PS_SOLID, 2, RGB(0x80, 0x80, 0x80));
  HPEN pen = (HPEN)::SelectObject(deviceContextHandle, grayPen);
  ::Rectangle(deviceContextHandle, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top,
              EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

  if (leftButtonDownInKeyplanRectangle) {
    OffsetRect(&EoDlgActiveViewKeyplan::m_rcWnd, (currentPoint.x - point.x), (currentPoint.y - point.y));
  } else {
    if (currentPoint.x > EoDlgActiveViewKeyplan::m_rcWnd.right) {
      EoDlgActiveViewKeyplan::m_rcWnd.right += (currentPoint.x - point.x);
    } else if (currentPoint.x < EoDlgActiveViewKeyplan::m_rcWnd.left) {
      EoDlgActiveViewKeyplan::m_rcWnd.left += (currentPoint.x - point.x);
    }
    if (currentPoint.y > EoDlgActiveViewKeyplan::m_rcWnd.bottom) {
      EoDlgActiveViewKeyplan::m_rcWnd.bottom += (currentPoint.y - point.y);
    } else if (currentPoint.y < EoDlgActiveViewKeyplan::m_rcWnd.top) {
      EoDlgActiveViewKeyplan::m_rcWnd.top += (currentPoint.y - point.y);
    }
    ::SendMessage(::GetParent(hwnd), WM_COMMAND, (WPARAM)::GetWindowLong(hwnd, GWL_ID), (LPARAM)hwnd);
  }
  point = currentPoint;
  ::Rectangle(deviceContextHandle, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top,
              EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

  ::SelectObject(deviceContextHandle, pen);
  ::SelectObject(deviceContextHandle, brush);
  ::DeleteObject(grayPen);
  ::SetROP2(deviceContextHandle, drawMode);
  ::ReleaseDC(hwnd, deviceContextHandle);
}

void WndProcKeyPlanOnNewRatio(HWND hwnd, LPARAM lParam) {
  double ratio = *(double*)(LPDWORD)lParam;

  auto* activeView = AeSysView::GetActiveView();
  assert(activeView != nullptr);

  auto cameraTarget = activeView->CameraTarget();

  double uExtent = activeView->WidthInInches() / ratio;
  double uMin = cameraTarget.x - (uExtent * 0.5);
  double uMax = uMin + uExtent;
  double vExtent = activeView->HeightInInches() / ratio;
  double vMin = cameraTarget.y - (vExtent * 0.5);
  double vMax = vMin + vExtent;

  HDC deviceContextHandle = ::GetDC(hwnd);
  int drawMode = ::SetROP2(deviceContextHandle, R2_XORPEN);

  HBRUSH brush = (HBRUSH)::SelectObject(deviceContextHandle, ::GetStockObject(NULL_BRUSH));
  HPEN grayPen = ::CreatePen(PS_SOLID, 2, RGB(0x80, 0x80, 0x80));
  HPEN pen = (HPEN)::SelectObject(deviceContextHandle, grayPen);
  Rectangle(deviceContextHandle, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top,
              EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

  CDC dcMem{};
  dcMem.CreateCompatibleDC(nullptr);

  dcMem.SelectObject(CBitmap::FromHandle(EoDlgActiveViewKeyplan::m_hbmKeyplan));
  BITMAP bitmap{};
  GetObjectW(EoDlgActiveViewKeyplan::m_hbmKeyplan, sizeof(BITMAP), &bitmap);

  double dUMinOverview = cameraTarget.x + activeView->OverviewUMin();
  double dVMinOverview = cameraTarget.y + activeView->OverviewVMin();

  EoDlgActiveViewKeyplan::m_rcWnd.left =
      Eo::Round((uMin - dUMinOverview) / activeView->OverviewUExt() * bitmap.bmWidth);
  EoDlgActiveViewKeyplan::m_rcWnd.top =
      Eo::Round((1.0 - (vMax - dVMinOverview) / activeView->OverviewVExt()) * bitmap.bmHeight);
  EoDlgActiveViewKeyplan::m_rcWnd.right =
      Eo::Round((uMax - dUMinOverview) / activeView->OverviewUExt() * bitmap.bmWidth);
  EoDlgActiveViewKeyplan::m_rcWnd.bottom =
      Eo::Round((1.0 - (vMin - dVMinOverview) / activeView->OverviewVExt()) * bitmap.bmHeight);

  ::Rectangle(deviceContextHandle, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top,
              EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);
  ::SelectObject(deviceContextHandle, pen);
  ::SelectObject(deviceContextHandle, brush);
  ::DeleteObject(grayPen);

  ::SetROP2(deviceContextHandle, drawMode);
  ::ReleaseDC(hwnd, deviceContextHandle);
}
}  // namespace

LRESULT CALLBACK WndProcKeyPlan(HWND hwnd, UINT message, WPARAM nParam, LPARAM lParam) {
  switch (message) {
    case WM_USER_ON_NEW_RATIO:
      WndProcKeyPlanOnNewRatio(hwnd, lParam);
      break;

    case WM_PAINT:
      WndProcKeyPlanOnDraw(hwnd);
      return FALSE;

    case WM_LBUTTONDOWN:
      SetFocus(hwnd);
      point.x = GET_X_LPARAM(lParam);
      point.y = GET_Y_LPARAM(lParam);
      leftButtonDownInKeyplanRectangle = ::PtInRect(&EoDlgActiveViewKeyplan::m_rcWnd, point) != 0;
      return FALSE;

    case WM_MOUSEMOVE:
      WndProcKeyPlanOnMouseMove(hwnd, nParam, lParam);
      return FALSE;
  }
  return DefWindowProc(hwnd, message, nParam, lParam);
}

ATOM WINAPI RegisterKeyPlanWindowClass(HINSTANCE instance) {
  WNDCLASS windowClass{};
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = WndProcKeyPlan;
  windowClass.hInstance = instance;
  windowClass.hCursor = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_CROSS, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
  windowClass.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
  windowClass.lpszClassName = L"KeyPlanWindow";

  return ::RegisterClass(&windowClass);
}