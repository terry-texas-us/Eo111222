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
#include "Section.h"

namespace {
constexpr double endCapTolerance{0.01};
constexpr std::int16_t endCapColor{15};
constexpr std::int16_t endCapPointStyle{8};

}  // namespace

/** @attention Only check for actual end-cap marker is by attributes. No error processing for invalid width or depth
 * values. Group data contains whatever primative follows marker (hopefully this is associated end-cap line). Issues:
 * xor operations on transition not clean
 * ending section with 3 key will generate a shortened section if the point is less than transition length from the
 * begin point. full el only works with center just
 */

void AeSysView::OnLpdModeOptions() {
  SetDuctOptions(m_CurrentSection);
}

void AeSysView::OnLpdModeJoin() {
  auto cursorPosition = GetCursorPosition();

  m_EndCapGroup = SelectPointUsingPoint(cursorPosition, 0.01, 15, 8, m_EndCapPoint);
  if (m_EndCapGroup != nullptr) {
    m_PreviousPnt = m_EndCapPoint->GetPt();
    m_PreviousSection.SetWidth(m_EndCapPoint->GetDat(0));
    m_PreviousSection.SetDepth(m_EndCapPoint->GetDat(1));
    m_ContinueSection = false;

    m_EndCapLocation = (m_PreviousOp == 0) ? 1 : -1;  // 1 (start) and -1 (end)

    CString message(L"Cross sectional dimension (Width by Depth) is ");
    CString length;
    app.FormatLength(length, std::max(app.GetUnits(), Eo::Units::Inches), m_PreviousSection.Width(), 12, 2);
    CString width;
    app.FormatLength(width, std::max(app.GetUnits(), Eo::Units::Inches), m_PreviousSection.Depth(), 12, 2);
    message.Append(length.TrimLeft() + L" by " + width.TrimLeft());
    app.AddStringToMessageList(message);
    SetCursorPosition(m_PreviousPnt);
  }
}

void AeSysView::OnLpdModeDuct() {
  auto cursorPosition = GetCursorPosition();

  if (m_PreviousOp != 0) { m_PreviewGroup.DeletePrimitivesAndRemoveAll(); }
  if (m_PreviousOp == ID_OP2) {
    cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);
    m_currentReferenceLine(m_PreviousPnt, cursorPosition);

    auto* document = GetDocument();
    if (m_ContinueSection) {
      auto* group = new EoDbGroup;
      document->AddWorkLayerGroup(group);
      GenerateRectangularElbow(
          m_previousReferenceLine, m_PreviousSection, m_currentReferenceLine, m_CurrentSection, group);
      m_OriginalPreviousGroup->DeletePrimitivesAndRemoveAll();
      GenerateRectangularSection(
          m_previousReferenceLine, m_centerLineEccentricity, m_PreviousSection, m_OriginalPreviousGroup);
      m_OriginalPreviousGroupDisplayed = true;
      m_PreviousSection = m_CurrentSection;
    }
    const double transitionLength = (m_PreviousSection == m_CurrentSection)
        ? 0.
        : LengthOfTransition(m_DuctJustification, m_TransitionSlope, m_PreviousSection, m_CurrentSection);
    EoGeLine referenceLine(m_currentReferenceLine);

    if (m_BeginWithTransition) {
      if (transitionLength != 0.0) {
        referenceLine.end = referenceLine.ProjectToEndPoint(transitionLength);
        auto* group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateTransition(referenceLine,
            m_centerLineEccentricity,
            m_DuctJustification,
            m_TransitionSlope,
            m_PreviousSection,
            m_CurrentSection,
            group);
        referenceLine.begin = referenceLine.end;
        referenceLine.end = m_currentReferenceLine.end;
        m_ContinueSection = false;
      }
      if (m_currentReferenceLine.Length() - transitionLength > Eo::geometricTolerance) {
        m_OriginalPreviousGroup = new EoDbGroup;
        document->AddWorkLayerGroup(m_OriginalPreviousGroup);
        GenerateRectangularSection(referenceLine, m_centerLineEccentricity, m_CurrentSection, m_OriginalPreviousGroup);
        m_ContinueSection = true;
      }
    } else {
      if (referenceLine.Length() - transitionLength > Eo::geometricTolerance) {
        referenceLine.end = referenceLine.ProjectToBeginPoint(transitionLength);
        m_OriginalPreviousGroup = new EoDbGroup;
        document->AddWorkLayerGroup(m_OriginalPreviousGroup);
        GenerateRectangularSection(referenceLine, m_centerLineEccentricity, m_PreviousSection, m_OriginalPreviousGroup);
        referenceLine.begin = referenceLine.end;
        referenceLine.end = m_currentReferenceLine.end;
        m_ContinueSection = true;
      }
      if (transitionLength != 0.0) {
        auto* group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateTransition(referenceLine,
            m_centerLineEccentricity,
            m_DuctJustification,
            m_TransitionSlope,
            m_PreviousSection,
            m_CurrentSection,
            group);
        m_ContinueSection = false;
      }
    }
    m_previousReferenceLine = m_currentReferenceLine;
    m_PreviousSection = m_CurrentSection;
  }
  m_PreviousOp = ID_OP2;
  m_PreviousPnt = cursorPosition;
}

