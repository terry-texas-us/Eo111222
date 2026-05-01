#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Resource.h"
#include "EoMsTrap.h"

void TrapModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"TrapModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousOp);
  context->RubberBandingDisable();
}

bool TrapModeState::OnEscape(AeSysView* context) {
  // Cancel any in-progress two-click stitch/field gesture.
  if (m_previousOp != 0) {
    context->RubberBandingDisable();
    UnhighlightOp(context);
  }
  return true;
}

void TrapModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}

void TrapModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  OnEscape(context);
}

bool TrapModeState::BuildContextMenu(AeSysView* context, CMenu& menu) {
  const auto* document = context->GetDocument();
  const bool isTrapMode = (app.CurrentMode() == ID_MODE_TRAP);

  // ── Mid-fence gesture: Stitch (line) or Field (rectangle) in progress ──
  if (m_previousOp != 0) {
    // Use the command that matches the active gesture so the handler takes the
    // second-click (commit) branch, not the first-click (start) branch.
    // ID_OP2 = Stitch (line fence), ID_OP4 = Field (rectangle fence).
    UINT finishCmd{};
    if (m_previousOp == ID_OP2) {
      finishCmd = isTrapMode ? ID_TRAP_MODE_STITCH : ID_TRAPR_MODE_STITCH;
    } else if (m_previousOp == ID_OP4) {
      finishCmd = isTrapMode ? ID_TRAP_MODE_FIELD : ID_TRAPR_MODE_FIELD;
    } else {
      // Unknown mid-gesture op — offer only Cancel.
      const UINT cancelCmd = isTrapMode ? ID_TRAP_MODE_ESCAPE : ID_TRAPR_MODE_ESCAPE;
      menu.AppendMenu(MF_STRING, cancelCmd, L"C&ancel\tEsc");
      return true;
    }
    const UINT cancelCmd = isTrapMode ? ID_TRAP_MODE_ESCAPE : ID_TRAPR_MODE_ESCAPE;
    menu.AppendMenu(MF_STRING, finishCmd, L"Finish &Trapping");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, cancelCmd, L"C&ancel\tEsc");
    return true;
  }

  // ── Idle: show trap-action menu ──
  if (document->IsTrapEmpty()) {
    // Nothing trapped — offer only a way to exit the mode so RMB doesn't
    // silently do nothing (AutoCAD users expect RMB to always produce a menu
    // in a mode context).
    const UINT exitCmd = isTrapMode ? ID_TRAP_MODE_ESCAPE : ID_TRAPR_MODE_ESCAPE;
    menu.AppendMenu(MF_STRING, exitCmd, isTrapMode ? L"Exit Trap Mode\tEsc" : L"Exit Trapr Mode\tEsc");
    return true;
  }

  if (isTrapMode) {
    menu.AppendMenu(MF_STRING, ID_EDIT_TRAPQUIT,   L"&Release Trap");
    menu.AppendMenu(MF_STRING, ID_EDIT_TRAPDELETE, L"&Delete Trapped");
    menu.AppendMenu(MF_STRING, ID_EDIT_TRAPCUT,    L"C&ut Trapped\tCtrl+X");
    menu.AppendMenu(MF_STRING, ID_EDIT_TRAPCOPY,   L"&Copy Trapped\tCtrl+C");
  } else {
    // Trapr (remove) mode: minimal set
    menu.AppendMenu(MF_STRING, ID_EDIT_TRAPQUIT, L"&Release Trap");
  }
  menu.AppendMenu(MF_SEPARATOR);
  menu.AppendMenu(MF_STRING,
      isTrapMode ? ID_TRAP_MODE_ESCAPE : ID_TRAPR_MODE_ESCAPE,
      L"C&ancel\tEsc");
  return true;
}

bool TrapModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToTrapCommand[] = {
      0,                   // ID_OP0
      ID_TRAP_MODE_POINT,  // ID_OP1
      ID_TRAP_MODE_STITCH, // ID_OP2
      0,                   // ID_OP3
      ID_TRAP_MODE_FIELD,  // ID_OP4
      ID_TRAP_MODE_LAST,   // ID_OP5
      ID_TRAP_MODE_ENGAGE, // ID_OP6
      0,                   // ID_OP7
      ID_TRAP_MODE_MENU,   // ID_OP8
      ID_TRAP_MODE_MODIFY, // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToTrapCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToTrapCommand[opIndex]);
    return true;
  }
  return false;
}
