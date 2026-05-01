#include "Stdafx.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>

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
#include "EoGeVector3d.h"
#include "EoGsRenderDeviceDirect2D.h"
#include "EoGsRenderDeviceGdi.h"
#include "EoGsRenderState.h"
#include "EoGsViewTransform.h"
#include "MainFrm.h"

#include "AeSysState.h"

#ifdef USING_DDE
#include "Dde.h"
#include "DdeGItms.h"
#endif

void AeSysView::OnDraw(CDC* deviceContext) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnDraw(%08.8lx) +", this, deviceContext);

  // Suppress all rendering during document teardown (async WM_CLOSE)
  if (const auto* doc = GetDocument(); doc != nullptr && doc->IsClosing()) {
    ValidateRect(nullptr);
    return;
  }

  // Printing bypasses the back buffer — render directly to the printer DC
  if (deviceContext->IsPrinting()) {
    auto* document = GetDocument();
    assert(document != nullptr);

    // Printer paper is white — force ACI 7 to black and ACI 0 to white so that
    // entities using the default color (including COLOR_BYBLOCK → 7) are visible.
    // Without this, the dark-scheme palette has ACI 7 = white → invisible on paper.
    const COLORREF savedAci0 = Eo::ColorPalette[0];
    const COLORREF savedAci7 = Eo::ColorPalette[7];
    const COLORREF savedGray0 = Eo::GrayPalette[0];
    const COLORREF savedGray7 = Eo::GrayPalette[7];
    Eo::ColorPalette[7] = Eo::colorBlack;
    Eo::ColorPalette[0] = Eo::colorWhite;
    Eo::GrayPalette[7] = RGB(0x22, 0x22, 0x22);
    Eo::GrayPalette[0] = RGB(0xdd, 0xdd, 0xdd);

    // D2D DC render target for anti-aliased hard copy output
    bool renderedWithD2D{};
    auto* factory = app.D2DFactory();
    if (m_useD2DForPrint && factory != nullptr) {
      const auto dpiX = static_cast<float>(deviceContext->GetDeviceCaps(LOGPIXELSX));
      const auto dpiY = static_cast<float>(deviceContext->GetDeviceCaps(LOGPIXELSY));
      const int horzRes = deviceContext->GetDeviceCaps(HORZRES);
      const int vertRes = deviceContext->GetDeviceCaps(VERTRES);

      const D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
          D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
          dpiX,
          dpiY);

      Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> dcTarget;
      HRESULT hr = factory->CreateDCRenderTarget(&rtProps, &dcTarget);
      if (SUCCEEDED(hr)) {
        const RECT bindRect{0, 0, horzRes, vertRes};
        hr = dcTarget->BindDC(deviceContext->GetSafeHdc(), &bindRect);
      }
      if (SUCCEEDED(hr)) {
        dcTarget->BeginDraw();
        dcTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        EoGsRenderDeviceDirect2D renderDevice(dcTarget.Get(), factory, app.DWriteFactory());
        renderDevice.SetPenWidthScale(dpiX / 96.0f);
        document->DisplayAllLayers(this, &renderDevice);

        hr = dcTarget->EndDraw();
        renderedWithD2D = SUCCEEDED(hr);
      }
      if (!renderedWithD2D) {
        ATLTRACE2(
            traceGeneral, 0, L"AeSysView<%p>::OnDraw — D2D print failed hr=0x%08X, falling back to GDI\n", this, hr);
      }
    }

    // GDI fallback if D2D is disabled or failed
    if (!renderedWithD2D) {
      EoGsRenderDeviceGdi renderDevice(deviceContext);
      document->DisplayAllLayers(this, &renderDevice);
    }

    Eo::ColorPalette[0] = savedAci0;
    Eo::ColorPalette[7] = savedAci7;
    Eo::GrayPalette[0] = savedGray0;
    Eo::GrayPalette[7] = savedGray7;
    return;
  }

  // Direct2D rendering path — the HWND render target is inherently double-buffered.
  // If m_useD2D is set but the target was not yet created (e.g. early OnSize before
  // the HWND was presentable), attempt creation now.
  if (m_useD2D && !m_d2dRenderTarget) { CreateD2DRenderTarget(); }
  if (m_useD2D && m_d2dRenderTarget) {
    if (m_sceneInvalid || m_overlayDirty) {
      auto* document = GetDocument();
      assert(document != nullptr);
      const bool isPaperSpace = document->ActiveSpace() == EoDxf::Space::PaperSpace;
      const bool isBlockEdit = document->IsEditingBlock();
      const bool needsWhiteBackground = isPaperSpace && !isBlockEdit;
      const auto bgColor = isBlockEdit ? Eo::BlockEditBackgroundColor() : Eo::ViewBackgroundColorForSpace(isPaperSpace);
      const auto bkColor =
          D2D1::ColorF(GetRValue(bgColor) / 255.0f, GetGValue(bgColor) / 255.0f, GetBValue(bgColor) / 255.0f);

      // White background (paper space) — force ACI 7→black so entities using the
      // default color are visible on the white sheet, matching print-path behavior.
      COLORREF savedAci0{}, savedAci7{}, savedGray0{}, savedGray7{};
      if (needsWhiteBackground) {
        savedAci0 = Eo::ColorPalette[0];
        savedAci7 = Eo::ColorPalette[7];
        savedGray0 = Eo::GrayPalette[0];
        savedGray7 = Eo::GrayPalette[7];
        Eo::ColorPalette[7] = Eo::colorBlack;
        Eo::ColorPalette[0] = Eo::colorWhite;
        Eo::GrayPalette[7] = RGB(0x22, 0x22, 0x22);
        Eo::GrayPalette[0] = RGB(0xdd, 0xdd, 0xdd);
      }

      // Save the user's current render state (linetype name, color, etc.) before entity
      // rendering, which mutates Gs::renderState via SetPen calls. Restore afterward so that
      // UpdateStateInformation reads the user's selection, not the last-rendered entity's state.
      const auto savedUserState = Gs::renderState.Save();

      // (Re)create the cached scene target if missing or sized to a stale viewport.
      if (m_sceneInvalid) {
        if (!m_d2dSceneTarget) { CreateD2DSceneTarget(); }
        if (m_d2dSceneTarget) {
          m_d2dSceneTarget->BeginDraw();
          m_d2dSceneTarget->SetAntialiasMode(
              m_d2dAliased ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
          const auto sceneClip =
              D2D1::RectF(0.0f, 0.0f, static_cast<float>(m_Viewport.Width()), static_cast<float>(m_Viewport.Height()));
          m_d2dSceneTarget->PushAxisAlignedClip(sceneClip, D2D1_ANTIALIAS_MODE_ALIASED);
          m_d2dSceneTarget->Clear(bkColor);
          EoGsRenderDeviceDirect2D sceneDevice(m_d2dSceneTarget.Get(), app.D2DFactory(), app.DWriteFactory());
          document->DisplayAllLayers(this, &sceneDevice);
          m_d2dSceneTarget->PopAxisAlignedClip();
          const HRESULT hrScene = m_d2dSceneTarget->EndDraw();
          if (hrScene == D2DERR_RECREATE_TARGET) {
            DiscardD2DResources();
            InvalidateScene();
            return;
          }
        }
      }

      // Composite cached scene + transient overlays onto the HWND render target.
      m_d2dRenderTarget->BeginDraw();
      m_d2dRenderTarget->SetAntialiasMode(
          m_d2dAliased ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

      const auto drawingClipRect =
          D2D1::RectF(0.0f, 0.0f, static_cast<float>(m_Viewport.Width()), static_cast<float>(m_Viewport.Height()));
      m_d2dRenderTarget->PushAxisAlignedClip(drawingClipRect, D2D1_ANTIALIAS_MODE_ALIASED);

      if (m_d2dSceneTarget) {
        Microsoft::WRL::ComPtr<ID2D1Bitmap> sceneBitmap;
        if (SUCCEEDED(m_d2dSceneTarget->GetBitmap(&sceneBitmap)) && sceneBitmap) {
          m_d2dRenderTarget->DrawBitmap(sceneBitmap.Get(), drawingClipRect, 1.0f,
              D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, drawingClipRect);
        } else {
          m_d2dRenderTarget->Clear(bkColor);
        }
      } else {
        // Scene target unavailable — fall back to direct render so the screen is not blank.
        m_d2dRenderTarget->Clear(bkColor);
        EoGsRenderDeviceDirect2D fallbackDevice(m_d2dRenderTarget.Get(), app.D2DFactory(), app.DWriteFactory());
        document->DisplayAllLayers(this, &fallbackDevice);
      }

      EoGsRenderDeviceDirect2D renderDevice(m_d2dRenderTarget.Get(), app.D2DFactory(), app.DWriteFactory());
      document->RenderUniquePoints(this, &renderDevice);

      // Preview group overlay — rendered on top of the committed scene.
      // When a viewport is active, the preview points are in model-space coordinates
      // (because GetCursorPosition routes through the viewport transform). Push the
      // viewport's model-space transform so the preview renders in the correct position.
      if (!m_PreviewGroup.IsEmpty()) {
        if (IsViewportActive() && ConfigureViewportTransform(m_activeViewportPrimitive)) {
          m_PreviewGroup.Display(this, &renderDevice);
          RestoreViewportTransform();
        } else {
          m_PreviewGroup.Display(this, &renderDevice);
        }
      }
      Gs::renderState.Restore(&renderDevice, savedUserState);

      if (needsWhiteBackground) {
        Eo::ColorPalette[0] = savedAci0;
        Eo::ColorPalette[7] = savedAci7;
        Eo::GrayPalette[0] = savedGray0;
        Eo::GrayPalette[7] = savedGray7;
      }

      // Draw rubberband overlay after the scene
      if (m_rubberbandType != None) {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> rubberbandBrush;
        const auto cr = Eo::RubberbandColor();
        const auto rubberbandColor =
            D2D1::ColorF((cr & 0xFF) / 255.0f, ((cr >> 8) & 0xFF) / 255.0f, ((cr >> 16) & 0xFF) / 255.0f);
        m_d2dRenderTarget->CreateSolidColorBrush(rubberbandColor, &rubberbandBrush);
        if (rubberbandBrush) {
          const auto begin = D2D1::Point2F(
              static_cast<float>(m_rubberbandLogicalBegin.x), static_cast<float>(m_rubberbandLogicalBegin.y));
          const auto end =
              D2D1::Point2F(static_cast<float>(m_rubberbandLogicalEnd.x), static_cast<float>(m_rubberbandLogicalEnd.y));
          if (m_rubberbandType == Lines) {
            m_d2dRenderTarget->DrawLine(begin, end, rubberbandBrush.Get(), 1.0f);
          } else if (m_rubberbandType == Rectangles || m_rubberbandType == RectanglesRemove ||
                     m_rubberbandType == RectanglesWindow || m_rubberbandType == RectanglesWindowRemove) {
            const auto rect = D2D1::RectF(
                std::min(begin.x, end.x), std::min(begin.y, end.y),
                std::max(begin.x, end.x), std::max(begin.y, end.y));
            // Fill color: green for crossing-add, blue for window-add,
            //             same hues (same opacity) for remove variants.
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fillBrush;
            if (m_rubberbandType == RectanglesWindow) {
              m_d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.47f, 1.0f, 0.18f), &fillBrush);
            } else if (m_rubberbandType == RectanglesWindowRemove) {
              m_d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.47f, 1.0f, 0.18f), &fillBrush);
            } else if (m_rubberbandType == RectanglesRemove) {
              m_d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.6f, 0.2f, 0.18f), &fillBrush);
            } else {
              m_d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.6f, 0.2f, 0.18f), &fillBrush);
            }
            if (fillBrush) { m_d2dRenderTarget->FillRectangle(rect, fillBrush.Get()); }
            // Border: dashed for window variants, solid for crossing variants.
            const bool isDashed = (m_rubberbandType == RectanglesWindow || m_rubberbandType == RectanglesWindowRemove);
            if (isDashed) {
              Microsoft::WRL::ComPtr<ID2D1Factory> factory;
              m_d2dRenderTarget->GetFactory(&factory);
              const float dashes[] = {5.0f, 3.0f};
              D2D1_STROKE_STYLE_PROPERTIES dashProps = D2D1::StrokeStyleProperties();
              dashProps.dashStyle = D2D1_DASH_STYLE_CUSTOM;
              Microsoft::WRL::ComPtr<ID2D1StrokeStyle> dashStyle;
              factory->CreateStrokeStyle(dashProps, dashes, 2, &dashStyle);
              m_d2dRenderTarget->DrawRectangle(rect, rubberbandBrush.Get(), 1.0f, dashStyle.Get());
            } else {
              m_d2dRenderTarget->DrawRectangle(rect, rubberbandBrush.Get(), 1.0f);
            }
          }
        }
      }

      m_d2dRenderTarget->PopAxisAlignedClip();

      const HRESULT hr = m_d2dRenderTarget->EndDraw();
      if (hr == D2DERR_RECREATE_TARGET) {
        DiscardD2DResources();
        InvalidateScene();
        return;
      }
      m_sceneInvalid = false;
      m_overlayDirty = false;
    }
    {
      const auto* doc = GetDocument();
      if (doc != nullptr && !doc->IsClosing()) {
        UpdateStateInformation(All);
        ModeLineDisplay();
      }
    }
    ValidateRect(nullptr);
    return;
  }

  CRect clipRect;
  deviceContext->GetClipBox(clipRect);
  ATLTRACE2(
      traceGeneral, 3, L" ClipBox(%i, %i, %i, %i)\n", clipRect.left, clipRect.top, clipRect.right, clipRect.bottom);

  if (clipRect.IsRectEmpty()) { return; }

  try {
    auto* document = GetDocument();
    assert(document != nullptr);

    // Save the user's current render state before entity rendering, which mutates
    // Gs::renderState via SetPen calls. Restore afterward so that UpdateStateInformation
    // reads the user's selection, not the last-rendered entity's state.
    const auto savedUserState = Gs::renderState.Save();

    // If back buffer exists and scene is dirty, re-render the entire scene into the back buffer
    if (m_backBufferDC.GetSafeHdc() != nullptr && m_sceneInvalid) {
      const bool isPaperSpace = document->ActiveSpace() == EoDxf::Space::PaperSpace;
      const bool isBlockEdit = document->IsEditingBlock();
      const bool needsWhiteBackground = isPaperSpace && !isBlockEdit;
      CRect bufferRect(0, 0, m_backBufferSize.cx, m_backBufferSize.cy);
      const auto gdiBgColor =
          isBlockEdit ? Eo::BlockEditBackgroundColor() : Eo::ViewBackgroundColorForSpace(isPaperSpace);
      m_backBufferDC.FillSolidRect(bufferRect, gdiBgColor);

      // White background (paper space) — force ACI 7→black so entities using the
      // default color are visible on the white sheet, matching print-path behavior.
      COLORREF savedAci0{}, savedAci7{}, savedGray0{}, savedGray7{};
      if (needsWhiteBackground) {
        savedAci0 = Eo::ColorPalette[0];
        savedAci7 = Eo::ColorPalette[7];
        savedGray0 = Eo::GrayPalette[0];
        savedGray7 = Eo::GrayPalette[7];
        Eo::ColorPalette[7] = Eo::colorBlack;
        Eo::ColorPalette[0] = Eo::colorWhite;
        Eo::GrayPalette[7] = RGB(0x22, 0x22, 0x22);
        Eo::GrayPalette[0] = RGB(0xdd, 0xdd, 0xdd);
      }

      if (!m_ViewRendered) {
        BackgroundImageDisplay(&m_backBufferDC);
        DisplayGrid(&m_backBufferDC);
        EoGsRenderDeviceGdi renderDevice(&m_backBufferDC);
        document->DisplayAllLayers(this, &renderDevice);
      }

      if (needsWhiteBackground) {
        Eo::ColorPalette[0] = savedAci0;
        Eo::ColorPalette[7] = savedAci7;
        Eo::GrayPalette[0] = savedGray0;
        Eo::GrayPalette[7] = savedGray7;
      }
      m_sceneInvalid = false;
    }

    // Blit back buffer to screen (or fall back to direct rendering if no back buffer yet)
    if (m_backBufferDC.GetSafeHdc() != nullptr) {
      // Recompose the overlay buffer when overlays or scene changed.
      // Otherwise the cached overlay buffer is reused — simple expose events
      // (uncover, focus changes) become a single BitBlt to the screen with no
      // GDI primitive calls and no overlay redraw.
      if (m_overlayDirty && m_overlayDC.GetSafeHdc() != nullptr) {
        m_overlayDC.BitBlt(0, 0, m_backBufferSize.cx, m_backBufferSize.cy, &m_backBufferDC, 0, 0, SRCCOPY);

        // Draw transient overlays into the overlay buffer.
        // When a viewport is active, push the viewport's model-space transform so
        // model-space markers/preview render in the correct position.
        EoGsRenderDeviceGdi overlayDevice(&m_overlayDC);
        const bool viewportActive = IsViewportActive() && ConfigureViewportTransform(m_activeViewportPrimitive);

        document->RenderUniquePoints(this, &overlayDevice);

        if (!m_PreviewGroup.IsEmpty()) {
          m_PreviewGroup.Display(this, &overlayDevice);
        }

        // Rubberband — GDI path draws into the overlay buffer instead of XOR-on-screen.
        // This eliminates XOR color artifacts and keeps the rubberband on the same clean
        // compose path used by the D2D target.
        if (m_rubberbandType != None) {
          if (m_rubberbandType == Lines) {
            CPen linePen(PS_SOLID, 0, Eo::RubberbandColor());
            auto* prevPen = m_overlayDC.SelectObject(&linePen);
            m_overlayDC.MoveTo(m_rubberbandLogicalBegin);
            m_overlayDC.LineTo(m_rubberbandLogicalEnd);
            m_overlayDC.SelectObject(prevPen);
          } else if (m_rubberbandType == Rectangles || m_rubberbandType == RectanglesRemove ||
                     m_rubberbandType == RectanglesWindow || m_rubberbandType == RectanglesWindowRemove) {
            // Dashed border for window variants (left-to-right); solid for crossing variants.
            const bool isDashed = (m_rubberbandType == RectanglesWindow || m_rubberbandType == RectanglesWindowRemove);
            CPen rectPen(isDashed ? PS_DASH : PS_SOLID, 0, Eo::RubberbandColor());
            auto* prevPen = m_overlayDC.SelectObject(&rectPen);
            auto* prevBrush = m_overlayDC.SelectStockObject(NULL_BRUSH);
            m_overlayDC.Rectangle(m_rubberbandLogicalBegin.x, m_rubberbandLogicalBegin.y,
                m_rubberbandLogicalEnd.x, m_rubberbandLogicalEnd.y);
            m_overlayDC.SelectObject(prevBrush);
            m_overlayDC.SelectObject(prevPen);
          }
        }

        // State overlay: mode-specific overlays (preview geometry, selection hints).
        auto* state = GetCurrentState();
        if (state != nullptr) { state->OnDraw(this, &m_overlayDC); }

        if (viewportActive) { RestoreViewportTransform(); }
      }

      // Blit overlay (or back buffer if overlay not yet allocated) to screen.
      auto* sourceDC = m_overlayDC.GetSafeHdc() != nullptr ? &m_overlayDC : &m_backBufferDC;
      deviceContext->BitBlt(clipRect.left,
          clipRect.top,
          clipRect.Width(),
          clipRect.Height(),
          sourceDC,
          clipRect.left,
          clipRect.top,
          SRCCOPY);
    } else {
      // Fallback: direct rendering before first OnSize delivers a back buffer
      if (!m_ViewRendered) {
        const bool isPaper = document->ActiveSpace() == EoDxf::Space::PaperSpace;
        const bool isBlockEd = document->IsEditingBlock();
        const bool needsWhiteBg = isPaper && !isBlockEd;
        COLORREF sAci0{}, sAci7{}, sGray0{}, sGray7{};
        if (needsWhiteBg) {
          sAci0 = Eo::ColorPalette[0];
          sAci7 = Eo::ColorPalette[7];
          sGray0 = Eo::GrayPalette[0];
          sGray7 = Eo::GrayPalette[7];
          Eo::ColorPalette[7] = Eo::colorBlack;
          Eo::ColorPalette[0] = Eo::colorWhite;
          Eo::GrayPalette[7] = RGB(0x22, 0x22, 0x22);
          Eo::GrayPalette[0] = RGB(0xdd, 0xdd, 0xdd);
        }
        BackgroundImageDisplay(deviceContext);
        DisplayGrid(deviceContext);
        EoGsRenderDeviceGdi renderDevice(deviceContext);
        document->DisplayAllLayers(this, &renderDevice);
        document->RenderUniquePoints(this, &renderDevice);
        if (needsWhiteBg) {
          Eo::ColorPalette[0] = sAci0;
          Eo::ColorPalette[7] = sAci7;
          Eo::GrayPalette[0] = sGray0;
          Eo::GrayPalette[7] = sGray7;
        }
      }
    }

    Gs::renderState.Restore(deviceContext, savedUserState);
    m_overlayDirty = false;

    {
      const auto* doc = AeSysDoc::GetDoc();
      if (doc != nullptr && !doc->IsClosing()) {
        UpdateStateInformation(All);
        ModeLineDisplay();
      }
    }
    ValidateRect(nullptr);
  } catch (CException* e) { e->Delete(); }
}

