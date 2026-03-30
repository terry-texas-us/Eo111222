#include "Stdafx.h"

#include <cassert>
#include <cmath>
#include <memory>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbLayer.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderDeviceDirect2D.h"
#include "EoGsRenderDeviceGdi.h"
#include "EoGsRenderState.h"
#include "EoGsViewTransform.h"
#include "MainFrm.h"

#if defined(USING_STATE_PATTERN)
#include "AeSysState.h"
#include "IdleState.h"
#endif

#if defined(USING_DDE)
#include "Dde.h"
#include "DdeGItms.h"
#endif

namespace {

#if defined(LEGACY_ODOMETER)
/** @deprecated This code is only used for the legacy odometer display, which draws the odometer values directly in the
 view. The current implementation displays the odometer values in the status bar, so this code is no longer needed.*/
void DrawOdometerInView(AeSysView* view, CDC* context, Eo::Units Units, EoGeVector3d& position) {
  auto* oldFont = static_cast<CFont*>(context->SelectStockObject(DEFAULT_GUI_FONT));
  auto oldTextAlign = context->SetTextAlign(TA_LEFT | TA_TOP);
  auto oldTextColor = context->SetTextColor(App::ViewTextColor());
  auto oldBackgroundColor = context->SetBkColor(~App::ViewTextColor() & 0x00ffffff);

  CRect clientArea;
  view->GetClientRect(&clientArea);
  TEXTMETRIC metrics;
  context->GetTextMetrics(&metrics);

  CString length;

  int left = clientArea.right - 16 * metrics.tmAveCharWidth;

  CRect rc(left, clientArea.top, clientArea.right, clientArea.top + metrics.tmHeight);
  app.FormatLength(length, Units, position.x);
  length.TrimLeft();
  context->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, length, (UINT)length.GetLength(), 0);

  rc.SetRect(left, clientArea.top + 1 * metrics.tmHeight, clientArea.right, clientArea.top + 2 * metrics.tmHeight);
  app.FormatLength(length, Units, position.y);
  length.TrimLeft();
  context->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, length, (UINT)length.GetLength(), 0);

  rc.SetRect(left, clientArea.top + 2 * metrics.tmHeight, clientArea.right, clientArea.top + 3 * metrics.tmHeight);
  app.FormatLength(length, Units, position.z);
  length.TrimLeft();
  context->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, length, (UINT)length.GetLength(), 0);

  context->SetBkColor(oldBackgroundColor);
  context->SetTextColor(oldTextColor);
  context->SetTextAlign(oldTextAlign);
  context->SelectObject(oldFont);
}
#endif

}  // namespace