void AeSysView::OnLpdModeTransition() {
  m_CurrentSection = m_PreviousSection;
  SetDuctOptions(m_CurrentSection);

  m_BeginWithTransition = (m_PreviousOp == 0) ? true : false;

  DoDuctModeMouseMove();
  OnLpdModeDuct();
}

void AeSysView::OnLpdModeTap() {
  auto cursorPosition = GetCursorPosition();

  auto* document = GetDocument();
  if (m_PreviousOp != 0) {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
  }
  EoDbLine* linePrimitive{};
  auto* group = SelectLineUsingPoint(cursorPosition, linePrimitive);
  if (group != nullptr) {
    const EoGePoint3d testPoint(cursorPosition);
    cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);
    cursorPosition = linePrimitive->ProjectPointToLine(cursorPosition);
    m_currentReferenceLine(m_PreviousPnt, cursorPosition);

    EJust justification;
    int relationship = m_currentReferenceLine.DirRelOfPt(testPoint);
    if (relationship == 1) {
      justification = Left;
    } else if (relationship == -1) {
      justification = Right;
    } else {
      app.AddStringToMessageList(std::wstring(L"Could not determine orientation of component"));
      return;
    }
    if (m_PreviousOp == ID_OP2) {
      if (m_ContinueSection) {
        group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateRectangularElbow(
            m_previousReferenceLine, m_PreviousSection, m_currentReferenceLine, m_CurrentSection, group);
        m_PreviousSection = m_CurrentSection;
      }
      double sectionLength = m_currentReferenceLine.Length();
      if (sectionLength >= m_DuctTapSize + m_DuctSeamSize) {
        EoGeLine referenceLine{m_currentReferenceLine};
        referenceLine.end = referenceLine.ProjectToBeginPoint(m_DuctTapSize + m_DuctSeamSize);
        group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateRectangularSection(referenceLine, m_centerLineEccentricity, m_PreviousSection, group);
        m_currentReferenceLine.begin = referenceLine.end;
        m_previousReferenceLine = m_currentReferenceLine;
        m_PreviousSection = m_CurrentSection;
      }
      GenerateRectangularTap(justification, m_PreviousSection);
      m_PreviousOp = 0;
      m_ContinueSection = false;
      m_PreviousPnt = cursorPosition;
    }
  } else {
    app.AddStringToMessageList(IDS_MSG_LINE_NOT_SELECTED);
  }
}

