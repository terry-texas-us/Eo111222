#include "Stdafx.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbConic.h"
#include "EoDbDimension.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbHeaderSection.h"
#include "EoDbLayer.h"
#include "EoDbLine.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPegFile.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoDbSpline.h"
#include "EoDbText.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeVector3d.h"
#include "Resource.h"

void EoDbPegFile::Load(AeSysDoc* document) {
  ReadHeaderSection(document);
  ReadTablesSection(document);
  ReadBlocksSection(document);
  ReadEntitiesSection(document);
}

/**
 * Reads the header section from the PEG file into the document's header section.
 * @param document Pointer to the AeSysDoc object where the header section will be populated.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kHeaderSection." if the expected sentinel is not found.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection." if the expected end of section sentinel is not found.
 */
void EoDbPegFile::ReadHeaderSection(AeSysDoc* document) {
  if (EoDb::ReadUInt16(*this) != EoDb::kHeaderSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kHeaderSection.");
  }
  EoDbHeaderSection& headerSection = document->HeaderSection();

  auto& variables = headerSection.GetVariables();
  if (variables.empty()) {
    // Legacy AeSys file
    headerSection.SetVariable(L"$AESVER", HeaderVariable(L"AE2011"));  // AeSys version AE2011
    headerSection.SetVariable(L"$CLAYER", HeaderVariable(L"0"));       // Current Layer default to `0`
    headerSection.SetVariable(L"$PDMODE", HeaderVariable(2));          // Point display mode is `+`
    headerSection.SetVariable(L"$PDSIZE", HeaderVariable(1.0));        // default point size 1.0
  }
  // 	With addition of info here will loop key-value pairs till EoDb::kEndOfSection sentinel

  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection.");
  }
}

/**
 * Reads the tables section from the PEG file into the document's tables.
 * @param document Pointer to the AeSysDoc object where the tables will be populated.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kTablesSection." if the expected sentinel is not found.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection." if the expected end of section sentinel is not found.
 */
void EoDbPegFile::ReadTablesSection(AeSysDoc* document) {
  if (EoDb::ReadUInt16(*this) != EoDb::kTablesSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kTablesSection.");
  }
  ReadViewportTable(document);
  ReadLinetypesTable(document);
  ReadLayerTable(document);

  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection.");
  }
}
void EoDbPegFile::ReadViewportTable(AeSysDoc*) {
  if (EoDb::ReadUInt16(*this) != EoDb::kViewPortTable) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kViewPortTable.");
  }

  if (EoDb::ReadUInt16(*this) != 0) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting number of viewports to be 0.");
  }

  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfTable) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable.");
  }
}

/**
 * Reads the linetype table from the PEG file into the document's linetype table.
 * @param document Pointer to the AeSysDoc object where the linetype table will be populated.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kLinetypeTable." if the expected sentinel is not found.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable." if the expected end of table sentinel is not found.
 */
void EoDbPegFile::ReadLinetypesTable(AeSysDoc* document) {
  auto* lineTypeTable = document->LineTypeTable();

  if (EoDb::ReadUInt16(*this) != EoDb::kLinetypeTable) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kLinetypeTable.");
  }
  std::vector<double> dashElements;

  const auto numberOfLinetypes = EoDb::ReadUInt16(*this);

  for (EoUInt16 n = 0; n < numberOfLinetypes; n++) {
    CString name;
    CString description;
    EoUInt16 definitionLength{};
    ReadLinetypeDefinition(dashElements, name, description, definitionLength);

    EoDbLineType* lineType{};
    if (!lineTypeTable->Lookup(name, lineType)) {
      const auto index = lineTypeTable->LegacyLineTypeIndex(name);
      lineType = new EoDbLineType(index, name, description, definitionLength, dashElements.data());
      lineTypeTable->SetAt(name, lineType);
    }
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"Index: %d - Name: `%s` `%p`\n", n, name.GetString(), lineType);
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfTable) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable.");
  }
}

/**
 * Reads a linetype definition from the PEG file.
 * @param dashLength A reference to a vector that will be populated with the dash lengths of the linetype.
 * @param name A reference to a CString that will be populated with the name of the linetype.
 * @param description A reference to a CString that will be populated with the description of the linetype.
 * @param definitionLength A reference to an EoUInt16 that will be set to the number of dash elements in the linetype.
 */
