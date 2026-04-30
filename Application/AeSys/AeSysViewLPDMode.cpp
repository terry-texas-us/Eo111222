#include "Stdafx.h"

#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbConic.h"
#include "EoDbFontDefinition.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDlgLowPressureDuctOptions.h"
#include "EoGeReferenceSystem.h"
#include "EoGsRenderState.h"
#include "EoLpdGeometry.h"
#include "EoMsLpd.h"
#include "Section.h"

namespace {
constexpr double endCapTolerance{0.01};
constexpr std::int16_t endCapColor{15};
constexpr std::int16_t endCapPointStyle{8};

/// Returns the active LpdModeState if LPD mode is engaged, else nullptr.
/// LPD handlers are only invoked from the LPD command map, so a non-null state
/// is the normal contract — callers should treat null as "mode not active" and
/// fall through quietly rather than crash.
LpdModeState* LpdState(AeSysView* view) {
  if (view == nullptr) { return nullptr; }
  return dynamic_cast<LpdModeState*>(view->GetCurrentState());
}

}  // namespace

/** @attention Only check for actual end-cap marker is by attributes. No error processing for invalid width or depth
 * values. Group data contains whatever primative follows marker (hopefully this is associated end-cap line). Issues:
 * xor operations on transition not clean
 * ending section with 3 key will generate a shortened section if the point is less than transition length from the
 * begin point. full el only works with center just
 */

void AeSysView::OnLpdModeOptions() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }
  SetDuctOptions(state->CurrentSectionRef());
}

void AeSysView::OnLpdModeJoin() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }

  const auto cursorPosition = GetCursorPosition();

  EoDbPoint* endCapPoint{};
  EoDbGroup* endCapGroup = SelectPointUsingPoint(cursorPosition, 0.01, 15, 8, endCapPoint);
  state->SetEndCapGroup(endCapGroup);
  state->SetEndCapPoint(endCapPoint);
  if (endCapGroup != nullptr) {
    auto& previousSection = state->PreviousSectionRef();
    state->SetPreviousPoint(endCapPoint->GetPt());
    previousSection.SetWidth(endCapPoint->GetDat(0));
    previousSection.SetDepth(endCapPoint->GetDat(1));
    state->SetContinueSection(false);

    state->SetEndCapLocation((state->PreviousOp() == 0) ? 1 : -1);  // 1 (start) / -1 (end)

    CString message(L"Cross sectional dimension (Width by Depth) is ");
    CString length;
    app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Inches), previousSection.Width(), 12, 2);
    CString width;
    app.FormatLength(width, std::max(app.GetUnits(), Eo::Units::Inches), previousSection.Depth(), 12, 2);
    message.Append(length.TrimLeft() + L" by " + width.TrimLeft());
    app.AddStringToMessageList(message);
    SetCursorPosition(state->PreviousPoint());
  }
}