void AeSysView::OnLpdModeEll() {
  auto* document = GetDocument();
  if (document == nullptr) { return; }

  auto cursorPosition = GetCursorPosition();

  if (m_PreviousOp != 0) {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
  }
  if (m_PreviousOp == ID_OP2) {
    EoDbPoint* endPointPrimitive{};
    EoDbGroup* existingGroup = SelectPointUsingPoint(cursorPosition, 0.01, 15, 8, endPointPrimitive);
    if (existingGroup == nullptr) {
      app.AddStringToMessageList(IDS_MSG_LPD_NO_END_CAP_LOC);
      return;
    }
    cursorPosition = endPointPrimitive->GetPt();
    const Section existingSection(endPointPrimitive->GetDat(0), endPointPrimitive->GetDat(1), Section::Rectangular);

    EoDbPoint* beginPointPrimitive = existingGroup->GetFirstDifferentPoint(endPointPrimitive);
    if (beginPointPrimitive != nullptr) {
      EoGeLine existingSectionReferenceLine(beginPointPrimitive->GetPt(), cursorPosition);

      const EoGePoint3d intersectionPoint(existingSectionReferenceLine.ProjectPointToLine(m_PreviousPnt));
      double relationship{};
      if (!existingSectionReferenceLine.ComputeParametricRelation(intersectionPoint, relationship)) { return; }
      if (relationship > Eo::geometricTolerance) {
        m_currentReferenceLine(m_PreviousPnt, intersectionPoint);
        const double sectionLength = m_currentReferenceLine.Length()
            - (m_PreviousSection.Width() + m_DuctSeamSize + existingSection.Width() * 0.5);
        if (sectionLength > Eo::geometricTolerance) {
          m_currentReferenceLine.end = m_currentReferenceLine.ProjectToEndPoint(sectionLength);
          auto* group = new EoDbGroup;
          document->AddWorkLayerGroup(group);
          GenerateRectangularSection(m_currentReferenceLine, m_centerLineEccentricity, m_PreviousSection, group);
          document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        }
        auto* group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateFullElbowTakeoff(existingGroup, existingSectionReferenceLine, existingSection, group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      }
    }
    // determine where cursor should be moved to.
  }
  m_ContinueSection = false;
  m_PreviousOp = ID_OP2;
}

void AeSysView::OnLpdModeTee() {
  // EoGePoint3d CurrentPnt = GetCursorPosition();

  if (m_PreviousOp != 0) {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
  }
  // m_PreviousPnt = GenerateBullheadTee(this, m_PreviousPnt, CurrentPnt, m_PreviousSection);

  m_ContinueSection = false;
  m_PreviousOp = ID_OP2;
}

void AeSysView::OnLpdModeUpDown() {
  auto cursorPosition = GetCursorPosition();

  int iRet = 0;  // dialog to "Select direction", 'Up.Down.'
  if (iRet >= 0) {
    if (m_PreviousOp == ID_OP2) {
      cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);
      m_currentReferenceLine(m_PreviousPnt, cursorPosition);

      auto* document = GetDocument();
      if (m_ContinueSection) {
        auto* group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateRectangularElbow(
            m_previousReferenceLine, m_PreviousSection, m_currentReferenceLine, m_CurrentSection, group);
        m_PreviousSection = m_CurrentSection;
      }
      double sectionLength = m_currentReferenceLine.Length();
      if (sectionLength > m_PreviousSection.Depth() * 0.5 + m_DuctSeamSize) {
        EoGeLine referenceLine(m_currentReferenceLine);
        referenceLine.end = referenceLine.begin.ProjectToward(
            referenceLine.end, sectionLength - m_PreviousSection.Depth() * 0.5 - m_DuctSeamSize);
        auto* group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateRectangularSection(referenceLine, m_centerLineEccentricity, m_PreviousSection, group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        m_currentReferenceLine.begin = referenceLine.end;
      }
      auto* group = new EoDbGroup;
      document->AddWorkLayerGroup(group);
      GenerateRiseDrop(L"CONTINUOUS", m_PreviousSection, m_currentReferenceLine, group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
    m_ContinueSection = false;
    m_PreviousOp = ID_OP2;
    m_PreviousPnt = cursorPosition;
  }
}

void AeSysView::OnLpdModeSize() {
  double angle{};
  if (m_EndCapPoint != nullptr) {
    if (m_EndCapPoint->Color() == 15 && m_EndCapPoint->PointStyle() == 8) {
      auto position = m_EndCapGroup->Find(m_EndCapPoint);
      m_EndCapGroup->GetNext(position);
      auto* endCap = dynamic_cast<EoDbLine*>(m_EndCapGroup->GetAt(position));
      const EoGeLine line = endCap->Line();
      angle = fmod(line.AngleFromXAxisXY(), Eo::Pi);
      if (angle <= Eo::Radian) { angle += Eo::Pi; }
      angle -= Eo::HalfPi;
    }
    m_EndCapPoint = nullptr;
  }
  const auto cursorPosition = GetCursorPosition();

  GenSizeNote(cursorPosition, angle, m_PreviousSection);
  if (m_PreviousOp != 0) { RubberBandingDisable(); }
  m_PreviousOp = 0;
  m_ContinueSection = false;
}

void AeSysView::OnLpdModeReturn() {
  const auto cursorPosition = GetCursorPosition();

  if (m_PreviousOp != 0) { OnLpdModeEscape(); }
  m_PreviousPnt = cursorPosition;
}

void AeSysView::OnLpdModeEscape() {
  auto* document = GetDocument();
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateOverlay();

  if (!m_OriginalPreviousGroupDisplayed) {
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_OriginalPreviousGroup);
    m_OriginalPreviousGroupDisplayed = true;
  }
  ModeLineUnhighlightOp(m_PreviousOp);
  m_PreviousOp = 0;
  m_ContinueSection = false;
  m_EndCapGroup = nullptr;
  m_EndCapPoint = nullptr;
}

void AeSysView::DoDuctModeMouseMove() {
  const EoDbHandleSuppressionScope suppressHandles;
  if (m_PreviousOp == 0) {
    m_OriginalPreviousGroupDisplayed = true;
    return;
  }
  if (m_PreviousOp != ID_OP2) { return; }

  auto* document = GetDocument();
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  auto cursorPosition = GetCursorPosition();
  cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);
  m_currentReferenceLine(m_PreviousPnt, cursorPosition);

  if (m_ContinueSection
      && m_currentReferenceLine.Length() > m_PreviousSection.Width() * m_centerLineEccentricity + m_DuctSeamSize) {
    EoGeLine previousReferenceLine = m_previousReferenceLine;
    if (m_OriginalPreviousGroupDisplayed) {
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_OriginalPreviousGroup);
      m_OriginalPreviousGroupDisplayed = false;
    }
    GenerateRectangularElbow(
        previousReferenceLine, m_PreviousSection, m_currentReferenceLine, m_CurrentSection, &m_PreviewGroup);
    GenerateRectangularSection(previousReferenceLine, m_centerLineEccentricity, m_PreviousSection, &m_PreviewGroup);
  }
  EoDbPoint* endPointPrimitive{};
  auto* existingGroup =
      SelectPointUsingPoint(cursorPosition, endCapTolerance, endCapColor, endCapPointStyle, endPointPrimitive);
  if (existingGroup != nullptr) {
    cursorPosition = endPointPrimitive->GetPt();
    const Section existingSection(endPointPrimitive->GetDat(0), endPointPrimitive->GetDat(1), Section::Rectangular);

    auto* beginPointPrimitive = existingGroup->GetFirstDifferentPoint(endPointPrimitive);
    if (beginPointPrimitive != nullptr) {
      EoGeLine existingSectionReferenceLine(beginPointPrimitive->GetPt(), cursorPosition);

      EoGePoint3d intersectionPoint(existingSectionReferenceLine.ProjectPointToLine(m_PreviousPnt));
      double relationship{};
      if (!existingSectionReferenceLine.ComputeParametricRelation(intersectionPoint, relationship)) { return; }
      if (relationship > Eo::geometricTolerance) {
        m_currentReferenceLine(m_PreviousPnt, intersectionPoint);
        const double sectionLength = m_currentReferenceLine.Length()
            - (m_PreviousSection.Width() + m_DuctSeamSize + existingSection.Width() * 0.5);
        if (sectionLength > Eo::geometricTolerance) {
          m_currentReferenceLine.end = m_currentReferenceLine.ProjectToEndPoint(sectionLength);
          GenerateRectangularSection(
              m_currentReferenceLine, m_centerLineEccentricity, m_PreviousSection, &m_PreviewGroup);
        }
        GenerateFullElbowTakeoff(existingGroup, existingSectionReferenceLine, existingSection, &m_PreviewGroup);
      }
    }
  } else {
    const double transitionLength = (m_PreviousSection == m_CurrentSection)
        ? 0.0
        : LengthOfTransition(m_DuctJustification, m_TransitionSlope, m_PreviousSection, m_CurrentSection);
    EoGeLine referenceLine{m_currentReferenceLine};

    if (m_BeginWithTransition) {
      if (transitionLength != 0.0) {
        referenceLine.end = referenceLine.ProjectToEndPoint(transitionLength);
        GenerateTransition(referenceLine,
            m_centerLineEccentricity,
            m_DuctJustification,
            m_TransitionSlope,
            m_PreviousSection,
            m_CurrentSection,
            &m_PreviewGroup);
        referenceLine.begin = referenceLine.end;
        referenceLine.end = m_currentReferenceLine.end;
      }
      if (m_currentReferenceLine.Length() - transitionLength > Eo::geometricTolerance) {
        GenerateRectangularSection(referenceLine, m_centerLineEccentricity, m_CurrentSection, &m_PreviewGroup);
      }
    } else {
      if (referenceLine.Length() - transitionLength > Eo::geometricTolerance) {
        referenceLine.end = referenceLine.ProjectToBeginPoint(transitionLength);
        GenerateRectangularSection(referenceLine, m_centerLineEccentricity, m_PreviousSection, &m_PreviewGroup);
        referenceLine.begin = referenceLine.end;
        referenceLine.end = m_currentReferenceLine.end;
      }
      if (transitionLength != 0.0) {
        GenerateTransition(referenceLine,
            m_centerLineEccentricity,
            m_DuctJustification,
            m_TransitionSlope,
            m_PreviousSection,
            m_CurrentSection,
            &m_PreviewGroup);
      }
    }
  }
  m_PreviewGroup.RemoveDuplicatePrimitives();
  InvalidateOverlay();
}

