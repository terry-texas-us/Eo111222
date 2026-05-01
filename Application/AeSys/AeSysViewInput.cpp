#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbPolygon.h"
#include "EoDbViewport.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "Resource.h"

#include "AeSysState.h"


BOOL AeSysView::PreTranslateMessage(MSG* pMsg) {
  auto* state = GetCurrentState();
  if (pMsg->message == WM_KEYDOWN) {
    if (pMsg->wParam == VK_ESCAPE && m_fieldTrapAnchorSet) {
      m_fieldTrapAnchorSet = false;
      RubberBandingDisable();
      return TRUE;
    }
    if (state != nullptr && state->HandleKeypad(this, static_cast<UINT>(pMsg->wParam), 1, static_cast<UINT>(pMsg->lParam))) {
      return TRUE;  // Handled by state
    }
  }
  if (pMsg->message == WM_COMMAND) {
    if (state != nullptr && state->ShouldBlockCommand(LOWORD(pMsg->wParam))) {
      return TRUE;  // Swallow command -- sub-mode holds raw doc pointers
    }
  }
  return CView::PreTranslateMessage(pMsg);
}

void AeSysView::OnLButtonDown(UINT flags, CPoint point) {
  auto* document = GetDocument();
  const auto worldPoint = GetCursorPosition();

  // --- Two-click field-trap: second click commits the rectangle ---
  if (m_fieldTrapAnchorSet) {
    m_fieldTrapAnchorSet = false;
    RubberBandingDisable();

    if (document != nullptr && !(m_rubberbandBegin == worldPoint)) {
      EoGePoint4d ptView[] = {EoGePoint4d(m_rubberbandBegin), EoGePoint4d(worldPoint)};
      ModelViewTransformPoints(2, ptView);
      const EoGePoint3d ptMin = EoGePoint3d{EoGePoint4d::Min(ptView[0], ptView[1])};
      const EoGePoint3d ptMax = EoGePoint3d{EoGePoint4d::Max(ptView[0], ptView[1])};
      EoDbPolygon::EdgeToEvaluate() = 0;
      const bool shiftHeld = (flags & MK_SHIFT) != 0;
      bool anyChanged = false;
      auto position = GetFirstVisibleGroupPosition();
      while (position != nullptr) {
        auto* group = GetNextVisibleGroup(position);
        if (shiftHeld) {
          // Shift+second-click: remove all trapped groups inside the rectangle.
          if (document->FindTrappedGroup(group) != nullptr &&
              group->SelectUsingRectangle(this, ptMin, ptMax)) {
            document->RemoveTrappedGroup(group);
            anyChanged = true;
          }
        } else {
          if (document->FindTrappedGroup(group) != nullptr) { continue; }
          if (group->SelectUsingRectangle(this, ptMin, ptMax)) {
            document->AddGroupToTrap(group);
            anyChanged = true;
          }
        }
      }
      if (anyChanged) {
        UpdateStateInformation(TrapCount);
        InvalidateScene();
      }
    }
    return;
  }

  // --- LMB trap-pick: hit-test visible groups first (any mode) ---
  // Plain click adds to trap; Shift+click removes from trap.
  // When a group is hit the click is consumed — the mode state does not see it.
  if (document != nullptr) {
    auto* group = SelectGroupAndPrimitive(worldPoint);
    if (group != nullptr) {
      const bool shiftHeld = (flags & MK_SHIFT) != 0;
      if (shiftHeld) {
        if (document->FindTrappedGroup(group) != nullptr) {
          document->RemoveTrappedGroup(group);
          UpdateStateInformation(TrapCount);
          InvalidateScene();
        }
      } else {
        if (document->FindTrappedGroup(group) == nullptr) {
          document->AddGroupToTrap(group);
          UpdateStateInformation(TrapCount);
          InvalidateScene();
        }
      }
      return;
    }
  }

  // --- Delegate to active mode state ---
  auto* state = GetCurrentState();
  if (state != nullptr) {
    // If the state has no active gesture (idle op), start a field-trap rectangle instead.
    if (state->GetActiveOp() == 0) {
      m_fieldTrapAnchorSet = true;
      RubberBandingStartAtEnable(worldPoint, Rectangles);
      return;
    }
    state->OnLButtonDown(this, flags, point);
    return;
  }

  // --- Fully idle: start field-trap rectangle ---
  m_fieldTrapAnchorSet = true;
  RubberBandingStartAtEnable(worldPoint, Rectangles);
}