void AeSysView::OnDraw(CDC* deviceContext) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnDraw(%08.8lx) +", this, deviceContext);

  // Printing bypasses the back buffer — render directly to the printer DC
  if (deviceContext->IsPrinting()) {
    auto* document = GetDocument();
    assert(document != nullptr);
    EoGsRenderDeviceGdi renderDevice(deviceContext);
    document->DisplayAllLayers(this, &renderDevice);
    return;
  }

  // Direct2D rendering path — the HWND render target is inherently double-buffered
  if (m_useD2D && m_d2dRenderTarget) {
    if (m_sceneInvalid || m_overlayDirty) {
      m_d2dRenderTarget->BeginDraw();
      m_d2dRenderTarget->SetAntialiasMode(
          m_d2dAliased ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

      auto* document = GetDocument();
      assert(document != nullptr);
      const bool isPaperSpace = document->ActiveSpace() == EoDxf::Space::PaperSpace;
      const auto bgColor = Eo::ViewBackgroundColorForSpace(isPaperSpace);
      auto bkColor = D2D1::ColorF(
          GetRValue(bgColor) / 255.0f,
          GetGValue(bgColor) / 255.0f,
          GetBValue(bgColor) / 255.0f);
      m_d2dRenderTarget->Clear(bkColor);
      EoGsRenderDeviceDirect2D renderDevice(m_d2dRenderTarget.Get(), app.D2DFactory(), app.DWriteFactory());
      document->DisplayAllLayers(this, &renderDevice);
      document->DisplayUniquePoints();

      // Preview group overlay — rendered on top of the committed scene
      if (!m_PreviewGroup.IsEmpty()) {
        auto savedState = renderState.Save();
        m_PreviewGroup.Display(this, &renderDevice);
        renderState.Restore(&renderDevice, savedState);
      }

      // Draw rubberband overlay after the scene
      if (m_rubberbandType != None) {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> rubberbandBrush;
        const auto cr = Eo::RubberbandColor();
        auto rubberbandColor = D2D1::ColorF(
            (cr & 0xFF) / 255.0f, ((cr >> 8) & 0xFF) / 255.0f, ((cr >> 16) & 0xFF) / 255.0f);
        m_d2dRenderTarget->CreateSolidColorBrush(rubberbandColor, &rubberbandBrush);
        if (rubberbandBrush) {
          auto begin = D2D1::Point2F(
              static_cast<float>(m_rubberbandLogicalBegin.x), static_cast<float>(m_rubberbandLogicalBegin.y));
          auto end = D2D1::Point2F(
              static_cast<float>(m_rubberbandLogicalEnd.x), static_cast<float>(m_rubberbandLogicalEnd.y));
          if (m_rubberbandType == Lines) {
            m_d2dRenderTarget->DrawLine(begin, end, rubberbandBrush.Get(), 1.0f);
          } else if (m_rubberbandType == Rectangles) {
            auto rect = D2D1::RectF(begin.x, begin.y, end.x, end.y);
            m_d2dRenderTarget->DrawRectangle(rect, rubberbandBrush.Get(), 1.0f);
          }
        }
      }

      HRESULT hr = m_d2dRenderTarget->EndDraw();
      if (hr == D2DERR_RECREATE_TARGET) {
        DiscardD2DResources();
        InvalidateScene();
        return;
      }
      m_sceneInvalid = false;
      m_overlayDirty = false;
    }
    UpdateStateInformation(All);
    ModeLineDisplay();
    ValidateRect(nullptr);
    return;
  }

  CRect clipRect;
  deviceContext->GetClipBox(clipRect);
  ATLTRACE2(traceGeneral, 3, L" ClipBox(%i, %i, %i, %i)\n", clipRect.left, clipRect.top, clipRect.right, clipRect.bottom);

  if (clipRect.IsRectEmpty()) { return; }

  try {
    auto* document = GetDocument();
    assert(document != nullptr);

    // If back buffer exists and scene is dirty, re-render the entire scene into the back buffer
    if (m_backBufferDC.GetSafeHdc() != nullptr && m_sceneInvalid) {
      const bool isPaperSpace = document->ActiveSpace() == EoDxf::Space::PaperSpace;
      CRect bufferRect(0, 0, m_backBufferSize.cx, m_backBufferSize.cy);
      m_backBufferDC.FillSolidRect(bufferRect, Eo::ViewBackgroundColorForSpace(isPaperSpace));

      if (!m_ViewRendered) {
        BackgroundImageDisplay(&m_backBufferDC);
        DisplayGrid(&m_backBufferDC);
        EoGsRenderDeviceGdi renderDevice(&m_backBufferDC);
#if defined(USING_STATE_PATTERN)
        auto* state = GetCurrentState();
        if (state) {
          state->OnDraw(this, &m_backBufferDC);
        } else {
          document->DisplayAllLayers(this, &renderDevice);
          document->DisplayUniquePoints();
        }
#else
        document->DisplayAllLayers(this, &renderDevice);
        document->DisplayUniquePoints();
#endif
      }
      m_sceneInvalid = false;
    }

    // Blit back buffer to screen (or fall back to direct rendering if no back buffer yet)
    if (m_backBufferDC.GetSafeHdc() != nullptr) {
      deviceContext->BitBlt(clipRect.left, clipRect.top, clipRect.Width(), clipRect.Height(), &m_backBufferDC,
                            clipRect.left, clipRect.top, SRCCOPY);
    } else {
      // Fallback: direct rendering before first OnSize delivers a back buffer
      if (!m_ViewRendered) {
        BackgroundImageDisplay(deviceContext);
        DisplayGrid(deviceContext);
        EoGsRenderDeviceGdi renderDevice(deviceContext);
        document->DisplayAllLayers(this, &renderDevice);
        document->DisplayUniquePoints();
      }
    }

    // Preview group overlay — rendered on the screen DC, not into the back buffer,
    // so it does not require a full scene re-render on each mouse move.
    if (!m_PreviewGroup.IsEmpty()) {
      auto savedState = renderState.Save();
      EoGsRenderDeviceGdi overlayDevice(deviceContext);
      m_PreviewGroup.Display(this, &overlayDevice);
      renderState.Restore(&overlayDevice, savedState);
    }
    m_overlayDirty = false;

    UpdateStateInformation(All);
    ModeLineDisplay();
    ValidateRect(nullptr);
  } catch (CException* e) { e->Delete(); }
}