void AeSysView::GenerateEndCap(EoGePoint3d& beginPoint, EoGePoint3d& endPoint, Section section, EoDbGroup* group) {
  const auto midpoint = EoGePoint3d::Mid(beginPoint, endPoint);

  double data[] = {section.Width(), section.Depth()};

  auto* pointPrimitive = new EoDbPoint(15, 8, midpoint);
  pointPrimitive->SetDat(2, data);
  group->AddTail(pointPrimitive);
  group->AddTail(EoDbLine::CreateLine(beginPoint, endPoint)->WithProperties(Gs::renderState));
}

EoGePoint3d AeSysView::GenerateBullheadTee(EoDbGroup* existingGroup,
    EoGeLine& existingSectionReferenceLine,
    double existingSectionWidth,
    double existingSectionDepth,
    EoDbGroup* group) {
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
  const EoGeVector3d newSectionDirection(existingSectionReferenceLine.begin, existingSectionReferenceLine.end);

  EoGePoint3d intersectionPoint{existingSectionReferenceLine.ProjectPointToLine(m_PreviousPnt)};
  EoGeLine previousReferenceLine(m_PreviousPnt, intersectionPoint);
  previousReferenceLine.end =
      previousReferenceLine.ProjectToBeginPoint((existingSection.Width() + m_PreviousSection.Width()) * 0.5);
  EoGeLine currentReferenceLine(previousReferenceLine.end, previousReferenceLine.end + newSectionDirection);

  GenerateRectangularElbow(previousReferenceLine, m_PreviousSection, currentReferenceLine, m_CurrentSection, group);
  intersectionPoint = existingSectionReferenceLine.ProjectPointToLine(currentReferenceLine.begin);
  double relationship;
  if (existingSectionReferenceLine.ComputeParametricRelation(intersectionPoint, relationship)) {
    if (std::abs(relationship) > Eo::geometricTolerance && std::abs(relationship - 1.0) > Eo::geometricTolerance) {
      // need to add a section either from the elbow or the existing section
      double sectionLength = existingSectionReferenceLine.Length();
      double distanceToBeginPoint = relationship * sectionLength;
      if (relationship > Eo::geometricTolerance
          && relationship < 1.0 - Eo::geometricTolerance) {  // section from the elbow
        currentReferenceLine.end =
            currentReferenceLine.begin.ProjectToward(currentReferenceLine.end, sectionLength - distanceToBeginPoint);
        GenerateRectangularSection(currentReferenceLine, m_centerLineEccentricity, m_PreviousSection, group);
      } else {
        distanceToBeginPoint = std::max(distanceToBeginPoint, sectionLength);
        existingSectionReferenceLine.end =
            existingSectionReferenceLine.begin.ProjectToward(existingSectionReferenceLine.end, distanceToBeginPoint);
      }
    }
    // generate the transition
    EoGePoint3d points[2]{};
    points[0] = existingSectionReferenceLine.end.ProjectToward(
        currentReferenceLine.end, existingSection.Width() * 0.5 + m_PreviousSection.Width());
    points[1] =
        points[0].ProjectToward(existingSectionReferenceLine.end, existingSection.Width() + m_PreviousSection.Width());

    const EoGePoint3d middleOfTransition = points[0] + EoGeVector3d(points[0], points[1]) * 0.5;
    EoGeLine transitionReferenceLine(middleOfTransition, middleOfTransition + newSectionDirection);

    const double width = m_PreviousSection.Width() + existingSection.Width();
    const double depth = m_PreviousSection.Depth() + existingSection.Depth();
    const Section continueGroup(width, depth, Section::Rectangular);
    const Section currentSection(width * 0.75, depth * 0.75, Section::Rectangular);

    GenerateTransition(transitionReferenceLine,
        m_centerLineEccentricity,
        m_DuctJustification,
        m_TransitionSlope,
        continueGroup,
        currentSection,
        group);
  }
  if (m_GenerateTurningVanes) {
    // TODO: Generate the splitter damper similar the one in `GenerateRectangulasrTap`
  }
}

