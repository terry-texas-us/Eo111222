#include "stdafx.h"
#include "AeSysDoc.h"

#include "EoDbLineTypeTable.h"

const WCHAR* EoDbLineTypeTable::LegacyLineTypes[] = {
    L"0",      L"Continuous", L"2",        L"3",       L"4",        L"5",        L"6",       L"7",        L"8",
    L"9",      L"10",         L"11",       L"12",      L"13",       L"14",       L"15",      L"16",       L"17",
    L"BORDER", L"BORDER2",    L"BORDERX2", L"CENTER",  L"CENTER2",  L"CENTERX2", L"DASHDOT", L"DASHDOT2", L"DASHDOTX2",
    L"DASHED", L"DASHED2",    L"DASHEDX2", L"DIVIDE",  L"DIVIDE2",  L"DIVIDEX2", L"DOT",     L"DOT2",     L"DOTX2",
    L"HIDDEN", L"HIDDEN2",    L"HIDDENX2", L"PHANTOM", L"PHANTOM2", L"PHANTOMX2"};
int EoDbLineTypeTable::FillComboBox(CComboBox& comboBox) {
  comboBox.ResetContent();

  CString Name;
  EoDbLineType* LineType;
  POSITION Position = m_MapLineTypes.GetStartPosition();
  while (Position) {
    m_MapLineTypes.GetNextAssoc(Position, Name, LineType);
    int ItemIndex = comboBox.AddString(Name);
    comboBox.SetItemData(ItemIndex, DWORD_PTR(LineType));
  }
  return (m_MapLineTypes.GetSize());
}
int EoDbLineTypeTable::FillListControl(CListCtrl& listControl) {
  CString Name;
  int ItemIndex = 0;
  EoDbLineType* LineType;
  POSITION Position = m_MapLineTypes.GetStartPosition();
  while (Position) {
    m_MapLineTypes.GetNextAssoc(Position, Name, LineType);

    listControl.InsertItem(ItemIndex, NULL);
    listControl.SetItemData(ItemIndex++, DWORD_PTR(LineType));
  }
  return (m_MapLineTypes.GetSize());
}
/// <remarks>
/// ByBlock and ByLayer should not be permitted in a legacy file. This should be managed in the outbound conversions back to legacy file.
/// </remarks>
EoUInt16 EoDbLineTypeTable::LegacyLineTypeIndex(const CString& name) {
  EoUInt16 Index = 0;
  if (name.CompareNoCase(L"ByBlock") == 0) {
    Index = EoDbPrimitive::LINETYPE_BYBLOCK;
  } else if (name.CompareNoCase(L"ByLayer") == 0) {
    Index = EoDbPrimitive::LINETYPE_BYLAYER;
  } else {
    while (Index < NumberOfLegacyLineTypes && name.CompareNoCase(LegacyLineTypes[Index]) != 0) { Index++; }
    Index = (Index < NumberOfLegacyLineTypes) ? Index : 0;
  }
  return Index;
}
bool EoDbLineTypeTable::Lookup(const CString& name, EoDbLineType*& lineType) {
  lineType = NULL;
  return (m_MapLineTypes.Lookup(name, lineType) != 0);
}
bool EoDbLineTypeTable::__Lookup(EoUInt16 index, EoDbLineType*& lineType) {
  lineType = NULL;
  return (index < NumberOfLegacyLineTypes) && m_MapLineTypes.Lookup(LegacyLineTypes[index], lineType);
}
int EoDbLineTypeTable::ReferenceCount(EoInt16 lineType) {
  AeSysDoc* Document = AeSysDoc::GetDoc();

  int Count = 0;

  for (EoUInt16 w = 0; w < Document->GetLayerTableSize(); w++) {
    EoDbLayer* Layer = Document->GetLayerTableLayerAt(w);
    Count += Layer->GetLineTypeRefCount(lineType);
  }

  CString Key;
  EoDbBlock* Block;

  CBlocks* BlocksTable = Document->BlocksTable();
  POSITION Position = BlocksTable->GetStartPosition();
  while (Position != NULL) {
    BlocksTable->GetNextAssoc(Position, Key, Block);
    Count += Block->GetLineTypeRefCount(lineType);
  }
  return (Count);
}
void EoDbLineTypeTable::LoadLineTypesFromTxtFile(const CString& pathName) {
  CStdioFile fl;

  if (fl.Open(pathName, CFile::modeRead | CFile::typeText)) {
    CString Description;
    CString Name;
    CString Line;

    EoUInt16 MaxNumberOfDashElements = 8;
    double* DashLengths = new double[MaxNumberOfDashElements];

    while (fl.ReadString(Line) != 0) {
      int NextToken = 0;
      EoUInt16 Label = EoUInt16(_wtoi(Line.Tokenize(L"=", NextToken)));

      Name = Line.Tokenize(L",", NextToken);
      Description = Line.Tokenize(L"\n", NextToken);

      fl.ReadString(Line);

      NextToken = 0;
      EoUInt16 NumberOfDashElements = EoUInt16(_wtoi(Line.Tokenize(L",\n", NextToken)));

      if (NumberOfDashElements > MaxNumberOfDashElements) {
        delete[] DashLengths;
        DashLengths = new double[NumberOfDashElements];
        MaxNumberOfDashElements = NumberOfDashElements;
      }
      for (EoUInt16 Index = 0; Index < NumberOfDashElements; Index++) {
        DashLengths[Index] = _wtof(Line.Tokenize(L",\n", NextToken));
      }
      EoDbLineType* LineType;
      if (!Lookup(Name, LineType)) {
        m_MapLineTypes.SetAt(Name, new EoDbLineType(Label, Name, Description, NumberOfDashElements, DashLengths));
      }
    }
    delete[] DashLengths;
  }
}
void EoDbLineTypeTable::RemoveAll() {
  CString Name;
  EoDbLineType* LineType;

  POSITION Position = m_MapLineTypes.GetStartPosition();
  while (Position) {
    m_MapLineTypes.GetNextAssoc(Position, Name, LineType);
    delete LineType;
  }
  m_MapLineTypes.RemoveAll();
}
void EoDbLineTypeTable::RemoveUnused() {
  //Note: test new logic for RemoveUnused required since LookupName is gone
  //int i = m_LineTypes.GetSize();
  //while (--i != 0) {
  //	EoDbLineType* LineType = m_LineTypes[i];
  //	EoInt16 LineTypeIndex = LookupName((LPCWSTR) LineType->Name());
  //	if (ReferenceCount(LineTypeIndex) != 0 || i < 10) {
  //		m_LineTypes.SetSize(i + 1);
  //		break;
  //	}
  //	delete LineType;
  //}
}