void AeSysView::OnLpdModeDuct() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }

  auto cursorPosition = GetCursorPosition();
  auto& previousOp = state->PreviousOpRef();
  auto& previousPoint = state->PreviousPointRef();
  auto& previousSection = state->PreviousSectionRef();
  auto& currentSection = state->CurrentSectionRef();
  auto& previousReferenceLine = state->PreviousReferenceLineRef();
  auto& currentReferenceLine = state->CurrentReferenceLineRef();

  if (previousOp != 0) { m_PreviewGroup.DeletePrimitivesAndRemoveAll(); }
  if (previousOp == ID_OP2) {
    cursorPosition = SnapPointToAxis(previousPoint, cursorPosition);
    currentReferenceLine(previousPoint, cursorPosition);

    auto* document = GetDocument();
    if (state->ContinueSection()) {
      auto* group = new EoDbGroup;
      document->AddWorkLayerGroup(group);
      GenerateRectangularElbow(
          previousReferenceLine, previousSection, currentReferenceLine, currentSection, group);
      auto* originalPrevious = state->OriginalPreviousGroup();
      if (originalPrevious != nullptr) {
        originalPrevious->DeletePrimitivesAndRemoveAll();
        GenerateRectangularSection(
            previousReferenceLine, m_centerLineEccentricity, previousSection, originalPrevious);
      }
      state->SetOriginalPreviousGroupDisplayed(true);
      previousSection = currentSection;
    }
    const double transitionLength = (previousSection == currentSection)
        ? 0.0
        : LengthOfTransition(m_lpdConfig.justification, m_lpdConfig.transitionSlope, previousSection, currentSection);
    EoGeLine referenceLine(currentReferenceLine);

    if (m_lpdConfig.beginWithTransition) {
      if (transitionLength != 0.0) {
        referenceLine.end = referenceLine.ProjectToEndPoint(transitionLength);
        auto* group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateTransition(referenceLine,
            m_centerLineEccentricity,
            m_lpdConfig.justification,
            m_lpdConfig.transitionSlope,
            previousSection,
            currentSection,
            group);
        referenceLine.begin = referenceLine.end;
        referenceLine.end = currentReferenceLine.end;
        state->SetContinueSection(false);
      }
      if (currentReferenceLine.Length() - transitionLength > Eo::geometricTolerance) {
        auto* originalPrevious = new EoDbGroup;
        document->AddWorkLayerGroup(originalPrevious);
        GenerateRectangularSection(referenceLine, m_centerLineEccentricity, currentSection, originalPrevious);
        state->SetOriginalPreviousGroup(originalPrevious);
        state->SetContinueSection(true);
      }
    } else {
      if (referenceLine.Length() - transitionLength > Eo::geometricTolerance) {
        referenceLine.end = referenceLine.ProjectToBeginPoint(transitionLength);
        auto* originalPrevious = new EoDbGroup;
        document->AddWorkLayerGroup(originalPrevious);
        GenerateRectangularSection(referenceLine, m_centerLineEccentricity, previousSection, originalPrevious);
        state->SetOriginalPreviousGroup(originalPrevious);
        referenceLine.begin = referenceLine.end;
        referenceLine.end = currentReferenceLine.end;
        state->SetContinueSection(true);
      }
      if (transitionLength != 0.0) {
        auto* group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateTransition(referenceLine,
            m_centerLineEccentricity,
            m_lpdConfig.justification,
            m_lpdConfig.transitionSlope,
            previousSection,
            currentSection,
            group);
        state->SetContinueSection(false);
      }
    }
    previousReferenceLine = currentReferenceLine;
    previousSection = currentSection;
  }
  previousOp = ModeLineHighlightOp(ID_OP2);
  previousPoint = cursorPosition;
}

void AeSysView::OnLpdModeTransition() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }

  state->CurrentSectionRef() = state->PreviousSectionRef();
  SetDuctOptions(state->CurrentSectionRef());

  m_lpdConfig.beginWithTransition = (state->PreviousOp() == 0);

  DoDuctModeMouseMove();
  OnLpdModeDuct();
}

