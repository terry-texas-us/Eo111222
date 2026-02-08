#pragma once

class EoDbCharacterCellDefinition;
class EoDbGroup;
class EoDbFontDefinition;

class EoDbGroupList : public CObList {
 public:
  EoDbGroupList() {}
  EoDbGroupList(const EoDbGroupList&) = delete;
  EoDbGroupList& operator=(const EoDbGroupList&) = delete;

  ~EoDbGroupList() override {}

 public:  // Base class wrappers
  auto AddHead(EoDbGroup* group) { return (CObList::AddHead((CObject*)group)); }
  auto AddTail(EoDbGroup* group) { return (CObList::AddTail((CObject*)group)); }
  void AddTail(EoDbGroupList* groupList) { CObList::AddTail((CObList*)groupList); }
  auto Find(EoDbGroup* group) { return (CObList::Find((CObject*)group)); }
  EoDbGroup* GetNext(POSITION& position) { return (EoDbGroup*)CObList::GetNext(position); }
  EoDbGroup* GetPrev(POSITION& position) { return (EoDbGroup*)CObList::GetPrev(position); }
  EoDbGroup* RemoveHead() { return (EoDbGroup*)CObList::RemoveHead(); }
  EoDbGroup* RemoveTail() { return (EoDbGroup*)CObList::RemoveTail(); }

 public:  // Methods
  void AddToTreeViewControl(HWND hTree, HTREEITEM htiParent);
  void BreakPolylines();
  void BreakSegRefs();
  void DeleteGroupsAndRemoveAll();
  void Display(AeSysView* view, CDC* deviceContext);
  int GetBlockRefCount(const CString& name);
  /// <summary>Determines the extent of all groups in list.</summary>
  void GetExtents(AeSysView* view, EoGePoint3d& minimum, EoGePoint3d& maximum, EoGeTransformMatrix& tm);
  int GetLineTypeRefCount(EoInt16 lineType);
  void ModifyNotes(const EoDbFontDefinition& fontDefinition, EoDbCharacterCellDefinition& characterCellDefinition, int iAtt = 0);
  void ModifyColor(EoInt16 color);
  void ModifyLineType(EoInt16 lineType);
  void PenTranslation(EoUInt16, EoInt16*, EoInt16*);
  int RemoveEmptyNotesAndDelete();
  int RemoveEmptyGroups();
  POSITION Remove(EoDbGroup* group);
  EoDbGroup* SelectGroupUsingPoint(const EoGePoint3d& pt);
  void Transform(EoGeTransformMatrix& tm);
  void Translate(EoGeVector3d translate);
  void Write(CFile& file, EoUInt8* buffer);
};
