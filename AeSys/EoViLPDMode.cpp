#include "Stdafx.h"
#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
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

/** @attention Only check for actual end-cap marker is by attributes. No error processing for invalid width or depth values.
 * Group data contains whatever primative follows marker (hopefully this is associated end-cap line).
 * Issues:
 * xor operations on transition not clean
 * ending section with 3 key will generate a shortened section if the point is less than transition length from the begin point.
 * full el only works with center just
 */

void AeSysView::OnLpdModeOptions() { SetDuctOptions(m_CurrentSection); }

void AeSysView::OnLpdModeJoin() {
  auto cursorPosition = GetCursorPosition();

  m_EndCapGroup = SelectPointUsingPoint(cursorPosition, 0.01, 15, 8, m_EndCapPoint);
  if (m_EndCapGroup != 0) {
    m_PreviousPnt = m_EndCapPoint->GetPt();
    m_PreviousSection.SetWidth(m_EndCapPoint->GetDat(0));
    m_PreviousSection.SetDepth(m_EndCapPoint->GetDat(1));
    m_ContinueSection = false;

    m_EndCapLocation = (m_PreviousOp == 0) ? 1 : -1;  // 1 (start) and -1 (end)

    CString Message(L"Cross sectional dimension (Width by Depth) is ");
    CString Length;
    app.FormatLength(Length, std::max(app.GetUnits(), Eo::Units::Inches), m_PreviousSection.Width(), 12, 2);
    CString Width;
    app.FormatLength(Width, std::max(app.GetUnits(), Eo::Units::Inches), m_PreviousSection.Depth(), 12, 2);
    Message.Append(Length.TrimLeft() + L" by " + Width.TrimLeft());
    app.AddStringToMessageList(Message);
    SetCursorPosition(m_PreviousPnt);
  }
}

