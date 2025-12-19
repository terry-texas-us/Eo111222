#include "stdafx.h"

#include "AeSysView.h"
#include "EoDbBlock.h"
#include "EoDbGroupList.h"
#include "PrimState.h"

CBitmap* WndProcPreview_Bitmap = nullptr;

LRESULT CALLBACK WndProcPreview(HWND, UINT, WPARAM, LPARAM);

ATOM WINAPI RegisterPreviewWindowClass(HINSTANCE instance) {
  WNDCLASS Class;

  Class.style = CS_HREDRAW | CS_VREDRAW;
  Class.lpfnWndProc = WndProcPreview;
  Class.cbClsExtra = 0;
  Class.cbWndExtra = 0;
  Class.hInstance = instance;
  Class.hIcon = 0;
  Class.hCursor = (HCURSOR)::LoadImage(nullptr, IDC_CROSS, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);
  Class.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
  Class.lpszMenuName = 0;
  Class.lpszClassName = L"PreviewWindow";

  return ::RegisterClass(&Class);
}
LRESULT CALLBACK WndProcPreview(HWND hwnd, UINT message, WPARAM nParam, LPARAM lParam) {
  switch (message) {
    case WM_CREATE: {
      auto* activeView = AeSysView::GetActiveView();
      CDC* DeviceContext = (activeView == nullptr) ? nullptr : activeView->GetDC();

      CRect rc;
      ::GetClientRect(hwnd, &rc);
      WndProcPreview_Bitmap = new CBitmap;
      WndProcPreview_Bitmap->CreateCompatibleBitmap(DeviceContext, int(rc.right), int(rc.bottom));
    }
      return (FALSE);

    case WM_DESTROY:
      if (WndProcPreview_Bitmap != nullptr) {
        delete WndProcPreview_Bitmap;
        WndProcPreview_Bitmap = nullptr;
      }
      return (FALSE);

    case WM_PAINT: {
      PAINTSTRUCT ps;

      CRect rc;
      ::GetClientRect(hwnd, &rc);

      CDC dc;
      dc.Attach(::BeginPaint(hwnd, &ps));

      CDC dcMem;
      dcMem.CreateCompatibleDC(nullptr);

      CBitmap* Bitmap = dcMem.SelectObject(WndProcPreview_Bitmap);
      dc.BitBlt(0, 0, rc.right, rc.bottom, &dcMem, 0, 0, SRCCOPY);
      dcMem.SelectObject(Bitmap);

      dc.Detach();

      ::EndPaint(hwnd, &ps);
    }

      return (FALSE);

    case WM_LBUTTONDOWN:
      ::SetFocus(hwnd);
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Preview WM_LBUTTONDOWN message\n");
      return (FALSE);
  }
  return DefWindowProc(hwnd, message, nParam, lParam);
}

void WndProcPreviewClear(HWND previewWindow) {
  CRect rc;
  ::GetClientRect(previewWindow, &rc);

  CDC dcMem;
  dcMem.CreateCompatibleDC(0);

  CBitmap* Bitmap = (CBitmap*)dcMem.SelectObject(WndProcPreview_Bitmap);
  dcMem.PatBlt(0, 0, rc.right, rc.bottom, BLACKNESS);

  dcMem.SelectObject(Bitmap);
  ::InvalidateRect(previewWindow, 0, TRUE);
}

void WndProcPreviewUpdate(HWND previewWindow, EoDbBlock* block) {
  auto* activeView = AeSysView::GetActiveView();

  CRect rc;
  ::GetClientRect(previewWindow, &rc);

  CDC dcMem;
  dcMem.CreateCompatibleDC(nullptr);

  CBitmap* Bitmap = dcMem.SelectObject(WndProcPreview_Bitmap);
  dcMem.PatBlt(0, 0, rc.right, rc.bottom, BLACKNESS);

  activeView->ViewportPushActive();
  activeView->SetViewportSize(rc.right, rc.bottom);
  activeView->SetDeviceWidthInInches(static_cast<double>(dcMem.GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch);
  activeView->SetDeviceHeightInInches(static_cast<double>(dcMem.GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch);

  EoGeTransformMatrix tm;

  EoGePoint3d ptMin(FLT_MAX, FLT_MAX, FLT_MAX);
  EoGePoint3d ptMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  block->GetExtents(activeView, ptMin, ptMax, tm);

  double UExtent = ptMax.x - ptMin.x;
  double VExtent = ptMax.y - ptMin.y;

  activeView->PushViewTransform();

  activeView->SetCenteredWindow(UExtent, VExtent);

  EoGePoint3d ptTarget((ptMin.x + ptMax.x) / 2.0, (ptMin.y + ptMax.y) / 2.0, 0.0);

  activeView->SetCameraTarget(ptTarget);
  activeView->SetCameraPosition(activeView->CameraDirection());

  int PrimitiveState = pstate.Save();
  block->Display(activeView, &dcMem);

  activeView->PopViewTransform();
  activeView->ViewportPopActive();

  pstate.Restore(&dcMem, PrimitiveState);
  dcMem.SelectObject(Bitmap);
  ::InvalidateRect(previewWindow, 0, TRUE);
}

void _WndProcPreviewUpdate(HWND previewWindow, EoDbGroupList* groups) {
  auto* activeView = AeSysView::GetActiveView();

  CRect rc;
  ::GetClientRect(previewWindow, &rc);

  CDC dcMem;
  dcMem.CreateCompatibleDC(nullptr);

  CBitmap* Bitmap = dcMem.SelectObject(WndProcPreview_Bitmap);
  dcMem.PatBlt(0, 0, rc.right, rc.bottom, BLACKNESS);

  activeView->ViewportPushActive();
  activeView->SetViewportSize(rc.right, rc.bottom);
  activeView->SetDeviceWidthInInches(static_cast<double>(dcMem.GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch);
  activeView->SetDeviceHeightInInches(static_cast<double>(dcMem.GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch);

  EoGeTransformMatrix tm;

  EoGePoint3d ptMin(FLT_MAX, FLT_MAX, FLT_MAX);
  EoGePoint3d ptMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  groups->GetExtents(activeView, ptMin, ptMax, tm);

  double UExtent = ptMax.x - ptMin.x;
  double VExtent = ptMax.y - ptMin.y;
  EoGePoint3d ptTarget((ptMin.x + ptMax.x) / 2.0, (ptMin.y + ptMax.y) / 2.0, 0.0);

  activeView->PushViewTransform();
  activeView->SetCenteredWindow(UExtent, VExtent);
  activeView->SetCameraTarget(ptTarget);
  activeView->SetCameraPosition(activeView->CameraDirection());

  int PrimitiveState = pstate.Save();
  groups->Display(activeView, &dcMem);
  pstate.Restore(&dcMem, PrimitiveState);

  activeView->PopViewTransform();
  activeView->ViewportPopActive();

  dcMem.SelectObject(Bitmap);
  ::InvalidateRect(previewWindow, 0, TRUE);
}
