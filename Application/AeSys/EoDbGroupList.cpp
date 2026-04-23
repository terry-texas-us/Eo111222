#include "Stdafx.h"

#include <cstdint>

#include "AeSysView.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPolygon.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

void EoDbGroupList::AddToTreeViewControl(HWND hTree, HTREEITEM htiParent) {
  auto position = GetHeadPosition();

  while (position != nullptr) {
    auto* group = GetNext(position);
    const HTREEITEM htiSeg = group->AddToTreeViewControl(hTree, htiParent);
    if (group->GetCount() == 1) { TreeView_Expand(hTree, htiSeg, TVE_EXPAND); }
  }
}

void EoDbGroupList::BreakPolylines() {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->BreakPolylines();
  }
}

void EoDbGroupList::ExplodeBlockReferences() {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->ExplodeBlockReferences();
  }
}

void EoDbGroupList::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->Display(view, renderDevice);
  }
}

POSITION EoDbGroupList::Remove(EoDbGroup* group) {
  const auto position = Find(group);
  if (position != nullptr) { RemoveAt(position); }

  return position;
}

int EoDbGroupList::GetBlockRefCount(const CString& strBlkNam) {
  int count{};

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    count += group->GetBlockRefCount(strBlkNam);
  }
  return count;
}

void EoDbGroupList::GetExtents(
    AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& transformMatrix) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->GetExtents(view, ptMin, ptMax, transformMatrix);
  }
}
int EoDbGroupList::GetLineTypeRefCount(const std::wstring& lineTypeName) {
  int count{};

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    count += group->GetLineTypeRefCount(lineTypeName);
  }
  return count;
}
void EoDbGroupList::ModifyColor(std::int16_t color) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->ModifyColor(color);
  }
}

void EoDbGroupList::ModifyLineType(const std::wstring& lineTypeName) {
  auto position = GetHeadPosition();
  while (position != nullptr) { (GetNext(position))->ModifyLineType(lineTypeName); }
}

void EoDbGroupList::ModifyNotes(const EoDbFontDefinition& fontDefinition,
    const EoDbCharacterCellDefinition& characterCellDefinition, int attributes) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->ModifyNotes(fontDefinition, characterCellDefinition, attributes);
  }
}

void EoDbGroupList::PenTranslation(std::uint16_t wCols, std::int16_t* pColNew, std::int16_t* pCol) {
  auto position = GetHeadPosition();
  while (position != nullptr) { (GetNext(position))->PenTranslation(wCols, pColNew, pCol); }
}

int EoDbGroupList::RemoveEmptyNotesAndDelete() {
  int count{};

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    count += group->RemoveEmptyNotesAndDelete();
  }
  return count;
}

int EoDbGroupList::RemoveEmptyGroups() {
  int count{};

  auto position = GetHeadPosition();
  while (position != nullptr) {
    const auto posPrev = position;
    auto* group = GetNext(position);
    if (group->GetCount() == 0) {
      RemoveAt(posPrev);
      delete group;
      count++;
    }
  }
  return count;
}

void EoDbGroupList::DeleteGroupsAndRemoveAll() {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->DeletePrimitivesAndRemoveAll();
    delete group;
  }
  RemoveAll();
}
EoDbGroup* EoDbGroupList::SelectGroupUsingPoint(const EoGePoint3d& pt) {
  auto* activeView = AeSysView::GetActiveView();

  EoGePoint3d ptEng;

  EoDbGroup* pPicSeg{};

  EoGePoint4d ptView(pt);
  activeView->ModelViewTransformPoint(ptView);

  EoGeTransformMatrix transformMatrix = activeView->ModelViewGetMatrixInverse();

  double dPicApert = activeView->SelectApertureSize();

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    if (group->SelPrimUsingPoint(activeView, ptView, dPicApert, ptEng) != nullptr) { pPicSeg = group; }
  }
  return pPicSeg;
}
void EoDbGroupList::Transform(const EoGeTransformMatrix& transformMatrix) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->Transform(transformMatrix);
  }
}

void EoDbGroupList::Translate(EoGeVector3d v) {
  if (v.IsNearNull()) { return; }

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->Translate(v);
  }
}

void EoDbGroupList::Write(CFile& file, std::uint8_t* buffer) {
  auto position = GetHeadPosition();
  while (position != nullptr) { GetNext(position)->Write(file, buffer); }
}
