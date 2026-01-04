#pragma once
#include "EoDbLineType.h"
#include <Windows.h>
#include <afx.h>
#include <afxcmn.h>
#include <afxcoll.h>
#include <afxstr.h>
#include <afxtempl.h>
#include <afxwin.h>

/**
 * @brief Manages a collection of line types for a CAD application.
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
  EoDbLineTypeTable& operator=(const EoDbLineTypeTable&) = delete;

  int FillComboBox(CComboBox& comboBox);
  int FillListControl(CListCtrl& listControl);
  auto GetStartPosition() { return m_MapLineTypes.GetStartPosition(); }
  void GetNextAssoc(POSITION& position, CString& name, EoDbLineType*& lineType) {
    m_MapLineTypes.GetNextAssoc(position, name, lineType);
  }
  EoUInt16 LegacyLineTypeIndex(const CString& name);
  bool Lookup(const CString& name, EoDbLineType*& lineType);
  bool __Lookup(EoUInt16 index, EoDbLineType*& lineType);
  void SetAt(const CString& name, EoDbLineType* lineType) { m_MapLineTypes.SetAt(name, lineType); }
  int ReferenceCount(EoInt16 lineType);
  int Size() { return (int)m_MapLineTypes.GetSize(); }
  /// <summary>Loads the Line Type table.</summary>
  void LoadLineTypesFromTxtFile(const CString& pathName);
  void RemoveAll();
  /// <summary>Removes line types which have no references.</summary>
  void RemoveUnused();
};