void AeSysView::OnLButtonUp(UINT flags, CPoint point) {
  auto* state = GetCurrentState();
  if (state != nullptr) { state->OnLButtonUp(this, flags, point); return; }
  CView::OnLButtonUp(flags, point);
}

void AeSysView::OnLButtonDblClk([[maybe_unused]] UINT flags, CPoint point) {
  auto* document = GetDocument();
  if (document == nullptr) { return; }

  // Viewport activation is only meaningful in paper space
  if (document->ActiveSpace() != EoDxf::Space::PaperSpace) {
    OnLButtonDown(flags, point);
    return;
  }

  // Convert the device click point to paper-space world coordinates
  EoGePoint3d worldPoint(static_cast<double>(point.x), static_cast<double>(point.y), 0.0);
  DoProjectionInverse(worldPoint);
  worldPoint = ModelViewGetMatrixInverse() * worldPoint;

  // Hit-test paper-space viewports at the click location
  auto* hitViewport = document->HitTestViewport(worldPoint);

  if (IsViewportActive()) {
    if (hitViewport == m_activeViewportPrimitive) {
      // Double-click inside the active viewport: no change
      return;
    }
    // Double-click outside or inside a different viewport: deactivate first
    DeactivateViewport();
    // If the click was inside a different viewport, activate it
    if (hitViewport != nullptr) { SetActiveViewportPrimitive(hitViewport); }
    return;
  }

  // No viewport active — activate the one under the cursor
  if (hitViewport != nullptr) { SetActiveViewportPrimitive(hitViewport); }
}

void AeSysView::SetActiveViewportPrimitive(EoDbViewport* viewport) {
  if (m_activeViewportPrimitive != viewport) {
    m_activeViewportPrimitive = viewport;

    // Route entity creation to model-space when a viewport is activated
    if (viewport != nullptr) {
      auto* document = GetDocument();
      if (document != nullptr) {
        // Save the current paper-space work layer so it can be restored on deactivation
        m_savedWorkLayerForViewport = document->GetWorkLayer();

        // Find model-space layer "0" and switch the work layer to it
        auto* modelLayer0 = document->FindLayerInSpace(L"0", EoDxf::Space::ModelSpace);
        if (modelLayer0 != nullptr) { document->SetWorkLayer(modelLayer0); }
      }

      // Track this viewport as the last-active for reactivation via the space label
      m_layoutTabBar.SetLastActiveViewport(viewport);
    }

    InvalidateScene();

    // Update the layout tab bar's viewport state controls
    const auto* document2 = GetDocument();
    const bool isPaperSpace = (document2 != nullptr && document2->ActiveSpace() == EoDxf::Space::PaperSpace);
    m_layoutTabBar.UpdateViewportState(viewport, isPaperSpace);
  }
}

void AeSysView::DeactivateViewport() {
  if (m_activeViewportPrimitive != nullptr) {
    // Restore the paper-space work layer that was saved during viewport activation
    if (m_savedWorkLayerForViewport != nullptr) {
      auto* document = GetDocument();
      if (document != nullptr) { document->SetWorkLayer(m_savedWorkLayerForViewport); }
      m_savedWorkLayerForViewport = nullptr;
    }

    m_activeViewportPrimitive = nullptr;
    InvalidateScene();

    // Update the layout tab bar's viewport state controls
    const auto* document2 = GetDocument();
    const bool isPaperSpace = (document2 != nullptr && document2->ActiveSpace() == EoDxf::Space::PaperSpace);
    m_layoutTabBar.UpdateViewportState(nullptr, isPaperSpace);
  }
}