void AeSysView::OnInitialUpdate() {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnInitialUpdate()\n", this);

  SetClassLongPtr(GetSafeHwnd(), GCLP_HBRBACKGROUND, (LONG_PTR)::CreateSolidBrush(Eo::ViewBackgroundColor));

  CView::OnInitialUpdate();

  ApplyActiveViewport();

#if defined(USING_STATE_PATTERN)
  PushState(std::make_unique<IdleState>());
#endif
  OnModeDraw();
}

void AeSysView::ApplyActiveViewport() {
  auto* document = GetDocument();
  if (document == nullptr) { return; }

  const auto* activeVPort = document->FindActiveVPort();
  if (activeVPort == nullptr) { return; }

  // viewHeight must be positive for a meaningful viewport
  if (activeVPort->m_viewHeight <= Eo::geometricTolerance) { return; }

  // --- Camera setup ---
  // DXF VPORT viewTargetPoint is in WCS. viewDirection is target→camera (WCS).
  auto targetPoint = EoGePoint3d(
      activeVPort->m_viewTargetPoint.x, activeVPort->m_viewTargetPoint.y, activeVPort->m_viewTargetPoint.z);
  auto viewDirection = activeVPort->m_viewDirection;

  m_ViewTransform.SetLensLength(activeVPort->m_lensLength);
  m_ViewTransform.SetTarget(targetPoint);
  m_ViewTransform.SetPosition(viewDirection);  // SetPosition(vector) computes target + direction * lensLength
  m_ViewTransform.SetDirection(viewDirection);

  // View up: for standard top-down (direction ≈ +Z), up = +Y.
  // For general 3D views, compute up from twist angle.
  if (std::abs(activeVPort->m_viewTwistAngle) > Eo::geometricTolerance) {
    // Twist rotates the view around the view direction. Construct the up vector
    // by rotating +Y (or the appropriate perpendicular) by the twist angle around the view direction.
    auto n = viewDirection;
    n.Unitize();
    auto arbitraryUp = (std::abs(n.z) < 0.99) ? EoGeVector3d::positiveUnitZ : EoGeVector3d::positiveUnitY;
    auto u = CrossProduct(arbitraryUp, n);
    u.Unitize();
    auto v = CrossProduct(n, u);
    v.Unitize();
    // Rotate v by -twistAngle around n (DXF twist is CW when looking along the view direction)
    auto cosT = std::cos(-activeVPort->m_viewTwistAngle);
    auto sinT = std::sin(-activeVPort->m_viewTwistAngle);
    auto viewUp = EoGeVector3d(u.x * sinT + v.x * cosT, u.y * sinT + v.y * cosT, u.z * sinT + v.z * cosT);
    viewUp.Unitize();
    m_ViewTransform.SetViewUp(viewUp);
  } else {
    // No twist — use standard up vector based on view direction
    auto n = viewDirection;
    n.Unitize();
    if (std::abs(n.z) > 0.99) {
      // Top or bottom view — up is +Y
      m_ViewTransform.SetViewUp(n.z > 0.0 ? EoGeVector3d::positiveUnitY : EoGeVector3d::positiveUnitY);
    } else {
      // General 3D — up is derived from crossing direction with Z, then back
      auto u = CrossProduct(EoGeVector3d::positiveUnitZ, n);
      u.Unitize();
      auto viewUp = CrossProduct(n, u);
      viewUp.Unitize();
      m_ViewTransform.SetViewUp(viewUp);
    }
  }

  // --- Perspective mode ---
  // DXF VIEWMODE bit 0 = perspective on
  m_ViewTransform.EnablePerspective((activeVPort->m_viewMode & 1) != 0);

  // --- Clip planes ---
  m_ViewTransform.SetNearClipDistance(-100.0);
  m_ViewTransform.SetFarClipDistance(100.0);

  // --- Projection window ---
  // DXF viewHeight is the visible height in drawing units.
  // viewCenter (group 12/22) is the 2D offset from viewTargetPoint in DCS.
  // For a standard 2D top-down view, DCS X = WCS X, DCS Y = WCS Y.
  double viewHeight = activeVPort->m_viewHeight;
  double viewWidth = viewHeight * activeVPort->m_viewAspectRatio;

  // DCS view center offset from target
  double dcsOffsetX = activeVPort->m_viewCenter.x;
  double dcsOffsetY = activeVPort->m_viewCenter.y;

  // Adjust for viewport device aspect ratio (the projection window may need
  // expansion to fill the device viewport without distortion)
  m_ViewTransform.SetCenteredWindow(m_Viewport, viewWidth, viewHeight);

  // If viewCenter has a non-zero offset from target, shift the window
  if (std::abs(dcsOffsetX) > Eo::geometricTolerance || std::abs(dcsOffsetY) > Eo::geometricTolerance) {
    // viewCenter is the DCS location that should appear at the screen center.
    // The target is at DCS origin. So the camera target in WCS should be
    // shifted by the viewCenter offset (for standard 2D views).
    targetPoint.x += dcsOffsetX;
    targetPoint.y += dcsOffsetY;
    m_ViewTransform.SetTarget(targetPoint);
    m_ViewTransform.SetPosition(viewDirection);
  }

  m_ViewTransform.BuildTransformMatrix();
  m_OverviewViewTransform = m_ViewTransform;

  ATLTRACE2(traceGeneral, 1, L"AeSysView<%p>::ApplyActiveViewport() — "
      L"target=(%.2f, %.2f, %.2f) height=%.2f width=%.2f aspect=%.4f\n",
      this, targetPoint.x, targetPoint.y, targetPoint.z,
      viewHeight, viewWidth, activeVPort->m_viewAspectRatio);
}