void EoDbPegFile::ReadLinetypeDefinition(
    std::vector<double>& dashLength, CString& name, CString& description, EoUInt16& definitionLength) {
  EoDb::Read(*this, name);
  uint16_t flags = EoDb::ReadUInt16(*this);
  (void)flags;  // currently unused, but may be used in the future to indicate properties of the linetype
  EoDb::Read(*this, description);
  definitionLength = EoDb::ReadUInt16(*this);
  double PatternLength;
  EoDb::Read(*this, PatternLength);

  dashLength.resize(definitionLength);
  for (EoUInt16 n = 0; n < definitionLength; n++) { EoDb::Read(*this, dashLength[n]); }
}

/**
 * Reads the layer table from the PEG file into the document's layer table.
 * @param document Pointer to the AeSysDoc object where the layer table will be populated.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kLayerTable." if the expected sentinel is not found.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable." if the expected end of table sentinel is not found.
 */
void EoDbPegFile::ReadLayerTable(AeSysDoc* document) {
  if (EoDb::ReadUInt16(*this) != EoDb::kLayerTable) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kLayerTable.");
  }

  CString layerName;
  CString lineTypeName;
  auto numberOfLayers = EoDb::ReadUInt16(*this);
  for (auto n = 0; n < numberOfLayers; n++) {
    EoDb::Read(*this, layerName);

    auto tracingFlags = EoDb::ReadUInt16(*this);
    auto stateFlags = EoDb::ReadUInt16(*this);

    stateFlags |= EoDbLayer::kIsResident;

    if ((stateFlags & EoDbLayer::kIsInternal) != EoDbLayer::kIsInternal) {
      if (layerName.Find('.') == -1) layerName += L".jb1";
    }
    auto colorIndex = EoDb::ReadInt16(*this);
    EoDb::Read(*this, lineTypeName);

    if (document->FindLayerTableLayer(layerName) < 0) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"Added Layer: `%s` to Layer Table\n", layerName.GetString());
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"  Linetype: `%s`\n", lineTypeName.GetString());
      auto* layer = new EoDbLayer(layerName, stateFlags);

      layer->SetTracingFlg(tracingFlags);
      layer->SetColorIndex(colorIndex);

      EoDbLineType* lineType{};
      if (document->LineTypeTable()->Lookup(lineTypeName, lineType)) {
        ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L" Lookup succeeded: `%p`\n", lineType);
        layer->SetLineType(lineType);
      } else {
        ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L" Lookup failed\n");
      }
      document->AddLayerTableLayer(layer);

      if (layer->IsWork()) { document->SetWorkLayer(layer); }
    }
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfTable) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable.");
  }
}

void EoDbPegFile::ReadBlocksSection(AeSysDoc* document) {
  if (EoDb::ReadUInt16(*this) != EoDb::kBlocksSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kBlocksSection.");
  }
  EoDbPrimitive* primitive{};
  CString Name;
  CString XRefPathName;

  auto numberOfBlocks = EoDb::ReadUInt16(*this);

  for (EoUInt16 n = 0; n < numberOfBlocks; n++) {
    auto numberOfPrimitives = EoDb::ReadUInt16(*this);

    EoDb::Read(*this, Name);
    auto blockTypeFlags = EoDb::ReadUInt16(*this);
    EoGePoint3d BasePoint = EoDb::ReadPoint3d(*this);
    auto* block = new EoDbBlock(blockTypeFlags, BasePoint, XRefPathName);
    document->InsertBlock(Name, block);

    for (EoUInt16 PrimitiveIndex = 0; PrimitiveIndex < numberOfPrimitives; PrimitiveIndex++) {
      EoDb::Read(*this, primitive);
      block->AddTail(primitive);
    }
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection.");
  }
}

