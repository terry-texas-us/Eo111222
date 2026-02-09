#include "Stdafx.h"

#include "AeSysView.h"
#include "Eo.h"
#include "EoDbBlock.h"
#include "EoDbGroupList.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "PrimState.h"

namespace {
CBitmap* previewBitmap{};
}  // namespace

/** Clears the preview window by filling it with black.
 *
 * @param previewWindow Handle to the preview window.
 */
void WndProcPreviewClear(HWND previewWindow) {
  if (previewWindow == nullptr || previewBitmap == nullptr) { return; }

  CRect previewWindowRect;
  if (!GetClientRect(previewWindow, &previewWindowRect)) { return; }

  CDC memoryContext;
  if (!memoryContext.CreateCompatibleDC(nullptr)) { return; }

  auto* bitmap = memoryContext.SelectObject(previewBitmap);
  if (bitmap == nullptr) { return; }

  memoryContext.PatBlt(0, 0, previewWindowRect.right, previewWindowRect.bottom, BLACKNESS);

  memoryContext.SelectObject(bitmap);
  ::InvalidateRect(previewWindow, nullptr, TRUE);
}

/** Updates the preview window to display the given block.
 *
 * @param previewWindow Handle to the preview window.
 * @param block Pointer to the block to display.
 */
void WndProcPreviewUpdateBlock(HWND previewWindow, EoDbBlock* block) {
  auto* activeView = AeSysView::GetActiveView();

  CRect previewWindowRect;
  GetClientRect(previewWindow, &previewWindowRect);

  CDC memoryContext;
  memoryContext.CreateCompatibleDC(nullptr);

  CBitmap* Bitmap = memoryContext.SelectObject(previewBitmap);
  memoryContext.PatBlt(0, 0, previewWindowRect.right, previewWindowRect.bottom, BLACKNESS);

  activeView->ViewportPushActive();
  activeView->SetViewportSize(previewWindowRect.right, previewWindowRect.bottom);
  activeView->SetDeviceWidthInInches(static_cast<double>(memoryContext.GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch);
  activeView->SetDeviceHeightInInches(static_cast<double>(memoryContext.GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch);

  EoGeTransformMatrix transformMatrix;

  EoGePoint3d ptMin(Eo::boundsMax, Eo::boundsMax, Eo::boundsMax);
  EoGePoint3d ptMax(Eo::boundsMin, Eo::boundsMin, Eo::boundsMin);

  block->GetExtents(activeView, ptMin, ptMax, transformMatrix);

  double UExtent = ptMax.x - ptMin.x;
  double VExtent = ptMax.y - ptMin.y;

  activeView->PushViewTransform();

  activeView->SetCenteredWindow(UExtent, VExtent);

  EoGePoint3d ptTarget((ptMin.x + ptMax.x) / 2.0, (ptMin.y + ptMax.y) / 2.0, 0.0);

  activeView->SetCameraTarget(ptTarget);
  activeView->SetCameraPosition(activeView->CameraDirection());

  int PrimitiveState = pstate.Save();
  block->Display(activeView, &memoryContext);

  activeView->PopViewTransform();
  activeView->ViewportPopActive();

  pstate.Restore(&memoryContext, PrimitiveState);
  memoryContext.SelectObject(Bitmap);
  InvalidateRect(previewWindow, 0, TRUE);
}

/** Updates the preview window to display the given group list.
 *
 * @param previewWindow Handle to the preview window.
 * @param groups Pointer to the group list to display.
 */
void WndProcPreviewUpdateLayer(HWND previewWindow, EoDbGroupList* groups) {
  auto* activeView = AeSysView::GetActiveView();

  CRect previewWindowRect;
  GetClientRect(previewWindow, &previewWindowRect);

  CDC memoryContext;
  memoryContext.CreateCompatibleDC(nullptr);

  CBitmap* Bitmap = memoryContext.SelectObject(previewBitmap);
  memoryContext.PatBlt(0, 0, previewWindowRect.right, previewWindowRect.bottom, BLACKNESS);

  activeView->ViewportPushActive();
  activeView->SetViewportSize(previewWindowRect.right, previewWindowRect.bottom);
  activeView->SetDeviceWidthInInches(static_cast<double>(memoryContext.GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch);
  activeView->SetDeviceHeightInInches(static_cast<double>(memoryContext.GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch);

  EoGeTransformMatrix transformMatrix;

  EoGePoint3d ptMin(Eo::boundsMax, Eo::boundsMax, Eo::boundsMax);
  EoGePoint3d ptMax(Eo::boundsMin, Eo::boundsMin, Eo::boundsMin);

  groups->GetExtents(activeView, ptMin, ptMax, transformMatrix);

  double UExtent = ptMax.x - ptMin.x;
  double VExtent = ptMax.y - ptMin.y;
  EoGePoint3d ptTarget((ptMin.x + ptMax.x) / 2.0, (ptMin.y + ptMax.y) / 2.0, 0.0);

  activeView->PushViewTransform();
  activeView->SetCenteredWindow(UExtent, VExtent);
  activeView->SetCameraTarget(ptTarget);
  activeView->SetCameraPosition(activeView->CameraDirection());

  int PrimitiveState = pstate.Save();
  groups->Display(activeView, &memoryContext);
  pstate.Restore(&memoryContext, PrimitiveState);

  activeView->PopViewTransform();
  activeView->ViewportPopActive();

  memoryContext.SelectObject(Bitmap);
  InvalidateRect(previewWindow, 0, TRUE);
}

/** Window procedure for the preview window.
 *
 * @param hwnd Handle to the window.
 * @param message Message identifier.
 * @param nParam Additional message information.
 * @param lParam Additional message information.
 * @return The result of the message processing.
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
    }
      return FALSE;

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

      CBitmap* Bitmap = memoryContext.SelectObject(previewBitmap);
      dc.BitBlt(0, 0, previewWindowRect.right, previewWindowRect.bottom, &memoryContext, 0, 0, SRCCOPY);
      memoryContext.SelectObject(Bitmap);

      dc.Detach();

      EndPaint(hwnd, &paintStruct);
    }

      return FALSE;

    case WM_LBUTTONDOWN:
      ::SetFocus(hwnd);
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Preview WM_LBUTTONDOWN message\n");
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
  WNDCLASS previewWindowClass{};

  previewWindowClass.style = CS_HREDRAW | CS_VREDRAW;
  previewWindowClass.lpfnWndProc = WndProcPreview;
  previewWindowClass.cbClsExtra = 0;
  previewWindowClass.cbWndExtra = 0;
  previewWindowClass.hInstance = instance;
  previewWindowClass.hIcon = nullptr;
  previewWindowClass.hCursor = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_CROSS, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
  previewWindowClass.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
  previewWindowClass.lpszMenuName = nullptr;
  previewWindowClass.lpszClassName = L"PreviewWindow";

  return ::RegisterClass(&previewWindowClass);
}
