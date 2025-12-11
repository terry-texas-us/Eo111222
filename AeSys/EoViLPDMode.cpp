#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

#include "EoDlgLowPressureDuctOptions.h"

LPWSTR TrimLeadingSpace(LPWSTR szString) {
	LPWSTR p = szString;

	while (p && *p && isspace(*p)) {
		p++;
	}
	return p;
}

/// <remarks>
///Only check for actual end-cap marker is by attributes. No error processing for invalid width or depth values.
///Group data contains whatever primative follows marker (hopefully this is associated end-cap line).
///Issues:
/// xor operations on transition not clean
/// ending section with 3 key will generate a shortened section if the point is less than transition length from the begin point.
/// full el only works with center just
/// </remarks>

void AeSysView::OnLpdModeOptions() {
	SetDuctOptions(m_CurrentSection);
}

void AeSysView::OnLpdModeJoin() {
	EoGePoint3d CurrentPnt = GetCursorPosition();

	m_EndCapGroup = SelectPointUsingPoint(CurrentPnt, .01, 15, 8, m_EndCapPoint);
	if (m_EndCapGroup != 0) {
		m_PreviousPnt = m_EndCapPoint->GetPt();
		m_PreviousSection.SetWidth(m_EndCapPoint->GetDat(0));
		m_PreviousSection.SetDepth(m_EndCapPoint->GetDat(1));
		m_ContinueSection = false;

		m_EndCapLocation = (m_PreviousOp == 0) ? 1 : - 1; // 1 (start) and -1 (end)

		CString Message(L"Cross sectional dimension (Width by Depth) is ");
		CString Length;
		app.FormatLength(Length, max(app.GetUnits(), AeSys::kInches), m_PreviousSection.Width(), 12, 2);
		CString Width;
		app.FormatLength(Width, max(app.GetUnits(), AeSys::kInches), m_PreviousSection.Depth(), 12, 2);
		Message.Append(Length.TrimLeft() + L" by " + Width.TrimLeft());
		app.AddStringToMessageList(Message);
		SetCursorPosition(m_PreviousPnt);
	}
}

void AeSysView::OnLpdModeDuct() {
	EoGePoint3d CurrentPnt = GetCursorPosition();

	if (m_PreviousOp != 0) {
		m_PreviewGroup.DeletePrimitivesAndRemoveAll();
	}
	if (m_PreviousOp == ID_OP2) {
		CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);
		m_CurrentReferenceLine(m_PreviousPnt, CurrentPnt);

		if (m_ContinueSection) {
			EoDbGroup* Group = new EoDbGroup;
			GetDocument()->AddWorkLayerGroup(Group);
			GenerateRectangularElbow(m_PreviousReferenceLine, m_PreviousSection, m_CurrentReferenceLine, m_CurrentSection, Group);
			m_OriginalPreviousGroup->DeletePrimitivesAndRemoveAll();
			GenerateRectangularSection(m_PreviousReferenceLine, m_CenterLineEccentricity, m_PreviousSection, m_OriginalPreviousGroup);
			m_OriginalPreviousGroupDisplayed = true;
			m_PreviousSection = m_CurrentSection;
		}
		double TransitionLength = (m_PreviousSection == m_CurrentSection) ? 0. : LengthOfTransition(m_DuctJustification, m_TransitionSlope, m_PreviousSection, m_CurrentSection);
		EoGeLine ReferenceLine(m_CurrentReferenceLine);

		if (m_BeginWithTransition) {
			if (TransitionLength != 0.0) {
				ReferenceLine.end = ReferenceLine.ProjToEndPt(TransitionLength);

				EoDbGroup* Group = new EoDbGroup;
				GetDocument()->AddWorkLayerGroup(Group);
				GenerateTransition(ReferenceLine, m_CenterLineEccentricity, m_DuctJustification, m_TransitionSlope, m_PreviousSection, m_CurrentSection, Group);
				ReferenceLine.begin = ReferenceLine.end;
				ReferenceLine.end = m_CurrentReferenceLine.end;
				m_ContinueSection = false;
			}
			if (m_CurrentReferenceLine.Length() - TransitionLength > FLT_EPSILON) {
				m_OriginalPreviousGroup = new EoDbGroup;
				GetDocument()->AddWorkLayerGroup(m_OriginalPreviousGroup);
				GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_CurrentSection, m_OriginalPreviousGroup);
				m_ContinueSection = true;
			}
		}
		else {
			if (ReferenceLine.Length() - TransitionLength > FLT_EPSILON) {
				ReferenceLine.end = ReferenceLine.ProjToBegPt(TransitionLength);
				m_OriginalPreviousGroup = new EoDbGroup;
				GetDocument()->AddWorkLayerGroup(m_OriginalPreviousGroup);
				GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_PreviousSection, m_OriginalPreviousGroup);
				ReferenceLine.begin = ReferenceLine.end;
				ReferenceLine.end = m_CurrentReferenceLine.end;
				m_ContinueSection = true;
			}
			if (TransitionLength != 0.0) {
				EoDbGroup* Group = new EoDbGroup;
				GetDocument()->AddWorkLayerGroup(Group);
				GenerateTransition(ReferenceLine, m_CenterLineEccentricity, m_DuctJustification, m_TransitionSlope, m_PreviousSection, m_CurrentSection, Group);
				m_ContinueSection = false;
			}
		}
		m_PreviousReferenceLine = m_CurrentReferenceLine;
		m_PreviousSection = m_CurrentSection;
	}
	m_PreviousOp = ID_OP2;
	m_PreviousPnt = CurrentPnt;
}

