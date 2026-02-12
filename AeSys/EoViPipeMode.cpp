#include "Stdafx.h"

#include <cmath>
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
constexpr double symbolSize[] = {0.09375, 0.09375, 0.09375, 0.09375, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125,
    0.125, 0.125, 0.125, 0.125, 0.0, 0.0, 0.09375};
constexpr double tickDistance[] = {0.125, 0.125, 0.125, 0.125, 0.15625, 0.15625, 0.15625, 0.15625, 0.15625, 0.15625,
    0.15625, 0.15625, 0.15625, 0.15625, 0.15625, 0.03125, 0.03125, 0.125};

void AddLineToGroup(EoDbGroup* group, const EoGePoint3d& begin, const EoGePoint3d& end) {
  group->AddTail(EoDbLine::CreateLine(begin, end)->WithProperties(renderState.Color(), renderState.LineTypeIndex()));
}

void AddCircleToGroup(EoDbGroup* group, const EoGePoint3d& center, double radius) {
  group->AddTail(
      EoDbConic::CreateCircleInView(center, radius)->WithProperties(renderState.Color(), renderState.LineTypeIndex()));
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
  EoDlgPipeOptions Dialog;
  Dialog.m_PipeTicSize = m_PipeTicSize;
  Dialog.m_PipeRiseDropRadius = m_PipeRiseDropRadius;
  if (Dialog.DoModal() == IDOK) {
    m_PipeTicSize = Dialog.m_PipeTicSize;
    m_PipeRiseDropRadius = Dialog.m_PipeRiseDropRadius;
  }
}

