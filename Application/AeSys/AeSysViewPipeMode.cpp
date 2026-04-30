#include "Stdafx.h"

#include <cstdlib>
#include <iterator>

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbConic.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoDlgPipeOptions.h"
#include "EoDlgPipeSymbol.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "EoGsRenderState.h"
#include "EoPipeGeometry.h"
#include "EoMsPipe.h"
#include "Resource.h"

namespace {
/// Returns the active PipeModeState from the view's state stack, or nullptr when
/// called outside pipe mode (e.g. during PopAllModeStates teardown).
PipeModeState* PipeState(AeSysView* view) {
  return dynamic_cast<PipeModeState*>(view->GetCurrentState());
}
}  // namespace

namespace {
constexpr double symbolSize[] = {0.09375,
    0.09375,
    0.09375,
    0.09375,
    0.125,
    0.125,
    0.125,
    0.125,
    0.125,
    0.125,
    0.125,
    0.125,
    0.125,
    0.125,
    0.125,
    0.0,
    0.0,
    0.09375};
constexpr double tickDistance[] = {0.125,
    0.125,
    0.125,
    0.125,
    0.15625,
    0.15625,
    0.15625,
    0.15625,
    0.15625,
    0.15625,
    0.15625,
    0.15625,
    0.15625,
    0.15625,
    0.15625,
    0.03125,
    0.03125,
    0.125};

void AddLineToGroup(EoDbGroup* group, const EoGePoint3d& begin, const EoGePoint3d& end) {
  group->AddTail(EoDbLine::CreateLine(begin, end)->WithProperties(Gs::renderState));
}

void AddCircleToGroup(EoDbGroup* group, const EoGePoint3d& center, double radius) {
  group->AddTail(EoDbConic::CreateCircleInView(center, radius)->WithProperties(Gs::renderState));
}

void CreateGateValve(EoDbGroup* group, const EoGeLine& beginSection, const EoGeLine& endSection, double size) {
  EoGePoint3d topEnd{};
  EoGePoint3d bottomEnd{};
  EoGePoint3d bottomBegin{};
  EoGePoint3d topBegin{};

  endSection.ProjPtFrom_xy(size, size * 0.5, &topEnd);
  endSection.ProjPtFrom_xy(size, -size * 0.5, &bottomEnd);
  beginSection.ProjPtFrom_xy(size, -size * 0.5, &bottomBegin);
  beginSection.ProjPtFrom_xy(size, size * 0.5, &topBegin);

  AddLineToGroup(group, topEnd, bottomEnd);
  AddLineToGroup(group, bottomEnd, bottomBegin);
  AddLineToGroup(group, bottomBegin, topBegin);
  AddLineToGroup(group, topBegin, topEnd);
}

}  // namespace

void AeSysView::OnPipeModeOptions() {
  EoDlgPipeOptions dialog;
  dialog.m_PipeTicSize = m_pipeConfig.ticSize;
  dialog.m_PipeRiseDropRadius = m_pipeConfig.riseDropRadius;
  if (dialog.DoModal() == IDOK) {
    m_pipeConfig.ticSize = dialog.m_PipeTicSize;
    m_pipeConfig.riseDropRadius = dialog.m_PipeRiseDropRadius;
  }
}

void AeSysView::OnPipeModeLine() {
  auto* state = PipeState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  auto cursorPosition = GetCursorPosition();

  if (points.IsEmpty()) {
    points.Add(cursorPosition);
  } else {
    cursorPosition = SnapPointToAxis(points[0], cursorPosition);
    auto* document = GetDocument();

    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();

    auto* group = new EoDbGroup;
    document->AddWorkLayerGroup(group);
    GenerateLineWithFittings(previousOp, points[0], ID_OP2, cursorPosition, group);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);

    points[0] = cursorPosition;
  }
  previousOp = ModeLineHighlightOp(ID_OP2);
}

