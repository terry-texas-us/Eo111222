#include "stdafx.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

#include "EoDlgSetLength.h"

void AeSysView::OnDraw2ModeOptions() {
  EoDlgSetLength dlg;
  dlg.m_strTitle = L"Set Distance Between Lines";
  dlg.m_dLength = m_DistanceBetweenLines;

  if (dlg.DoModal() == IDOK) { m_DistanceBetweenLines = dlg.m_dLength; }
}

void AeSysView::OnDraw2ModeJoin() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);

  EoDbGroup* Group = SelectGroupAndPrimitive(CurrentPnt);

  if (Group != 0) {
    CurrentPnt = DetPt();
    if (m_PreviousOp == 0) {  // Starting at existing wall
      m_BeginSectionGroup = Group;
      m_BeginSectionLine = static_cast<EoDbLine*>(EngagedPrimitive());
      m_PreviousPnt = CurrentPnt;
      m_PreviousOp = ID_OP1;
    } else {  // Ending at existing wall
      m_EndSectionGroup = Group;
      m_EndSectionLine = static_cast<EoDbLine*>(EngagedPrimitive());
      OnDraw2ModeWall();
      OnDraw2ModeEscape();
    }
    SetCursorPosition(CurrentPnt);
  }
}

void AeSysView::OnDraw2ModeWall() {
  EoGePoint3d ptEnd;
  EoGePoint3d ptBeg;
  EoGePoint3d ptInt;

  EoGePoint3d CurrentPnt = GetCursorPosition();

  if (m_PreviousOp != 0) {
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
  if (m_EndSectionGroup == 0) {
    if (m_PreviousOp != 0) {
      CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);

      m_CurrentReferenceLine(m_PreviousPnt, CurrentPnt);
      m_CurrentReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, m_CurrentLeftLine,
                                          m_CurrentRightLine);

      if (m_ContinueCorner) {
        CleanPreviousLines();
      } else if (m_BeginSectionGroup != 0) {
        StartAssemblyFromLine();
      } else if (m_PreviousOp == ID_OP2) {
        m_AssemblyGroup = new EoDbGroup;
        GetDocument()->AddWorkLayerGroup(m_AssemblyGroup);
        m_AssemblyGroup->AddTail(new EoDbLine(m_CurrentLeftLine.begin, m_CurrentRightLine.begin));
      }
      m_AssemblyGroup->AddTail(new EoDbLine(pstate.PenColor(), pstate.LineType(), m_CurrentLeftLine));
      m_AssemblyGroup->AddTail(new EoDbLine(pstate.PenColor(), pstate.LineType(), m_CurrentRightLine));

      m_AssemblyGroup->AddTail(new EoDbLine(m_CurrentRightLine.end, m_CurrentLeftLine.end));
      GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_AssemblyGroup);
      m_ContinueCorner = true;
      m_PreviousReferenceLine = m_CurrentReferenceLine;
    }
    m_PreviousOp = ID_OP2;
    m_PreviousPnt = CurrentPnt;
    SetCursorPosition(m_PreviousPnt);
  } else {
    m_CurrentReferenceLine(m_PreviousPnt, CurrentPnt);
    m_CurrentReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, m_CurrentLeftLine,
                                        m_CurrentRightLine);

    if (m_ContinueCorner) {
      CleanPreviousLines();
    } else if (m_BeginSectionGroup != 0) {
      StartAssemblyFromLine();
    } else if (m_PreviousOp == ID_OP2) {
      m_AssemblyGroup = new EoDbGroup;
      GetDocument()->AddWorkLayerGroup(m_AssemblyGroup);
      m_AssemblyGroup->AddTail(new EoDbLine(m_CurrentLeftLine.begin, m_CurrentRightLine.begin));
    }
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_EndSectionGroup);
    ptBeg = m_EndSectionLine->BeginPoint();
    ptEnd = m_EndSectionLine->EndPoint();

    m_AssemblyGroup->AddTail(new EoDbLine(pstate.PenColor(), pstate.LineType(), m_CurrentLeftLine));
    m_AssemblyGroup->AddTail(new EoDbLine(pstate.PenColor(), pstate.LineType(), m_CurrentRightLine));
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_AssemblyGroup);

    EoDbLine* LinePrimitive = new EoDbLine(*m_EndSectionLine);
    if (EoGeLine(m_PreviousPnt, CurrentPnt).DirRelOfPt(ptBeg) < 0.0) {
      m_EndSectionLine->EndPoint(m_CurrentRightLine.end);
      LinePrimitive->BeginPoint(m_CurrentLeftLine.end);
    } else {
      m_EndSectionLine->EndPoint(m_CurrentLeftLine.end);
      LinePrimitive->BeginPoint(m_CurrentRightLine.end);
    }
    m_EndSectionGroup->AddTail(LinePrimitive);
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_EndSectionGroup);
    m_EndSectionGroup = 0;

    ModeLineUnhighlightOp(m_PreviousOp);
    m_ContinueCorner = false;
  }
  m_PreviousPnt = CurrentPnt;
}