bool AeSysView::ConfigureViewportTransform(const EoDbViewport* viewport) {
  if (viewport == nullptr || viewport->ViewHeight() < Eo::geometricTolerance) { return false; }

  const auto& centerPoint = viewport->CenterPoint();
  const double halfWidth = viewport->Width() / 2.0;
  const double halfHeight = viewport->Height() / 2.0;

  // Project the viewport's four paper-space corners through the current (paper-space) view transform
  // to obtain the device clip rectangle — same math as DisplayModelSpaceThroughViewports.
  const EoGePoint3d corners[4] = {
      {centerPoint.x - halfWidth, centerPoint.y - halfHeight, centerPoint.z},
      {centerPoint.x + halfWidth, centerPoint.y - halfHeight, centerPoint.z},
      {centerPoint.x + halfWidth, centerPoint.y + halfHeight, centerPoint.z},
      {centerPoint.x - halfWidth, centerPoint.y + halfHeight, centerPoint.z},
  };

  CPoint deviceCorners[4];
  for (int i = 0; i < 4; ++i) {
    EoGePoint4d ndcPoint(corners[i]);
    ModelViewTransformPoint(ndcPoint);
    deviceCorners[i] = ProjectToClient(ndcPoint);
  }

  const int clipLeft = (std::min)({deviceCorners[0].x, deviceCorners[1].x, deviceCorners[2].x, deviceCorners[3].x});
  const int clipTop = (std::min)({deviceCorners[0].y, deviceCorners[1].y, deviceCorners[2].y, deviceCorners[3].y});
  const int clipRight = (std::max)({deviceCorners[0].x, deviceCorners[1].x, deviceCorners[2].x, deviceCorners[3].x});
  const int clipBottom = (std::max)({deviceCorners[0].y, deviceCorners[1].y, deviceCorners[2].y, deviceCorners[3].y});

  if (clipRight <= clipLeft || clipBottom <= clipTop) { return false; }

  // Push the current paper-space view transform and configure for viewport model-space
  PushViewTransform();

  const auto& viewDirection = viewport->ViewDirection();
  const double viewHeight = viewport->ViewHeight();
  const double aspectRatio = viewport->Width() / viewport->Height();
  const double viewWidth = viewHeight * aspectRatio;

  EoGeVector3d dcsX, dcsY;
  EoGePoint3d wcsCameraTarget;
  viewport->ComputeViewPlaneAxes(dcsX, dcsY, wcsCameraTarget);

  SetCameraTarget(wcsCameraTarget);
  SetCameraViewUp(dcsY);
  SetCameraPosition(EoGeVector3d(viewDirection.x, viewDirection.y, viewDirection.z));

  // Compute the off-center projection window so that DoProjectionInverse + ModelViewGetMatrixInverse
  // correctly map any device pixel to model-space coordinates through this viewport.
  EoGsViewport deviceViewport;
  ModelViewGetViewport(deviceViewport);
  const double deviceWidth = deviceViewport.Width();
  const double deviceHeight = deviceViewport.Height();
  const auto clipWidth = static_cast<double>(clipRight - clipLeft);
  const auto clipHeight = static_cast<double>(clipBottom - clipTop);
  const double clipCenterX = (clipLeft + clipRight) / 2.0;
  const double clipCenterY = (clipTop + clipBottom) / 2.0;

  const double halfExtentU = viewWidth * deviceWidth / (2.0 * clipWidth);
  const double halfExtentV = viewHeight * deviceHeight / (2.0 * clipHeight);
  const double windowCenterU = halfExtentU * (1.0 - 2.0 * clipCenterX / deviceWidth);
  const double windowCenterV = halfExtentV * (2.0 * clipCenterY / deviceHeight - 1.0);

  SetViewWindow(windowCenterU - halfExtentU,
      windowCenterV - halfExtentV,
      windowCenterU + halfExtentU,
      windowCenterV + halfExtentV);

  return true;
}