void AeSysView::OnLpdModeTap() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }

  auto cursorPosition = GetCursorPosition();
  auto& previousOp = state->PreviousOpRef();
  auto& previousPoint = state->PreviousPointRef();
  auto& previousSection = state->PreviousSectionRef();
  auto& currentSection = state->CurrentSectionRef();
  auto& previousReferenceLine = state->PreviousReferenceLineRef();
  auto& currentReferenceLine = state->CurrentReferenceLineRef();

  auto* document = GetDocument();
  if (previousOp != 0) {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
  }
  EoDbLine* linePrimitive{};
  auto* group = SelectLineUsingPoint(cursorPosition, linePrimitive);
  if (group != nullptr) {
    const EoGePoint3d testPoint(cursorPosition);
    cursorPosition = SnapPointToAxis(previousPoint, cursorPosition);
    cursorPosition = linePrimitive->ProjectPointToLine(cursorPosition);
    currentReferenceLine(previousPoint, cursorPosition);

    EJust justification;
    const int relationship = currentReferenceLine.DirRelOfPt(testPoint);
    if (relationship == 1) {
      justification = Left;
    } else if (relationship == -1) {
      justification = Right;
    } else {
      app.AddStringToMessageList(std::wstring(L"Could not determine orientation of component"));
      return;
    }
    if (previousOp == ID_OP2) {
      if (state->ContinueSection()) {
        group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateRectangularElbow(
            previousReferenceLine, previousSection, currentReferenceLine, currentSection, group);
        previousSection = currentSection;
      }
      const double sectionLength = currentReferenceLine.Length();
      if (sectionLength >= m_lpdConfig.ductTapSize + m_lpdConfig.ductSeamSize) {
        EoGeLine referenceLine{currentReferenceLine};
        referenceLine.end = referenceLine.ProjectToBeginPoint(m_lpdConfig.ductTapSize + m_lpdConfig.ductSeamSize);
        group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateRectangularSection(referenceLine, m_centerLineEccentricity, previousSection, group);
        currentReferenceLine.begin = referenceLine.end;
        previousReferenceLine = currentReferenceLine;
        previousSection = currentSection;
      }
      GenerateRectangularTap(justification, previousSection);
      ModeLineUnhighlightOp(previousOp);
      previousOp = 0;
      state->SetContinueSection(false);
      previousPoint = cursorPosition;
    }
  } else {
    app.AddStringToMessageList(IDS_MSG_LINE_NOT_SELECTED);
  }
}

void AeSysView::OnLpdModeEll() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }
  auto* document = GetDocument();
  if (document == nullptr) { return; }

  auto cursorPosition = GetCursorPosition();
  auto& previousOp = state->PreviousOpRef();
  auto& previousPoint = state->PreviousPointRef();
  auto& previousSection = state->PreviousSectionRef();
  auto& currentReferenceLine = state->CurrentReferenceLineRef();

  if (previousOp != 0) {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
  }
  if (previousOp == ID_OP2) {
    EoDbPoint* endPointPrimitive{};
    EoDbGroup* existingGroup = SelectPointUsingPoint(cursorPosition, 0.01, 15, 8, endPointPrimitive);
    if (existingGroup == nullptr) {
      app.AddStringToMessageList(IDS_MSG_LPD_NO_END_CAP_LOC);
      return;
    }
    cursorPosition = endPointPrimitive->GetPt();
    const Section existingSection(endPointPrimitive->GetDat(0), endPointPrimitive->GetDat(1), Section::Rectangular);

    const EoDbPoint* beginPointPrimitive = existingGroup->GetFirstDifferentPoint(endPointPrimitive);
    if (beginPointPrimitive != nullptr) {
      EoGeLine existingSectionReferenceLine(beginPointPrimitive->GetPt(), cursorPosition);

      const EoGePoint3d intersectionPoint(existingSectionReferenceLine.ProjectPointToLine(previousPoint));
      double relationship{};
      if (!existingSectionReferenceLine.ComputeParametricRelation(intersectionPoint, relationship)) { return; }
      if (relationship > Eo::geometricTolerance) {
        currentReferenceLine(previousPoint, intersectionPoint);
        const double sectionLength = currentReferenceLine.Length()
            - (previousSection.Width() + m_lpdConfig.ductSeamSize + existingSection.Width() * 0.5);
        if (sectionLength > Eo::geometricTolerance) {
          currentReferenceLine.end = currentReferenceLine.ProjectToEndPoint(sectionLength);
          auto* group = new EoDbGroup;
          document->AddWorkLayerGroup(group);
          GenerateRectangularSection(currentReferenceLine, m_centerLineEccentricity, previousSection, group);
          document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        }
        auto* group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateFullElbowTakeoff(existingGroup, existingSectionReferenceLine, existingSection, group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      }
    }
  }
  state->SetContinueSection(false);
  previousOp = ModeLineHighlightOp(ID_OP2);
}