void AeSysView::OnLpdModeDuct() {
  auto cursorPosition = GetCursorPosition();

  if (m_PreviousOp != 0) { m_PreviewGroup.DeletePrimitivesAndRemoveAll(); }
  if (m_PreviousOp == ID_OP2) {
    cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);
    m_CurrentReferenceLine(m_PreviousPnt, cursorPosition);

    auto* document = GetDocument();
    if (m_ContinueSection) {
      auto* Group = new EoDbGroup;
      document->AddWorkLayerGroup(Group);
      GenerateRectangularElbow(
          m_PreviousReferenceLine, m_PreviousSection, m_CurrentReferenceLine, m_CurrentSection, Group);
      m_OriginalPreviousGroup->DeletePrimitivesAndRemoveAll();
      GenerateRectangularSection(
          m_PreviousReferenceLine, m_CenterLineEccentricity, m_PreviousSection, m_OriginalPreviousGroup);
      m_OriginalPreviousGroupDisplayed = true;
      m_PreviousSection = m_CurrentSection;
    }
    double TransitionLength =
        (m_PreviousSection == m_CurrentSection)
            ? 0.
            : LengthOfTransition(m_DuctJustification, m_TransitionSlope, m_PreviousSection, m_CurrentSection);
    EoGeLine ReferenceLine(m_CurrentReferenceLine);

    if (m_BeginWithTransition) {
      if (TransitionLength != 0.0) {
        ReferenceLine.end = ReferenceLine.ProjToEndPt(TransitionLength);

        auto* Group = new EoDbGroup;
        document->AddWorkLayerGroup(Group);
        GenerateTransition(ReferenceLine, m_CenterLineEccentricity, m_DuctJustification, m_TransitionSlope,
            m_PreviousSection, m_CurrentSection, Group);
        ReferenceLine.begin = ReferenceLine.end;
        ReferenceLine.end = m_CurrentReferenceLine.end;
        m_ContinueSection = false;
      }
      if (m_CurrentReferenceLine.Length() - TransitionLength > Eo::geometricTolerance) {
        m_OriginalPreviousGroup = new EoDbGroup;
        document->AddWorkLayerGroup(m_OriginalPreviousGroup);
        GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_CurrentSection, m_OriginalPreviousGroup);
        m_ContinueSection = true;
      }
    } else {
      if (ReferenceLine.Length() - TransitionLength > Eo::geometricTolerance) {
        ReferenceLine.end = ReferenceLine.ProjToBegPt(TransitionLength);
        m_OriginalPreviousGroup = new EoDbGroup;
        document->AddWorkLayerGroup(m_OriginalPreviousGroup);
        GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_PreviousSection, m_OriginalPreviousGroup);
        ReferenceLine.begin = ReferenceLine.end;
        ReferenceLine.end = m_CurrentReferenceLine.end;
        m_ContinueSection = true;
      }
      if (TransitionLength != 0.0) {
        auto* Group = new EoDbGroup;
        document->AddWorkLayerGroup(Group);
        GenerateTransition(ReferenceLine, m_CenterLineEccentricity, m_DuctJustification, m_TransitionSlope,
            m_PreviousSection, m_CurrentSection, Group);
        m_ContinueSection = false;
      }
    }
    m_PreviousReferenceLine = m_CurrentReferenceLine;
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
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
  EoDbLine* LinePrimitive;
  auto* group = SelectLineUsingPoint(cursorPosition, LinePrimitive);
  if (group != nullptr) {
    EoGePoint3d TestPoint(cursorPosition);
    cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);
    cursorPosition = LinePrimitive->ProjectPointToLine(cursorPosition);
    m_CurrentReferenceLine(m_PreviousPnt, cursorPosition);

    EJust Justification;
    int Relationship = m_CurrentReferenceLine.DirRelOfPt(TestPoint);
    if (Relationship == 1) {
      Justification = Left;
    } else if (Relationship == -1) {
      Justification = Right;
    } else {
      app.AddStringToMessageList(std::wstring(L"Could not determine orientation of component"));
      return;
    }
    if (m_PreviousOp == ID_OP2) {
      if (m_ContinueSection) {
        group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateRectangularElbow(
            m_PreviousReferenceLine, m_PreviousSection, m_CurrentReferenceLine, m_CurrentSection, group);
        m_PreviousSection = m_CurrentSection;
      }
      double SectionLength = m_CurrentReferenceLine.Length();
      if (SectionLength >= m_DuctTapSize + m_DuctSeamSize) {
        EoGeLine ReferenceLine(m_CurrentReferenceLine);
        ReferenceLine.end = ReferenceLine.ProjToBegPt(m_DuctTapSize + m_DuctSeamSize);
        group = new EoDbGroup;
        document->AddWorkLayerGroup(group);
        GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_PreviousSection, group);
        m_CurrentReferenceLine.begin = ReferenceLine.end;
        m_PreviousReferenceLine = m_CurrentReferenceLine;
        m_PreviousSection = m_CurrentSection;
      }
      GenerateRectangularTap(Justification, m_PreviousSection);
      m_PreviousOp = 0;
      m_ContinueSection = false;
      m_PreviousPnt = cursorPosition;
    }
  } else {
    app.AddStringToMessageList(IDS_MSG_LINE_NOT_SELECTED);
  }
}

