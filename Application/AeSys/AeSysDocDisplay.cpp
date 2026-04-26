#include "Stdafx.h"

#include <algorithm>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbLayer.h"
#include "EoDbPolygon.h"
#include "EoDbViewport.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeVector3d.h"
#include "EoGsRenderDevice.h"
#include "EoGsRenderDeviceDirect2D.h"
#include "EoGsRenderState.h"

void AeSysDoc::DisplayAllLayers(AeSysView* view, EoGsRenderDevice* renderDevice) {
  ATLTRACE2(traceGeneral, 3, L"AeSysDoc<%p>::DisplayAllLayers(%p, %p)\n", this, view, renderDevice);

  try {
    // Editor mode: render only the editing layer
    if (IsInEditor()) {
      EoDbLayer* editLayer = IsEditingBlock() ? m_blockEditLayer : m_tracingEditLayer;
      if (editLayer != nullptr) {
        const bool identifyTrap = app.IsTrapHighlighted() && !IsTrapEmpty();

        RemoveAllGroupsFromAllViews();
        EoDbPolygon::SetSpecialPolygonStyle(
            view->RenderAsWireframe() ? EoDb::PolygonStyle::Hollow : EoDb::PolygonStyle::Special);
        const int savedRenderState = Gs::renderState.Save();
        editLayer->Display(view, renderDevice, identifyTrap);
        Gs::renderState.Restore(renderDevice, savedRenderState);
        EoDbPolygon::SetSpecialPolygonStyle(EoDb::PolygonStyle::Special);
        return;
      }
    }

    const bool identifyTrap = app.IsTrapHighlighted() && !IsTrapEmpty();

    RemoveAllGroupsFromAllViews();

    const bool isPaperSpace = m_activeSpace == EoDxf::Space::PaperSpace;
    const auto backgroundColor = renderDevice->SetBkColor(Eo::ViewBackgroundColorForSpace(isPaperSpace));

    // Draw the paper-space sheet background and viewport outlines before entity content
    if (isPaperSpace) { DisplayPaperSpaceSheet(view, renderDevice); }

    EoDbPolygon::SetSpecialPolygonStyle(
        view->RenderAsWireframe() ? EoDb::PolygonStyle::Hollow : EoDb::PolygonStyle::Special);
    const int savedRenderState = Gs::renderState.Save();

    for (int i = 0; i < GetLayerTableSize(); i++) {
      auto* layer = GetLayerTableLayerAt(i);
      layer->Display(view, renderDevice, identifyTrap);
    }

    // When in paper space, render model-space entities through each viewport
    if (m_activeSpace == EoDxf::Space::PaperSpace) { DisplayModelSpaceThroughViewports(view, renderDevice); }

    // When a viewport is active, dim everything except the active viewport area
    // so the active viewport's model-space content stands out at full brightness.
    // Must be after DisplayModelSpaceThroughViewports so non-active viewport content is dimmed too.
    if (m_activeSpace == EoDxf::Space::PaperSpace) { DimPaperSpaceOverlay(view, renderDevice); }

    Gs::renderState.Restore(renderDevice, savedRenderState);
    EoDbPolygon::SetSpecialPolygonStyle(EoDb::PolygonStyle::Special);

    renderDevice->SetBkColor(backgroundColor);
  } catch (CException* e) { e->Delete(); }
}

