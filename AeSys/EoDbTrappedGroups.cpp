#include "Stdafx.h"

#include <corecrt.h>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "PrimState.h"

POSITION AeSysDoc::AddGroupToTrap(EoDbGroup* group) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupSafeTrap, group); }
  return m_TrappedGroupList.AddTail(group);
}
void AeSysDoc::AddGroupsToTrap(EoDbGroupList* groups) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, groups); }
  m_TrappedGroupList.AddTail(groups);
}
void AeSysDoc::ModifyTrappedGroupsNoteAttributes(const EoDbFontDefinition& fontDefinition,
                                                 const EoDbCharacterCellDefinition& characterCellDefinition, int attributes) {
  m_TrappedGroupList.ModifyNotes(fontDefinition, characterCellDefinition, attributes);
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
  while (GroupPosition != nullptr) {
    auto* Group = m_TrappedGroupList.GetNext(GroupPosition);

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
  while (GroupPosition != nullptr) {
    auto* Group = m_TrappedGroupList.GetNext(GroupPosition);
    auto* NewGroup = new EoDbGroup(*Group);

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

    auto groupPosition = GetFirstTrappedGroupPosition();
    while (groupPosition != 0) {
      auto* group = GetNextTrappedGroup(groupPosition);

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != 0) {
        auto* primitive = group->GetNext(primitivePosition);
        if (primitive->Is(EoDb::kTextPrimitive)) {
          strBuf += static_cast<EoDbText*>(primitive)->Text();
          strBuf += L"\r\n";
        }
      }
    }
    int allocationSize = strBuf.GetLength() + 1;
    GLOBALHANDLE clipboardDataHandle = GlobalAlloc(GHND, static_cast<SIZE_T>(allocationSize));
    if (clipboardDataHandle != nullptr) {
      auto clipboardData = (LPTSTR)GlobalLock(clipboardDataHandle);

      if (clipboardData != nullptr) { wcscpy_s(clipboardData, static_cast<rsize_t>(allocationSize), strBuf); }
      GlobalUnlock(clipboardDataHandle);
      ::SetClipboardData(CF_TEXT, clipboardDataHandle);
    }
  }
  if (app.IsClipboardDataImage()) {
    int PrimitiveState = pstate.Save();

    auto enhancedMetafileContext = ::CreateEnhMetaFileW(0, 0, 0, 0);
    m_TrappedGroupList.Display(view, CDC::FromHandle(enhancedMetafileContext));
    auto enhancedMetafileHandle = ::CloseEnhMetaFile(enhancedMetafileContext);
    ::SetClipboardData(CF_ENHMETAFILE, enhancedMetafileHandle);

    pstate.Restore(CDC::FromHandle(enhancedMetafileContext), PrimitiveState);
  }
  if (app.IsClipboardDataGroups()) {
    EoGePoint3d minPoint(Eo::boundsMax, Eo::boundsMax, Eo::boundsMax);
    EoGePoint3d maxPoint(Eo::boundsMin, Eo::boundsMin, Eo::boundsMin);

    CMemFile memoryFile;

    memoryFile.SetLength(96);
    memoryFile.SeekToEnd();

    auto* primitiveBuffer = new EoUInt8[EoDbPrimitive::BUFFER_SIZE];

    m_TrappedGroupList.Write(memoryFile, primitiveBuffer);
    m_TrappedGroupList.GetExtents(view, minPoint, maxPoint, view->ModelViewGetMatrix());

    delete[] primitiveBuffer;

    minPoint = view->ModelViewGetMatrixInverse() * minPoint;

    auto memoryFileLength = memoryFile.GetLength();

    memoryFile.SeekToBegin();
    memoryFile.Write(&memoryFileLength, sizeof(DWORD));
    minPoint.Write(memoryFile);

    GLOBALHANDLE clipboardDataHandle = GlobalAlloc(GHND, SIZE_T(memoryFileLength));
    if (clipboardDataHandle != nullptr) {
      auto clipboardData = (LPTSTR)GlobalLock(clipboardDataHandle);

      memoryFile.SeekToBegin();
      memoryFile.Read(clipboardData, UINT(memoryFileLength));

      GlobalUnlock(clipboardDataHandle);
      ::SetClipboardData(app.ClipboardFormatIdentifierForEoGroups(), clipboardDataHandle);
    }
  }
  ::CloseClipboard();
}

void AeSysDoc::DeleteAllTrappedGroups() {
  auto GroupPosition = m_TrappedGroupList.GetHeadPosition();
  while (GroupPosition != nullptr) {
    auto* Group = m_TrappedGroupList.GetNext(GroupPosition);
    AnyLayerRemove(Group);
    RemoveGroupFromAllViews(Group);
    Group->DeletePrimitivesAndRemoveAll();
    delete Group;
  }
  m_TrappedGroupList.RemoveAll();
}

void AeSysDoc::ExpandTrappedGroups() {
  if (m_TrappedGroupList.IsEmpty()) { return; }
  auto* groups = new EoDbGroupList;

  try {
    groups->AddTail(&m_TrappedGroupList);
    m_TrappedGroupList.RemoveAll();

    auto groupPosition = groups->GetHeadPosition();
    while (groupPosition != 0) {
      auto group = groups->GetNext(groupPosition);

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != 0) {
        auto primitive = group->GetNext(primitivePosition);
        auto newGroup = new EoDbGroup(primitive);
        AddWorkLayerGroup(newGroup);
        m_TrappedGroupList.AddTail(newGroup);
      }
      AnyLayerRemove(group);
      RemoveGroupFromAllViews(group);
      delete group;
    }
    delete groups;
  } catch (...) {
    delete groups;
    throw;
  }
}

void AeSysDoc::SquareTrappedGroups(AeSysView* view) {
  UpdateAllViews(nullptr, EoDb::kGroupsEraseSafeTrap, &m_TrappedGroupList);

  auto GroupPosition = m_TrappedGroupList.GetHeadPosition();
  while (GroupPosition != nullptr) {
    auto* Group = m_TrappedGroupList.GetNext(GroupPosition);
    Group->Square(view);
  }
  UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, &m_TrappedGroupList);
}
void AeSysDoc::TransformTrappedGroups(EoGeTransformMatrix& transformMatrix) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsEraseSafeTrap, &m_TrappedGroupList); }
  m_TrappedGroupList.Transform(transformMatrix);

  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, &m_TrappedGroupList); }
}