void AeSysView::OnLpdModeEll() {
  auto cursorPosition = GetCursorPosition();

  auto* document = GetDocument();
  if (m_PreviousOp != 0) {
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
  if (m_PreviousOp == ID_OP2) {
    EoDbPoint* EndPointPrimitive = 0;
    EoDbGroup* ExistingGroup = SelectPointUsingPoint(cursorPosition, 0.01, 15, 8, EndPointPrimitive);
    if (ExistingGroup == 0) {
      app.AddStringToMessageList(IDS_MSG_LPD_NO_END_CAP_LOC);
      return;
    }
    cursorPosition = EndPointPrimitive->GetPt();
    Section ExistingSection(EndPointPrimitive->GetDat(0), EndPointPrimitive->GetDat(1), Section::Rectangular);

    EoDbPoint* BeginPointPrimitive = ExistingGroup->GetFirstDifferentPoint(EndPointPrimitive);
    if (BeginPointPrimitive != 0) {
      EoGeLine ExistingSectionReferenceLine(BeginPointPrimitive->GetPt(), cursorPosition);

      EoGePoint3d IntersectionPoint(ExistingSectionReferenceLine.ProjectPointToLine(m_PreviousPnt));
      double Relationship;
      ExistingSectionReferenceLine.RelOfPtToEndPts(IntersectionPoint, Relationship);
      if (Relationship > Eo::geometricTolerance) {
        m_CurrentReferenceLine(m_PreviousPnt, IntersectionPoint);
        double SectionLength = m_CurrentReferenceLine.Length() -
                               (m_PreviousSection.Width() + m_DuctSeamSize + ExistingSection.Width() * 0.5);
        if (SectionLength > Eo::geometricTolerance) {
          m_CurrentReferenceLine.end = m_CurrentReferenceLine.ProjToEndPt(SectionLength);
          auto* Group = new EoDbGroup;
          document->AddWorkLayerGroup(Group);
          GenerateRectangularSection(m_CurrentReferenceLine, m_CenterLineEccentricity, m_PreviousSection, Group);
          document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        }
        auto* Group = new EoDbGroup;
        document->AddWorkLayerGroup(Group);
        GenerateFullElbowTakeoff(ExistingGroup, ExistingSectionReferenceLine, ExistingSection, Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      }
    }
    // determine where cursor should be moved to.
  }
  m_ContinueSection = false;
  m_PreviousOp = ID_OP2;
}

void AeSysView::OnLpdModeTee() {
  //EoGePoint3d CurrentPnt = GetCursorPosition();

  if (m_PreviousOp != 0) {
    auto* document = GetDocument();
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
  //m_PreviousPnt = GenerateBullheadTee(this, m_PreviousPnt, CurrentPnt, m_PreviousSection);

  m_ContinueSection = false;
  m_PreviousOp = ID_OP2;
}

void AeSysView::OnLpdModeUpDown() {
  auto cursorPosition = GetCursorPosition();

  int iRet = 0;  // dialog to "Select direction", 'Up.Down.'
  if (iRet >= 0) {
    if (m_PreviousOp == ID_OP2) {
      cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);
      m_CurrentReferenceLine(m_PreviousPnt, cursorPosition);

      auto* document = GetDocument();
      if (m_ContinueSection) {
        auto* Group = new EoDbGroup;
        document->AddWorkLayerGroup(Group);
        GenerateRectangularElbow(
            m_PreviousReferenceLine, m_PreviousSection, m_CurrentReferenceLine, m_CurrentSection, Group);
        m_PreviousSection = m_CurrentSection;
      }
      double SectionLength = m_CurrentReferenceLine.Length();
      if (SectionLength > m_PreviousSection.Depth() * 0.5 + m_DuctSeamSize) {
        EoGeLine ReferenceLine(m_CurrentReferenceLine);
        ReferenceLine.end = ReferenceLine.begin.ProjectToward(
            ReferenceLine.end, SectionLength - m_PreviousSection.Depth() * 0.5 - m_DuctSeamSize);
        auto* Group = new EoDbGroup;
        document->AddWorkLayerGroup(Group);
        GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_PreviousSection, Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        m_CurrentReferenceLine.begin = ReferenceLine.end;
      }
      auto* Group = new EoDbGroup;
      document->AddWorkLayerGroup(Group);
      GenerateRiseDrop(1, m_PreviousSection, m_CurrentReferenceLine, Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
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
      auto* endCap = static_cast<EoDbLine*>(m_EndCapGroup->GetAt(position));
      EoGeLine line = endCap->Line();
      angle = fmod(line.AngleFromXAxisXY(), Eo::Pi);
      if (angle <= Eo::Radian) { angle += Eo::Pi; }
      angle -= Eo::HalfPi;
    }
    m_EndCapPoint = nullptr;
  }
  auto cursorPosition = GetCursorPosition();

  GenSizeNote(cursorPosition, angle, m_PreviousSection);
  if (m_PreviousOp != 0) { RubberBandingDisable(); }
  m_PreviousOp = 0;
  m_ContinueSection = false;
}

void AeSysView::OnLpdModeReturn() {
  auto cursorPosition = GetCursorPosition();

  if (m_PreviousOp != 0) { OnLpdModeEscape(); }
  m_PreviousPnt = cursorPosition;
}

void AeSysView::OnLpdModeEscape() {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  if (!m_OriginalPreviousGroupDisplayed) {
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_OriginalPreviousGroup);
    m_OriginalPreviousGroupDisplayed = true;
  }
  ModeLineUnhighlightOp(m_PreviousOp);
  m_ContinueSection = false;
  m_EndCapGroup = 0;
  m_EndCapPoint = 0;
}

void AeSysView::DoDuctModeMouseMove() {
  if (m_PreviousOp == 0) {
    m_OriginalPreviousGroupDisplayed = true;
    return;
  }
  if (m_PreviousOp != ID_OP2) { return; }

  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  auto cursorPosition = GetCursorPosition();
  cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);
  m_CurrentReferenceLine(m_PreviousPnt, cursorPosition);

  if (m_ContinueSection &&
      m_CurrentReferenceLine.Length() > m_PreviousSection.Width() * m_CenterLineEccentricity + m_DuctSeamSize) {
    EoGeLine PreviousReferenceLine = m_PreviousReferenceLine;
    if (m_OriginalPreviousGroupDisplayed) {
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_OriginalPreviousGroup);
      m_OriginalPreviousGroupDisplayed = false;
    }
    GenerateRectangularElbow(
        PreviousReferenceLine, m_PreviousSection, m_CurrentReferenceLine, m_CurrentSection, &m_PreviewGroup);
    GenerateRectangularSection(PreviousReferenceLine, m_CenterLineEccentricity, m_PreviousSection, &m_PreviewGroup);
  }
  EoDbPoint* EndPointPrimitive{};
  auto* ExistingGroup =
      SelectPointUsingPoint(cursorPosition, endCapTolerance, endCapColor, endCapPointStyle, EndPointPrimitive);
  if (ExistingGroup != nullptr) {
    cursorPosition = EndPointPrimitive->GetPt();
    Section ExistingSection(EndPointPrimitive->GetDat(0), EndPointPrimitive->GetDat(1), Section::Rectangular);

    auto* BeginPointPrimitive = ExistingGroup->GetFirstDifferentPoint(EndPointPrimitive);
    if (BeginPointPrimitive != nullptr) {
      EoGeLine ExistingSectionReferenceLine(BeginPointPrimitive->GetPt(), cursorPosition);

      EoGePoint3d IntersectionPoint(ExistingSectionReferenceLine.ProjectPointToLine(m_PreviousPnt));
      double Relationship;
      ExistingSectionReferenceLine.RelOfPtToEndPts(IntersectionPoint, Relationship);
      if (Relationship > Eo::geometricTolerance) {
        m_CurrentReferenceLine(m_PreviousPnt, IntersectionPoint);
        double SectionLength = m_CurrentReferenceLine.Length() -
                               (m_PreviousSection.Width() + m_DuctSeamSize + ExistingSection.Width() * 0.5);
        if (SectionLength > Eo::geometricTolerance) {
          m_CurrentReferenceLine.end = m_CurrentReferenceLine.ProjToEndPt(SectionLength);
          GenerateRectangularSection(
              m_CurrentReferenceLine, m_CenterLineEccentricity, m_PreviousSection, &m_PreviewGroup);
        }
        GenerateFullElbowTakeoff(ExistingGroup, ExistingSectionReferenceLine, ExistingSection, &m_PreviewGroup);
      }
    }
  } else {
    double TransitionLength =
        (m_PreviousSection == m_CurrentSection)
            ? 0.0
            : LengthOfTransition(m_DuctJustification, m_TransitionSlope, m_PreviousSection, m_CurrentSection);
    EoGeLine ReferenceLine(m_CurrentReferenceLine);

    if (m_BeginWithTransition) {
      if (TransitionLength != 0.0) {
        ReferenceLine.end = ReferenceLine.ProjToEndPt(TransitionLength);
        GenerateTransition(ReferenceLine, m_CenterLineEccentricity, m_DuctJustification, m_TransitionSlope,
            m_PreviousSection, m_CurrentSection, &m_PreviewGroup);
        ReferenceLine.begin = ReferenceLine.end;
        ReferenceLine.end = m_CurrentReferenceLine.end;
      }
      if (m_CurrentReferenceLine.Length() - TransitionLength > Eo::geometricTolerance) {
        GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_CurrentSection, &m_PreviewGroup);
      }
    } else {
      if (ReferenceLine.Length() - TransitionLength > Eo::geometricTolerance) {
        ReferenceLine.end = ReferenceLine.ProjToBegPt(TransitionLength);
        GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_PreviousSection, &m_PreviewGroup);
        ReferenceLine.begin = ReferenceLine.end;
        ReferenceLine.end = m_CurrentReferenceLine.end;
      }
      if (TransitionLength != 0.0) {
        GenerateTransition(ReferenceLine, m_CenterLineEccentricity, m_DuctJustification, m_TransitionSlope,
            m_PreviousSection, m_CurrentSection, &m_PreviewGroup);
      }
    }
  }
  m_PreviewGroup.RemoveDuplicatePrimitives();
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
}

