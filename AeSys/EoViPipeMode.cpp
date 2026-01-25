#include "Stdafx.h"

#include <cfloat>
#include <cmath>

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbConic.h"
#include "EoDbEllipse.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoDlgPipeOptions.h"
#include "EoDlgPipeSymbol.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "PrimState.h"
#include "Resource.h"

void AeSysView::OnPipeModeOptions() {
  EoDlgPipeOptions Dialog;
  Dialog.m_PipeTicSize = m_PipeTicSize;
  Dialog.m_PipeRiseDropRadius = m_PipeRiseDropRadius;
  if (Dialog.DoModal() == IDOK) {
    m_PipeTicSize = Dialog.m_PipeTicSize;
    m_PipeRiseDropRadius = Dialog.m_PipeRiseDropRadius;
  }
}

void AeSysView::OnPipeModeLine() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  if (pts.IsEmpty()) {
    pts.Add(CurrentPnt);
  } else {
    CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
    auto* document = GetDocument();

    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();

    auto* Group = new EoDbGroup;
    document->AddWorkLayerGroup(Group);
    GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP2, CurrentPnt, Group);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);

    pts[0] = CurrentPnt;
  }
  m_PreviousOp = ModeLineHighlightOp(ID_OP2);
}

void AeSysView::OnPipeModeFitting() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  auto* document = GetDocument();

  EoDbLine* HorizontalSection;
  auto* Group = SelectLineUsingPoint(CurrentPnt, HorizontalSection);
  if (Group != 0) {
    EoGePoint3d BeginPoint = HorizontalSection->BeginPoint();
    EoGePoint3d EndPoint = HorizontalSection->EndPoint();

    if (!pts.IsEmpty()) { CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt); }
    CurrentPnt = HorizontalSection->ProjPt(CurrentPnt);
    HorizontalSection->EndPoint(CurrentPnt);
    Group->AddTail(new EoDbLine(HorizontalSection->Color(), HorizontalSection->LineTypeIndex(), CurrentPnt, EndPoint));

    Group = new EoDbGroup;
    GenerateTicMark(CurrentPnt, BeginPoint, m_PipeRiseDropRadius, Group);
    GenerateTicMark(CurrentPnt, EndPoint, m_PipeRiseDropRadius, Group);
    document->AddWorkLayerGroup(Group);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

    if (pts.IsEmpty()) {
      pts.Add(CurrentPnt);
      m_PreviousOp = ModeLineHighlightOp(ID_OP3);
    } else {
      GenerateTicMark(CurrentPnt, pts[0], m_PipeRiseDropRadius, Group);

      Group = new EoDbGroup;
      GenerateLineWithFittings(m_PreviousOp, pts[0], 0, CurrentPnt, Group);
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);
      OnPipeModeEscape();
    }
  } else {
    EoDbEllipse* VerticalSection;
    Group = SelectCircleUsingPoint(CurrentPnt, m_PipeRiseDropRadius, VerticalSection);
    if (Group != 0) {
      CurrentPnt = VerticalSection->CenterPoint();

      if (pts.IsEmpty()) {
        pts.Add(CurrentPnt);
        m_PreviousOp = ModeLineHighlightOp(ID_OP4);
      } else {
        Group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP5, CurrentPnt, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);
        OnPipeModeEscape();
      }
    } else {
      if (pts.IsEmpty()) {
        pts.Add(CurrentPnt);
      } else {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        Group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, CurrentPnt, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);

        pts[0] = CurrentPnt;
      }
      m_PreviousOp = ModeLineHighlightOp(ID_OP3);
    }
  }
}

