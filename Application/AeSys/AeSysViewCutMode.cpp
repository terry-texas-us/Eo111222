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
std::uint16_t previousKeyDown{};
EoGePoint3d previousPosition{};
}  // namespace

void AeSysView::OnCutModeOptions() {}

void AeSysView::OnCutModeTorch() {
  auto* document = GetDocument();

  const auto cursorPosition = GetCursorPosition();
  auto* groups = new EoDbGroupList;

  EoGePoint3d ptCut;

  const EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    const auto* group = GetNextVisibleGroup(groupPosition);

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
  const auto cursorPosition = GetCursorPosition();
  if (previousKeyDown != ID_OP2) {
    previousPosition = cursorPosition;
    RubberBandingStartAtEnable(cursorPosition, Lines);
    previousKeyDown = ModeLineHighlightOp(ID_OP2);
  } else {
    const EoGePoint3d pt1 = previousPosition;
    const EoGePoint3d pt2 = cursorPosition;

    auto* document = GetDocument();

    auto* groups = new EoDbGroupList;

    EoGeLine ln;
    EoGePoint3dArray intersections;

    EoGePoint4d ptView[] = {EoGePoint4d(pt1), EoGePoint4d(pt2)};
    ModelViewTransformPoints(2, ptView);

    const EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

    auto groupPosition = GetFirstVisibleGroupPosition();
    while (groupPosition != nullptr) {
      auto* group = GetNextVisibleGroup(groupPosition);

      if (document->FindTrappedGroup(group) != nullptr) { continue; }

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != nullptr) {
        auto* primitive = group->GetNext(primitivePosition);

        ln = EoGeLine(EoGePoint3d{ptView[0]}, EoGePoint3d{ptView[1]});
        primitive->SelectUsingLine(this, ln, intersections);
        for (auto i = 0; i < intersections.GetSize(); i++) {
          auto* newGroup = new EoDbGroup;

          intersections[i] = transformMatrix * intersections[i];

          document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, primitive);
          primitive->CutAtPoint(intersections[i], newGroup);
          document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, primitive);
          groups->AddTail(newGroup);
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
  const auto cursorPosition = GetCursorPosition();
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

    const EoGePoint3d lowerLeft = rLL;
    const EoGePoint3d upperRight = rUR;

    EoDbGroup* group{};
    EoDbPrimitive* primitive{};

    int iInts{};
    EoGePoint3d ptInt[10]{};

    auto* document = GetDocument();

    const auto color = Gs::renderState.Color();
    const auto lineType = Gs::renderState.LineTypeIndex();

    auto* groupsOut = new EoDbGroupList;
    auto* groupsIn = new EoDbGroupList;

    POSITION posSeg, posSegPrv;
    for (posSeg = GetFirstVisibleGroupPosition(); (posSegPrv = posSeg) != nullptr;) {
      group = GetNextVisibleGroup(posSeg);

      if (document->FindTrappedGroup(group) != nullptr) { continue; }

      POSITION primitivePosition;
      POSITION previousPrimitivePosition;
      for (primitivePosition = group->GetHeadPosition(); (previousPrimitivePosition = primitivePosition) != nullptr;) {
        primitive = group->GetNext(primitivePosition);

        if ((iInts = primitive->IsWithinArea(lowerLeft, upperRight, ptInt)) == 0) { continue; }

        group->RemoveAt(previousPrimitivePosition);

        for (int i = 0; i < iInts; i += 2) {
          if (i != 0) { groupsOut->RemoveTail(); }
          primitive->CutAt2Points(ptInt[i], ptInt[i + 1], groupsOut, groupsIn);
        }
      }
      if (group->IsEmpty()) {  // seg was emptied remove from lists
        document->AnyLayerRemove(group);
        document->RemoveGroupFromAllViews(group);
        group->DeletePrimitivesAndRemoveAll();
        delete group;
      }
    }

    if (groupsOut->GetCount() > 0) {
      document->AddWorkLayerGroups(groupsOut);
      document->UpdateAllViews(nullptr, EoDb::kGroups, groupsOut);
    }
    if (groupsIn->GetCount() > 0) {
      document->AddWorkLayerGroups(groupsIn);
      document->AddGroupsToTrap(groupsIn);
    }
    delete groupsIn;
    delete groupsOut;

    auto* deviceContext = GetDC();
    if (!deviceContext) { return; }
    Gs::renderState.SetPen(this, deviceContext, color, lineType);
    ReleaseDC(deviceContext);

    UpdateStateInformation(BothCounts);
    RubberBandingDisable();
    ModeLineUnhighlightOp(previousKeyDown);
  }
}