void EoDbPegFile::ReadEntitiesSection(AeSysDoc* document) {
  if (EoDb::ReadUInt16(*this) != EoDb::kGroupsSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kGroupsSection.");
  }

  EoDbPrimitive* primitive{};

  auto numberOfLayers = EoDb::ReadUInt16(*this);

  for (auto n = 0; n < numberOfLayers; n++) {
    auto* layer = document->GetLayerTableLayerAt(n);

    if (layer == nullptr) { return; }

    auto numberOfGroups = EoDb::ReadUInt16(*this);

    if (layer->IsInternal()) {
      for (auto GroupIndex = 0; GroupIndex < numberOfGroups; GroupIndex++) {
        auto* group = new EoDbGroup;
        auto numberOfPrimitives = EoDb::ReadUInt16(*this);
        for (auto PrimitiveIndex = 0; PrimitiveIndex < numberOfPrimitives; PrimitiveIndex++) {
          EoDb::Read(*this, primitive);
          group->AddTail(primitive);
        }
        layer->AddTail(group);
      }
    } else {
      document->TracingLoadLayer(layer->Name(), layer);
    }
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection.");
  }
}

void EoDbPegFile::Unload(AeSysDoc* document) {
  CFile::SetLength(0);
  CFile::SeekToBegin();

  WriteHeaderSection(document);
  WriteTablesSection(document);
  WriteBlocksSection(document);
  WriteEntitiesSection(document);
  EoDb::Write(*this, L"EOF");

  CFile::Flush();
}
void EoDbPegFile::WriteHeaderSection(AeSysDoc* document) {
  EoDb::Write(*this, EoUInt16(EoDb::kHeaderSection));

  EoDbHeaderSection& headerSection = document->HeaderSection();
  auto& variables = headerSection.GetVariables();

  // write header variant
  for (const auto& [name, value] : variables) {
    // Write the variable name
    EoDb::Write(*this, CString(name.c_str()));

    std::visit(
        [&](auto&& arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, double>) {
            EoDb::Write(*this, arg);
          } else if constexpr (std::is_same_v<T, std::wstring>) {
            EoDb::Write(*this, arg.c_str());
          } else if constexpr (std::is_same_v<T, EoGePoint3d>) {
            arg.Write(*this);
          } else if constexpr (std::is_same_v<T, EoGeVector3d>) {
            arg.Write(*this);
          }
        },
        value);
  }

  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}
void EoDbPegFile::WriteTablesSection(AeSysDoc* document) {
  EoDb::Write(*this, EoUInt16(EoDb::kTablesSection));

  WriteVPortTable(document);
  WriteLinetypeTable(document);
  WriteLayerTable(document);
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}

void EoDbPegFile::WriteVPortTable(AeSysDoc*) {
  EoDb::Write(*this, EoUInt16(EoDb::kViewPortTable));
  EoDb::Write(*this, EoUInt16(0));
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfTable));
}

void EoDbPegFile::WriteLinetypeTable(AeSysDoc* document) {
  auto* lineTypeTable = document->LineTypeTable();

  EoUInt16 numberOfLinetypes = EoUInt16(lineTypeTable->Size());

  EoDb::Write(*this, EoUInt16(EoDb::kLinetypeTable));
  EoDb::Write(*this, numberOfLinetypes);

  CString name;
  EoDbLineType* linetype{};

  auto position = lineTypeTable->GetStartPosition();
  while (position) {
    lineTypeTable->GetNextAssoc(position, name, linetype);

    EoDb::Write(*this, (LPCWSTR)name);
    EoDb::Write(*this, EoUInt16(0));
    EoDb::Write(*this, (LPCWSTR)linetype->Description());

    EoUInt16 DefinitionLength = linetype->GetNumberOfDashes();
    EoDb::Write(*this, DefinitionLength);
    double PatternLength = linetype->GetPatternLength();
    EoDb::Write(*this, PatternLength);

    if (DefinitionLength > 0) {
      double* dashElements = new double[DefinitionLength];
      linetype->GetDashElements(dashElements);
      for (EoUInt16 w = 0; w < DefinitionLength; w++) EoDb::Write(*this, dashElements[w]);

      delete[] dashElements;
    }
  }
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfTable));
}