void AeSysDoc::DisplayPaperSpaceSheet(AeSysView* view, EoGsRenderDevice* renderDevice) {
  auto& paperLayers = PaperSpaceLayers();

  // Collect all viewport primitives and compute the overall sheet bounding box
  // from the layout extents or the union of viewport boundaries.
  double sheetMinX = (std::numeric_limits<double>::max)();
  double sheetMinY = (std::numeric_limits<double>::max)();
  double sheetMaxX = (std::numeric_limits<double>::lowest)();
  double sheetMaxY = (std::numeric_limits<double>::lowest)();

  // Try to derive the sheet rectangle from the active EoDxfLayout extents
  bool sheetFromLayout = false;
  for (const auto& layout : m_layouts) {
    if (layout.m_blockRecordHandle == m_activeLayoutHandle && !layout.IsModelLayout()) {
      // Use layout limits (DXF codes 10/20, 11/21) which represent the printable paper area
      if (layout.m_limmaxX - layout.m_limminX > Eo::geometricTolerance &&
          layout.m_limmaxY - layout.m_limminY > Eo::geometricTolerance) {
        sheetMinX = layout.m_limminX;
        sheetMinY = layout.m_limminY;
        sheetMaxX = layout.m_limmaxX;
        sheetMaxY = layout.m_limmaxY;
        sheetFromLayout = true;
      }
      break;
    }
  }

  // Gather viewport primitives for boundary rendering and fallback sheet bounds
  struct ViewportInfo {
    EoGePoint3d center;
    double halfWidth;
    double halfHeight;
    EoDbViewport* primitive;
  };
  std::vector<ViewportInfo> viewports;

  for (INT_PTR layerIndex = 0; layerIndex < paperLayers.GetSize(); layerIndex++) {
    auto* layer = paperLayers.GetAt(layerIndex);
    if (layer == nullptr) { continue; }

    auto position = layer->GetHeadPosition();
    while (position != nullptr) {
      auto* group = layer->GetNext(position);
      if (group == nullptr) { continue; }

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != nullptr) {
        auto* primitive = group->GetNext(primitivePosition);
        if (primitive == nullptr || !primitive->Is(EoDb::kViewportPrimitive)) { continue; }

        auto* viewport = static_cast<EoDbViewport*>(primitive);
        if (viewport->ViewportId() < 2) { continue; }  // Skip overall paper viewport (id 1)
        if (viewport->Width() < Eo::geometricTolerance || viewport->Height() < Eo::geometricTolerance) { continue; }

        const auto& center = viewport->CenterPoint();
        const double halfW = viewport->Width() / 2.0;
        const double halfH = viewport->Height() / 2.0;
        viewports.push_back({center, halfW, halfH, viewport});

        // If no layout extents, accumulate viewport bounds for the sheet rectangle
        if (!sheetFromLayout) {
          sheetMinX = (std::min)(sheetMinX, center.x - halfW);
          sheetMinY = (std::min)(sheetMinY, center.y - halfH);
          sheetMaxX = (std::max)(sheetMaxX, center.x + halfW);
          sheetMaxY = (std::max)(sheetMaxY, center.y + halfH);
        }
      }
    }
  }

  // If no sheet bounds could be determined, nothing to draw
  if (sheetMaxX - sheetMinX < Eo::geometricTolerance || sheetMaxY - sheetMinY < Eo::geometricTolerance) { return; }

  // Add a 2% margin around viewport-derived sheet bounds for visual separation
  if (!sheetFromLayout) {
    const double marginX = (sheetMaxX - sheetMinX) * 0.02;
    const double marginY = (sheetMaxY - sheetMinY) * 0.02;
    sheetMinX -= marginX;
    sheetMinY -= marginY;
    sheetMaxX += marginX;
    sheetMaxY += marginY;
  }

  // Project sheet corners to device coordinates
  auto projectToDevice = [&](double wx, double wy) -> CPoint {
    EoGePoint4d ndcPoint(EoGePoint3d{wx, wy, 0.0});
    view->ModelViewTransformPoint(ndcPoint);
    return view->ProjectToClient(ndcPoint);
  };

  // --- Draw the white sheet with drop shadow on the gray table ---
  constexpr COLORREF sheetFillColor = RGB(255, 255, 255);  // White paper sheet
  constexpr COLORREF sheetBorderColor = RGB(100, 99, 94);  // Dark warm gray border
  constexpr COLORREF shadowColor = RGB(64, 63, 58);  // Drop shadow behind sheet
  constexpr COLORREF viewportBorderColor = RGB(140, 140, 134);  // Viewport outline gray

  CPoint sheetCorners[4] = {
      projectToDevice(sheetMinX, sheetMinY),
      projectToDevice(sheetMaxX, sheetMinY),
      projectToDevice(sheetMaxX, sheetMaxY),
      projectToDevice(sheetMinX, sheetMaxY),
  };

  // Drop shadow — offset +4px right, +4px down from the sheet
  constexpr int shadowOffset = 4;
  CPoint shadowCorners[4] = {
      {sheetCorners[0].x + shadowOffset, sheetCorners[0].y + shadowOffset},
      {sheetCorners[1].x + shadowOffset, sheetCorners[1].y + shadowOffset},
      {sheetCorners[2].x + shadowOffset, sheetCorners[2].y + shadowOffset},
      {sheetCorners[3].x + shadowOffset, sheetCorners[3].y + shadowOffset},
  };
  renderDevice->SelectSolidBrush(shadowColor);
  renderDevice->SelectPen(PS_SOLID, 1, shadowColor);
  renderDevice->Polygon(shadowCorners, 4);
  renderDevice->RestorePen();
  renderDevice->RestoreBrush();

  // White sheet on top of shadow
  renderDevice->SelectSolidBrush(sheetFillColor);
  renderDevice->SelectPen(PS_SOLID, 1, sheetBorderColor);
  renderDevice->Polygon(sheetCorners, 4);
  renderDevice->RestorePen();
  renderDevice->RestoreBrush();

  // --- Draw viewport boundary outlines ---
  // Active viewport gets an accent blue border; inactive viewports get the default gray border.
  const auto* activeViewport = view->ActiveViewportPrimitive();
  renderDevice->SelectNullBrush();

  for (const auto& vp : viewports) {
    const bool isActive = (vp.primitive == activeViewport) && (activeViewport != nullptr);
    renderDevice->SelectPen(PS_SOLID, isActive ? 2 : 1,
        isActive ? Eo::chromeColors.captionActiveBackground : viewportBorderColor);

    CPoint vpCorners[5] = {
        projectToDevice(vp.center.x - vp.halfWidth, vp.center.y - vp.halfHeight),
        projectToDevice(vp.center.x + vp.halfWidth, vp.center.y - vp.halfHeight),
        projectToDevice(vp.center.x + vp.halfWidth, vp.center.y + vp.halfHeight),
        projectToDevice(vp.center.x - vp.halfWidth, vp.center.y + vp.halfHeight),
        projectToDevice(vp.center.x - vp.halfWidth, vp.center.y - vp.halfHeight),  // Close the outline
    };
    renderDevice->Polyline(vpCorners, 5);
    renderDevice->RestorePen();
  }

  renderDevice->RestoreBrush();
}

