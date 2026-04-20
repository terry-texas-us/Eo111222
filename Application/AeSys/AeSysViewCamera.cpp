#include "Stdafx.h"

#include <stdexcept>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbLayer.h"
#include "EoDbViewport.h"
#include "EoDlgSelectIsometricView.h"
#include "EoDlgSheetSetupFormFactor.h"
#include "EoDlgViewParameters.h"
#include "EoDlgViewZoom.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "MainFrm.h"
#include "Resource.h"

namespace {

constexpr double maximumWindowRatio{999.0};
constexpr double minimumWindowRatio{1e-6};

}  // namespace

CMFCStatusBar& AeSysView::GetStatusBar() const { return (static_cast<CMainFrame*>(AfxGetMainWnd()))->GetStatusBar(); }

void AeSysView::PopViewTransform() {
  if (!m_ViewTransforms.IsEmpty()) { m_ViewTransform = m_ViewTransforms.RemoveTail(); }
  m_ViewTransform.BuildTransformMatrix();
}

void AeSysView::PushViewTransform() { m_ViewTransforms.AddTail(m_ViewTransform); }

void AeSysView::ModelViewAdjustWindow(double& uMin, double& vMin, double& uMax, double& vMax, double ratio) {
  double AspectRatio = static_cast<double>(m_Viewport.Height() / m_Viewport.Width());

  double UExtent = std::abs(static_cast<double>(uMax - uMin));
  double VExtent = std::abs(static_cast<double>(vMax - vMin));

  double XAdjustment = 0.0;
  double YAdjustment = 0.0;

  double Scale = 1.0 - (m_Viewport.WidthInInches() / UExtent) / ratio;

  if (Eo::IsGeometricallyNonZero(Scale)) {
    XAdjustment = Scale * UExtent;
    YAdjustment = Scale * VExtent;
  }
  if (UExtent < Eo::geometricTolerance || VExtent / UExtent > AspectRatio) {
    XAdjustment += (VExtent / AspectRatio - UExtent) * 0.5;
  } else {
    YAdjustment += (UExtent * AspectRatio - VExtent) * 0.5f;
  }
  uMin -= XAdjustment;
  uMax += XAdjustment;
  vMin -= YAdjustment;
  vMax += YAdjustment;
}

void AeSysView::PushModelTransform() { m_ModelTransform.Push(); }

void AeSysView::SetLocalModelTransform(EoGeTransformMatrix& transformation) {
  m_ModelTransform.SetLocalTM(transformation);
}

void AeSysView::PopModelTransform() { m_ModelTransform.Pop(); }

void AeSysView::DoCameraRotate(int rotationDirection) {
  try {
    auto normal = m_ViewTransform.Position() - m_ViewTransform.Target();
    normal.Unitize();

    auto u = CrossProduct(ViewUp(), normal);
    u.Unitize();

    auto v = CrossProduct(normal, u);
    v.Unitize();

    auto position = m_ViewTransform.Position();
    auto target = m_ViewTransform.Target();
    switch (rotationDirection) {
      case ID_CAMERA_ROTATELEFT:
        position = position.RotateAboutAxis(target, v, Eo::DegreeToRadian(-10.0));
        break;
      case ID_CAMERA_ROTATERIGHT:
        position = position.RotateAboutAxis(target, v, Eo::DegreeToRadian(10.0));
        break;

      case ID_CAMERA_ROTATEUP:
        position = position.RotateAboutAxis(target, u, Eo::DegreeToRadian(-10.0));
        break;

      case ID_CAMERA_ROTATEDOWN:
        position = position.RotateAboutAxis(target, u, Eo::DegreeToRadian(10.0));
        break;
    }
    m_ViewTransform.SetPosition(position);
    m_ViewTransform.SetViewUp(v);
    m_ViewTransform.BuildTransformMatrix();
    InvalidateScene();
  } catch (const std::domain_error& error) {
    ::MessageBoxA(nullptr, error.what(), "Camera Rotate Error", MB_ICONWARNING | MB_OK);
    return;
  }
}

