#pragma once

#include <string>

#include "EoDbLineType.h"

/**
 * @class Manages the collection of line types.
 *
 * This class provides functionality to store, retrieve, and manage line types,
 * including legacy line types and custom-defined types. It supports operations
 * such as filling UI elements (combo boxes and list controls) with line type data,
 * looking up line types by name or index, counting references to line types,
 * loading line types from text files, and removing unused line types.
 */
class EoDbLineTypeTable {
 private:
  CTypedPtrMap<CMapStringToOb, CString, EoDbLineType*> m_MapLineTypes;

 public:
  EoDbLineTypeTable() = default;
  EoDbLineTypeTable(const EoDbLineTypeTable&) = delete;

  EoDbLineTypeTable& operator=(const EoDbLineTypeTable& other);
  ~EoDbLineTypeTable() { RemoveAll(); }

  int FillListControl(CListCtrl& listControl);
  auto GetStartPosition() { return m_MapLineTypes.GetStartPosition(); }
  void GetNextAssoc(POSITION& position, CString& name, EoDbLineType*& lineType) {
    m_MapLineTypes.GetNextAssoc(position, name, lineType);
  }
  bool IsEmpty() { return m_MapLineTypes.IsEmpty(); }
  
  EoInt16 LegacyLineTypeIndex(CString& name);
  EoInt16 LegacyLineTypeIndex(std::wstring& name);
  
  /** @brief Looks up a line type by its name.
   *
   * This method searches for a line type in the collection based on its name.
   *
   * @param name The name of the line type to look up.
   * @param lineType A reference to a pointer that will be set to the found line type, if any.
   * @return true if the line type was found; false otherwise.
   */
  bool Lookup(const CString& name, EoDbLineType*& lineType);

  /** @brief Looks up a line type using its legacy index.
   *
   * This method searches for a line type in the collection based on its legacy index.
   *
   * @param index The legacy index of the line type to look up.
   * @param lineType A reference to a pointer that will be set to the found line type, if any.
   * @return true if the line type was found; false otherwise.
   */
  bool LookupUsingLegacyIndex(EoUInt16 index, EoDbLineType*& lineType);
  
  void SetAt(const CString& name, EoDbLineType* lineType) { m_MapLineTypes.SetAt(name, lineType); }

  /** @brief Counts the number of references to a specific line type in the document.
   * @param lineType The line type index to count references for.
   * @return The number of references to the specified line type.
   */
  int ReferenceCount(EoInt16 lineType);
  
  int Size() { return (int)m_MapLineTypes.GetSize(); }
  /// <summary>Loads the Line Type table.</summary>
  void LoadLineTypesFromTxtFile(const CString& pathName);
  void RemoveAll();
  void RemoveUnused();
};