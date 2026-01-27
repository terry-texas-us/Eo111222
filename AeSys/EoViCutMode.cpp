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
#include "PrimState.h"
#include "Resource.h"

namespace {
EoUInt16 previousKeyDown{};
EoGePoint3d previousPosition{};
}  // namespace

void AeSysView::OnCutModeOptions() {}

void AeSysView::OnCutModeTorch() {
  auto* Document = GetDocument();

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
        Document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, primitive);
        primitive->CutAtPt(ptCut, newGroup);
        Document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, primitive);
        groups->AddTail(newGroup);
        break;
      }
    }
  }
  Document->AddWorkLayerGroups(groups);
  Document->UpdateAllViews(nullptr, EoDb::kGroupsSafe, groups);
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

    auto* Document = GetDocument();

    EoDbGroupList* Groups = new EoDbGroupList;

    EoGeLine ln;
    EoGePoint3dArray intersections;

    EoGePoint4d ptView[] = {EoGePoint4d(pt1), EoGePoint4d(pt2)};
    ModelViewTransformPoints(2, ptView);

    EoGeTransformMatrix tm = ModelViewGetMatrixInverse();

    auto GroupPosition = GetFirstVisibleGroupPosition();
    while (GroupPosition != nullptr) {
      auto* Group = GetNextVisibleGroup(GroupPosition);

      if (Document->FindTrappedGroup(Group) != 0) continue;

      auto PrimitivePosition = Group->GetHeadPosition();
      while (PrimitivePosition != nullptr) {
        EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

        ln = EoGeLine(ptView[0], ptView[1]);
        Primitive->SelectUsingLine(this, ln, intersections);
        for (EoUInt16 w = 0; w < intersections.GetSize(); w++) {
          EoDbGroup* NewGroup = new EoDbGroup;

          intersections[w] = tm * intersections[w];

          Document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, Primitive);
          Primitive->CutAtPt(intersections[w], NewGroup);
          Document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, Primitive);
          Groups->AddTail(NewGroup);
        }
      }
    }
    Document->AddWorkLayerGroups(Groups);
    Document->UpdateAllViews(nullptr, EoDb::kGroupsSafe, Groups);
    delete Groups;

    RubberBandingDisable();
    ModeLineUnhighlightOp(previousKeyDown);
  }
}
void AeSysView::OnCutModeField() {
  CDC* DeviceContext = GetDC();
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

    EoGePoint3d ptLL = rLL;
    EoGePoint3d ptUR = rUR;

    EoDbGroup* Group;
    EoDbPrimitive* Primitive;

    int iInts;
    EoGePoint3d ptInt[10]{};

    auto* Document = GetDocument();

    EoInt16 color = pstate.PenColor();
    EoInt16 LineType = pstate.LineType();

    EoDbGroupList* GroupsOut = new EoDbGroupList;
    EoDbGroupList* GroupsIn = new EoDbGroupList;

    POSITION posSeg, posSegPrv;
    for (posSeg = GetFirstVisibleGroupPosition(); (posSegPrv = posSeg) != 0;) {
      Group = GetNextVisibleGroup(posSeg);

      if (Document->FindTrappedGroup(Group) != 0) continue;

      POSITION PrimitivePosition, posPrimPrv;
      for (PrimitivePosition = Group->GetHeadPosition(); (posPrimPrv = PrimitivePosition) != 0;) {
        Primitive = Group->GetNext(PrimitivePosition);

        if ((iInts = Primitive->IsWithinArea(ptLL, ptUR, ptInt)) == 0) continue;

        Group->RemoveAt(posPrimPrv);

        for (int i = 0; i < iInts; i += 2) {
          if (i != 0) GroupsOut->RemoveTail();
          Primitive->CutAt2Pts(&ptInt[i], GroupsOut, GroupsIn);
        }
      }
      if (Group->IsEmpty()) {  // seg was emptied remove from lists
        Document->AnyLayerRemove(Group);
        Document->RemoveGroupFromAllViews(Group);
        Group->DeletePrimitivesAndRemoveAll();
        delete Group;
      }
    }

    if (GroupsOut->GetCount() > 0) {
      Document->AddWorkLayerGroups(GroupsOut);
      Document->UpdateAllViews(nullptr, EoDb::kGroups, GroupsOut);
    }
    if (GroupsIn->GetCount() > 0) {
      Document->AddWorkLayerGroups(GroupsIn);
      Document->AddGroupsToTrap(GroupsIn);
    }
    delete GroupsIn;
    delete GroupsOut;

    pstate.SetPen(this, DeviceContext, color, LineType);
    UpdateStateInformation(BothCounts);

    RubberBandingDisable();
    ModeLineUnhighlightOp(previousKeyDown);
  }
}
void AeSysView::OnCutModeClip() {
  CDC* DeviceContext = GetDC();
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

    EoInt16 color = pstate.PenColor();
    EoInt16 LineType = pstate.LineType();

    auto* Document = GetDocument();

    EoGeTransformMatrix tm = ModelViewGetMatrixInverse();

    EoGePoint4d ptView[] = {EoGePoint4d(pt1), EoGePoint4d(pt2)};

    ModelViewTransformPoints(2, ptView);

    EoDbGroupList* GroupsOut = new EoDbGroupList;
    EoDbGroupList* GroupsIn = new EoDbGroupList;

    POSITION posSeg;
    POSITION posSegPrv;

    for (posSeg = GetFirstVisibleGroupPosition(); (posSegPrv = posSeg) != 0;) {
      auto* Group = GetNextVisibleGroup(posSeg);

      if (Document->FindTrappedGroup(Group) != 0) continue;

      POSITION posPrim1;
      POSITION posPrim2;

      for (posPrim1 = Group->GetHeadPosition(); (posPrim2 = posPrim1) != 0;) {
        EoDbPrimitive* Primitive = Group->GetNext(posPrim1);

        if (!Primitive->SelectUsingPoint(this, ptView[0], ptCut[0])) continue;
        dRel[0] = EoDbPrimitive::Rel();
        if (!Primitive->SelectUsingPoint(this, ptView[1], ptCut[1])) continue;
        dRel[1] = EoDbPrimitive::Rel();
        // Both pick points are within tolerance of primative
        ptCut[0] = tm * ptCut[0];
        ptCut[1] = tm * ptCut[1];
        if (dRel[0] > dRel[1]) {
          EoGePoint3d ptTmp = ptCut[0];
          ptCut[0] = ptCut[1];
          ptCut[1] = ptTmp;
        }
        Group->RemoveAt(posPrim2);

        Primitive->CutAt2Pts(ptCut, GroupsOut, GroupsIn);
      }
      if (Group->IsEmpty()) {  // seg was emptied remove from lists
        Document->AnyLayerRemove(Group);
        Document->RemoveGroupFromAllViews(Group);
        Group->DeletePrimitivesAndRemoveAll();
        delete Group;
      }
    }
    if (GroupsOut->GetCount() > 0) {
      Document->AddWorkLayerGroups(GroupsOut);
      Document->UpdateAllViews(nullptr, EoDb::kGroups, GroupsOut);
    }
    if (GroupsIn->GetCount() > 0) {
      Document->AddWorkLayerGroups(GroupsIn);
      Document->AddGroupsToTrap(GroupsIn);
      Document->UpdateAllViews(nullptr, EoDb::kGroupsTrap, GroupsIn);
    }
    delete GroupsIn;
    delete GroupsOut;

    pstate.SetPen(this, DeviceContext, color, LineType);
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
