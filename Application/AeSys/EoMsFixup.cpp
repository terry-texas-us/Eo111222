#include "Stdafx.h"

#include "AeSysView.h"
#include "EoMsFixup.h"
#include "Resource.h"

void FixupModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"FixupModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousCommand);
  context->RubberBandingDisable();
}

bool FixupModeState::OnReturn(AeSysView* context) {
  context->OnFixupModeReturn();
  return true;
}

bool FixupModeState::OnEscape(AeSysView* context) {
  context->OnFixupModeEscape();
  return true;
}

void FixupModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousCommand);
}

void FixupModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  OnEscape(context);
}

bool FixupModeState::BuildContextMenu([[maybe_unused]] AeSysView* context, CMenu& menu) {
  if (m_previousCommand == 0) { return false; }
  menu.AppendMenu(MF_STRING, ID_FIXUP_MODE_ESCAPE, L"C&ancel\tEsc");
  return true;
}

bool FixupModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToFixupCommand[] = {
      0,                        // ID_OP0
      ID_FIXUP_MODE_REFERENCE,  // ID_OP1
      ID_FIXUP_MODE_MEND,       // ID_OP2
      ID_FIXUP_MODE_CHAMFER,    // ID_OP3
      ID_FIXUP_MODE_FILLET,     // ID_OP4
      ID_FIXUP_MODE_SQUARE,     // ID_OP5
      ID_FIXUP_MODE_PARALLEL,   // ID_OP6
      0, 0, 0                   // ID_OP7..ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToFixupCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToFixupCommand[opIndex]);
    return true;
  }
  return false;
}

const wchar_t* FixupModeState::PromptString() const noexcept {
  switch (m_previousCommand) {
    case ID_FIXUP_MODE_MEND:     return L"Select vertex to mend";
    case ID_FIXUP_MODE_CHAMFER:  return L"Select second line for chamfer";
    case ID_FIXUP_MODE_FILLET:   return L"Select second line for fillet";
    case ID_FIXUP_MODE_PARALLEL: return L"Specify parallel offset distance";
    default:                     return L"Fixup -- choose sub-command (MEND CHAMFER FILLET...)";
  }
}