void AeSysView::GenerateEndCap(EoGePoint3d& beginPoint, EoGePoint3d& endPoint, Section section, EoDbGroup* group) {
  EoGePoint3d Midpoint = EoGePoint3d::Mid(beginPoint, endPoint);

  double Data[] = {section.Width(), section.Depth()};

  EoDbPoint* PointPrimitive = new EoDbPoint(15, 8, Midpoint);
  PointPrimitive->SetDat(2, Data);
  group->AddTail(PointPrimitive);
  group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), beginPoint, endPoint));
}

EoGePoint3d AeSysView::GenerateBullheadTee(EoDbGroup* existingGroup, EoGeLine& existingSectionReferenceLine,
    double existingSectionWidth, double existingSectionDepth, EoDbGroup* group) {
  (void)existingGroup;
  (void)existingSectionReferenceLine;
  (void)existingSectionWidth;
  (void)existingSectionDepth;
  (void)group;
  return EoGePoint3d(0.0, 0.0, 0.0);
};

void AeSysView::GenerateFullElbowTakeoff(
    EoDbGroup*, EoGeLine& existingSectionReferenceLine, Section existingSection, EoDbGroup* group) {
  EoGeVector3d NewSectionDirection(existingSectionReferenceLine.begin, existingSectionReferenceLine.end);

  EoGePoint3d IntersectionPoint(existingSectionReferenceLine.ProjectPointToLine(m_PreviousPnt));
  EoGeLine PreviousReferenceLine(m_PreviousPnt, IntersectionPoint);
  PreviousReferenceLine.end =
      PreviousReferenceLine.ProjToBegPt((existingSection.Width() + m_PreviousSection.Width()) * 0.5);
  EoGeLine CurrentReferenceLine(PreviousReferenceLine.end, PreviousReferenceLine.end + NewSectionDirection);

  GenerateRectangularElbow(PreviousReferenceLine, m_PreviousSection, CurrentReferenceLine, m_CurrentSection, group);
  IntersectionPoint = existingSectionReferenceLine.ProjectPointToLine(CurrentReferenceLine.begin);
  double Relationship;
  if (existingSectionReferenceLine.RelOfPtToEndPts(IntersectionPoint, Relationship)) {
    if (fabs(Relationship) > Eo::geometricTolerance && fabs(Relationship - 1.0) > Eo::geometricTolerance) {
      // need to add a section either from the elbow or the existing section
      double SectionLength = existingSectionReferenceLine.Length();
      double DistanceToBeginPoint = Relationship * SectionLength;
      if (Relationship > Eo::geometricTolerance &&
          Relationship < 1.0 - Eo::geometricTolerance) {  // section from the elbow
        CurrentReferenceLine.end =
            CurrentReferenceLine.begin.ProjectToward(CurrentReferenceLine.end, SectionLength - DistanceToBeginPoint);
        GenerateRectangularSection(CurrentReferenceLine, m_CenterLineEccentricity, m_PreviousSection, group);
      } else {
        DistanceToBeginPoint = std::max(DistanceToBeginPoint, SectionLength);
        existingSectionReferenceLine.end =
            existingSectionReferenceLine.begin.ProjectToward(existingSectionReferenceLine.end, DistanceToBeginPoint);
      }
    }
    // generate the transition
    EoGePoint3d Points[2]{};
    Points[0] = existingSectionReferenceLine.end.ProjectToward(
        CurrentReferenceLine.end, existingSection.Width() * 0.5 + m_PreviousSection.Width());
    Points[1] =
        Points[0].ProjectToward(existingSectionReferenceLine.end, existingSection.Width() + m_PreviousSection.Width());

    EoGePoint3d MiddleOfTransition = Points[0] + EoGeVector3d(Points[0], Points[1]) * 0.5;
    EoGeLine TransitionReferenceLine(MiddleOfTransition, MiddleOfTransition + NewSectionDirection);

    double Width = m_PreviousSection.Width() + existingSection.Width();
    double Depth = m_PreviousSection.Depth() + existingSection.Depth();
    Section ContinueGroup(Width, Depth, Section::Rectangular);
    Section CurrentSection(Width * 0.75, Depth * 0.75, Section::Rectangular);

    GenerateTransition(TransitionReferenceLine, m_CenterLineEccentricity, m_DuctJustification, m_TransitionSlope,
        ContinueGroup, CurrentSection, group);
  }
  if (m_GenerateTurningVanes) {
    // TODO: Generate the splitter damper similar the one in `GenerateRectangulasrTap`
  }
}

