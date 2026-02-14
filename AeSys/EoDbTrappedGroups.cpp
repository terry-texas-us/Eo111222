#include "Stdafx.h"

#include <corecrt.h>
#include <cstdint>

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
#include "EoGsRenderState.h"

POSITION AeSysDoc::AddGroupToTrap(EoDbGroup* group) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupSafeTrap, group); }
  return m_trappedGroups.AddTail(group);
}

void AeSysDoc::AddGroupsToTrap(EoDbGroupList* groups) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, groups); }
  m_trappedGroups.AddTail(groups);
}

void AeSysDoc::ModifyTrappedGroupsNoteAttributes(const EoDbFontDefinition& fontDefinition,
    const EoDbCharacterCellDefinition& characterCellDefinition, int attributes) {
  m_trappedGroups.ModifyNotes(fontDefinition, characterCellDefinition, attributes);
}

void AeSysDoc::RemoveAllTrappedGroups() {
  if (m_trappedGroups.IsEmpty()) { return; }
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafe, &m_trappedGroups); }
  m_trappedGroups.RemoveAll();
}

void AeSysDoc::TranslateTrappedGroups(const EoGeVector3d& translate) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafe, &m_trappedGroups); }
  m_trappedGroups.Translate(translate);
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, &m_trappedGroups); }
}

void AeSysDoc::CompressTrappedGroups() {
  if (m_trappedGroups.GetCount() <= 1) { return; }
  auto* newGroup = new EoDbGroup;

  auto groupPosition = m_trappedGroups.GetHeadPosition();
  while (groupPosition != nullptr) {
    auto* group = m_trappedGroups.GetNext(groupPosition);

    AnyLayerRemove(group);
    RemoveGroupFromAllViews(group);
    newGroup->AddTail(group);
    // delete the original group but not its primitives
    delete group;
  }
  // emtpy trap group list
  m_trappedGroups.RemoveAll();
  AddWorkLayerGroup(newGroup);
  m_trappedGroups.AddTail(newGroup);

  newGroup->SortTextOnY();
}

void AeSysDoc::CopyTrappedGroups(const EoGeVector3d& translate) {
  auto groupPosition = m_trappedGroups.GetHeadPosition();
  while (groupPosition != nullptr) {
    auto* group = m_trappedGroups.GetNext(groupPosition);
    auto* newGroup = new EoDbGroup(*group);

    AddWorkLayerGroup(newGroup);
    UpdateAllViews(nullptr, EoDb::kGroup, group);
    group->Translate(translate);

    LPARAM hint = (app.IsTrapHighlighted()) ? EoDb::kGroupSafeTrap : EoDb::kGroupSafe;
    UpdateAllViews(nullptr, hint, group);
  }
}

void AeSysDoc::CopyTrappedGroupsToClipboard(AeSysView* view) {
  ::OpenClipboard(nullptr);
  ::EmptyClipboard();

  if (app.IsClipboardDataText()) {
    // UNDONE possible
    CString strBuf;

    auto groupPosition = GetFirstTrappedGroupPosition();
    while (groupPosition != nullptr) {
      auto* group = GetNextTrappedGroup(groupPosition);

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != nullptr) {
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
    int savedRenderState = renderState.Save();

    auto enhancedMetafileContext = ::CreateEnhMetaFileW(0, 0, 0, 0);
    m_trappedGroups.Display(view, CDC::FromHandle(enhancedMetafileContext));
    auto enhancedMetafileHandle = ::CloseEnhMetaFile(enhancedMetafileContext);
    ::SetClipboardData(CF_ENHMETAFILE, enhancedMetafileHandle);

    renderState.Restore(CDC::FromHandle(enhancedMetafileContext), savedRenderState);
  }
  if (app.IsClipboardDataGroups()) {
    EoGePoint3d minPoint(Eo::boundsMax, Eo::boundsMax, Eo::boundsMax);
    EoGePoint3d maxPoint(Eo::boundsMin, Eo::boundsMin, Eo::boundsMin);

    CMemFile memoryFile;

    memoryFile.SetLength(96);
    memoryFile.SeekToEnd();

    auto* primitiveBuffer = new std::uint8_t[EoDbPrimitive::BUFFER_SIZE];

    m_trappedGroups.Write(memoryFile, primitiveBuffer);
    m_trappedGroups.GetExtents(view, minPoint, maxPoint, view->ModelViewGetMatrix());

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
  auto groupPosition = m_trappedGroups.GetHeadPosition();
  while (groupPosition != nullptr) {
    auto* group = m_trappedGroups.GetNext(groupPosition);
    AnyLayerRemove(group);
    RemoveGroupFromAllViews(group);
    group->DeletePrimitivesAndRemoveAll();
    delete group;
  }
  m_trappedGroups.RemoveAll();
}

void AeSysDoc::ExpandTrappedGroups() {
  if (m_trappedGroups.IsEmpty()) { return; }
  auto* groups = new EoDbGroupList;

  try {
    groups->AddTail(&m_trappedGroups);
    m_trappedGroups.RemoveAll();

    auto groupPosition = groups->GetHeadPosition();
    while (groupPosition != 0) {
      auto group = groups->GetNext(groupPosition);

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != 0) {
        auto primitive = group->GetNext(primitivePosition);
        auto newGroup = new EoDbGroup(primitive);
        AddWorkLayerGroup(newGroup);
        m_trappedGroups.AddTail(newGroup);
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
  UpdateAllViews(nullptr, EoDb::kGroupsEraseSafeTrap, &m_trappedGroups);

  auto GroupPosition = m_trappedGroups.GetHeadPosition();
  while (GroupPosition != nullptr) {
    auto* Group = m_trappedGroups.GetNext(GroupPosition);
    Group->Square(view);
  }
  UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, &m_trappedGroups);
}

void AeSysDoc::TransformTrappedGroups(EoGeTransformMatrix& transformMatrix) {
  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsEraseSafeTrap, &m_trappedGroups); }
  m_trappedGroups.Transform(transformMatrix);

  if (app.IsTrapHighlighted()) { UpdateAllViews(nullptr, EoDb::kGroupsSafeTrap, &m_trappedGroups); }
}