void AeSysView::OnPipeModeFitting() {
  auto* state = PipeState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  EoDbLine* horizontalSection{};
  auto* group = SelectLineUsingPoint(cursorPosition, horizontalSection);
  if (group != nullptr) {
    const EoGePoint3d begin = horizontalSection->Begin();
    const EoGePoint3d end = horizontalSection->End();
    if (!points.IsEmpty()) { cursorPosition = SnapPointToAxis(points[0], cursorPosition); }
    cursorPosition = horizontalSection->ProjectPointToLine(cursorPosition);
    horizontalSection->SetEndPoint(cursorPosition);
    group->AddTail(EoDbLine::CreateLine(cursorPosition, end)
            ->WithProperties(
                horizontalSection->Color(), horizontalSection->LineTypeName(), horizontalSection->LineWeight()));

    group = new EoDbGroup;
    GenerateTickMark(cursorPosition, begin, m_pipeConfig.riseDropRadius, group);
    GenerateTickMark(cursorPosition, end, m_pipeConfig.riseDropRadius, group);
    document->AddWorkLayerGroup(group);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

    if (points.IsEmpty()) {
      points.Add(cursorPosition);
      previousOp = ModeLineHighlightOp(ID_OP3);
    } else {
      GenerateTickMark(cursorPosition, points[0], m_pipeConfig.riseDropRadius, group);

      group = new EoDbGroup;
      GenerateLineWithFittings(previousOp, points[0], 0, cursorPosition, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);
      OnPipeModeEscape();
    }
  } else {
    EoDbConic* verticalSection{};
    group = SelectCircleUsingPoint(cursorPosition, m_pipeConfig.riseDropRadius, verticalSection);
    if (group != nullptr) {
      cursorPosition = verticalSection->Center();

      if (points.IsEmpty()) {
        points.Add(cursorPosition);
        previousOp = ModeLineHighlightOp(ID_OP4);
      } else {
        group = new EoDbGroup;
        GenerateLineWithFittings(previousOp, points[0], ID_OP5, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);
        OnPipeModeEscape();
      }
    } else {
      if (points.IsEmpty()) {
        points.Add(cursorPosition);
      } else {
        cursorPosition = SnapPointToAxis(points[0], cursorPosition);

        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        InvalidateOverlay();

        group = new EoDbGroup;
        GenerateLineWithFittings(previousOp, points[0], ID_OP3, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);

        points[0] = cursorPosition;
      }
      previousOp = ModeLineHighlightOp(ID_OP3);
    }
  }
}