void AeSysView::DoWindowPan(double ratio) {
  ratio = std::min(std::max(ratio, minimumWindowRatio), maximumWindowRatio);

  double UExtent = m_Viewport.WidthInInches() / ratio;
  double VExtent = m_Viewport.HeightInInches() / ratio;

  m_ViewTransform.SetCenteredWindow(m_Viewport, UExtent, VExtent);

  auto cursorPosition = GetCursorPosition();

  auto direction = m_ViewTransform.Direction();
  auto target = m_ViewTransform.Target();

  EoGeLine::IntersectionWithPln(cursorPosition, direction, target, direction, &cursorPosition);

  m_ViewTransform.SetTarget(target);
  m_ViewTransform.SetPosition(direction);
  m_ViewTransform.BuildTransformMatrix();

  SetCursorPosition(cursorPosition);
  InvalidateScene();
}

void AeSysView::On3dViewsTop() {
  m_ViewTransform.SetPosition(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.SetDirection(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitY);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();

  InvalidateScene();
}

void AeSysView::On3dViewsBottom() {
  m_ViewTransform.SetPosition(-EoGeVector3d::positiveUnitZ);
  m_ViewTransform.SetDirection(-EoGeVector3d::positiveUnitZ);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitY);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateScene();
}

void AeSysView::On3dViewsLeft() {
  m_ViewTransform.SetPosition(-EoGeVector3d::positiveUnitX);
  m_ViewTransform.SetDirection(-EoGeVector3d::positiveUnitX);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateScene();
}

void AeSysView::On3dViewsRight() {
  m_ViewTransform.SetPosition(EoGeVector3d::positiveUnitX);
  m_ViewTransform.SetDirection(EoGeVector3d::positiveUnitX);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateScene();
}

void AeSysView::On3dViewsFront() {
  m_ViewTransform.SetPosition(-EoGeVector3d::positiveUnitY);
  m_ViewTransform.SetDirection(-EoGeVector3d::positiveUnitY);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateScene();
}

void AeSysView::On3dViewsBack() {
  m_ViewTransform.SetPosition(EoGeVector3d::positiveUnitY);
  m_ViewTransform.SetDirection(EoGeVector3d::positiveUnitY);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateScene();
}

void AeSysView::On3dViewsIsometric() {
  static int iLeftRight = 0;
  static int iFrontBack = 0;
  static int iAboveUnder = 0;

  EoDlgSelectIsometricView Dialog;
  Dialog.m_LeftRight = iLeftRight;
  Dialog.m_FrontBack = iFrontBack;
  Dialog.m_AboveUnder = iAboveUnder;
  if (Dialog.DoModal()) {
    iLeftRight = Dialog.m_LeftRight;
    iFrontBack = Dialog.m_FrontBack;
    iAboveUnder = Dialog.m_AboveUnder;

    EoGeVector3d Direction;

    Direction.x = iLeftRight == 0 ? 0.5773503 : -0.5773503;
    Direction.y = iFrontBack == 0 ? 0.5773503 : -0.5773503;
    Direction.z = iAboveUnder == 0 ? -0.5773503 : 0.5773503;

    m_ViewTransform.SetPosition(-Direction);
    m_ViewTransform.SetDirection(-Direction);
    m_ViewTransform.EnablePerspective(false);

    auto viewUp = CrossProduct(Direction, EoGeVector3d::positiveUnitZ);
    viewUp = CrossProduct(viewUp, Direction);
    viewUp.Unitize();

    m_ViewTransform.SetViewUp(viewUp);
    m_ViewTransform.SetCenteredWindow(m_Viewport, 0.0, 0.0);
  }
  InvalidateScene();
}

void AeSysView::OnCameraRotateLeft() { DoCameraRotate(ID_CAMERA_ROTATELEFT); }

void AeSysView::OnCameraRotateRight() { DoCameraRotate(ID_CAMERA_ROTATERIGHT); }

void AeSysView::OnCameraRotateUp() { DoCameraRotate(ID_CAMERA_ROTATEUP); }

void AeSysView::OnCameraRotateDown() { DoCameraRotate(ID_CAMERA_ROTATEDOWN); }

void AeSysView::OnViewParameters() {
  EoDlgViewParameters Dialog;

  EoGsViewTransform ModelView(m_ViewTransform);

  Dialog.m_ModelView = reinterpret_cast<uintptr_t>(&ModelView);
  Dialog.m_PerspectiveProjection = m_ViewTransform.IsPerspectiveOn();

  if (Dialog.DoModal() == IDOK) { m_ViewTransform.EnablePerspective(Dialog.m_PerspectiveProjection == TRUE); }
}

void AeSysView::OnViewLighting() {}

void AeSysView::OnWindowNormal() {
  CopyActiveModelViewToPreviousModelView();
  DoWindowPan(1.0);
}