void AeSysDoc::DimPaperSpaceOverlay(AeSysView* view, EoGsRenderDevice* renderDevice) {
  if (view == nullptr || !view->IsViewportActive()) { return; }

  // Helper: project a viewport paper-space corner to device coordinates
  auto projectViewportCorner = [&](double wx, double wy) -> CPoint {
    EoGePoint4d ndcPoint(EoGePoint3d{wx, wy, 0.0});
    view->ModelViewTransformPoint(ndcPoint);
    return view->ProjectToClient(ndcPoint);
  };

  // Compute the active viewport's device-coordinate bounding rectangle
  const auto* activeViewport = view->ActiveViewportPrimitive();
  const bool hasValidViewport = activeViewport != nullptr &&
      activeViewport->Width() >= Eo::geometricTolerance &&
      activeViewport->Height() >= Eo::geometricTolerance;

  int vpLeft{}, vpTop{}, vpRight{}, vpBottom{};
  if (hasValidViewport) {
    const auto& center = activeViewport->CenterPoint();
    const double halfW = activeViewport->Width() / 2.0;
    const double halfH = activeViewport->Height() / 2.0;

    CPoint vpDeviceCorners[4] = {
        projectViewportCorner(center.x - halfW, center.y - halfH),
        projectViewportCorner(center.x + halfW, center.y - halfH),
        projectViewportCorner(center.x + halfW, center.y + halfH),
        projectViewportCorner(center.x - halfW, center.y + halfH),
    };
    vpLeft = (std::min)({vpDeviceCorners[0].x, vpDeviceCorners[1].x, vpDeviceCorners[2].x, vpDeviceCorners[3].x});
    vpTop = (std::min)({vpDeviceCorners[0].y, vpDeviceCorners[1].y, vpDeviceCorners[2].y, vpDeviceCorners[3].y});
    vpRight = (std::max)({vpDeviceCorners[0].x, vpDeviceCorners[1].x, vpDeviceCorners[2].x, vpDeviceCorners[3].x});
    vpBottom = (std::max)({vpDeviceCorners[0].y, vpDeviceCorners[1].y, vpDeviceCorners[2].y, vpDeviceCorners[3].y});
  }

  // ── D2D path ──────────────────────────────────────────────────────────
  auto* d2dDevice = dynamic_cast<EoGsRenderDeviceDirect2D*>(renderDevice);
  if (d2dDevice != nullptr) {
    auto* renderTarget = d2dDevice->RenderTarget();
    auto* factory = d2dDevice->D2DFactory();
    if (renderTarget == nullptr || factory == nullptr) { return; }

    RECT clipRectGdi{};
    renderDevice->GetClipBox(clipRectGdi);
    if (clipRectGdi.right <= clipRectGdi.left || clipRectGdi.bottom <= clipRectGdi.top) { return; }

    auto fullRect = D2D1::RectF(static_cast<float>(clipRectGdi.left), static_cast<float>(clipRectGdi.top),
        static_cast<float>(clipRectGdi.right), static_cast<float>(clipRectGdi.bottom));

    const auto bgColor = Eo::PaperSpaceBackgroundColor();
    const auto dimColor = D2D1::ColorF(GetRValue(bgColor) / 255.0f, GetGValue(bgColor) / 255.0f,
        GetBValue(bgColor) / 255.0f, 100.0f / 255.0f);

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> dimBrush;
    renderTarget->CreateSolidColorBrush(dimColor, &dimBrush);
    if (!dimBrush) { return; }

    if (!hasValidViewport) {
      renderTarget->FillRectangle(fullRect, dimBrush.Get());
      return;
    }

    // Create combined geometry: full clip area minus the active viewport rectangle.
    // This dims everything except the active viewport's model-space content.
    const auto vpRect = D2D1::RectF(
        static_cast<float>(vpLeft), static_cast<float>(vpTop),
        static_cast<float>(vpRight), static_cast<float>(vpBottom));

    Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> fullGeometry;
    Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> vpGeometry;
    factory->CreateRectangleGeometry(fullRect, &fullGeometry);
    factory->CreateRectangleGeometry(vpRect, &vpGeometry);
    if (!fullGeometry || !vpGeometry) { return; }

    Microsoft::WRL::ComPtr<ID2D1PathGeometry> combinedGeometry;
    factory->CreatePathGeometry(&combinedGeometry);
    if (!combinedGeometry) { return; }

    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
    combinedGeometry->Open(&sink);
    if (!sink) { return; }

    const HRESULT hr = fullGeometry->CombineWithGeometry(
        vpGeometry.Get(), D2D1_COMBINE_MODE_EXCLUDE, nullptr, sink.Get());
    sink->Close();
    if (FAILED(hr)) { return; }

    renderTarget->FillGeometry(combinedGeometry.Get(), dimBrush.Get());
    return;
  }

  // ── GDI path ──────────────────────────────────────────────────────────
  auto* dc = renderDevice->GetCDC();
  if (dc == nullptr) { return; }

  CRect clipRect;
  dc->GetClipBox(&clipRect);
  if (clipRect.IsRectEmpty()) { return; }

  const int savedDC = dc->SaveDC();

  // Exclude the active viewport's device rectangle from the dim overlay
  if (hasValidViewport) {
    dc->ExcludeClipRect(vpLeft, vpTop, vpRight, vpBottom);
  }

  // Create a 1x1 memory bitmap filled with the paper-space table background color,
  // then alpha-blend it over everything EXCEPT the active viewport.
  CDC memDC;
  if (!memDC.CreateCompatibleDC(dc)) { dc->RestoreDC(savedDC); return; }

  CBitmap bitmap;
  if (!bitmap.CreateCompatibleBitmap(dc, 1, 1)) { dc->RestoreDC(savedDC); return; }
  auto* oldBitmap = memDC.SelectObject(&bitmap);

  memDC.SetPixel(0, 0, Eo::PaperSpaceBackgroundColor());

  BLENDFUNCTION blend{};
  blend.BlendOp = AC_SRC_OVER;
  blend.BlendFlags = 0;
  blend.SourceConstantAlpha = 100;  // ~40% opacity — dims without obscuring
  blend.AlphaFormat = 0;

  ::GdiAlphaBlend(dc->GetSafeHdc(), clipRect.left, clipRect.top, clipRect.Width(), clipRect.Height(),
      memDC.GetSafeHdc(), 0, 0, 1, 1, blend);

  memDC.SelectObject(oldBitmap);
  dc->RestoreDC(savedDC);
}