void AeSysView::GenerateRiseDrop(
    std::uint16_t riseDropIndicator, Section section, EoGeLine& referenceLine, EoDbGroup* group) {
  double SectionLength = referenceLine.Length();

  EoGeLine LeftLine;
  EoGeLine RightLine;
  referenceLine.GetParallels(section.Width(), m_CenterLineEccentricity, LeftLine, RightLine);

  if (SectionLength >= section.Depth() * 0.5 + m_DuctSeamSize) {
    EoGeLine ReferenceLine(referenceLine);
    ReferenceLine.end = ReferenceLine.begin.ProjectToward(ReferenceLine.end, m_DuctSeamSize);
    ReferenceLine.GetParallels(section.Width(), m_CenterLineEccentricity, LeftLine, RightLine);
    group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), LeftLine.begin, LeftLine.end));
    group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), RightLine.begin, RightLine.end));
    referenceLine.begin = ReferenceLine.end;
  }
  referenceLine.GetParallels(section.Width(), m_CenterLineEccentricity, LeftLine, RightLine);
  GenerateRectangularSection(referenceLine, m_CenterLineEccentricity, section, group);
  // need to allow continuation perpendicular to vertical section ?

  group->AddTail(new EoDbLine(pstate.Color(), static_cast<std::int16_t>(riseDropIndicator), LeftLine.begin, RightLine.end));
  group->AddTail(new EoDbLine(pstate.Color(), static_cast<std::int16_t>(riseDropIndicator), RightLine.begin, LeftLine.end));
}