void AeSysView::OnInitialUpdate() {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnInitialUpdate()\n", this);

  SetClassLongPtr(GetSafeHwnd(), GCLP_HBRBACKGROUND, (LONG_PTR)::CreateSolidBrush(Eo::ViewBackgroundColor));

  CView::OnInitialUpdate();

  // CView::OnInitialUpdate → OnUpdate(0) → UpdateLayoutTabs populates the layout
  // tab bar. Tab population changes GetTabsHeight() (0 before tabs, full strip height
  // after), which changes PreferredHeight(). Force a resize so the viewport dimensions
  // and back buffer match the actual tab bar height before building the view transform.
  {
    CRect rc;
    GetClientRect(&rc);
    if (rc.Width() > 0 && rc.Height() > 0) { OnSize(SIZE_RESTORED, rc.Width(), rc.Height()); }
  }

  ApplyActiveViewport();

  // Ensure PEG files (and any document without paper-space viewports) get a default
  // layout viewport so the plot pipeline can use the same paper-space rendering path.
  if (auto* document = GetDocument()) { document->CreateDefaultPaperSpaceViewport(this); }

  OnModeDraw();

  // If the document entered an editor mode during OnOpenDocument (before the view existed),
  // apply the tab bar editor state now that the view and tab bar are ready.
  if (auto* doc = GetDocument()) {
    if (doc->IsEditingTracing()) {
      const std::filesystem::path filePath(static_cast<const wchar_t*>(doc->EditingTracingPath()));
      const CString tracingName = filePath.stem().wstring().c_str();
      LayoutTabBar().UpdateBlockEditState(true, tracingName, L"TRACING");

      // MFC's SetPathName overrides our SetTitle from EnterTracingEditMode,
      // so re-apply the editor title now that the framework is done.
      CString editTitle;
      editTitle.Format(L"[|%s]", tracingName.GetString());
      doc->SetTitle(editTitle);

      ModelViewInitialize();
      OnWindowBest();
    }
  }
}

