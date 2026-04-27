#include "Stdafx.h"

#include <cstdlib>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"

namespace {

enum class ElementType { Text, Shape };

using SymbolContent = std::variant<int, std::wstring>;
struct EoDbLineTypeSymbol {
  double length;  // Dash length (often negative for the space occupied by the element)
  double scale;  // Scale factor (S value)
  double rotation;  // Rotation angle (R or U value)
  double xOffset;  // X offset
  double yOffset;  // Y offset
  std::wstring file;  // Text style name (e.g., "Standard" or SHP/SHX file name)
  SymbolContent content;  // Text string (e.g. "HW" or shape number)
  ElementType type;  // Text or Shape
  int absoluteRotation;  // 0 for relative rotation (No), 1 for absolute
  int upright;  // 0 for no upright orientation (No), 1 for upright (keeps text readable)
};

constexpr std::uint16_t maxNumberOfDashElementsDefault{8};

const std::pair<const wchar_t*, const wchar_t*> legacyLineTypes[] = {{L"Null", L"0"},
    {L"CONTINUOUS", L"CONTINUOUS"},
    {L"Dash2", L"2"},
    {L"Dash", L"3"},
    {L"DashX2", L"4"},
    {L"Center2", L"5"},
    {L"DashX2-dot", L"6"},
    {L"Divide2", L"7"},
    {L"DashX2-triple-dot", L"8"},
    {L"Dot", L"9"},
    {L"Center", L"10"},
    {L"DashX4-dot", L"11"},
    {L"Divide", L"12"},
    {L"DashX4-triple-dot", L"13"},
    {L"CenterX2", L"14"},
    {L"DashX8-dot", L"15"},
    {L"DivideX2", L"16"},
    {L"DashX8-triple-dot", L"17"},
    {L"BORDER", L"BORDER"},
    {L"BORDER2", L"BORDER2"},
    {L"BORDERX2", L"BORDERX2"},
    {L"CENTER", L"CENTER"},
    {L"CENTER2", L"CENTER2"},
    {L"CENTERX2", L"CENTERX2"},
    {L"DASHDOT", L"DASHDOT"},
    {L"DASHDOT2", L"DASHDOT2"},
    {L"DASHDOTX2", L"DASHDOTX2"},
    {L"DASHED", L"DASHED"},
    {L"DASHED2", L"DASHED2"},
    {L"DASHEDX2", L"DASHEDX2"},
    {L"DIVIDE", L"DIVIDE"},
    {L"DIVIDE2", L"DIVIDE2"},
    {L"DIVIDEX2", L"DIVIDEX2"},
    {L"DOT", L"DOT"},
    {L"DOT2", L"DOT2"},
    {L"DOTX2", L"DOTX2"},
    {L"HIDDEN", L"HIDDEN"},
    {L"HIDDEN2", L"HIDDEN2"},
    {L"HIDDENX2", L"HIDDENX2"},
    {L"PHANTOM", L"PHANTOM"},
    {L"PHANTOM2", L"PHANTOM2"},
    {L"PHANTOMX2", L"PHANTOMX2"}};

constexpr std::uint16_t numberOfLegacyLineTypes{size(legacyLineTypes)};
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

int EoDbLineTypeTable::FillListControl(CListCtrl& listControl) {
  listControl.SetRedraw(FALSE);
  listControl.DeleteAllItems();

  int item{};
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

std::int16_t EoDbLineTypeTable::LegacyLineTypeIndex(const CString& name) {
  if (name.CompareNoCase(L"ByBlock") == 0) { return EoDbPrimitive::LINETYPE_BYBLOCK; }
  if (name.CompareNoCase(L"ByLayer") == 0) { return EoDbPrimitive::LINETYPE_BYLAYER; }

  // Search by DXF / numeric name (second element of legacy pair)
  for (std::int16_t index = 0; index < numberOfLegacyLineTypes; ++index) {
    if (name.CompareNoCase(legacyLineTypes[index].second) == 0) { return index; }
  }

  // Fallback: search by display name (first element of legacy pair)
  for (std::int16_t index = 0; index < numberOfLegacyLineTypes; ++index) {
    if (name.CompareNoCase(legacyLineTypes[index].first) == 0) { return index; }
  }

  return 1;
}

std::int16_t EoDbLineTypeTable::LegacyLineTypeIndex(const std::wstring& name) {
  CString cstrName(name.c_str());
  return LegacyLineTypeIndex(cstrName);
}
std::wstring EoDbLineTypeTable::LegacyLineTypeName(std::int16_t index) noexcept {
  if (index == EoDbPrimitive::LINETYPE_BYLAYER) { return L"ByLayer"; }
  if (index == EoDbPrimitive::LINETYPE_BYBLOCK) { return L"ByBlock"; }
  if (index >= 0 && index < numberOfLegacyLineTypes) { return std::wstring(legacyLineTypes[index].second); }
  return L"CONTINUOUS";
}

bool EoDbLineTypeTable::Lookup(const CString& name, EoDbLineType*& lineType) {
  lineType = nullptr;
  POSITION position = m_MapLineTypes.GetStartPosition();
  while (position != nullptr) {
    CString key;
    EoDbLineType* candidate{};
    m_MapLineTypes.GetNextAssoc(position, key, candidate);
    if (key.CompareNoCase(name) == 0) {
      lineType = candidate;
      return true;
    }
  }
  return false;
}

bool EoDbLineTypeTable::LookupUsingLegacyIndex(std::uint16_t index, EoDbLineType*& lineType) {
  lineType = nullptr;
  return (index < numberOfLegacyLineTypes) && m_MapLineTypes.Lookup(legacyLineTypes[index].second, lineType);
}

/**
 * Counts the number of references to a specific line type in the document.
 * @param lineTypeName The line type name to count references for.
 * @return The number of references to the specified line type.
 */
int EoDbLineTypeTable::ReferenceCount(const std::wstring& lineTypeName) {
  auto* document = AeSysDoc::GetDoc();

  int count{};

  for (auto i = 0; i < document->GetLayerTableSize(); i++) {
    auto* layer = document->GetLayerTableLayerAt(i);
    count += layer->GetLineTypeRefCount(lineTypeName);
  }

  CString key;
  EoDbBlock* block{};

  const auto* blocksTable = document->BlocksTable();
  auto position = blocksTable->GetStartPosition();
  while (position != nullptr) {
    blocksTable->GetNextAssoc(position, key, block);
    count += block->GetLineTypeRefCount(lineTypeName);
  }
  return count;
}

void EoDbLineTypeTable::LoadLineTypesFromTxtFile(const CString& pathName) {
  std::uint16_t maxNumberOfDashElements{maxNumberOfDashElementsDefault};
  std::vector<double> dashLengths;
  dashLengths.resize(maxNumberOfDashElements);

  CStdioFile file;
  CFileException e;
  if (file.Open(pathName, CFile::modeRead | CFile::typeText, &e)) {
    CString inputLine;

    while (file.ReadString(inputLine) != 0) {
      if (inputLine.IsEmpty() || inputLine[0] == L';') { continue; }

      int nextToken{};
      const auto label = static_cast<std::int16_t>(_wtoi(inputLine.Tokenize(L"=", nextToken)));

      auto name = inputLine.Tokenize(L",", nextToken);
      auto comment = inputLine.Tokenize(L"\n", nextToken);
      comment.Trim();

      // Second line definines dash lengths
      file.ReadString(inputLine);
      // Determine number of dash elements by counting commas
      std::uint16_t numberOfDashElements = 0;
      for (int i = 0; i < inputLine.GetLength(); i++) {
        if (inputLine[i] == L',') { numberOfDashElements++; }
      }
      numberOfDashElements++;  // Account for the last element

      if (numberOfDashElements > maxNumberOfDashElements) {
        dashLengths.resize(numberOfDashElements);
        maxNumberOfDashElements = numberOfDashElements;
      }
      nextToken = 0;
      for (std::uint16_t i = 0; i < numberOfDashElements; i++) {
        dashLengths[i] = _wtof(inputLine.Tokenize(L",\n", nextToken));
      }
      EoDbLineType* lineType;
      if (!Lookup(name, lineType)) {
        m_MapLineTypes.SetAt(name, new EoDbLineType(label, name, comment, numberOfDashElements, dashLengths.data()));
        ATLTRACE2(traceGeneral, 2, L"%d - Name: %s `%s`\n", label, name.GetString(), comment.GetString());
        for (size_t i = 0; i < numberOfDashElements; ++i) {
          ATLTRACE2(traceGeneral, 2, L"  Dash[%d] = %f\n", i, dashLengths[i]);
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

  auto position = m_MapLineTypes.GetStartPosition();
  while (position) {
    m_MapLineTypes.GetNextAssoc(position, name, lineType);
    delete lineType;
  }
  m_MapLineTypes.RemoveAll();
}

void EoDbLineTypeTable::RemoveUnused() noexcept {
  // Note: test new logic for RemoveUnused required since LookupName is gone
  // int i = m_LineTypes.GetSize();
  // while (--i != 0) {
  //	EoDbLineType* LineType = m_LineTypes[i];
  //	std::int16_t LineTypeIndex = LookupName((const wchar_t*) LineType->Name());
  //	if (ReferenceCount(LineTypeIndex) != 0 || i < 10) {
  //		m_LineTypes.SetSize(i + 1);
  //		break;
  //	}
  //	delete LineType;
  // }
}