void AeSysView::OnPipeModeLine() {
  auto cursorPosition = GetCursorPosition();

  if (pts.IsEmpty()) {
    pts.Add(cursorPosition);
  } else {
    cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
    auto* document = GetDocument();

    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();

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
    EoGePoint3d begin = horizontalSection->Begin();
    EoGePoint3d end = horizontalSection->End();
    if (!pts.IsEmpty()) { cursorPosition = SnapPointToAxis(pts[0], cursorPosition); }
    cursorPosition = horizontalSection->ProjectPointToLine(cursorPosition);
    horizontalSection->SetEndPoint(cursorPosition);
    group->AddTail(EoDbLine::CreateLine(cursorPosition, end)
            ->WithProperties(horizontalSection->Color(), horizontalSection->LineTypeIndex()));

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
    EoDbConic* VerticalSection{};
    group = SelectCircleUsingPoint(cursorPosition, m_PipeRiseDropRadius, VerticalSection);
    if (group != nullptr) {
      cursorPosition = VerticalSection->Center();

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

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

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
    EoDbConic* VerticalSection{};
    group = SelectCircleUsingPoint(cursorPosition, m_PipeRiseDropRadius, VerticalSection);
    if (group != nullptr) {  // On an existing vertical pipe section
      cursorPosition = VerticalSection->Center();
      if (pts.IsEmpty()) {
        pts.Add(cursorPosition);
        m_PreviousOp = ModeLineHighlightOp(ID_OP4);
      } else {  // Rising into an existing vertical pipe section
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP5, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        OnPipeModeEscape();
      }
    } else {
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();

      if (pts.IsEmpty()) {
        pts.Add(cursorPosition);
      } else {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP5, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      }
      auto* circle = EoDbConic::CreateCircleInView(cursorPosition, m_PipeRiseDropRadius)->WithProperties(1, 1);
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
    EoDbConic* VerticalSection{};
    group = SelectCircleUsingPoint(cursorPosition, m_PipeRiseDropRadius, VerticalSection);
    if (group != nullptr) {  // On an existing vertical pipe section
      cursorPosition = VerticalSection->Center();
      if (pts.IsEmpty()) {
        pts.Add(cursorPosition);
        m_PreviousOp = ModeLineHighlightOp(ID_OP5);
      } else {  // Dropping into an existing vertical pipe section
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP4, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        OnPipeModeEscape();
      }
    } else {
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();

      if (pts.IsEmpty()) {
        pts.Add(cursorPosition);
      } else {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);

        group = new EoDbGroup;
        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP4, cursorPosition, group);
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      }
      auto* circle = EoDbConic::CreateCircleInView(cursorPosition, m_PipeRiseDropRadius)->WithProperties(1, 1);

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

  EoDlgPipeSymbol Dialog;
  Dialog.m_CurrentPipeSymbolIndex = m_CurrentPipeSymbolIndex;
  if (Dialog.DoModal() == IDOK) { m_CurrentPipeSymbolIndex = Dialog.m_CurrentPipeSymbolIndex; }
  EoGePoint3d begin = horizontalSection->Begin();
  EoGePoint3d end = horizontalSection->End();
  EoGePoint3d PointOnSection = horizontalSection->ProjectPointToLine(cursorPosition);

  EoGeLine BeginSection(PointOnSection, begin);
  EoGeLine EndSection(PointOnSection, end);

  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, horizontalSection);

  EoGePoint3d SymbolBeginPoint = PointOnSection.ProjectToward(begin, symbolSize[m_CurrentPipeSymbolIndex]);
  EoGePoint3d SymbolEndPoint = PointOnSection.ProjectToward(end, symbolSize[m_CurrentPipeSymbolIndex]);
  double TicSize = m_PipeTicSize;

  horizontalSection->SetEndPoint(SymbolBeginPoint);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, horizontalSection);
  group = new EoDbGroup(EoDbLine::CreateLine(SymbolEndPoint, end)
          ->WithProperties(horizontalSection->Color(), horizontalSection->LineTypeIndex()));
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

  group = new EoDbGroup;
  GenerateTickMark(PointOnSection, begin, tickDistance[m_CurrentPipeSymbolIndex], group);
  GenerateTickMark(PointOnSection, end, tickDistance[m_CurrentPipeSymbolIndex], group);

  switch (m_CurrentPipeSymbolIndex) {
    case 0: {  // flow switch
      double radius = EoGePoint3d::Distance(PointOnSection, SymbolBeginPoint);
      AddCircleToGroup(group, PointOnSection, radius);
      EndSection.ProjPtFrom_xy(symbolSize[0], -symbolSize[0] * 1.5, &pts[0]);
      EndSection.ProjPtFrom_xy(symbolSize[0], -symbolSize[0] * 2.0, &pts[1]);
      BeginSection.ProjPtFrom_xy(symbolSize[0], symbolSize[0] * 1.5, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(symbolSize[0], symbolSize[0] * 2.0, &SymbolEndPoint);
      AddLineToGroup(group, pts[1], SymbolEndPoint);
      AddLineToGroup(group, SymbolEndPoint, SymbolBeginPoint);
      AddLineToGroup(group, SymbolBeginPoint, pts[0]);
      AddLineToGroup(group, pts[0], pts[1]);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
    } break;

    case 1: {  // float and thermostatic trap
      double radius = EoGePoint3d::Distance(PointOnSection, SymbolBeginPoint);
      AddCircleToGroup(group, PointOnSection, radius);
      pts[0] = SymbolBeginPoint.RotateAboutAxis(PointOnSection, EoGeVector3d::positiveUnitZ, Eo::QuarterPi);
      pts[1] = pts[0].RotateAboutAxis(PointOnSection, EoGeVector3d::positiveUnitZ, Eo::Pi);
      AddLineToGroup(group, pts[0], pts[1]);
      pts[0] = SymbolBeginPoint.RotateAboutAxis(PointOnSection, EoGeVector3d::positiveUnitZ, Eo::ThreeQuartersPi);
      pts[1] = pts[0].RotateAboutAxis(PointOnSection, EoGeVector3d::positiveUnitZ, Eo::Pi);
      AddLineToGroup(group, pts[0], pts[1]);
    } break;

    case 2: {  // ball valve
      double radius = EoGePoint3d::Distance(PointOnSection, SymbolBeginPoint);
      AddCircleToGroup(group, PointOnSection, radius);
      EndSection.ProjPtFrom_xy(symbolSize[2], symbolSize[2] * 1.5, &pts[0]);
      EndSection.ProjPtFrom_xy(0.0, symbolSize[2] * 1.5, &pts[1]);
      AddLineToGroup(group, pts[0], pts[1]);
      EndSection.ProjPtFrom_xy(0.0, symbolSize[2], &SymbolBeginPoint);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
    } break;

    case 3: {  // butterfly
      double radius = EoGePoint3d::Distance(PointOnSection, SymbolBeginPoint);
      AddCircleToGroup(group, PointOnSection, radius);
      EndSection.ProjPtFrom_xy(symbolSize[3], symbolSize[3] * 1.5, &pts[0]);
      EndSection.ProjPtFrom_xy(0.0, symbolSize[3] * 1.5, &pts[1]);
      AddLineToGroup(group, pts[0], pts[1]);
      BeginSection.ProjPtFrom_xy(0.0, symbolSize[3], &SymbolBeginPoint);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
    } break;

    case 4: {  // check valve
      EndSection.ProjPtFrom_xy(symbolSize[4], symbolSize[4] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(symbolSize[4], -symbolSize[4] * 0.5, &pts[1]);
      AddLineToGroup(group, pts[0], pts[1]);
      BeginSection.ProjPtFrom_xy(symbolSize[4], -symbolSize[4] * 0.5, &SymbolBeginPoint);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(symbolSize[4], symbolSize[4] * 0.5, &SymbolEndPoint);
      AddLineToGroup(group, SymbolBeginPoint, SymbolEndPoint);
      BeginSection.ProjPtFrom_xy(symbolSize[4], -symbolSize[4] * 0.3, &pts[0]);
      double radius = EoGePoint3d::Distance(SymbolBeginPoint, pts[0]);
      AddCircleToGroup(group, SymbolBeginPoint, radius);
    } break;

    case 5: {  // non-slam check valve
      EndSection.ProjPtFrom_xy(symbolSize[5], symbolSize[5] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(symbolSize[5], -symbolSize[5] * 0.5, &pts[1]);
      AddLineToGroup(group, pts[0], pts[1]);
      BeginSection.ProjPtFrom_xy(symbolSize[5], -symbolSize[5] * 0.5, &SymbolBeginPoint);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(symbolSize[5], symbolSize[5] * 0.5, &SymbolEndPoint);
      AddLineToGroup(group, SymbolBeginPoint, SymbolEndPoint);
      BeginSection.ProjPtFrom_xy(symbolSize[5], -symbolSize[5] * 0.3, &pts[0]);
      double radius = EoGePoint3d::Distance(SymbolBeginPoint, pts[0]);
      AddCircleToGroup(group, SymbolBeginPoint, radius);
      AddLineToGroup(group, SymbolEndPoint, PointOnSection);
    } break;

    case 6:  // gate valve
      CreateGateValve(group, BeginSection, EndSection, symbolSize[6]);
      break;

    case 7: {  // globe valve
      CreateGateValve(group, BeginSection, EndSection, symbolSize[7]);
      pts[0] = PointOnSection.ProjectToward(end, symbolSize[7] * 0.25);
      double radius = EoGePoint3d::Distance(PointOnSection, pts[0]);
      AddCircleToGroup(group, PointOnSection, radius);
    } break;

    case 8: {  // OS&Y gate valve
      CreateGateValve(group, BeginSection, EndSection, symbolSize[8]);
      pts[0] = PointOnSection.ProjectToward(end, symbolSize[8] * 0.25);
      double radius = EoGePoint3d::Distance(PointOnSection, pts[0]);
      AddCircleToGroup(group, PointOnSection, radius);
      EndSection.ProjPtFrom_xy(0.0, symbolSize[8], &pts[0]);
      AddLineToGroup(group, PointOnSection, pts[0]);
      m_PipeTicSize = symbolSize[8] * 0.25;
      GenerateTickMark(PointOnSection, pts[0], symbolSize[8] * 0.75, group);
    } break;

    case 9: {  // pressure reducing valve
      CreateGateValve(group, BeginSection, EndSection, symbolSize[9]);
      pts[0] = PointOnSection.ProjectToward(end, symbolSize[9] * 0.25);
      double radius = EoGePoint3d::Distance(PointOnSection, pts[0]);
      AddCircleToGroup(group, PointOnSection, radius);
      EndSection.ProjPtFrom_xy(0.0, symbolSize[9], &pts[0]);
      AddLineToGroup(group, PointOnSection, pts[0]);
      EndSection.ProjPtFrom_xy(symbolSize[9] * 0.5, symbolSize[9] * 0.75, &pts[1]);
      AddLineToGroup(group, pts[0], pts[1]);
      EndSection.ProjPtFrom_xy(0.0, symbolSize[9] * 0.5, &SymbolBeginPoint);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
    } break;

    case 10:  // auto 2-way valve
      CreateGateValve(group, BeginSection, EndSection, symbolSize[10]);
      EndSection.ProjPtFrom_xy(0.0, symbolSize[10] * 0.5, &pts[0]);
      AddLineToGroup(group, PointOnSection, pts[0]);

      EndSection.ProjPtFrom_xy(symbolSize[10] * 0.25, symbolSize[10] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(symbolSize[10] * 0.25, symbolSize[10] * 0.75, &pts[1]);
      BeginSection.ProjPtFrom_xy(symbolSize[10] * 0.25, -symbolSize[10] * 0.75, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(symbolSize[10] * 0.25, -symbolSize[10] * 0.5, &SymbolEndPoint);
      AddLineToGroup(group, pts[0], pts[1]);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
      AddLineToGroup(group, SymbolBeginPoint, SymbolEndPoint);
      AddLineToGroup(group, SymbolEndPoint, pts[0]);
      break;

    case 11:  // auto 3-way valve
      CreateGateValve(group, BeginSection, EndSection, symbolSize[11]);

      EndSection.ProjPtFrom_xy(0.0, symbolSize[11] * 0.5, &pts[0]);
      AddLineToGroup(group, PointOnSection, pts[0]);
      EndSection.ProjPtFrom_xy(symbolSize[11] * 0.25, symbolSize[11] * 0.5, &pts[0]);
      EndSection.ProjPtFrom_xy(symbolSize[11] * 0.25, symbolSize[11] * 0.75, &pts[1]);
      BeginSection.ProjPtFrom_xy(symbolSize[11] * 0.25, -symbolSize[11] * 0.75, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(symbolSize[11] * 0.25, -symbolSize[11] * 0.5, &SymbolEndPoint);
      AddLineToGroup(group, pts[0], pts[1]);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
      AddLineToGroup(group, SymbolBeginPoint, SymbolEndPoint);
      AddLineToGroup(group, SymbolEndPoint, pts[0]);
      EndSection.ProjPtFrom_xy(symbolSize[11] * 0.5, -symbolSize[11], &pts[0]);
      BeginSection.ProjPtFrom_xy(symbolSize[11] * 0.5, symbolSize[11], &pts[1]);
      AddLineToGroup(group, PointOnSection, pts[0]);
      AddLineToGroup(group, pts[0], pts[1]);
      AddLineToGroup(group, pts[1], PointOnSection);
      break;

    case 12:  // self operated valve
      CreateGateValve(group, BeginSection, EndSection, symbolSize[12]);

      EndSection.ProjPtFrom_xy(0.0, symbolSize[12] * 0.5, &pts[0]);
      AddLineToGroup(group, PointOnSection, pts[0]);
      EndSection.ProjPtFrom_xy(symbolSize[12] * 0.25, symbolSize[12] * 0.5, &pts[1]);
      BeginSection.ProjPtFrom_xy(symbolSize[12] * 0.25, -symbolSize[12] * 0.5, &SymbolBeginPoint);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
      // add a half circle here i think
      BeginSection.ProjPtFrom_xy(symbolSize[12] * 1.25, -symbolSize[12] * 0.5, &pts[0]);
      AddLineToGroup(group, SymbolBeginPoint, pts[0]);
      BeginSection.ProjPtFrom_xy(symbolSize[12] * 1.25, -symbolSize[12] * 0.75, &pts[1]);
      BeginSection.ProjPtFrom_xy(symbolSize[12] * 2.0, -symbolSize[12] * 0.75, &SymbolBeginPoint);
      BeginSection.ProjPtFrom_xy(symbolSize[12] * 2.0, -symbolSize[12] * 0.5, &SymbolEndPoint);
      AddLineToGroup(group, pts[0], pts[1]);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
      AddLineToGroup(group, SymbolBeginPoint, SymbolEndPoint);
      AddLineToGroup(group, SymbolEndPoint, pts[0]);
      break;

    case 13:  // plug valve
      EndSection.ProjPtFrom_xy(0.0, -symbolSize[13], &pts[0]);
      EndSection.ProjPtFrom_xy(0.0, symbolSize[13], &pts[1]);
      AddLineToGroup(group, SymbolEndPoint, pts[1]);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
      AddLineToGroup(group, SymbolBeginPoint, pts[0]);
      AddLineToGroup(group, pts[0], SymbolEndPoint);
      break;

    case 14:  // balancing cock
      EndSection.ProjPtFrom_xy(0.0, -symbolSize[14], &pts[0]);
      EndSection.ProjPtFrom_xy(0.0, symbolSize[14], &pts[1]);
      AddLineToGroup(group, SymbolEndPoint, pts[1]);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
      AddLineToGroup(group, SymbolBeginPoint, pts[0]);
      AddLineToGroup(group, pts[0], SymbolEndPoint);
      AddLineToGroup(group, pts[0], pts[1]);
      break;

    case 15:  // gauge cock
      EndSection.ProjPtFrom_xy(0.0, -0.250, &pts[0]);
      AddLineToGroup(group, PointOnSection, pts[0]);
      GenerateTickMark(PointOnSection, pts[0], tickDistance[15], group);
      BeginSection.ProjPtFrom_xy(0.0625, 0.1875, &pts[1]);
      EndSection.ProjPtFrom_xy(0.0625, -0.1875, &SymbolBeginPoint);
      EndSection.ProjPtFrom_xy(0.0625, -0.125, &SymbolEndPoint);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
      AddLineToGroup(group, SymbolBeginPoint, SymbolEndPoint);
      break;

    case 16: {  // gauge cock with gauge
      EndSection.ProjPtFrom_xy(0.0, -0.250, &pts[0]);
      AddLineToGroup(group, PointOnSection, pts[0]);
      GenerateTickMark(PointOnSection, pts[0], tickDistance[16], group);
      BeginSection.ProjPtFrom_xy(0.0625, 0.1875, &pts[1]);
      EndSection.ProjPtFrom_xy(0.0625, -0.1875, &SymbolBeginPoint);
      EndSection.ProjPtFrom_xy(0.0625, -0.125, &SymbolEndPoint);
      AddLineToGroup(group, pts[1], SymbolBeginPoint);
      AddLineToGroup(group, SymbolBeginPoint, SymbolEndPoint);
      pts[1] = PointOnSection.ProjectToward(pts[0], 0.28125);
      double radius = EoGePoint3d::Distance(pts[1], pts[0]);
      AddCircleToGroup(group, pts[1], radius);
    } break;

    case 17:  // union
      m_PipeTicSize = symbolSize[17];
      GenerateTickMark(PointOnSection, begin, symbolSize[17], group);
      GenerateTickMark(PointOnSection, end, symbolSize[17], group);
      m_PipeTicSize = m_PipeTicSize * 2.0;
      GenerateTickMark(PointOnSection, begin, 0.0, group);
      break;
  }
  m_PipeTicSize = TicSize;
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
  EoGePoint3d PointOnSection = horizontalSection->ProjectPointToLine(cursorPosition);
  EoGePoint3d BeginPointProjectedToSection = horizontalSection->ProjectPointToLine(pts[0]);
  double DistanceToSection = EoGeVector3d(pts[0], BeginPointProjectedToSection).Length();

  if (DistanceToSection >= 0.25) {
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    EoGePoint3d begin = horizontalSection->Begin();
    EoGePoint3d EndPoint = horizontalSection->End();

    double DistanceBetweenSectionPoints = EoGeVector3d(BeginPointProjectedToSection, PointOnSection).Length();

    if (fabs(DistanceBetweenSectionPoints - DistanceToSection) <= 0.25) {
      // Just need to shift point on section and do a single 45 degree line
      PointOnSection = BeginPointProjectedToSection.ProjectToward(PointOnSection, DistanceToSection);
      horizontalSection->SetEndPoint(PointOnSection);
      group = new EoDbGroup(EoDbLine::CreateLine(PointOnSection, EndPoint)
              ->WithProperties(horizontalSection->Color(), horizontalSection->LineTypeIndex()));
      document->AddWorkLayerGroup(group);

      group = new EoDbGroup;
      GenerateTickMark(PointOnSection, begin, m_PipeRiseDropRadius, group);
      GenerateTickMark(PointOnSection, EndPoint, m_PipeRiseDropRadius, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      group = new EoDbGroup;
      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, PointOnSection, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    } else {
      EoGePoint3d PointAtBend;

      if (DistanceBetweenSectionPoints - 0.25 <= DistanceToSection) {
        double d3 = (DistanceBetweenSectionPoints > 0.25) ? DistanceBetweenSectionPoints : 0.125;
        PointAtBend = BeginPointProjectedToSection.ProjectToward(pts[0], d3);
        PointOnSection = BeginPointProjectedToSection.ProjectToward(PointOnSection, d3);
      } else {
        PointAtBend = BeginPointProjectedToSection.ProjectToward(
            PointOnSection, DistanceBetweenSectionPoints - DistanceToSection);
        PointAtBend = pts[0] + EoGeVector3d(BeginPointProjectedToSection, PointAtBend);
      }
      horizontalSection->SetEndPoint(PointOnSection);

      group = new EoDbGroup;
      GenerateTickMark(PointOnSection, begin, m_PipeRiseDropRadius, group);
      GenerateTickMark(PointOnSection, EndPoint, m_PipeRiseDropRadius, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      group = new EoDbGroup(EoDbLine::CreateLine(PointOnSection, EndPoint)
              ->WithProperties(horizontalSection->Color(), horizontalSection->LineTypeIndex()));
      document->AddWorkLayerGroup(group);
      group = new EoDbGroup;
      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, PointAtBend, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      group = new EoDbGroup;
      GenerateLineWithFittings(ID_OP3, PointAtBend, ID_OP3, PointOnSection, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
  }
  OnPipeModeEscape();
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
  auto cursorPosition = GetCursorPosition();
  INT_PTR NumberOfPoints = pts.GetSize();
  if (NumberOfPoints == 0) { return; }

  switch (m_PreviousOp) {
    case ID_OP2:
      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        pts.Add(cursorPosition);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP2, cursorPosition, &m_PreviewGroup);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP3:
      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        pts.Add(cursorPosition);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, cursorPosition, &m_PreviewGroup);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP4:
    case ID_OP5:
    case ID_OP9: {
      cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
      pts.Add(cursorPosition);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();

      GenerateLineWithFittings(m_PreviousOp, pts[0], ID_OP3, cursorPosition, &m_PreviewGroup);
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;
    }
  }
  pts.SetSize(NumberOfPoints);
}

void AeSysView::GenerateLineWithFittings(
    int beginType, const EoGePoint3d& begin, int endType, const EoGePoint3d& end, EoDbGroup* group) {
  EoGePoint3d pt1 = begin;
  EoGePoint3d pt2 = end;

  if (beginType == ID_OP3)
    // Previous fitting is an elbow or side tee
    GenerateTickMark(begin, end, m_PipeRiseDropRadius, group);
  else if (beginType == ID_OP4) {  // Previous fitting is an elbow down, riser down or bottom tee
    pt1 = begin.ProjectToward(end, m_PipeRiseDropRadius);
    GenerateTickMark(pt1, end, m_PipeRiseDropRadius, group);
  } else if (beginType == ID_OP5)
    // Previous fitting is an elbow up, riser up or top tee
    GenerateTickMark(begin, end, 2.0 * m_PipeRiseDropRadius, group);

  if (endType == ID_OP3)
    // Current fitting is an elbow or side tee
    GenerateTickMark(end, begin, m_PipeRiseDropRadius, group);
  else if (endType == ID_OP4)
    // Current fitting is an elbow down, riser down or bottom tee
    GenerateTickMark(end, begin, 2.0 * m_PipeRiseDropRadius, group);
  else if (endType == ID_OP5) {  // Current fitting is an elbow up, riser up or top tee
    pt2 = end.ProjectToward(begin, m_PipeRiseDropRadius);
    GenerateTickMark(end, begin, 2.0 * m_PipeRiseDropRadius, group);
  }
  group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(renderState.Color(), renderState.LineTypeIndex()));
}

void AeSysView::DropIntoOrRiseFromHorizontalSection(const EoGePoint3d& point, EoDbGroup* group, EoDbLine* section) {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, section);

  EoGePoint3d begin = section->Begin();
  EoGePoint3d end = section->End();

  auto cutPoint = point.ProjectToward(begin, m_PipeRiseDropRadius);
  section->SetEndPoint(cutPoint);
  cutPoint = point.ProjectToward(end, m_PipeRiseDropRadius);
  group->AddTail(EoDbLine::CreateLine(cutPoint, end)->WithProperties(section->Color(), section->LineTypeIndex()));
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

  group = new EoDbGroup;
  GenerateTickMark(point, begin, 2.0 * m_PipeRiseDropRadius, group);

  auto* circle = EoDbConic::CreateCircleInView(point, m_PipeRiseDropRadius)->WithProperties(1, 1);

  group->AddTail(circle);
  GenerateTickMark(point, end, 2.0 * m_PipeRiseDropRadius, group);
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

void AeSysView::DropFromOrRiseIntoHorizontalSection(const EoGePoint3d& point, EoDbGroup* group, EoDbLine* section) {
  auto* document = GetDocument();
  EoGePoint3d begin = section->Begin();
  EoGePoint3d end = section->End();

  section->SetEndPoint(point);
  group->AddTail(EoDbLine::CreateLine(point, end)->WithProperties(section->Color(), section->LineTypeIndex()));

  group = new EoDbGroup{};
  GenerateTickMark(point, begin, 2.0 * m_PipeRiseDropRadius, group);

  auto* circle = EoDbConic::CreateCircleInView(point, m_PipeRiseDropRadius)->WithProperties(1, 1);
  group->AddTail(circle);

  GenerateTickMark(point, end, 2.0 * m_PipeRiseDropRadius, group);
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

bool AeSysView::GenerateTickMark(
    const EoGePoint3d& begin, const EoGePoint3d& end, double distance, EoDbGroup* group) const {
  auto pointOnLine = begin.ProjectToward(end, distance);

  EoGeVector3d Projection(pointOnLine, end);

  double DistanceToEndPoint = Projection.Length();

  bool markGenerated = DistanceToEndPoint > Eo::geometricTolerance;
  if (markGenerated) {
    Projection *= m_PipeTicSize / DistanceToEndPoint;

    EoGePoint3d pt1(pointOnLine);
    pt1 += EoGeVector3d(Projection.y, -Projection.x, 0.0);

    EoGePoint3d pt2(pointOnLine);
    pt2 += EoGeVector3d(-Projection.y, Projection.x, 0.0);
    group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(1, 1));
  }
  return markGenerated;
}