void AeSysView::ApplyActiveViewport() {
  const auto* document = GetDocument();
  if (document == nullptr) { return; }

  const auto* activeVPort = document->FindActiveVPort();
  if (activeVPort == nullptr) { return; }

  // viewHeight must be positive for a meaningful viewport
  if (activeVPort->m_viewHeight <= Eo::geometricTolerance) { return; }

  // --- Camera setup ---
  // DXF VPORT viewTargetPoint is in WCS. viewDirection is target→camera (WCS).
  auto targetPoint =
      EoGePoint3d(activeVPort->m_viewTargetPoint.x, activeVPort->m_viewTargetPoint.y, activeVPort->m_viewTargetPoint.z);
  const auto viewDirection = activeVPort->m_viewDirection;

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
    const auto arbitraryUp = (std::abs(n.z) < 0.99) ? EoGeVector3d::positiveUnitZ : EoGeVector3d::positiveUnitY;
    auto u = CrossProduct(arbitraryUp, n);
    u.Unitize();
    auto v = CrossProduct(n, u);
    v.Unitize();
    // Rotate v by -twistAngle around n (DXF twist is CW when looking along the view direction)
    const auto cosT = std::cos(-activeVPort->m_viewTwistAngle);
    const auto sinT = std::sin(-activeVPort->m_viewTwistAngle);
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
  const double viewHeight = activeVPort->m_viewHeight;
  const double viewWidth = viewHeight * activeVPort->m_viewAspectRatio;

  // DCS view center offset from target
  const double dcsOffsetX = activeVPort->m_viewCenter.x;
  const double dcsOffsetY = activeVPort->m_viewCenter.y;

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

  ATLTRACE2(traceGeneral,
      1,
      L"AeSysView<%p>::ApplyActiveViewport() — "
      L"target=(%.2f, %.2f, %.2f) height=%.2f width=%.2f aspect=%.4f\n",
      this,
      targetPoint.x,
      targetPoint.y,
      targetPoint.z,
      viewHeight,
      viewWidth,
      activeVPort->m_viewAspectRatio);
}

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
    // Sync layout tab selection with the document's current active space/layout
    UpdateLayoutTabs();
    InvalidateScene();
    return;
  }

  // D2D path: incremental XOR-based updates are not supported — invalidate
  // the scene and let OnDraw do a full re-render (fast with D2D).
  if (m_useD2D) {
    InvalidateScene();
    return;
  }

  // GDI path: XOR erasure (R2_XORPEN) produces black artifacts on non-black
  // backgrounds (e.g., dark theme charcoal RGB(40,40,36)). XOR only erases
  // correctly when color XOR color = 0 = background, which requires black.
  // Invalidate the scene to trigger a full re-render instead.
  if ((hint & EoDb::kErase) == EoDb::kErase) {
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

  const auto backgroundColor = targetDC->GetBkColor();
  const auto* updateDoc = GetDocument();
  const bool isPaperSpaceUpdate = updateDoc != nullptr && updateDoc->ActiveSpace() == EoDxf::Space::PaperSpace;
  targetDC->SetBkColor(Eo::ViewBackgroundColorForSpace(isPaperSpaceUpdate));
  int savedRenderState{};
  if ((hint & EoDb::kSafe) == EoDb::kSafe) { savedRenderState = Gs::renderState.Save(); }
  if ((hint & EoDb::kTrap) == EoDb::kTrap) { EoDbPrimitive::SetSpecialColor(app.TrapHighlightColor()); }

  EoGsRenderDeviceGdi renderDevice(targetDC);
  bool isHandledByState{};
  auto* state = GetCurrentState();
  if (state != nullptr) { isHandledByState = state->OnUpdate(this, sender, hint, hintObject); }
  if (!isHandledByState) { DisplayUsingHint(sender, hint, hintObject, &renderDevice); }

  if ((hint & EoDb::kTrap) == EoDb::kTrap) { EoDbPrimitive::SetSpecialColor(0); }
  if ((hint & EoDb::kSafe) == EoDb::kSafe) { Gs::renderState.Restore(targetDC, savedRenderState); }
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

  const int horizontalPixelWidth = deviceContext->GetDeviceCaps(HORZRES);
  const int verticalPixelWidth = deviceContext->GetDeviceCaps(VERTRES);

  SetViewportSize(horizontalPixelWidth, verticalPixelWidth);

  const auto horizontalSize = static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE));
  const auto verticalSize = static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE));

  SetDeviceWidthInInches(horizontalSize / Eo::MmPerInch);
  SetDeviceHeightInInches(verticalSize / Eo::MmPerInch);

  if (m_Plot) {
    // When fit-to-paper is active, compute the scale that maps the entire
    // drawing onto a single page of the actual printer paper.
    if (m_plotSettings.fitToPaper) {
      auto* document = GetDocument();
      EoGePoint3d ptMin;
      EoGePoint3d ptMax;
      const EoGeTransformMatrix transformMatrix;
      document->GetExtents(this, ptMin, ptMax, transformMatrix);

      const double extentWidth = ptMax.x - ptMin.x;
      const double extentHeight = ptMax.y - ptMin.y;

      const double paperWidthInches = horizontalSize / Eo::MmPerInch;
      const double paperHeightInches = verticalSize / Eo::MmPerInch;
      if (extentWidth > Eo::geometricTolerance && extentHeight > Eo::geometricTolerance
          && paperWidthInches > Eo::geometricTolerance && paperHeightInches > Eo::geometricTolerance) {
        m_PlotScaleFactor = std::min(paperWidthInches / extentWidth, paperHeightInches / extentHeight);
      } else {
        m_PlotScaleFactor = 1.0;
      }
    }

    UINT horizontalPages;
    UINT verticalPages;
    pInfo->SetMaxPage(NumPages(deviceContext, m_PlotScaleFactor, horizontalPages, verticalPages));
  } else {
    m_ViewTransform.AdjustWindow(static_cast<double>(verticalPixelWidth) / static_cast<double>(horizontalPixelWidth));
  }
}

