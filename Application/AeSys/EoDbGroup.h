#pragma once

#include <cstdint>

#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeVector3d.h"

class AeSysView;
class EoDbBlock;
class EoDbCharacterCellDefinition;
class EoDbFontDefinition;
class EoDbPoint;
class EoDbPrimitive;
class EoGeTransformMatrix;

/**
 * @brief Container for EoDbPrimitive objects.
 *
 * @par Ownership Model
 * EoDbGroup **owns** primitives added via AddTail/AddHead when used as a "primary group"
 * in the document's layer structure. Call DeletePrimitivesAndRemoveAll() before destruction.
 *
 * @par Selection/Trap Usage
 * When used as a temporary selection set (trap), EoDbGroup stores **non-owning references**
 * to primitives that are owned by their primary groups. Do NOT call DeletePrimitivesAndRemoveAll()
 * on trap groups - only RemoveAll().
 */
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

  /** @brief Replaces block reference primitives in the group with the primitives from the referenced blocks,
   * transformed according to the block reference's properties.  This is a recursive process that continues until no
   * block reference primitives remain in the group.
   */
  void ExplodeBlockReferences();
  void DeletePrimitivesAndRemoveAll();
  void Display(AeSysView* view, CDC* deviceContext);
  POSITION FindAndRemovePrim(EoDbPrimitive* primitive);
  EoDbPrimitive* GetAt(POSITION position);
  int GetBlockRefCount(const CString& blockName);
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&);
  EoDbPrimitive* GetNext(POSITION& position) const { return ((EoDbPrimitive*)CObList::GetNext(position)); }
  EoDbPoint* GetFirstDifferentPoint(EoDbPoint* pointPrimitive);
  int GetLineTypeRefCount(std::int16_t lineType);
  void InsertBefore(POSITION position, EoDbGroup* group);
  bool IsInView(AeSysView* view);
  void ModifyNotes(const EoDbFontDefinition& fontDefinition, const EoDbCharacterCellDefinition& characterCellDefinition,
      int attributes = 0);
  void ModifyColor(std::int16_t color);
  void ModifyLineType(std::int16_t lineType);
  void PenTranslation(std::uint16_t, std::int16_t*, std::int16_t*);

  /** @brief Removes duplicate primitives from the group.  Two primitives are considered duplicates if their
   * Identical() method returns true.  The first instance of a primitive is kept, and subsequent duplicates are removed
   * and deleted.
   */
  void RemoveDuplicatePrimitives();
  int RemoveEmptyNotesAndDelete();
  EoDbPrimitive* SelPrimAtCtrlPt(AeSysView* view, const EoGePoint4d&, EoGePoint3d*);
  /// <summary>Picks a prim if close enough to point.  Working in view space.</summary>
  EoDbPrimitive* SelPrimUsingPoint(AeSysView* view, const EoGePoint4d&, double&, EoGePoint3d&);
  bool SelectUsingLine(AeSysView* view, const EoGePoint3d& pt1, const EoGePoint3d& pt2);
  bool SelectUsingPoint_(AeSysView* view, EoGePoint4d pt);
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2);

  /** @brief Sorts text primitives in the group by their Y coordinate in descending order, with text primitives
   * appearing before non-text primitives. This method uses a bubble sort algorithm to reorder the primitives in the
   * group. It compares adjacent primitives and swaps them if they are both text primitives and the first has a lower Y
   * coordinate than the second, or if the first is a non-text primitive and the second is a text primitive (to ensure
   * text primitives are sorted before non-text primitives).
   */
  void SortTextOnY();
  void Square(AeSysView* view);
  void Transform(const EoGeTransformMatrix& transformMatrix);
  void Translate(EoGeVector3d translate);
  void Write(CFile& file);

  /** @brief Writes the group to the specified file, using the provided buffer for temporary storage.  The buffer should
   * be at least EoDbPrimitive::BUFFER_SIZE bytes in size to ensure that all primitive types can be written without risk
   * of overflow.
   * @param file The file to write to.
   * @param buffer A temporary buffer for use in writing primitives.  Should be at least EoDbPrimitive::BUFFER_SIZE
   * bytes in size.
   */
  void Write(CFile& file, std::uint8_t* buffer);
};
