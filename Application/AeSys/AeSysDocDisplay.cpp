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
#include "EoGsRenderDeviceGdi.h"
#include "EoGsRenderState.h"

void AeSysDoc::DisplayAllLayers(AeSysView* view, EoGsRenderDevice* renderDevice) {
  ATLTRACE2(traceGeneral, 3, L"AeSysDoc<%p>::DisplayAllLayers(%p, %p)\n", this, view, renderDevice);

  try {
    bool identifyTrap = app.IsTrapHighlighted() && !IsTrapEmpty();

    RemoveAllGroupsFromAllViews();

    const bool isPaperSpace = m_activeSpace == EoDxf::Space::PaperSpace;
    auto backgroundColor = renderDevice->SetBkColor(Eo::ViewBackgroundColorForSpace(isPaperSpace));

    EoDbPolygon::SetSpecialPolygonStyle(
        view->RenderAsWireframe() ? EoDb::PolygonStyle::Hollow : EoDb::PolygonStyle::Special);
    int savedRenderState = Gs::renderState.Save();

    for (int i = 0; i < GetLayerTableSize(); i++) {
      auto* layer = GetLayerTableLayerAt(i);
      layer->Display(view, renderDevice, identifyTrap);
    }

    // When in paper space, render model-space entities through each viewport
    if (m_activeSpace == EoDxf::Space::PaperSpace) { DisplayModelSpaceThroughViewports(view, renderDevice); }

    Gs::renderState.Restore(renderDevice, savedRenderState);
    EoDbPolygon::SetSpecialPolygonStyle(EoDb::PolygonStyle::Special);

    renderDevice->SetBkColor(backgroundColor);
  } catch (CException* e) { e->Delete(); }
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
  auto& paperLayers = m_paperSpaceLayers;

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
        if (viewport->ViewHeight() < Eo::geometricTolerance) { continue; }

        const auto& centerPoint = viewport->CenterPoint();
        const double halfWidth = viewport->Width() / 2.0;
        const double halfHeight = viewport->Height() / 2.0;

        // Compute the 4 corners of the viewport boundary in paper space
        EoGePoint3d corners[4] = {
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
        int clipLeft = (std::min)({deviceCorners[0].x, deviceCorners[1].x, deviceCorners[2].x, deviceCorners[3].x});
        int clipTop = (std::min)({deviceCorners[0].y, deviceCorners[1].y, deviceCorners[2].y, deviceCorners[3].y});
        int clipRight = (std::max)({deviceCorners[0].x, deviceCorners[1].x, deviceCorners[2].x, deviceCorners[3].x});
        int clipBottom = (std::max)({deviceCorners[0].y, deviceCorners[1].y, deviceCorners[2].y, deviceCorners[3].y});

        // Skip if the clip rectangle is degenerate
        if (clipRight <= clipLeft || clipBottom <= clipTop) { continue; }

        // Save DC state (includes clip region) and apply viewport clip
        renderDevice->PushClipRect(clipLeft, clipTop, clipRight, clipBottom);

        // Save the current view transform and configure for this viewport's model-space view
        view->PushViewTransform();

        // Configure camera: target at viewport's view target, direction from viewDirection
        const auto& viewDirection = viewport->ViewDirection();
        const auto& viewTargetPoint = viewport->ViewTargetPoint();
        const auto& viewCenter = viewport->ViewCenter();
        const double viewHeight = viewport->ViewHeight();
        const double aspectRatio = viewport->Width() / viewport->Height();
        const double viewWidth = viewHeight * aspectRatio;

        // Set the camera target and direction
        view->SetCameraTarget(EoGePoint3d(viewCenter.x, viewCenter.y, viewTargetPoint.z));
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
        const double clipWidth = static_cast<double>(clipRight - clipLeft);
        const double clipHeight = static_cast<double>(clipBottom - clipTop);
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
        int savedModelRenderState = Gs::renderState.Save();
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
  auto& paperLayers = m_paperSpaceLayers;
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
  while (viewPosition != 0) {
    auto* view = DYNAMIC_DOWNCAST(AeSysView, GetNextView(viewPosition));
    if (view != nullptr) { view->AddGroup(group); }
  }
}
void AeSysDoc::AddGroupsToAllViews(EoDbGroupList* groups) {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != 0) {
    auto* view = DYNAMIC_DOWNCAST(AeSysView, GetNextView(viewPosition));
    if (view != nullptr) { view->AddGroups(groups); }
  }
}
void AeSysDoc::RemoveAllGroupsFromAllViews() {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != 0) {
    auto* view = DYNAMIC_DOWNCAST(AeSysView, GetNextView(viewPosition));
    if (view != nullptr) { view->RemoveAllGroups(); }
  }
}
void AeSysDoc::RemoveGroupFromAllViews(EoDbGroup* group) {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != 0) {
    auto* view = DYNAMIC_DOWNCAST(AeSysView, GetNextView(viewPosition));
    if (view != nullptr) { view->RemoveGroup(group); }
  }
}
void AeSysDoc::ResetAllViews() {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != 0) {
    auto* view = DYNAMIC_DOWNCAST(AeSysView, GetNextView(viewPosition));
    if (view != nullptr) { view->ResetView(); }
  }
}