void AeSysView::OnPipeModeRise() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  auto* document = GetDocument();

  EoDbLine* HorizontalSection;
  auto* Group = SelectLineUsingPoint(CurrentPnt, HorizontalSection);
  if (Group != 0) {  // On an existing horizontal pipe section
    CurrentPnt = HorizontalSection->ProjPt(CurrentPnt);

    if (pts.IsEmpty()) {  // Rising from an existing horizontal pipe section
      pts.Add(CurrentPnt);
      DropIntoOrRiseFromHorizontalSection(CurrentPnt, Group, HorizontalSection);
    } else {  // Rising into an existing horizontal pipe section
      DropFromOrRiseIntoHorizontalSection(CurrentPnt, Group, HorizontalSection);
      Group = new EoDbGroup;
      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP5, CurrentPnt, Group);
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
    }
    m_PreviousOp = ModeLineHighlightOp(ID_OP5);
  } else {
    EoDbEllipse* VerticalSection;
    Group = SelectCircleUsingPoint(CurrentPnt, m_PipeRiseDropRadius, VerticalSection);
    if (Group != 0) {  // On an existing vertical pipe section
      CurrentPnt = VerticalSection->CenterPoint();
      if (pts.IsEmpty()) {
        pts.Add(CurrentPnt);
        m_PreviousOp = ModeLineHighlightOp(ID_OP4);
      } else {  // Rising into an existing vertical pipe section
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        Group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP5, CurrentPnt, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        OnPipeModeEscape();
      }
    } else {
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();

      if (pts.IsEmpty()) {
        pts.Add(CurrentPnt);
      } else {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        Group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP5, CurrentPnt, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      }
      Group = new EoDbGroup(new EoDbEllipse(CurrentPnt, m_PipeRiseDropRadius, 1, 1));
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      m_PreviousOp = ModeLineHighlightOp(ID_OP5);
      pts[0] = CurrentPnt;
    }
  }
}

void AeSysView::OnPipeModeDrop() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  auto* document = GetDocument();

  EoDbLine* HorizontalSection;
  auto* Group = SelectLineUsingPoint(CurrentPnt, HorizontalSection);
  if (Group != 0) {  // On an existing horizontal pipe section
    CurrentPnt = HorizontalSection->ProjPt(CurrentPnt);

    if (pts.IsEmpty()) {  // Dropping from an existing horizontal pipe section
      pts.Add(CurrentPnt);
      DropFromOrRiseIntoHorizontalSection(CurrentPnt, Group, HorizontalSection);
    } else {  // Dropping into an existing horizontal pipe section
      DropIntoOrRiseFromHorizontalSection(CurrentPnt, Group, HorizontalSection);
      Group = new EoDbGroup;
      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP4, CurrentPnt, Group);
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
    }
    m_PreviousOp = ModeLineHighlightOp(ID_OP4);
  } else {
    EoDbEllipse* VerticalSection;
    Group = SelectCircleUsingPoint(CurrentPnt, m_PipeRiseDropRadius, VerticalSection);
    if (Group != 0) {  // On an existing vertical pipe section
      CurrentPnt = VerticalSection->CenterPoint();
      if (pts.IsEmpty()) {
        pts.Add(CurrentPnt);
        m_PreviousOp = ModeLineHighlightOp(ID_OP5);
      } else {  // Dropping into an existing vertical pipe section
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        Group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP4, CurrentPnt, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        OnPipeModeEscape();
      }
    } else {
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();

      if (pts.IsEmpty()) {
        pts.Add(CurrentPnt);
      } else {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);

        Group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP4, CurrentPnt, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      }
      Group = new EoDbGroup(new EoDbEllipse(CurrentPnt, m_PipeRiseDropRadius, 1, 1));
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

      m_PreviousOp = ModeLineHighlightOp(ID_OP4);
      pts[0] = CurrentPnt;
    }
  }
}

