#include "Stdafx.h"

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Draw2ModeState.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDlgSetLength.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "EoGsRenderState.h"

namespace {
/// Returns the active Draw2ModeState from the view's state stack, or nullptr when
/// called outside draw2 mode (e.g. during PopAllModeStates teardown).
Draw2ModeState* Draw2State(AeSysView* view) {
  return dynamic_cast<Draw2ModeState*>(view->GetCurrentState());
}
}  // namespace

void AeSysView::OnDraw2ModeOptions() {
  EoDlgSetLength dialog;
  dialog.SetTitle(L"Set Distance Between Lines");
  dialog.SetLength(m_distanceBetweenLines);

  if (dialog.DoModal() == IDOK) { m_distanceBetweenLines = dialog.Length(); }
}

void AeSysView::OnDraw2ModeJoin() {
  auto* state = Draw2State(this);
  if (state == nullptr) { return; }

  auto cursorPosition = GetCursorPosition();
  cursorPosition = SnapPointToAxis(state->PreviousPoint(), cursorPosition);

  auto* group = SelectGroupAndPrimitive(cursorPosition);
  if (group == nullptr) { return; }

  auto* engagedPrimitive = EngagedPrimitive();
  if (engagedPrimitive == nullptr || !engagedPrimitive->Is(EoDb::kLinePrimitive)) { return; }

  cursorPosition = DetPt();

  if (state->PreviousOp() == 0) {  // Starting at existing wall
    state->BeginSectionGroupRef() = group;
    state->BeginSectionLinePrimitiveRef() = static_cast<EoDbLine*>(engagedPrimitive);
    state->SetPreviousPoint(cursorPosition);
    state->SetPreviousOp(ID_OP1);
  } else {  // Ending at existing wall
    state->EndSectionGroupRef() = group;
    state->EndSectionLinePrimitiveRef() = static_cast<EoDbLine*>(EngagedPrimitive());
    OnDraw2ModeWall();
    OnDraw2ModeEscape();
  }
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnDraw2ModeWall() {
  auto* state = Draw2State(this);
  if (state == nullptr) { return; }

  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();
  if (document == nullptr) { return; }

  auto& previousOp = state->PreviousOpRef();
  auto& previousPoint = state->PreviousPointRef();
  auto& continuingCorner = state->ContinuingCornerRef();
  auto& currentReferenceLine = state->CurrentReferenceLineRef();
  auto& previousReferenceLine = state->PreviousReferenceLineRef();
  auto& currentLeftLine = state->CurrentLeftLineRef();
  auto& currentRightLine = state->CurrentRightLineRef();
  auto*& assemblyGroup = state->AssemblyGroupRef();
  auto*& beginSectionGroup = state->BeginSectionGroupRef();
  auto*& endSectionGroup = state->EndSectionGroupRef();
  auto*& endSectionLinePrimitive = state->EndSectionLinePrimitiveRef();

  if (previousOp != 0) {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
  }
  if (endSectionGroup == nullptr) {
    if (previousOp != 0) {
      cursorPosition = SnapPointToAxis(previousPoint, cursorPosition);

      currentReferenceLine(previousPoint, cursorPosition);
      if (!currentReferenceLine.GetParallels(
              m_distanceBetweenLines, m_centerLineEccentricity, currentLeftLine, currentRightLine)) {
        return;
      }

      if (continuingCorner) {
        CleanPreviousLines();
      } else if (beginSectionGroup != nullptr) {
        StartAssemblyFromLine();
      } else if (previousOp == ID_OP2) {
        assemblyGroup = new EoDbGroup;
        document->AddWorkLayerGroup(assemblyGroup);

        auto* beginCapLine =
            EoDbLine::CreateLine(currentLeftLine.begin, currentRightLine.begin)->WithProperties(Gs::renderState);
        assemblyGroup->AddTail(beginCapLine);
      }
      auto* leftLine = EoDbLine::CreateLine(currentLeftLine)->WithProperties(Gs::renderState);
      auto* rightLine = EoDbLine::CreateLine(currentRightLine)->WithProperties(Gs::renderState);
      auto* endCapLine =
          EoDbLine::CreateLine(currentRightLine.end, currentLeftLine.end)->WithProperties(Gs::renderState);

      assemblyGroup->AddTail(leftLine);
      assemblyGroup->AddTail(rightLine);
      assemblyGroup->AddTail(endCapLine);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, assemblyGroup);
      continuingCorner = true;
      previousReferenceLine = currentReferenceLine;
    }
    previousOp = ID_OP2;
    previousPoint = cursorPosition;
    SetCursorPosition(previousPoint);
  } else {
    currentReferenceLine(previousPoint, cursorPosition);
    if (!currentReferenceLine.GetParallels(
            m_distanceBetweenLines, m_centerLineEccentricity, currentLeftLine, currentRightLine)) {
      return;
    }
    if (continuingCorner) {
      CleanPreviousLines();
    } else if (beginSectionGroup != nullptr) {
      StartAssemblyFromLine();
    } else if (previousOp == ID_OP2) {
      assemblyGroup = new EoDbGroup;
      document->AddWorkLayerGroup(assemblyGroup);
      const EoGeLine beginCap{currentLeftLine.begin, currentRightLine.begin};
      auto* beginCapLine = EoDbLine::CreateLine(beginCap)->WithProperties(Gs::renderState);
      assemblyGroup->AddTail(beginCapLine);
    }
    auto* leftLine = EoDbLine::CreateLine(currentLeftLine)->WithProperties(Gs::renderState);
    auto* rightLine = EoDbLine::CreateLine(currentRightLine)->WithProperties(Gs::renderState);
    assemblyGroup->AddTail(leftLine);
    assemblyGroup->AddTail(rightLine);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, assemblyGroup);

    if (endSectionLinePrimitive == nullptr) { return; }

    const EoGePoint3d begin = endSectionLinePrimitive->Begin();
    auto* linePrimitive = new EoDbLine(*endSectionLinePrimitive);
    if (EoGeLine(previousPoint, cursorPosition).DirRelOfPt(begin) < 0.0) {
      endSectionLinePrimitive->SetEndPoint(currentRightLine.end);
      linePrimitive->SetBeginPoint(currentLeftLine.end);
    } else {
      endSectionLinePrimitive->SetEndPoint(currentLeftLine.end);
      linePrimitive->SetBeginPoint(currentRightLine.end);
    }
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, endSectionGroup);

    endSectionGroup->AddTail(linePrimitive);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, endSectionGroup);
    endSectionGroup = nullptr;

    ModeLineUnhighlightOp(previousOp);
    continuingCorner = false;
  }
  previousPoint = cursorPosition;
}