void AeSysView::GenerateRectangularElbow(EoGeLine& previousReferenceLine, Section previousSection,
    EoGeLine& currentReferenceLine, Section currentSection, EoDbGroup* group) {
  if (previousReferenceLine.ParallelTo(currentReferenceLine)) return;

  previousReferenceLine.end = previousReferenceLine.end.ProjectToward(
      previousReferenceLine.begin, m_DuctSeamSize + previousSection.Width() * m_CenterLineEccentricity);

  EoGeLine PreviousLeftLine;
  EoGeLine PreviousRightLine;
  previousReferenceLine.GetParallels(
      previousSection.Width(), m_CenterLineEccentricity, PreviousLeftLine, PreviousRightLine);

  currentReferenceLine.begin = currentReferenceLine.begin.ProjectToward(
      currentReferenceLine.end, m_DuctSeamSize + previousSection.Width() * m_CenterLineEccentricity);

  EoGeLine CurrentLeftLine;
  EoGeLine CurrentRightLine;
  currentReferenceLine.GetParallels(
      currentSection.Width(), m_CenterLineEccentricity, CurrentLeftLine, CurrentRightLine);

  EoGePoint3d InsideCorner;
  EoGePoint3d OutsideCorner;
  EoGeLine::Intersection_xy(PreviousLeftLine, CurrentLeftLine, InsideCorner);
  EoGeLine::Intersection_xy(PreviousRightLine, CurrentRightLine, OutsideCorner);

  GenerateEndCap(PreviousLeftLine.end, PreviousRightLine.end, previousSection, group);
  group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), PreviousLeftLine.end, InsideCorner));
  group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), InsideCorner, CurrentLeftLine.begin));
  group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), PreviousRightLine.end, OutsideCorner));
  group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), OutsideCorner, CurrentRightLine.begin));
  if (m_GenerateTurningVanes) { group->AddTail(new EoDbLine(2, 2, InsideCorner, OutsideCorner)); }
  GenerateEndCap(CurrentLeftLine.begin, CurrentRightLine.begin, currentSection, group);
}

void AeSysView::GenerateRectangularSection(
    EoGeLine& referenceLine, double eccentricity, Section section, EoDbGroup* group) {
  EoGeLine LeftLine;
  EoGeLine RightLine;

  if (referenceLine.GetParallels(section.Width(), eccentricity, LeftLine, RightLine)) {
    GenerateEndCap(LeftLine.begin, RightLine.begin, section, group);

    group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), LeftLine.begin, LeftLine.end));
    GenerateEndCap(LeftLine.end, RightLine.end, section, group);
    group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), RightLine.begin, RightLine.end));
  }
}

