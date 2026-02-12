#include "Stdafx.h"

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDlgSetLength.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
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

  auto* engagedPrimitive = EngagedPrimitive();
  if (engagedPrimitive == nullptr || !engagedPrimitive->Is(EoDb::kLinePrimitive)) { return; }

  cursorPosition = DetPt();

  if (m_PreviousOp == 0) {  // Starting at existing wall
    m_BeginSectionGroup = group;
    m_BeginSectionLine = static_cast<EoDbLine*>(engagedPrimitive);
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
  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();
  if (document == nullptr) { return; }

  if (m_PreviousOp != 0) {
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
  if (m_EndSectionGroup == nullptr) {
    if (m_PreviousOp != 0) {
      cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);

      m_CurrentReferenceLine(m_PreviousPnt, cursorPosition);
      m_CurrentReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, m_CurrentLeft, m_CurrentRight);

      if (m_ContinueCorner) {
        CleanPreviousLines();
      } else if (m_BeginSectionGroup != nullptr) {
        StartAssemblyFromLine();
      } else if (m_PreviousOp == ID_OP2) {
        m_AssemblyGroup = new EoDbGroup;
        document->AddWorkLayerGroup(m_AssemblyGroup);

        auto* beginCapLine = EoDbLine::CreateLine(m_CurrentLeft.begin, m_CurrentRight.begin)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
        m_AssemblyGroup->AddTail(beginCapLine);
      }
      auto* leftLine = EoDbLine::CreateLine(m_CurrentLeft)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
      auto* rightLine = EoDbLine::CreateLine(m_CurrentRight)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
      auto* endCapLine = EoDbLine::CreateLine(m_CurrentRight.end, m_CurrentLeft.end)->WithProperties(renderState.Color(), renderState.LineTypeIndex());

      m_AssemblyGroup->AddTail(leftLine);
      m_AssemblyGroup->AddTail(rightLine);
      m_AssemblyGroup->AddTail(endCapLine);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_AssemblyGroup);
      m_ContinueCorner = true;
      m_PreviousReferenceLine = m_CurrentReferenceLine;
    }
    m_PreviousOp = ID_OP2;
    m_PreviousPnt = cursorPosition;
    SetCursorPosition(m_PreviousPnt);
  } else {
    m_CurrentReferenceLine(m_PreviousPnt, cursorPosition);
    m_CurrentReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, m_CurrentLeft, m_CurrentRight);

    if (m_ContinueCorner) {
      CleanPreviousLines();
    } else if (m_BeginSectionGroup != nullptr) {
      StartAssemblyFromLine();
    } else if (m_PreviousOp == ID_OP2) {
      m_AssemblyGroup = new EoDbGroup;
      document->AddWorkLayerGroup(m_AssemblyGroup);
      EoGeLine beginCap{m_CurrentLeft.begin, m_CurrentRight.begin};
      auto* beginCapLine = EoDbLine::CreateLine(beginCap)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
      m_AssemblyGroup->AddTail(beginCapLine);
    }
    auto* leftLine = EoDbLine::CreateLine(m_CurrentLeft)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
    auto* rightLine = EoDbLine::CreateLine(m_CurrentRight)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
    m_AssemblyGroup->AddTail(leftLine);
    m_AssemblyGroup->AddTail(rightLine);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_AssemblyGroup);

    if (m_EndSectionLine == nullptr) { return; }

    EoGePoint3d ptBeg = m_EndSectionLine->Begin();
    EoDbLine* LinePrimitive = new EoDbLine(*m_EndSectionLine);
    if (EoGeLine(m_PreviousPnt, cursorPosition).DirRelOfPt(ptBeg) < 0.0) {
      m_EndSectionLine->SetEndPoint(m_CurrentRight.end);
      LinePrimitive->SetBeginPoint(m_CurrentLeft.end);
    } else {
      m_EndSectionLine->SetEndPoint(m_CurrentLeft.end);
      LinePrimitive->SetBeginPoint(m_CurrentRight.end);
    }
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_EndSectionGroup);

    m_EndSectionGroup->AddTail(LinePrimitive);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_EndSectionGroup);
    m_EndSectionGroup = nullptr;

    ModeLineUnhighlightOp(m_PreviousOp);
    m_ContinueCorner = false;
  }
  m_PreviousPnt = cursorPosition;
}

void AeSysView::OnDraw2ModeReturn() {
  if (m_PreviousOp != 0) { OnDraw2ModeEscape(); }

  m_PreviousPnt = GetCursorPosition();
}