void AeSysView::OnPipeModeRise() {
  auto* state = PipeState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  EoDbLine* horizontalSection{};
  auto* group = SelectLineUsingPoint(cursorPosition, horizontalSection);
  if (group != nullptr) {  // On an existing horizontal pipe section
    cursorPosition = horizontalSection->ProjectPointToLine(cursorPosition);

    if (points.IsEmpty()) {  // Rising from an existing horizontal pipe section
      points.Add(cursorPosition);
      DropIntoOrRiseFromHorizontalSection(cursorPosition, group, horizontalSection);
    } else {  // Rising into an existing horizontal pipe section
      DropFromOrRiseIntoHorizontalSection(cursorPosition, group, horizontalSection);
      group = new EoDbGroup;
      GenerateLineWithFittings(previousOp, points[0], ID_OP5, cursorPosition, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
    previousOp = ModeLineHighlightOp(ID_OP5);
  } else {
    EoDbConic* verticalSection{};
    group = SelectCircleUsingPoint(cursorPosition, m_pipeConfig.riseDropRadius, verticalSection);
    if (group != nullptr) {  // On an existing vertical pipe section
      cursorPosition = verticalSection->Center();
      if (points.IsEmpty()) {
        points.Add(cursorPosition);
        previousOp = ModeLineHighlightOp(ID_OP4);
      } else {  // Rising into an existing vertical pipe section
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        InvalidateOverlay();
        group = new EoDbGroup;
        GenerateLineWithFittings(previousOp, points[0], ID_OP5, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        OnPipeModeEscape();
      }
    } else {
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      InvalidateOverlay();

      if (points.IsEmpty()) {
        points.Add(cursorPosition);
      } else {
        cursorPosition = SnapPointToAxis(points[0], cursorPosition);
        group = new EoDbGroup;
        GenerateLineWithFittings(previousOp, points[0], ID_OP5, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      }
      auto* circle =
          EoDbConic::CreateCircleInView(cursorPosition, m_pipeConfig.riseDropRadius)->WithProperties(1, L"CONTINUOUS");
      group = new EoDbGroup(circle);

      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      previousOp = ModeLineHighlightOp(ID_OP5);
      points[0] = cursorPosition;
    }
  }
}

void AeSysView::OnPipeModeDrop() {
  auto* state = PipeState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  EoDbLine* horizontalSection{};
  auto* group = SelectLineUsingPoint(cursorPosition, horizontalSection);
  if (group != nullptr) {  // On an existing horizontal pipe section
    cursorPosition = horizontalSection->ProjectPointToLine(cursorPosition);

    if (points.IsEmpty()) {  // Dropping from an existing horizontal pipe section
      points.Add(cursorPosition);
      DropFromOrRiseIntoHorizontalSection(cursorPosition, group, horizontalSection);
    } else {  // Dropping into an existing horizontal pipe section
      DropIntoOrRiseFromHorizontalSection(cursorPosition, group, horizontalSection);
      group = new EoDbGroup;
      GenerateLineWithFittings(previousOp, points[0], ID_OP4, cursorPosition, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
    previousOp = ModeLineHighlightOp(ID_OP4);
  } else {
    EoDbConic* verticalSection{};
    group = SelectCircleUsingPoint(cursorPosition, m_pipeConfig.riseDropRadius, verticalSection);
    if (group != nullptr) {  // On an existing vertical pipe section
      cursorPosition = verticalSection->Center();
      if (points.IsEmpty()) {
        points.Add(cursorPosition);
        previousOp = ModeLineHighlightOp(ID_OP5);
      } else {  // Dropping into an existing vertical pipe section
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        InvalidateOverlay();
        group = new EoDbGroup;
        GenerateLineWithFittings(previousOp, points[0], ID_OP4, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        OnPipeModeEscape();
      }
    } else {
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      InvalidateOverlay();

      if (points.IsEmpty()) {
        points.Add(cursorPosition);
      } else {
        cursorPosition = SnapPointToAxis(points[0], cursorPosition);

        group = new EoDbGroup;
        GenerateLineWithFittings(previousOp, points[0], ID_OP4, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      }
      auto* circle =
          EoDbConic::CreateCircleInView(cursorPosition, m_pipeConfig.riseDropRadius)->WithProperties(1, L"CONTINUOUS");

      group = new EoDbGroup(circle);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      previousOp = ModeLineHighlightOp(ID_OP4);
      points[0] = cursorPosition;
    }
  }
}

void AeSysView::OnPipeModeSymbol() {
  if (m_pipeConfig.currentSymbolIndex < 0 || m_pipeConfig.currentSymbolIndex >= static_cast<int>(std::size(symbolSize))) { return; }

  const auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  OnPipeModeEscape();
  auto* state = PipeState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  points.SetSize(2);

  EoDbLine* horizontalSection{};
  auto* group = SelectLineUsingPoint(cursorPosition, horizontalSection);
  if (group == nullptr) { return; }

  EoDlgPipeSymbol dialog;
  dialog.m_CurrentPipeSymbolIndex = m_pipeConfig.currentSymbolIndex;
  if (dialog.DoModal() == IDOK) { m_pipeConfig.currentSymbolIndex = dialog.m_CurrentPipeSymbolIndex; }
  const EoGePoint3d begin = horizontalSection->Begin();
  const EoGePoint3d end = horizontalSection->End();
  const EoGePoint3d pointOnSection = horizontalSection->ProjectPointToLine(cursorPosition);

  EoGeLine beginSection(pointOnSection, begin);
  EoGeLine endSection(pointOnSection, end);

  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, horizontalSection);

  EoGePoint3d symbolBeginPoint = pointOnSection.ProjectToward(begin, symbolSize[m_pipeConfig.currentSymbolIndex]);
  EoGePoint3d symbolEndPoint = pointOnSection.ProjectToward(end, symbolSize[m_pipeConfig.currentSymbolIndex]);
  const double ticSize{m_pipeConfig.ticSize};

  horizontalSection->SetEndPoint(symbolBeginPoint);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, horizontalSection);
  group = new EoDbGroup(EoDbLine::CreateLine(symbolEndPoint, end)
          ->WithProperties(
              horizontalSection->Color(), horizontalSection->LineTypeName(), horizontalSection->LineWeight()));
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

  group = new EoDbGroup;
  GenerateTickMark(pointOnSection, begin, tickDistance[m_pipeConfig.currentSymbolIndex], group);
  GenerateTickMark(pointOnSection, end, tickDistance[m_pipeConfig.currentSymbolIndex], group);

  switch (m_pipeConfig.currentSymbolIndex) {
    case 0: {  // flow switch
      const double radius = EoGePoint3d::Distance(pointOnSection, symbolBeginPoint);
      AddCircleToGroup(group, pointOnSection, radius);
      endSection.ProjPtFrom_xy(symbolSize[0], -symbolSize[0] * 1.5, &points[0]);
      endSection.ProjPtFrom_xy(symbolSize[0], -symbolSize[0] * 2.0, &points[1]);
      beginSection.ProjPtFrom_xy(symbolSize[0], symbolSize[0] * 1.5, &symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[0], symbolSize[0] * 2.0, &symbolEndPoint);
      AddLineToGroup(group, points[1], symbolEndPoint);
      AddLineToGroup(group, symbolEndPoint, symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, points[0]);
      AddLineToGroup(group, points[0], points[1]);
      AddLineToGroup(group, points[1], symbolBeginPoint);
    } break;

    case 1: {  // float and thermostatic trap
      const double radius = EoGePoint3d::Distance(pointOnSection, symbolBeginPoint);
      AddCircleToGroup(group, pointOnSection, radius);
      points[0] = symbolBeginPoint.RotateAboutAxis(pointOnSection, EoGeVector3d::positiveUnitZ, Eo::QuarterPi);
      points[1] = points[0].RotateAboutAxis(pointOnSection, EoGeVector3d::positiveUnitZ, Eo::Pi);
      AddLineToGroup(group, points[0], points[1]);
      points[0] = symbolBeginPoint.RotateAboutAxis(pointOnSection, EoGeVector3d::positiveUnitZ, Eo::ThreeQuartersPi);
      points[1] = points[0].RotateAboutAxis(pointOnSection, EoGeVector3d::positiveUnitZ, Eo::Pi);
      AddLineToGroup(group, points[0], points[1]);
    } break;

    case 2: {  // ball valve
      const double radius = EoGePoint3d::Distance(pointOnSection, symbolBeginPoint);
      AddCircleToGroup(group, pointOnSection, radius);
      endSection.ProjPtFrom_xy(symbolSize[2], symbolSize[2] * 1.5, &points[0]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[2] * 1.5, &points[1]);
      AddLineToGroup(group, points[0], points[1]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[2], &symbolBeginPoint);
      AddLineToGroup(group, points[1], symbolBeginPoint);
    } break;

    case 3: {  // butterfly
      const double radius = EoGePoint3d::Distance(pointOnSection, symbolBeginPoint);
      AddCircleToGroup(group, pointOnSection, radius);
      endSection.ProjPtFrom_xy(symbolSize[3], symbolSize[3] * 1.5, &points[0]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[3] * 1.5, &points[1]);
      AddLineToGroup(group, points[0], points[1]);
      beginSection.ProjPtFrom_xy(0.0, symbolSize[3], &symbolBeginPoint);
      AddLineToGroup(group, points[1], symbolBeginPoint);
    } break;

    case 4: {  // check valve
      endSection.ProjPtFrom_xy(symbolSize[4], symbolSize[4] * 0.5, &points[0]);
      endSection.ProjPtFrom_xy(symbolSize[4], -symbolSize[4] * 0.5, &points[1]);
      AddLineToGroup(group, points[0], points[1]);
      beginSection.ProjPtFrom_xy(symbolSize[4], -symbolSize[4] * 0.5, &symbolBeginPoint);
      AddLineToGroup(group, points[1], symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[4], symbolSize[4] * 0.5, &symbolEndPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      beginSection.ProjPtFrom_xy(symbolSize[4], -symbolSize[4] * 0.3, &points[0]);
      const double radius = EoGePoint3d::Distance(symbolBeginPoint, points[0]);
      AddCircleToGroup(group, symbolBeginPoint, radius);
    } break;

    case 5: {  // non-slam check valve
      endSection.ProjPtFrom_xy(symbolSize[5], symbolSize[5] * 0.5, &points[0]);
      endSection.ProjPtFrom_xy(symbolSize[5], -symbolSize[5] * 0.5, &points[1]);
      AddLineToGroup(group, points[0], points[1]);
      beginSection.ProjPtFrom_xy(symbolSize[5], -symbolSize[5] * 0.5, &symbolBeginPoint);
      AddLineToGroup(group, points[1], symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[5], symbolSize[5] * 0.5, &symbolEndPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      beginSection.ProjPtFrom_xy(symbolSize[5], -symbolSize[5] * 0.3, &points[0]);
      const double radius = EoGePoint3d::Distance(symbolBeginPoint, points[0]);
      AddCircleToGroup(group, symbolBeginPoint, radius);
      AddLineToGroup(group, symbolEndPoint, pointOnSection);
    } break;

    case 6:  // gate valve
      CreateGateValve(group, beginSection, endSection, symbolSize[6]);
      break;

    case 7: {  // globe valve
      CreateGateValve(group, beginSection, endSection, symbolSize[7]);
      points[0] = pointOnSection.ProjectToward(end, symbolSize[7] * 0.25);
      const double radius = EoGePoint3d::Distance(pointOnSection, points[0]);
      AddCircleToGroup(group, pointOnSection, radius);
    } break;

    case 8: {  // OS&Y gate valve
      CreateGateValve(group, beginSection, endSection, symbolSize[8]);
      points[0] = pointOnSection.ProjectToward(end, symbolSize[8] * 0.25);
      const double radius = EoGePoint3d::Distance(pointOnSection, points[0]);
      AddCircleToGroup(group, pointOnSection, radius);
      endSection.ProjPtFrom_xy(0.0, symbolSize[8], &points[0]);
      AddLineToGroup(group, pointOnSection, points[0]);
      m_pipeConfig.ticSize = symbolSize[8] * 0.25;
      GenerateTickMark(pointOnSection, points[0], symbolSize[8] * 0.75, group);
    } break;

    case 9: {  // pressure reducing valve
      CreateGateValve(group, beginSection, endSection, symbolSize[9]);
      points[0] = pointOnSection.ProjectToward(end, symbolSize[9] * 0.25);
      const double radius = EoGePoint3d::Distance(pointOnSection, points[0]);
      AddCircleToGroup(group, pointOnSection, radius);
      endSection.ProjPtFrom_xy(0.0, symbolSize[9], &points[0]);
      AddLineToGroup(group, pointOnSection, points[0]);
      endSection.ProjPtFrom_xy(symbolSize[9] * 0.5, symbolSize[9] * 0.75, &points[1]);
      AddLineToGroup(group, points[0], points[1]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[9] * 0.5, &symbolBeginPoint);
      AddLineToGroup(group, points[1], symbolBeginPoint);
    } break;

    case 10:  // auto 2-way valve
      CreateGateValve(group, beginSection, endSection, symbolSize[10]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[10] * 0.5, &points[0]);
      AddLineToGroup(group, pointOnSection, points[0]);

      endSection.ProjPtFrom_xy(symbolSize[10] * 0.25, symbolSize[10] * 0.5, &points[0]);
      endSection.ProjPtFrom_xy(symbolSize[10] * 0.25, symbolSize[10] * 0.75, &points[1]);
      beginSection.ProjPtFrom_xy(symbolSize[10] * 0.25, -symbolSize[10] * 0.75, &symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[10] * 0.25, -symbolSize[10] * 0.5, &symbolEndPoint);
      AddLineToGroup(group, points[0], points[1]);
      AddLineToGroup(group, points[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      AddLineToGroup(group, symbolEndPoint, points[0]);
      break;

    case 11:  // auto 3-way valve
      CreateGateValve(group, beginSection, endSection, symbolSize[11]);

      endSection.ProjPtFrom_xy(0.0, symbolSize[11] * 0.5, &points[0]);
      AddLineToGroup(group, pointOnSection, points[0]);
      endSection.ProjPtFrom_xy(symbolSize[11] * 0.25, symbolSize[11] * 0.5, &points[0]);
      endSection.ProjPtFrom_xy(symbolSize[11] * 0.25, symbolSize[11] * 0.75, &points[1]);
      beginSection.ProjPtFrom_xy(symbolSize[11] * 0.25, -symbolSize[11] * 0.75, &symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[11] * 0.25, -symbolSize[11] * 0.5, &symbolEndPoint);
      AddLineToGroup(group, points[0], points[1]);
      AddLineToGroup(group, points[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      AddLineToGroup(group, symbolEndPoint, points[0]);
      endSection.ProjPtFrom_xy(symbolSize[11] * 0.5, -symbolSize[11], &points[0]);
      beginSection.ProjPtFrom_xy(symbolSize[11] * 0.5, symbolSize[11], &points[1]);
      AddLineToGroup(group, pointOnSection, points[0]);
      AddLineToGroup(group, points[0], points[1]);
      AddLineToGroup(group, points[1], pointOnSection);
      break;

    case 12:  // self operated valve
      CreateGateValve(group, beginSection, endSection, symbolSize[12]);

      endSection.ProjPtFrom_xy(0.0, symbolSize[12] * 0.5, &points[0]);
      AddLineToGroup(group, pointOnSection, points[0]);
      endSection.ProjPtFrom_xy(symbolSize[12] * 0.25, symbolSize[12] * 0.5, &points[1]);
      beginSection.ProjPtFrom_xy(symbolSize[12] * 0.25, -symbolSize[12] * 0.5, &symbolBeginPoint);
      AddLineToGroup(group, points[1], symbolBeginPoint);
      // add a half circle here i think
      beginSection.ProjPtFrom_xy(symbolSize[12] * 1.25, -symbolSize[12] * 0.5, &points[0]);
      AddLineToGroup(group, symbolBeginPoint, points[0]);
      beginSection.ProjPtFrom_xy(symbolSize[12] * 1.25, -symbolSize[12] * 0.75, &points[1]);
      beginSection.ProjPtFrom_xy(symbolSize[12] * 2.0, -symbolSize[12] * 0.75, &symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[12] * 2.0, -symbolSize[12] * 0.5, &symbolEndPoint);
      AddLineToGroup(group, points[0], points[1]);
      AddLineToGroup(group, points[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      AddLineToGroup(group, symbolEndPoint, points[0]);
      break;

    case 13:  // plug valve
      endSection.ProjPtFrom_xy(0.0, -symbolSize[13], &points[0]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[13], &points[1]);
      AddLineToGroup(group, symbolEndPoint, points[1]);
      AddLineToGroup(group, points[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, points[0]);
      AddLineToGroup(group, points[0], symbolEndPoint);
      break;

    case 14:  // balancing cock
      endSection.ProjPtFrom_xy(0.0, -symbolSize[14], &points[0]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[14], &points[1]);
      AddLineToGroup(group, symbolEndPoint, points[1]);
      AddLineToGroup(group, points[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, points[0]);
      AddLineToGroup(group, points[0], symbolEndPoint);
      AddLineToGroup(group, points[0], points[1]);
      break;

    case 15:  // gauge cock
      endSection.ProjPtFrom_xy(0.0, -0.250, &points[0]);
      AddLineToGroup(group, pointOnSection, points[0]);
      GenerateTickMark(pointOnSection, points[0], tickDistance[15], group);
      beginSection.ProjPtFrom_xy(0.0625, 0.1875, &points[1]);
      endSection.ProjPtFrom_xy(0.0625, -0.1875, &symbolBeginPoint);
      endSection.ProjPtFrom_xy(0.0625, -0.125, &symbolEndPoint);
      AddLineToGroup(group, points[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      break;

    case 16: {  // gauge cock with gauge
      endSection.ProjPtFrom_xy(0.0, -0.250, &points[0]);
      AddLineToGroup(group, pointOnSection, points[0]);
      GenerateTickMark(pointOnSection, points[0], tickDistance[16], group);
      beginSection.ProjPtFrom_xy(0.0625, 0.1875, &points[1]);
      endSection.ProjPtFrom_xy(0.0625, -0.1875, &symbolBeginPoint);
      endSection.ProjPtFrom_xy(0.0625, -0.125, &symbolEndPoint);
      AddLineToGroup(group, points[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      points[1] = pointOnSection.ProjectToward(points[0], 0.28125);
      const double radius = EoGePoint3d::Distance(points[1], points[0]);
      AddCircleToGroup(group, points[1], radius);
    } break;

    case 17:  // union
      m_pipeConfig.ticSize = symbolSize[17];
      GenerateTickMark(pointOnSection, begin, symbolSize[17], group);
      GenerateTickMark(pointOnSection, end, symbolSize[17], group);
      m_pipeConfig.ticSize = m_pipeConfig.ticSize * 2.0;
      GenerateTickMark(pointOnSection, begin, 0.0, group);
      break;
  }
  m_pipeConfig.ticSize = ticSize;
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

void AeSysView::OnPipeModeWye() {
  auto* state = PipeState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  const auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  if (points.IsEmpty()) {
    points.Add(cursorPosition);
    previousOp = ModeLineHighlightOp(ID_OP9);
    return;
  }
  EoDbLine* horizontalSection{};
  auto* group = SelectLineUsingPoint(cursorPosition, horizontalSection);
  if (group == nullptr) { return; }
  EoGePoint3d pointOnSection = horizontalSection->ProjectPointToLine(cursorPosition);
  const EoGePoint3d beginPointProjectedToSection = horizontalSection->ProjectPointToLine(points[0]);
  const double distanceToSection = EoGeVector3d(points[0], beginPointProjectedToSection).Length();

  if (distanceToSection >= 0.25) {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
    const EoGePoint3d begin = horizontalSection->Begin();
    const EoGePoint3d endPoint = horizontalSection->End();

    const double distanceBetweenSectionPoints = EoGeVector3d(beginPointProjectedToSection, pointOnSection).Length();

    if (std::abs(distanceBetweenSectionPoints - distanceToSection) <= 0.25) {
      // Just need to shift point on section and do a single 45 degree line
      pointOnSection = beginPointProjectedToSection.ProjectToward(pointOnSection, distanceToSection);
      horizontalSection->SetEndPoint(pointOnSection);
      group = new EoDbGroup(EoDbLine::CreateLine(pointOnSection, endPoint)
              ->WithProperties(
                  horizontalSection->Color(), horizontalSection->LineTypeName(), horizontalSection->LineWeight()));
      document->AddWorkLayerGroup(group);

      group = new EoDbGroup;
      GenerateTickMark(pointOnSection, begin, m_pipeConfig.riseDropRadius, group);
      GenerateTickMark(pointOnSection, endPoint, m_pipeConfig.riseDropRadius, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      group = new EoDbGroup;
      GenerateLineWithFittings(previousOp, points[0], ID_OP3, pointOnSection, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    } else {
      EoGePoint3d pointAtBend;

      if (distanceBetweenSectionPoints - 0.25 <= distanceToSection) {
        const double d3 = (distanceBetweenSectionPoints > 0.25) ? distanceBetweenSectionPoints : 0.125;
        pointAtBend = beginPointProjectedToSection.ProjectToward(points[0], d3);
        pointOnSection = beginPointProjectedToSection.ProjectToward(pointOnSection, d3);
      } else {
        pointAtBend = beginPointProjectedToSection.ProjectToward(
            pointOnSection, distanceBetweenSectionPoints - distanceToSection);
        pointAtBend = points[0] + EoGeVector3d(beginPointProjectedToSection, pointAtBend);
      }
      horizontalSection->SetEndPoint(pointOnSection);

      group = new EoDbGroup;
      GenerateTickMark(pointOnSection, begin, m_pipeConfig.riseDropRadius, group);
      GenerateTickMark(pointOnSection, endPoint, m_pipeConfig.riseDropRadius, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      group = new EoDbGroup(EoDbLine::CreateLine(pointOnSection, endPoint)
              ->WithProperties(
                  horizontalSection->Color(), horizontalSection->LineTypeName(), horizontalSection->LineWeight()));
      document->AddWorkLayerGroup(group);
      group = new EoDbGroup;
      GenerateLineWithFittings(previousOp, points[0], ID_OP3, pointAtBend, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      group = new EoDbGroup;
      GenerateLineWithFittings(ID_OP3, pointAtBend, ID_OP3, pointOnSection, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
  }
  OnPipeModeEscape();
}

void AeSysView::OnPipeModeReturn() {
  OnPipeModeEscape();
}

void AeSysView::OnPipeModeEscape() {
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateOverlay();

  auto* state = PipeState(this);
  if (state == nullptr) { return; }
  state->Points().RemoveAll();
  state->UnhighlightOp(this);
}

void AeSysView::DoPipeModeMouseMove() {
  // Preview logic moved to PipeModeState::OnMouseMove.
}

void AeSysView::GenerateLineWithFittings(int beginType,
    const EoGePoint3d& begin,
    int endType,
    const EoGePoint3d& end,
    EoDbGroup* group) {
  Pipe::GenerateLineWithFittings(beginType, begin, endType, end, m_pipeConfig.riseDropRadius, m_pipeConfig.ticSize, group);
}

void AeSysView::DropIntoOrRiseFromHorizontalSection(const EoGePoint3d& point, EoDbGroup* group, EoDbLine* section) {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, section);

  const EoGePoint3d begin = section->Begin();
  const EoGePoint3d end = section->End();

  auto cutPoint = point.ProjectToward(begin, m_pipeConfig.riseDropRadius);
  section->SetEndPoint(cutPoint);
  cutPoint = point.ProjectToward(end, m_pipeConfig.riseDropRadius);
  group->AddTail(EoDbLine::CreateLine(cutPoint, end)
          ->WithProperties(section->Color(), section->LineTypeName(), section->LineWeight()));
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

  group = new EoDbGroup;
  GenerateTickMark(point, begin, 2.0 * m_pipeConfig.riseDropRadius, group);

  auto* circle = EoDbConic::CreateCircleInView(point, m_pipeConfig.riseDropRadius)->WithProperties(1, L"CONTINUOUS");

  group->AddTail(circle);
  GenerateTickMark(point, end, 2.0 * m_pipeConfig.riseDropRadius, group);
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

void AeSysView::DropFromOrRiseIntoHorizontalSection(const EoGePoint3d& point,
    EoDbGroup* group,
    EoDbLine* section) const {
  auto* document = GetDocument();
  const EoGePoint3d begin = section->Begin();
  const EoGePoint3d end = section->End();

  section->SetEndPoint(point);
  group->AddTail(EoDbLine::CreateLine(point, end)
          ->WithProperties(section->Color(), section->LineTypeName(), section->LineWeight()));

  group = new EoDbGroup{};
  GenerateTickMark(point, begin, 2.0 * m_pipeConfig.riseDropRadius, group);

  auto* circle = EoDbConic::CreateCircleInView(point, m_pipeConfig.riseDropRadius)->WithProperties(1, L"CONTINUOUS");
  group->AddTail(circle);

  GenerateTickMark(point, end, 2.0 * m_pipeConfig.riseDropRadius, group);
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

bool AeSysView::GenerateTickMark(const EoGePoint3d& begin,
    const EoGePoint3d& end,
    double distance,
    EoDbGroup* group) const {
  return Pipe::GenerateTickMark(begin, end, distance, m_pipeConfig.ticSize, group);
}