/** @brief Helper function to display content based on the provided hint, used by OnUpdate for delegation
 * This centralizes the logic for interpreting hints and displaying the appropriate content, allowing OnUpdate to focus
 * on setup/delegation/cleanup
 * @param sender The view that sent the update notification
 * @param hint A bitmask hint indicating what type of content needs to be updated (e.g., primitive, group, layer)
 * @param hintObject The specific object related to the hint (e.g., the primitive or group that changed)
 * @param deviceContext The device context to use for drawing
 */
void AeSysView::DisplayUsingHint(CView* sender, LPARAM hint, CObject* hintObject, EoGsRenderDevice* renderDevice) {
  switch (hint) {
    case EoDb::kPrimitive:
    case EoDb::kPrimitiveSafe:
    case EoDb::kPrimitiveEraseSafe:
      static_cast<EoDbPrimitive*>(hintObject)->Display(this, renderDevice);
      break;

    case EoDb::kGroup:
    case EoDb::kGroupSafe:
    case EoDb::kGroupEraseSafe:
    case EoDb::kGroupSafeTrap:
    case EoDb::kGroupEraseSafeTrap:
      static_cast<EoDbGroup*>(hintObject)->Display(this, renderDevice);
      break;

    case EoDb::kGroups:
    case EoDb::kGroupsSafe:
    case EoDb::kGroupsSafeTrap:
    case EoDb::kGroupsEraseSafeTrap:
      static_cast<EoDbGroupList*>(hintObject)->Display(this, renderDevice);
      break;

    case EoDb::kLayer:
    case EoDb::kLayerErase:
      static_cast<EoDbLayer*>(hintObject)->Display(this, renderDevice);
      break;

    default:
      CView::OnUpdate(sender, hint, hintObject);
  }
}

void AeSysView::OnUpdate(CView* sender, LPARAM hint, CObject* hintObject) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnUpdate(%p, %p, %p)\n", this, sender, hint, hintObject);

  // Hint 0 means full scene refresh (e.g., model/paper space toggle, layer visibility change).
  // Mark scene dirty and let OnDraw re-render from scratch.
  if (hint == 0) {
    InvalidateScene();
    return;
  }

  // D2D path: incremental XOR-based updates are not supported — invalidate
  // the scene and let OnDraw do a full re-render (fast with D2D).
  if (m_useD2D) {
    InvalidateScene();
    return;
  }

  // Choose the target DC: back buffer if available, otherwise screen DC
  CDC* targetDC{};
  CDC* screenDC{};
  if (m_backBufferDC.GetSafeHdc() != nullptr) {
    targetDC = &m_backBufferDC;
  } else {
    screenDC = GetDC();
    targetDC = screenDC;
  }

  auto backgroundColor = targetDC->GetBkColor();
  const auto* updateDoc = GetDocument();
  const bool isPaperSpaceUpdate = updateDoc != nullptr && updateDoc->ActiveSpace() == EoDxf::Space::PaperSpace;
  targetDC->SetBkColor(Eo::ViewBackgroundColorForSpace(isPaperSpaceUpdate));
  int savedRenderState{};
  int drawMode{};
  if ((hint & EoDb::kSafe) == EoDb::kSafe) { savedRenderState = renderState.Save(); }
  if ((hint & EoDb::kErase) == EoDb::kErase) { drawMode = renderState.SetROP2(targetDC, R2_XORPEN); }
  if ((hint & EoDb::kTrap) == EoDb::kTrap) { EoDbPrimitive::SetSpecialColor(app.TrapHighlightColor()); }

  EoGsRenderDeviceGdi renderDevice(targetDC);
  bool isHandledByState{};