void AeSysView::OnLpdModeTransition() {
	m_CurrentSection = m_PreviousSection;
	SetDuctOptions(m_CurrentSection);

	m_BeginWithTransition = (m_PreviousOp == 0) ? true : false;

	DoDuctModeMouseMove();
	OnLpdModeDuct();
}

void AeSysView::OnLpdModeTap() {
	EoGePoint3d CurrentPnt = GetCursorPosition();

	if (m_PreviousOp != 0) {
		GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
		m_PreviewGroup.DeletePrimitivesAndRemoveAll();
	}
	EoDbLine* LinePrimitive;
	EoDbGroup* Group = SelectLineUsingPoint(CurrentPnt, LinePrimitive);
	if (Group != 0) {
		EoGePoint3d TestPoint(CurrentPnt);
		CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);
		CurrentPnt = LinePrimitive->ProjPt(CurrentPnt);
		m_CurrentReferenceLine(m_PreviousPnt, CurrentPnt);

		EJust Justification;
		int Relationship = m_CurrentReferenceLine.DirRelOfPt(TestPoint);
		if (Relationship == 1) {
			Justification = Left;
		}
		else if (Relationship == - 1) {
			Justification = Right;
		}
		else {
			app.AddStringToMessageList(L"Could not determine orientation of component");
			return;
		}
		if (m_PreviousOp == ID_OP2) {
			if (m_ContinueSection) {
				Group = new EoDbGroup;
				GetDocument()->AddWorkLayerGroup(Group);
				GenerateRectangularElbow(m_PreviousReferenceLine, m_PreviousSection, m_CurrentReferenceLine, m_CurrentSection, Group);
				m_PreviousSection = m_CurrentSection;
			}
			double SectionLength = m_CurrentReferenceLine.Length();
			if (SectionLength >= m_DuctTapSize + m_DuctSeamSize) {
				EoGeLine ReferenceLine(m_CurrentReferenceLine);
				ReferenceLine.end = ReferenceLine.ProjToBegPt(m_DuctTapSize + m_DuctSeamSize);
				Group = new EoDbGroup;
				GetDocument()->AddWorkLayerGroup(Group);
				GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_PreviousSection, Group);
				m_CurrentReferenceLine.begin = ReferenceLine.end;
				m_PreviousReferenceLine = m_CurrentReferenceLine;
				m_PreviousSection = m_CurrentSection;
			}
			GenerateRectangularTap(Justification, m_PreviousSection);
			m_PreviousOp = 0;
			m_ContinueSection = false;
			m_PreviousPnt = CurrentPnt;
		}
	}
	else
		app.AddStringToMessageList(IDS_MSG_LINE_NOT_SELECTED);
}