void AeSysView::OnEndPrinting([[maybe_unused]] CDC* deviceContext, [[maybe_unused]] CPrintInfo* printInformation) {
  PopViewTransform();
  ViewportPopActive();
  InvalidateScene();
}

BOOL AeSysView::OnPreparePrinting(CPrintInfo* pInfo) {
  if (m_Plot) {
    // The Plot dialog already collected printer/paper/scale settings and
    // OnFilePlot applied them to the app's DEVMODE via SelectPrinter.
    // Skip the Windows Print dialog — print directly.
    pInfo->m_bDirect = TRUE;

    // Estimate page count for the framework.  When fit-to-paper is active
    // this uses the temporary m_PlotScaleFactor (1.0) — OnBeginPrinting
    // recomputes the actual scale once the real printer DC is available.
    PRINTDLG pd{};
    pd.lStructSize = sizeof(pd);
    if (AfxGetApp()->GetPrinterDeviceDefaults(&pd)) {
      HDC hDC{};
      if (pd.hDevNames != nullptr && pd.hDevMode != nullptr) {
        const auto* devNames = static_cast<DEVNAMES*>(::GlobalLock(pd.hDevNames));
        const auto* devMode = static_cast<DEVMODE*>(::GlobalLock(pd.hDevMode));
        if (devNames != nullptr && devMode != nullptr) {
          auto* driverName = reinterpret_cast<const wchar_t*>(devNames) + devNames->wDriverOffset;
          auto* deviceName = reinterpret_cast<const wchar_t*>(devNames) + devNames->wDeviceOffset;
          auto* outputName = reinterpret_cast<const wchar_t*>(devNames) + devNames->wOutputOffset;
          hDC = ::CreateDCW(driverName, deviceName, outputName, devMode);
        }
        if (pd.hDevMode != nullptr) { ::GlobalUnlock(pd.hDevMode); }
        if (pd.hDevNames != nullptr) { ::GlobalUnlock(pd.hDevNames); }
      }
      if (hDC != nullptr) {
        UINT nHorzPages;
        UINT nVertPages;
        CDC deviceContext;
        deviceContext.Attach(hDC);
        pInfo->SetMaxPage(NumPages(&deviceContext, m_PlotScaleFactor, nHorzPages, nVertPages));
        ::DeleteDC(deviceContext.Detach());
      }
    }
  }
  return DoPreparePrinting(pInfo);
}