#if defined(USING_STATE_PATTERN)
  auto* state = GetCurrentState();
  if (state) { isHandledByState = state->OnUpdate(this, sender, hint, hintObject); }
#endif
  if (!isHandledByState) { DisplayUsingHint(sender, hint, hintObject, &renderDevice); }

  if ((hint & EoDb::kTrap) == EoDb::kTrap) { EoDbPrimitive::SetSpecialColor(0); }
  if ((hint & EoDb::kErase) == EoDb::kErase) { renderState.SetROP2(targetDC, drawMode); }
  if ((hint & EoDb::kSafe) == EoDb::kSafe) { renderState.Restore(targetDC, savedRenderState); }
  targetDC->SetBkColor(backgroundColor);

  if (screenDC != nullptr) {
    ReleaseDC(screenDC);
  } else {
    // Rendered into back buffer — blit the updated region to screen
    auto* dc = GetDC();
    if (dc != nullptr) {
      CRect clientRect;
      GetClientRect(clientRect);
      dc->BitBlt(0, 0, clientRect.Width(), clientRect.Height(), &m_backBufferDC, 0, 0, SRCCOPY);
      ReleaseDC(dc);
    }
  }
}

void AeSysView::OnBeginPrinting(CDC* deviceContext, CPrintInfo* pInfo) {
  ViewportPushActive();
  PushViewTransform();

  int HorizontalPixelWidth = deviceContext->GetDeviceCaps(HORZRES);
  int VerticalPixelWidth = deviceContext->GetDeviceCaps(VERTRES);

  SetViewportSize(HorizontalPixelWidth, VerticalPixelWidth);

  double HorizontalSize = static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE));
  double VerticalSize = static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE));

  SetDeviceWidthInInches(HorizontalSize / Eo::MmPerInch);
  SetDeviceHeightInInches(VerticalSize / Eo::MmPerInch);

  if (m_Plot) {
    UINT HorizontalPages;
    UINT VerticalPages;
    pInfo->SetMaxPage(NumPages(deviceContext, m_PlotScaleFactor, HorizontalPages, VerticalPages));
  } else {
    m_ViewTransform.AdjustWindow(static_cast<double>(VerticalPixelWidth) / static_cast<double>(HorizontalPixelWidth));
  }
}

void AeSysView::OnEndPrinting([[maybe_unused]] CDC* deviceContext, [[maybe_unused]] CPrintInfo* printInformation) {
  PopViewTransform();
  ViewportPopActive();
}

BOOL AeSysView::OnPreparePrinting(CPrintInfo* pInfo) {
  if (m_Plot) {
    CPrintInfo pi;
    if (AfxGetApp()->GetPrinterDeviceDefaults(&pi.m_pPD->m_pd)) {
      HDC hDC = pi.m_pPD->m_pd.hDC;
      if (hDC == nullptr) { hDC = pi.m_pPD->CreatePrinterDC(); }
      if (hDC != nullptr) {
        UINT nHorzPages;
        UINT nVertPages;
        CDC DeviceContext;
        DeviceContext.Attach(hDC);
        pInfo->SetMaxPage(NumPages(&DeviceContext, m_PlotScaleFactor, nHorzPages, nVertPages));
        ::DeleteDC(DeviceContext.Detach());
      }
    }
  }
  return DoPreparePrinting(pInfo);
}

void AeSysView::OnPrepareDC(CDC* deviceContext, CPrintInfo* pInfo) {
  CView::OnPrepareDC(deviceContext, pInfo);

  if (deviceContext->IsPrinting()) {
    if (m_Plot) {
      double HorizontalSizeInInches =
          static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch / m_PlotScaleFactor;
      double VerticalSizeInInches =
          static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch / m_PlotScaleFactor;

      m_ViewTransform.Initialize(m_Viewport);
      m_ViewTransform.SetWindow(0.0, 0.0, HorizontalSizeInInches, VerticalSizeInInches);

      UINT nHorzPages;
      UINT nVertPages;

      NumPages(deviceContext, m_PlotScaleFactor, nHorzPages, nVertPages);

      double dX = ((pInfo->m_nCurPage - 1) % nHorzPages) * HorizontalSizeInInches;
      double dY = ((pInfo->m_nCurPage - 1) / nHorzPages) * VerticalSizeInInches;

      m_ViewTransform.SetTarget(EoGePoint3d(dX, dY, 0.0));
      m_ViewTransform.SetPosition(EoGeVector3d::positiveUnitZ);
      m_ViewTransform.BuildTransformMatrix();
    } else {
    }
  }
}

