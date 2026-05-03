#include "Stdafx.h"

#include "AeSysView.h"
#include "EoMsCut.h"
#include "Resource.h"

void CutModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"CutModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousOp);
  context->RubberBandingDisable();
}

bool CutModeState::OnReturn(AeSysView* context) {
  context->OnCutModeReturn();
  return true;
}

bool CutModeState::OnEscape(AeSysView* context) {
  context->OnCutModeEscape();
  return true;
}

void CutModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousOp);
}

void CutModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  OnEscape(context);
}

bool CutModeState::BuildContextMenu([[maybe_unused]] AeSysView* context, CMenu& menu) {
  if (m_previousOp == 0) { return false; }
  // Mid-slice or mid-field: offer commit and cancel.
  if (m_previousOp == ID_OP2) {
    menu.AppendMenu(MF_STRING, ID_CUT_MODE_SLICE, L"Commit &Slice");
  } else if (m_previousOp == ID_OP4) {
    menu.AppendMenu(MF_STRING, ID_CUT_MODE_FIELD, L"Commit &Field");
  } else if (m_previousOp == ID_OP7) {
    menu.AppendMenu(MF_STRING, ID_CUT_MODE_CLIP, L"Commit C&lip");
  }
  menu.AppendMenu(MF_SEPARATOR);
  menu.AppendMenu(MF_STRING, ID_CUT_MODE_ESCAPE, L"C&ancel\tEsc");
  return true;
}

bool CutModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToCutCommand[] = {
      0,                  // ID_OP0
      ID_CUT_MODE_TORCH,  // ID_OP1
      ID_CUT_MODE_SLICE,  // ID_OP2
      0,                  // ID_OP3
      ID_CUT_MODE_FIELD,  // ID_OP4
      0,                  // ID_OP5
      0,                  // ID_OP6
      ID_CUT_MODE_CLIP,   // ID_OP7
      0,                  // ID_OP8
      ID_CUT_MODE_DIVIDE, // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToCutCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToCutCommand[opIndex]);
    return true;
  }
  return false;
}

const wchar_t* CutModeState::PromptString() const noexcept {
  switch (m_previousOp) {
    case ID_CUT_MODE_SLICE: return L"Specify slice line second point";
    case ID_CUT_MODE_FIELD: return L"Specify field-cut second corner";
    case ID_CUT_MODE_CLIP:  return L"Specify clip boundary second corner";
    default:                return L"Cut -- choose sub-command (SLICE FIELD CLIP...)";
  }
}