void AeSysView::OnPrepareDC(CDC* deviceContext, CPrintInfo* pInfo) {
  CView::OnPrepareDC(deviceContext, pInfo);

  if (deviceContext->IsPrinting()) {
    if (m_Plot) {
      const double horizontalSizeInInches =
          static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch / m_PlotScaleFactor;
      const double verticalSizeInInches =
          static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch / m_PlotScaleFactor;

      m_ViewTransform.Initialize(m_Viewport);
      m_ViewTransform.SetWindow(0.0, 0.0, horizontalSizeInInches, verticalSizeInInches);

      UINT nHorzPages;
      UINT nVertPages;

      NumPages(deviceContext, m_PlotScaleFactor, nHorzPages, nVertPages);

      const double dX = ((pInfo->m_nCurPage - 1) % nHorzPages) * horizontalSizeInInches;
      const double dY = ((pInfo->m_nCurPage - 1) / nHorzPages) * verticalSizeInInches;

      m_ViewTransform.SetTarget(EoGePoint3d(dX, dY, 0.0));
      m_ViewTransform.SetPosition(EoGeVector3d::positiveUnitZ);
      m_ViewTransform.BuildTransformMatrix();
    } else {
    }
  }
}

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
  if (m_overlayDC.GetSafeHdc() != nullptr) {
    m_overlayDC.SelectObject(static_cast<CBitmap*>(nullptr));
    m_overlayBuffer.DeleteObject();
    m_overlayDC.DeleteDC();
  }

  m_backBufferDC.CreateCompatibleDC(screenDC);
  m_backBuffer.CreateCompatibleBitmap(screenDC, width, height);
  m_backBufferDC.SelectObject(&m_backBuffer);

  m_overlayDC.CreateCompatibleDC(screenDC);
  m_overlayBuffer.CreateCompatibleBitmap(screenDC, width, height);
  m_overlayDC.SelectObject(&m_overlayBuffer);

  m_backBufferSize.SetSize(width, height);
  m_sceneInvalid = true;
  m_overlayDirty = true;

  ReleaseDC(screenDC);
}