void EoDbPegFile::WriteLayerTable(AeSysDoc* document) {
  int NumberOfLayers = document->GetLayerTableSize();

  EoDb::Write(*this, EoUInt16(EoDb::kLayerTable));

  auto SavedFilePosition = CFile::GetPosition();
  EoDb::Write(*this, EoUInt16(NumberOfLayers));

  for (int n = 0; n < document->GetLayerTableSize(); n++) {
    EoDbLayer* Layer = document->GetLayerTableLayerAt(n);
    if (Layer->IsResident()) {
      EoDb::Write(*this, Layer->Name());
      EoDb::Write(*this, Layer->GetTracingFlgs());
      EoDb::Write(*this, Layer->LayerStateFlags());
      EoDb::Write(*this, Layer->ColorIndex());
      EoDb::Write(*this, Layer->LineTypeName());
    } else
      NumberOfLayers--;
  }
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfTable));

  if (NumberOfLayers != document->GetLayerTableSize()) {
    auto CurrentFilePosition = CFile::GetPosition();
    CFile::Seek(static_cast<LONGLONG>(SavedFilePosition), CFile::begin);
    EoDb::Write(*this, EoUInt16(NumberOfLayers));
    CFile::Seek(static_cast<LONGLONG>(CurrentFilePosition), CFile::begin);
  }
}

void EoDbPegFile::WriteBlocksSection(AeSysDoc* document) {
  EoDb::Write(*this, EoUInt16(EoDb::kBlocksSection));

  EoUInt16 NumberOfBlocks = document->BlockTableSize();
  EoDb::Write(*this, NumberOfBlocks);

  CString Name;
  EoDbBlock* Block{};

  auto position = document->GetFirstBlockPosition();
  while (position != nullptr) {
    document->GetNextBlock(position, Name, Block);

    auto SavedFilePosition = CFile::GetPosition();
    EoDb::Write(*this, EoUInt16(0));
    EoUInt16 NumberOfPrimitives = 0;

    EoDb::Write(*this, Name);
    EoDb::Write(*this, Block->BlockTypeFlags());
    Block->BasePoint().Write(*this);

    auto PrimitivePosition = Block->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      EoDbPrimitive* primitive = Block->GetNext(PrimitivePosition);
      if (primitive->Write(*this)) NumberOfPrimitives++;
    }
    auto CurrentFilePosition = CFile::GetPosition();
    CFile::Seek(static_cast<LONGLONG>(SavedFilePosition), CFile::begin);
    EoDb::Write(*this, NumberOfPrimitives);
    CFile::Seek(static_cast<LONGLONG>(CurrentFilePosition), CFile::begin);
  }

  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}

void EoDbPegFile::WriteEntitiesSection(AeSysDoc* document) {
  EoDb::Write(*this, EoUInt16(EoDb::kGroupsSection));

  int NumberOfLayers = document->GetLayerTableSize();
  EoDb::Write(*this, EoUInt16(NumberOfLayers));

  for (int n = 0; n < NumberOfLayers; n++) {
    auto* layer = document->GetLayerTableLayerAt(n);
    if (layer->IsInternal()) {
      EoDb::Write(*this, EoUInt16(layer->GetCount()));

      auto position = layer->GetHeadPosition();
      while (position != nullptr) {
        auto* group = layer->GetNext(position);
        group->Write(*this);
      }
    } else {
      EoDb::Write(*this, EoUInt16(0));
    }
  }
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}

bool EoDb::Read(CFile& file, EoDbPrimitive*& primitive) {
  switch (EoDb::ReadUInt16(file)) {
    case EoDb::kPointPrimitive:
      ConstructPointPrimitive(file, primitive);
      break;
    case EoDb::kInsertPrimitive:
      ConstructBlockReferencePrimitiveFromInsertPrimitive(file, primitive);
      break;
    case EoDb::kGroupReferencePrimitive:
      ConstructBlockReferencePrimitive(file, primitive);
      break;
    case EoDb::kLinePrimitive:
      ConstructLinePrimitive(file, primitive);
      break;
    case EoDb::kPolygonPrimitive:
      ConstructPolygonPrimitive(file, primitive);
      break;
    case EoDb::kEllipsePrimitive:
      ConstructEllipsePrimitive(file, primitive);
      break;
    case EoDb::kConicPrimitive:
      ConstructConicPrimitive(file, primitive);
      break;
    case EoDb::kSplinePrimitive:
      ConstructSplinePrimitive(file, primitive);
      break;
    case EoDb::kCSplinePrimitive:
      ConstructPolylinePrimitiveFromCSplinePrimitive(file, primitive);
      break;
    case EoDb::kPolylinePrimitive:
      ConstructPolylinePrimitive(file, primitive);
      break;
    case EoDb::kTextPrimitive:
      ConstructTextPrimitive(file, primitive);
      break;
    case EoDb::kDimensionPrimitive:
      ConstructDimensionPrimitive(file, primitive);
      break;
    case EoDb::kTagPrimitive:
      ConstructPointPrimitiveFromTagPrimitive(file, primitive);
      break;

    default:
      app.WarningMessageBox(IDS_MSG_BAD_PRIM_TYPE);
      return false;
  }
  return true;
}

