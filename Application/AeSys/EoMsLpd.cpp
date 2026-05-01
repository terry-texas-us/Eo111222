#include "Stdafx.h"

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbPoint.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "Eo.h"
#include "EoLpdGeometry.h"
#include "EoMsLpd.h"
#include "Resource.h"
#include "Section.h"

namespace {
constexpr double endCapTolerance{0.01};
constexpr std::int16_t endCapColor{15};
constexpr std::int16_t endCapPointStyle{8};
}  // namespace

void LpdModeState::OnExit(AeSysView* context) {
  ATLTRACE2(traceGeneral, 2, L"LpdModeState::OnExit\n");
  ResetSequence(context);
}

void LpdModeState::OnMouseMove(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  const EoDbHandleSuppressionScope suppressHandles;

  if (m_previousOp == 0) {
    m_originalPreviousGroupDisplayed = true;
    return;
  }
  if (m_previousOp != ID_OP2) { return; }

  auto cursorPosition = context->GetCursorPosition();
  cursorPosition = context->SnapPointToAxis(m_previousPoint, cursorPosition);
  m_currentReferenceLine(m_previousPoint, cursorPosition);

  auto* document = context->GetDocument();
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();

  if (m_continueSection
      && m_currentReferenceLine.Length()
             > m_previousSection.Width() * context->CenterLineEccentricity() + context->DuctSeamSize()) {
    EoGeLine snapshotPreviousReferenceLine = m_previousReferenceLine;
    if (m_originalPreviousGroupDisplayed && m_originalPreviousGroup != nullptr) {
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_originalPreviousGroup);
      m_originalPreviousGroupDisplayed = false;
    }
    Lpd::GenerateRectangularElbow(snapshotPreviousReferenceLine,
        m_previousSection,
        m_currentReferenceLine,
        m_currentSection,
        context->DuctSeamSize(),
        context->CenterLineEccentricity(),
        context->GenerateTurningVanes(),
        &context->PreviewGroup());
    Lpd::GenerateRectangularSection(snapshotPreviousReferenceLine,
        context->CenterLineEccentricity(),
        m_previousSection,
        &context->PreviewGroup());
  }

  EoDbPoint* endPointPrimitive{};
  auto* existingGroup = context->SelectPointUsingPoint(
      cursorPosition, endCapTolerance, endCapColor, endCapPointStyle, endPointPrimitive);
  if (existingGroup != nullptr) {
    cursorPosition = endPointPrimitive->GetPt();
    const Section existingSection(
        endPointPrimitive->GetDat(0), endPointPrimitive->GetDat(1), Section::Rectangular);

    const auto* beginPointPrimitive = existingGroup->GetFirstDifferentPoint(endPointPrimitive);
    if (beginPointPrimitive != nullptr) {
      EoGeLine existingSectionReferenceLine(beginPointPrimitive->GetPt(), cursorPosition);

      const EoGePoint3d intersectionPoint{existingSectionReferenceLine.ProjectPointToLine(m_previousPoint)};
      double relationship{};
      if (!existingSectionReferenceLine.ComputeParametricRelation(intersectionPoint, relationship)) { return; }
      if (relationship > Eo::geometricTolerance) {
        m_currentReferenceLine(m_previousPoint, intersectionPoint);
        const double sectionLength = m_currentReferenceLine.Length()
            - (m_previousSection.Width() + context->DuctSeamSize() + existingSection.Width() * 0.5);
        if (sectionLength > Eo::geometricTolerance) {
          m_currentReferenceLine.end = m_currentReferenceLine.ProjectToEndPoint(sectionLength);
          Lpd::GenerateRectangularSection(
              m_currentReferenceLine, context->CenterLineEccentricity(), m_previousSection, &context->PreviewGroup());
        }
        context->GenerateFullElbowTakeoff(
            existingGroup, existingSectionReferenceLine, existingSection, &context->PreviewGroup());
      }
    }
  } else {
    const double transitionLength = (m_previousSection == m_currentSection)
        ? 0.0
        : Lpd::LengthOfTransition(
              context->DuctJustification(), context->TransitionSlope(), m_previousSection, m_currentSection);
    EoGeLine referenceLine{m_currentReferenceLine};

    if (context->BeginWithTransition()) {
      if (transitionLength != 0.0) {
        referenceLine.end = referenceLine.ProjectToEndPoint(transitionLength);
        Lpd::GenerateTransition(referenceLine,
            context->CenterLineEccentricity(),
            context->DuctJustification(),
            context->TransitionSlope(),
            m_previousSection,
            m_currentSection,
            &context->PreviewGroup());
        referenceLine.begin = referenceLine.end;
        referenceLine.end = m_currentReferenceLine.end;
      }
      if (m_currentReferenceLine.Length() - transitionLength > Eo::geometricTolerance) {
        Lpd::GenerateRectangularSection(
            referenceLine, context->CenterLineEccentricity(), m_currentSection, &context->PreviewGroup());
      }
    } else {
      if (referenceLine.Length() - transitionLength > Eo::geometricTolerance) {
        referenceLine.end = referenceLine.ProjectToBeginPoint(transitionLength);
        Lpd::GenerateRectangularSection(
            referenceLine, context->CenterLineEccentricity(), m_previousSection, &context->PreviewGroup());
        referenceLine.begin = referenceLine.end;
        referenceLine.end = m_currentReferenceLine.end;
      }
      if (transitionLength != 0.0) {
        Lpd::GenerateTransition(referenceLine,
            context->CenterLineEccentricity(),
            context->DuctJustification(),
            context->TransitionSlope(),
            m_previousSection,
            m_currentSection,
            &context->PreviewGroup());
      }
    }
  }
  context->PreviewGroup().RemoveDuplicatePrimitives();
  context->InvalidateOverlay();
}