void AeSysView::InvalidateScene() {
  m_sceneInvalid = true;
  m_overlayDirty = true;  // overlay must be recomposited from refreshed scene
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

  const D2D1_SIZE_U size =
      D2D1::SizeU(static_cast<UINT32>(clientRect.Width()), static_cast<UINT32>(clientRect.Height()));

  D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
  rtProps.dpiX = 96.0f;
  rtProps.dpiY = 96.0f;
  const D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(GetSafeHwnd(), size);

  const HRESULT hr = factory->CreateHwndRenderTarget(rtProps, hwndProps, &m_d2dRenderTarget);
  if (FAILED(hr)) {
    ATLTRACE2(traceGeneral, 0, L"AeSysView<%p>::CreateD2DRenderTarget() FAILED hr=0x%08X\n", this, hr);
    m_d2dRenderTarget.Reset();
  }
}

void AeSysView::DiscardD2DResources() {
  m_d2dSceneTarget.Reset();
  m_d2dRenderTarget.Reset();
}

void AeSysView::CreateD2DSceneTarget() {
  if (m_d2dSceneTarget) { return; }
  if (!m_d2dRenderTarget) { return; }

  const double width = m_Viewport.Width();
  const double height = m_Viewport.Height();
  if (width <= 0.0 || height <= 0.0) { return; }

  const D2D1_SIZE_F sizeF = D2D1::SizeF(static_cast<float>(width), static_cast<float>(height));
  const HRESULT hr = m_d2dRenderTarget->CreateCompatibleRenderTarget(sizeF, &m_d2dSceneTarget);
  if (FAILED(hr)) {
    ATLTRACE2(traceGeneral, 0, L"AeSysView<%p>::CreateD2DSceneTarget() FAILED hr=0x%08X\n", this, hr);
    m_d2dSceneTarget.Reset();
  }
}

