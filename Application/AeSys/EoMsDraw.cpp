#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"
#include "EoMsDraw.h"
#include "EoDbConic.h"
#include "EoDbPolygon.h"
#include "EoDbPolyline.h"
#include "EoDbSpline.h"
#include "EoGePoint3d.h"
#include "EoGsRenderState.h"
#include "Resource.h"

void DrawModeState::OnEnter(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"DrawModeState::OnEnter\n");
  m_previousDrawCommand = 0;
  // Restore full draw-mode UI when re-entered after a sub-mode pop (e.g. PickAndDragState).
  app.SetModeResourceIdentifier(IDR_DRAW_MODE);
  app.LoadModeResources(ID_MODE_DRAW, context);
}

void DrawModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"DrawModeState::OnExit\n");
  // Single cleanup point for all draw-mode exit paths.
  context->ModeLineUnhighlightOp(m_previousDrawCommand);
  m_previousDrawCommand = 0;
  m_points.RemoveAll();
  context->RubberBandingDisable();
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();
}

void DrawModeState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  const EoDbHandleSuppressionScope suppressHandles;
  auto cursorPosition = context->GetCursorPosition();
  const auto numberOfPoints = m_points.GetSize();

  switch (m_previousDrawCommand) {
    case ID_OP2: {
      VERIFY(numberOfPoints > 0);
      if (numberOfPoints == 0) { break; }

      if (m_points[0] != cursorPosition) {
        cursorPosition = context->SnapPointToAxis(m_points[0], cursorPosition);
        EoGePoint3dArray previewPoints{}; previewPoints.Copy(m_points);
        previewPoints.Add(cursorPosition);

        context->PreviewGroup().DeletePrimitivesAndRemoveAll();
        context->PreviewGroup().AddTail(new EoDbPolyline(previewPoints));
        context->InvalidateOverlay();
      }
    } break;

    case ID_OP3: {
      if (numberOfPoints == 0) { break; }
      cursorPosition = context->SnapPointToAxis(m_points[numberOfPoints - 1], cursorPosition);
      EoGePoint3dArray previewPoints{}; previewPoints.Copy(m_points);
      previewPoints.Add(cursorPosition);

      context->PreviewGroup().DeletePrimitivesAndRemoveAll();
      if (numberOfPoints == 1) {
        context->PreviewGroup().AddTail(new EoDbPolyline(previewPoints));
      } else {
        context->PreviewGroup().AddTail(new EoDbPolygon(previewPoints));
      }
      context->InvalidateOverlay();
    } break;

    case ID_OP4: {
      if (numberOfPoints == 0) { break; }
      cursorPosition = context->SnapPointToAxis(m_points[numberOfPoints - 1], cursorPosition);
      EoGePoint3dArray previewPoints{}; previewPoints.Copy(m_points);
      previewPoints.Add(cursorPosition);

      if (numberOfPoints == 2) {
        previewPoints.Add(m_points[0] + EoGeVector3d(m_points[1], cursorPosition));
        previewPoints.Add(m_points[0]);
      }
      context->PreviewGroup().DeletePrimitivesAndRemoveAll();
      context->PreviewGroup().AddTail(new EoDbPolyline(previewPoints));
      context->InvalidateOverlay();
    } break;

    case ID_OP5: {
      EoGePoint3dArray previewPoints{}; previewPoints.Copy(m_points);
      previewPoints.Add(cursorPosition);

      context->PreviewGroup().DeletePrimitivesAndRemoveAll();

      if (numberOfPoints == 1) { context->PreviewGroup().AddTail(new EoDbPolyline(previewPoints)); }
      if (numberOfPoints == 2) {
        EoGePoint3d start{previewPoints[0]};
        const EoGePoint3d intermediate{previewPoints[1]};
        EoGePoint3d end{previewPoints[2]};
        const auto normal = CrossProduct({start, intermediate}, {start, end});
        if (normal.IsNearNull()) {
          context->PreviewGroup().AddTail(new EoDbPolyline(previewPoints));
        } else {
          auto* radialArc = EoDbConic::CreateRadialArcFrom3Points(start, intermediate, end);
          if (radialArc == nullptr) { break; }
          radialArc->SetColor(Gs::renderState.Color());
          radialArc->SetLineTypeName(Gs::renderState.LineTypeName());
          context->PreviewGroup().AddTail(radialArc);
        }
      }
      context->InvalidateOverlay();
    } break;

    case ID_OP6: {
      EoGePoint3dArray previewPoints{}; previewPoints.Copy(m_points);
      previewPoints.Add(cursorPosition);

      context->PreviewGroup().DeletePrimitivesAndRemoveAll();
      context->PreviewGroup().AddTail(new EoDbSpline(previewPoints));
      context->InvalidateOverlay();
    } break;

    case ID_OP7: {
      if (numberOfPoints == 0) { break; }
      const double radius = EoGePoint3d::Distance(m_points[0], cursorPosition);
      if (radius > Eo::geometricTolerance) {
        auto* circle = EoDbConic::CreateCircleInView(m_points[0], radius);
        if (circle != nullptr) {
          context->PreviewGroup().DeletePrimitivesAndRemoveAll();
          context->PreviewGroup().AddTail(circle);
          context->InvalidateOverlay();
        }
      }
    } break;

    case ID_OP8: {
      if (numberOfPoints == 0) { break; }
      EoGePoint3dArray previewPoints{}; previewPoints.Copy(m_points);
      previewPoints.Add(cursorPosition);

      context->PreviewGroup().DeletePrimitivesAndRemoveAll();
      if (numberOfPoints == 1) {
        context->PreviewGroup().AddTail(new EoDbPolyline(previewPoints));
      } else {
        const EoGeVector3d majorAxis(previewPoints[0], previewPoints[1]);
        const EoGeVector3d minorAxis(previewPoints[0], cursorPosition);

        if (minorAxis.Length() < Eo::geometricTolerance) { break; }
        auto extrusion = CrossProduct(majorAxis, minorAxis);
        extrusion.Unitize();
        const double ratio = minorAxis.Length() / majorAxis.Length();
        auto* ellipse = EoDbConic::CreateEllipse(previewPoints[0], extrusion, majorAxis, ratio);
        ellipse->SetColor(Gs::renderState.Color());
        ellipse->SetLineTypeName(Gs::renderState.LineTypeName());
        context->PreviewGroup().AddTail(ellipse);
      }
      context->InvalidateOverlay();
    } break;
  }
}

bool DrawModeState::OnReturn(AeSysView* context) {
  context->OnDrawModeReturn();
  return true;
}

bool DrawModeState::OnEscape(AeSysView* context) {
  context->OnDrawModeEscape();
  return true;
}

void DrawModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  // RMB finishes the gesture from already-collected points without appending cursor position.
  context->OnDrawModeFinish();
}

bool DrawModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToDrawCommand[] = {
      0,                    // ID_OP0 — no Draw mapping
      0,                    // ID_OP1
      ID_DRAW_MODE_LINE,    // ID_OP2
      ID_DRAW_MODE_POLYGON, // ID_OP3
      ID_DRAW_MODE_QUAD,    // ID_OP4
      ID_DRAW_MODE_ARC,     // ID_OP5
      ID_DRAW_MODE_BSPLINE, // ID_OP6
      ID_DRAW_MODE_CIRCLE,  // ID_OP7
      ID_DRAW_MODE_ELLIPSE, // ID_OP8
      0,                    // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToDrawCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToDrawCommand[opIndex]);
    return true;
  }
  return false;
}
