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
#include "Resource.h"

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
  dialog.m_PipeTicSize = m_PipeTicSize;
  dialog.m_PipeRiseDropRadius = m_PipeRiseDropRadius;
  if (dialog.DoModal() == IDOK) {
    m_PipeTicSize = dialog.m_PipeTicSize;
    m_PipeRiseDropRadius = dialog.m_PipeRiseDropRadius;
  }
}

void AeSysView::OnPipeModeLine() {
  auto cursorPosition = GetCursorPosition();

  if (pts.IsEmpty()) {
    pts.Add(cursorPosition);
  } else {
    cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
    auto* document = GetDocument();

    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();

    auto* group = new EoDbGroup;
    document->AddWorkLayerGroup(group);
    GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP2, cursorPosition, group);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);

    pts[0] = cursorPosition;
  }
  m_PreviousOp = ModeLineHighlightOp(ID_OP2);
}

void AeSysView::OnPipeModeFitting() {
  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  EoDbLine* horizontalSection{};
  auto* group = SelectLineUsingPoint(cursorPosition, horizontalSection);
  if (group != nullptr) {
    const EoGePoint3d begin = horizontalSection->Begin();
    const EoGePoint3d end = horizontalSection->End();
    if (!pts.IsEmpty()) { cursorPosition = SnapPointToAxis(pts[0], cursorPosition); }
    cursorPosition = horizontalSection->ProjectPointToLine(cursorPosition);
    horizontalSection->SetEndPoint(cursorPosition);
    group->AddTail(EoDbLine::CreateLine(cursorPosition, end)
            ->WithProperties(
                horizontalSection->Color(), horizontalSection->LineTypeName(), horizontalSection->LineWeight()));

    group = new EoDbGroup;
    GenerateTickMark(cursorPosition, begin, m_PipeRiseDropRadius, group);
    GenerateTickMark(cursorPosition, end, m_PipeRiseDropRadius, group);
    document->AddWorkLayerGroup(group);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

    if (pts.IsEmpty()) {
      pts.Add(cursorPosition);
      m_PreviousOp = ModeLineHighlightOp(ID_OP3);
    } else {
      GenerateTickMark(cursorPosition, pts[0], m_PipeRiseDropRadius, group);

      group = new EoDbGroup;
      GenerateLineWithFittings(m_PreviousOp, pts[0], 0, cursorPosition, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);
      OnPipeModeEscape();
    }
  } else {
    EoDbConic* verticalSection{};
    group = SelectCircleUsingPoint(cursorPosition, m_PipeRiseDropRadius, verticalSection);
    if (group != nullptr) {
      cursorPosition = verticalSection->Center();

      if (pts.IsEmpty()) {
        pts.Add(cursorPosition);
        m_PreviousOp = ModeLineHighlightOp(ID_OP4);
      } else {
        group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP5, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);
        OnPipeModeEscape();
      }
    } else {
      if (pts.IsEmpty()) {
        pts.Add(cursorPosition);
      } else {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);

        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        InvalidateOverlay();

        group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);

        pts[0] = cursorPosition;
      }
      m_PreviousOp = ModeLineHighlightOp(ID_OP3);
    }
  }
}