void AeSysView::OnContextMenu(CWnd*, CPoint point) {
  app.ShowPopupMenu(IDR_CONTEXT_MENU, point, this);
}

void AeSysView::OnSize(UINT type, int cx, int cy) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>OnSize(%i, %i, %i)\n", this, type, cx, cy);

  if (cx && cy) {
    // Position the layout tab bar at the bottom of the client area
    int tabBarHeight = 0;
    if (m_layoutTabBar.GetSafeHwnd() != nullptr) {
      tabBarHeight = m_layoutTabBar.PreferredHeight();
      m_layoutTabBar.MoveWindow(0, cy - tabBarHeight, cx, tabBarHeight);
      m_layoutTabBar.RepositionControls();
    }

    // Drawing area excludes the tab bar
    int drawingHeight = cy - tabBarHeight;
    drawingHeight = std::max(drawingHeight, 1);

    const double oldWidth = m_Viewport.Width();
    const double oldHeight = m_Viewport.Height();

    SetViewportSize(cx, drawingHeight);

    if (oldWidth > 0.0 && oldHeight > 0.0) {
      // Preserve current view center and zoom level — scale the window extents
      // proportionally to the viewport dimension change (world units per pixel stays constant)
      const double scaleX = static_cast<double>(cx) / oldWidth;
      const double scaleY = static_cast<double>(drawingHeight) / oldHeight;

      const double centerU = (m_ViewTransform.UMin() + m_ViewTransform.UMax()) / 2.0;
      const double centerV = (m_ViewTransform.VMin() + m_ViewTransform.VMax()) / 2.0;
      const double halfUExtent = m_ViewTransform.UExtent() / 2.0 * scaleX;
      const double halfVExtent = m_ViewTransform.VExtent() / 2.0 * scaleY;

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
        // Size the D2D render target to the full HWND client area (cx × cy) so that
        // D2D does not scale the output.  A clip rect in OnDraw constrains rendering
        // to the drawing area, leaving the tab-bar region untouched.
        const D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT32>(cx), static_cast<UINT32>(cy));
        const HRESULT hr = m_d2dRenderTarget->Resize(size);
        if (FAILED(hr)) { DiscardD2DResources(); }
        // Scene cache must be re-created at the new viewport size on next OnDraw.
        m_d2dSceneTarget.Reset();
        m_sceneInvalid = true;
      } else {
        // D2D creation failed — could be early OnSize before HWND is presentable.
        // Fall back to GDI for now; OnDraw will retry D2D when the target is needed.
        RecreateBackBuffer(cx, drawingHeight);
      }
    } else {
      RecreateBackBuffer(cx, drawingHeight);
    }
  }
}

void AeSysView::OnTimer(UINT_PTR nIDEvent) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnTimer(%i)\n", this, nIDEvent);

  if (nIDEvent == kHoverTimerId) {
    KillTimer(kHoverTimerId);

    // Hit-test at the current cursor position.
    const auto worldPoint = GetCursorPosition();
    auto* group = SelectGroupAndPrimitive(worldPoint);
    auto* primitive = (group != nullptr) ? m_EngagedPrimitive : nullptr;

    if (primitive != nullptr) {
      // Build popup content from FormatExtra (Name;Value\t pairs) + FormatGeometry.
      CString extra;
      primitive->FormatExtra(extra);
      CString geometry;
      primitive->FormatGeometry(geometry);
      if (!geometry.IsEmpty()) { extra += L'\t' + geometry; }

      if (!extra.IsEmpty()) {
        CPoint cursorScreen;
        ::GetCursorPos(&cursorScreen);
        m_hoverTooltip.Show(extra, cursorScreen);
      }
    } else {
      // No hit — dismiss any active popup.
      m_hoverTooltip.Hide();
    }
    return;
  }

  CView::OnTimer(nIDEvent);
}