void AeSysView::OnLpdModeEll() {
	EoGePoint3d CurrentPnt = GetCursorPosition();

	if (m_PreviousOp != 0) {
		GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
		m_PreviewGroup.DeletePrimitivesAndRemoveAll();
	}
	if (m_PreviousOp == ID_OP2) {
		EoDbPoint* EndPointPrimitive = 0;
		EoDbGroup* ExistingGroup = SelectPointUsingPoint(CurrentPnt, .01, 15, 8, EndPointPrimitive);
		if (ExistingGroup == 0) {
			app.AddStringToMessageList(IDS_MSG_LPD_NO_END_CAP_LOC);
			return;
		}
		CurrentPnt = EndPointPrimitive->GetPt();
		Section ExistingSection(EndPointPrimitive->GetDat(0), EndPointPrimitive->GetDat(1), Section::Rectangular);

		EoDbPoint* BeginPointPrimitive = ExistingGroup->GetFirstDifferentPoint(EndPointPrimitive);
		if (BeginPointPrimitive != 0) {
			EoGeLine ExistingSectionReferenceLine(BeginPointPrimitive->GetPt(), CurrentPnt);

			EoGePoint3d IntersectionPoint(ExistingSectionReferenceLine.ProjPt(m_PreviousPnt));
			double Relationship;
			ExistingSectionReferenceLine.RelOfPtToEndPts(IntersectionPoint, Relationship);
			if (Relationship > FLT_EPSILON) {
				m_CurrentReferenceLine(m_PreviousPnt, IntersectionPoint);
				double SectionLength = m_CurrentReferenceLine.Length() - (m_PreviousSection.Width() + m_DuctSeamSize + ExistingSection.Width() * .5);
				if (SectionLength > FLT_EPSILON) {
					m_CurrentReferenceLine.end = m_CurrentReferenceLine.ProjToEndPt(SectionLength);
					EoDbGroup* Group = new EoDbGroup;
					GetDocument()->AddWorkLayerGroup(Group);
					GenerateRectangularSection(m_CurrentReferenceLine, m_CenterLineEccentricity, m_PreviousSection, Group);
					GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
				}
				EoDbGroup* Group = new EoDbGroup;
				GetDocument()->AddWorkLayerGroup(Group);
				GenerateFullElbowTakeoff(ExistingGroup, ExistingSectionReferenceLine, ExistingSection, Group);
				GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
			}
		}
		// determine where cursor should be moved to.
	}
	m_ContinueSection = false;
	m_PreviousOp = ID_OP2;
}

void AeSysView::OnLpdModeTee() {
	EoGePoint3d CurrentPnt = GetCursorPosition();

	if (m_PreviousOp != 0) {
		GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
		m_PreviewGroup.DeletePrimitivesAndRemoveAll();
	}
	//m_PreviousPnt = GenerateBullheadTee(this, m_PreviousPnt, CurrentPnt, m_PreviousSection);

	m_ContinueSection = false;
	m_PreviousOp = ID_OP2;
}

void AeSysView::OnLpdModeUpDown() {
	EoGePoint3d CurrentPnt = GetCursorPosition();

	int iRet = 0; // dialog to "Select direction", 'Up.Down.'
	if (iRet >= 0) {
		if (m_PreviousOp == ID_OP2) {
			CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);
			m_CurrentReferenceLine(m_PreviousPnt, CurrentPnt);

			if (m_ContinueSection) {
				EoDbGroup* Group = new EoDbGroup;
				GetDocument()->AddWorkLayerGroup(Group);
				GenerateRectangularElbow(m_PreviousReferenceLine, m_PreviousSection, m_CurrentReferenceLine, m_CurrentSection, Group);
				m_PreviousSection = m_CurrentSection;
			}
			double SectionLength = m_CurrentReferenceLine.Length();
			if (SectionLength > m_PreviousSection.Depth() * .5 + m_DuctSeamSize) {
				EoGeLine ReferenceLine(m_CurrentReferenceLine);
				ReferenceLine.end = ReferenceLine.begin.ProjectToward(ReferenceLine.end, SectionLength - m_PreviousSection.Depth() * .5 - m_DuctSeamSize);
				EoDbGroup* Group = new EoDbGroup;
				GetDocument()->AddWorkLayerGroup(Group);
				GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_PreviousSection, Group);
				GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
				m_CurrentReferenceLine.begin = ReferenceLine.end;
			}
			EoDbGroup* Group = new EoDbGroup;
			GetDocument()->AddWorkLayerGroup(Group);
			GenerateRiseDrop(1, m_PreviousSection, m_CurrentReferenceLine, Group);
			GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
		}
		m_ContinueSection = false;
		m_PreviousOp = ID_OP2;
		m_PreviousPnt = CurrentPnt;
	}
}

void AeSysView::OnLpdModeSize() {
	EoGePoint3d CurrentPnt = GetCursorPosition();

	double dAng = 0.;
	if (m_EndCapPoint != 0) {
		if (m_EndCapPoint->PenColor() == 15 && m_EndCapPoint->PointStyle() == 8) {
			POSITION Position = m_EndCapGroup->Find(m_EndCapPoint);
			m_EndCapGroup->GetNext(Position);
			EoDbLine* pLine = static_cast<EoDbLine*>(m_EndCapGroup->GetAt(Position));
			EoGeLine Line = pLine->Ln();
			dAng = fmod(Line.AngleFromXAxisXY(), PI);
			if (dAng <= RADIAN)
				dAng += PI;
			dAng -= HALF_PI;
		}
		m_EndCapPoint = 0;
	}
	GenSizeNote(CurrentPnt, dAng, m_PreviousSection);
	if (m_PreviousOp != 0)
		RubberBandingDisable();
	m_PreviousOp = 0;
	m_ContinueSection = false;
}