EoDbViewport* AeSysDoc::HitTestViewport(const EoGePoint3d& worldPoint) {
  auto& paperLayers = PaperSpaceLayers();

  for (INT_PTR layerIndex = 0; layerIndex < paperLayers.GetSize(); layerIndex++) {
    auto* layer = paperLayers.GetAt(layerIndex);
    if (layer == nullptr) { continue; }

    auto position = layer->GetHeadPosition();
    while (position != nullptr) {
      auto* const group = layer->GetNext(position);
      if (group == nullptr) { continue; }

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != nullptr) {
        auto* primitive = group->GetNext(primitivePosition);
        if (primitive == nullptr || !primitive->Is(EoDb::kViewportPrimitive)) { continue; }

        auto* viewport = static_cast<EoDbViewport*>(primitive);
        if (viewport->ViewportId() < 2) { continue; }
        if (viewport->Width() < Eo::geometricTolerance || viewport->Height() < Eo::geometricTolerance) { continue; }

        const auto& center = viewport->CenterPoint();
        const double halfW = viewport->Width() / 2.0;
        const double halfH = viewport->Height() / 2.0;

        if (worldPoint.x >= center.x - halfW && worldPoint.x <= center.x + halfW &&
            worldPoint.y >= center.y - halfH && worldPoint.y <= center.y + halfH) {
          return viewport;
        }
      }
    }
  }
  return nullptr;
}

