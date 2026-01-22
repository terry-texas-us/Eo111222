#include "Stdafx.h"

#include "AeSysView.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPolygon.h"

void EoDbGroupList::AddToTreeViewControl(HWND hTree, HTREEITEM htiParent) {
  auto position = GetHeadPosition();

  while (position != nullptr) {
    auto* group = GetNext(position);
    HTREEITEM htiSeg = group->AddToTreeViewControl(hTree, htiParent);
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
void EoDbGroupList::BreakSegRefs() {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->BreakSegRefs();
  }
}
void EoDbGroupList::Display(AeSysView* view, CDC* deviceContext) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->Display(view, deviceContext);
  }
}
POSITION EoDbGroupList::Remove(EoDbGroup* group) {
  auto position = Find(group);
  if (position != nullptr) RemoveAt(position);

  return (position);
}

int EoDbGroupList::GetBlockRefCount(const CString& strBlkNam) {
  int iCount = 0;

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    iCount += group->GetBlockRefCount(strBlkNam);
  }
  return (iCount);
}

void EoDbGroupList::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->GetExtents(view, ptMin, ptMax, tm);
  }
}
int EoDbGroupList::GetLineTypeRefCount(EoInt16 lineType) {
  int iCount = 0;

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    iCount += group->GetLineTypeRefCount(lineType);
  }
  return (iCount);
}
void EoDbGroupList::ModifyColor(EoInt16 color) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->ModifyColor(color);
  }
}

void EoDbGroupList::ModifyLineType(EoInt16 nStyle) {
  auto position = GetHeadPosition();
  while (position != nullptr) (GetNext(position))->ModifyLineType(nStyle);
}

void EoDbGroupList::ModifyNotes(EoDbFontDefinition& fd, EoDbCharacterCellDefinition& ccd, int iAtt) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->ModifyNotes(fd, ccd, iAtt);
  }
}

void EoDbGroupList::PenTranslation(EoUInt16 wCols, EoInt16* pColNew, EoInt16* pCol) {
  auto position = GetHeadPosition();
  while (position != 0) (GetNext(position))->PenTranslation(wCols, pColNew, pCol);
}
int EoDbGroupList::RemoveEmptyNotesAndDelete() {
  int iCount = 0;

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    iCount += group->RemoveEmptyNotesAndDelete();
  }
  return (iCount);
}
int EoDbGroupList::RemoveEmptyGroups() {
  int iCount = 0;

  auto position = GetHeadPosition();
  while (position != nullptr) {
    POSITION posPrev = position;
    auto* group = GetNext(position);
    if (group->GetCount() == 0) {
      RemoveAt(posPrev);
      delete group;
      iCount++;
    }
  }
  return (iCount);
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

  EoDbGroup* pPicSeg = 0;

  EoGePoint4d ptView(pt);
  activeView->ModelViewTransformPoint(ptView);

  EoGeTransformMatrix tm = activeView->ModelViewGetMatrixInverse();

  double dPicApert = activeView->SelectApertureSize();

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    if (group->SelPrimUsingPoint(activeView, ptView, dPicApert, ptEng) != 0) { pPicSeg = group; }
  }
  return (pPicSeg);
}
void EoDbGroupList::Transform(EoGeTransformMatrix& tm) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->Transform(tm);
  }
}

void EoDbGroupList::Translate(EoGeVector3d v) {
  if (v.IsNearNull()) return;

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* group = GetNext(position);
    group->Translate(v);
  }
}

void EoDbGroupList::Write(CFile& file, EoUInt8* buffer) {
  auto position = GetHeadPosition();
  while (position != nullptr) GetNext(position)->Write(file, buffer);
}