void AeSysView::OnDraw2ModeReturn() {
  if (m_PreviousOp != 0) OnDraw2ModeEscape();

  m_PreviousPnt = GetCursorPosition();
}

void AeSysView::OnDraw2ModeEscape() {
  GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  ModeLineUnhighlightOp(m_PreviousOp);

  m_ContinueCorner = false;
  m_AssemblyGroup = 0;
  m_BeginSectionGroup = 0;
  m_BeginSectionLine = 0;
  m_EndSectionGroup = 0;
  m_EndSectionLine = 0;
}

bool AeSysView::CleanPreviousLines() {
  bool ParallelLines = m_PreviousReferenceLine.ParallelTo(m_CurrentReferenceLine);
  if (ParallelLines) return false;

  EoGePoint3d ptInt;
  EoGeLine PreviousLeftLine;
  EoGeLine PreviousRightLine;

  m_PreviousReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, PreviousLeftLine,
                                       PreviousRightLine);

  EoGeLine::Intersection_xy(PreviousLeftLine, m_CurrentLeftLine, ptInt);
  PreviousLeftLine.end = m_CurrentLeftLine.begin = ptInt;

  EoGeLine::Intersection_xy(PreviousRightLine, m_CurrentRightLine, ptInt);
  PreviousRightLine.end = m_CurrentRightLine.begin = ptInt;

  GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_AssemblyGroup);

  delete m_AssemblyGroup->RemoveTail();
  POSITION Position = m_AssemblyGroup->GetTailPosition();

  EoDbLine* pLine = static_cast<EoDbLine*>(m_AssemblyGroup->GetPrev(Position));
  pLine->EndPoint(PreviousRightLine.end);
  pLine = static_cast<EoDbLine*>(m_AssemblyGroup->GetPrev(Position));
  pLine->EndPoint(PreviousLeftLine.end);

  return true;
}
bool AeSysView::StartAssemblyFromLine() {
  EoGeLine Line = m_BeginSectionLine->Ln();

  bool ParallelLines = Line.ParallelTo(m_CurrentReferenceLine);
  if (ParallelLines) return false;

  EoGePoint3d ptInt;
  m_AssemblyGroup = m_BeginSectionGroup;

  GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_AssemblyGroup);

  EoGeLine::Intersection_xy(Line, m_CurrentLeftLine, ptInt);
  m_CurrentLeftLine.begin = ptInt;
  EoGeLine::Intersection_xy(Line, m_CurrentRightLine, ptInt);
  m_CurrentRightLine.begin = ptInt;

  EoDbLine* LinePrimitive = new EoDbLine(*m_BeginSectionLine);

  if (EoGeVector3d(Line.begin, m_CurrentLeftLine.begin).Length() >
      EoGeVector3d(Line.begin, m_CurrentRightLine.begin).Length()) {
    m_BeginSectionLine->EndPoint(m_CurrentRightLine.begin);
    LinePrimitive->BeginPoint(m_CurrentLeftLine.begin);
  } else {
    m_BeginSectionLine->EndPoint(m_CurrentLeftLine.begin);
    LinePrimitive->BeginPoint(m_CurrentRightLine.begin);
  }
  m_AssemblyGroup->AddTail(LinePrimitive);
  m_BeginSectionLine = 0;

  return true;
}
void AeSysView::DoDraw2ModeMouseMove() {
  static EoGePoint3d CurrentPnt = EoGePoint3d();

  if (m_PreviousOp == 0) {
    CurrentPnt = GetCursorPosition();
  } else if (m_PreviousOp == ID_OP1 || m_PreviousOp == ID_OP2) {
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();

    CurrentPnt = GetCursorPosition();
    CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);

    EoGeLine PreviewLines[2];
    EoGeLine ln(m_PreviousPnt, CurrentPnt);
    ln.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, PreviewLines[0], PreviewLines[1]);

    m_PreviewGroup.AddTail(new EoDbLine(PreviewLines[0].begin, PreviewLines[1].begin));
    m_PreviewGroup.AddTail(new EoDbLine(pstate.PenColor(), pstate.LineType(), PreviewLines[0]));
    m_PreviewGroup.AddTail(new EoDbLine(pstate.PenColor(), pstate.LineType(), PreviewLines[1]));
    m_PreviewGroup.AddTail(new EoDbLine(PreviewLines[1].end, PreviewLines[0].end));
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  }
}
