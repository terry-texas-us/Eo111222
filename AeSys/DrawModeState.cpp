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
  ATLTRACE2(traceGeneral, 2, L"DrawModeState::OnEnter\n");
  context->SetModeCursor(IDR_DRAW_MODE);  // Draw cursor
  // ModeLineHighlightOp(ID_DRAW_MODE);  // Adapt your UI
  m_pts.RemoveAll();
  m_previousDrawCommand = 0;
}

void DrawModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"DrawModeState::OnExit\n");
  context->m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  context->GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &context->m_PreviewGroup);
  m_pts.RemoveAll();
  // ModeLineUnhighlightOp(m_PreviousDrawCommand);
}

void DrawModeState::HandleCommand(AeSysView* context, UINT command) {
  ATLTRACE2(traceGeneral, 2, L"DrawModeState::HandleCommand - command: %u\n", command);
  auto* document = context->GetDocument();
  auto cursorPosition = context->GetCursorPosition();

  switch (command) {
    case ID_DRAW_MODE_OPTIONS:
      document->OnSetupOptionsDraw();
      break;
    case ID_DRAW_MODE_POINT: {
      auto* pointPrimitive = new EoDbPoint(renderState.Color(), renderState.PointStyle(), cursorPosition);
      auto* group = new EoDbGroup(pointPrimitive);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    } break;
    case ID_DRAW_MODE_LINE:
      if (m_previousDrawCommand != ID_OP2) {  // Start new line
        m_pts.RemoveAll();
        m_pts.Add(cursorPosition);
        m_previousDrawCommand = ID_OP2;  // Update UI
      } else {                           // Complete line
        cursorPosition = context->SnapPointToAxis(m_pts[0], cursorPosition);
        auto* line = EoDbLine::CreateLine(m_pts[0], cursorPosition)
                         ->WithProperties(renderState.Color(), renderState.LineTypeIndex());
        auto* group = new EoDbGroup(line);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        m_pts[0] = cursorPosition;  // Chain for next segment
        context->m_PreviewGroup.DeletePrimitivesAndRemoveAll();
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

void DrawModeState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  ATLTRACE2(traceGeneral, 2, L"DrawModeState::OnMouseMove - flags: %u, point: (%d, %d)\n", flags, point.x, point.y);
  EoGePoint3d cursorPos = context->GetCursorPosition();
  auto* doc = context->GetDocument();

  if (m_previousDrawCommand == ID_OP2 && m_pts.GetSize() > 0) {  // Line preview
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

void DrawModeState::OnLButtonDown(
    [[maybe_unused]] AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  // If draw mode uses clicks for points
  auto cursorPosition = context->GetCursorPosition();
  // e.g., Add to m_pts and trigger command logic
  HandleCommand(context, m_previousDrawCommand);  // Or specific
}

void DrawModeState::OnDraw([[maybe_unused]] AeSysView* context, [[maybe_unused]] CDC* deviceContext) {
  ATLTRACE2(traceGeneral, 2, L"DrawModeState::OnDraw\n");
  auto* document = context->GetDocument();
  document->DisplayAllLayers(context, deviceContext);
  document->DisplayUniquePoints();

  // @todo add mode-specific overlays (e.g., rubber-band preview)
  // The preview group is already being managed in OnMouseMove
}

bool DrawModeState::OnUpdate(AeSysView* context, [[maybe_unused]] CView* sender, LPARAM hint, CObject* objectHint) {
  ATLTRACE2(traceGeneral, 2, L"DrawModeState::OnUpdate\n");
  if ((hint & EoDb::kGroupEraseSafe) == EoDb::kGroupEraseSafe && objectHint == &context->m_PreviewGroup) {
    return true;
  }
  // Add other draw-specific hints (e.g., for new points/lines)
  return false;  // Not handled, fallback to base switch for full Display
}
#endif