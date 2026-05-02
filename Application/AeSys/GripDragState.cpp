#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbGroup.h"
#include "EoDbPrimitive.h"
#include "GripDragState.h"
#include "Resource.h"

GripDragState::~GripDragState() {
  DeleteCopy();
}

void GripDragState::DeleteCopy() noexcept {
  delete m_originalCopy;
  m_originalCopy = nullptr;
}

void GripDragState::OnEnter([[maybe_unused]] AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"GripDragState::OnEnter\n");
  m_primitive->Copy(m_originalCopy);
  // Preserve the primary mode UI bar — grip drag is a lightweight transient overlay.
}

void GripDragState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"GripDragState::OnExit\n");
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->RubberBandingDisable();
  DeleteCopy();
}

void GripDragState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT flags, CPoint point) {
  if (m_originalCopy == nullptr) { return; }

  auto* document = context->GetDocument();
  const auto cursorPosition = context->GetCursorPosition();

  // --- Endpoint snap scan ---
  // Scan every visible primitive's control points in device space. If any falls
  // within kSnapRadiusPx pixels of the current device cursor, snap the drag target
  // to that exact world-space position. Skip the control point we are dragging on
  // the primitive being edited (it would always snap to its current dragged position).
  m_isSnapped = false;
  if (document != nullptr) {
    int bestDistSq{kSnapRadiusPx * kSnapRadiusPx + 1};  // one beyond threshold
    auto visiblePos = context->GetFirstVisibleGroupPosition();
    while (visiblePos != nullptr) {
      auto* group = context->GetNextVisibleGroup(visiblePos);
      if (group == nullptr) { continue; }
      auto primPos = group->GetHeadPosition();
      while (primPos != nullptr) {
        auto* prim = group->GetNext(primPos);
        if (prim == nullptr) { continue; }
        EoGePoint3dArray pts;
        prim->GetAllPoints(pts);
        for (int i = 0; i < static_cast<int>(pts.GetSize()); ++i) {
          // Skip all control points on the primitive being dragged — avoids
          // self-snap for both endpoint and midpoint (whole-line) grips.
          if (prim == m_primitive) { continue; }
          EoGePoint4d ndcPt(pts[i]);
          context->ModelViewTransformPoint(ndcPt);
          if (ndcPt.w < Eo::geometricTolerance) { continue; }
          const CPoint devPt = context->ProjectToClient(ndcPt);
          const int dx = point.x - devPt.x;
          const int dy = point.y - devPt.y;
          const int distSq = dx * dx + dy * dy;
          if (distSq < bestDistSq) {
            bestDistSq = distSq;
            m_snapTarget = pts[i];
            m_isSnapped = true;
          }
        }
      }
    }
  }
  // Use the snapped world position when close enough, otherwise use the live cursor.
  const EoGePoint3d& dragTarget = m_isSnapped ? m_snapTarget : cursorPosition;

  // Restore the primitive to its original state then apply a single absolute translation
  // from the original anchor to the drag target. Absolute delta avoids drift.
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_primitive);
  m_primitive->Assign(m_originalCopy);
  const EoGeVector3d translate{m_anchorPoint, dragTarget};
  m_primitive->TranslateUsingMask(translate, m_controlPointMask);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, m_primitive);
  context->InvalidateScene();
}

bool GripDragState::HandleKeypad(AeSysView* context, UINT nChar, [[maybe_unused]] UINT nRepCnt, [[maybe_unused]] UINT nFlags) {
  if (nChar == VK_RETURN) { return OnReturn(context); }
  if (nChar == VK_ESCAPE) { return OnEscape(context); }
  return false;
}

void GripDragState::OnLButtonDown(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  // LMB while dragging commits the grip to the current cursor position.
  (void)OnReturn(context);
}

void GripDragState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  // RMB reverts the grip drag.
  (void)OnEscape(context);
}

bool GripDragState::OnReturn(AeSysView* context) {
  // Commit: the primitive is already at its dragged position — just discard the copy.
  auto* document = context->GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, m_primitive);
  DeleteCopy();
  context->PopState();
  context->InvalidateScene();
  return true;
}

bool GripDragState::OnEscape(AeSysView* context) {
  // Revert: restore the live primitive from the saved snapshot, then discard copy.
  if (m_originalCopy != nullptr) {
    auto* document = context->GetDocument();
    document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_primitive);
    m_primitive->Assign(m_originalCopy);   // restore original geometry from snapshot
    document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, m_primitive);
    DeleteCopy();
  }
  context->PopState();
  context->InvalidateScene();
  return true;
}

bool GripDragState::ShouldBlockCommand([[maybe_unused]] UINT commandId) const noexcept {
  // Block everything while a grip is being dragged — mode switches would
  // pop the state stack and leave the primitive in a half-moved state.
  return true;
}