void AeSysView::OnCutModeClip() {
  const auto cursorPosition = GetCursorPosition();
  if (previousKeyDown != ID_OP7) {
    previousPosition = cursorPosition;
    previousKeyDown = ModeLineHighlightOp(ID_OP7);
  } else {
    const EoGePoint3d pt1 = previousPosition;
    const EoGePoint3d pt2 = cursorPosition;

    if (pt1 == pt2) { return; }

    double dRel[2]{};
    EoGePoint3d ptCut[2]{};

    const auto color = Gs::renderState.Color();
    const auto lineType = Gs::renderState.LineTypeIndex();

    auto* document = GetDocument();

    const EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

    EoGePoint4d ptView[] = {EoGePoint4d(pt1), EoGePoint4d(pt2)};

    ModelViewTransformPoints(2, ptView);

    auto* groupsOut = new EoDbGroupList;
    auto* groupsIn = new EoDbGroupList;

    POSITION posSeg;
    POSITION posSegPrv;

    for (posSeg = GetFirstVisibleGroupPosition(); (posSegPrv = posSeg) != nullptr;) {
      auto* group = GetNextVisibleGroup(posSeg);

      if (document->FindTrappedGroup(group) != nullptr) { continue; }

      POSITION posPrim1;
      POSITION posPrim2;

      for (posPrim1 = group->GetHeadPosition(); (posPrim2 = posPrim1) != nullptr;) {
        EoDbPrimitive* primitive = group->GetNext(posPrim1);

        if (!primitive->SelectUsingPoint(this, ptView[0], ptCut[0])) { continue; }
        dRel[0] = EoDbPrimitive::Rel();
        if (!primitive->SelectUsingPoint(this, ptView[1], ptCut[1])) { continue; }
        dRel[1] = EoDbPrimitive::Rel();
        // Both pick points are within tolerance of primative
        ptCut[0] = transformMatrix * ptCut[0];
        ptCut[1] = transformMatrix * ptCut[1];
        if (dRel[0] > dRel[1]) {
          const EoGePoint3d ptTmp = ptCut[0];
          ptCut[0] = ptCut[1];
          ptCut[1] = ptTmp;
        }
        group->RemoveAt(posPrim2);

        primitive->CutAt2Points(ptCut[0], ptCut[1], groupsOut, groupsIn);
      }
      if (group->IsEmpty()) {  // seg was emptied remove from lists
        document->AnyLayerRemove(group);
        document->RemoveGroupFromAllViews(group);
        group->DeletePrimitivesAndRemoveAll();
        delete group;
      }
    }
    if (groupsOut->GetCount() > 0) {
      document->AddWorkLayerGroups(groupsOut);
      document->UpdateAllViews(nullptr, EoDb::kGroups, groupsOut);
    }
    if (groupsIn->GetCount() > 0) {
      document->AddWorkLayerGroups(groupsIn);
      document->AddGroupsToTrap(groupsIn);
      document->UpdateAllViews(nullptr, EoDb::kGroupsTrap, groupsIn);
    }
    delete groupsIn;
    delete groupsOut;

    auto* deviceContext = GetDC();
    if (!deviceContext) { return; }
    Gs::renderState.SetPen(this, deviceContext, color, lineType);
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
