#include "stdafx.h"

#include "AeSysView.h"
#include "EoDbPolygon.h"

void EoDbGroupList::AddToTreeViewControl(HWND hTree, HTREEITEM htiParent) {
  POSITION position = GetHeadPosition();

  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    HTREEITEM htiSeg = Group->AddToTreeViewControl(hTree, htiParent);
    if (Group->GetCount() == 1) { TreeView_Expand(hTree, htiSeg, TVE_EXPAND); }
  }
}

void EoDbGroupList::BreakPolylines() {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    Group->BreakPolylines();
  }
}
void EoDbGroupList::BreakSegRefs() {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    Group->BreakSegRefs();
  }
}
void EoDbGroupList::Display(AeSysView* view, CDC* deviceContext) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    Group->Display(view, deviceContext);
  }
}
POSITION EoDbGroupList::Remove(EoDbGroup* group) {
  POSITION position = Find(group);
  if (position != 0) RemoveAt(position);

  return (position);
}

int EoDbGroupList::GetBlockRefCount(const CString& strBlkNam) {
  int iCount = 0;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    iCount += Group->GetBlockRefCount(strBlkNam);
  }
  return (iCount);
}

void EoDbGroupList::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    Group->GetExtents(view, ptMin, ptMax, tm);
  }
}
int EoDbGroupList::GetLineTypeRefCount(EoInt16 lineType) {
  int iCount = 0;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    iCount += Group->GetLineTypeRefCount(lineType);
  }
  return (iCount);
}
void EoDbGroupList::ModifyPenColor(EoInt16 nPenColor) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    Group->ModifyPenColor(nPenColor);
  }
}

void EoDbGroupList::ModifyLineType(EoInt16 nStyle) {
  POSITION position = GetHeadPosition();
  while (position != 0) (GetNext(position))->ModifyLineType(nStyle);
}

void EoDbGroupList::ModifyNotes(EoDbFontDefinition& fd, EoDbCharacterCellDefinition& ccd, int iAtt) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    Group->ModifyNotes(fd, ccd, iAtt);
  }
}

void EoDbGroupList::PenTranslation(EoUInt16 wCols, EoInt16* pColNew, EoInt16* pCol) {
  POSITION position = GetHeadPosition();
  while (position != 0) (GetNext(position))->PenTranslation(wCols, pColNew, pCol);
}
int EoDbGroupList::RemoveEmptyNotesAndDelete() {
  int iCount = 0;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    iCount += Group->RemoveEmptyNotesAndDelete();
  }
  return (iCount);
}
int EoDbGroupList::RemoveEmptyGroups() {
  int iCount = 0;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    POSITION posPrev = position;
    EoDbGroup* Group = GetNext(position);
    if (Group->GetCount() == 0) {
      RemoveAt(posPrev);
      delete Group;
      iCount++;
    }
  }
  return (iCount);
}
void EoDbGroupList::DeleteGroupsAndRemoveAll() {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    Group->DeletePrimitivesAndRemoveAll();
    delete Group;
  }
  RemoveAll();
}
EoDbGroup* EoDbGroupList::SelectGroupUsingPoint(const EoGePoint3d& pt) {
  AeSysView* ActiveView = AeSysView::GetActiveView();

  EoGePoint3d ptEng;

  EoDbGroup* pPicSeg = 0;

  EoGePoint4d ptView(pt);
  ActiveView->ModelViewTransformPoint(ptView);

  EoGeTransformMatrix tm = ActiveView->ModelViewGetMatrixInverse();

  double dPicApert = ActiveView->SelectApertureSize();

  EoDbPolygon::EdgeToEvaluate() = 0;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    if (Group->SelPrimUsingPoint(ActiveView, ptView, dPicApert, ptEng) != 0) { pPicSeg = Group; }
  }
  return (pPicSeg);
}
void EoDbGroupList::Transform(EoGeTransformMatrix& tm) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    Group->Transform(tm);
  }
}

void EoDbGroupList::Translate(EoGeVector3d v) {
  if (v.IsNearNull()) return;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = GetNext(position);
    Group->Translate(v);
  }
}

void EoDbGroupList::Write(CFile& file, EoByte* buffer) {
  POSITION position = GetHeadPosition();
  while (position != 0) GetNext(position)->Write(file, buffer);
}