void AeSysView::RestoreViewportTransform() {
  PopViewTransform();
}

void AeSysView::OnRButtonDown(UINT flags, CPoint point) {
  // Do not suppress WM_RBUTTONDOWN when a state is active — Windows needs the
  // paired down/up to keep mouse-capture and WM_CONTEXTMENU state consistent.
  // No concrete state overrides OnRButtonDown; action happens in OnRButtonUp.
  CView::OnRButtonDown(flags, point);
}

void AeSysView::OnRButtonUp(UINT flags, CPoint point) {
  auto* state = GetCurrentState();
  if (state != nullptr) {
    CMenu menu;
    if (!menu.CreatePopupMenu()) { state->OnRButtonUp(this, flags, point); return; }
    if (state->BuildContextMenu(this, menu)) {
      // Snapshot the world-space cursor position at RMB-up time, before the
      // menu is shown.  TrackPopupMenuEx with TPM_RETURNCMD is synchronous and
      // returns the chosen command ID (or 0 for dismiss) without posting a
      // WM_COMMAND message.  We then restore the cursor to the RMB point so
      // that any commit handler (Pipe/LPD/Draw2 return, Edit move/copy place,
      // Trap stitch) reads the correct world position rather than the position
      // of the menu item that was clicked.
      const EoGePoint3d rmbWorldPoint = GetCursorPosition();

      CPoint screenPoint{point};
      ClientToScreen(&screenPoint);
      const UINT chosenCommand = menu.TrackPopupMenuEx(
          TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
          screenPoint.x, screenPoint.y, this, nullptr);

      if (chosenCommand != 0) {
        // Restore the world cursor to the RMB click position before dispatch
        // so commit handlers use the intended point.
        SetCursorPosition(rmbWorldPoint);
        SendMessage(WM_COMMAND, MAKEWPARAM(chosenCommand, 0));
      }
      return;
    }
    state->OnRButtonUp(this, flags, point);
    return;
  }
  CView::OnRButtonUp(flags, point);
}

void AeSysView::OnMButtonDown([[maybe_unused]] UINT flags, CPoint point) {
  m_middleButtonPanStartPoint = point;
  m_middleButtonPanInProgress = true;
  SetCapture();
}

