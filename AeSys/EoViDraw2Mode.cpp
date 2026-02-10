#include "Stdafx.h"

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbLine.h"
#include "EoDlgSetLength.h"
#include "EoGsRenderState.h"

void AeSysView::OnDraw2ModeOptions() {
  EoDlgSetLength dlg;
  dlg.m_strTitle = L"Set Distance Between Lines";
  dlg.m_dLength = m_DistanceBetweenLines;

  if (dlg.DoModal() == IDOK) { m_DistanceBetweenLines = dlg.m_dLength; }
}

void AeSysView::OnDraw2ModeJoin() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);

  auto* group = SelectGroupAndPrimitive(cursorPosition);

  if (group == nullptr) { return; }
  cursorPosition = DetPt();
  if (m_PreviousOp == 0) {  // Starting at existing wall
    m_BeginSectionGroup = group;
    m_BeginSectionLine = static_cast<EoDbLine*>(EngagedPrimitive());
    m_PreviousPnt = cursorPosition;
    m_PreviousOp = ID_OP1;
  } else {  // Ending at existing wall
    m_EndSectionGroup = group;
    m_EndSectionLine = static_cast<EoDbLine*>(EngagedPrimitive());
    OnDraw2ModeWall();
    OnDraw2ModeEscape();
  }
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnDraw2ModeWall() {
  EoGePoint3d ptEnd;
  EoGePoint3d ptBeg;
  EoGePoint3d ptInt;

  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();
  if (m_PreviousOp != 0) {
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
  if (m_EndSectionGroup == 0) {
    if (m_PreviousOp != 0) {
      cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);

      m_CurrentReferenceLine(m_PreviousPnt, cursorPosition);
      m_CurrentReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, m_CurrentLeftLine,
                                          m_CurrentRightLine);

      if (m_ContinueCorner) {
        CleanPreviousLines();
      } else if (m_BeginSectionGroup != 0) {
        StartAssemblyFromLine();
      } else if (m_PreviousOp == ID_OP2) {
        m_AssemblyGroup = new EoDbGroup;
        document->AddWorkLayerGroup(m_AssemblyGroup);
        m_AssemblyGroup->AddTail(
            new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), m_CurrentLeftLine.begin, m_CurrentRightLine.begin));
      }
      m_AssemblyGroup->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), m_CurrentLeftLine));
      m_AssemblyGroup->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), m_CurrentRightLine));

      m_AssemblyGroup->AddTail(
          new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), m_CurrentRightLine.end, m_CurrentLeftLine.end));
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_AssemblyGroup);
      m_ContinueCorner = true;
      m_PreviousReferenceLine = m_CurrentReferenceLine;
    }
    m_PreviousOp = ID_OP2;
    m_PreviousPnt = cursorPosition;
    SetCursorPosition(m_PreviousPnt);
  } else {
    m_CurrentReferenceLine(m_PreviousPnt, cursorPosition);
    m_CurrentReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, m_CurrentLeftLine,
                                        m_CurrentRightLine);

    if (m_ContinueCorner) {
      CleanPreviousLines();
    } else if (m_BeginSectionGroup != 0) {
      StartAssemblyFromLine();
    } else if (m_PreviousOp == ID_OP2) {
      m_AssemblyGroup = new EoDbGroup;
      document->AddWorkLayerGroup(m_AssemblyGroup);
      m_AssemblyGroup->AddTail(
          new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), m_CurrentLeftLine.begin, m_CurrentRightLine.begin));
    }
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_EndSectionGroup);
    ptBeg = m_EndSectionLine->Begin();
    ptEnd = m_EndSectionLine->End();

    m_AssemblyGroup->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), m_CurrentLeftLine));
    m_AssemblyGroup->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), m_CurrentRightLine));
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_AssemblyGroup);

    EoDbLine* LinePrimitive = new EoDbLine(*m_EndSectionLine);
    if (EoGeLine(m_PreviousPnt, cursorPosition).DirRelOfPt(ptBeg) < 0.0) {
      m_EndSectionLine->SetEndPoint(m_CurrentRightLine.end);
      LinePrimitive->SetBeginPoint(m_CurrentLeftLine.end);
    } else {
      m_EndSectionLine->SetEndPoint(m_CurrentLeftLine.end);
      LinePrimitive->SetBeginPoint(m_CurrentRightLine.end);
    }
    m_EndSectionGroup->AddTail(LinePrimitive);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_EndSectionGroup);
    m_EndSectionGroup = 0;

    ModeLineUnhighlightOp(m_PreviousOp);
    m_ContinueCorner = false;
  }
  m_PreviousPnt = cursorPosition;
}