// Window messages ////////////////////////////////////////////////////////////
void AeSysView::OnContextMenu(CWnd*, CPoint point) { app.ShowPopupMenu(IDR_CONTEXT_MENU, point, this); }

BOOL AeSysView::OnEraseBkgnd([[maybe_unused]] CDC* deviceContext) {
  // Suppress GDI background erase — the off-screen back buffer covers the entire client area via BitBlt.
  return TRUE;
}

void AeSysView::RecreateBackBuffer(int width, int height) {
  if (width <= 0 || height <= 0) { return; }

  auto* screenDC = GetDC();
  if (screenDC == nullptr) { return; }

  // Delete previous back buffer resources
  if (m_backBufferDC.GetSafeHdc() != nullptr) {
    m_backBufferDC.SelectObject(static_cast<CBitmap*>(nullptr));
    m_backBuffer.DeleteObject();
    m_backBufferDC.DeleteDC();
  }

  m_backBufferDC.CreateCompatibleDC(screenDC);
  m_backBuffer.CreateCompatibleBitmap(screenDC, width, height);
  m_backBufferDC.SelectObject(&m_backBuffer);
  m_backBufferSize.SetSize(width, height);
  m_sceneInvalid = true;

  ReleaseDC(screenDC);
}

void AeSysView::InvalidateScene() {
  m_sceneInvalid = true;
  InvalidateRect(nullptr, FALSE);
}

void AeSysView::InvalidateOverlay() {
  m_overlayDirty = true;
  InvalidateRect(nullptr, FALSE);
}

void AeSysView::CreateD2DRenderTarget() {
  if (m_d2dRenderTarget) { return; }  // already created

  auto* factory = app.D2DFactory();
  if (factory == nullptr) { return; }

  CRect clientRect;
  GetClientRect(&clientRect);
  if (clientRect.IsRectEmpty()) { return; }

  D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT32>(clientRect.Width()), static_cast<UINT32>(clientRect.Height()));

  D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
  rtProps.dpiX = 96.0f;
  rtProps.dpiY = 96.0f;
  D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(GetSafeHwnd(), size);

  HRESULT hr = factory->CreateHwndRenderTarget(rtProps, hwndProps, &m_d2dRenderTarget);
  if (FAILED(hr)) {
    ATLTRACE2(traceGeneral, 0, L"AeSysView<%p>::CreateD2DRenderTarget() FAILED hr=0x%08X\n", this, hr);
    m_d2dRenderTarget.Reset();
  }
}

void AeSysView::DiscardD2DResources() {
  m_d2dRenderTarget.Reset();
}

void AeSysView::OnSize(UINT type, int cx, int cy) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>OnSize(%i, %i, %i)\n", this, type, cx, cy);

  if (cx && cy) {
    double oldWidth = m_Viewport.Width();
    double oldHeight = m_Viewport.Height();

    SetViewportSize(cx, cy);

    if (oldWidth > 0.0 && oldHeight > 0.0) {
      // Preserve current view center and zoom level — scale the window extents
      // proportionally to the viewport dimension change (world units per pixel stays constant)
      double scaleX = static_cast<double>(cx) / oldWidth;
      double scaleY = static_cast<double>(cy) / oldHeight;

      double centerU = (m_ViewTransform.UMin() + m_ViewTransform.UMax()) / 2.0;
      double centerV = (m_ViewTransform.VMin() + m_ViewTransform.VMax()) / 2.0;
      double halfUExtent = m_ViewTransform.UExtent() / 2.0 * scaleX;
      double halfVExtent = m_ViewTransform.VExtent() / 2.0 * scaleY;

      m_ViewTransform.SetWindow(
          centerU - halfUExtent, centerV - halfVExtent, centerU + halfUExtent, centerV + halfVExtent);
    } else {
      // First OnSize — no prior viewport dimensions, initialize to default view
      m_ViewTransform.Initialize(m_Viewport);
    }
    m_OverviewViewTransform = m_ViewTransform;

    if (m_useD2D) {
      if (!m_d2dRenderTarget) { CreateD2DRenderTarget(); }
      if (m_d2dRenderTarget) {
        D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT32>(cx), static_cast<UINT32>(cy));
        HRESULT hr = m_d2dRenderTarget->Resize(size);
        if (FAILED(hr)) {
          DiscardD2DResources();
        }
        m_sceneInvalid = true;
      } else {
        // D2D creation failed — fall back to GDI
        m_useD2D = false;
        RecreateBackBuffer(cx, cy);
      }
    } else {
      RecreateBackBuffer(cx, cy);
    }
  }
}