void AeSysView::GenerateRiseDrop(const std::wstring& indicatorLineTypeName,
    Section section,
    EoGeLine& referenceLine,
    EoDbGroup* group) {
  const double sectionLength = referenceLine.Length();

  EoGeLine leftLine;
  EoGeLine rightLine;
  if (!referenceLine.GetParallels(section.Width(), m_centerLineEccentricity, leftLine, rightLine)) { return; }

  if (sectionLength >= section.Depth() * 0.5 + m_DuctSeamSize) {
    EoGeLine seamReferenceLine(referenceLine);
    seamReferenceLine.end = seamReferenceLine.begin.ProjectToward(seamReferenceLine.end, m_DuctSeamSize);
    if (!seamReferenceLine.GetParallels(section.Width(), m_centerLineEccentricity, leftLine, rightLine)) { return; }
    group->AddTail(EoDbLine::CreateLine(leftLine.begin, leftLine.end)->WithProperties(Gs::renderState));
    group->AddTail(EoDbLine::CreateLine(rightLine.begin, rightLine.end)->WithProperties(Gs::renderState));
    referenceLine.begin = seamReferenceLine.end;
  }
  (void)referenceLine.GetParallels(section.Width(), m_centerLineEccentricity, leftLine, rightLine);
  GenerateRectangularSection(referenceLine, m_centerLineEccentricity, section, group);
  // need to allow continuation perpendicular to vertical section ?

  group->AddTail(EoDbLine::CreateLine(leftLine.begin, rightLine.end)
          ->WithProperties(Gs::renderState.Color(), indicatorLineTypeName));
  group->AddTail(EoDbLine::CreateLine(rightLine.begin, leftLine.end)
          ->WithProperties(Gs::renderState.Color(), indicatorLineTypeName));
}