void AeSysView::OnDraw2ModeReturn() {
  auto* state = Draw2State(this);
  if (state == nullptr) { return; }

  if (state->PreviousOp() != 0) { OnDraw2ModeEscape(); }

  state->SetPreviousPoint(GetCursorPosition());
}

void AeSysView::OnDraw2ModeEscape() {
  auto* state = Draw2State(this);
  if (state == nullptr) { return; }

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateOverlay();

  ModeLineUnhighlightOp(state->PreviousOpRef());
  state->SetPreviousOp(0);

  state->ContinuingCornerRef() = false;
  state->AssemblyGroupRef() = nullptr;
  state->BeginSectionGroupRef() = nullptr;
  state->BeginSectionLinePrimitiveRef() = nullptr;
  state->EndSectionGroupRef() = nullptr;
  state->EndSectionLinePrimitiveRef() = nullptr;
}

bool AeSysView::CleanPreviousLines() {
  auto* state = Draw2State(this);
  if (state == nullptr) { return false; }

  auto* document = GetDocument();
  if (document == nullptr) { return false; }

  auto& previousReferenceLine = state->PreviousReferenceLineRef();
  auto& currentReferenceLine = state->CurrentReferenceLineRef();
  auto& currentLeftLine = state->CurrentLeftLineRef();
  auto& currentRightLine = state->CurrentRightLineRef();
  auto* assemblyGroup = state->AssemblyGroupRef();

  if (previousReferenceLine.ParallelTo(currentReferenceLine)) { return false; }

  EoGeLine previousLeftLine{};
  EoGeLine previousRightLine{};

  if (!previousReferenceLine.GetParallels(
          m_distanceBetweenLines, m_centerLineEccentricity, previousLeftLine, previousRightLine)) {
    return false;
  }

  EoGePoint3d intersection{};
  if (!EoGeLine::Intersection_xy(previousLeftLine, currentLeftLine, intersection)) { return false; }
  previousLeftLine.end = currentLeftLine.begin = intersection;

  if (!EoGeLine::Intersection_xy(previousRightLine, currentRightLine, intersection)) { return false; }
  previousRightLine.end = currentRightLine.begin = intersection;

  if (assemblyGroup == nullptr) { return false; }

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, assemblyGroup);

  auto tailPrimitive = assemblyGroup->RemoveTail();
  delete tailPrimitive;

  auto position = assemblyGroup->GetTailPosition();
  if (position == nullptr) { return false; }

  auto* linePrimitive = static_cast<EoDbLine*>(assemblyGroup->GetPrev(position));
  if (linePrimitive == nullptr) { return false; }

  linePrimitive->SetEndPoint(previousRightLine.end);
  linePrimitive = static_cast<EoDbLine*>(assemblyGroup->GetPrev(position));
  if (linePrimitive == nullptr) { return false; }

  linePrimitive->SetEndPoint(previousLeftLine.end);

  return true;
}