void AeSysView::OnTimer(UINT_PTR nIDEvent) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnTimer(%i)\n", this, nIDEvent);

  CView::OnTimer(nIDEvent);
}

void AeSysView::BackgroundImageDisplay(CDC* deviceContext) {
  if (!m_viewBackgroundImage || (static_cast<HBITMAP>(m_backgroundImageBitmap) == 0)) { return; }

  int iWidDst = int(m_Viewport.Width());
  int iHgtDst = int(m_Viewport.Height());

  BITMAP bm{};
  m_backgroundImageBitmap.GetBitmap(&bm);
  CDC dcMem;
  dcMem.CreateCompatibleDC(nullptr);
  CBitmap* pBitmap = dcMem.SelectObject(&m_backgroundImageBitmap);
  CPalette* pPalette = deviceContext->SelectPalette(&m_backgroundImagePalette, FALSE);
  deviceContext->RealizePalette();

  auto Target = m_ViewTransform.Target();
  auto ptTargetOver = m_OverviewViewTransform.Target();
  double dU = Target.x - ptTargetOver.x;
  double dV = Target.y - ptTargetOver.y;

  // Determine the region of the bitmap to tranfer to display
  CRect rcWnd;
  rcWnd.left =
      Eo::Round((m_ViewTransform.UMin() - OverviewUMin() + dU) / OverviewUExt() * static_cast<double>(bm.bmWidth));
  rcWnd.top = Eo::Round(
      (1.0 - (m_ViewTransform.VMax() - OverviewVMin() + dV) / OverviewVExt()) * static_cast<double>(bm.bmHeight));
  rcWnd.right =
      Eo::Round((m_ViewTransform.UMax() - OverviewUMin() + dU) / OverviewUExt() * static_cast<double>(bm.bmWidth));
  rcWnd.bottom = Eo::Round(
      (1.0 - (m_ViewTransform.VMin() - OverviewVMin() + dV) / OverviewVExt()) * static_cast<double>(bm.bmHeight));

  int iWidSrc = rcWnd.Width();
  int iHgtSrc = rcWnd.Height();

  deviceContext->StretchBlt(0, 0, iWidDst, iHgtDst, &dcMem, (int)rcWnd.left, (int)rcWnd.top, iWidSrc, iHgtSrc, SRCCOPY);

  dcMem.SelectObject(pBitmap);
  deviceContext->SelectPalette(pPalette, FALSE);
}

void AeSysView::ViewportPopActive() {
  if (!m_Viewports.IsEmpty()) { m_Viewport = m_Viewports.RemoveTail(); }
}

void AeSysView::ViewportPushActive() { m_Viewports.AddTail(m_Viewport); }
// AeSysView printing

void AeSysView::DisplayPixel(CDC* deviceContext, COLORREF cr, const EoGePoint3d& point) {
  EoGePoint4d ndcPoint(point);

  ModelViewTransformPoint(ndcPoint);

  if (ndcPoint.IsInView()) { deviceContext->SetPixel(ProjectToClient(ndcPoint), cr); }
}

void AeSysView::DisplayOdometer() {
  auto cursorPosition = GetCursorPosition();

  m_vRelPos = cursorPosition - GridOrign();

  if (m_ViewOdometer) {
    auto units = app.GetUnits();

    CString lengthText;

    app.FormatLength(lengthText, units, m_vRelPos.x);
    CString Position = lengthText.TrimLeft();
    app.FormatLength(lengthText, units, m_vRelPos.y);
    Position.Append(L", " + lengthText.TrimLeft());
    app.FormatLength(lengthText, units, m_vRelPos.z);
    Position.Append(L", " + lengthText.TrimLeft());

    if (m_rubberbandType == Lines) {
      EoGeLine line(m_rubberbandBegin, cursorPosition);

      auto lineLength = line.Length();
      auto angleInXYPlane = line.AngleFromXAxisXY();
      app.FormatLength(lengthText, units, lineLength);

      CString angle;
      app.FormatAngle(angle, angleInXYPlane, 8, 3);
      angle.ReleaseBuffer();
      Position.Append(L" [" + lengthText.TrimLeft() + L" @ " + angle + L"]");
    }
    auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());
    mainFrame->SetPaneText(1, Position);
#if defined(LEGACY_ODOMETER)
    DrawOdometerInView(this, GetDC(), units, m_vRelPos);
#endif
  }
