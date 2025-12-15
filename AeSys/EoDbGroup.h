#pragma once

class EoDbBlock;
class EoDbCharacterCellDefinition;
class EoDbFontDefinition;
class EoDbPoint;
class EoDbPrimitive;

class EoDbGroup : public CObList {
  static EoDbPrimitive* sm_PrimitiveToIgnore;

 public:
  static void SetPrimitiveToIgnore(EoDbPrimitive* primitive) { sm_PrimitiveToIgnore = primitive; }

 public:
  EoDbGroup();
  EoDbGroup(EoDbPrimitive* primitive);

  EoDbGroup& operator=(const EoDbGroup&) = delete;

  ~EoDbGroup() override {}

  EoDbGroup(const EoDbGroup& group);
  EoDbGroup(const EoDbBlock& group);

  void AddPrimsToTreeViewControl(HWND tree, HTREEITEM parent);
  HTREEITEM AddToTreeViewControl(HWND tree, HTREEITEM parent);
  void BreakPolylines();
  void BreakSegRefs();
  void DeletePrimitivesAndRemoveAll();
  void Display(AeSysView* view, CDC* deviceContext);
  POSITION FindAndRemovePrim(EoDbPrimitive* primitive);
  EoDbPrimitive* GetAt(POSITION position);
  int GetBlockRefCount(const CString& blockName);
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&);
  EoDbPrimitive* GetNext(POSITION& position) const { return ((EoDbPrimitive*)CObList::GetNext(position)); }
  EoDbPoint* GetFirstDifferentPoint(EoDbPoint* pointPrimitive);
  int GetLineTypeRefCount(EoInt16 lineType);
  void InsertBefore(POSITION position, EoDbGroup* group);
  bool IsInView(AeSysView* view);
  void ModifyNotes(EoDbFontDefinition& fd, EoDbCharacterCellDefinition& ccd, int iAtt = 0);
  void ModifyPenColor(EoInt16 penColor);
  void ModifyLineType(EoInt16 lineType);
  void PenTranslation(EoUInt16, EoInt16*, EoInt16*);
  void RemoveDuplicatePrimitives();
  int RemoveEmptyNotesAndDelete();
  EoDbPrimitive* SelPrimAtCtrlPt(AeSysView* view, const EoGePoint4d&, EoGePoint3d*);
  /// <summary>Picks a prim if close enough to point.  Working in view space.</summary>
  EoDbPrimitive* SelPrimUsingPoint(AeSysView* view, const EoGePoint4d&, double&, EoGePoint3d&);
  bool SelectUsingLine(AeSysView* view, const EoGePoint3d& pt1, const EoGePoint3d& pt2);
  bool SelectUsingPoint_(AeSysView* view, EoGePoint4d pt);
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2);
  void SortTextOnY();
  void Square(AeSysView* view);
  void Transform(EoGeTransformMatrix& tm);
  void Translate(EoGeVector3d translate);
  void Write(CFile& file);
  void Write(CFile& file, EoByte* buffer);
};
