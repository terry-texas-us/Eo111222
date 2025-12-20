#include "stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "PrimState.h"

POSITION AeSysDoc::AddGroupToTrap(EoDbGroup* group) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupSafeTrap, group); }
  return m_TrappedGroupList.AddTail(group);
}
void AeSysDoc::AddGroupsToTrap(EoDbGroupList* groups) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, groups); }
  m_TrappedGroupList.AddTail(groups);
}
void AeSysDoc::ModifyTrappedGroupsNoteAttributes(EoDbFontDefinition& fontDef, EoDbCharacterCellDefinition& cellDef,
                                                 int attributes) {
  m_TrappedGroupList.ModifyNotes(fontDef, cellDef, attributes);
}
void AeSysDoc::RemoveAllTrappedGroups() {
  if (!m_TrappedGroupList.IsEmpty()) {
    if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafe, &m_TrappedGroupList); }
    m_TrappedGroupList.RemoveAll();
  }
}
void AeSysDoc::TranslateTrappedGroups(EoGeVector3d translate) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafe, &m_TrappedGroupList); }
  m_TrappedGroupList.Translate(translate);
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, &m_TrappedGroupList); }
}
void AeSysDoc::CompressTrappedGroups() {
  if (m_TrappedGroupList.GetCount() <= 1) { return; }
  EoDbGroup* NewGroup = new EoDbGroup;

  auto GroupPosition = m_TrappedGroupList.GetHeadPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = m_TrappedGroupList.GetNext(GroupPosition);

    AnyLayerRemove(Group);
    RemoveGroupFromAllViews(Group);
    NewGroup->AddTail(Group);
    // delete the original group but not its primitives
    delete Group;
  }
  // emtpy trap group list
  m_TrappedGroupList.RemoveAll();
  AddWorkLayerGroup(NewGroup);
  m_TrappedGroupList.AddTail(NewGroup);

  NewGroup->SortTextOnY();
}
void AeSysDoc::CopyTrappedGroups(EoGeVector3d translate) {
  auto GroupPosition = m_TrappedGroupList.GetHeadPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = m_TrappedGroupList.GetNext(GroupPosition);
    EoDbGroup* NewGroup = new EoDbGroup(*Group);

    AddWorkLayerGroup(NewGroup);
    UpdateAllViews(nullptr, EoDb::kGroup, Group);
    Group->Translate(translate);

    LPARAM Hint = (app.IsTrapHighlighted()) ? EoDb::kGroupSafeTrap : EoDb::kGroupSafe;
    UpdateAllViews(nullptr, Hint, Group);
  }
}
void AeSysDoc::CopyTrappedGroupsToClipboard(AeSysView* view) {
  ::OpenClipboard(nullptr);
  ::EmptyClipboard();

  if (app.IsClipboardDataText()) {
    // UNDONE possible
    CString strBuf;

    auto GroupPosition = GetFirstTrappedGroupPosition();
    while (GroupPosition != 0) {
      EoDbGroup* Group = GetNextTrappedGroup(GroupPosition);

      auto PrimitivePosition = Group->GetHeadPosition();
      while (PrimitivePosition != 0) {
        EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
        if (Primitive->Is(EoDb::kTextPrimitive)) {
          strBuf += static_cast<EoDbText*>(Primitive)->Text();
          strBuf += L"\r\n";
        }
      }
    }
    int AllocationSize = strBuf.GetLength() + 1;
    GLOBALHANDLE ClipboardDataHandle = GlobalAlloc(GHND, static_cast<SIZE_T>(AllocationSize));
    LPTSTR ClipboardData = (LPTSTR)GlobalLock(ClipboardDataHandle);
    wcscpy_s(ClipboardData, static_cast<rsize_t>(AllocationSize), strBuf);
    GlobalUnlock(ClipboardDataHandle);
    ::SetClipboardData(CF_TEXT, ClipboardDataHandle);
  }
  if (app.IsClipboardDataImage()) {
    int PrimitiveState = pstate.Save();

    HDC hdcEMF = ::CreateEnhMetaFile(0, 0, 0, 0);
    m_TrappedGroupList.Display(view, CDC::FromHandle(hdcEMF));
    HENHMETAFILE hemf = ::CloseEnhMetaFile(hdcEMF);
    ::SetClipboardData(CF_ENHMETAFILE, hemf);

    pstate.Restore(CDC::FromHandle(hdcEMF), PrimitiveState);
  }
  if (app.IsClipboardDataGroups()) {
    EoGePoint3d ptMin(FLT_MAX, FLT_MAX, FLT_MAX);
    EoGePoint3d ptMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    CMemFile mf;

    mf.SetLength(96);
    mf.SeekToEnd();

    EoByte* pBuf = new EoByte[EoDbPrimitive::BUFFER_SIZE];

    m_TrappedGroupList.Write(mf, pBuf);
    m_TrappedGroupList.GetExtents(view, ptMin, ptMax, view->ModelViewGetMatrix());

    delete[] pBuf;

    ptMin = view->ModelViewGetMatrixInverse() * ptMin;

    ULONGLONG dwSizeOfBuffer = mf.GetLength();

    mf.SeekToBegin();
    mf.Write(&dwSizeOfBuffer, sizeof(DWORD));
    ptMin.Write(mf);

    GLOBALHANDLE ClipboardDataHandle = GlobalAlloc(GHND, SIZE_T(dwSizeOfBuffer));
    if (ClipboardDataHandle != nullptr) {
      LPTSTR ClipboardData = (LPTSTR)GlobalLock(ClipboardDataHandle);

      mf.SeekToBegin();
      mf.Read(ClipboardData, UINT(dwSizeOfBuffer));

      GlobalUnlock(ClipboardDataHandle);
      ::SetClipboardData(app.ClipboardFormatIdentifierForEoGroups(), ClipboardDataHandle);
    }
  }
  ::CloseClipboard();
}
void AeSysDoc::DeleteAllTrappedGroups() {
  auto GroupPosition = m_TrappedGroupList.GetHeadPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = m_TrappedGroupList.GetNext(GroupPosition);
    AnyLayerRemove(Group);
    RemoveGroupFromAllViews(Group);
    Group->DeletePrimitivesAndRemoveAll();
    delete Group;
  }
  m_TrappedGroupList.RemoveAll();
}
void AeSysDoc::ExpandTrappedGroups() {
  if (m_TrappedGroupList.IsEmpty()) { return; }
  EoDbGroup* Group;
  EoDbGroup* NewGroup;
  EoDbPrimitive* Primitive;

  EoDbGroupList* Groups = new EoDbGroupList;
  Groups->AddTail(&m_TrappedGroupList);
  m_TrappedGroupList.RemoveAll();

  auto GroupPosition = Groups->GetHeadPosition();
  while (GroupPosition != 0) {
    Group = Groups->GetNext(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != 0) {
      Primitive = Group->GetNext(PrimitivePosition);
      NewGroup = new EoDbGroup(Primitive);
      AddWorkLayerGroup(NewGroup);
      m_TrappedGroupList.AddTail(NewGroup);
    }
    AnyLayerRemove(Group);
    RemoveGroupFromAllViews(Group);
    delete Group;
  }
  delete Groups;
}
void AeSysDoc::SquareTrappedGroups(AeSysView* view) {
  UpdateAllViews(nullptr, EoDb::kGroupsEraseSafeTrap, &m_TrappedGroupList);

  auto GroupPosition = m_TrappedGroupList.GetHeadPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = m_TrappedGroupList.GetNext(GroupPosition);
    Group->Square(view);
  }
  UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, &m_TrappedGroupList);
}
void AeSysDoc::TransformTrappedGroups(EoGeTransformMatrix& tm) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsEraseSafeTrap, &m_TrappedGroupList); }
  m_TrappedGroupList.Transform(tm);

  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, &m_TrappedGroupList); }
}
