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
  EoDlgSetLength dialog;
  dialog.SetTitle(L"Set Distance Between Lines");
  dialog.SetLength(m_distanceBetweenLines);

  if (dialog.DoModal() == IDOK) { m_distanceBetweenLines = dialog.Length(); }
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
    m_beginSectionGroup = group;
    m_beginSectionLinePrimitive = static_cast<EoDbLine*>(engagedPrimitive);
    m_PreviousPnt = cursorPosition;
    m_PreviousOp = ID_OP1;
  } else {  // Ending at existing wall
    m_endSectionGroup = group;
    m_endSectionLinePrimitive = static_cast<EoDbLine*>(EngagedPrimitive());
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
  if (m_endSectionGroup == nullptr) {
    if (m_PreviousOp != 0) {
      cursorPosition = SnapPointToAxis(m_PreviousPnt, cursorPosition);

      m_currentReferenceLine(m_PreviousPnt, cursorPosition);
      m_currentReferenceLine.GetParallels(m_distanceBetweenLines, m_centerLineEccentricity, m_currentLeftLine, m_currentRightLine);

      if (m_continuingCorner) {
        CleanPreviousLines();
      } else if (m_beginSectionGroup != nullptr) {
        StartAssemblyFromLine();
      } else if (m_PreviousOp == ID_OP2) {
        m_assemblyGroup = new EoDbGroup;
        document->AddWorkLayerGroup(m_assemblyGroup);

        auto* beginCapLine = EoDbLine::CreateLine(m_currentLeftLine.begin, m_currentRightLine.begin)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
        m_assemblyGroup->AddTail(beginCapLine);
      }
      auto* leftLine = EoDbLine::CreateLine(m_currentLeftLine)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
      auto* rightLine = EoDbLine::CreateLine(m_currentRightLine)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
      auto* endCapLine = EoDbLine::CreateLine(m_currentRightLine.end, m_currentLeftLine.end)->WithProperties(renderState.Color(), renderState.LineTypeIndex());

      m_assemblyGroup->AddTail(leftLine);
      m_assemblyGroup->AddTail(rightLine);
      m_assemblyGroup->AddTail(endCapLine);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_assemblyGroup);
      m_continuingCorner = true;
      m_previousReferenceLine = m_currentReferenceLine;
    }
    m_PreviousOp = ID_OP2;
    m_PreviousPnt = cursorPosition;
    SetCursorPosition(m_PreviousPnt);
  } else {
    m_currentReferenceLine(m_PreviousPnt, cursorPosition);
    m_currentReferenceLine.GetParallels(m_distanceBetweenLines, m_centerLineEccentricity, m_currentLeftLine, m_currentRightLine);

    if (m_continuingCorner) {
      CleanPreviousLines();
    } else if (m_beginSectionGroup != nullptr) {
      StartAssemblyFromLine();
    } else if (m_PreviousOp == ID_OP2) {
      m_assemblyGroup = new EoDbGroup;
      document->AddWorkLayerGroup(m_assemblyGroup);
      EoGeLine beginCap{m_currentLeftLine.begin, m_currentRightLine.begin};
      auto* beginCapLine = EoDbLine::CreateLine(beginCap)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
      m_assemblyGroup->AddTail(beginCapLine);
    }
    auto* leftLine = EoDbLine::CreateLine(m_currentLeftLine)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
    auto* rightLine = EoDbLine::CreateLine(m_currentRightLine)->WithProperties(renderState.Color(), renderState.LineTypeIndex());
    m_assemblyGroup->AddTail(leftLine);
    m_assemblyGroup->AddTail(rightLine);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_assemblyGroup);

    if (m_endSectionLinePrimitive == nullptr) { return; }

    EoGePoint3d ptBeg = m_endSectionLinePrimitive->Begin();
    EoDbLine* LinePrimitive = new EoDbLine(*m_endSectionLinePrimitive);
    if (EoGeLine(m_PreviousPnt, cursorPosition).DirRelOfPt(ptBeg) < 0.0) {
      m_endSectionLinePrimitive->SetEndPoint(m_currentRightLine.end);
      LinePrimitive->SetBeginPoint(m_currentLeftLine.end);
    } else {
      m_endSectionLinePrimitive->SetEndPoint(m_currentLeftLine.end);
      LinePrimitive->SetBeginPoint(m_currentRightLine.end);
    }
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_endSectionGroup);

    m_endSectionGroup->AddTail(LinePrimitive);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_endSectionGroup);
    m_endSectionGroup = nullptr;

    ModeLineUnhighlightOp(m_PreviousOp);
    m_continuingCorner = false;
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

  m_continuingCorner = false;
  m_assemblyGroup = nullptr;
  m_beginSectionGroup = nullptr;
  m_beginSectionLinePrimitive = nullptr;
  m_endSectionGroup = nullptr;
  m_endSectionLinePrimitive = nullptr;
}