bool LpdModeState::OnReturn(AeSysView* context) {
  context->OnLpdModeReturn();
  return true;
}

bool LpdModeState::OnEscape(AeSysView* context) {
  context->OnLpdModeEscape();
  return true;
}

void LpdModeState::OnRButtonUp(AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {
  OnReturn(context);
}

bool LpdModeState::BuildContextMenu([[maybe_unused]] AeSysView* context, CMenu& menu) {
  if (m_previousOp == 0) { return false; }
  menu.AppendMenu(MF_STRING, ID_LPD_MODE_ESCAPE, L"C&ancel\tEsc");
  return true;
}

bool LpdModeState::HandleCommand(AeSysView* context, UINT command) {
  if (command < ID_OP0 || command > ID_OP9) { return false; }
  static constexpr UINT opToLpdCommand[] = {
      0,                      // ID_OP0
      ID_LPD_MODE_JOIN,       // ID_OP1
      ID_LPD_MODE_DUCT,       // ID_OP2
      ID_LPD_MODE_TRANSITION, // ID_OP3
      ID_LPD_MODE_TAP,        // ID_OP4
      ID_LPD_MODE_ELL,        // ID_OP5
      ID_LPD_MODE_TEE,        // ID_OP6
      ID_LPD_MODE_UP_DOWN,    // ID_OP7
      0,                      // ID_OP8
      ID_LPD_MODE_SIZE,       // ID_OP9
  };
  const auto opIndex = command - ID_OP0;
  if (opToLpdCommand[opIndex] != 0) {
    context->SendMessage(WM_COMMAND, opToLpdCommand[opIndex]);
    return true;
  }
  return false;
}

void LpdModeState::ResetSequence(AeSysView* context) {
  if (context == nullptr) { return; }

  // Drop any preview geometry.
  context->PreviewGroup().DeletePrimitivesAndRemoveAll();
  context->InvalidateOverlay();

  // If the most recently committed section was hidden behind preview, redraw it.
  if (!m_originalPreviousGroupDisplayed && m_originalPreviousGroup != nullptr) {
    auto* document = context->GetDocument();
    if (document != nullptr) {
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_originalPreviousGroup);
    }
  }
  m_originalPreviousGroupDisplayed = true;
  m_originalPreviousGroup = nullptr;

  // Tear down rubber-banding so the next sequence starts clean.
  context->RubberBandingDisable();

  // Unhighlight the active op pane and reset the op id (passed by reference).
  context->ModeLineUnhighlightOp(m_previousOp);
  m_previousOp = 0;

  // Reset sequence flags / selection pointers.
  m_continueSection = false;
  m_endCapGroup = nullptr;
  m_endCapPoint = nullptr;
  m_endCapLocation = 0;
  m_previousReferenceLine = EoGeLine{};
  m_currentReferenceLine = EoGeLine{};
  m_previousPoint = EoGePoint3d{};
}