EoDbViewport* AeSysDoc::FindFirstViewport() {
  auto& paperLayers = PaperSpaceLayers();

  for (INT_PTR layerIndex = 0; layerIndex < paperLayers.GetSize(); layerIndex++) {
    auto* layer = paperLayers.GetAt(layerIndex);
    if (layer == nullptr) { continue; }

    auto position = layer->GetHeadPosition();
    while (position != nullptr) {
      auto* group = layer->GetNext(position);
      if (group == nullptr) { continue; }

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != nullptr) {
        auto* primitive = group->GetNext(primitivePosition);
        if (primitive == nullptr || !primitive->Is(EoDb::kViewportPrimitive)) { continue; }

        auto* viewport = static_cast<EoDbViewport*>(primitive);
        if (viewport->ViewportId() < 2) { continue; }
        if (viewport->Width() < Eo::geometricTolerance || viewport->Height() < Eo::geometricTolerance) { continue; }

        return viewport;
      }
    }
  }
  return nullptr;
}

void AeSysDoc::DisplayModelSpaceLayers(AeSysView* view, EoGsRenderDevice* renderDevice) {
  auto& layers = m_modelSpaceLayers;
  for (INT_PTR i = 0; i < layers.GetSize(); i++) {
    auto* layer = layers.GetAt(i);
    if (layer != nullptr && !layer->IsOff()) { layer->Display(view, renderDevice); }
  }
}