/**
 * Reads a string from the file until a tab character is encountered.
 *
 * @param file The file to read from.
 * @param string The string to store the read value.
 * @note Use the code page CP_ACP gives you the same pre-Unicode behavior, but now correctly represented in Unicode CString. Will require adding a file format version number at the beginning of the file to read strings as CP_UTF8.
 */
void EoDb::Read(CFile& file, CString& string) {
  string.Empty();

  std::vector<char> buffer;
  char character;

  while (file.Read(&character, 1) == 1) {
    if (character == '\t') break;
    buffer.push_back(character);
  }
  if (buffer.empty()) return;

  int wideLength = MultiByteToWideChar(CP_ACP, 0, buffer.data(), static_cast<int>(buffer.size()), nullptr, 0);

  if (wideLength <= 0) { return; }

  wchar_t* wideBuffer = string.GetBuffer(wideLength);
  if (wideBuffer) {
    MultiByteToWideChar(CP_ACP, 0, buffer.data(), static_cast<int>(buffer.size()), wideBuffer, wideLength);
    string.ReleaseBuffer(wideLength);
  }
}

void EoDb::Read(CFile& file, double& number) { file.Read(&number, sizeof(double)); }
void EoDb::Read(CFile& file, EoInt16& number) { file.Read(&number, sizeof(EoInt16)); }

[[nodiscard]] EoGePoint3d EoDb::ReadPoint3d(CFile& file) {
  EoGePoint3d Point;
  Point.Read(file);
  return Point;
}

[[nodiscard]] EoGeVector3d EoDb::ReadVector3d(CFile& file) {
  EoGeVector3d Vector;
  Vector.Read(file);
  return Vector;
}

void EoDb::Read(CFile& file, EoUInt16& number) { file.Read(&number, sizeof(EoUInt16)); }

[[nodiscard]] double EoDb::ReadDouble(CFile& file) {
  double number;
  file.Read(&number, sizeof(double));
  return number;
}
[[nodiscard]] EoInt16 EoDb::ReadInt16(CFile& file) {
  EoInt16 number;
  file.Read(&number, sizeof(EoInt16));
  return number;
}

[[nodiscard]] EoUInt16 EoDb::ReadUInt16(CFile& file) {
  EoUInt16 number;
  file.Read(&number, sizeof(EoUInt16));
  return number;
}
/**
 * Writes a string to the file using the specified code page.
 *
 * @param file The file to write to.
 * @param string The string to write.
 * @param codePage The code page to use for encoding the string.
 */
void EoDb::Write(CFile& file, const CString& string, UINT codePage) {
  int wideLength = string.GetLength();
  if (wideLength > 0) {
    auto bufferSize =
        static_cast<size_t>(WideCharToMultiByte(codePage, 0, string, wideLength, nullptr, 0, nullptr, nullptr));
    if (bufferSize > 0) {
      char* buffer = new char[bufferSize];
      WideCharToMultiByte(codePage, 0, string, wideLength, buffer, static_cast<int>(bufferSize), nullptr, nullptr);
      file.Write(buffer, static_cast<UINT>(bufferSize));
      delete[] buffer;
    }
  }
  file.Write("\t", 1);
}