void AeSysView::GenSizeNote(EoGePoint3d point, double angle, Section section) {
  EoGeVector3d XDirection = RotateVectorAboutZAxis(EoGeVector3d(0.06, 0.0, 0.0), angle);
  EoGeVector3d YDirection = RotateVectorAboutZAxis(EoGeVector3d(0.0, 0.1, 0.0), angle);
  EoGeReferenceSystem ReferenceSystem(point, XDirection, YDirection);

  CString Width;
  app.FormatLength(Width, std::max(app.GetUnits(), Eo::Units::Inches), section.Width(), 0, 2);
  CString Depth;
  app.FormatLength(Depth, std::max(app.GetUnits(), Eo::Units::Inches), section.Depth(), 0, 2);
  CString Note = Width.TrimLeft() + L"/" + Depth.TrimLeft();

  auto* deviceContext = GetDC();
  int PrimitiveState = pstate.Save();
  pstate.SetColor(deviceContext, 2);

  EoDbFontDefinition fontDefinition = pstate.FontDefinition();
  fontDefinition.SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

  EoDbCharacterCellDefinition characterCellDefinition = pstate.CharacterCellDefinition();
  characterCellDefinition.SetRotationAngle(0.0);
  pstate.SetCharacterCellDefinition(characterCellDefinition);

  auto* Group = new EoDbGroup(new EoDbText(fontDefinition, ReferenceSystem, Note));
  auto* document = GetDocument();
  document->AddWorkLayerGroup(Group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
  pstate.Restore(deviceContext, PrimitiveState);
  ReleaseDC(deviceContext);
}

bool AeSysView::GenerateRectangularTap(EJust justification, Section section) {
  EoGeLine LeftLine;
  EoGeLine RightLine;

  double SectionLength = m_CurrentReferenceLine.Length();

  if (SectionLength < m_DuctTapSize + m_DuctSeamSize) {
    m_CurrentReferenceLine.begin = m_CurrentReferenceLine.ProjToBegPt(m_DuctTapSize + m_DuctSeamSize);
    SectionLength = m_DuctTapSize + m_DuctSeamSize;
  }
  EoGeLine ReferenceLine(m_CurrentReferenceLine);
  ReferenceLine.end = ReferenceLine.ProjToEndPt(m_DuctSeamSize);
  ReferenceLine.GetParallels(section.Width(), m_CenterLineEccentricity, LeftLine, RightLine);

  EoDbGroup* Section = new EoDbGroup;
  auto* document = GetDocument();
  document->AddWorkLayerGroup(Section);

  GenerateEndCap(LeftLine.begin, RightLine.begin, section, Section);

  Section->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), RightLine.begin, RightLine.end));
  Section->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), RightLine.end, LeftLine.end));
  Section->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), LeftLine.begin, LeftLine.end));

  m_CurrentReferenceLine.begin = ReferenceLine.end;
  m_CurrentReferenceLine.GetParallels(section.Width(), m_CenterLineEccentricity, LeftLine, RightLine);

  if (justification == Right) {
    RightLine.ProjPtFrom_xy(m_DuctTapSize, -m_DuctTapSize, &RightLine.end);
  } else {
    LeftLine.ProjPtFrom_xy(m_DuctTapSize, m_DuctTapSize, &LeftLine.end);
  }
  Section->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), RightLine.begin, RightLine.end));
  Section->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), LeftLine.end, LeftLine.begin));

  if (m_GenerateTurningVanes) {
    EoGePoint3d beginPoint = ((justification == Left) ? RightLine : LeftLine).ProjToBegPt(-m_DuctTapSize / 3.0);
    EoGePoint3d endPoint = m_CurrentReferenceLine.ProjToBegPt(-m_DuctTapSize / 2.0);
    auto* circle = EoDbConic::CreateCircleInView(beginPoint, 0.01);
    circle->SetColor(1);
    circle->SetLineTypeIndex(pstate.LineTypeIndex());
    Section->AddTail(circle);
    Section->AddTail(new EoDbLine(1, pstate.LineTypeIndex(), beginPoint, endPoint));
  }
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Section);
  return true;
}