void AeSysDoc::DisplayModelSpaceThroughViewports(AeSysView* view, EoGsRenderDevice* renderDevice) {
  // Walk paper-space layers to find EoDbViewport primitives
  auto& paperLayers = PaperSpaceLayers();

  for (INT_PTR layerIndex = 0; layerIndex < paperLayers.GetSize(); layerIndex++) {
    auto* layer = paperLayers.GetAt(layerIndex);
    if (layer == nullptr || layer->IsOff()) { continue; }

    auto position = layer->GetHeadPosition();
    while (position != nullptr) {
      auto* group = layer->GetNext(position);
      if (group == nullptr) { continue; }

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != nullptr) {
        auto* primitive = group->GetNext(primitivePosition);
        if (primitive == nullptr || !primitive->Is(EoDb::kViewportPrimitive)) { continue; }

        auto* viewport = static_cast<EoDbViewport*>(primitive);

        // Skip the overall paper-space viewport (id 1) and viewports with no model-space view
        if (viewport->ViewportId() == 1) { continue; }
        if (viewport->ViewHeight() < Eo::geometricTolerance) {
          ATLTRACE2(traceGeneral, 3, L"DisplayModelSpaceThroughViewports: VP id=%d SKIPPED (viewHeight=%.6f)\n",
              viewport->ViewportId(), viewport->ViewHeight());
          continue;
        }

        const auto& centerPoint = viewport->CenterPoint();
        const double halfWidth = viewport->Width() / 2.0;
        const double halfHeight = viewport->Height() / 2.0;
        // Compute the 4 corners of the viewport boundary in paper space
        const EoGePoint3d corners[4] = {
            {centerPoint.x - halfWidth, centerPoint.y - halfHeight, centerPoint.z},
            {centerPoint.x + halfWidth, centerPoint.y - halfHeight, centerPoint.z},
            {centerPoint.x + halfWidth, centerPoint.y + halfHeight, centerPoint.z},
            {centerPoint.x - halfWidth, centerPoint.y + halfHeight, centerPoint.z},
        };

        // Transform corners through the current (paper-space) view to device coordinates
        CPoint deviceCorners[4];
        for (int i = 0; i < 4; ++i) {
          EoGePoint4d ndcPoint(corners[i]);
          view->ModelViewTransformPoint(ndcPoint);
          deviceCorners[i] = view->ProjectToClient(ndcPoint);
        }

        // Compute the bounding device rectangle for the GDI clip
        const int clipLeft = (std::min)({deviceCorners[0].x, deviceCorners[1].x, deviceCorners[2].x, deviceCorners[3].x});
        const int clipTop = (std::min)({deviceCorners[0].y, deviceCorners[1].y, deviceCorners[2].y, deviceCorners[3].y});
        const int clipRight = (std::max)({deviceCorners[0].x, deviceCorners[1].x, deviceCorners[2].x, deviceCorners[3].x});
        const int clipBottom = (std::max)({deviceCorners[0].y, deviceCorners[1].y, deviceCorners[2].y, deviceCorners[3].y});

        // Skip if the clip rectangle is degenerate
        if (clipRight <= clipLeft || clipBottom <= clipTop) { continue; }

        // Save DC state (includes clip region) and apply viewport clip
        renderDevice->PushClipRect(clipLeft, clipTop, clipRight, clipBottom);

        // Save the current view transform and configure for this viewport's model-space view
        view->PushViewTransform();

        // Configure camera using DCS-to-WCS mapping for correct non-XY plane views
        const auto& viewDirection = viewport->ViewDirection();
        const double viewHeight = viewport->ViewHeight();
        const double aspectRatio = viewport->Width() / viewport->Height();
        const double viewWidth = viewHeight * aspectRatio;

        EoGeVector3d dcsX, dcsY;
        EoGePoint3d wcsCameraTarget;
        viewport->ComputeViewPlaneAxes(dcsX, dcsY, wcsCameraTarget);

        view->SetCameraTarget(wcsCameraTarget);
        view->SetCameraViewUp(dcsY);
        view->SetCameraPosition(EoGeVector3d(viewDirection.x, viewDirection.y, viewDirection.z));

        // Compute an off-center projection window so the model-space view area
        // (viewWidth × viewHeight) maps exactly to the GDI clip region.
        // The orthographic projection maps UMin..UMax → NDC [-1,1], and
        // ProjectToClient maps NDC to the FULL device viewport. Since we clip
        // to a sub-region, we enlarge and offset the window proportionally.
        EoGsViewport deviceViewport;
        view->ModelViewGetViewport(deviceViewport);
        const double deviceWidth = deviceViewport.Width();
        const double deviceHeight = deviceViewport.Height();
        const auto clipWidth = static_cast<double>(clipRight - clipLeft);
        const auto clipHeight = static_cast<double>(clipBottom - clipTop);
        const double clipCenterX = (clipLeft + clipRight) / 2.0;
        const double clipCenterY = (clipTop + clipBottom) / 2.0;

        // Half-extents scaled so the desired model-space area fills the clip region
        const double halfExtentU = viewWidth * deviceWidth / (2.0 * clipWidth);
        const double halfExtentV = viewHeight * deviceHeight / (2.0 * clipHeight);

        // Offset the window center so the camera target projects to the clip region center
        const double windowCenterU = halfExtentU * (1.0 - 2.0 * clipCenterX / deviceWidth);
        const double windowCenterV = halfExtentV * (2.0 * clipCenterY / deviceHeight - 1.0);

        view->SetViewWindow(windowCenterU - halfExtentU, windowCenterV - halfExtentV, windowCenterU + halfExtentU,
            windowCenterV + halfExtentV);

        // Render model-space layers through this viewport
        const int savedModelRenderState = Gs::renderState.Save();
        DisplayModelSpaceLayers(view, renderDevice);
        Gs::renderState.Restore(renderDevice, savedModelRenderState);

        // Restore the previous view transform
        view->PopViewTransform();

        // Restore the DC (removes the clip region)
        renderDevice->PopClipRect();
      }
    }
  }
}