void AeSysView::OnDraw2ModeReturn() {
  if (m_PreviousOp != 0) OnDraw2ModeEscape();

  m_PreviousPnt = GetCursorPosition();
}

void AeSysView::OnDraw2ModeEscape() {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
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
  auto* document = GetDocument();
  bool ParallelLines = m_PreviousReferenceLine.ParallelTo(m_CurrentReferenceLine);
  if (ParallelLines) { return false; }

  EoGePoint3d ptInt;
  EoGeLine PreviousLeftLine;
  EoGeLine PreviousRightLine;

  m_PreviousReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, PreviousLeftLine,
                                       PreviousRightLine);

  EoGeLine::Intersection_xy(PreviousLeftLine, m_CurrentLeftLine, ptInt);
  PreviousLeftLine.end = m_CurrentLeftLine.begin = ptInt;

  EoGeLine::Intersection_xy(PreviousRightLine, m_CurrentRightLine, ptInt);
  PreviousRightLine.end = m_CurrentRightLine.begin = ptInt;

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_AssemblyGroup);

  delete m_AssemblyGroup->RemoveTail();
  auto position = m_AssemblyGroup->GetTailPosition();

  EoDbLine* pLine = static_cast<EoDbLine*>(m_AssemblyGroup->GetPrev(position));
  pLine->SetEndPoint(PreviousRightLine.end);
  pLine = static_cast<EoDbLine*>(m_AssemblyGroup->GetPrev(position));
  pLine->SetEndPoint(PreviousLeftLine.end);

  return true;
}

bool AeSysView::StartAssemblyFromLine() {
  auto* document = GetDocument();
  auto line = m_BeginSectionLine->Line();

  bool isParallelTo = line.ParallelTo(m_CurrentReferenceLine);
  if (isParallelTo) { return false; }

  EoGePoint3d ptInt;
  m_AssemblyGroup = m_BeginSectionGroup;

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_AssemblyGroup);

  EoGeLine::Intersection_xy(line, m_CurrentLeftLine, ptInt);
  m_CurrentLeftLine.begin = ptInt;
  EoGeLine::Intersection_xy(line, m_CurrentRightLine, ptInt);
  m_CurrentRightLine.begin = ptInt;

  EoDbLine* LinePrimitive = new EoDbLine(*m_BeginSectionLine);

  if (EoGeVector3d(line.begin, m_CurrentLeftLine.begin).Length() >
      EoGeVector3d(line.begin, m_CurrentRightLine.begin).Length()) {
    m_BeginSectionLine->SetEndPoint(m_CurrentRightLine.begin);
    LinePrimitive->SetBeginPoint(m_CurrentLeftLine.begin);
  } else {
    m_BeginSectionLine->SetEndPoint(m_CurrentLeftLine.begin);
    LinePrimitive->SetBeginPoint(m_CurrentRightLine.begin);
  }
  m_AssemblyGroup->AddTail(LinePrimitive);
  m_BeginSectionLine = nullptr;

  return true;
}

void AeSysView::DoDraw2ModeMouseMove() {
  static EoGePoint3d CurrentPnt = EoGePoint3d();

  if (m_PreviousOp == 0) {
    CurrentPnt = GetCursorPosition();
  } else if (m_PreviousOp == ID_OP1 || m_PreviousOp == ID_OP2) {
    auto* document = GetDocument();
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();

    CurrentPnt = GetCursorPosition();
    CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);

    EoGeLine PreviewLines[2]{};
    EoGeLine ln(m_PreviousPnt, CurrentPnt);
    ln.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, PreviewLines[0], PreviewLines[1]);

    m_PreviewGroup.AddTail(
        new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), PreviewLines[0].begin, PreviewLines[1].begin));
    m_PreviewGroup.AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), PreviewLines[0]));
    m_PreviewGroup.AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), PreviewLines[1]));
    m_PreviewGroup.AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), PreviewLines[1].end, PreviewLines[0].end));
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  }
}