void AeSysView::OnPipeModeRise() {
  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  EoDbLine* horizontalSection{};
  auto* group = SelectLineUsingPoint(cursorPosition, horizontalSection);
  if (group != nullptr) {  // On an existing horizontal pipe section
    cursorPosition = horizontalSection->ProjectPointToLine(cursorPosition);

    if (pts.IsEmpty()) {  // Rising from an existing horizontal pipe section
      pts.Add(cursorPosition);
      DropIntoOrRiseFromHorizontalSection(cursorPosition, group, horizontalSection);
    } else {  // Rising into an existing horizontal pipe section
      DropFromOrRiseIntoHorizontalSection(cursorPosition, group, horizontalSection);
      group = new EoDbGroup;
      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP5, cursorPosition, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
    m_PreviousOp = ModeLineHighlightOp(ID_OP5);
  } else {
    EoDbConic* verticalSection{};
    group = SelectCircleUsingPoint(cursorPosition, m_PipeRiseDropRadius, verticalSection);
    if (group != nullptr) {  // On an existing vertical pipe section
      cursorPosition = verticalSection->Center();
      if (pts.IsEmpty()) {
        pts.Add(cursorPosition);
        m_PreviousOp = ModeLineHighlightOp(ID_OP4);
      } else {  // Rising into an existing vertical pipe section
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        InvalidateOverlay();
        group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP5, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        OnPipeModeEscape();
      }
    } else {
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      InvalidateOverlay();

      if (pts.IsEmpty()) {
        pts.Add(cursorPosition);
      } else {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP5, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      }
      auto* circle =
          EoDbConic::CreateCircleInView(cursorPosition, m_PipeRiseDropRadius)->WithProperties(1, L"CONTINUOUS");
      group = new EoDbGroup(circle);

      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      m_PreviousOp = ModeLineHighlightOp(ID_OP5);
      pts[0] = cursorPosition;
    }
  }
}

void AeSysView::OnPipeModeDrop() {
  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  EoDbLine* horizontalSection{};
  auto* group = SelectLineUsingPoint(cursorPosition, horizontalSection);
  if (group != nullptr) {  // On an existing horizontal pipe section
    cursorPosition = horizontalSection->ProjectPointToLine(cursorPosition);

    if (pts.IsEmpty()) {  // Dropping from an existing horizontal pipe section
      pts.Add(cursorPosition);
      DropFromOrRiseIntoHorizontalSection(cursorPosition, group, horizontalSection);
    } else {  // Dropping into an existing horizontal pipe section
      DropIntoOrRiseFromHorizontalSection(cursorPosition, group, horizontalSection);
      group = new EoDbGroup;
      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP4, cursorPosition, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
    m_PreviousOp = ModeLineHighlightOp(ID_OP4);
  } else {
    EoDbConic* verticalSection{};
    group = SelectCircleUsingPoint(cursorPosition, m_PipeRiseDropRadius, verticalSection);
    if (group != nullptr) {  // On an existing vertical pipe section
      cursorPosition = verticalSection->Center();
      if (pts.IsEmpty()) {
        pts.Add(cursorPosition);
        m_PreviousOp = ModeLineHighlightOp(ID_OP5);
      } else {  // Dropping into an existing vertical pipe section
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        InvalidateOverlay();
        group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP4, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        OnPipeModeEscape();
      }
    } else {
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      InvalidateOverlay();

      if (pts.IsEmpty()) {
        pts.Add(cursorPosition);
      } else {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);

        group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP4, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      }
      auto* circle =
          EoDbConic::CreateCircleInView(cursorPosition, m_PipeRiseDropRadius)->WithProperties(1, L"CONTINUOUS");

      group = new EoDbGroup(circle);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      m_PreviousOp = ModeLineHighlightOp(ID_OP4);
      pts[0] = cursorPosition;
    }
  }
}

void AeSysView::OnPipeModeSymbol() {
  if (m_CurrentPipeSymbolIndex < 0 || m_CurrentPipeSymbolIndex >= static_cast<int>(std::size(symbolSize))) { return; }

  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  OnPipeModeEscape();
  pts.SetSize(2);

  EoDbLine* horizontalSection{};
  auto* group = SelectLineUsingPoint(cursorPosition, horizontalSection);
  if (group == nullptr) { return; }

  EoDlgPipeSymbol dialog;
  dialog.m_CurrentPipeSymbolIndex = m_CurrentPipeSymbolIndex;
  if (dialog.DoModal() == IDOK) { m_CurrentPipeSymbolIndex = dialog.m_CurrentPipeSymbolIndex; }
  EoGePoint3d begin = horizontalSection->Begin();
  EoGePoint3d end = horizontalSection->End();
  EoGePoint3d pointOnSection = horizontalSection->ProjectPointToLine(cursorPosition);

  EoGeLine beginSection(pointOnSection, begin);
  EoGeLine endSection(pointOnSection, end);

  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, horizontalSection);

  EoGePoint3d symbolBeginPoint = pointOnSection.ProjectToward(begin, symbolSize[m_CurrentPipeSymbolIndex]);
  EoGePoint3d symbolEndPoint = pointOnSection.ProjectToward(end, symbolSize[m_CurrentPipeSymbolIndex]);
  double ticSize = m_PipeTicSize;

  horizontalSection->SetEndPoint(symbolBeginPoint);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, horizontalSection);
  group = new EoDbGroup(EoDbLine::CreateLine(symbolEndPoint, end)
          ->WithProperties(
              horizontalSection->Color(), horizontalSection->LineTypeName(), horizontalSection->LineWeight()));
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

  group = new EoDbGroup;
  GenerateTickMark(pointOnSection, begin, tickDistance[m_CurrentPipeSymbolIndex], group);
  GenerateTickMark(pointOnSection, end, tickDistance[m_CurrentPipeSymbolIndex], group);

  switch (m_CurrentPipeSymbolIndex) {
    case 0: {  // flow switch
      double radius = EoGePoint3d::Distance(pointOnSection, symbolBeginPoint);
      AddCircleToGroup(group, pointOnSection, radius);
      endSection.ProjPtFrom_xy(symbolSize[0], -symbolSize[0] * 1.5, &pts[0]);
      endSection.ProjPtFrom_xy(symbolSize[0], -symbolSize[0] * 2.0, &pts[1]);
      beginSection.ProjPtFrom_xy(symbolSize[0], symbolSize[0] * 1.5, &symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[0], symbolSize[0] * 2.0, &symbolEndPoint);
      AddLineToGroup(group, pts[1], symbolEndPoint);
      AddLineToGroup(group, symbolEndPoint, symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, pts[0]);
      AddLineToGroup(group, pts[0], pts[1]);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
    } break;

    case 1: {  // float and thermostatic trap
      double radius = EoGePoint3d::Distance(pointOnSection, symbolBeginPoint);
      AddCircleToGroup(group, pointOnSection, radius);
      pts[0] = symbolBeginPoint.RotateAboutAxis(pointOnSection, EoGeVector3d::positiveUnitZ, Eo::QuarterPi);
      pts[1] = pts[0].RotateAboutAxis(pointOnSection, EoGeVector3d::positiveUnitZ, Eo::Pi);
      AddLineToGroup(group, pts[0], pts[1]);
      pts[0] = symbolBeginPoint.RotateAboutAxis(pointOnSection, EoGeVector3d::positiveUnitZ, Eo::ThreeQuartersPi);
      pts[1] = pts[0].RotateAboutAxis(pointOnSection, EoGeVector3d::positiveUnitZ, Eo::Pi);
      AddLineToGroup(group, pts[0], pts[1]);
    } break;

    case 2: {  // ball valve
      double radius = EoGePoint3d::Distance(pointOnSection, symbolBeginPoint);
      AddCircleToGroup(group, pointOnSection, radius);
      endSection.ProjPtFrom_xy(symbolSize[2], symbolSize[2] * 1.5, &pts[0]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[2] * 1.5, &pts[1]);
      AddLineToGroup(group, pts[0], pts[1]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[2], &symbolBeginPoint);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
    } break;

    case 3: {  // butterfly
      double radius = EoGePoint3d::Distance(pointOnSection, symbolBeginPoint);
      AddCircleToGroup(group, pointOnSection, radius);
      endSection.ProjPtFrom_xy(symbolSize[3], symbolSize[3] * 1.5, &pts[0]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[3] * 1.5, &pts[1]);
      AddLineToGroup(group, pts[0], pts[1]);
      beginSection.ProjPtFrom_xy(0.0, symbolSize[3], &symbolBeginPoint);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
    } break;

    case 4: {  // check valve
      endSection.ProjPtFrom_xy(symbolSize[4], symbolSize[4] * 0.5, &pts[0]);
      endSection.ProjPtFrom_xy(symbolSize[4], -symbolSize[4] * 0.5, &pts[1]);
      AddLineToGroup(group, pts[0], pts[1]);
      beginSection.ProjPtFrom_xy(symbolSize[4], -symbolSize[4] * 0.5, &symbolBeginPoint);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[4], symbolSize[4] * 0.5, &symbolEndPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      beginSection.ProjPtFrom_xy(symbolSize[4], -symbolSize[4] * 0.3, &pts[0]);
      double radius = EoGePoint3d::Distance(symbolBeginPoint, pts[0]);
      AddCircleToGroup(group, symbolBeginPoint, radius);
    } break;

    case 5: {  // non-slam check valve
      endSection.ProjPtFrom_xy(symbolSize[5], symbolSize[5] * 0.5, &pts[0]);
      endSection.ProjPtFrom_xy(symbolSize[5], -symbolSize[5] * 0.5, &pts[1]);
      AddLineToGroup(group, pts[0], pts[1]);
      beginSection.ProjPtFrom_xy(symbolSize[5], -symbolSize[5] * 0.5, &symbolBeginPoint);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[5], symbolSize[5] * 0.5, &symbolEndPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      beginSection.ProjPtFrom_xy(symbolSize[5], -symbolSize[5] * 0.3, &pts[0]);
      double radius = EoGePoint3d::Distance(symbolBeginPoint, pts[0]);
      AddCircleToGroup(group, symbolBeginPoint, radius);
      AddLineToGroup(group, symbolEndPoint, pointOnSection);
    } break;

    case 6:  // gate valve
      CreateGateValve(group, beginSection, endSection, symbolSize[6]);
      break;

    case 7: {  // globe valve
      CreateGateValve(group, beginSection, endSection, symbolSize[7]);
      pts[0] = pointOnSection.ProjectToward(end, symbolSize[7] * 0.25);
      double radius = EoGePoint3d::Distance(pointOnSection, pts[0]);
      AddCircleToGroup(group, pointOnSection, radius);
    } break;

    case 8: {  // OS&Y gate valve
      CreateGateValve(group, beginSection, endSection, symbolSize[8]);
      pts[0] = pointOnSection.ProjectToward(end, symbolSize[8] * 0.25);
      double radius = EoGePoint3d::Distance(pointOnSection, pts[0]);
      AddCircleToGroup(group, pointOnSection, radius);
      endSection.ProjPtFrom_xy(0.0, symbolSize[8], &pts[0]);
      AddLineToGroup(group, pointOnSection, pts[0]);
      m_PipeTicSize = symbolSize[8] * 0.25;
      GenerateTickMark(pointOnSection, pts[0], symbolSize[8] * 0.75, group);
    } break;

    case 9: {  // pressure reducing valve
      CreateGateValve(group, beginSection, endSection, symbolSize[9]);
      pts[0] = pointOnSection.ProjectToward(end, symbolSize[9] * 0.25);
      double radius = EoGePoint3d::Distance(pointOnSection, pts[0]);
      AddCircleToGroup(group, pointOnSection, radius);
      endSection.ProjPtFrom_xy(0.0, symbolSize[9], &pts[0]);
      AddLineToGroup(group, pointOnSection, pts[0]);
      endSection.ProjPtFrom_xy(symbolSize[9] * 0.5, symbolSize[9] * 0.75, &pts[1]);
      AddLineToGroup(group, pts[0], pts[1]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[9] * 0.5, &symbolBeginPoint);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
    } break;

    case 10:  // auto 2-way valve
      CreateGateValve(group, beginSection, endSection, symbolSize[10]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[10] * 0.5, &pts[0]);
      AddLineToGroup(group, pointOnSection, pts[0]);

      endSection.ProjPtFrom_xy(symbolSize[10] * 0.25, symbolSize[10] * 0.5, &pts[0]);
      endSection.ProjPtFrom_xy(symbolSize[10] * 0.25, symbolSize[10] * 0.75, &pts[1]);
      beginSection.ProjPtFrom_xy(symbolSize[10] * 0.25, -symbolSize[10] * 0.75, &symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[10] * 0.25, -symbolSize[10] * 0.5, &symbolEndPoint);
      AddLineToGroup(group, pts[0], pts[1]);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      AddLineToGroup(group, symbolEndPoint, pts[0]);
      break;

    case 11:  // auto 3-way valve
      CreateGateValve(group, beginSection, endSection, symbolSize[11]);

      endSection.ProjPtFrom_xy(0.0, symbolSize[11] * 0.5, &pts[0]);
      AddLineToGroup(group, pointOnSection, pts[0]);
      endSection.ProjPtFrom_xy(symbolSize[11] * 0.25, symbolSize[11] * 0.5, &pts[0]);
      endSection.ProjPtFrom_xy(symbolSize[11] * 0.25, symbolSize[11] * 0.75, &pts[1]);
      beginSection.ProjPtFrom_xy(symbolSize[11] * 0.25, -symbolSize[11] * 0.75, &symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[11] * 0.25, -symbolSize[11] * 0.5, &symbolEndPoint);
      AddLineToGroup(group, pts[0], pts[1]);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      AddLineToGroup(group, symbolEndPoint, pts[0]);
      endSection.ProjPtFrom_xy(symbolSize[11] * 0.5, -symbolSize[11], &pts[0]);
      beginSection.ProjPtFrom_xy(symbolSize[11] * 0.5, symbolSize[11], &pts[1]);
      AddLineToGroup(group, pointOnSection, pts[0]);
      AddLineToGroup(group, pts[0], pts[1]);
      AddLineToGroup(group, pts[1], pointOnSection);
      break;

    case 12:  // self operated valve
      CreateGateValve(group, beginSection, endSection, symbolSize[12]);

      endSection.ProjPtFrom_xy(0.0, symbolSize[12] * 0.5, &pts[0]);
      AddLineToGroup(group, pointOnSection, pts[0]);
      endSection.ProjPtFrom_xy(symbolSize[12] * 0.25, symbolSize[12] * 0.5, &pts[1]);
      beginSection.ProjPtFrom_xy(symbolSize[12] * 0.25, -symbolSize[12] * 0.5, &symbolBeginPoint);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
      // add a half circle here i think
      beginSection.ProjPtFrom_xy(symbolSize[12] * 1.25, -symbolSize[12] * 0.5, &pts[0]);
      AddLineToGroup(group, symbolBeginPoint, pts[0]);
      beginSection.ProjPtFrom_xy(symbolSize[12] * 1.25, -symbolSize[12] * 0.75, &pts[1]);
      beginSection.ProjPtFrom_xy(symbolSize[12] * 2.0, -symbolSize[12] * 0.75, &symbolBeginPoint);
      beginSection.ProjPtFrom_xy(symbolSize[12] * 2.0, -symbolSize[12] * 0.5, &symbolEndPoint);
      AddLineToGroup(group, pts[0], pts[1]);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      AddLineToGroup(group, symbolEndPoint, pts[0]);
      break;

    case 13:  // plug valve
      endSection.ProjPtFrom_xy(0.0, -symbolSize[13], &pts[0]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[13], &pts[1]);
      AddLineToGroup(group, symbolEndPoint, pts[1]);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, pts[0]);
      AddLineToGroup(group, pts[0], symbolEndPoint);
      break;

    case 14:  // balancing cock
      endSection.ProjPtFrom_xy(0.0, -symbolSize[14], &pts[0]);
      endSection.ProjPtFrom_xy(0.0, symbolSize[14], &pts[1]);
      AddLineToGroup(group, symbolEndPoint, pts[1]);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, pts[0]);
      AddLineToGroup(group, pts[0], symbolEndPoint);
      AddLineToGroup(group, pts[0], pts[1]);
      break;

    case 15:  // gauge cock
      endSection.ProjPtFrom_xy(0.0, -0.250, &pts[0]);
      AddLineToGroup(group, pointOnSection, pts[0]);
      GenerateTickMark(pointOnSection, pts[0], tickDistance[15], group);
      beginSection.ProjPtFrom_xy(0.0625, 0.1875, &pts[1]);
      endSection.ProjPtFrom_xy(0.0625, -0.1875, &symbolBeginPoint);
      endSection.ProjPtFrom_xy(0.0625, -0.125, &symbolEndPoint);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      break;

    case 16: {  // gauge cock with gauge
      endSection.ProjPtFrom_xy(0.0, -0.250, &pts[0]);
      AddLineToGroup(group, pointOnSection, pts[0]);
      GenerateTickMark(pointOnSection, pts[0], tickDistance[16], group);
      beginSection.ProjPtFrom_xy(0.0625, 0.1875, &pts[1]);
      endSection.ProjPtFrom_xy(0.0625, -0.1875, &symbolBeginPoint);
      endSection.ProjPtFrom_xy(0.0625, -0.125, &symbolEndPoint);
      AddLineToGroup(group, pts[1], symbolBeginPoint);
      AddLineToGroup(group, symbolBeginPoint, symbolEndPoint);
      pts[1] = pointOnSection.ProjectToward(pts[0], 0.28125);
      double radius = EoGePoint3d::Distance(pts[1], pts[0]);
      AddCircleToGroup(group, pts[1], radius);
    } break;

    case 17:  // union
      m_PipeTicSize = symbolSize[17];
      GenerateTickMark(pointOnSection, begin, symbolSize[17], group);
      GenerateTickMark(pointOnSection, end, symbolSize[17], group);
      m_PipeTicSize = m_PipeTicSize * 2.0;
      GenerateTickMark(pointOnSection, begin, 0.0, group);
      break;
  }
  m_PipeTicSize = ticSize;
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

