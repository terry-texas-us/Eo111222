#include "Stdafx.h"

#include "AeSysView.h"
#include "AnnotateModeState.h"
#include "EoDbLine.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoGePoint3d.h"

void AnnotateModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"AnnotateModeState::OnExit\n");
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
  context->ModeLineUnhighlightOp(m_previousOp);
}

void AnnotateModeState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  const auto previousOp = m_previousOp;
  if (previousOp == 0) { return; }

  const EoDbHandleSuppressionScope suppressHandles;
  const auto cursorPosition = context->GetCursorPosition();
  const auto numberOfPoints = m_points.GetSize();

  // Build a local preview copy — never mutate m_points from OnMouseMove.
  EoGePoint3dArray previewPoints{m_points};
  previewPoints.Add(cursorPosition);

  context->PreviewGroup().DeletePrimitivesAndRemoveAll();

  switch (previousOp) {
    case ID_OP2:
    case ID_OP3:
      if (m_points[0] != cursorPosition) {
        if (previousOp == ID_OP3) {
          context->GenerateLineEndItem(context->EndItemType(), context->EndItemSize(), cursorPosition, m_points[0], &context->PreviewGroup());
        }
        context->PreviewGroup().AddTail(new EoDbPolyline(previewPoints));
      }
      break;

    case ID_OP4:
    case ID_OP5: {
      const auto savedFirst = previewPoints[0];
      if (context->CorrectLeaderEndpoints(previousOp, 0, previewPoints[0], previewPoints[1])) {
        context->PreviewGroup().AddTail(new EoDbPolyline(previewPoints));
      }
      // Restore in case previewPoints[0] was modified by CorrectLeaderEndpoints.
      previewPoints[0] = savedFirst;
    } break;

    case ID_OP9:
      if (numberOfPoints == 0) { break; }
      if (m_points[0] != cursorPosition) {
        auto snapped = context->SnapPointToAxis(m_points[0], cursorPosition);
        previewPoints.Add(m_points[0].ProjectToward(snapped, 48.0));
        previewPoints.Add(previewPoints[2].ProjectToward(m_points[0], 96.0));
        context->PreviewGroup().AddTail(EoDbLine::CreateLine(previewPoints[2], previewPoints[3])->WithProperties(15, L"Dash2"));
      }
      break;
  }
  context->InvalidateOverlay();
}

bool AnnotateModeState::OnEscape(AeSysView* context) {
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
  m_points.RemoveAll();
  context->ModeLineUnhighlightOp(m_previousOp);
  return true;
}

void AnnotateModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}
