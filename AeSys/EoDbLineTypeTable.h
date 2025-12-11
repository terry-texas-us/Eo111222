#pragma once

class EoDbLineTypeTable {
 private:
  static const WCHAR* LegacyLineTypes[];
  static const EoUInt16 NumberOfLegacyLineTypes = 42;

  CTypedPtrMap<CMapStringToOb, CString, EoDbLineType*> m_MapLineTypes;

 public:
  int FillComboBox(CComboBox& comboBox);
  int FillListControl(CListCtrl& listControl);
  POSITION GetStartPosition() { return m_MapLineTypes.GetStartPosition(); }
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