bool AeSysView::StartAssemblyFromLine() {
  auto* state = Draw2State(this);
  if (state == nullptr) { return false; }

  auto* document = GetDocument();
  auto*& beginSectionLinePrimitive = state->BeginSectionLinePrimitiveRef();
  if (document == nullptr || beginSectionLinePrimitive == nullptr) { return false; }

  auto& currentReferenceLine = state->CurrentReferenceLineRef();
  auto& currentLeftLine = state->CurrentLeftLineRef();
  auto& currentRightLine = state->CurrentRightLineRef();
  auto*& assemblyGroup = state->AssemblyGroupRef();
  auto* beginSectionGroup = state->BeginSectionGroupRef();

  const EoGeLine line = beginSectionLinePrimitive->Line();

  const bool isParallelTo = line.ParallelTo(currentReferenceLine);
  if (isParallelTo) { return false; }

  EoGePoint3d intersection;
  assemblyGroup = beginSectionGroup;

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, assemblyGroup);

  if (!EoGeLine::Intersection_xy(line, currentLeftLine, intersection)) { return false; }
  currentLeftLine.begin = intersection;

  if (!EoGeLine::Intersection_xy(line, currentRightLine, intersection)) { return false; }
  currentRightLine.begin = intersection;

  auto* linePrimitive = new EoDbLine(*beginSectionLinePrimitive);

  const double leftDistance = EoGeVector3d(line.begin, currentLeftLine.begin).Length();
  const double rightDistance = EoGeVector3d(line.begin, currentRightLine.begin).Length();

  if (leftDistance > rightDistance) {
    beginSectionLinePrimitive->SetEndPoint(currentRightLine.begin);
    linePrimitive->SetBeginPoint(currentLeftLine.begin);
  } else {
    beginSectionLinePrimitive->SetEndPoint(currentLeftLine.begin);
    linePrimitive->SetBeginPoint(currentRightLine.begin);
  }
  assemblyGroup->AddTail(linePrimitive);
  beginSectionLinePrimitive = nullptr;

  return true;
}

void AeSysView::DoDraw2ModeMouseMove() {
  // Preview logic moved to Draw2ModeState::OnMouseMove.
}
