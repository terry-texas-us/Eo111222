#include "Stdafx.h"

#include <cstdint>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoMsDraw.h"
#include "EoDb.h"
#include "EoDbConic.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPolyline.h"
#include "EoDbSpline.h"
#include "EoDlgBlockInsert.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "EoGsRenderState.h"
#include "Resource.h"

// draw-mode handlers alias DrawModeState::Points() as a local `pts` reference to keep the body unchanged from when
// `pts` was an AeSysView member.
#pragma warning(disable : 4458)

namespace {
// Returns the active DrawModeState from the view's state stack, or nullptr when called from outside draw mode (e.g.
// during PopAllModeStates teardown).
DrawModeState* DrawState(AeSysView* view) {
  return dynamic_cast<DrawModeState*>(view->GetCurrentState());
}
}  // namespace

void AeSysView::OnDrawModeOptions() {
  AeSysDoc::GetDoc()->OnSetupOptionsDraw();
}

void AeSysView::OnDrawModePoint() {
  const auto cursorPosition = GetCursorPosition();

  auto* group = new EoDbGroup(new EoDbPoint(Gs::renderState.Color(), Gs::renderState.PointStyle(), cursorPosition));
  auto* document = GetDocument();
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

void AeSysView::OnDrawModeLine() {
  auto* drawState = DrawState(this);
  if (drawState == nullptr) { return; }
  auto& pts = drawState->Points();
  auto cursorPosition = GetCursorPosition();
  if (drawState->PreviousDrawCommand() != ID_OP2) {
    pts.RemoveAll();
    pts.Add(cursorPosition);
    drawState->SetPreviousDrawCommand(ModeLineHighlightOp(ID_OP2));
  } else {
    auto* document = GetDocument();
    cursorPosition = SnapPointToAxis(pts[0], cursorPosition);

    auto* group = new EoDbGroup(EoDbLine::CreateLine(pts[0], cursorPosition)->WithProperties(Gs::renderState));
    document->AddWorkLayerGroup(group);
    pts[0] = cursorPosition;
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateScene();
  }
}

void AeSysView::OnDrawModePolygon() {
  auto* drawState = DrawState(this);
  if (drawState == nullptr) { return; }
  auto& pts = drawState->Points();
  auto cursorPosition = GetCursorPosition();

  if (drawState->PreviousDrawCommand() != ID_OP3) {
    drawState->SetPreviousDrawCommand(ModeLineHighlightOp(ID_OP3));
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    const auto numberOfPoints = pts.GetSize();

    if (pts[numberOfPoints - 1] != cursorPosition) {
      cursorPosition = SnapPointToAxis(pts[numberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);
    }
  }
}

void AeSysView::OnDrawModeQuad() {
  auto* drawState = DrawState(this);
  if (drawState == nullptr) { return; }
  auto& pts = drawState->Points();
  const auto cursorPosition = GetCursorPosition();

  if (drawState->PreviousDrawCommand() != ID_OP4) {
    drawState->SetPreviousDrawCommand(ModeLineHighlightOp(ID_OP4));
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeArc() {
  auto* drawState = DrawState(this);
  if (drawState == nullptr) { return; }
  auto& pts = drawState->Points();
  const auto cursorPosition = GetCursorPosition();

  if (drawState->PreviousDrawCommand() != ID_OP5) {
    drawState->SetPreviousDrawCommand(ModeLineHighlightOp(ID_OP5));
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeBspline() {
  auto* drawState = DrawState(this);
  if (drawState == nullptr) { return; }
  auto& pts = drawState->Points();
  const auto cursorPosition = GetCursorPosition();

  if (drawState->PreviousDrawCommand() != ID_OP6) {
    drawState->SetPreviousDrawCommand(ModeLineHighlightOp(ID_OP6));
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    pts.Add(cursorPosition);
  }
}

void AeSysView::OnDrawModeCircle() {
  auto* drawState = DrawState(this);
  if (drawState == nullptr) { return; }
  auto& pts = drawState->Points();
  const auto cursorPosition = GetCursorPosition();

  if (drawState->PreviousDrawCommand() != ID_OP7) {
    drawState->SetPreviousDrawCommand(ModeLineHighlightOp(ID_OP7));
    pts.RemoveAll();
    pts.Add(cursorPosition);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeEllipse() {
  auto* drawState = DrawState(this);
  if (drawState == nullptr) { return; }
  auto& pts = drawState->Points();
  const auto cursorPosition = GetCursorPosition();

  if (drawState->PreviousDrawCommand() != ID_OP8) {
    drawState->SetPreviousDrawCommand(ModeLineHighlightOp(ID_OP8));
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeInsert() {
  auto* document = GetDocument();
  if (document->BlockTableSize() > 0) {
    EoDlgBlockInsert dialog(document);
    dialog.DoModal();
  }
}

void AeSysView::OnDrawModeReturn() {
  auto* drawState = DrawState(this);
  if (drawState == nullptr) { return; }
  auto& pts = drawState->Points();
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  std::uint16_t previousDrawCommand = drawState->PreviousDrawCommand();

  const auto numberOfPoints = pts.GetSize();
  EoDbGroup* group{};

  switch (previousDrawCommand) {
    case ID_OP2:
      cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
      group = new EoDbGroup(EoDbLine::CreateLine(pts[0], cursorPosition)->WithProperties(Gs::renderState));
      break;

    case ID_OP3:
      if (numberOfPoints == 1) { return; }

      if (pts[numberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      cursorPosition = SnapPointToAxis(pts[numberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);
      group = new EoDbGroup(new EoDbPolygon(pts));
      break;

    case ID_OP4:
      if (pts[numberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      cursorPosition = SnapPointToAxis(pts[numberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);

      if (numberOfPoints == 1) { return; }

      pts.Add(pts[0] + EoGeVector3d(pts[1], pts[2]));

      group = new EoDbGroup;

      for (int i = 0; i < 4; i++) {
        group->AddTail(EoDbLine::CreateLine(pts[i], pts[(i + 1) % 4])->WithProperties(Gs::renderState));
      }
      break;

    case ID_OP5: {
      if (pts[numberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      if (numberOfPoints == 1) {
        // Second click — record intermediate point and wait for the end point.
        pts.Add(cursorPosition);
        return;
      }
      // Third click — validate before committing so a colinear click is ignored
      // and the gesture stays alive with pts[0..1] intact.
      EoGePoint3d start{pts[0]};
      const EoGePoint3d intermediate{pts[1]};
      EoGePoint3d end{cursorPosition};
      auto* radialArc = EoDbConic::CreateRadialArcFrom3Points(start, intermediate, end);
      if (radialArc == nullptr) {
        app.AddStringToMessageList(IDS_MSG_PTS_COLINEAR);
        return;  // Do NOT add the point — gesture stays alive with pts[0..1] intact.
      }
      if (std::abs(radialArc->SweepAngle()) < Eo::geometricTolerance) {
        delete radialArc;
        app.AddStringToMessageList(IDS_MSG_PTS_COLINEAR);
        return;  // Same — do not add.
      }
      radialArc->SetColor(Gs::renderState.Color());
      radialArc->SetLineTypeName(Gs::renderState.LineTypeName());
      pts.Add(cursorPosition);  // pts[2] committed only after validation passes.
      group = new EoDbGroup(radialArc);
      break;
    }
    case ID_OP6:
      if (numberOfPoints == 1) { return; }

      pts.Add(cursorPosition);

      group = new EoDbGroup(new EoDbSpline(pts));
      break;

    case ID_OP7: {
      const double radius = EoGePoint3d::Distance(pts[0], cursorPosition);
      if (radius < Eo::geometricTolerance) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      group = new EoDbGroup(EoDbConic::CreateCircleInView(pts[0], radius));
    } break;

    case ID_OP8: {
      if (pts[numberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      if (numberOfPoints == 1) {
        // Second click — record major-axis endpoint and wait for the third click.
        pts.Add(cursorPosition);
        SetCursorPosition(pts[0]);
        return;
      }
      // Third click — validate minor axis before committing so a degenerate click
      // is silently ignored and the user can try again without resetting the gesture.
      const EoGeVector3d majorAxis(pts[0], pts[1]);
      const EoGeVector3d minorAxis(pts[0], cursorPosition);
      if (minorAxis.Length() < Eo::geometricTolerance) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;  // Do NOT add the point — gesture stays alive with pts[0..1] intact.
      }
      pts.Add(cursorPosition);  // pts[2] committed only after validation passes.
      auto extrusion = CrossProduct(majorAxis, minorAxis);
      extrusion.Unitize();
      const double ratio = minorAxis.Length() / majorAxis.Length();
      auto* ellipse = EoDbConic::CreateEllipse(pts[0], extrusion, majorAxis, ratio);
      ellipse->SetColor(Gs::renderState.Color());
      ellipse->SetLineTypeName(Gs::renderState.LineTypeName());

      group = new EoDbGroup(ellipse);

      break;
    }
    default:
      return;
  }
  document->AddWorkLayerGroup(group);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateScene();
  pts.RemoveAll();
  ModeLineUnhighlightOp(previousDrawCommand);
  if (drawState != nullptr) { drawState->SetPreviousDrawCommand(0); }
}

void AeSysView::OnDrawModeEscape() {
  auto* drawState = DrawState(this);
  auto previousDrawCommand = drawState != nullptr ? drawState->PreviousDrawCommand() : std::uint16_t{0};
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateOverlay();
  if (drawState != nullptr) { drawState->Points().RemoveAll(); }
  ModeLineUnhighlightOp(previousDrawCommand);
  if (drawState != nullptr) { drawState->SetPreviousDrawCommand(0); }
}

void AeSysView::OnDrawModeFinish() {
  // RMB commit: close the current gesture from already-collected pts only.
  // No cursor position is appended — this distinguishes RMB from Enter (OnDrawModeReturn).
  auto* drawState = DrawState(this);
  if (drawState == nullptr) { return; }
  auto previousDrawCommand = drawState->PreviousDrawCommand();
  if (previousDrawCommand == 0) { return; }
  auto& pts = drawState->Points();
  const auto numberOfPoints = pts.GetSize();
  auto* document = GetDocument();
  EoDbGroup* group{};

  switch (previousDrawCommand) {
    case ID_OP3: {
      // Polygon — need at least 3 points to close.
      if (numberOfPoints >= 3) {
        auto* primitive = (new EoDbPolygon(pts))->WithProperties(Gs::renderState);
        group = new EoDbGroup(primitive);
      }
      break;
    }
    case ID_OP6: {
      // B-spline — need at least 2 control points.
      if (numberOfPoints >= 2) {
        auto* primitive = (new EoDbSpline(pts))->WithProperties(Gs::renderState);
        group = new EoDbGroup(primitive);
      }
      break;
    }

    default:
      // All other ops (line, arc, quad, circle, ellipse) require the second point to
      // have already been collected via a second LMB/key press — OnReturn handles those.
      // With only 1 point in pts, treat RMB as cancel.
      OnDrawModeEscape();
      return;
  }

  if (group != nullptr) {
    document->AddWorkLayerGroup(group);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateScene();
    pts.RemoveAll();
    ModeLineUnhighlightOp(previousDrawCommand);
    drawState->SetPreviousDrawCommand(0);
  } else {
    // Not enough points — cancel silently.
    OnDrawModeEscape();
  }
}

void AeSysView::OnDrawModeShiftReturn() {
  auto* drawState = DrawState(this);
  if (drawState == nullptr) { return; }
  auto& pts = drawState->Points();
  auto previousDrawCommand = drawState->PreviousDrawCommand();
  if (previousDrawCommand == ID_OP3 && pts.GetSize() >= 2) {
    // Include cursor position as the final vertex so ShiftReturn commits to
    // where the cursor is now, matching the same behaviour as Return.
    const auto cursorPosition = SnapPointToAxis(pts[pts.GetSize() - 1], GetCursorPosition());
    if (cursorPosition != pts[pts.GetSize() - 1]) { pts.Add(cursorPosition); }

    auto* group = new EoDbGroup(new EoDbPolyline(pts));
    auto* document = GetDocument();
    document->AddWorkLayerGroup(group);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
  }
  ModeLineUnhighlightOp(previousDrawCommand);
  drawState->SetPreviousDrawCommand(0);
  pts.RemoveAll();
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateScene();
}
void AeSysView::OnDrawCommand(UINT command) {
  auto* state = GetCurrentState();
  if (state != nullptr) { [[maybe_unused]] auto consumed = state->HandleCommand(this, command); }
}
// DoDrawModeMouseMove has been removed — preview logic now lives in DrawModeState::OnMouseMove.