void AeSysView::OnMButtonUp([[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  if (m_middleButtonPanInProgress) {
    m_middleButtonPanInProgress = false;
    ReleaseCapture();
  }
}

void AeSysView::OnMouseMove([[maybe_unused]] UINT flags, CPoint point) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView::OnMouseMove - flags: %u, point: (%d, %d)\n", flags, point.x, point.y);

  // Reset hover timer only when the cursor pixel position actually changes.
  // The tooltip window appearing near the cursor generates a spurious WM_MOUSEMOVE
  // from Windows; ignoring same-pixel moves prevents the tooltip from flashing.
  static CPoint lastMousePoint{-1, -1};
  if (point != lastMousePoint) {
    lastMousePoint = point;
    KillTimer(kHoverTimerId);
    TOOLINFOW ti{};
    ti.cbSize = sizeof(ti);
    ti.hwnd = GetSafeHwnd();
    ti.uId = 1;
    m_hoverTooltip.SendMessage(TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&ti));
    SetTimer(kHoverTimerId, kHoverDelayMs, nullptr);
  }
  auto* state = GetCurrentState();
  if (state != nullptr) { state->OnMouseMove(this, flags, point); }
  if (m_middleButtonPanInProgress) {
    const auto delta = point - m_middleButtonPanStartPoint;
    m_middleButtonPanStartPoint = point;

    if (IsViewportActive()) {
      if (!m_activeViewportPrimitive->IsDisplayLocked()) {
        // Pan the viewport's model-space view center instead of the paper-space view transform.
        // Convert pixel delta to model-space units using the viewport's projected device dimensions.
        auto* viewport = m_activeViewportPrimitive;
        const auto& centerPoint = viewport->CenterPoint();
        const double halfWidth = viewport->Width() / 2.0;
        const double halfHeight = viewport->Height() / 2.0;

        // Project two diagonal corners of the viewport boundary to device coordinates
        EoGePoint4d cornerMin(EoGePoint3d{centerPoint.x - halfWidth, centerPoint.y - halfHeight, centerPoint.z});
        EoGePoint4d cornerMax(EoGePoint3d{centerPoint.x + halfWidth, centerPoint.y + halfHeight, centerPoint.z});
        ModelViewTransformPoint(cornerMin);
        ModelViewTransformPoint(cornerMax);
        const auto deviceMin = ProjectToClient(cornerMin);
        const auto deviceMax = ProjectToClient(cornerMax);

        const double clipWidth = std::abs(static_cast<double>(deviceMax.x - deviceMin.x));
        const double clipHeight = std::abs(static_cast<double>(deviceMax.y - deviceMin.y));

        if (clipWidth > 0.0 && clipHeight > 0.0) {
          const double viewHeight = viewport->ViewHeight();
          const double viewWidth = viewHeight * (viewport->Width() / viewport->Height());

          EoGePoint3d viewCenter = viewport->ViewCenter();  // intentional copy
          viewCenter.x += static_cast<double>(-delta.cx) * viewWidth / clipWidth;
          viewCenter.y += static_cast<double>(delta.cy) * viewHeight / clipHeight;
          viewport->SetViewCenter(viewCenter);
        }
        InvalidateScene();
      }
    } else {
      // Standard paper-space / model-space pan
      auto target = m_ViewTransform.Target();
      target.x += static_cast<double>(-delta.cx) * m_ViewTransform.UExtent() / m_Viewport.Width();
      target.y += static_cast<double>(delta.cy) * m_ViewTransform.VExtent() / m_Viewport.Height();

      m_ViewTransform.SetTarget(target);
      m_ViewTransform.SetPosition(m_ViewTransform.Direction());
      m_ViewTransform.BuildTransformMatrix();
      InvalidateScene();
    }
  }
  DisplayOdometer();

  if (m_rubberbandType == Lines || m_rubberbandType == Rectangles) {
    // Both D2D and GDI paths: update endpoint and trigger an overlay-only repaint.
    // Rubberband is drawn into the overlay buffer in OnDraw — no XOR-on-screen needed.
    m_rubberbandLogicalEnd = point;
    InvalidateOverlay();
  }
}

BOOL AeSysView::OnMouseWheel(UINT flags, std::int16_t zDelta, CPoint point) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>OnMouseWheel(%i, %i, %08.8lx)\n", this, flags, zDelta, point);

  if (zDelta > 0) {
    OnWindowZoomIn();
  } else {
    OnWindowZoomOut();
  }
  return __super::OnMouseWheel(flags, zDelta, point);
}

void AeSysView::RubberBandingDisable() {
  if (m_rubberbandType != None) {
    m_rubberbandType = None;
    // Both D2D and GDI paths: next overlay compose will omit the rubberband since
    // m_rubberbandType is now None. A single overlay-only repaint cleans it up.
    InvalidateOverlay();
  }
}

void AeSysView::RubberBandingStartAtEnable(EoGePoint3d point, ERubs type) {
  EoGePoint4d ndcPoint(point);

  // When a viewport is active, the point is in model-space coordinates.
  // Project through the viewport transform to get correct device coordinates.
  if (IsViewportActive() && ConfigureViewportTransform(m_activeViewportPrimitive)) {
    ModelViewTransformPoint(ndcPoint);
    RestoreViewportTransform();
  } else {
    ModelViewTransformPoint(ndcPoint);
  }

  if (ndcPoint.IsInView()) {
    m_rubberbandBegin = point;
    m_rubberbandLogicalBegin = ProjectToClient(ndcPoint);
    m_rubberbandLogicalEnd = m_rubberbandLogicalBegin;
  }
  m_rubberbandType = type;
}

