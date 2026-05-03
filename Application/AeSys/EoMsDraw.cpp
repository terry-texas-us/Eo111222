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

bool DrawModeState::BuildContextMenu([[maybe_unused]] AeSysView* context, CMenu& menu) {
  // No gesture in progress — let the dispatcher fall through to the default cancel.
  if (m_previousDrawCommand == 0) { return false; }

  const auto nPoints = m_points.GetSize();

  // Finish (close as Polygon) is available for polygon op with 3+ collected points.
  const bool canFinishPolygon = (m_previousDrawCommand == ID_OP3 && nPoints >= 3);
  // Finish as Polyline (open) is available for polygon op with 2+ collected points.
  const bool canFinishPolyline = (m_previousDrawCommand == ID_OP3 && nPoints >= 2);
  // Finish (close) for b-spline needs 2+ control points.
  const bool canFinishSpline = (m_previousDrawCommand == ID_OP6 && nPoints >= 2);
  const bool canFinish = canFinishPolygon || canFinishSpline;

  // Undo Last Point is available for multi-click ops once at least two points exist.
  const bool canUndoLast = (nPoints >= 2)
      && (m_previousDrawCommand == ID_OP3   // polygon — accumulates vertices
       || m_previousDrawCommand == ID_OP4   // quad — 3-click
       || m_previousDrawCommand == ID_OP5   // arc — 3-click
       || m_previousDrawCommand == ID_OP6   // bspline — accumulates control points
       || m_previousDrawCommand == ID_OP8); // ellipse — 3-click

  const UINT finishFlags    = MF_STRING | (canFinish        ? MF_ENABLED : MF_GRAYED);
  const UINT polylineFlags  = MF_STRING | (canFinishPolyline ? MF_ENABLED : MF_GRAYED);
  const UINT undoLastFlags  = MF_STRING | (canUndoLast       ? MF_ENABLED : MF_GRAYED);

  if (m_previousDrawCommand == ID_OP3) {
    menu.AppendMenu(finishFlags,   ID_DRAW_MODE_FINISH,   L"&Close as Polygon");
    menu.AppendMenu(polylineFlags, ID_DRAW_MODE_POLYLINE, L"Finish as &Polyline");
  } else {
    menu.AppendMenu(finishFlags,   ID_DRAW_MODE_FINISH,   L"&Finish");
  }
  menu.AppendMenu(undoLastFlags, ID_DRAW_MODE_UNDO_LAST, L"Undo &Last Point");
  menu.AppendMenu(MF_SEPARATOR);
  menu.AppendMenu(MF_STRING,     ID_DRAW_MODE_ESCAPE,    L"&Cancel");
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

const wchar_t* DrawModeState::PromptString() const noexcept {
  switch (m_previousDrawCommand) {
    case ID_DRAW_MODE_LINE:     return L"Specify next point or [Undo/Return=finish]";
    case ID_DRAW_MODE_POLYLINE: return L"Specify next vertex or [Undo/Return=finish]";
    case ID_DRAW_MODE_POLYGON:  return L"Specify next vertex or [Return=close/Undo]";
    case ID_DRAW_MODE_QUAD:     return L"Specify quad corner";
    case ID_DRAW_MODE_ARC:      return L"Specify arc pass-through point";
    case ID_DRAW_MODE_CIRCLE:   return L"Specify point on circle circumference";
    case ID_DRAW_MODE_ELLIPSE:  return L"Specify ellipse axis endpoint";
    case ID_DRAW_MODE_BSPLINE:  return L"Specify next spline control point or [Return=finish]";
    default:                    return L"Draw -- choose sub-command (LINE CIRCLE POLYLINE ...)";
  }
}

const wchar_t* DrawModeState::GesturePrompt() const noexcept {
  const auto n = m_points.GetSize();
  switch (m_previousDrawCommand) {
    case ID_OP2:  // Line
      return (n == 0) ? L"Specify first point"
                      : L"Specify next point";
    case ID_OP3:  // Polygon / Polyline
      if (n == 0) { return L"Specify first vertex"; }
      if (n == 1) { return L"Specify next vertex"; }
      return L"Specify next vertex  [Return=close]";  // >= 3: close available
    case ID_OP4:  // Quad
      if (n == 0) { return L"Specify first corner"; }
      if (n == 1) { return L"Specify opposite corner"; }
      return L"Specify quad corner";
    case ID_OP5:  // Arc
      if (n == 0) { return L"Specify arc start point"; }
      if (n == 1) { return L"Specify arc pass-through point"; }
      return L"Specify arc end point";
    case ID_OP6:  // B-Spline
      return (n == 0) ? L"Specify first control point"
                      : L"Specify next control point  [Return=finish]";
    case ID_OP7:  // Circle
      return (n == 0) ? L"Specify center point"
                      : L"Specify point on circumference";
    case ID_OP8:  // Ellipse
      if (n == 0) { return L"Specify ellipse center"; }
      if (n == 1) { return L"Specify major axis endpoint"; }
      return L"Specify minor axis endpoint";
    default:
      return L"";
  }
}