void AeSysView::OnLpdModeTee() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }

  if (state->PreviousOp() != 0) {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
  }
  // TODO: GenerateBullheadTee — body not yet implemented (see legacy commented-out call).

  state->SetContinueSection(false);
  state->SetPreviousOp(ModeLineHighlightOp(ID_OP2));
}

void AeSysView::OnLpdModeUpDown() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }

  auto cursorPosition = GetCursorPosition();
  auto& previousOp = state->PreviousOpRef();
  auto& previousPoint = state->PreviousPointRef();
  auto& previousSection = state->PreviousSectionRef();
  auto& currentSection = state->CurrentSectionRef();
  auto& previousReferenceLine = state->PreviousReferenceLineRef();
  auto& currentReferenceLine = state->CurrentReferenceLineRef();

  const int iRet{};  // dialog to "Select direction", 'Up.Down.'
  if (iRet >= 0) {
    if (previousOp == ID_OP2) {
      cursorPosition = SnapPointToAxis(previousPoint, cursorPosition);
      currentReferenceLine(previousPoint, cursorPosition);

      auto* document = GetDocument();
      if (state->ContinueSection()) {
        auto* group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateRectangularElbow(
            previousReferenceLine, previousSection, currentReferenceLine, currentSection, group);
        previousSection = currentSection;
      }
      const double sectionLength = currentReferenceLine.Length();
      if (sectionLength > previousSection.Depth() * 0.5 + m_lpdConfig.ductSeamSize) {
        EoGeLine referenceLine(currentReferenceLine);
        referenceLine.end = referenceLine.begin.ProjectToward(
            referenceLine.end, sectionLength - previousSection.Depth() * 0.5 - m_lpdConfig.ductSeamSize);
        auto* group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateRectangularSection(referenceLine, m_centerLineEccentricity, previousSection, group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        currentReferenceLine.begin = referenceLine.end;
      }
      auto* group = new EoDbGroup;
      document->AddWorkLayerGroup(group);
      GenerateRiseDrop(L"CONTINUOUS", previousSection, currentReferenceLine, group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
    state->SetContinueSection(false);
    previousOp = ModeLineHighlightOp(ID_OP2);
    previousPoint = cursorPosition;
  }
}

void AeSysView::OnLpdModeSize() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }

  double angle{};
  auto* endCapPoint = state->EndCapPoint();
  if (endCapPoint != nullptr) {
    auto* endCapGroup = state->EndCapGroup();
    if (endCapPoint->Color() == 15 && endCapPoint->PointStyle() == 8 && endCapGroup != nullptr) {
      auto position = endCapGroup->Find(endCapPoint);
      endCapGroup->GetNext(position);
      const auto* endCap = dynamic_cast<EoDbLine*>(endCapGroup->GetAt(position));
      if (endCap != nullptr) {
        const EoGeLine line = endCap->Line();
        angle = fmod(line.AngleFromXAxisXY(), Eo::Pi);
        if (angle <= Eo::Radian) { angle += Eo::Pi; }
        angle -= Eo::HalfPi;
      }
    }
    state->SetEndCapPoint(nullptr);
  }
  const auto cursorPosition = GetCursorPosition();

  GenSizeNote(cursorPosition, angle, state->PreviousSectionRef());
  if (state->PreviousOp() != 0) {
    ModeLineUnhighlightOp(state->PreviousOpRef());
    RubberBandingDisable();
  }
  state->SetPreviousOp(0);
  state->SetContinueSection(false);
}

void AeSysView::OnLpdModeReturn() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }

  // RETURN ends the current sequence (if any) and re-anchors at the cursor so
  // the next op (Duct, Tap, Ell, ...) starts fresh from where the user is.
  const auto cursorPosition = GetCursorPosition();
  if (state->PreviousOp() != 0) { state->ResetSequence(this); }
  state->SetPreviousPoint(cursorPosition);
}

