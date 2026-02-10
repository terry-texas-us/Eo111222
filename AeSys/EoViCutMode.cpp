#include "Stdafx.h"

#include <algorithm>

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGsRenderState.h"
#include "Resource.h"

namespace {
EoUInt16 previousKeyDown{};
EoGePoint3d previousPosition{};
}  // namespace

void AeSysView::OnCutModeOptions() {}

void AeSysView::OnCutModeTorch() {
  auto* document = GetDocument();

  auto cursorPosition = GetCursorPosition();
  auto* groups = new EoDbGroupList;

  EoGePoint3d ptCut;

  EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);

    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);

      if (primitive->SelectUsingPoint(this, ptView, ptCut)) {  // Pick point is within tolerance of primative
        auto* newGroup = new EoDbGroup;

        ptCut = transformMatrix * ptCut;
        document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, primitive);
        primitive->CutAtPoint(ptCut, newGroup);
        document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, primitive);
        groups->AddTail(newGroup);
        break;
      }
    }
  }
  document->AddWorkLayerGroups(groups);
  document->UpdateAllViews(nullptr, EoDb::kGroupsSafe, groups);
  delete groups;
}

void AeSysView::OnCutModeSlice() {
  auto cursorPosition = GetCursorPosition();
  if (previousKeyDown != ID_OP2) {
    previousPosition = cursorPosition;
    RubberBandingStartAtEnable(cursorPosition, Lines);
    previousKeyDown = ModeLineHighlightOp(ID_OP2);
  } else {
    EoGePoint3d pt1 = previousPosition;
    EoGePoint3d pt2 = cursorPosition;

    auto* document = GetDocument();

    EoDbGroupList* groups = new EoDbGroupList;

    EoGeLine ln;
    EoGePoint3dArray intersections;

    EoGePoint4d ptView[] = {EoGePoint4d(pt1), EoGePoint4d(pt2)};
    ModelViewTransformPoints(2, ptView);

    EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

    auto GroupPosition = GetFirstVisibleGroupPosition();
    while (GroupPosition != nullptr) {
      auto* group = GetNextVisibleGroup(GroupPosition);

      if (document->FindTrappedGroup(group) != nullptr) { continue; }

      auto PrimitivePosition = group->GetHeadPosition();
      while (PrimitivePosition != nullptr) {
        auto* primitive = group->GetNext(PrimitivePosition);

        ln = EoGeLine(ptView[0], ptView[1]);
        primitive->SelectUsingLine(this, ln, intersections);
        for (EoUInt16 w = 0; w < intersections.GetSize(); w++) {
          EoDbGroup* NewGroup = new EoDbGroup;

          intersections[w] = transformMatrix * intersections[w];

          document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, primitive);
          primitive->CutAtPoint(intersections[w], NewGroup);
          document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, primitive);
          groups->AddTail(NewGroup);
        }
      }
    }
    document->AddWorkLayerGroups(groups);
    document->UpdateAllViews(nullptr, EoDb::kGroupsSafe, groups);
    delete groups;

    RubberBandingDisable();
    ModeLineUnhighlightOp(previousKeyDown);
  }
}

void AeSysView::OnCutModeField() {
  auto cursorPosition = GetCursorPosition();
  if (previousKeyDown != ID_OP4) {
    previousPosition = cursorPosition;
    RubberBandingStartAtEnable(cursorPosition, Rectangles);
    previousKeyDown = ModeLineHighlightOp(ID_OP4);
  } else {
    EoGePoint3d rLL, rUR;

    rLL.x = std::min(previousPosition.x, cursorPosition.x);
    rLL.y = std::min(previousPosition.y, cursorPosition.y);
    rUR.x = std::max(previousPosition.x, cursorPosition.x);
    rUR.y = std::max(previousPosition.y, cursorPosition.y);

    EoGePoint3d lowerLeft = rLL;
    EoGePoint3d upperRight = rUR;

    EoDbGroup* group{};
    EoDbPrimitive* primitive{};

    int iInts{};
    EoGePoint3d ptInt[10]{};

    auto* document = GetDocument();

    auto color = pstate.Color();
    auto lineType = pstate.LineType();

    EoDbGroupList* GroupsOut = new EoDbGroupList;
    EoDbGroupList* GroupsIn = new EoDbGroupList;

    POSITION posSeg, posSegPrv;
    for (posSeg = GetFirstVisibleGroupPosition(); (posSegPrv = posSeg) != 0;) {
      group = GetNextVisibleGroup(posSeg);

      if (document->FindTrappedGroup(group) != nullptr) { continue; }

      POSITION PrimitivePosition, posPrimPrv;
      for (PrimitivePosition = group->GetHeadPosition(); (posPrimPrv = PrimitivePosition) != 0;) {
        primitive = group->GetNext(PrimitivePosition);

        if ((iInts = primitive->IsWithinArea(lowerLeft, upperRight, ptInt)) == 0) { continue; }

        group->RemoveAt(posPrimPrv);

        for (int i = 0; i < iInts; i += 2) {
          if (i != 0) GroupsOut->RemoveTail();
          primitive->CutAt2Points(ptInt[i], ptInt[i + 1], GroupsOut, GroupsIn);
        }
      }
      if (group->IsEmpty()) {  // seg was emptied remove from lists
        document->AnyLayerRemove(group);
        document->RemoveGroupFromAllViews(group);
        group->DeletePrimitivesAndRemoveAll();
        delete group;
      }
    }

    if (GroupsOut->GetCount() > 0) {
      document->AddWorkLayerGroups(GroupsOut);
      document->UpdateAllViews(nullptr, EoDb::kGroups, GroupsOut);
    }
    if (GroupsIn->GetCount() > 0) {
      document->AddWorkLayerGroups(GroupsIn);
      document->AddGroupsToTrap(GroupsIn);
    }
    delete GroupsIn;
    delete GroupsOut;

    auto* deviceContext = GetDC();
    if (!deviceContext) { return; }
    pstate.SetPen(this, deviceContext, color, lineType);
    ReleaseDC(deviceContext);

    UpdateStateInformation(BothCounts);
    RubberBandingDisable();
    ModeLineUnhighlightOp(previousKeyDown);
  }
}

