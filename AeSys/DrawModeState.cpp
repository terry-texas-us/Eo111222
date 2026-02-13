#include "Stdafx.h"
#if defined(USING_STATE_PATTERN)
#include "AeSysDoc.h"
#include "AeSysView.h"

#include "DrawModeState.h"

#include "EoDb.h"  // For EoDbGroup, EoDbPrimitive, etc.
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolyline.h"
#include "EoGePoint3d.h"
#include "EoGsRenderState.h"  // state if global; consider migrating to member
#include "Resource.h"         // For command IDs

void DrawModeState::OnEnter(AeSysView* context) {
  // Migrated setup: Highlight draw menu, set cursor
  context->SetModeCursor(IDR_DRAW_MODE);  // Draw cursor
  // ModeLineHighlightOp(ID_DRAW_MODE);  // Adapt your UI
  m_pts.RemoveAll();
  m_PreviousDrawCommand = 0;
}

void DrawModeState::OnExit(AeSysView* context) {
  // Cleanup: Clear previews, unhighlight
  context->m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  context->GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &context->m_PreviewGroup);
  m_pts.RemoveAll();
  // ModeLineUnhighlightOp(m_PreviousDrawCommand);
}

void DrawModeState::HandleCommand(AeSysView* context, UINT nID) {
  auto* doc = context->GetDocument();
  EoGePoint3d cursorPos = context->GetCursorPosition();

  switch (nID) {
    case ID_DRAW_MODE_POINT: {
      // Migrated from OnDrawModePoint
      auto* prim = new EoDbPoint(renderState.Color(), renderState.PointStyle(), cursorPos);  // Adapt renderState
      auto* group = new EoDbGroup(prim);
      doc->AddWorkLayerGroup(group);
      doc->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    } break;
    case ID_DRAW_MODE_LINE:
      // Migrated logic: Handle pts accumulation
      if (m_PreviousDrawCommand != ID_OP2) {  // Start new line
        m_pts.RemoveAll();
        m_pts.Add(cursorPos);
        m_PreviousDrawCommand = ID_OP2;  // Update UI
      } else {                           // Complete line
        cursorPos = context->SnapPointToAxis(m_pts[0], cursorPos);
        auto* line =
            EoDbLine::CreateLine(m_pts[0], cursorPos)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
        auto* group = new EoDbGroup(line);
        doc->AddWorkLayerGroup(group);
        doc->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        m_pts[0] = cursorPos;  // Chain for next segment
      }
      break;
    // Migrate others: Polygon, Arc, etc. For DXF: Map to libdxfrw entities with handles
    case ID_DRAW_MODE_ESCAPE:  // Or return
      context->PopState();     // Exit to idle/previous
      break;
    default:
      break;
  }
}

void DrawModeState::OnMouseMove([[maybe_unused]] AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  // Migrated from DoDrawModeMouseMove: Preview rubberbanding
  EoGePoint3d cursorPos = context->GetCursorPosition();
  auto* doc = context->GetDocument();

  if (m_PreviousDrawCommand == ID_OP2 && m_pts.GetSize() > 0) {  // Line preview
    cursorPos = context->SnapPointToAxis(m_pts[0], cursorPos);
    doc->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &context->m_PreviewGroup);
    context->m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    EoGePoint3dArray previewPts;
    previewPts.Copy(m_pts);
    previewPts.Add(cursorPos);
    context->m_PreviewGroup.AddTail(new EoDbPolyline(previewPts));  // Temp preview
    doc->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &context->m_PreviewGroup);
  }
  // Add cases for other primitives (e.g., arc preview with handles for future DXF)
}

void DrawModeState::OnLButtonDown([[maybe_unused]] AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  // If draw mode uses clicks for points
  EoGePoint3d cursorPos = context->GetCursorPosition();
  // e.g., Add to m_pts and trigger command logic
  HandleCommand(context, m_PreviousDrawCommand);  // Or specific
}

void DrawModeState::OnUpdate([[maybe_unused]] AeSysView* context, [[maybe_unused]] CView* sender, [[maybe_unused]] LPARAM hint, [[maybe_unused]] CObject* hintObject) {
  // Mode-specific update: e.g., redraw previews on hint
  if ((hint & EoDb::kGroupEraseSafe) == EoDb::kGroupEraseSafe) {
    // Custom erase/redraw for draw previews
  }
  // For DXF: Handle entity updates with handle resolution (e.g., layer ref)
}
#endif