void AeSysView::OnPipeModeWye() {
  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  if (pts.IsEmpty()) {
    pts.Add(cursorPosition);
    m_PreviousOp = ModeLineHighlightOp(ID_OP9);
    return;
  }
  EoDbLine* horizontalSection{};
  auto* group = SelectLineUsingPoint(cursorPosition, horizontalSection);
  if (group == nullptr) { return; }
  EoGePoint3d pointOnSection = horizontalSection->ProjectPointToLine(cursorPosition);
  EoGePoint3d beginPointProjectedToSection = horizontalSection->ProjectPointToLine(pts[0]);
  double distanceToSection = EoGeVector3d(pts[0], beginPointProjectedToSection).Length();

  if (distanceToSection >= 0.25) {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
    const EoGePoint3d begin = horizontalSection->Begin();
    const EoGePoint3d endPoint = horizontalSection->End();

    double distanceBetweenSectionPoints = EoGeVector3d(beginPointProjectedToSection, pointOnSection).Length();

    if (std::abs(distanceBetweenSectionPoints - distanceToSection) <= 0.25) {
      // Just need to shift point on section and do a single 45 degree line
      pointOnSection = beginPointProjectedToSection.ProjectToward(pointOnSection, distanceToSection);
      horizontalSection->SetEndPoint(pointOnSection);
      group = new EoDbGroup(EoDbLine::CreateLine(pointOnSection, endPoint)
              ->WithProperties(
                  horizontalSection->Color(), horizontalSection->LineTypeName(), horizontalSection->LineWeight()));
      document->AddWorkLayerGroup(group);

      group = new EoDbGroup;
      GenerateTickMark(pointOnSection, begin, m_PipeRiseDropRadius, group);
      GenerateTickMark(pointOnSection, endPoint, m_PipeRiseDropRadius, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      group = new EoDbGroup;
      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, pointOnSection, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    } else {
      EoGePoint3d pointAtBend;

      if (distanceBetweenSectionPoints - 0.25 <= distanceToSection) {
        const double d3 = (distanceBetweenSectionPoints > 0.25) ? distanceBetweenSectionPoints : 0.125;
        pointAtBend = beginPointProjectedToSection.ProjectToward(pts[0], d3);
        pointOnSection = beginPointProjectedToSection.ProjectToward(pointOnSection, d3);
      } else {
        pointAtBend = beginPointProjectedToSection.ProjectToward(
            pointOnSection, distanceBetweenSectionPoints - distanceToSection);
        pointAtBend = pts[0] + EoGeVector3d(beginPointProjectedToSection, pointAtBend);
      }
      horizontalSection->SetEndPoint(pointOnSection);

      group = new EoDbGroup;
      GenerateTickMark(pointOnSection, begin, m_PipeRiseDropRadius, group);
      GenerateTickMark(pointOnSection, endPoint, m_PipeRiseDropRadius, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      group = new EoDbGroup(EoDbLine::CreateLine(pointOnSection, endPoint)
              ->WithProperties(
                  horizontalSection->Color(), horizontalSection->LineTypeName(), horizontalSection->LineWeight()));
      document->AddWorkLayerGroup(group);
      group = new EoDbGroup;
      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, pointAtBend, group);
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
  pts.RemoveAll();

  ModeLineUnhighlightOp(m_PreviousOp);
  m_PreviousOp = 0;
}

void AeSysView::DoPipeModeMouseMove() {
  const EoDbHandleSuppressionScope suppressHandles;
  auto cursorPosition = GetCursorPosition();
  const auto numberOfPoints = pts.GetSize();
  if (numberOfPoints == 0) { return; }

  switch (m_PreviousOp) {
    case ID_OP2:
      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        pts.Add(cursorPosition);

        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP2, cursorPosition, &m_PreviewGroup);
        InvalidateOverlay();
      }
      break;

    case ID_OP3:
      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        pts.Add(cursorPosition);

        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, cursorPosition, &m_PreviewGroup);
        InvalidateOverlay();
      }
      break;

    case ID_OP4:
    case ID_OP5:
    case ID_OP9: {
      cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
      pts.Add(cursorPosition);

      m_PreviewGroup.DeletePrimitivesAndRemoveAll();

      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, cursorPosition, &m_PreviewGroup);
      InvalidateOverlay();
      break;
    }
  }
  pts.SetSize(numberOfPoints);
}