void AeSysView::OnDraw2ModeEscape() {
  auto* document = GetDocument();
  if (document == nullptr) { return; }

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  ModeLineUnhighlightOp(m_PreviousOp);

  m_ContinueCorner = false;
  m_AssemblyGroup = nullptr;
  m_BeginSectionGroup = nullptr;
  m_BeginSectionLine = nullptr;
  m_EndSectionGroup = nullptr;
  m_EndSectionLine = nullptr;
}

bool AeSysView::CleanPreviousLines() {
  auto* document = GetDocument();
  if (document == nullptr) { return false; }

  if (m_PreviousReferenceLine.ParallelTo(m_CurrentReferenceLine)) { return false; }

  EoGeLine previousLeftLine{};
  EoGeLine previousRightLine{};

  m_PreviousReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, previousLeftLine, previousRightLine);

  EoGePoint3d intersection{};
  if (!EoGeLine::Intersection_xy(previousLeftLine, m_CurrentLeft, intersection)) { return false; }
  previousLeftLine.end = m_CurrentLeft.begin = intersection;

  if (!EoGeLine::Intersection_xy(previousRightLine, m_CurrentRight, intersection)) { return false; }
  previousRightLine.end = m_CurrentRight.begin = intersection;

  if (m_AssemblyGroup == nullptr) { return false; }

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_AssemblyGroup);

  auto tailPrimitive = m_AssemblyGroup->RemoveTail();
  delete tailPrimitive;

  auto position = m_AssemblyGroup->GetTailPosition();
  if (position == nullptr) { return false; }

  auto* linePrimitive = static_cast<EoDbLine*>(m_AssemblyGroup->GetPrev(position));
  if (linePrimitive == nullptr) { return false; }

  linePrimitive->SetEndPoint(previousRightLine.end);
  linePrimitive = static_cast<EoDbLine*>(m_AssemblyGroup->GetPrev(position));
  if (linePrimitive == nullptr) { return false; }

  linePrimitive->SetEndPoint(previousLeftLine.end);

  return true;
}

bool AeSysView::StartAssemblyFromLine() {
  auto* document = GetDocument();
  if (document == nullptr || m_BeginSectionLine == nullptr) { return false; }

  EoGeLine line = m_BeginSectionLine->Line();

  bool isParallelTo = line.ParallelTo(m_CurrentReferenceLine);
  if (isParallelTo) { return false; }

  EoGePoint3d ptInt;
  m_AssemblyGroup = m_BeginSectionGroup;

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_AssemblyGroup);

  if (!EoGeLine::Intersection_xy(line, m_CurrentLeft, ptInt)) { return false; }
  m_CurrentLeft.begin = ptInt;

  if (!EoGeLine::Intersection_xy(line, m_CurrentRight, ptInt)) { return false; }
  m_CurrentRight.begin = ptInt;

  auto* LinePrimitive = new EoDbLine(*m_BeginSectionLine);

  double leftDist = EoGeVector3d(line.begin, m_CurrentLeft.begin).Length();
  double rightDist = EoGeVector3d(line.begin, m_CurrentRight.begin).Length();

  if (leftDist > rightDist) {
    m_BeginSectionLine->SetEndPoint(m_CurrentRight.begin);
    LinePrimitive->SetBeginPoint(m_CurrentLeft.begin);
  } else {
    m_BeginSectionLine->SetEndPoint(m_CurrentLeft.begin);
    LinePrimitive->SetBeginPoint(m_CurrentRight.begin);
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
    if (document == nullptr) { return; }

    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();

    CurrentPnt = GetCursorPosition();
    CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);

    EoGeLine PreviewLines[2]{};
    EoGeLine ln(m_PreviousPnt, CurrentPnt);
    ln.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, PreviewLines[0], PreviewLines[1]);

    EoGeLine beginCap{PreviewLines[0].begin, PreviewLines[1].begin};
    auto* beginCapLine = EoDbLine::CreateLine(beginCap)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
    m_PreviewGroup.AddTail(beginCapLine);

    auto* leftLine = EoDbLine::CreateLine(PreviewLines[0])->WithProperties(renderState.Color(), renderState.LineTypeIndex());
    auto* rightLine = EoDbLine::CreateLine(PreviewLines[1])->WithProperties(renderState.Color(), renderState.LineTypeIndex());

    m_PreviewGroup.AddTail(leftLine);
    m_PreviewGroup.AddTail(rightLine);
    EoGeLine endCap{PreviewLines[1].end, PreviewLines[0].end};

    auto* endCapLine = EoDbLine::CreateLine(endCap)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
    m_PreviewGroup.AddTail(endCapLine);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  }
}