void AeSysDoc::CreateDefaultPaperSpaceViewport(AeSysView* view) {
  // Check if paper-space layers already have a valid model-space viewport (ID > 1 with viewHeight > 0)
  auto& paperLayers = PaperSpaceLayers();
  for (INT_PTR layerIndex = 0; layerIndex < paperLayers.GetSize(); layerIndex++) {
    auto* layer = paperLayers.GetAt(layerIndex);
    if (layer == nullptr) { continue; }

    auto position = layer->GetHeadPosition();
    while (position != nullptr) {
      auto* const group = layer->GetNext(position);
      if (group == nullptr) { continue; }

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != nullptr) {
        auto* primitive = group->GetNext(primitivePosition);
        if (primitive != nullptr && primitive->Is(EoDb::kViewportPrimitive)) {
          auto* viewport = static_cast<EoDbViewport*>(primitive);
          if (viewport->ViewportId() > 1 && viewport->ViewHeight() > Eo::geometricTolerance) {
            return;  // Already has a valid model-space viewport — nothing to do
          }
        }
      }
    }
  }

  // Use the current view window as the paper-space sheet.
  // PEG files were originally just paper-space — the view window IS the paper.
  // This preserves 1:1 scale for precision plot output and ensures new work
  // is never clipped in paper-space.
  const double viewWidth = view->UExtent();
  const double viewHeight = view->VExtent();

  // Guard against degenerate view window (should not happen after OnSize)
  if (viewWidth < Eo::geometricTolerance || viewHeight < Eo::geometricTolerance) { return; }

  const auto viewCenter = view->CameraTarget();
  const double sheetWidth = viewWidth;
  const double sheetHeight = viewHeight;

  // Center the paper-space sheet on the camera target so the viewport fills
  // the PaperSpace view regardless of where the model-space camera is pointed.
  // When a VPORT places the target at origin, this avoids the viewport
  // appearing only in the upper-right quadrant of the view.
  const double sheetCenterX = viewCenter.x;
  const double sheetCenterY = viewCenter.y;

  // Create the model-space viewport (ID 2) — constructor auto-assigns a unique handle
  auto* viewportPrimitive = new EoDbViewport();
  viewportPrimitive->SetCenterPoint(EoGePoint3d{sheetCenterX, sheetCenterY, 0.0});
  viewportPrimitive->SetWidth(sheetWidth);
  viewportPrimitive->SetHeight(sheetHeight);
  viewportPrimitive->SetViewportStatus(1);
  viewportPrimitive->SetViewportId(2);
  viewportPrimitive->SetViewCenter(EoGePoint3d{viewCenter.x, viewCenter.y, 0.0});
  viewportPrimitive->SetViewDirection(EoGePoint3d{0.0, 0.0, 1.0});
  viewportPrimitive->SetViewTargetPoint(EoGePoint3d{viewCenter.x, viewCenter.y, 0.0});
  viewportPrimitive->SetViewHeight(viewHeight);
  viewportPrimitive->SetLensLength(50.0);
  viewportPrimitive->SetTwistAngle(0.0);

  RegisterHandle(viewportPrimitive);

  auto* viewportGroup = new EoDbGroup();
  viewportGroup->AddTail(viewportPrimitive);

  auto* paperLayer0 = FindLayerInSpace(L"0", EoDxf::Space::PaperSpace);
  if (paperLayer0 == nullptr) {
    // PEG files (especially V1) may have no paper-space layers at all.
    // Create a minimal paper-space layer "0" to host the viewport.
    constexpr auto layerState = static_cast<std::uint16_t>(
        std::to_underlying(EoDbLayer::State::isResident) | std::to_underlying(EoDbLayer::State::isInternal));
    paperLayer0 = new EoDbLayer(L"0", layerState);
    AddLayerToSpace(paperLayer0, EoDxf::Space::PaperSpace);
  }
  paperLayer0->AddTail(viewportGroup);

  ATLTRACE2(traceGeneral, 1,
      L"AeSysDoc<%p>::CreateDefaultPaperSpaceViewport() — created viewport id=2 "
      L"sheet=(%.2f x %.2f) view center=(%.2f, %.2f) viewHeight=%.2f\n",
      this, sheetWidth, sheetHeight, viewCenter.x, viewCenter.y, viewHeight);
}