void AeSysView::BackgroundImageDisplay(CDC* deviceContext) {
  if (!m_viewBackgroundImage || (static_cast<HBITMAP>(m_backgroundImageBitmap) == nullptr)) { return; }

  const int iWidDst = static_cast<int>(m_Viewport.Width());
  const int iHgtDst = static_cast<int>(m_Viewport.Height());

  BITMAP bm{};
  m_backgroundImageBitmap.GetBitmap(&bm);
  CDC dcMem;
  dcMem.CreateCompatibleDC(nullptr);
  CBitmap* pBitmap = dcMem.SelectObject(&m_backgroundImageBitmap);
  CPalette* pPalette = deviceContext->SelectPalette(&m_backgroundImagePalette, FALSE);
  deviceContext->RealizePalette();

  const auto target = m_ViewTransform.Target();
  const auto ptTargetOver = m_OverviewViewTransform.Target();
  const double dU = target.x - ptTargetOver.x;
  const double dV = target.y - ptTargetOver.y;

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

  const int iWidSrc = rcWnd.Width();
  const int iHgtSrc = rcWnd.Height();

  deviceContext->StretchBlt(
      0, 0, iWidDst, iHgtDst, &dcMem, static_cast<int>(rcWnd.left), (int)rcWnd.top, iWidSrc, iHgtSrc, SRCCOPY);

  dcMem.SelectObject(pBitmap);
  deviceContext->SelectPalette(pPalette, FALSE);
}

void AeSysView::ViewportPopActive() {
  if (!m_Viewports.IsEmpty()) { m_Viewport = m_Viewports.RemoveTail(); }
}

void AeSysView::ViewportPushActive() {
  m_Viewports.AddTail(m_Viewport);
}

void AeSysView::DisplayPixel(CDC* deviceContext, COLORREF cr, const EoGePoint3d& point) {
  EoGePoint4d ndcPoint(point);

  ModelViewTransformPoint(ndcPoint);

  if (ndcPoint.IsInView()) { deviceContext->SetPixel(ProjectToClient(ndcPoint), cr); }
}

void AeSysView::DisplayOdometer() {
  const auto cursorPosition = GetCursorPosition();

  m_vRelPos = cursorPosition - GridOrign();

  const auto units = app.GetUnits();
  CString lengthText;

  app.FormatLength(lengthText, units, m_vRelPos.x);
  CString position = lengthText.TrimLeft();
  app.FormatLength(lengthText, units, m_vRelPos.y);
  position.Append(L", " + lengthText.TrimLeft());
  app.FormatLength(lengthText, units, m_vRelPos.z);
  position.Append(L", " + lengthText.TrimLeft());

  if (m_rubberbandType == Lines) {
    const EoGeLine line(m_rubberbandBegin, cursorPosition);

    const auto lineLength = line.Length();
    const auto angleInXYPlane = line.AngleFromXAxisXY();
    app.FormatLength(lengthText, units, lineLength);

    CString angle;
    app.FormatAngle(angle, angleInXYPlane, 8, 3);
    position.Append(L" [" + lengthText.TrimLeft() + L" @ " + angle + L"]");
  }
  if (auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd())) { mainFrame->SetPaneText(0, position); }

#ifdef USING_DDE
  dde::PostAdvise(dde::RelPosXInfo);
  dde::PostAdvise(dde::RelPosYInfo);
  dde::PostAdvise(dde::RelPosZInfo);
#endif
}

void AeSysView::UpdateStateInformation(EStateInformationItem item) {
  auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());
  if (mainFrame == nullptr) { return; }

  // Suppress state updates during document teardown
  if (const auto* doc = GetDocument(); doc != nullptr && doc->IsClosing()) { return; }

  if ((item & BothCounts) != 0) { mainFrame->GetPropertiesPane().UpdateDocumentStatistics(); }

  if ((item & Pen) == Pen) {
    mainFrame->SyncColorCombo(Gs::renderState.Color());
    mainFrame->SyncLineWeightCombo(Gs::renderState.LineWeight());
  }
  if ((item & Line) == Line) {
    mainFrame->SyncLineTypeCombo(Gs::renderState.LineTypeIndex(), Gs::renderState.LineTypeName());
  }
  if ((item & TextHeight) == TextHeight) { mainFrame->SyncTextStyleCombo(Gs::renderState.TextStyleName()); }
  if ((item & Scale) == Scale) {
    CString scaleText;
    scaleText.Format(L"1:%.2f", GetWorldScale());
    mainFrame->SetPaneText(13, scaleText);
  }
  if ((item & WndRatio) == WndRatio) {
    const double widthInInches = WidthInInches();
    const double uExtent = UExtent();
    CString zoomText{L"---"};
    if (widthInInches > Eo::geometricTolerance && uExtent > Eo::geometricTolerance) {
      zoomText.Format(L"%.6f", widthInInches / uExtent);
    }
    mainFrame->SetPaneText(14, zoomText);
  }
  if ((item & DimAng) == DimAng) {
    CString angle;
    app.FormatAngle(angle, Eo::DegreeToRadian(app.DimensionAngle()), 8, 3);
    mainFrame->SetPaneText(2, angle);
  }
  if ((item & Layer) == Layer) {
    const auto* document = GetDocument();
    if (document != nullptr && !document->IsClosing() && document->GetWorkLayer() != nullptr) {
      mainFrame->SyncLayerCombo(document->GetWorkLayer()->Name());
    }
  }
  if ((item & DimLen) == DimLen) {
    CString lengthText;
    app.FormatLength(lengthText, app.GetUnits(), app.DimensionLength());
    lengthText.TrimLeft();
    mainFrame->SetPaneText(1, lengthText);
  }
}
