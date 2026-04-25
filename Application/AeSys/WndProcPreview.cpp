#include "Stdafx.h"

#include "AeSysView.h"
#include "Eo.h"
#include "EoDbBlock.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGsRenderDeviceGdi.h"
#include "EoGsRenderState.h"

namespace {
CBitmap* previewBitmap{};
}  // namespace

void WndProcPreviewClear(HWND previewWindow) {
  if (previewWindow == nullptr || previewBitmap == nullptr) { return; }

  CRect previewWindowRect;
  if (!GetClientRect(previewWindow, &previewWindowRect)) { return; }

  CDC memoryContext;
  if (!memoryContext.CreateCompatibleDC(nullptr)) { return; }

  auto* bitmap = memoryContext.SelectObject(previewBitmap);
  if (bitmap == nullptr) { return; }

  memoryContext.PatBlt(0, 0, previewWindowRect.right, previewWindowRect.bottom, WHITENESS);

  memoryContext.SelectObject(bitmap);
  ::InvalidateRect(previewWindow, nullptr, TRUE);
}

void WndProcPreviewUpdateBlock(HWND previewWindow, EoDbBlock* block) {
  auto* activeView = AeSysView::GetActiveView();

  CRect previewWindowRect;
  GetClientRect(previewWindow, &previewWindowRect);

  CDC memoryContext;
  memoryContext.CreateCompatibleDC(nullptr);

  CBitmap* bitmap = memoryContext.SelectObject(previewBitmap);
  memoryContext.PatBlt(0, 0, previewWindowRect.right, previewWindowRect.bottom, WHITENESS);

  // Swap ACI 7 and ACI 0 to black for white background visibility
  const auto savedAci7 = Eo::ColorPalette[7];
  const auto savedAci0 = Eo::ColorPalette[0];
  const auto savedGray7 = Eo::GrayPalette[7];
  const auto savedGray0 = Eo::GrayPalette[0];
  Eo::ColorPalette[7] = Eo::colorBlack;
  Eo::ColorPalette[0] = Eo::colorBlack;
  Eo::GrayPalette[7] = RGB(0x22, 0x22, 0x22);
  Eo::GrayPalette[0] = RGB(0x22, 0x22, 0x22);

  // 2px padding around preview content
  constexpr int pad = 2;
  CRect paddedRect = previewWindowRect;
  paddedRect.DeflateRect(pad, pad);

  activeView->ViewportPushActive();
  activeView->SetViewportSize(paddedRect.Width(), paddedRect.Height());
  activeView->SetDeviceWidthInInches(static_cast<double>(memoryContext.GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch);
  activeView->SetDeviceHeightInInches(static_cast<double>(memoryContext.GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch);

  const EoGeTransformMatrix transformMatrix;

  EoGePoint3d ptMin(Eo::boundsMax, Eo::boundsMax, Eo::boundsMax);
  EoGePoint3d ptMax(Eo::boundsMin, Eo::boundsMin, Eo::boundsMin);

  block->GetExtents(activeView, ptMin, ptMax, transformMatrix);

  const double uExtent = ptMax.x - ptMin.x;
  const double vExtent = ptMax.y - ptMin.y;

  activeView->PushViewTransform();

  activeView->SetCenteredWindow(uExtent, vExtent);

  const EoGePoint3d ptTarget((ptMin.x + ptMax.x) / 2.0, (ptMin.y + ptMax.y) / 2.0, 0.0);

  activeView->SetCameraTarget(ptTarget);
  activeView->SetCameraPosition(activeView->CameraDirection());

  const int savedRenderState = Gs::renderState.Save();
  EoGsRenderDeviceGdi renderDevice(&memoryContext);
  block->Display(activeView, &renderDevice);

  activeView->PopViewTransform();
  activeView->ViewportPopActive();

  Gs::renderState.Restore(&memoryContext, savedRenderState);
  Eo::ColorPalette[7] = savedAci7;
  Eo::ColorPalette[0] = savedAci0;
  Eo::GrayPalette[7] = savedGray7;
  Eo::GrayPalette[0] = savedGray0;
  memoryContext.SelectObject(bitmap);
  InvalidateRect(previewWindow, nullptr, TRUE);
}

/** Window procedure for the preview window.
 *
 * @param hwnd Handle to the preview window.
 * @param message The message being processed.
 * @param nParam Additional message information (WPARAM).
 * @param lParam Additional message information (LPARAM).
 * @return The result of message processing.
 */
LRESULT CALLBACK WndProcPreview(HWND hwnd, UINT message, WPARAM nParam, LPARAM lParam) {
  switch (message) {
    case WM_CREATE: {
      auto* activeView = AeSysView::GetActiveView();
      CDC* deviceContext = (activeView == nullptr) ? nullptr : activeView->GetDC();

      CRect previewWindowRect;
      GetClientRect(hwnd, &previewWindowRect);
      previewBitmap = new CBitmap;
      previewBitmap->CreateCompatibleBitmap(deviceContext, int(previewWindowRect.right), int(previewWindowRect.bottom));
      return FALSE;
    }
    case WM_DESTROY:
      if (previewBitmap != nullptr) {
        delete previewBitmap;
        previewBitmap = nullptr;
      }
      return FALSE;

    case WM_PAINT: {
      PAINTSTRUCT paintStruct;

      CRect previewWindowRect;
      GetClientRect(hwnd, &previewWindowRect);

      CDC dc;
      dc.Attach(BeginPaint(hwnd, &paintStruct));

      CDC memoryContext;
      memoryContext.CreateCompatibleDC(nullptr);

      CBitmap* bitmap = memoryContext.SelectObject(previewBitmap);
      dc.BitBlt(0, 0, previewWindowRect.right, previewWindowRect.bottom, &memoryContext, 0, 0, SRCCOPY);
      memoryContext.SelectObject(bitmap);

      dc.Detach();

      EndPaint(hwnd, &paintStruct);
      return FALSE;
    }
    case WM_SIZE: {
      if (previewBitmap != nullptr) {
        delete previewBitmap;
        previewBitmap = nullptr;
      }
      CRect rect;
      if (GetClientRect(hwnd, &rect) && rect.Width() > 0 && rect.Height() > 0) {
        auto* activeView = AeSysView::GetActiveView();
        CDC* deviceContext = (activeView == nullptr) ? nullptr : activeView->GetDC();
        // Fallback to screen DC if no view (safer than nullptr)
        if (deviceContext == nullptr) {
          CClientDC screenContext(nullptr);
          deviceContext = &screenContext;
        }
        previewBitmap = new CBitmap();
        if (!previewBitmap->CreateCompatibleBitmap(deviceContext, rect.Width(), rect.Height())) {
          delete previewBitmap;
          previewBitmap = nullptr;
        }
      }
      InvalidateRect(hwnd, nullptr, TRUE);
      return FALSE;
    }
    case WM_LBUTTONDOWN:
      ::SetFocus(hwnd);
      ATLTRACE2(traceGeneral, 3, L"Preview WM_LBUTTONDOWN message\n");
      return FALSE;
  }
  return DefWindowProc(hwnd, message, nParam, lParam);
}

/** Registers the preview window class.
 *
 * @param instance Handle to the application instance.
 * @return The atom identifying the registered class.
 */
ATOM WINAPI RegisterPreviewWindowClass(HINSTANCE instance) {
  WNDCLASS windowClass{};
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = WndProcPreview;
  windowClass.hInstance = instance;
  windowClass.hCursor = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_CROSS, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
  windowClass.hbrBackground = static_cast<HBRUSH>(::GetStockObject(DKGRAY_BRUSH));
  windowClass.lpszClassName = L"PreviewWindow";

  return ::RegisterClassW(&windowClass);
}