void AeSysView::OnLpdModeEscape() {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }

  // ESC fully aborts the current sequence — preview cleared, original section
  // restored, op pane unhighlighted, continuation flags reset.
  state->ResetSequence(this);
}

void AeSysView::DoDuctModeMouseMove() {
  // Preview logic moved to LpdModeState::OnMouseMove.
  auto* state = LpdState(this);
  if (state != nullptr) { state->OnMouseMove(this, 0, CPoint{}); }
}

void AeSysView::GenerateEndCap(const EoGePoint3d& beginPoint,
    const EoGePoint3d& endPoint,
    Section section,
    EoDbGroup* group) {
  Lpd::GenerateEndCap(beginPoint, endPoint, section, group);
}

EoGePoint3d AeSysView::GenerateBullheadTee(const EoDbGroup* existingGroup,
    const EoGeLine& existingSectionReferenceLine,
    double existingSectionWidth,
    double existingSectionDepth,
    const EoDbGroup* group) noexcept {
  (void)existingGroup;
  (void)existingSectionReferenceLine;
  (void)existingSectionWidth;
  (void)existingSectionDepth;
  (void)group;
  return EoGePoint3d(0.0, 0.0, 0.0);
};

void AeSysView::GenerateFullElbowTakeoff(EoDbGroup*,
    EoGeLine& existingSectionReferenceLine,
    Section existingSection,
    EoDbGroup* group) {
  auto* state = LpdState(this);
  if (state == nullptr) { return; }
  const auto& sequencePreviousPoint = state->PreviousPoint();
  const auto& sequencePreviousSection = state->PreviousSectionRef();
  const auto& sequenceCurrentSection = state->CurrentSectionRef();

  const EoGeVector3d newSectionDirection(existingSectionReferenceLine.begin, existingSectionReferenceLine.end);

  EoGePoint3d intersectionPoint{existingSectionReferenceLine.ProjectPointToLine(sequencePreviousPoint)};
  EoGeLine previousReferenceLine(sequencePreviousPoint, intersectionPoint);
  previousReferenceLine.end =
      previousReferenceLine.ProjectToBeginPoint((existingSection.Width() + sequencePreviousSection.Width()) * 0.5);
  EoGeLine currentReferenceLine(previousReferenceLine.end, previousReferenceLine.end + newSectionDirection);

  GenerateRectangularElbow(
      previousReferenceLine, sequencePreviousSection, currentReferenceLine, sequenceCurrentSection, group);
  intersectionPoint = existingSectionReferenceLine.ProjectPointToLine(currentReferenceLine.begin);
  double relationship;
  if (existingSectionReferenceLine.ComputeParametricRelation(intersectionPoint, relationship)) {
    if (std::abs(relationship) > Eo::geometricTolerance && std::abs(relationship - 1.0) > Eo::geometricTolerance) {
      // need to add a section either from the elbow or the existing section
      const double sectionLength = existingSectionReferenceLine.Length();
      double distanceToBeginPoint = relationship * sectionLength;
      if (relationship > Eo::geometricTolerance
          && relationship < 1.0 - Eo::geometricTolerance) {  // section from the elbow
        currentReferenceLine.end =
            currentReferenceLine.begin.ProjectToward(currentReferenceLine.end, sectionLength - distanceToBeginPoint);
        GenerateRectangularSection(currentReferenceLine, m_centerLineEccentricity, sequencePreviousSection, group);
      } else {
        distanceToBeginPoint = std::max(distanceToBeginPoint, sectionLength);
        existingSectionReferenceLine.end =
            existingSectionReferenceLine.begin.ProjectToward(existingSectionReferenceLine.end, distanceToBeginPoint);
      }
    }
    // generate the transition
    EoGePoint3d points[2]{};
    points[0] = existingSectionReferenceLine.end.ProjectToward(
        currentReferenceLine.end, existingSection.Width() * 0.5 + sequencePreviousSection.Width());
    points[1] = points[0].ProjectToward(
        existingSectionReferenceLine.end, existingSection.Width() + sequencePreviousSection.Width());

    const EoGePoint3d middleOfTransition = points[0] + EoGeVector3d(points[0], points[1]) * 0.5;
    EoGeLine transitionReferenceLine(middleOfTransition, middleOfTransition + newSectionDirection);

    const double width = sequencePreviousSection.Width() + existingSection.Width();
    const double depth = sequencePreviousSection.Depth() + existingSection.Depth();
    const Section continueGroup(width, depth, Section::Rectangular);
    const Section currentSection(width * 0.75, depth * 0.75, Section::Rectangular);

    GenerateTransition(transitionReferenceLine,
        m_centerLineEccentricity,
        m_lpdConfig.justification,
        m_lpdConfig.transitionSlope,
        continueGroup,
        currentSection,
        group);
  }
  if (m_lpdConfig.generateTurningVanes) {
    // TODO: Generate the splitter damper similar the one in `GenerateRectangulasrTap`
  }
}