void EoDb::Write(CFile& file, double number) { file.Write(&number, sizeof(double)); }
void EoDb::Write(CFile& file, EoInt16 number) { file.Write(&number, sizeof(EoInt16)); }
void EoDb::Write(CFile& file, EoUInt16 number) { file.Write(&number, sizeof(EoUInt16)); }
void EoDb::ConstructBlockReferencePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 color = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);
  CString Name;
  EoDb::Read(file, Name);
  EoGePoint3d Point(EoDb::ReadPoint3d(file));
  EoGeVector3d Normal(EoDb::ReadVector3d(file));
  EoGeVector3d ScaleFactors(EoDb::ReadVector3d(file));
  double Rotation = EoDb::ReadDouble(file);
  uint16_t numberOfColumns = EoDb::ReadUInt16(file);
  (void)numberOfColumns;  // currently unused, but may be used in the future to indicate the number of columns
  EoUInt16 numberOfRows = EoDb::ReadUInt16(file);
  (void)numberOfRows;  // currently unused, but may be used in the future to indicate the number of rows
  double columnSpacing = EoDb::ReadDouble(file);
  (void)columnSpacing;  // currently unused, but may be used in the future to indicate the column spacing
  double rowSpacing = EoDb::ReadDouble(file);
  (void)rowSpacing;  // currently unused, but may be used in the future to indicate the row spacing

  primitive = new EoDbBlockReference(
      static_cast<EoUInt16>(color), static_cast<EoUInt16>(LineType), Name, Point, Normal, ScaleFactors, Rotation);
}
void EoDb::ConstructBlockReferencePrimitiveFromInsertPrimitive(CFile& /* file */, EoDbPrimitive*& /* primitive */) {}

void EoDb::ConstructDimensionPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 color = EoDb::ReadInt16(file);
  EoInt16 lineType = EoDb::ReadInt16(file);
  EoGePoint3d beginPoint = EoDb::ReadPoint3d(file);
  EoGePoint3d endPoint = EoDb::ReadPoint3d(file);
  EoInt16 textColor = EoDb::ReadInt16(file);
  EoDbFontDefinition fontDefinition;
  fontDefinition.Read(file);
  EoGeReferenceSystem referenceSystem;
  referenceSystem.Read(file);
  CString text;
  EoDb::Read(file, text);

  primitive = new EoDbDimension(EoGeLine(beginPoint, endPoint), fontDefinition, referenceSystem, text, textColor);
  primitive->SetColor(color);
  primitive->SetLineTypeIndex(lineType);
}

void EoDb::ConstructConicPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  auto color = EoDb::ReadInt16(file);
  auto lineTypeIndex = EoDb::ReadInt16(file);
  auto center(ReadPoint3d(file));
  auto majorAxis(ReadVector3d(file));
  auto extrusion(ReadVector3d(file));
  double ratio;
  EoDb::Read(file, ratio);
  double startAngle;
  EoDb::Read(file, startAngle);
  double endAngle;
  EoDb::Read(file, endAngle);

  primitive = EoDbConic::CreateConic(center, extrusion, majorAxis, ratio, startAngle, endAngle);

  primitive->SetColor(color);
  primitive->SetLineTypeIndex(lineTypeIndex);
}

/**
 * Constructs a ellipse (now conic) primitive.
 *
 * @param file The `peg` file to read the ellipse data from.
 * @param primitive A reference to a pointer that will hold the constructed conic primitive.
 */
void EoDb::ConstructEllipsePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 color = EoDb::ReadInt16(file);
  EoInt16 lineTypeIndex = EoDb::ReadInt16(file);
  EoGePoint3d center(ReadPoint3d(file));
  EoGeVector3d majorAxis(ReadVector3d(file));
  EoGeVector3d minorAxis(ReadVector3d(file));
  double sweepAngle;
  EoDb::Read(file, sweepAngle);
  primitive = EoDbConic::CreateConicFromEllipsePrimitive(center, majorAxis, minorAxis, sweepAngle);

  primitive->SetColor(color);
  primitive->SetLineTypeIndex(lineTypeIndex);
}

void EoDb::ConstructLinePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 color = EoDb::ReadInt16(file);
  EoInt16 lineTypeIndex = EoDb::ReadInt16(file);
  EoGePoint3d begin(EoDb::ReadPoint3d(file));
  EoGePoint3d end(EoDb::ReadPoint3d(file));
  primitive = new EoDbLine(begin, end);

  primitive->SetColor(color);
  primitive->SetLineTypeIndex(lineTypeIndex);
}

