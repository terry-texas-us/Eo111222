#pragma once

#include <cstdint>

class EoDbCharacterCellDefinition;
class EoDbGroup;
class EoDbFontDefinition;
class EoGsRenderDevice;

class EoDbGroupList : public CObList {
 public:
  EoDbGroupList() {}
  EoDbGroupList(const EoDbGroupList&) = delete;
  EoDbGroupList& operator=(const EoDbGroupList&) = delete;

  ~EoDbGroupList() override {}

  auto AddHead(EoDbGroup* group) { return (CObList::AddHead((CObject*)group)); }
  auto AddTail(EoDbGroup* group) { return (CObList::AddTail((CObject*)group)); }
  void AddTail(EoDbGroupList* groupList) { CObList::AddTail((CObList*)groupList); }
  auto Find(EoDbGroup* group) { return (CObList::Find((CObject*)group)); }
  EoDbGroup* GetNext(POSITION& position) { return (EoDbGroup*)CObList::GetNext(position); }
  EoDbGroup* GetPrev(POSITION& position) { return (EoDbGroup*)CObList::GetPrev(position); }
  EoDbGroup* RemoveHead() { return (EoDbGroup*)CObList::RemoveHead(); }
  EoDbGroup* RemoveTail() { return (EoDbGroup*)CObList::RemoveTail(); }

  void AddToTreeViewControl(HWND hTree, HTREEITEM htiParent);
  void BreakPolylines();
  void DeleteGroupsAndRemoveAll();
  void Display(AeSysView* view, EoGsRenderDevice* renderDevice);

  /** @brief Replaces block reference primitives in all groups with the primitives from the referenced blocks,
   * transformed according to the block reference's properties.  This is a recursive process that continues until no
   * block reference primitives remain in any group.
   */
  void ExplodeBlockReferences();
  
  int GetBlockRefCount(const CString& name);
  /// @brief Determines the extent of all groups in list.
  void GetExtents(AeSysView* view, EoGePoint3d& minimum, EoGePoint3d& maximum, EoGeTransformMatrix& transformMatrix);
  int GetLineTypeRefCount(std::int16_t lineType);

  /** Modifies the notes of all groups in the list using the specified font definition, character cell definition and
   * attributes.
   * @param fontDefinition The font definition to apply to the notes.
   * @param characterCellDefinition The character cell definition to apply to the notes.
   * @param attributes The attributes that specify which properties of the notes to modify. This can be a combination of
   * TM_TEXT_ALL, TM_TEXT_FONT, and TM_TEXT_HEIGHT.
   */
  void ModifyNotes(const EoDbFontDefinition& fontDefinition, const EoDbCharacterCellDefinition& characterCellDefinition,
      int attributes = 0);
  void ModifyColor(std::int16_t color);
  void ModifyLineType(std::int16_t lineType);
  void PenTranslation(std::uint16_t, std::int16_t*, std::int16_t*);
  int RemoveEmptyNotesAndDelete();
  int RemoveEmptyGroups();
  POSITION Remove(EoDbGroup* group);
  EoDbGroup* SelectGroupUsingPoint(const EoGePoint3d& pt);
  void Transform(const EoGeTransformMatrix& transformMatrix);
  void Translate(EoGeVector3d translate);
  void Write(CFile& file, std::uint8_t* buffer);
};