void AeSysView::GenerateLineWithFittings(int beginType,
    const EoGePoint3d& begin,
    int endType,
    const EoGePoint3d& end,
    EoDbGroup* group) {
  EoGePoint3d pt1 = begin;
  EoGePoint3d pt2 = end;

  if (beginType == ID_OP3) {
    // Previous fitting is an elbow or side tee
    GenerateTickMark(begin, end, m_PipeRiseDropRadius, group);
  } else if (beginType == ID_OP4) {  // Previous fitting is an elbow down, riser down or bottom tee
    pt1 = begin.ProjectToward(end, m_PipeRiseDropRadius);
    GenerateTickMark(pt1, end, m_PipeRiseDropRadius, group);
  } else if (beginType == ID_OP5) {
    // Previous fitting is an elbow up, riser up or top tee
    GenerateTickMark(begin, end, 2.0 * m_PipeRiseDropRadius, group);
  }

  if (endType == ID_OP3) {
    // Current fitting is an elbow or side tee
    GenerateTickMark(end, begin, m_PipeRiseDropRadius, group);
  } else if (endType == ID_OP4) {
    // Current fitting is an elbow down, riser down or bottom tee
    GenerateTickMark(end, begin, 2.0 * m_PipeRiseDropRadius, group);
  } else if (endType == ID_OP5) {  // Current fitting is an elbow up, riser up or top tee
    pt2 = end.ProjectToward(begin, m_PipeRiseDropRadius);
    GenerateTickMark(end, begin, 2.0 * m_PipeRiseDropRadius, group);
  }
  group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(Gs::renderState));
}