void AeSysView::GenerateRectangularElbow(EoGeLine& previousReferenceLine,
    Section previousSection,
    EoGeLine& currentReferenceLine,
    Section currentSection,
    EoDbGroup* group) {
  if (previousReferenceLine.ParallelTo(currentReferenceLine)) { return; }

  previousReferenceLine.end = previousReferenceLine.end.ProjectToward(
      previousReferenceLine.begin, m_DuctSeamSize + previousSection.Width() * m_centerLineEccentricity);

  EoGeLine previousLeftLine;
  EoGeLine previousRightLine;
  if (!previousReferenceLine.GetParallels(
          previousSection.Width(), m_centerLineEccentricity, previousLeftLine, previousRightLine)) {
    return;
  }
  currentReferenceLine.begin = currentReferenceLine.begin.ProjectToward(
      currentReferenceLine.end, m_DuctSeamSize + previousSection.Width() * m_centerLineEccentricity);

  EoGeLine currentLeftLine;
  EoGeLine currentRightLine;
  if (!currentReferenceLine.GetParallels(
          currentSection.Width(), m_centerLineEccentricity, currentLeftLine, currentRightLine)) {
    return;
  }
  EoGePoint3d insideCorner;
  EoGePoint3d outsideCorner;
  if (!EoGeLine::Intersection_xy(previousLeftLine, currentLeftLine, insideCorner)) { return; }
  if (!EoGeLine::Intersection_xy(previousRightLine, currentRightLine, outsideCorner)) { return; }

  GenerateEndCap(previousLeftLine.end, previousRightLine.end, previousSection, group);
  group->AddTail(EoDbLine::CreateLine(previousLeftLine.end, insideCorner)->WithProperties(Gs::renderState));
  group->AddTail(EoDbLine::CreateLine(insideCorner, currentLeftLine.begin)->WithProperties(Gs::renderState));
  group->AddTail(EoDbLine::CreateLine(previousRightLine.end, outsideCorner)->WithProperties(Gs::renderState));
  group->AddTail(EoDbLine::CreateLine(outsideCorner, currentRightLine.begin)->WithProperties(Gs::renderState));
  if (m_GenerateTurningVanes) {
    group->AddTail(EoDbLine::CreateLine(insideCorner, outsideCorner)->WithProperties(2, L"Dash2"));
  }
  GenerateEndCap(currentLeftLine.begin, currentRightLine.begin, currentSection, group);
}