EoGePoint3d AeSysView::GetCursorPosition() {
  CPoint cursorPosition;

  ::GetCursorPos(&cursorPosition);
  ScreenToClient(&cursorPosition);

  const EoGePoint3d pt{static_cast<double>(cursorPosition.x), static_cast<double>(cursorPosition.y), 0.0};
  if (pt.x != m_ptCursorPosDev.x || pt.y != m_ptCursorPosDev.y) {
    m_ptCursorPosDev = pt;
    m_ptCursorPosWorld = m_ptCursorPosDev;

    // When a viewport is active, route device coordinates through the viewport's
    // model-space transform instead of the paper-space transform.
    if (IsViewportActive() && ConfigureViewportTransform(m_activeViewportPrimitive)) {
      DoProjectionInverse(m_ptCursorPosWorld);
      m_ptCursorPosWorld = ModelViewGetMatrixInverse() * m_ptCursorPosWorld;
      RestoreViewportTransform();
    } else {
      DoProjectionInverse(m_ptCursorPosWorld);
      m_ptCursorPosWorld = ModelViewGetMatrixInverse() * m_ptCursorPosWorld;
    }
    m_ptCursorPosWorld.z = 0.0;
    m_ptCursorPosWorld = SnapPointToGrid(m_ptCursorPosWorld);
  }
  return m_ptCursorPosWorld;
}

void AeSysView::SetCursorPosition(const EoGePoint3d& position) {
  // When a viewport is active, the position is in model-space coordinates.
  // Project through the viewport transform to get device coordinates.
  if (IsViewportActive() && ConfigureViewportTransform(m_activeViewportPrimitive)) {
    EoGePoint4d ndcPoint(position);
    ModelViewTransformPoint(ndcPoint);
    CPoint clientPoint = ProjectToClient(ndcPoint);
    RestoreViewportTransform();

    m_ptCursorPosDev =
        EoGePoint3d(static_cast<double>(clientPoint.x), static_cast<double>(clientPoint.y), ndcPoint.z / ndcPoint.w);
    m_ptCursorPosWorld = position;

    ClientToScreen(&clientPoint);
    ::SetCursorPos(clientPoint.x, clientPoint.y);
    return;
  }

  EoGePoint4d ndcPoint(position);
  ModelViewTransformPoint(ndcPoint);

  if (!ndcPoint.IsInView()) {
    // Redefine the view so targeted position becomes center
    m_ViewTransform.SetTarget(position);
    m_ViewTransform.SetPosition(m_ViewTransform.Direction());
    m_ViewTransform.BuildTransformMatrix();

    InvalidateScene();

    ndcPoint = EoGePoint4d(position);
    ModelViewTransformPoint(ndcPoint);
  }
  // Move the cursor to specified position.
  CPoint clientPoint = ProjectToClient(ndcPoint);

  m_ptCursorPosDev =
      EoGePoint3d(static_cast<double>(clientPoint.x), static_cast<double>(clientPoint.y), ndcPoint.z / ndcPoint.w);

  m_ptCursorPosWorld = position;

  ClientToScreen(&clientPoint);
  ::SetCursorPos(clientPoint.x, clientPoint.y);
}

