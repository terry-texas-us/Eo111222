#pragma once

#include <cstdint>
#include <string>

#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeVector3d.h"

class EoGsRenderDevice;

namespace EoDb {
enum class PegFileVersion : std::uint16_t;
}  // namespace EoDb

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
  static void SetPrimitiveToIgnore(EoDbPrimitive* primitive) noexcept { sm_PrimitiveToIgnore = primitive; }

 public:
  EoDbGroup();
  EoDbGroup(EoDbPrimitive* primitive);

  EoDbGroup& operator=(const EoDbGroup&) = delete;

  ~EoDbGroup() override {}

  EoDbGroup(const EoDbGroup& group);
  EoDbGroup(const EoDbBlock& block);

  void AddPrimsToTreeViewControl(HWND tree, HTREEITEM parent);
  HTREEITEM AddToTreeViewControl(HWND tree, HTREEITEM parent);
  void BreakPolylines();

  /** @brief Replaces block reference primitives in the group with the primitives from the referenced blocks,
   * transformed according to the block reference's properties.  This is a recursive process that continues until no
   * block reference primitives remain in the group.
   */
  void ExplodeBlockReferences();
  void DeletePrimitivesAndRemoveAll();
  void Display(AeSysView* view, EoGsRenderDevice* renderDevice);
  bool FindAndRemovePrim(EoDbPrimitive* primitive);
  EoDbPrimitive* GetAt(POSITION position);
  int GetBlockRefCount(const CString& blockName);
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&);
  EoDbPrimitive* GetNext(POSITION& position) const { return ((EoDbPrimitive*)CObList::GetNext(position)); }
  EoDbPoint* GetFirstDifferentPoint(const EoDbPoint* pointPrimitive);
  int GetLineTypeRefCount(const std::wstring& lineTypeName);
  void InsertBefore(POSITION position, const EoDbGroup* group);
  bool IsInView(AeSysView* view);
  void ModifyNotes(const EoDbFontDefinition& fontDefinition,
      const EoDbCharacterCellDefinition& characterCellDefinition,
      int attributes = 0);
  void ModifyColor(std::int16_t color);
  void ModifyLineType(const std::wstring& lineTypeName);
  void PenTranslation(std::uint16_t, const std::int16_t* newColors, const std::int16_t* sourceColors);

  /** @brief Removes duplicate primitives from the group.  Two primitives are considered duplicates if their
   * Identical() method returns true.  The first instance of a primitive is kept, and subsequent duplicates are removed
   * and deleted.
   */
  void RemoveDuplicatePrimitives();
  int RemoveEmptyNotesAndDelete();
  EoDbPrimitive* SelPrimAtCtrlPt(AeSysView* view, const EoGePoint4d&, EoGePoint3d*);
  /// @brief Picks a prim if close enough to point.  Working in view space.
  EoDbPrimitive* SelPrimUsingPoint(AeSysView* view, const EoGePoint4d&, double&, EoGePoint3d&);
  bool SelectUsingLine(AeSysView* view, const EoGePoint3d& pt1, const EoGePoint3d& pt2);
  bool SelectUsingPoint_(AeSysView* view, EoGePoint4d point);
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
  void Translate(EoGeVector3d v);
  void Write(CFile& file) const;

  /** @brief Writes the group's primitives to a file, including additional handle information for AE2026 file version.
   *
   * This method first writes the number of primitives in the group as a 16-bit unsigned integer. Then, for each
   * primitive in the group, it calls the primitive's Write method to write its data to the file. If the file version is
   * AE2026, it also writes the primitive's handle and owner handle as 64-bit unsigned integers.
   *
   * @param file The file to which the group's primitives will be written.
   * @param fileVersion The version of the PEG file format being written, which determines whether additional handle
   * information is included.
   */
  void Write(CFile& file, EoDb::PegFileVersion fileVersion) const;

  /** @brief Writes the group data to a buffer for file output.
   *
   * This method writes the group flags and the number of primitives in the group to the provided buffer. It then
   * iterates through each primitive in the group and calls its Write method to write its data to the buffer.
   *
   * @param file The file to which the data will be written (not used in this method but may be needed for primitive
   * Write calls).
   * @param buffer A pointer to a byte buffer where the group data will be written. The buffer should be large enough to
   * hold the group flags, primitive count, and all primitive data.
   */
  void Write(CFile& file, std::uint8_t* buffer);
};