void AeSysView::OnLpdModeReturn() {
	EoGePoint3d CurrentPnt = GetCursorPosition();

	if (m_PreviousOp != 0) {
		OnLpdModeEscape();
	}
	m_PreviousPnt = CurrentPnt;
}

void AeSysView::OnLpdModeEscape() {
	GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
	m_PreviewGroup.DeletePrimitivesAndRemoveAll();

	if (!m_OriginalPreviousGroupDisplayed) {
		GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, m_OriginalPreviousGroup);
		m_OriginalPreviousGroupDisplayed = true;
	}
	ModeLineUnhighlightOp(m_PreviousOp);
	m_ContinueSection = false;
	m_EndCapGroup = 0;
	m_EndCapPoint = 0;
}

void AeSysView::DoDuctModeMouseMove() {
	static EoGePoint3d CurrentPnt = EoGePoint3d();

	if (m_PreviousOp == 0) {
		CurrentPnt = GetCursorPosition();
		m_OriginalPreviousGroupDisplayed = true;
	}
	else if (m_PreviousOp == ID_OP2) {
		GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
		m_PreviewGroup.DeletePrimitivesAndRemoveAll();

		CurrentPnt = GetCursorPosition();
		CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);
		m_CurrentReferenceLine(m_PreviousPnt, CurrentPnt);

		if (m_ContinueSection && m_CurrentReferenceLine.Length() > m_PreviousSection.Width() * m_CenterLineEccentricity + m_DuctSeamSize) {
			EoGeLine PreviousReferenceLine = m_PreviousReferenceLine;
			if (m_OriginalPreviousGroupDisplayed) {
				GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, m_OriginalPreviousGroup);
				m_OriginalPreviousGroupDisplayed = false;
			}
			GenerateRectangularElbow(PreviousReferenceLine, m_PreviousSection, m_CurrentReferenceLine, m_CurrentSection, &m_PreviewGroup);
			GenerateRectangularSection(PreviousReferenceLine, m_CenterLineEccentricity, m_PreviousSection, &m_PreviewGroup);
		}
		EoDbPoint* EndPointPrimitive = 0;
		EoDbGroup* ExistingGroup = SelectPointUsingPoint(CurrentPnt, .01, 15, 8, EndPointPrimitive);
		if (ExistingGroup != 0) {
			CurrentPnt = EndPointPrimitive->GetPt();
			Section ExistingSection(EndPointPrimitive->GetDat(0), EndPointPrimitive->GetDat(1), Section::Rectangular);

			EoDbPoint* BeginPointPrimitive = ExistingGroup->GetFirstDifferentPoint(EndPointPrimitive);
			if (BeginPointPrimitive != 0) {
				EoGeLine ExistingSectionReferenceLine(BeginPointPrimitive->GetPt(), CurrentPnt);

				EoGePoint3d IntersectionPoint(ExistingSectionReferenceLine.ProjPt(m_PreviousPnt));
				double Relationship;
				ExistingSectionReferenceLine.RelOfPtToEndPts(IntersectionPoint, Relationship);
				if (Relationship > FLT_EPSILON) {
					m_CurrentReferenceLine(m_PreviousPnt, IntersectionPoint);
					double SectionLength = m_CurrentReferenceLine.Length() - (m_PreviousSection.Width() + m_DuctSeamSize + ExistingSection.Width() * .5);
					if (SectionLength > FLT_EPSILON) {
						m_CurrentReferenceLine.end = m_CurrentReferenceLine.ProjToEndPt(SectionLength);
						GenerateRectangularSection(m_CurrentReferenceLine, m_CenterLineEccentricity, m_PreviousSection, &m_PreviewGroup);
					}
					GenerateFullElbowTakeoff(ExistingGroup, ExistingSectionReferenceLine, ExistingSection, &m_PreviewGroup);
				}
			}
		}
		else {
			double TransitionLength = (m_PreviousSection == m_CurrentSection) ? 0. : LengthOfTransition(m_DuctJustification, m_TransitionSlope, m_PreviousSection, m_CurrentSection);
			EoGeLine ReferenceLine(m_CurrentReferenceLine);

			if (m_BeginWithTransition) {
				if (TransitionLength != 0.0) {
					ReferenceLine.end = ReferenceLine.ProjToEndPt(TransitionLength);
					GenerateTransition(ReferenceLine, m_CenterLineEccentricity, m_DuctJustification, m_TransitionSlope, m_PreviousSection, m_CurrentSection, &m_PreviewGroup);
					ReferenceLine.begin = ReferenceLine.end;
					ReferenceLine.end = m_CurrentReferenceLine.end;
				}
				if (m_CurrentReferenceLine.Length() - TransitionLength > FLT_EPSILON) {
					GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_CurrentSection, &m_PreviewGroup);
				}
			}
			else {
				if (ReferenceLine.Length() - TransitionLength > FLT_EPSILON) {
					ReferenceLine.end = ReferenceLine.ProjToBegPt(TransitionLength);
					GenerateRectangularSection(ReferenceLine, m_CenterLineEccentricity, m_PreviousSection, &m_PreviewGroup);
					ReferenceLine.begin = ReferenceLine.end;
					ReferenceLine.end = m_CurrentReferenceLine.end;
				}
				if (TransitionLength != 0.0) {
					GenerateTransition(ReferenceLine, m_CenterLineEccentricity, m_DuctJustification, m_TransitionSlope, m_PreviousSection, m_CurrentSection, &m_PreviewGroup);
				}
			}
		}
		m_PreviewGroup.RemoveDuplicatePrimitives();
		GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
	}
}
void AeSysView::GenerateEndCap(EoGePoint3d& beginPoint, EoGePoint3d& endPoint, Section section, EoDbGroup* group) {
	EoGePoint3d Midpoint = EoGePoint3d::Mid(beginPoint, endPoint);

	double Data[] = {section.Width(), section.Depth()};

	EoDbPoint* PointPrimitive = new EoDbPoint(15, 8, Midpoint);
	PointPrimitive->SetDat(2, Data);
	group->AddTail(PointPrimitive);
	group->AddTail(new EoDbLine(beginPoint, endPoint));
}
void AeSysView::GenerateFullElbowTakeoff(EoDbGroup*, EoGeLine& existingSectionReferenceLine, Section existingSection, EoDbGroup* group) {
	EoGeVector3d NewSectionDirection(existingSectionReferenceLine.begin, existingSectionReferenceLine.end);

	EoGePoint3d IntersectionPoint(existingSectionReferenceLine.ProjPt(m_PreviousPnt));
	EoGeLine PreviousReferenceLine(m_PreviousPnt, IntersectionPoint);
	PreviousReferenceLine.end = PreviousReferenceLine.ProjToBegPt((existingSection.Width() + m_PreviousSection.Width()) * .5);
	EoGeLine CurrentReferenceLine(PreviousReferenceLine.end, PreviousReferenceLine.end + NewSectionDirection);

	GenerateRectangularElbow(PreviousReferenceLine, m_PreviousSection, CurrentReferenceLine, m_CurrentSection, group);
	IntersectionPoint = existingSectionReferenceLine.ProjPt(CurrentReferenceLine.begin);
	double Relationship;
	if (existingSectionReferenceLine.RelOfPtToEndPts(IntersectionPoint, Relationship)) {
		if (fabs(Relationship) > FLT_EPSILON && fabs(Relationship - 1.) > FLT_EPSILON) { // need to add a section either from the elbow or the existing section
			double SectionLength = existingSectionReferenceLine.Length();
			double DistanceToBeginPoint = Relationship * SectionLength;
			if (Relationship > FLT_EPSILON && Relationship < 1. - FLT_EPSILON) { // section from the elbow
				CurrentReferenceLine.end = CurrentReferenceLine.begin.ProjectToward(CurrentReferenceLine.end, SectionLength - DistanceToBeginPoint);
				GenerateRectangularSection(CurrentReferenceLine, m_CenterLineEccentricity, m_PreviousSection, group);
			}
			else {
				DistanceToBeginPoint = EoMax(DistanceToBeginPoint, SectionLength);
				existingSectionReferenceLine.end = existingSectionReferenceLine.begin.ProjectToward(existingSectionReferenceLine.end, DistanceToBeginPoint);
			}
		}
		// generate the transition
		EoGePoint3d Points[2];
		Points[0] = existingSectionReferenceLine.end.ProjectToward(CurrentReferenceLine.end, existingSection.Width() * .5 + m_PreviousSection.Width());
		Points[1] = Points[0].ProjectToward(existingSectionReferenceLine.end, existingSection.Width() + m_PreviousSection.Width());

		EoGePoint3d MiddleOfTransition = Points[0] + EoGeVector3d(Points[0], Points[1]) * .5;
		EoGeLine TransitionReferenceLine(MiddleOfTransition, MiddleOfTransition + NewSectionDirection);

		double Width = m_PreviousSection.Width() + existingSection.Width();
		double Depth = m_PreviousSection.Depth() + existingSection.Depth();
		Section ContinueGroup(Width, Depth, Section::Rectangular);
		Section CurrentSection(Width * .75, Depth * .75, Section::Rectangular);

		GenerateTransition(TransitionReferenceLine, m_CenterLineEccentricity, m_DuctJustification, m_TransitionSlope, ContinueGroup, CurrentSection, group);
	}
	if (m_GenerateTurningVanes) {
/*
		EoGePoint3dArray Points;
		Points.SetSize(5);

		Points[2] = rPar[0][1].ProjectToward(rPar[1][1], dEcc2 * m_PreviousSection.Width());
		EoGeLine(Points[2], rPar[1][1]).ProjPtFrom_xy(0.0, m_DuctSeamSize, &Points[3]);
		dDSiz = dDSiz / m_PreviousSection.Width() * m_PreviousSection.Width();
		EoGeLine(Points[2], rPar[1][1]).ProjPtFrom_xy(0.0, dDSiz + m_DuctSeamSize, &Points[4]);
		EoDbGroup* Group = new EoDbGroup;
		GetDocument()->AddWorkLayerGroup(Group);
		Group->AddTail(new EoDbLine(1, pstate.LineType(), lnLead[0], Points[2]));
		Group->AddTail(new EoDbEllipse(1, pstate.LineType(), Points[3], .01));
		Group->AddTail(new EoDbLine(1, pstate.LineType(), Points[3], Points[4]));
		GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
*/
	}
}
void AeSysView::GenerateRiseDrop(EoUInt16 riseDropIndicator, Section section, EoGeLine& referenceLine, EoDbGroup* group) {
	double SectionLength = referenceLine.Length();

	EoGeLine LeftLine;
	EoGeLine RightLine;
	referenceLine.GetParallels(section.Width(), m_CenterLineEccentricity, LeftLine, RightLine);

	if (SectionLength >= section.Depth() * .5 + m_DuctSeamSize) {
		EoGeLine ReferenceLine(referenceLine);
		ReferenceLine.end = ReferenceLine.begin.ProjectToward(ReferenceLine.end, m_DuctSeamSize);
		ReferenceLine.GetParallels(section.Width(), m_CenterLineEccentricity, LeftLine, RightLine);
		group->AddTail(new EoDbLine(LeftLine));
		group->AddTail(new EoDbLine(RightLine));
		referenceLine.begin = ReferenceLine.end;
	}
	referenceLine.GetParallels(section.Width(), m_CenterLineEccentricity, LeftLine, RightLine);
	GenerateRectangularSection(referenceLine, m_CenterLineEccentricity, section, group);
	// need to allow continuation perpendicular to vertical section ?

	group->AddTail(new EoDbLine(pstate.PenColor(), riseDropIndicator, LeftLine.begin, RightLine.end));
	group->AddTail(new EoDbLine(pstate.PenColor(), riseDropIndicator, RightLine.begin, LeftLine.end));
}
void AeSysView::GenerateRectangularElbow(EoGeLine& previousReferenceLine, Section previousSection, EoGeLine& currentReferenceLine, Section currentSection, EoDbGroup* group) {
	if (previousReferenceLine.ParallelTo(currentReferenceLine))
		return;

	previousReferenceLine.end = previousReferenceLine.end.ProjectToward(previousReferenceLine.begin, m_DuctSeamSize + previousSection.Width() * m_CenterLineEccentricity);

	EoGeLine PreviousLeftLine;
	EoGeLine PreviousRightLine;
	previousReferenceLine.GetParallels(previousSection.Width(), m_CenterLineEccentricity, PreviousLeftLine, PreviousRightLine);

	currentReferenceLine.begin = currentReferenceLine.begin.ProjectToward(currentReferenceLine.end, m_DuctSeamSize + previousSection.Width() * m_CenterLineEccentricity);

	EoGeLine CurrentLeftLine;
	EoGeLine CurrentRightLine;
	currentReferenceLine.GetParallels(currentSection.Width(), m_CenterLineEccentricity, CurrentLeftLine, CurrentRightLine);

	EoGePoint3d InsideCorner;
	EoGePoint3d OutsideCorner;
	EoGeLine::Intersection_xy(PreviousLeftLine, CurrentLeftLine, InsideCorner);
	EoGeLine::Intersection_xy(PreviousRightLine, CurrentRightLine, OutsideCorner);

	GenerateEndCap(PreviousLeftLine.end, PreviousRightLine.end, previousSection, group);
	group->AddTail(new EoDbLine(PreviousLeftLine.end, InsideCorner));
	group->AddTail(new EoDbLine(InsideCorner, CurrentLeftLine.begin));
	group->AddTail(new EoDbLine(PreviousRightLine.end, OutsideCorner));
	group->AddTail(new EoDbLine(OutsideCorner, CurrentRightLine.begin));
	if (m_GenerateTurningVanes) {
		group->AddTail(new EoDbLine(2, 2, InsideCorner, OutsideCorner));
	}
	GenerateEndCap(CurrentLeftLine.begin, CurrentRightLine.begin, currentSection, group);
}
void AeSysView::GenerateRectangularSection(EoGeLine& referenceLine, double eccentricity, Section section, EoDbGroup* group) {
	EoGeLine LeftLine;
	EoGeLine RightLine;

	if (referenceLine.GetParallels(section.Width(), eccentricity, LeftLine, RightLine)) {
		GenerateEndCap(LeftLine.begin, RightLine.begin, section, group);

		group->AddTail(new EoDbLine(LeftLine));
		GenerateEndCap(LeftLine.end, RightLine.end, section, group);
		group->AddTail(new EoDbLine(RightLine));
	}
}
void AeSysView::GenSizeNote(EoGePoint3d point, double angle, Section section) {
	EoGeVector3d XDirection = RotateVectorAboutZAxis(EoGeVector3d(0.06, 0.0, 0.0), angle);
	EoGeVector3d YDirection = RotateVectorAboutZAxis(EoGeVector3d(0.0, 0.1, 0.0), angle);
	EoGeReferenceSystem ReferenceSystem(point, XDirection, YDirection);

	CString Width;
	app.FormatLength(Width, max(app.GetUnits(), AeSys::kInches), section.Width(), 8, 0);
	CString Depth;
	app.FormatLength(Depth, max(app.GetUnits(), AeSys::kInches), section.Depth(), 8, 0);
	CString Note = Width.TrimLeft() + L"/" + Depth.TrimLeft();

	CDC* DeviceContext = GetDC();
	int PrimitiveState = pstate.Save();
	pstate.SetPenColor(DeviceContext, 2);

	EoDbFontDefinition fd;
	pstate.GetFontDef(fd);
	fd.HorizontalAlignment(EoDb::kAlignCenter);
	fd.VerticalAlignment(EoDb::kAlignMiddle);

	EoDbCharacterCellDefinition ccd;
	pstate.GetCharCellDef(ccd);
	ccd.TextRotAngSet(0.0);
	pstate.SetCharCellDef(ccd);

	EoDbGroup* Group = new EoDbGroup(new EoDbText(fd, ReferenceSystem, Note));
	GetDocument()->AddWorkLayerGroup(Group);
	GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
	pstate.Restore(DeviceContext, PrimitiveState);
	ReleaseDC(DeviceContext);
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
	GetDocument()->AddWorkLayerGroup(Section);

	GenerateEndCap(LeftLine.begin, RightLine.begin, section, Section);

	Section->AddTail(new EoDbLine(RightLine));
	Section->AddTail(new EoDbLine(RightLine.end, LeftLine.end));
	Section->AddTail(new EoDbLine(LeftLine));

	m_CurrentReferenceLine.begin = ReferenceLine.end;
	m_CurrentReferenceLine.GetParallels(section.Width(), m_CenterLineEccentricity, LeftLine, RightLine);

	if (justification == Right) {
		RightLine.ProjPtFrom_xy(m_DuctTapSize, - m_DuctTapSize, &RightLine.end);
	}
	else {
		LeftLine.ProjPtFrom_xy(m_DuctTapSize, m_DuctTapSize, &LeftLine.end);
	}
	Section->AddTail(new EoDbLine(RightLine.begin, RightLine.end));
	Section->AddTail(new EoDbLine(LeftLine.end, LeftLine.begin));

	if (m_GenerateTurningVanes) {
		EoGePoint3d BeginPoint = ((justification == Left) ? RightLine : LeftLine).ProjToBegPt(- m_DuctTapSize / 3.);
		EoGePoint3d EndPoint = m_CurrentReferenceLine.ProjToBegPt(- m_DuctTapSize / 2.);

		Section->AddTail(new EoDbEllipse(1, pstate.LineType(), BeginPoint, .01));
		Section->AddTail(new EoDbLine(1, pstate.LineType(), BeginPoint, EndPoint));
	}
	GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Section);
	return true;
}
void AeSysView::GenerateTransition(EoGeLine& referenceLine, double eccentricity, EJust justification, double slope, Section previousSection, Section currentSection, EoDbGroup* group) {
	double ReferenceLength = referenceLine.Length();
	if (ReferenceLength <= FLT_EPSILON) return;

	double WidthChange = currentSection.Width() - previousSection.Width();
	double TransitionLength = LengthOfTransition(justification, slope, previousSection, currentSection);
	TransitionLength = EoMin(TransitionLength, ReferenceLength);

	EoGeLine LeftLine;
	EoGeLine RightLine;
	referenceLine.GetParallels(previousSection.Width(), eccentricity, LeftLine, RightLine);

	if (justification == Center) {
		LeftLine.ProjPtFrom_xy(TransitionLength, WidthChange * .5, &LeftLine.end);
		RightLine.ProjPtFrom_xy(TransitionLength, - WidthChange * .5, &RightLine.end);
	}
	else if (justification == Right) {
		RightLine.ProjPtFrom_xy(TransitionLength, - WidthChange, &RightLine.end);
	}
	else {
		LeftLine.ProjPtFrom_xy(TransitionLength, WidthChange, &LeftLine.end);
	}
	GenerateEndCap(LeftLine.begin, RightLine.begin, previousSection, group);
	group->AddTail(new EoDbLine(RightLine.begin, RightLine.end));
	GenerateEndCap(RightLine.end, LeftLine.end, currentSection, group);
	group->AddTail(new EoDbLine(LeftLine.end, LeftLine.begin));
}
void AeSysView::SetDuctOptions(Section& section) {
	AeSys::Units Units = app.GetUnits();
	app.SetUnits(max(Units, AeSys::kInches));

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
	app.SetUnits(Units);
}
double AeSysView::LengthOfTransition(EJust justification, double slope, Section previousSection, Section currentSection) {
	double WidthChange = currentSection.Width() - previousSection.Width();
	double DepthChange = currentSection.Depth() - previousSection.Depth();

	double Length = EoMax(fabs(WidthChange), fabs(DepthChange)) * slope;
	if (justification == Center) {
		Length *= .5;
	}
	return (Length);
}
bool AeSysView::Find2LinesUsingLineEndpoints(EoDbLine* testLinePrimitive, double angularTolerance, EoGeLine& leftLine, EoGeLine& rightLine) {
	EoGeLine Line;

	EoDbLine* LeftLinePrimitive = 0;
	EoDbLine* RightLinePrimitive = 0;
	int DirectedRelationship = 0;

	EoGeLine TestLine;
	testLinePrimitive->GetLine(TestLine);

	double TestLineAngle = fmod(TestLine.AngleFromXAxisXY(), PI);

	POSITION GroupPosition = GetLastGroupPosition();
	while (GroupPosition != 0) {
		EoDbGroup* Group = GetPreviousGroup(GroupPosition);

		POSITION PrimitivePosition = Group->GetHeadPosition();
		while (PrimitivePosition != 0) {
			EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
			if (Primitive == testLinePrimitive || !Primitive->Is(EoDb::kLinePrimitive))
				continue;

			EoDbLine* LinePrimitive = static_cast<EoDbLine*>(Primitive);
			LinePrimitive->GetLine(Line);
			if (Line.begin == TestLine.begin || Line.begin == TestLine.end) { // Exchange points
				EoGePoint3d Point = Line.begin;
				Line.begin = Line.end;
				Line.end = Point;
			}
			else if (Line.end != TestLine.begin && Line.end != TestLine.end) { //	No endpoint coincides with one of the test line endpoints
				continue;
			}
			double LineAngle = fmod(Line.AngleFromXAxisXY(), PI);
			if (fabs(fabs(TestLineAngle - LineAngle) - HALF_PI) <= angularTolerance) {
				if (LeftLinePrimitive == 0) { // No qualifiers yet
					DirectedRelationship = TestLine.DirRelOfPt(Line.begin);
					LeftLinePrimitive = LinePrimitive;
					leftLine = Line;
				}
				else {
					if (DirectedRelationship == TestLine.DirRelOfPt(Line.begin)) { // Both lines are on the same side of test line
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