void EoDb::ConstructPointPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  auto PenColor = EoDb::ReadInt16(file);
  auto PointStyle = EoDb::ReadInt16(file);

  EoGePoint3d Point(EoDb::ReadPoint3d(file));
  auto numberOfDatums = EoDb::ReadUInt16(file);

  double* Data = (numberOfDatums == 0) ? 0 : new double[numberOfDatums];

  for (EoUInt16 n = 0; n < numberOfDatums; n++) { EoDb::Read(file, Data[n]); }
  primitive = new EoDbPoint(PenColor, PointStyle, Point, numberOfDatums, Data);
}

void EoDb::ConstructPointPrimitiveFromTagPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 PointStyle = EoDb::ReadInt16(file);
  EoGePoint3d Point(EoDb::ReadPoint3d(file));
  primitive = new EoDbPoint(PenColor, PointStyle, Point);
}

void EoDb::ConstructPolygonPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoDb::PolygonStyle polygonStyle = static_cast<EoDb::PolygonStyle>(EoDb::ReadInt16(file));
  EoInt16 InteriorStyleIndex = EoDb::ReadInt16(file);
  auto numberOfPoints = EoDb::ReadUInt16(file);
  EoGePoint3d HatchOrigin(EoDb::ReadPoint3d(file));
  EoGeVector3d HatchXAxis(EoDb::ReadVector3d(file));
  EoGeVector3d HatchYAxis(EoDb::ReadVector3d(file));

  EoGePoint3dArray Points;
  Points.SetSize(numberOfPoints);
  for (EoUInt16 n = 0; n < numberOfPoints; n++) { Points[n] = EoDb::ReadPoint3d(file); }
  primitive = new EoDbPolygon(PenColor, polygonStyle, InteriorStyleIndex, HatchOrigin, HatchXAxis, HatchYAxis, Points);
}

void EoDb::ConstructPolylinePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);
  auto numberOfPoints = EoDb::ReadUInt16(file);

  EoGePoint3dArray Points;
  Points.SetSize(numberOfPoints);

  for (EoUInt16 n = 0; n < numberOfPoints; n++) { Points[n] = EoDb::ReadPoint3d(file); }
  primitive = new EoDbPolyline(PenColor, LineType, Points);
}

void EoDb::ConstructPolylinePrimitiveFromCSplinePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);

  file.Seek(sizeof(EoUInt16), CFile::current);
  auto numberOfPoints = EoDb::ReadUInt16(file);
  file.Seek(sizeof(EoUInt16), CFile::current);
  file.Seek(sizeof(EoGeVector3d), CFile::current);
  file.Seek(sizeof(EoGeVector3d), CFile::current);
  EoGePoint3dArray Points;
  Points.SetSize(numberOfPoints);
  for (EoUInt16 n = 0; n < numberOfPoints; n++) { Points[n] = EoDb::ReadPoint3d(file); }
  primitive = new EoDbPolyline(PenColor, LineType, Points);
}

void EoDb::ConstructSplinePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);

  auto numberOfPoints = EoDb::ReadUInt16(file);

  EoGePoint3dArray Points;
  Points.SetSize(numberOfPoints);

  for (EoUInt16 n = 0; n < numberOfPoints; n++) { Points[n] = EoDb::ReadPoint3d(file); }
  primitive = new EoDbSpline(PenColor, LineType, Points);
}

void EoDb::ConstructTextPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 color = EoDb::ReadInt16(file);
  (void)color;  // currently unused, but may be used in the future to indicate the text color
  EoInt16 lineType = EoDb::ReadInt16(file);
  (void)lineType;  // currently unused, but may be used in the future to indicate the text line type
  EoDbFontDefinition fontDefinition;
  fontDefinition.Read(file);
  EoGeReferenceSystem referenceSystem;
  referenceSystem.Read(file);
  CString text;
  EoDb::Read(file, text);

  primitive = new EoDbText(fontDefinition, referenceSystem, text);
  static_cast<EoDbText*>(primitive)->ConvertFormattingCharacters();
}