/// @brief Loads a cursor from an RCDATA resource containing a raw .cur file.
/// Parses the .cur directory, selects the image closest to desiredSize, and creates the
/// cursor via CreateIconFromResourceEx. This bypasses rc.exe's inability to parse
/// PNG-compressed .cur images (RC2176).
/// @param resourceIdentifier RCDATA resource ID
/// @param desiredSize Target cursor size in pixels (e.g., DPI value for 1-inch cursor)
/// @return HCURSOR on success, nullptr on failure
static HCURSOR LoadCursorFromRcData(UINT resourceIdentifier, int desiredSize) {
  const auto instance = AeSys::GetInstance();
  const auto resourceInfo = FindResourceW(instance, MAKEINTRESOURCE(resourceIdentifier), RT_RCDATA);
  if (resourceInfo == nullptr) { return nullptr; }

  const auto resourceHandle = LoadResource(instance, resourceInfo);
  if (resourceHandle == nullptr) { return nullptr; }

  const auto* data = static_cast<const BYTE*>(LockResource(resourceHandle));
  const auto dataSize = SizeofResource(instance, resourceInfo);
  if (data == nullptr || dataSize < 6) { return nullptr; }

  // .cur header: WORD reserved (0), WORD type (2 = cursor), WORD imageCount
  const auto imageCount = *reinterpret_cast<const WORD*>(data + 4);
  if (imageCount == 0 || dataSize < static_cast<DWORD>(6 + imageCount * 16)) { return nullptr; }

  // .cur directory entry (16 bytes each):
  //   BYTE width, BYTE height, BYTE colorCount, BYTE reserved,
  //   WORD xHotspot, WORD yHotspot, DWORD imageSize, DWORD imageOffset
  // Width/height of 0 means 256.
  int bestIndex = 0;
  int bestDelta = INT_MAX;
  for (int i = 0; i < imageCount; i++) {
    const auto* entry = data + 6 + i * 16;
    const int entryWidth = entry[0] == 0 ? 256 : entry[0];
    const int delta = std::abs(entryWidth - desiredSize);
    if (delta < bestDelta) {
      bestDelta = delta;
      bestIndex = i;
    }
  }

  const auto* bestEntry = data + 6 + bestIndex * 16;
  const int bestWidth = bestEntry[0] == 0 ? 256 : bestEntry[0];
  const auto hotspotX = *reinterpret_cast<const WORD*>(bestEntry + 4);
  const auto hotspotY = *reinterpret_cast<const WORD*>(bestEntry + 6);
  const auto imageSize = *reinterpret_cast<const DWORD*>(bestEntry + 8);
  const auto imageOffset = *reinterpret_cast<const DWORD*>(bestEntry + 12);

  if (imageOffset + imageSize > dataSize) { return nullptr; }

  // CreateIconFromResourceEx for cursors expects: [WORD xHotspot][WORD yHotspot][image data]
  const auto bufferSize = static_cast<DWORD>(4 + imageSize);
  auto buffer = std::make_unique<BYTE[]>(bufferSize);
  *reinterpret_cast<WORD*>(buffer.get()) = hotspotX;
  *reinterpret_cast<WORD*>(buffer.get() + 2) = hotspotY;
  std::memcpy(buffer.get() + 4, data + imageOffset, imageSize);

  return static_cast<HCURSOR>(
      CreateIconFromResourceEx(buffer.get(), bufferSize, FALSE, 0x00030000, bestWidth, bestWidth, 0));
}