void AeSysView::GenerateRiseDrop(const std::wstring& indicatorLineTypeName,
    Section section,
    EoGeLine& referenceLine,
    EoDbGroup* group) {
  Lpd::GenerateRiseDrop(indicatorLineTypeName, section, referenceLine, m_lpdConfig.ductSeamSize, m_centerLineEccentricity, group);
}

void AeSysView::GenerateRectangularElbow(EoGeLine& previousReferenceLine,
    Section previousSection,
    EoGeLine& currentReferenceLine,
    Section currentSection,
    EoDbGroup* group) {
  Lpd::GenerateRectangularElbow(previousReferenceLine, previousSection,
      currentReferenceLine, currentSection,
      m_lpdConfig.ductSeamSize, m_centerLineEccentricity, m_lpdConfig.generateTurningVanes, group);
}

void AeSysView::GenerateRectangularSection(const EoGeLine& referenceLine,
    double eccentricity,
    Section section,
    EoDbGroup* group) {
  Lpd::GenerateRectangularSection(referenceLine, eccentricity, section, group);
}

void AeSysView::GenSizeNote(EoGePoint3d point, double angle, Section section) {
  const EoGeVector3d xDirection = RotateVectorAboutZAxis(EoGeVector3d(0.06, 0.0, 0.0), angle);
  const EoGeVector3d yDirection = RotateVectorAboutZAxis(EoGeVector3d(0.0, 0.1, 0.0), angle);
  EoGeReferenceSystem referenceSystem{point, xDirection, yDirection};

  CString width;
  app.FormatLength(width, std::max(app.GetUnits(), Eo::Units::Inches), section.Width(), 0, 2);
  CString depth;
  app.FormatLength(depth, std::max(app.GetUnits(), Eo::Units::Inches), section.Depth(), 0, 2);
  const CString note = width.TrimLeft() + L"/" + depth.TrimLeft();
  auto* deviceContext = GetDC();
  const int savedRenderState = Gs::renderState.Save();
  Gs::renderState.SetColor(deviceContext, 2);

  EoDbFontDefinition fontDefinition = Gs::renderState.FontDefinition();
  fontDefinition.SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

  EoDbCharacterCellDefinition characterCellDefinition = Gs::renderState.CharacterCellDefinition();
  characterCellDefinition.SetRotationAngle(0.0);
  Gs::renderState.SetCharacterCellDefinition(characterCellDefinition);

  auto* group = new EoDbGroup(new EoDbText(fontDefinition, referenceSystem, note));
  auto* document = GetDocument();
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
  Gs::renderState.Restore(deviceContext, savedRenderState);
  ReleaseDC(deviceContext);
}