void AeSysView::OnPipeModeSymbol() {
  double SymbolSize[] = {0.09375, 0.09375, 0.09375, 0.09375, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.0, 0.0, 0.09375};
  double TicDistance[] = {0.125,   0.125,   0.125,   0.125,   0.15625, 0.15625, 0.15625, 0.15625, 0.15625,
                          0.15625, 0.15625, 0.15625, 0.15625, 0.15625, 0.15625, 0.03125, 0.03125, 0.125};

  EoGePoint3d CurrentPnt = GetCursorPosition();
  auto* document = GetDocument();

  OnPipeModeEscape();
  pts.SetSize(2);

  EoDbLine* HorizontalSection;
  auto* Group = SelectLineUsingPoint(CurrentPnt, HorizontalSection);
  if (Group == 0) return;

  EoDlgPipeSymbol Dialog;
  Dialog.m_CurrentPipeSymbolIndex = m_CurrentPipeSymbolIndex;
  if (Dialog.DoModal() == IDOK) { m_CurrentPipeSymbolIndex = Dialog.m_CurrentPipeSymbolIndex; }
  EoGePoint3d BeginPoint = HorizontalSection->BeginPoint();
  EoGePoint3d EndPoint = HorizontalSection->EndPoint();
  EoGePoint3d PointOnSection = HorizontalSection->ProjPt(CurrentPnt);

  EoGeLine BeginSection(PointOnSection, BeginPoint);
  EoGeLine EndSection(PointOnSection, EndPoint);

  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, HorizontalSection);

  EoGePoint3d SymbolBeginPoint = PointOnSection.ProjectToward(BeginPoint, SymbolSize[m_CurrentPipeSymbolIndex]);
  EoGePoint3d SymbolEndPoint = PointOnSection.ProjectToward(EndPoint, SymbolSize[m_CurrentPipeSymbolIndex]);
  double TicSize = m_PipeTicSize;

  HorizontalSection->EndPoint(SymbolBeginPoint);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, HorizontalSection);
  Group = new EoDbGroup(new EoDbLine(HorizontalSection->Color(), HorizontalSection->LineTypeIndex(), SymbolEndPoint, EndPoint));
  document->AddWorkLayerGroup(Group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

  Group = new EoDbGroup;
  GenerateTicMark(PointOnSection, BeginPoint, TicDistance[m_CurrentPipeSymbolIndex], Group);
  GenerateTicMark(PointOnSection, EndPoint, TicDistance[m_CurrentPipeSymbolIndex], Group);

  switch (m_CurrentPipeSymbolIndex) {
    case 0:  // Generate flow switch
      Group->AddTail(new EoDbEllipse(PointOnSection, SymbolBeginPoint));
      EndSection.ProjPtFrom_xy(SymbolSize[0], -SymbolSize[0] * 1.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[0], -SymbolSize[0] * 2.0, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[0], SymbolSize[0] * 1.5, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[0], SymbolSize[0] * 2.0, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[1], SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, pts[0]));
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      break;

    case 1:  // Generate float and thermostatic trap
      Group->AddTail(new EoDbEllipse(PointOnSection, SymbolBeginPoint));
      pts[0] = SymbolBeginPoint.RotateAboutAxis(PointOnSection, EoGeVector3d::positiveUnitZ, Eo::QuarterPi);
      pts[1] = pts[0].RotateAboutAxis(PointOnSection, EoGeVector3d::positiveUnitZ, Eo::Pi);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      pts[0] = SymbolBeginPoint.RotateAboutAxis(PointOnSection, EoGeVector3d::positiveUnitZ, Eo::ThreeQuartersPi);
      pts[1] = pts[0].RotateAboutAxis(PointOnSection, EoGeVector3d::positiveUnitZ, Eo::Pi);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      break;

    case 2:
      Group->AddTail(new EoDbEllipse(PointOnSection, SymbolBeginPoint));
      EndSection.ProjPtFrom_xy(SymbolSize[2], SymbolSize[2] * 1.5, &pts[0]);
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[2] * 1.5, &pts[1]);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[2], &SymbolBeginPoint);
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      break;

    case 3:
      Group->AddTail(new EoDbEllipse(PointOnSection, SymbolBeginPoint));
      EndSection.ProjPtFrom_xy(SymbolSize[3], SymbolSize[3] * 1.5, &pts[0]);
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[3] * 1.5, &pts[1]);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      BeginSection.ProjPtFrom_xy(0.0, SymbolSize[3], &SymbolBeginPoint);
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      break;

    case 4:
      EndSection.ProjPtFrom_xy(SymbolSize[4], SymbolSize[4] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[4], -SymbolSize[4] * 0.5, &pts[1]);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      BeginSection.ProjPtFrom_xy(SymbolSize[4], -SymbolSize[4] * 0.5, &SymbolBeginPoint);
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      BeginSection.ProjPtFrom_xy(SymbolSize[4], SymbolSize[4] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      BeginSection.ProjPtFrom_xy(SymbolSize[4], -SymbolSize[4] * 0.3, &pts[0]);
      Group->AddTail(new EoDbEllipse(SymbolBeginPoint, pts[0]));
      break;

    case 5:
      EndSection.ProjPtFrom_xy(SymbolSize[5], SymbolSize[5] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[5], -SymbolSize[5] * 0.5, &pts[1]);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      BeginSection.ProjPtFrom_xy(SymbolSize[5], -SymbolSize[5] * 0.5, &SymbolBeginPoint);
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      BeginSection.ProjPtFrom_xy(SymbolSize[5], SymbolSize[5] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      BeginSection.ProjPtFrom_xy(SymbolSize[5], -SymbolSize[5] * 0.3, &pts[0]);
      Group->AddTail(new EoDbEllipse(SymbolBeginPoint, pts[0]));
      Group->AddTail(new EoDbLine(SymbolEndPoint, PointOnSection));
      break;

    case 6:  // Generate gate valve
      EndSection.ProjPtFrom_xy(SymbolSize[6], SymbolSize[6] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[6], -SymbolSize[6] * 0.5, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[6], -SymbolSize[6] * 0.5, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[6], SymbolSize[6] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[0]));
      break;

    case 7:  // Generate globe valve
      EndSection.ProjPtFrom_xy(SymbolSize[7], SymbolSize[7] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[7], -SymbolSize[7] * 0.5, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[7], -SymbolSize[7] * 0.5, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[7], SymbolSize[7] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[0]));
      pts[0] = PointOnSection.ProjectToward(EndPoint, SymbolSize[7] * 0.25);
      Group->AddTail(new EoDbEllipse(PointOnSection, pts[0]));
      break;

    case 8:  // Generate stop check valve
      EndSection.ProjPtFrom_xy(SymbolSize[8], SymbolSize[8] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[8], -SymbolSize[8] * 0.5, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[8], -SymbolSize[8] * 0.5, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[8], SymbolSize[8] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[0]));
      pts[0] = PointOnSection.ProjectToward(EndPoint, SymbolSize[8] * 0.25);
      Group->AddTail(new EoDbEllipse(PointOnSection, pts[0]));
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[8], &pts[0]);
      Group->AddTail(new EoDbLine(PointOnSection, pts[0]));
      m_PipeTicSize = SymbolSize[8] * 0.25;
      GenerateTicMark(PointOnSection, pts[0], SymbolSize[8] * 0.75, Group);
      break;

    case 9:  // Generate pressure reducing valve
      EndSection.ProjPtFrom_xy(SymbolSize[9], SymbolSize[9] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[9], -SymbolSize[9] * 0.5, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[9], -SymbolSize[9] * 0.5, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[9], SymbolSize[9] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[0]));
      pts[0] = PointOnSection.ProjectToward(EndPoint, SymbolSize[9] * 0.25);
      Group->AddTail(new EoDbEllipse(PointOnSection, pts[0]));
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[9], &pts[0]);
      Group->AddTail(new EoDbLine(PointOnSection, pts[0]));
      EndSection.ProjPtFrom_xy(SymbolSize[9] * 0.5, SymbolSize[9] * 0.75, &pts[1]);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[9] * 0.5, &SymbolBeginPoint);
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      break;

    case 10:
      EndSection.ProjPtFrom_xy(SymbolSize[10], SymbolSize[10] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[10], -SymbolSize[10] * 0.5, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[10], -SymbolSize[10] * 0.5, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[10], SymbolSize[10] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[0]));
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[10] * 0.5, &pts[0]);
      Group->AddTail(new EoDbLine(PointOnSection, pts[0]));
      EndSection.ProjPtFrom_xy(SymbolSize[10] * 0.25, SymbolSize[10] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[10] * 0.25, SymbolSize[10] * 0.75, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[10] * 0.25, -SymbolSize[10] * 0.75, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[10] * 0.25, -SymbolSize[10] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[0]));
      break;

    case 11:  // Generate automatic 3-way valve
      EndSection.ProjPtFrom_xy(SymbolSize[11], SymbolSize[11] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[11], -SymbolSize[11] * 0.5, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[11], -SymbolSize[11] * 0.5, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[11], SymbolSize[11] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[0]));
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[11] * 0.5, &pts[0]);
      Group->AddTail(new EoDbLine(PointOnSection, pts[0]));
      EndSection.ProjPtFrom_xy(SymbolSize[11] * 0.25, SymbolSize[11] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[11] * 0.25, SymbolSize[11] * 0.75, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[11] * 0.25, -SymbolSize[11] * 0.75, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[11] * 0.25, -SymbolSize[11] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[0]));
      EndSection.ProjPtFrom_xy(SymbolSize[11] * 0.5, -SymbolSize[11], &pts[0]);
      BeginSection.ProjPtFrom_xy(SymbolSize[11] * 0.5, SymbolSize[11], &pts[1]);
      Group->AddTail(new EoDbLine(PointOnSection, pts[0]));
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], PointOnSection));
      break;

    case 12:  // Generate self operated valve
      EndSection.ProjPtFrom_xy(SymbolSize[12], SymbolSize[12] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(SymbolSize[12], -SymbolSize[12] * 0.5, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[12], -SymbolSize[12] * 0.5, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[12], SymbolSize[12] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[0]));
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[12] * 0.5, &pts[0]);
      Group->AddTail(new EoDbLine(PointOnSection, pts[0]));
      EndSection.ProjPtFrom_xy(SymbolSize[12] * 0.25, SymbolSize[12] * 0.5, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[12] * 0.25, -SymbolSize[12] * 0.5, &SymbolBeginPoint);
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      // add a half circle here i think
      BeginSection.ProjPtFrom_xy(SymbolSize[12] * 1.25, -SymbolSize[12] * 0.5, &pts[0]);
      Group->AddTail(new EoDbLine(pstate.PenColor(), 2, SymbolBeginPoint, pts[0]));
      BeginSection.ProjPtFrom_xy(SymbolSize[12] * 1.25, -SymbolSize[12] * 0.75, &pts[1]);
      BeginSection.ProjPtFrom_xy(SymbolSize[12] * 2.0, -SymbolSize[12] * 0.75, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(SymbolSize[12] * 2.0, -SymbolSize[12] * 0.5, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[0]));
      break;

    case 13:
      EndSection.ProjPtFrom_xy(0.0, -SymbolSize[13], &pts[0]);
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[13], &pts[1]);
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, pts[0]));
      Group->AddTail(new EoDbLine(pts[0], SymbolEndPoint));
      break;

    case 14:
      EndSection.ProjPtFrom_xy(0.0, -SymbolSize[14], &pts[0]);
      EndSection.ProjPtFrom_xy(0.0, SymbolSize[14], &pts[1]);
      Group->AddTail(new EoDbLine(SymbolEndPoint, pts[1]));
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, pts[0]));
      Group->AddTail(new EoDbLine(pts[0], SymbolEndPoint));
      Group->AddTail(new EoDbLine(pts[0], pts[1]));
      break;

    case 15:
      EndSection.ProjPtFrom_xy(0.0, -0.250, &pts[0]);
      Group->AddTail(new EoDbLine(PointOnSection, pts[0]));
      GenerateTicMark(PointOnSection, pts[0], TicDistance[15], Group);
      BeginSection.ProjPtFrom_xy(0.0625, 0.1875, &pts[1]);
      EndSection.ProjPtFrom_xy(0.0625, -0.1875, &SymbolBeginPoint);
      EndSection.ProjPtFrom_xy(0.0625, -0.125, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      break;

    case 16:
      EndSection.ProjPtFrom_xy(0.0, -0.250, &pts[0]);
      Group->AddTail(new EoDbLine(PointOnSection, pts[0]));
      GenerateTicMark(PointOnSection, pts[0], TicDistance[16], Group);
      BeginSection.ProjPtFrom_xy(0.0625, 0.1875, &pts[1]);
      EndSection.ProjPtFrom_xy(0.0625, -0.1875, &SymbolBeginPoint);
      EndSection.ProjPtFrom_xy(0.0625, -0.125, &SymbolEndPoint);
      Group->AddTail(new EoDbLine(pts[1], SymbolBeginPoint));
      Group->AddTail(new EoDbLine(SymbolBeginPoint, SymbolEndPoint));
      pts[1] = PointOnSection.ProjectToward(pts[0], 0.28125);
      Group->AddTail(new EoDbEllipse(pts[1], pts[0]));
      break;

    case 17:  // Generate union
      m_PipeTicSize = SymbolSize[17];
      GenerateTicMark(PointOnSection, BeginPoint, SymbolSize[17], Group);
      GenerateTicMark(PointOnSection, EndPoint, SymbolSize[17], Group);
      m_PipeTicSize = m_PipeTicSize * 2.;
      GenerateTicMark(PointOnSection, BeginPoint, 0.0, Group);
      break;
  }
  m_PipeTicSize = TicSize;
  document->AddWorkLayerGroup(Group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
}