void AeSysView::GenerateRectangularSection(EoGeLine& referenceLine,
    double eccentricity,
    Section section,
    EoDbGroup* group) {
  EoGeLine leftLine;
  EoGeLine rightLine;

  if (referenceLine.GetParallels(section.Width(), eccentricity, leftLine, rightLine)) {
    GenerateEndCap(leftLine.begin, rightLine.begin, section, group);

    group->AddTail(EoDbLine::CreateLine(leftLine.begin, leftLine.end)->WithProperties(Gs::renderState));
    GenerateEndCap(leftLine.end, rightLine.end, section, group);
    group->AddTail(EoDbLine::CreateLine(rightLine.begin, rightLine.end)->WithProperties(Gs::renderState));
  }
}

void AeSysView::GenSizeNote(EoGePoint3d point, double angle, Section section) {
  EoGeVector3d xDirection = RotateVectorAboutZAxis(EoGeVector3d(0.06, 0.0, 0.0), angle);
  EoGeVector3d yDirection = RotateVectorAboutZAxis(EoGeVector3d(0.0, 0.1, 0.0), angle);
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
  double sectionLength = m_currentReferenceLine.Length();

  if (sectionLength < m_DuctTapSize + m_DuctSeamSize) {
    m_currentReferenceLine.begin = m_currentReferenceLine.ProjectToBeginPoint(m_DuctTapSize + m_DuctSeamSize);
    sectionLength = m_DuctTapSize + m_DuctSeamSize;
  }
  EoGeLine referenceLine{m_currentReferenceLine};
  referenceLine.end = referenceLine.ProjectToEndPoint(m_DuctSeamSize);

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

  m_currentReferenceLine.begin = referenceLine.end;
  (void)m_currentReferenceLine.GetParallels(section.Width(), m_centerLineEccentricity, leftLine, rightLine);
  if (justification == Right) {
    rightLine.ProjPtFrom_xy(m_DuctTapSize, -m_DuctTapSize, &rightLine.end);
  } else {
    leftLine.ProjPtFrom_xy(m_DuctTapSize, m_DuctTapSize, &leftLine.end);
  }
  newSection->AddTail(EoDbLine::CreateLine(rightLine.begin, rightLine.end)->WithProperties(Gs::renderState));
  newSection->AddTail(EoDbLine::CreateLine(leftLine.end, leftLine.begin)->WithProperties(Gs::renderState));

  if (m_GenerateTurningVanes) {
    const EoGePoint3d beginPoint =
        ((justification == Left) ? rightLine : leftLine).ProjectToBeginPoint(-m_DuctTapSize / 3.0);
    const EoGePoint3d endPoint = m_currentReferenceLine.ProjectToBeginPoint(-m_DuctTapSize / 2.0);
    auto* circle = EoDbConic::CreateCircleInView(beginPoint, 0.01);
    circle->SetColor(1);
    circle->SetLineTypeName(Gs::renderState.LineTypeName());
    newSection->AddTail(circle);
    newSection->AddTail(EoDbLine::CreateLine(beginPoint, endPoint)->WithProperties(1, Gs::renderState.LineTypeName()));
  }
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, newSection);
  return true;
}