void AeSysView::SetModeCursor(int mode) {
  ATLTRACE2(
      traceGeneral, 1, L"AeSysView::SetModeCursor(Mode: %i, Cursor Size: %i)\n", mode, GetSystemMetrics(SM_CXCURSOR));

  // Paper space is always white — cursor must use white-background variant.
  // Block edit uses gray background — cursor uses the normal dark-scheme variant.
  const auto* document = GetDocument();
  const auto isWhiteBackground = (Eo::activeViewBackground == Eo::ViewBackground::White)
      || (document != nullptr && document->ActiveSpace() == EoDxf::Space::PaperSpace && !document->IsEditingBlock());
  std::uint16_t resourceIdentifier{};

  switch (mode) {
    case ID_MODE_ANNOTATE:
      resourceIdentifier = isWhiteBackground ? IDR_ANNOTATE_MODE_WHITE : IDR_ANNOTATE_MODE;
      break;

    case ID_MODE_CUT:
      resourceIdentifier = isWhiteBackground ? IDR_CUT_MODE_WHITE : IDR_CUT_MODE;
      break;

    case ID_MODE_DIMENSION:
      resourceIdentifier = isWhiteBackground ? IDR_DIMENSION_MODE_WHITE : IDR_DIMENSION_MODE;
      break;

    case ID_MODE_DRAW:
      resourceIdentifier = isWhiteBackground ? IDR_DRAW_MODE_WHITE : IDR_DRAW_MODE;
      break;

    case ID_MODE_LPD:
      resourceIdentifier = isWhiteBackground ? IDR_LPD_MODE_WHITE : IDR_LPD_MODE;
      break;

    case ID_MODE_PIPE:
      resourceIdentifier = isWhiteBackground ? IDR_PIPE_MODE_WHITE : IDR_PIPE_MODE;
      break;

    case ID_MODE_POWER:
      resourceIdentifier = isWhiteBackground ? IDR_POWER_MODE_WHITE : IDR_POWER_MODE;
      break;

    case ID_MODE_DRAW2:
      resourceIdentifier = isWhiteBackground ? IDR_DRAW2_MODE_WHITE : IDR_DRAW2_MODE;
      break;

    case ID_MODE_EDIT:
      resourceIdentifier = isWhiteBackground ? IDR_EDIT_MODE_WHITE : IDR_EDIT_MODE;
      break;

    case ID_MODE_FIXUP:
      resourceIdentifier = isWhiteBackground ? IDR_FIXUP_MODE_WHITE : IDR_FIXUP_MODE;
      break;

    case ID_MODE_NODAL:
      resourceIdentifier = isWhiteBackground ? IDR_NODAL_MODE_WHITE : IDR_NODAL_MODE;
      break;

    case ID_MODE_NODALR:
      resourceIdentifier = isWhiteBackground ? IDR_NODALR_MODE_WHITE : IDR_NODALR_MODE;
      break;

    case ID_MODE_TRAP:
      resourceIdentifier = isWhiteBackground ? IDR_TRAP_MODE_WHITE : IDR_TRAP_MODE;
      break;

    case ID_MODE_TRAPR:
      resourceIdentifier = isWhiteBackground ? IDR_TRAPR_MODE_WHITE : IDR_TRAPR_MODE;
      break;

    default:
      SetCursor(static_cast<HCURSOR>(LoadImageW(nullptr, IDC_CROSS, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE)));
      return;
  }

  // Compute DPI-scaled cursor size for resources that contain high-resolution images.
  // SM_CXCURSOR is not DPI-aware and always returns 32, so LR_DEFAULTSIZE picks 32x32 even on high-DPI displays.
  // For Draw mode, target a 1-inch cursor: size in pixels equals the DPI value. LoadImageW picks the closest match
  // from the .cur file (e.g., 128x128 at 144 DPI, 256x256 at 288+ DPI).
  //
  // Draw mode uses RCDATA (not CURSOR) because the .cur file contains PNG-compressed images that rc.exe cannot parse.
  // LoadCursorFromRcData handles the directory parsing and calls CreateIconFromResourceEx with the best-match image.
  HCURSOR cursorHandle{};
  if (mode == ID_MODE_ANNOTATE || mode == ID_MODE_DRAW || mode == ID_MODE_TRAP || mode == ID_MODE_TRAPR) {
    const auto desiredSize = static_cast<int>(GetDpiForWindow(GetSafeHwnd()));
    cursorHandle = LoadCursorFromRcData(resourceIdentifier, desiredSize);
  }
  if (cursorHandle == nullptr) {
    cursorHandle = static_cast<HCURSOR>(
        LoadImageW(AeSys::GetInstance(), MAKEINTRESOURCE(resourceIdentifier), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
  }
  VERIFY(cursorHandle);
  SetCursor(cursorHandle);

  SetClassLongPtr(this->GetSafeHwnd(), GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(cursorHandle));
}
