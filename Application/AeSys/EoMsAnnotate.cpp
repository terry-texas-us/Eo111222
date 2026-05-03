#include "Stdafx.h"

#include "AeSysView.h"
#include "EoMsAnnotate.h"
#include "EoDbLine.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoGePoint3d.h"
#include "Resource.h"

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
  EoGePoint3dArray previewPoints{}; previewPoints.Copy(m_points);
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

void AnnotateModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  // RMB = "break the sequence" — resets the active gesture, ready for a new leader.
  OnEscape(context);
}

bool AnnotateModeState::BuildContextMenu([[maybe_unused]] AeSysView* context, CMenu& menu) {
  // Annotate gestures are single- or two-click; mid-gesture RMB cancels.
  // Only show a menu when a gesture op is active so the user can cancel explicitly.
  if (m_previousOp == 0) { return false; }
  menu.AppendMenu(MF_STRING, ID_ANNOTATE_MODE_ESCAPE, L"C&ancel\tEsc");
  return true;
}

bool AnnotateModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToAnnotateCommand[] = {
      0,                                  // ID_OP0
      0,                                  // ID_OP1
      ID_ANNOTATE_MODE_LINE,              // ID_OP2
      ID_ANNOTATE_MODE_ARROW,             // ID_OP3
      ID_ANNOTATE_MODE_BUBBLE,            // ID_OP4
      ID_ANNOTATE_MODE_HOOK,              // ID_OP5
      ID_ANNOTATE_MODE_UNDERLINE,         // ID_OP6
      ID_ANNOTATE_MODE_BOX,               // ID_OP7
      ID_ANNOTATE_MODE_CUT_IN,            // ID_OP8
      ID_ANNOTATE_MODE_CONSTRUCTION_LINE, // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToAnnotateCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToAnnotateCommand[opIndex]);
    return true;
  }
  return false;
}

void AnnotateModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}

const wchar_t* AnnotateModeState::PromptString() const noexcept {
  switch (m_previousOp) {
    case ID_ANNOTATE_MODE_LINE:               return L"Specify annotation line start point";
    case ID_ANNOTATE_MODE_ARROW:              return L"Specify arrow leader start point";
    case ID_ANNOTATE_MODE_BUBBLE:             return L"Specify bubble center";
    case ID_ANNOTATE_MODE_HOOK:               return L"Specify hook leader start point";
    case ID_ANNOTATE_MODE_BOX:               return L"Specify box corner";
    case ID_ANNOTATE_MODE_CONSTRUCTION_LINE: return L"Specify construction line point";
    default:                                  return L"Annotate -- choose sub-command";
  }
}