void AeSysDoc::AddGroupToAllViews(EoDbGroup* group) {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != nullptr) {
    auto* view = DYNAMIC_DOWNCAST(AeSysView, GetNextView(viewPosition));
    if (view != nullptr) { view->AddGroup(group); }
  }
}
void AeSysDoc::AddGroupsToAllViews(EoDbGroupList* groups) {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != nullptr) {
    auto* view = DYNAMIC_DOWNCAST(AeSysView, GetNextView(viewPosition));
    if (view != nullptr) { view->AddGroups(groups); }
  }
}
void AeSysDoc::RemoveAllGroupsFromAllViews() {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != nullptr) {
    auto* view = DYNAMIC_DOWNCAST(AeSysView, GetNextView(viewPosition));
    if (view != nullptr) { view->RemoveAllGroups(); }
  }
}
void AeSysDoc::RemoveGroupFromAllViews(EoDbGroup* group) {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != nullptr) {
    auto* view = DYNAMIC_DOWNCAST(AeSysView, GetNextView(viewPosition));
    if (view != nullptr) { view->RemoveGroup(group); }
  }
}
void AeSysDoc::ResetAllViews() {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != nullptr) {
    auto* view = DYNAMIC_DOWNCAST(AeSysView, GetNextView(viewPosition));
    if (view != nullptr) { view->ResetView(); }
  }
}