void AeSysView::OnPipeModeWye() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  auto* document = GetDocument();

  if (pts.IsEmpty()) {
    pts.Add(CurrentPnt);
    m_PreviousOp = ModeLineHighlightOp(ID_OP9);
    return;
  }
  EoDbLine* HorizontalSection;
  auto* Group = SelectLineUsingPoint(CurrentPnt, HorizontalSection);
  if (Group != 0) {
    EoGePoint3d PointOnSection = HorizontalSection->ProjPt(CurrentPnt);
    EoGePoint3d BeginPointProjectedToSection = HorizontalSection->ProjPt(pts[0]);
    double DistanceToSection = EoGeVector3d(pts[0], BeginPointProjectedToSection).Length();

    if (DistanceToSection >= 0.25) {
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      EoGePoint3d BeginPoint = HorizontalSection->BeginPoint();
      EoGePoint3d EndPoint = HorizontalSection->EndPoint();

      double DistanceBetweenSectionPoints = EoGeVector3d(BeginPointProjectedToSection, PointOnSection).Length();

      if (fabs(DistanceBetweenSectionPoints - DistanceToSection) <= 0.25) {  // Just need to shift point on section and do a single 45 degree line
        PointOnSection = BeginPointProjectedToSection.ProjectToward(PointOnSection, DistanceToSection);
        HorizontalSection->EndPoint(PointOnSection);
        Group = new EoDbGroup(new EoDbLine(HorizontalSection->Color(), HorizontalSection->LineTypeIndex(), PointOnSection, EndPoint));
        document->AddWorkLayerGroup(Group);

        Group = new EoDbGroup;
        GenerateTicMark(PointOnSection, BeginPoint, m_PipeRiseDropRadius, Group);
        GenerateTicMark(PointOnSection, EndPoint, m_PipeRiseDropRadius, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

        Group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, PointOnSection, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      } else {
        EoGePoint3d PointAtBend;

        if (DistanceBetweenSectionPoints - 0.25 <= DistanceToSection) {
          double d3 = (DistanceBetweenSectionPoints > 0.25) ? DistanceBetweenSectionPoints : 0.125;
          PointAtBend = BeginPointProjectedToSection.ProjectToward(pts[0], d3);
          PointOnSection = BeginPointProjectedToSection.ProjectToward(PointOnSection, d3);
        } else {
          PointAtBend = BeginPointProjectedToSection.ProjectToward(PointOnSection, DistanceBetweenSectionPoints - DistanceToSection);
          PointAtBend = pts[0] + EoGeVector3d(BeginPointProjectedToSection, PointAtBend);
        }
        HorizontalSection->EndPoint(PointOnSection);

        Group = new EoDbGroup;
        GenerateTicMark(PointOnSection, BeginPoint, m_PipeRiseDropRadius, Group);
        GenerateTicMark(PointOnSection, EndPoint, m_PipeRiseDropRadius, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

        Group = new EoDbGroup(new EoDbLine(HorizontalSection->Color(), HorizontalSection->LineTypeIndex(), PointOnSection, EndPoint));
        document->AddWorkLayerGroup(Group);
        Group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, PointAtBend, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        Group = new EoDbGroup;
        GenerateLineWithFittings(ID_OP3, PointAtBend, ID_OP3, PointOnSection, Group);
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      }
    }
    OnPipeModeEscape();
  }
}

void AeSysView::OnPipeModeReturn() { OnPipeModeEscape(); }

void AeSysView::OnPipeModeEscape() {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  pts.RemoveAll();

  ModeLineUnhighlightOp(m_PreviousOp);
  m_PreviousOp = 0;
}

void AeSysView::DoPipeModeMouseMove() {
  auto* document = GetDocument();
  EoGePoint3d CurrentPnt = GetCursorPosition();
  INT_PTR NumberOfPoints = pts.GetSize();

  switch (m_PreviousOp) {
    case ID_OP2:
      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        pts.Add(CurrentPnt);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP2, CurrentPnt, &m_PreviewGroup);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP3:
      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        pts.Add(CurrentPnt);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, CurrentPnt, &m_PreviewGroup);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP4:
    case ID_OP5:
    case ID_OP9: {
      CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
      pts.Add(CurrentPnt);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();

      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, CurrentPnt, &m_PreviewGroup);
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;
    }
  }
  pts.SetSize(NumberOfPoints);
}
void AeSysView::GenerateLineWithFittings(int beginType, EoGePoint3d& beginPoint, int endType, EoGePoint3d& endPoint, EoDbGroup* group) {
  EoGePoint3d pt1 = beginPoint;
  EoGePoint3d pt2 = endPoint;

  if (beginType == ID_OP3)
    // Previous fitting is an elbow or side tee
    GenerateTicMark(beginPoint, endPoint, m_PipeRiseDropRadius, group);
  else if (beginType == ID_OP4) {  // Previous fitting is an elbow down, riser down or bottom tee
    pt1 = beginPoint.ProjectToward(endPoint, m_PipeRiseDropRadius);
    GenerateTicMark(pt1, endPoint, m_PipeRiseDropRadius, group);
  } else if (beginType == ID_OP5)
    // Previous fitting is an elbow up, riser up or top tee
    GenerateTicMark(beginPoint, endPoint, 2. * m_PipeRiseDropRadius, group);

  if (endType == ID_OP3)
    // Current fitting is an elbow or side tee
    GenerateTicMark(endPoint, beginPoint, m_PipeRiseDropRadius, group);
  else if (endType == ID_OP4)
    // Current fitting is an elbow down, riser down or bottom tee
    GenerateTicMark(endPoint, beginPoint, 2. * m_PipeRiseDropRadius, group);
  else if (endType == ID_OP5) {  // Current fitting is an elbow up, riser up or top tee
    pt2 = endPoint.ProjectToward(beginPoint, m_PipeRiseDropRadius);
    GenerateTicMark(endPoint, beginPoint, 2. * m_PipeRiseDropRadius, group);
  }
  group->AddTail(new EoDbLine(pt1, pt2));
}
void AeSysView::DropIntoOrRiseFromHorizontalSection(EoGePoint3d& point, EoDbGroup* group, EoDbLine* section) {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, section);

  EoGePoint3d BeginPoint = section->BeginPoint();
  EoGePoint3d EndPoint = section->EndPoint();

  EoGePoint3d CutPoint = point.ProjectToward(BeginPoint, m_PipeRiseDropRadius);
  section->EndPoint(CutPoint);
  CutPoint = point.ProjectToward(EndPoint, m_PipeRiseDropRadius);
  group->AddTail(new EoDbLine(section->Color(), section->LineTypeIndex(), CutPoint, EndPoint));
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

  group = new EoDbGroup;
  GenerateTicMark(point, BeginPoint, 2. * m_PipeRiseDropRadius, group);
  group->AddTail(new EoDbEllipse(point, m_PipeRiseDropRadius, 1, 1));
  GenerateTicMark(point, EndPoint, 2. * m_PipeRiseDropRadius, group);
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}
void AeSysView::DropFromOrRiseIntoHorizontalSection(EoGePoint3d& point, EoDbGroup* group, EoDbLine* section) {
  auto* document = GetDocument();
  EoGePoint3d BeginPoint = section->BeginPoint();
  EoGePoint3d EndPoint = section->EndPoint();

  section->EndPoint(point);
  group->AddTail(new EoDbLine(section->Color(), section->LineTypeIndex(), point, EndPoint));

  group = new EoDbGroup;
  GenerateTicMark(point, BeginPoint, 2. * m_PipeRiseDropRadius, group);
  group->AddTail(new EoDbEllipse(point, m_PipeRiseDropRadius, 1, 1));
  GenerateTicMark(point, EndPoint, 2. * m_PipeRiseDropRadius, group);
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}
bool AeSysView::GenerateTicMark(EoGePoint3d& beginPoint, EoGePoint3d& endPoint, double distance, EoDbGroup* group) const {
  EoGePoint3d PointOnLine = beginPoint.ProjectToward(endPoint, distance);

  EoGeVector3d Projection(PointOnLine, endPoint);

  double DistanceToEndPoint = Projection.Length();

  bool MarkGenerated = DistanceToEndPoint > DBL_EPSILON;
  if (MarkGenerated) {
    Projection *= m_PipeTicSize / DistanceToEndPoint;

    EoGePoint3d pt1(PointOnLine);
    pt1 += EoGeVector3d(Projection.y, -Projection.x, 0.0);

    EoGePoint3d pt2(PointOnLine);
    pt2 += EoGeVector3d(-Projection.y, Projection.x, 0.0);
    group->AddTail(new EoDbLine(1, 1, pt1, pt2));
  }
  return MarkGenerated;
}