void AeSysView::OnWindowBest() {
  auto* document = GetDocument();
  EoGePoint3d ptMin;
  EoGePoint3d ptMax;

  document->GetExtents(this, ptMin, ptMax, ModelViewGetMatrix());

  if (ptMin.x < ptMax.x) {
    m_PreviousViewTransform = m_ViewTransform;

    double UExtent = m_ViewTransform.UExtent() * (ptMax.x - ptMin.x) / 2.0;
    double VExtent = m_ViewTransform.VExtent() * (ptMax.y - ptMin.y) / 2.0;

    m_ViewTransform.SetCenteredWindow(m_Viewport, UExtent, VExtent);

    EoGeTransformMatrix transformMatrix;
    document->GetExtents(this, ptMin, ptMax, transformMatrix);

    EoGePoint3d Target = EoGePoint3d((ptMin.x + ptMax.x) / 2.0, (ptMin.y + ptMax.y) / 2.0, (ptMin.z + ptMax.z) / 2.0);

    m_ViewTransform.SetTarget(Target);
    m_ViewTransform.SetPosition(m_ViewTransform.Direction());
    m_ViewTransform.BuildTransformMatrix();

    ViewZoomExtents();

    SetCursorPosition(Target);
    InvalidateScene();
  }
}

void AeSysView::OnWindowLast() {
  ExchangeActiveAndPreviousModelViews();
  InvalidateScene();
}

void AeSysView::OnWindowSheet() {
  ModelViewInitialize();
  InvalidateScene();
}

void AeSysView::OnSheetSetupFormFactor() {
  EoDlgSheetSetupFormFactor dialog(this);

  if (dialog.DoModal() != IDOK) { return; }

  const double sheetWidth = dialog.SheetWidth();
  const double sheetHeight = dialog.SheetHeight();

  auto* document = GetDocument();
  if (document == nullptr) { return; }

  // Find existing paper-space viewport (ID >= 2) or create one
  EoDbViewport* viewport = nullptr;
  auto& paperLayers = document->PaperSpaceLayers();
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
          auto* candidate = static_cast<EoDbViewport*>(primitive);
          if (candidate->ViewportId() >= 2) {
            viewport = candidate;
            break;
          }
        }
      }
      if (viewport != nullptr) { break; }
    }
    if (viewport != nullptr) { break; }
  }

  if (viewport != nullptr) {
    // Update existing viewport — paper-space geometry and model-space view
    viewport->SetWidth(sheetWidth);
    viewport->SetHeight(sheetHeight);

    // Keep the model-space view center but adjust viewHeight to match new sheet aspect
    const double currentViewHeight = viewport->ViewHeight();

    // Scale model-space view to fit the new sheet proportions
    viewport->SetViewHeight(currentViewHeight);
  } else {
    // No viewport exists — create one (same pattern as CreateDefaultPaperSpaceViewport)
    const auto viewCenter = CameraTarget();

    viewport = new EoDbViewport();
    viewport->SetCenterPoint(EoGePoint3d{viewCenter.x, viewCenter.y, 0.0});
    viewport->SetWidth(sheetWidth);
    viewport->SetHeight(sheetHeight);
    viewport->SetViewportStatus(1);
    viewport->SetViewportId(2);
    viewport->SetViewCenter(EoGePoint3d{viewCenter.x, viewCenter.y, 0.0});
    viewport->SetViewDirection(EoGePoint3d{0.0, 0.0, 1.0});
    viewport->SetViewTargetPoint(EoGePoint3d{viewCenter.x, viewCenter.y, 0.0});
    viewport->SetViewHeight(sheetHeight);
    viewport->SetLensLength(50.0);
    viewport->SetTwistAngle(0.0);

    document->RegisterHandle(viewport);

    auto* viewportGroup = new EoDbGroup();
    viewportGroup->AddTail(viewport);

    auto* paperLayer0 = document->FindLayerInSpace(L"0", EoDxf::Space::PaperSpace);
    if (paperLayer0 == nullptr) {
      constexpr auto layerState = static_cast<std::uint16_t>(
          std::to_underlying(EoDbLayer::State::isResident) | std::to_underlying(EoDbLayer::State::isInternal));
      paperLayer0 = new EoDbLayer(L"0", layerState);
      document->AddLayerToSpace(paperLayer0, EoDxf::Space::PaperSpace);
    }
    paperLayer0->AddTail(viewportGroup);
  }

  document->SetModifiedFlag(TRUE);
  InvalidateScene();
}