void AeSysView::OnCutModeClip() {
  auto cursorPosition = GetCursorPosition();
  if (previousKeyDown != ID_OP7) {
    previousPosition = cursorPosition;
    previousKeyDown = ModeLineHighlightOp(ID_OP7);
  } else {
    EoGePoint3d pt1 = previousPosition;
    EoGePoint3d pt2 = cursorPosition;

    if (pt1 == pt2) { return; }

    double dRel[2]{};
    EoGePoint3d ptCut[2]{};

    EoInt16 color = pstate.Color();
    EoInt16 LineType = pstate.LineType();

    auto* document = GetDocument();

    EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

    EoGePoint4d ptView[] = {EoGePoint4d(pt1), EoGePoint4d(pt2)};

    ModelViewTransformPoints(2, ptView);

    EoDbGroupList* GroupsOut = new EoDbGroupList;
    EoDbGroupList* GroupsIn = new EoDbGroupList;

    POSITION posSeg;
    POSITION posSegPrv;

    for (posSeg = GetFirstVisibleGroupPosition(); (posSegPrv = posSeg) != 0;) {
      auto* group = GetNextVisibleGroup(posSeg);

      if (document->FindTrappedGroup(group) != nullptr) { continue; }

      POSITION posPrim1;
      POSITION posPrim2;

      for (posPrim1 = group->GetHeadPosition(); (posPrim2 = posPrim1) != 0;) {
        EoDbPrimitive* primitive = group->GetNext(posPrim1);

        if (!primitive->SelectUsingPoint(this, ptView[0], ptCut[0])) continue;
        dRel[0] = EoDbPrimitive::Rel();
        if (!primitive->SelectUsingPoint(this, ptView[1], ptCut[1])) continue;
        dRel[1] = EoDbPrimitive::Rel();
        // Both pick points are within tolerance of primative
        ptCut[0] = transformMatrix * ptCut[0];
        ptCut[1] = transformMatrix * ptCut[1];
        if (dRel[0] > dRel[1]) {
          EoGePoint3d ptTmp = ptCut[0];
          ptCut[0] = ptCut[1];
          ptCut[1] = ptTmp;
        }
        group->RemoveAt(posPrim2);

        primitive->CutAt2Points(ptCut[0], ptCut[1], GroupsOut, GroupsIn);
      }
      if (group->IsEmpty()) {  // seg was emptied remove from lists
        document->AnyLayerRemove(group);
        document->RemoveGroupFromAllViews(group);
        group->DeletePrimitivesAndRemoveAll();
        delete group;
      }
    }
    if (GroupsOut->GetCount() > 0) {
      document->AddWorkLayerGroups(GroupsOut);
      document->UpdateAllViews(nullptr, EoDb::kGroups, GroupsOut);
    }
    if (GroupsIn->GetCount() > 0) {
      document->AddWorkLayerGroups(GroupsIn);
      document->AddGroupsToTrap(GroupsIn);
      document->UpdateAllViews(nullptr, EoDb::kGroupsTrap, GroupsIn);
    }
    delete GroupsIn;
    delete GroupsOut;

    auto* deviceContext = GetDC();
    if (!deviceContext) { return; }
    pstate.SetPen(this, deviceContext, color, LineType);
    ReleaseDC(deviceContext);

    UpdateStateInformation(BothCounts);
    ModeLineUnhighlightOp(previousKeyDown);
  }
}

void AeSysView::OnCutModeDivide() {}
void AeSysView::OnCutModeReturn() {
  RubberBandingDisable();
  ModeLineUnhighlightOp(previousKeyDown);
}
void AeSysView::OnCutModeEscape() {
  RubberBandingDisable();
  ModeLineUnhighlightOp(previousKeyDown);
}