void AeSysView::GenerateTransition(EoGeLine& referenceLine, double eccentricity, EJust justification, double slope,
    Section previousSection, Section currentSection, EoDbGroup* group) {
  double ReferenceLength = referenceLine.Length();
  if (ReferenceLength < Eo::geometricTolerance) { return; }

  double WidthChange = currentSection.Width() - previousSection.Width();
  double TransitionLength = LengthOfTransition(justification, slope, previousSection, currentSection);
  TransitionLength = std::min(TransitionLength, ReferenceLength);

  EoGeLine LeftLine;
  EoGeLine RightLine;
  referenceLine.GetParallels(previousSection.Width(), eccentricity, LeftLine, RightLine);

  if (justification == Center) {
    LeftLine.ProjPtFrom_xy(TransitionLength, WidthChange * 0.5, &LeftLine.end);
    RightLine.ProjPtFrom_xy(TransitionLength, -WidthChange * 0.5, &RightLine.end);
  } else if (justification == Right) {
    RightLine.ProjPtFrom_xy(TransitionLength, -WidthChange, &RightLine.end);
  } else {
    LeftLine.ProjPtFrom_xy(TransitionLength, WidthChange, &LeftLine.end);
  }
  GenerateEndCap(LeftLine.begin, RightLine.begin, previousSection, group);
  group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), RightLine.begin, RightLine.end));
  GenerateEndCap(RightLine.end, LeftLine.end, currentSection, group);
  group->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), LeftLine.end, LeftLine.begin));
}

void AeSysView::SetDuctOptions(Section& section) {
  Eo::Units units = app.GetUnits();
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

double AeSysView::LengthOfTransition(
    EJust justification, double slope, Section previousSection, Section currentSection) {
  double widthChange = currentSection.Width() - previousSection.Width();
  double depthChange = currentSection.Depth() - previousSection.Depth();

  double length = std::max(fabs(widthChange), fabs(depthChange)) * slope;
  if (justification == Center) { length *= 0.5; }
  return length;
}

EoDbGroup* AeSysView::SelectPointUsingPoint(
    EoGePoint3d& cursorPosition, double tolerance, std::int16_t color, std::int16_t pointStyle, EoDbPoint*& endCapPoint) {
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

bool AeSysView::Find2LinesUsingLineEndpoints(
    EoDbLine* testLinePrimitive, double angularTolerance, EoGeLine& leftLine, EoGeLine& rightLine) {
  EoGeLine Line;

  EoDbLine* LeftLinePrimitive{};
  EoDbLine* RightLinePrimitive{};
  int DirectedRelationship{};

  EoGeLine testLine = testLinePrimitive->Line();

  double TestLineAngle = fmod(testLine.AngleFromXAxisXY(), Eo::Pi);

  auto GroupPosition = GetLastGroupPosition();
  while (GroupPosition != nullptr) {
    auto* Group = GetPreviousGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      auto* primitive = Group->GetNext(PrimitivePosition);
      if (primitive == testLinePrimitive || !primitive->Is(EoDb::kLinePrimitive)) { continue; }

      auto* LinePrimitive = static_cast<EoDbLine*>(primitive);
      Line = LinePrimitive->Line();
      if (Line.begin == testLine.begin || Line.begin == testLine.end) {  // Exchange points
        EoGePoint3d Point = Line.begin;
        Line.begin = Line.end;
        Line.end = Point;
      } else if (Line.end != testLine.begin && Line.end != testLine.end) {
        // No endpoint coincides with one of the test line endpoints
        continue;
      }
      double LineAngle = fmod(Line.AngleFromXAxisXY(), Eo::Pi);
      if (fabs(fabs(TestLineAngle - LineAngle) - Eo::HalfPi) <= angularTolerance) {
        if (LeftLinePrimitive == 0) {  // No qualifiers yet
          DirectedRelationship = testLine.DirRelOfPt(Line.begin);
          LeftLinePrimitive = LinePrimitive;
          leftLine = Line;
        } else {
          if (DirectedRelationship == testLine.DirRelOfPt(Line.begin)) {
            // Both lines are on the same side of test line
            RightLinePrimitive = LinePrimitive;
            rightLine = Line;
            if (rightLine.DirRelOfPt(leftLine.begin) != 1) {
              RightLinePrimitive = LeftLinePrimitive;
              rightLine = leftLine;
              LeftLinePrimitive = LinePrimitive;
              leftLine = Line;
            }
            return true;
          }
        }
      }
    }
  }
  return false;
}
