#include "Stdafx.h"

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPoint.h"
#include "EoDbPrimitive.h"
#include "EoGsRenderState.h"
#include "EoMsNodal.h"
#include "Resource.h"

void NodalModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"NodalModeState::OnExit\n");
  context->ModeLineUnhighlightOp(m_previousCommand);
  context->RubberBandingDisable();
  m_points.RemoveAll();
}

void NodalModeState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  if (m_points.IsEmpty()) { return; }

  const EoDbHandleSuppressionScope suppressHandles;
  auto cursorPosition = context->GetCursorPosition();

  switch (m_previousCommand) {
    case ID_OP4: {
      if (m_points[0] == cursorPosition) { return; }
      cursorPosition = context->SnapPointToAxis(m_points[0], cursorPosition);
      const EoGeVector3d translate(m_points[0], cursorPosition);

      context->PreviewGroup().DeletePrimitivesAndRemoveAll();

      auto* document = context->GetDocument();
      auto maskedPrimitivePosition = document->GetFirstMaskedPrimitivePosition();
      while (maskedPrimitivePosition != nullptr) {
        auto* maskedPrimitive = document->GetNextMaskedPrimitive(maskedPrimitivePosition);
        auto* primitive = maskedPrimitive->GetPrimitive();
        const auto mask = maskedPrimitive->GetMask();
        context->PreviewGroup().AddTail(primitive->Copy(primitive));
        static_cast<EoDbPrimitive*>(context->PreviewGroup().GetTail())->TranslateUsingMask(translate, mask);
      }
      auto uniquePointPosition = document->GetFirstUniquePointPosition();
      while (uniquePointPosition != nullptr) {
        const auto* uniquePoint = document->GetNextUniquePoint(uniquePointPosition);
        const EoGePoint3d translatedPoint = uniquePoint->m_Point + translate;
        context->PreviewGroup().AddTail(new EoDbPoint(252, 65, translatedPoint));
      }
      context->InvalidateOverlay();
      break;
    }
    case ID_OP5: {
      if (m_points[0] == cursorPosition) { return; }
      cursorPosition = context->SnapPointToAxis(m_points[0], cursorPosition);
      const EoGeVector3d translate(m_points[0], cursorPosition);

      context->PreviewGroup().DeletePrimitivesAndRemoveAll();
      context->ConstructPreviewGroupForNodalGroups();
      context->PreviewGroup().Translate(translate);
      context->InvalidateOverlay();
      break;
    }
    default:
      break;
  }
}

bool NodalModeState::OnReturn(AeSysView* context) {
  context->OnNodalModeReturn();
  return true;
}

bool NodalModeState::OnEscape(AeSysView* context) {
  context->OnNodalModeEscape();
  return true;
}

void NodalModeState::UnhighlightOp(AeSysView* context) {
  context->ModeLineUnhighlightOp(m_previousCommand);
}

void NodalModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  OnEscape(context);
}

bool NodalModeState::BuildContextMenu([[maybe_unused]] AeSysView* context, CMenu& menu) {
  if (m_previousCommand == 0) { return false; }
  if (m_previousCommand == ID_OP4) {
    menu.AppendMenu(MF_STRING, ID_NODAL_MODE_RETURN, L"&Place Here\tEnter");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_NODAL_MODE_ESCAPE, L"C&ancel Move\tEsc");
    return true;
  }
  if (m_previousCommand == ID_OP5) {
    menu.AppendMenu(MF_STRING, ID_NODAL_MODE_RETURN, L"Place &Copy Here\tEnter");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_NODAL_MODE_ESCAPE, L"C&ancel Copy\tEsc");
    return true;
  }
  menu.AppendMenu(MF_STRING, ID_NODAL_MODE_ESCAPE, L"C&ancel\tEsc");
  return true;
}

bool NodalModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToNodalCommand[] = {
      0,                       // ID_OP0
      ID_NODAL_MODE_POINT,     // ID_OP1
      ID_NODAL_MODE_LINE,      // ID_OP2
      ID_NODAL_MODE_AREA,      // ID_OP3
      ID_NODAL_MODE_MOVE,      // ID_OP4
      ID_NODAL_MODE_COPY,      // ID_OP5
      ID_NODAL_MODE_TOLINE,    // ID_OP6
      ID_NODAL_MODE_TOPOLYGON, // ID_OP7
      ID_NODAL_MODE_EMPTY,     // ID_OP8
      ID_NODAL_MODE_ENGAGE,    // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToNodalCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToNodalCommand[opIndex]);
    return true;
  }
  return false;
}

const wchar_t* NodalModeState::PromptString() const noexcept {
  switch (m_previousCommand) {
    case ID_NODAL_MODE_MOVE:      return L"Specify nodal move destination";
    case ID_NODAL_MODE_COPY:      return L"Specify nodal copy destination";
    case ID_NODAL_MODE_TOLINE:    return L"Specify to-line target";
    case ID_NODAL_MODE_TOPOLYGON: return L"Specify to-polygon vertex";
    default:                      return L"Nodal  —  choose sub-command";
  }
}