bool AeSysView::GenerateRectangularTap(EJust justification, Section section) {
  auto* state = LpdState(this);
  if (state == nullptr) { return false; }
  auto& currentReferenceLine = state->CurrentReferenceLineRef();

  double sectionLength = currentReferenceLine.Length();

  if (sectionLength < m_lpdConfig.ductTapSize + m_lpdConfig.ductSeamSize) {
    currentReferenceLine.begin = currentReferenceLine.ProjectToBeginPoint(m_lpdConfig.ductTapSize + m_lpdConfig.ductSeamSize);
    sectionLength = m_lpdConfig.ductTapSize + m_lpdConfig.ductSeamSize;
  }
  EoGeLine referenceLine{currentReferenceLine};
  referenceLine.end = referenceLine.ProjectToEndPoint(m_lpdConfig.ductSeamSize);

  EoGeLine leftLine;
  EoGeLine rightLine;

  if (!referenceLine.GetParallels(section.Width(), m_centerLineEccentricity, leftLine, rightLine)) { return false; }

  auto* newSection = new EoDbGroup;
  auto* document = GetDocument();
  document->AddWorkLayerGroup(newSection);

  GenerateEndCap(leftLine.begin, rightLine.begin, section, newSection);
  newSection->AddTail(EoDbLine::CreateLine(rightLine.begin, rightLine.end)->WithProperties(Gs::renderState));
  newSection->AddTail(EoDbLine::CreateLine(rightLine.end, leftLine.end)->WithProperties(Gs::renderState));
  newSection->AddTail(EoDbLine::CreateLine(leftLine.begin, leftLine.end)->WithProperties(Gs::renderState));

  currentReferenceLine.begin = referenceLine.end;
  (void)currentReferenceLine.GetParallels(section.Width(), m_centerLineEccentricity, leftLine, rightLine);
  if (justification == Right) {
    rightLine.ProjPtFrom_xy(m_lpdConfig.ductTapSize, -m_lpdConfig.ductTapSize, &rightLine.end);
  } else {
    leftLine.ProjPtFrom_xy(m_lpdConfig.ductTapSize, m_lpdConfig.ductTapSize, &leftLine.end);
  }
  newSection->AddTail(EoDbLine::CreateLine(rightLine.begin, rightLine.end)->WithProperties(Gs::renderState));
  newSection->AddTail(EoDbLine::CreateLine(leftLine.end, leftLine.begin)->WithProperties(Gs::renderState));

  if (m_lpdConfig.generateTurningVanes) {
    const EoGePoint3d beginPoint =
        ((justification == Left) ? rightLine : leftLine).ProjectToBeginPoint(-m_lpdConfig.ductTapSize / 3.0);
    const EoGePoint3d endPoint = currentReferenceLine.ProjectToBeginPoint(-m_lpdConfig.ductTapSize / 2.0);
    auto* circle = EoDbConic::CreateCircleInView(beginPoint, 0.01);
    circle->SetColor(1);
    circle->SetLineTypeName(Gs::renderState.LineTypeName());
    newSection->AddTail(circle);
    newSection->AddTail(EoDbLine::CreateLine(beginPoint, endPoint)->WithProperties(1, Gs::renderState.LineTypeName()));
  }
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, newSection);
  return true;
}

void AeSysView::GenerateTransition(const EoGeLine& referenceLine,
    double eccentricity,
    EJust justification,
    double slope,
    Section previousSection,
    Section currentSection,
    EoDbGroup* group) {
  Lpd::GenerateTransition(referenceLine, eccentricity, justification, slope,
      previousSection, currentSection, group);
}