void AeSysView::DropIntoOrRiseFromHorizontalSection(const EoGePoint3d& point, EoDbGroup* group, EoDbLine* section) {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, section);

  const EoGePoint3d begin = section->Begin();
  const EoGePoint3d end = section->End();

  auto cutPoint = point.ProjectToward(begin, m_PipeRiseDropRadius);
  section->SetEndPoint(cutPoint);
  cutPoint = point.ProjectToward(end, m_PipeRiseDropRadius);
  group->AddTail(EoDbLine::CreateLine(cutPoint, end)
          ->WithProperties(section->Color(), section->LineTypeName(), section->LineWeight()));
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

  group = new EoDbGroup;
  GenerateTickMark(point, begin, 2.0 * m_PipeRiseDropRadius, group);

  auto* circle = EoDbConic::CreateCircleInView(point, m_PipeRiseDropRadius)->WithProperties(1, L"CONTINUOUS");

  group->AddTail(circle);
  GenerateTickMark(point, end, 2.0 * m_PipeRiseDropRadius, group);
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
  GenerateTickMark(point, begin, 2.0 * m_PipeRiseDropRadius, group);

  auto* circle = EoDbConic::CreateCircleInView(point, m_PipeRiseDropRadius)->WithProperties(1, L"CONTINUOUS");
  group->AddTail(circle);

  GenerateTickMark(point, end, 2.0 * m_PipeRiseDropRadius, group);
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

bool AeSysView::GenerateTickMark(const EoGePoint3d& begin,
    const EoGePoint3d& end,
    double distance,
    EoDbGroup* group) const {
  const auto pointOnLine = begin.ProjectToward(end, distance);

  EoGeVector3d projection(pointOnLine, end);

  const double distanceToEndPoint = projection.Length();
  const bool markGenerated = distanceToEndPoint > Eo::geometricTolerance;
  if (markGenerated) {
    projection *= m_PipeTicSize / distanceToEndPoint;

    EoGePoint3d pt1(pointOnLine);
    pt1 += EoGeVector3d(projection.y, -projection.x, 0.0);

    EoGePoint3d pt2(pointOnLine);
    pt2 += EoGeVector3d(-projection.y, projection.x, 0.0);
    group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(1, L"CONTINUOUS"));
  }
  return markGenerated;
}