void AeSysView::OnWindowZoomIn() {
  if (IsViewportActive()) {
    if (m_activeViewportPrimitive->IsDisplayLocked()) { return; }

    // Cursor-anchored zoom: project WCS cursor to DCS 2D before interpolating
    // with the DCS ViewCenter, so the point under the cursor stays stationary.
    constexpr double factor = 0.9;
    const auto cursorWCS = GetCursorPosition();
    const auto& oldCenter = m_activeViewportPrimitive->ViewCenter();

    EoGeVector3d dcsX;
    EoGeVector3d dcsY;
    EoGePoint3d wcsCameraTarget;
    m_activeViewportPrimitive->ComputeViewPlaneAxes(dcsX, dcsY, wcsCameraTarget);

    // Project WCS cursor onto the DCS plane
    const EoGeVector3d offset(m_activeViewportPrimitive->ViewTargetPoint(), cursorWCS);
    const double dcsCursorX = DotProduct(offset, dcsX);
    const double dcsCursorY = DotProduct(offset, dcsY);

    EoGePoint3d newCenter(
        dcsCursorX + (oldCenter.x - dcsCursorX) * factor,
        dcsCursorY + (oldCenter.y - dcsCursorY) * factor,
        oldCenter.z);
    m_activeViewportPrimitive->SetViewCenter(newCenter);
    m_activeViewportPrimitive->SetViewHeight(m_activeViewportPrimitive->ViewHeight() * factor);
    InvalidateScene();
    return;
  }
  DoWindowPan(m_Viewport.WidthInInches() / m_ViewTransform.UExtent() / 0.9);
}

void AeSysView::OnWindowZoomOut() {
  if (IsViewportActive()) {
    if (m_activeViewportPrimitive->IsDisplayLocked()) { return; }

    // Cursor-anchored zoom: project WCS cursor to DCS 2D before interpolating
    // with the DCS ViewCenter, so the point under the cursor stays stationary.
    constexpr double factor = 1.0 / 0.9;
    const auto cursorWCS = GetCursorPosition();
    const auto& oldCenter = m_activeViewportPrimitive->ViewCenter();

    EoGeVector3d dcsX;
    EoGeVector3d dcsY;
    EoGePoint3d wcsCameraTarget;
    m_activeViewportPrimitive->ComputeViewPlaneAxes(dcsX, dcsY, wcsCameraTarget);

    // Project WCS cursor onto the DCS plane
    const EoGeVector3d offset(m_activeViewportPrimitive->ViewTargetPoint(), cursorWCS);
    const double dcsCursorX = DotProduct(offset, dcsX);
    const double dcsCursorY = DotProduct(offset, dcsY);

    EoGePoint3d newCenter(
        dcsCursorX + (oldCenter.x - dcsCursorX) * factor,
        dcsCursorY + (oldCenter.y - dcsCursorY) * factor,
        oldCenter.z);
    m_activeViewportPrimitive->SetViewCenter(newCenter);
    m_activeViewportPrimitive->SetViewHeight(m_activeViewportPrimitive->ViewHeight() * factor);
    InvalidateScene();
    return;
  }
  DoWindowPan(m_Viewport.WidthInInches() / m_ViewTransform.UExtent() * 0.9);
}

void AeSysView::OnWindowPan() {
  CopyActiveModelViewToPreviousModelView();
  DoWindowPan(m_Viewport.WidthInInches() / m_ViewTransform.UExtent());
  InvalidateScene();
}

void AeSysView::OnWindowPanLeft() {
  auto Target = m_ViewTransform.Target();

  Target.x -= 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateScene();
}

void AeSysView::OnWindowPanRight() {
  auto Target = m_ViewTransform.Target();

  Target.x += 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateScene();
}

void AeSysView::OnWindowPanUp() {
  auto Target = m_ViewTransform.Target();

  Target.y += 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateScene();
}

void AeSysView::OnWindowPanDown() {
  auto Target = m_ViewTransform.Target();

  Target.y -= 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateScene();
}

void AeSysView::OnWindowZoomSpecial() {
  EoDlgViewZoom dlg(this);

  dlg.m_Ratio = m_Viewport.WidthInInches() / m_ViewTransform.UExtent();

  if (dlg.DoModal() == IDOK) {
    CopyActiveModelViewToPreviousModelView();
    DoWindowPan(dlg.m_Ratio);
    InvalidateScene();
  }
}