void AeSysView::SetDuctOptions(Section& section) {
  const Eo::Units units = app.GetUnits();
  app.SetUnits(std::max(units, Eo::Units::Inches));

  EoDlgLowPressureDuctOptions dlg(this);

  dlg.m_Width = section.Width();
  dlg.m_Depth = section.Depth();
  dlg.m_RadiusFactor = m_lpdConfig.insideRadiusFactor;
  dlg.m_Justification = m_lpdConfig.justification;
  dlg.m_GenerateVanes = m_lpdConfig.generateTurningVanes;
  dlg.m_BeginWithTransition = m_lpdConfig.beginWithTransition;
  if (dlg.DoModal() == IDOK) {
    section.SetWidth(dlg.m_Width);
    section.SetDepth(dlg.m_Depth);
    m_lpdConfig.insideRadiusFactor = dlg.m_RadiusFactor;
    m_lpdConfig.justification = EJust(dlg.m_Justification);
    m_lpdConfig.generateTurningVanes = dlg.m_GenerateVanes;
    m_lpdConfig.beginWithTransition = dlg.m_BeginWithTransition;
  }
  app.SetUnits(units);
}

double AeSysView::LengthOfTransition(EJust justification,
    double slope,
    Section previousSection,
    Section currentSection) {
  return Lpd::LengthOfTransition(justification, slope, previousSection, currentSection);
}

EoDbGroup* AeSysView::SelectPointUsingPoint(const EoGePoint3d& cursorPosition,
    double tolerance,
    std::int16_t color,
    std::int16_t pointStyle,
    EoDbPoint*& endCapPoint) {
  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);
    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);
      if (primitive->Is(EoDb::kPointPrimitive)) {
        auto* point = static_cast<EoDbPoint*>(primitive);

        if (point->Color() == color && point->PointStyle() == pointStyle) {
          if (cursorPosition.DistanceTo(point->GetPt()) <= tolerance) {
            endCapPoint = point;
            return group;
          }
        }
      }
    }
  }
  return nullptr;
}

bool AeSysView::Find2LinesUsingLineEndpoints(const EoDbLine* testLinePrimitive,
    double angularTolerance,
    EoGeLine& leftLine,
    EoGeLine& rightLine) {
  EoGeLine line;

  EoDbLine* leftLinePrimitive{};
  EoDbLine* rightLinePrimitive{};
  int directedRelationship{};

  const EoGeLine testLine = testLinePrimitive->Line();

  const double testLineAngle = fmod(testLine.AngleFromXAxisXY(), Eo::Pi);

  auto groupPosition = GetLastGroupPosition();
  while (groupPosition != nullptr) {
    const auto* group = GetPreviousGroup(groupPosition);

    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);
      if (primitive == testLinePrimitive || !primitive->Is(EoDb::kLinePrimitive)) { continue; }

      auto* linePrimitive = static_cast<EoDbLine*>(primitive);
      line = linePrimitive->Line();
      if (line.begin == testLine.begin || line.begin == testLine.end) {  // Exchange points
        const auto point = line.begin;
        line.begin = line.end;
        line.end = point;
      } else if (line.end != testLine.begin && line.end != testLine.end) {
        // No endpoint coincides with one of the test line endpoints
        continue;
      }
      const double lineAngle = fmod(line.AngleFromXAxisXY(), Eo::Pi);
      if (std::abs(std::abs(testLineAngle - lineAngle) - Eo::HalfPi) <= angularTolerance) {
        if (leftLinePrimitive == nullptr) {  // No qualifiers yet
          directedRelationship = testLine.DirRelOfPt(line.begin);
          leftLinePrimitive = linePrimitive;
          leftLine = line;
        } else {
          if (directedRelationship == testLine.DirRelOfPt(line.begin)) {
            // Both lines are on the same side of test line
            rightLinePrimitive = linePrimitive;
            rightLine = line;
            if (rightLine.DirRelOfPt(leftLine.begin) != 1) {
              rightLinePrimitive = leftLinePrimitive;
              rightLine = leftLine;
              leftLinePrimitive = linePrimitive;
              leftLine = line;
            }
            return true;
          }
        }
      }
    }
  }
  return false;
}
