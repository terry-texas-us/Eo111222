#include "Stdafx.h"

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPoint.h"
#include "EoDbPrimitive.h"
#include "EoGsRenderState.h"
#include "NodalModeState.h"

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
        context->PreviewGroup().AddTail(new EoDbPoint(252, 8, translatedPoint));
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

