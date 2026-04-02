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

  /** @brief Converts a line type name to its corresponding legacy line type index.
   * If the name matches "ByBlock" or "ByLayer", returns the special sentinel indices.
   * Otherwise, searches the legacy line types array (DXF name then display name) for a
   * case-insensitive match and returns its index.
   * If no match is found, returns 1 (the index for "CONTINUOUS").
   * @param name The line type name to convert.
   * @return The corresponding legacy line type index, or 1 if not found.
   */
  static std::int16_t LegacyLineTypeIndex(const CString& name);

  static std::int16_t LegacyLineTypeIndex(const std::wstring& name);

  /**
   * Retrieves the legacy line type name corresponding to a given index.
   * If the index is within the range of defined legacy line types, returns the associated name.
   * Otherwise, returns "CONTINUOUS" as a default.
   * @param index The index of the legacy line type.
   * @return The name of the legacy line type as a std::wstring.
   */
  [[nodiscard]] static std::wstring LegacyLineTypeName(std::int16_t index) noexcept;

  /** @brief Looks up a line type by its name (case-insensitive).
   *
   * This method searches for a line type in the collection based on its name.
   * The comparison is case-insensitive because DXF linetype names are case-insensitive.
   *
   * @param name The name of the line type to look up.
   * @param lineType A reference to a pointer that will be set to the found line type, if any.
   * @return true if the line type was found; false otherwise.
   */
  [[nodiscard]] bool Lookup(const CString& name, EoDbLineType*& lineType);

  /** @brief Looks up a line type using its legacy index.
   *
   * This method searches for a line type in the collection based on its legacy index.
   *
   * @param index The legacy index of the line type to look up.
   * @param lineType A reference to a pointer that will be set to the found line type, if any.
   * @return true if the line type was found; false otherwise.
   */
  bool LookupUsingLegacyIndex(std::uint16_t index, EoDbLineType*& lineType);

  void SetAt(const CString& name, EoDbLineType* lineType) { m_MapLineTypes.SetAt(name, lineType); }

  /** @brief Counts the number of references to a specific line type in the document.
   * @param lineTypeName The line type name to count references for.
   * @return The number of references to the specified line type.
   */
  static int ReferenceCount(const std::wstring& lineTypeName);

  int Size() { return (int)m_MapLineTypes.GetSize(); }
  /// @brief Loads the Line Type table.
  void LoadLineTypesFromTxtFile(const CString& pathName);
  void RemoveAll();
  void RemoveUnused();
};