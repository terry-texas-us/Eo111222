#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoMsEdit.h"
#include "Resource.h"

void EditModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"EditModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousOp);
  context->RubberBandingDisable();
}

bool EditModeState::OnReturn(AeSysView* context) {
  // Commit (or abandon) any in-progress two-click move/copy gesture.
  if (m_previousOp == ID_OP4 || m_previousOp == ID_OP5) {
    context->RubberBandingDisable();
    UnhighlightOp(context);
    return true;
  }
  return false;
}

bool EditModeState::OnEscape(AeSysView* context) {
  // Cancel any in-progress two-click move/copy gesture.
  if (m_previousOp == ID_OP4 || m_previousOp == ID_OP5) {
    context->RubberBandingDisable();
    UnhighlightOp(context);
    return true;
  }
  return false;
}

void EditModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}

void EditModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  OnEscape(context);
}

bool EditModeState::BuildContextMenu(AeSysView* context, CMenu& menu) {
  // ── Mid-Move or mid-Copy gesture ──
  if (m_previousOp == ID_OP4) {
    menu.AppendMenu(MF_STRING, ID_EDIT_MODE_MOVE, L"&Place Here");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_EDIT_MODE_ESCAPE, L"C&ancel Move\tEsc");
    return true;
  }
  if (m_previousOp == ID_OP5) {
    menu.AppendMenu(MF_STRING, ID_EDIT_MODE_COPY, L"Place &Copy Here");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_EDIT_MODE_ESCAPE, L"C&ancel Copy\tEsc");
    return true;
  }

  // ── Idle: show transform menu only when trap is non-empty ──
  const auto* document = context->GetDocument();
  if (document->IsTrapEmpty()) { return false; }

  menu.AppendMenu(MF_STRING, ID_EDIT_MODE_MOVE,    L"&Move...");
  menu.AppendMenu(MF_STRING, ID_EDIT_MODE_COPY,    L"&Copy...");
  menu.AppendMenu(MF_SEPARATOR);
  menu.AppendMenu(MF_STRING, ID_EDIT_MODE_ROTCCW,  L"Rotat&e CCW");
  menu.AppendMenu(MF_STRING, ID_EDIT_MODE_ROTCW,   L"Rotate C&W");
  menu.AppendMenu(MF_STRING, ID_EDIT_MODE_FLIP,    L"Fli&p");
  menu.AppendMenu(MF_STRING, ID_EDIT_MODE_REDUCE,  L"&Reduce");
  menu.AppendMenu(MF_STRING, ID_EDIT_MODE_ENLARGE, L"&Enlarge");
  menu.AppendMenu(MF_SEPARATOR);
  menu.AppendMenu(MF_STRING, ID_EDIT_MODE_ESCAPE,  L"C&ancel\tEsc");
  return true;
}

bool EditModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToEditCommand[] = {
      0,                    // ID_OP0
      ID_EDIT_MODE_PIVOT,   // ID_OP1
      ID_EDIT_MODE_ROTCCW,  // ID_OP2
      ID_EDIT_MODE_ROTCW,   // ID_OP3
      ID_EDIT_MODE_MOVE,    // ID_OP4
      ID_EDIT_MODE_COPY,    // ID_OP5
      ID_EDIT_MODE_FLIP,    // ID_OP6
      ID_EDIT_MODE_REDUCE,  // ID_OP7
      ID_EDIT_MODE_ENLARGE, // ID_OP8
      0,                    // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToEditCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToEditCommand[opIndex]);
    return true;
  }
  return false;
}

const wchar_t* EditModeState::PromptString() const noexcept {
  switch (m_previousOp) {
    case ID_EDIT_MODE_MOVE:   return L"Specify destination point (move)";
    case ID_EDIT_MODE_COPY:   return L"Specify destination point (copy)";
    default:                  return L"Edit -- trap groups then MOVE COPY ROTATE FLIP...";
  }
}