bool AeSysView::CleanPreviousLines() {
  auto* document = GetDocument();
  if (document == nullptr) { return false; }

  if (m_previousReferenceLine.ParallelTo(m_currentReferenceLine)) { return false; }

  EoGeLine previousLeftLine{};
  EoGeLine previousRightLine{};

  m_previousReferenceLine.GetParallels(m_distanceBetweenLines, m_centerLineEccentricity, previousLeftLine, previousRightLine);

  EoGePoint3d intersection{};
  if (!EoGeLine::Intersection_xy(previousLeftLine, m_currentLeftLine, intersection)) { return false; }
  previousLeftLine.end = m_currentLeftLine.begin = intersection;

  if (!EoGeLine::Intersection_xy(previousRightLine, m_currentRightLine, intersection)) { return false; }
  previousRightLine.end = m_currentRightLine.begin = intersection;

  if (m_assemblyGroup == nullptr) { return false; }

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_assemblyGroup);

  auto tailPrimitive = m_assemblyGroup->RemoveTail();
  delete tailPrimitive;

  auto position = m_assemblyGroup->GetTailPosition();
  if (position == nullptr) { return false; }

  auto* linePrimitive = static_cast<EoDbLine*>(m_assemblyGroup->GetPrev(position));
  if (linePrimitive == nullptr) { return false; }

  linePrimitive->SetEndPoint(previousRightLine.end);
  linePrimitive = static_cast<EoDbLine*>(m_assemblyGroup->GetPrev(position));
  if (linePrimitive == nullptr) { return false; }

  linePrimitive->SetEndPoint(previousLeftLine.end);

  return true;
}

bool AeSysView::StartAssemblyFromLine() {
  auto* document = GetDocument();
  if (document == nullptr || m_beginSectionLinePrimitive == nullptr) { return false; }

  EoGeLine line = m_beginSectionLinePrimitive->Line();

  bool isParallelTo = line.ParallelTo(m_currentReferenceLine);
  if (isParallelTo) { return false; }

  EoGePoint3d ptInt;
  m_assemblyGroup = m_beginSectionGroup;

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_assemblyGroup);

  if (!EoGeLine::Intersection_xy(line, m_currentLeftLine, ptInt)) { return false; }
  m_currentLeftLine.begin = ptInt;

  if (!EoGeLine::Intersection_xy(line, m_currentRightLine, ptInt)) { return false; }
  m_currentRightLine.begin = ptInt;

  auto* LinePrimitive = new EoDbLine(*m_beginSectionLinePrimitive);

  double leftDist = EoGeVector3d(line.begin, m_currentLeftLine.begin).Length();
  double rightDist = EoGeVector3d(line.begin, m_currentRightLine.begin).Length();

  if (leftDist > rightDist) {
    m_beginSectionLinePrimitive->SetEndPoint(m_currentRightLine.begin);
    LinePrimitive->SetBeginPoint(m_currentLeftLine.begin);
  } else {
    m_beginSectionLinePrimitive->SetEndPoint(m_currentLeftLine.begin);
    LinePrimitive->SetBeginPoint(m_currentRightLine.begin);
  }
  m_assemblyGroup->AddTail(LinePrimitive);
  m_beginSectionLinePrimitive = nullptr;

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
    ln.GetParallels(m_distanceBetweenLines, m_centerLineEccentricity, PreviewLines[0], PreviewLines[1]);

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
