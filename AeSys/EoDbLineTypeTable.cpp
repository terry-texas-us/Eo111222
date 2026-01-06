#include "stdafx.h"
#include <Windows.h>
#include <afx.h>
#include <afxcmn.h>
#include <afxstr.h>
#include <afxwin.h>
#include <atltrace.h>
#include <cstdlib>
#include <vector>

#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDbLayer.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"

namespace {
constexpr EoUInt16 maxNumberOfDashElementsDefault{8};

const wchar_t* legacyLineTypes[] = {
    L"0",      L"Continuous", L"2",        L"3",       L"4",        L"5",        L"6",       L"7",        L"8",
    L"9",      L"10",         L"11",       L"12",      L"13",       L"14",       L"15",      L"16",       L"17",
    L"BORDER", L"BORDER2",    L"BORDERX2", L"CENTER",  L"CENTER2",  L"CENTERX2", L"DASHDOT", L"DASHDOT2", L"DASHDOTX2",
    L"DASHED", L"DASHED2",    L"DASHEDX2", L"DIVIDE",  L"DIVIDE2",  L"DIVIDEX2", L"DOT",     L"DOT2",     L"DOTX2",
    L"HIDDEN", L"HIDDEN2",    L"HIDDENX2", L"PHANTOM", L"PHANTOM2", L"PHANTOMX2"};
constexpr EoUInt16 NumberOfLegacyLineTypes{42};
}  // namespace

/**
 * Assignment operator for EoDbLineTypeTable.
 * Performs a deep copy of the line types from another EoDbLineTypeTable.
 * @param other The other EoDbLineTypeTable to copy from.
 * @return A reference to this EoDbLineTypeTable.
 */
EoDbLineTypeTable& EoDbLineTypeTable::operator=(const EoDbLineTypeTable& other) {
  if (this != &other) {
    RemoveAll();
    POSITION position = other.m_MapLineTypes.GetStartPosition();
    while (position != nullptr) {
      CString name;
      EoDbLineType* otherLineType = nullptr;
      other.m_MapLineTypes.GetNextAssoc(position, name, otherLineType);
      if (otherLineType != nullptr) {
        auto* thisLineType = new EoDbLineType(*otherLineType);
        m_MapLineTypes.SetAt(name, thisLineType);
      }
    }
  }
  return *this;
}

int EoDbLineTypeTable::FillComboBox(CComboBox& comboBox) {
  comboBox.ResetContent();

  CString name;
  EoDbLineType* lineType;
  auto position = m_MapLineTypes.GetStartPosition();
  while (position) {
    m_MapLineTypes.GetNextAssoc(position, name, lineType);
    int itemIndex = comboBox.AddString(name);
    comboBox.SetItemData(itemIndex, DWORD_PTR(lineType));
  }
  return (static_cast<int>(m_MapLineTypes.GetSize()));
}

/**
 * Fills a list control with the line types in the line type table.
 * @param listControl The list control to fill.
 * @return The number of line types added to the list control.
 */
int EoDbLineTypeTable::FillListControl(CListCtrl& listControl) {
  listControl.SetRedraw(FALSE);
  listControl.DeleteAllItems();

  int item{0};
  CString name;
  EoDbLineType* lineType;
  auto position = m_MapLineTypes.GetStartPosition();
  while (position) {
    m_MapLineTypes.GetNextAssoc(position, name, lineType);

    listControl.InsertItem(item, name);
    listControl.SetItemData(item++, DWORD_PTR(lineType));
  }
  return (static_cast<int>(m_MapLineTypes.GetSize()));
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
    while (Index < NumberOfLegacyLineTypes && name.CompareNoCase(legacyLineTypes[Index]) != 0) { Index++; }
    Index = (Index < NumberOfLegacyLineTypes) ? Index : 0U;
  }
  return Index;
}

bool EoDbLineTypeTable::Lookup(const CString& name, EoDbLineType*& lineType) {
  lineType = nullptr;
  return (m_MapLineTypes.Lookup(name, lineType) != 0);
}

bool EoDbLineTypeTable::__Lookup(EoUInt16 index, EoDbLineType*& lineType) {
  lineType = nullptr;
  return (index < NumberOfLegacyLineTypes) && m_MapLineTypes.Lookup(legacyLineTypes[index], lineType);
}

int EoDbLineTypeTable::ReferenceCount(EoInt16 lineType) {
  auto* document = AeSysDoc::GetDoc();

  int Count = 0;

  for (EoUInt16 w = 0; w < document->GetLayerTableSize(); w++) {
    EoDbLayer* Layer = document->GetLayerTableLayerAt(w);
    Count += Layer->GetLineTypeRefCount(lineType);
  }

  CString Key;
  EoDbBlock* Block;

  CBlocks* BlocksTable = document->BlocksTable();
  auto Position = BlocksTable->GetStartPosition();
  while (Position != nullptr) {
    BlocksTable->GetNextAssoc(Position, Key, Block);
    Count += Block->GetLineTypeRefCount(lineType);
  }
  return (Count);
}

void EoDbLineTypeTable::LoadLineTypesFromTxtFile(const CString& pathName) {
  EoUInt16 maxNumberOfDashElements{maxNumberOfDashElementsDefault};
  std::vector<double> dashLengths;
  dashLengths.resize(maxNumberOfDashElements);

  CStdioFile file;
  CFileException e;
  if (file.Open(pathName, CFile::modeRead | CFile::typeText, &e)) {
    CString inputLine;

    while (file.ReadString(inputLine) != 0) {
      if (inputLine.IsEmpty() || inputLine[0] == L';') { continue; }

      int nextToken{0};
      auto label = EoUInt16(_wtoi(inputLine.Tokenize(L"=", nextToken)));

      auto name = inputLine.Tokenize(L",", nextToken);
      auto comment = inputLine.Tokenize(L"\n", nextToken);
      comment.Trim();

      // Second line definines dash lengths
      file.ReadString(inputLine);
      // Determine number of dash elements by counting commas
      EoUInt16 numberOfDashElements = 0;
      for (int i = 0; i < inputLine.GetLength(); i++) {
        if (inputLine[i] == L',') { numberOfDashElements++; }
      }
      numberOfDashElements++;  // Account for the last element

      if (numberOfDashElements > maxNumberOfDashElements) {
        dashLengths.resize(numberOfDashElements);
        maxNumberOfDashElements = numberOfDashElements;
      }
      nextToken = 0;
      for (EoUInt16 i = 0; i < numberOfDashElements; i++) {
        dashLengths[i] = _wtof(inputLine.Tokenize(L",\n", nextToken));
      }
      EoDbLineType* lineType;
      if (!Lookup(name, lineType)) {
        m_MapLineTypes.SetAt(name, new EoDbLineType(label, name, comment, numberOfDashElements, dashLengths.data()));
        ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"%d - Name: %s `%s`\n", label, name.GetString(),
                  comment.GetString());
        for (size_t i = 0; i < numberOfDashElements; ++i) {
          ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"  Dash[%d] = %f\n", i, dashLengths[i]);
        }
      }
    }
  } else {
    ATLTRACE2(L"Exception opening file: %s", pathName.GetString());
  }
}

void EoDbLineTypeTable::RemoveAll() {
  CString name;
  EoDbLineType* lineType;

  auto Position = m_MapLineTypes.GetStartPosition();
  while (Position) {
    m_MapLineTypes.GetNextAssoc(Position, name, lineType);
    delete lineType;
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
