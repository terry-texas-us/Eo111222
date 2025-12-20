#include "stdafx.h"

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "PrimState.h"

EoUInt16 wPrvKeyDwn{0};
EoGePoint3d rPrvPos;

void AeSysView::OnCutModeOptions() {}
void AeSysView::OnCutModeTorch() {
  AeSysDoc* Document = GetDocument();

  EoGePoint3d pt = GetCursorPosition();
  EoDbGroupList* Groups = new EoDbGroupList;

  EoGePoint3d ptCut;

  EoGeTransformMatrix tm = ModelViewGetMatrixInverse();

  EoGePoint4d ptView(pt);
  ModelViewTransformPoint(ptView);

  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != 0) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

      if (Primitive->SelectUsingPoint(this, ptView, ptCut)) {  // Pick point is within tolerance of primative
        EoDbGroup* NewGroup = new EoDbGroup;

        ptCut = tm * ptCut;
        Document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, Primitive);
        Primitive->CutAtPt(ptCut, NewGroup);
        Document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, Primitive);
        Groups->AddTail(NewGroup);
        break;
      }
    }
  }
  Document->AddWorkLayerGroups(Groups);
  Document->UpdateAllViews(nullptr, EoDb::kGroupsSafe, Groups);
  delete Groups;
}
void AeSysView::OnCutModeSlice() {
  EoGePoint3d ptCur = GetCursorPosition();
  if (wPrvKeyDwn != ID_OP2) {
    rPrvPos = ptCur;
    RubberBandingStartAtEnable(ptCur, Lines);
    wPrvKeyDwn = ModeLineHighlightOp(ID_OP2);
  } else {
    EoGePoint3d pt1 = rPrvPos;
    EoGePoint3d pt2 = ptCur;

    AeSysDoc* Document = GetDocument();

    EoDbGroupList* Groups = new EoDbGroupList;

    EoGeLine ln;
    EoGePoint3dArray ptsInt;

    EoGePoint4d ptView[] = {EoGePoint4d(pt1), EoGePoint4d(pt2)};
    ModelViewTransformPoints(2, ptView);

    EoGeTransformMatrix tm = ModelViewGetMatrixInverse();

    auto GroupPosition = GetFirstVisibleGroupPosition();
    while (GroupPosition != 0) {
      EoDbGroup* Group = GetNextVisibleGroup(GroupPosition);

      if (Document->FindTrappedGroup(Group) != 0) continue;

      auto PrimitivePosition = Group->GetHeadPosition();
      while (PrimitivePosition != 0) {
        EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

        ln = EoGeLine(ptView[0], ptView[1]);
        Primitive->SelectUsingLine(this, ln, ptsInt);
        for (EoUInt16 w = 0; w < ptsInt.GetSize(); w++) {
          EoDbGroup* NewGroup = new EoDbGroup;

          ptsInt[w] = tm * ptsInt[w];

          Document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, Primitive);
          Primitive->CutAtPt(ptsInt[w], NewGroup);
          Document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, Primitive);
          Groups->AddTail(NewGroup);
        }
      }
    }
    Document->AddWorkLayerGroups(Groups);
    Document->UpdateAllViews(nullptr, EoDb::kGroupsSafe, Groups);
    delete Groups;

    RubberBandingDisable();
    ModeLineUnhighlightOp(wPrvKeyDwn);
  }
}
void AeSysView::OnCutModeField() {
  CDC* DeviceContext = GetDC();
  EoGePoint3d ptCur = GetCursorPosition();
  if (wPrvKeyDwn != ID_OP4) {
    rPrvPos = ptCur;
    RubberBandingStartAtEnable(ptCur, Rectangles);
    wPrvKeyDwn = ModeLineHighlightOp(ID_OP4);
  } else {
    EoGePoint3d rLL, rUR;

    rLL.x = std::min(rPrvPos.x, ptCur.x);
    rLL.y = std::min(rPrvPos.y, ptCur.y);
    rUR.x = std::max(rPrvPos.x, ptCur.x);
    rUR.y = std::max(rPrvPos.y, ptCur.y);

    EoGePoint3d ptLL = rLL;
    EoGePoint3d ptUR = rUR;

    EoDbGroup* Group;
    EoDbPrimitive* Primitive;

    int iInts;
    EoGePoint3d ptInt[10];

    AeSysDoc* Document = GetDocument();

    EoInt16 nPenColor = pstate.PenColor();
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

    pstate.SetPen(this, DeviceContext, nPenColor, LineType);
    UpdateStateInformation(BothCounts);

    RubberBandingDisable();
    ModeLineUnhighlightOp(wPrvKeyDwn);
  }
}
void AeSysView::OnCutModeClip() {
  CDC* DeviceContext = GetDC();
  EoGePoint3d ptCur = GetCursorPosition();
  if (wPrvKeyDwn != ID_OP7) {
    rPrvPos = ptCur;
    wPrvKeyDwn = ModeLineHighlightOp(ID_OP7);
  } else {
    EoGePoint3d pt1 = rPrvPos;
    EoGePoint3d pt2 = ptCur;

    if (pt1 == pt2) { return; }

    double dRel[2];
    EoGePoint3d ptCut[2];

    EoInt16 nPenColor = pstate.PenColor();
    EoInt16 LineType = pstate.LineType();

    AeSysDoc* Document = GetDocument();

    EoGeTransformMatrix tm = ModelViewGetMatrixInverse();

    EoGePoint4d ptView[] = {EoGePoint4d(pt1), EoGePoint4d(pt2)};

    ModelViewTransformPoints(2, ptView);

    EoDbGroupList* GroupsOut = new EoDbGroupList;
    EoDbGroupList* GroupsIn = new EoDbGroupList;

    POSITION posSeg;
    POSITION posSegPrv;

    for (posSeg = GetFirstVisibleGroupPosition(); (posSegPrv = posSeg) != 0;) {
      EoDbGroup* Group = GetNextVisibleGroup(posSeg);

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

    pstate.SetPen(this, DeviceContext, nPenColor, LineType);
    UpdateStateInformation(BothCounts);

    ModeLineUnhighlightOp(wPrvKeyDwn);
  }
}
void AeSysView::OnCutModeDivide() {}
void AeSysView::OnCutModeReturn() {
  RubberBandingDisable();
  ModeLineUnhighlightOp(wPrvKeyDwn);
}
void AeSysView::OnCutModeEscape() {
  RubberBandingDisable();
  ModeLineUnhighlightOp(wPrvKeyDwn);
}