void AeSysView::GenerateTransition(EoGeLine& referenceLine,
    double eccentricity,
    EJust justification,
    double slope,
    Section previousSection,
    Section currentSection,
    EoDbGroup* group) {
  const double referenceLength = referenceLine.Length();
  if (referenceLength < Eo::geometricTolerance) { return; }

  const double widthChange = currentSection.Width() - previousSection.Width();
  double transitionLength = LengthOfTransition(justification, slope, previousSection, currentSection);
  transitionLength = std::min(transitionLength, referenceLength);

  EoGeLine leftLine;
  EoGeLine rightLine;
  if (!referenceLine.GetParallels(previousSection.Width(), eccentricity, leftLine, rightLine)) { return; }

  if (justification == Center) {
    leftLine.ProjPtFrom_xy(transitionLength, widthChange * 0.5, &leftLine.end);
    rightLine.ProjPtFrom_xy(transitionLength, -widthChange * 0.5, &rightLine.end);
  } else if (justification == Right) {
    rightLine.ProjPtFrom_xy(transitionLength, -widthChange, &rightLine.end);
  } else {
    leftLine.ProjPtFrom_xy(transitionLength, widthChange, &leftLine.end);
  }
  GenerateEndCap(leftLine.begin, rightLine.begin, previousSection, group);
  group->AddTail(EoDbLine::CreateLine(rightLine.begin, rightLine.end)->WithProperties(Gs::renderState));
  GenerateEndCap(rightLine.end, leftLine.end, currentSection, group);
  group->AddTail(EoDbLine::CreateLine(leftLine.end, leftLine.begin)->WithProperties(Gs::renderState));
}

void AeSysView::SetDuctOptions(Section& section) {
  const Eo::Units units = app.GetUnits();
  app.SetUnits(std::max(units, Eo::Units::Inches));

  EoDlgLowPressureDuctOptions dlg(this);

  dlg.m_Width = section.Width();
  dlg.m_Depth = section.Depth();
  dlg.m_RadiusFactor = m_InsideRadiusFactor;
  dlg.m_Justification = m_DuctJustification;
  dlg.m_GenerateVanes = m_GenerateTurningVanes;
  dlg.m_BeginWithTransition = m_BeginWithTransition;
  if (dlg.DoModal() == IDOK) {
    section.SetWidth(dlg.m_Width);
    section.SetDepth(dlg.m_Depth);
    m_InsideRadiusFactor = dlg.m_RadiusFactor;
    m_DuctJustification = EJust(dlg.m_Justification);
    m_GenerateTurningVanes = dlg.m_GenerateVanes;
    m_BeginWithTransition = dlg.m_BeginWithTransition;
  }
  app.SetUnits(units);
}

double AeSysView::LengthOfTransition(EJust justification,
    double slope,
    Section previousSection,
    Section currentSection) {
  const double widthChange = currentSection.Width() - previousSection.Width();
  const double depthChange = currentSection.Depth() - previousSection.Depth();

  double length = std::max(std::abs(widthChange), std::abs(depthChange)) * slope;
  if (justification == Center) { length *= 0.5; }
  return length;
}

EoDbGroup* AeSysView::SelectPointUsingPoint(EoGePoint3d& cursorPosition,
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

bool AeSysView::Find2LinesUsingLineEndpoints(EoDbLine* testLinePrimitive,
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
    auto* group = GetPreviousGroup(groupPosition);

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
      double lineAngle = fmod(line.AngleFromXAxisXY(), Eo::Pi);
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