#if defined(USING_DDE)
  dde::PostAdvise(dde::RelPosXInfo);
  dde::PostAdvise(dde::RelPosYInfo);
  dde::PostAdvise(dde::RelPosZInfo);
#endif
}

void AeSysView::UpdateStateInformation(EStateInformationItem item) {
  if (!m_ViewStateInformation) { return; }

  auto* document = AeSysDoc::GetDoc();
  auto* deviceContext = GetDC();

  auto oldFont = deviceContext->SelectStockObject(DEFAULT_GUI_FONT);
  auto oldTextAlign = deviceContext->SetTextAlign(TA_LEFT | TA_TOP);
  auto oldTextColor = deviceContext->SetTextColor(App::ViewTextColor());
  auto oldBkColor = deviceContext->SetBkColor(~App::ViewTextColor() & 0x00ffffff);

  TEXTMETRIC textMetric{};
  deviceContext->GetTextMetrics(&textMetric);
  auto averageCharacterWidth = textMetric.tmAveCharWidth;
  auto height = textMetric.tmHeight;

  CRect clientRect{};
  GetClientRect(&clientRect);
  auto top = clientRect.top;

  CRect rectangle{};
  constexpr UINT options = ETO_CLIPPED | ETO_OPAQUE;
  wchar_t szBuf[32]{};

  if ((item & WorkCount) == WorkCount) {
    rectangle.SetRect(0, top, 8 * averageCharacterWidth, top + height);
    swprintf_s(szBuf, 32, L"%-4i", document->NumberOfGroupsInWorkLayer() + document->NumberOfGroupsInActiveLayers());
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & TrapCount) == TrapCount) {
    rectangle.SetRect(8 * averageCharacterWidth, top, 16 * averageCharacterWidth, top + height);
    long trapCount = static_cast<long>(document->TrapGroupCount());
    swprintf_s(szBuf, 32, L"%-4ld", trapCount);
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & Pen) == Pen) {
    rectangle.SetRect(16 * averageCharacterWidth, top, 22 * averageCharacterWidth, top + height);
    swprintf_s(szBuf, 32, L"P%-4i", renderState.Color());
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & Line) == Line) {
    rectangle.SetRect(22 * averageCharacterWidth, top, 28 * averageCharacterWidth, top + height);
    swprintf_s(szBuf, 32, L"L%-4i", renderState.LineTypeIndex());
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & TextHeight) == TextHeight) {
    auto characterCellDefinition = renderState.CharacterCellDefinition();
    rectangle.SetRect(28 * averageCharacterWidth, top, 38 * averageCharacterWidth, top + height);
    swprintf_s(szBuf, 32, L"T%-6.2f", characterCellDefinition.Height());
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & Scale) == Scale) {
    rectangle.SetRect(38 * averageCharacterWidth, top, 48 * averageCharacterWidth, top + height);
    swprintf_s(szBuf, 32, L"1:%-6.2f", GetWorldScale());
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & WndRatio) == WndRatio) {
    rectangle.SetRect(48 * averageCharacterWidth, top, 58 * averageCharacterWidth, top + height);
    double Ratio = WidthInInches() / UExtent();
    CString RatioAsString;
    RatioAsString.Format(L"=%-8.3f", Ratio);
    deviceContext->ExtTextOutW(rectangle.left, rectangle.top, options, &rectangle, RatioAsString,
        static_cast<UINT>(RatioAsString.GetLength()), 0);
  }
  if ((item & DimLen) == DimLen || (item & DimAng) == DimAng) {
    rectangle.SetRect(58 * averageCharacterWidth, top, 90 * averageCharacterWidth, top + height);
    CString LengthAndAngle;
    app.FormatLength(LengthAndAngle, app.GetUnits(), app.DimensionLength());
    LengthAndAngle.TrimLeft();
    CString Angle;
    app.FormatAngle(Angle, Eo::DegreeToRadian(app.DimensionAngle()), 8, 3);
    Angle.ReleaseBuffer();
    LengthAndAngle.Append(L" @ " + Angle);
    deviceContext->ExtTextOutW(rectangle.left, rectangle.top, options, &rectangle, LengthAndAngle,
        static_cast<UINT>(LengthAndAngle.GetLength()), 0);
  }
  deviceContext->SetBkColor(oldBkColor);
  deviceContext->SetTextColor(oldTextColor);
  deviceContext->SetTextAlign(oldTextAlign);
  deviceContext->SelectObject(oldFont);
  ReleaseDC(deviceContext);
}
