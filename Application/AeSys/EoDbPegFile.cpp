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
#include "EoDbAttrib.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbConic.h"
#include "EoDbFace.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbHeaderSection.h"
#include "EoDbLabeledLine.h"
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
#include "EoDbVPortTableEntry.h"
#include "EoDbViewport.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeVector3d.h"
#include "Resource.h"

// Type discriminator tags
namespace {
constexpr std::uint16_t kTypeTagDouble = 0;
constexpr std::uint16_t kTypeTagInt = 1;
constexpr std::uint16_t kTypeTagWString = 2;
constexpr std::uint16_t kTypeTagPoint3d = 3;
constexpr std::uint16_t kTypeTagVector3d = 4;
constexpr std::uint16_t kTypeTagHandle = 5;
constexpr std::uint16_t kTypeTagBool = 6;
}  // namespace

void EoDbPegFile::Load(AeSysDoc* document) {
  ReadHeaderSection(document);

  auto fileVersion = EoDb::PegFileVersion::AE2011;
  const auto* aesVerVariable = document->HeaderSection().SetVariable(L"$AESVER");
  if (aesVerVariable != nullptr) {
    const auto* versionString = std::get_if<std::wstring>(aesVerVariable);
    if (versionString != nullptr && *versionString == L"AE2026") {
      fileVersion = EoDb::PegFileVersion::AE2026;
    }
  }

  ReadTablesSection(document, fileVersion);
  ReadBlocksSection(document, fileVersion);
  ReadEntitiesSection(document, fileVersion);
  ReadPaperSpaceSection(document, fileVersion);
}

/**
 * Reads the header section from the PEG file into the document's header section.
 *
 * Uses a peek-ahead to distinguish legacy files from V2 files:
 * - **Legacy**: kHeaderSection is followed immediately by kEndOfSection. Default variables
 *   are populated with $AESVER = "AE2011".
 * - **V2**: kHeaderSection is followed by one or more variable triples (name, type tag, value),
 *   terminated by kEndOfSection.
 *
 * The sentinel kEndOfSection (0x01ff) cannot collide with the first two bytes of a
 * tab-terminated variable name (which starts with '$' = 0x24 in CP_ACP), so the peek
 * is unambiguous.
 *
 * @param document Pointer to the AeSysDoc object where the header section will be populated.
 * @throws std::runtime_error if the expected kHeaderSection sentinel is not found.
 * @throws std::runtime_error if an unknown type discriminator tag is encountered.
 */
void EoDbPegFile::ReadHeaderSection(AeSysDoc* document) {
  if (EoDb::ReadUInt16(*this) != EoDb::kHeaderSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kHeaderSection.");
  }
  EoDbHeaderSection& headerSection = document->HeaderSection();

  // Peek at the next uint16_t to distinguish legacy (kEndOfSection) from V2 (variable triples).
  auto peekPosition = CFile::GetPosition();
  auto peekedValue = EoDb::ReadUInt16(*this);

  if (peekedValue == EoDb::kEndOfSection) {
    // Legacy AeSys file — no variables stored on disk; populate defaults
    headerSection.SetVariable(L"$AESVER", HeaderVariable(std::wstring(L"AE2011")));
    headerSection.SetVariable(L"$CLAYER", HeaderVariable(std::wstring(L"0")));
    headerSection.SetVariable(L"$PDMODE", HeaderVariable(2));
    headerSection.SetVariable(L"$PDSIZE", HeaderVariable(1.0));
    return;
  }

  // V2 file: rewind past the peeked bytes and read variable triples
  CFile::Seek(static_cast<LONGLONG>(peekPosition), CFile::begin);

  while (true) {
    // Peek to check for the end-of-section sentinel
    peekPosition = CFile::GetPosition();
    peekedValue = EoDb::ReadUInt16(*this);
    if (peekedValue == EoDb::kEndOfSection) { break; }

    // Not a sentinel — rewind and read the variable name string
    CFile::Seek(static_cast<LONGLONG>(peekPosition), CFile::begin);

    CString variableName;
    EoDb::Read(*this, variableName);

    const auto typeTag = EoDb::ReadUInt16(*this);

    HeaderVariable value;
    switch (typeTag) {
      case kTypeTagDouble:
        value = EoDb::ReadDouble(*this);
        break;
      case kTypeTagInt:
        value = EoDb::ReadInt32(*this);
        break;
      case kTypeTagWString: {
        CString stringValue;
        EoDb::Read(*this, stringValue);
        value = std::wstring(stringValue.GetString());
        break;
      }
      case kTypeTagPoint3d:
        value = EoDb::ReadPoint3d(*this);
        break;
      case kTypeTagVector3d:
        value = EoDb::ReadVector3d(*this);
        break;
      case kTypeTagHandle:
        value = EoDb::ReadUInt64(*this);
        break;
      case kTypeTagBool:
        value = EoDb::ReadUInt16(*this) != 0;
        break;
      default:
        throw std::runtime_error("Exception EoDbPegFile: Unknown header variable type tag.");
    }
    headerSection.SetVariable(std::wstring(variableName.GetString()), value);
  }
}

/**
 * Reads the tables section from the PEG file into the document's tables.
 * @param document Pointer to the AeSysDoc object where the tables will be populated.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kTablesSection." if the expected sentinel is not found.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection." if the expected end of section sentinel is
 * not found.
 */
void EoDbPegFile::ReadTablesSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  if (EoDb::ReadUInt16(*this) != EoDb::kTablesSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kTablesSection.");
  }
  ReadViewportTable(document, fileVersion);
  ReadLinetypesTable(document, fileVersion);
  ReadLayerTable(document, fileVersion);

  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection.");
  }
}
void EoDbPegFile::ReadViewportTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  if (EoDb::ReadUInt16(*this) != EoDb::kViewPortTable) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kViewPortTable.");
  }

  const auto numberOfViewports = EoDb::ReadUInt16(*this);

  if (fileVersion == EoDb::PegFileVersion::AE2026 && numberOfViewports > 0) {
    document->ClearVPortTable();
    for (std::uint16_t n = 0; n < numberOfViewports; n++) {
      EoDbVPortTableEntry entry;
      entry.Read(*this);
      document->AddVPortTableEntry(std::move(entry));
    }
  } else if (numberOfViewports != 0) {
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
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable." if the expected end of table sentinel is not
 * found.
 */
void EoDbPegFile::ReadLinetypesTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  auto* lineTypeTable = document->LineTypeTable();

  if (EoDb::ReadUInt16(*this) != EoDb::kLinetypeTable) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kLinetypeTable.");
  }
  std::vector<double> dashElements;

  const auto numberOfLinetypes = EoDb::ReadUInt16(*this);

  for (std::uint16_t n = 0; n < numberOfLinetypes; n++) {
    CString name;
    CString description;
    std::uint16_t definitionLength{};
    ReadLinetypeDefinition(dashElements, name, description, definitionLength);

    EoDbLineType* lineType{};
    if (!lineTypeTable->Lookup(name, lineType)) {
      const auto index = lineTypeTable->LegacyLineTypeIndex(name);
      lineType = new EoDbLineType(index, name, description, definitionLength, dashElements.data());
      lineTypeTable->SetAt(name, lineType);
    }
    if (fileVersion == EoDb::PegFileVersion::AE2026) {
      lineType->SetHandle(EoDb::ReadUInt64(*this));
      lineType->SetOwnerHandle(EoDb::ReadUInt64(*this));
      document->RegisterHandle(lineType);
    }
    ATLTRACE2(traceGeneral, 2, L"Index: %d - Name: `%s` `%p`\n", n, name.GetString(), lineType);
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
 * @param definitionLength A reference to an std::uint16_t that will be set to the number of dash elements in the
 * linetype.
 */
void EoDbPegFile::ReadLinetypeDefinition(
    std::vector<double>& dashLength, CString& name, CString& description, std::uint16_t& definitionLength) {
  EoDb::Read(*this, name);
  uint16_t flags = EoDb::ReadUInt16(*this);
  (void)flags;  // currently unused, but may be used in the future to indicate properties of the linetype
  EoDb::Read(*this, description);
  definitionLength = EoDb::ReadUInt16(*this);
  double patternLength;
  EoDb::Read(*this, patternLength);

  dashLength.resize(definitionLength);
  for (std::uint16_t n = 0; n < definitionLength; n++) { EoDb::Read(*this, dashLength[n]); }
}

/**
 * Reads the layer table from the PEG file into the document's layer table.
 * @param document Pointer to the AeSysDoc object where the layer table will be populated.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kLayerTable." if the expected sentinel is not found.
 * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable." if the expected end of table sentinel is not
 * found.
 */
void EoDbPegFile::ReadLayerTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  if (EoDb::ReadUInt16(*this) != EoDb::kLayerTable) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kLayerTable.");
  }

  CString layerName;
  CString lineTypeName;
  auto numberOfLayers = EoDb::ReadUInt16(*this);
  for (auto n = 0; n < numberOfLayers; n++) {
    EoDb::Read(*this, layerName);

    auto tracingState = static_cast<std::uint16_t>(EoDb::ReadUInt16(*this));
    auto state = static_cast<std::uint16_t>(EoDb::ReadUInt16(*this));

    state |= std::to_underlying(EoDbLayer::State::isResident);

    if ((state & std::to_underlying(EoDbLayer::State::isInternal)) !=
        std::to_underlying(EoDbLayer::State::isInternal)) {
      if (layerName.Find('.') == -1) { layerName += L".jb1"; }
    }
    auto colorIndex = EoDb::ReadInt16(*this);
    EoDb::Read(*this, lineTypeName);

    std::uint64_t layerHandle{};
    std::uint64_t layerOwnerHandle{};
    EoDxfLineWeights::LineWeight layerLineWeight{EoDxfLineWeights::LineWeight::kLnWtByLwDefault};
    double layerLineTypeScale{1.0};
    bool isFrozen{};
    bool isLocked{};
    bool plottingFlag{true};
    std::int32_t color24{-1};
    if (fileVersion == EoDb::PegFileVersion::AE2026) {
      layerHandle = EoDb::ReadUInt64(*this);
      layerOwnerHandle = EoDb::ReadUInt64(*this);
      auto lineWeightDxfCode = EoDb::ReadInt16(*this);
      layerLineWeight = EoDxfLineWeights::DxfIndexToLineWeight(lineWeightDxfCode);
      layerLineTypeScale = EoDb::ReadDouble(*this);
      auto layerPropertyFlags = EoDb::ReadUInt16(*this);
      isFrozen = (layerPropertyFlags & 0x01) != 0;
      isLocked = (layerPropertyFlags & 0x02) != 0;
      plottingFlag = (layerPropertyFlags & 0x04) != 0;
      color24 = EoDb::ReadInt32(*this);
    }

    if (document->FindLayerTableLayer(layerName) < 0) {
      ATLTRACE2(traceGeneral, 2, L"Added Layer: `%s` to Layer Table\n", layerName.GetString());
      ATLTRACE2(traceGeneral, 2, L"  Linetype: `%s`\n", lineTypeName.GetString());
      auto* layer = new EoDbLayer(layerName, state);

      layer->SetTracingState(tracingState);
      layer->SetColorIndex(colorIndex);
      layer->SetHandle(layerHandle);
      layer->SetOwnerHandle(layerOwnerHandle);
      layer->SetLineWeight(layerLineWeight);
      layer->SetLineTypeScale(layerLineTypeScale);
      layer->SetFrozen(isFrozen);
      layer->SetLocked(isLocked);
      layer->SetPlottingFlag(plottingFlag);
      layer->SetColor24(color24);

      EoDbLineType* lineType{};
      if (document->LineTypeTable()->Lookup(lineTypeName, lineType)) {
        ATLTRACE2(traceGeneral, 2, L" Lookup succeeded: `%p`\n", lineType);
        layer->SetLineType(lineType);
      } else {
        ATLTRACE2(traceGeneral, 2, L" Lookup failed\n");
      }
      document->AddLayerTableLayer(layer);

      if (layer->IsWork()) { document->SetWorkLayer(layer); }
    }
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfTable) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable.");
  }
}

void EoDbPegFile::ReadBlocksSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  if (EoDb::ReadUInt16(*this) != EoDb::kBlocksSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kBlocksSection.");
  }
  EoDbPrimitive* primitive{};
  CString Name;
  CString XRefPathName;

  auto numberOfBlocks = EoDb::ReadUInt16(*this);

  for (std::uint16_t n = 0; n < numberOfBlocks; n++) {
    auto numberOfPrimitives = EoDb::ReadUInt16(*this);

    EoDb::Read(*this, Name);
    auto blockTypeFlags = EoDb::ReadUInt16(*this);
    auto basePoint = EoDb::ReadPoint3d(*this);
    auto* block = new EoDbBlock(blockTypeFlags, basePoint, XRefPathName);
    document->InsertBlock(Name, block);

    if (fileVersion == EoDb::PegFileVersion::AE2026) {
      block->SetHandle(EoDb::ReadUInt64(*this));
      block->SetOwnerHandle(EoDb::ReadUInt64(*this));
    }

    for (std::uint16_t PrimitiveIndex = 0; PrimitiveIndex < numberOfPrimitives; PrimitiveIndex++) {
      if (EoDb::Read(*this, primitive, fileVersion)) {
        document->RegisterHandle(primitive);
        block->AddTail(primitive);
      }
    }
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection.");
  }
}

void EoDbPegFile::ReadEntitiesSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  if (EoDb::ReadUInt16(*this) != EoDb::kGroupsSection) {
    throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kGroupsSection.");
  }

  EoDbPrimitive* primitive{};

  auto numberOfLayers = EoDb::ReadUInt16(*this);

  for (auto n = 0; n < numberOfLayers; n++) {
    auto* layer = document->GetLayerTableLayerAt(n);

    if (layer == nullptr) {
      throw std::runtime_error("Exception EoDbPegFile: Layer table index out of range in entities section.");
    }

    auto numberOfGroups = EoDb::ReadUInt16(*this);

    if (layer->IsInternal()) {
      for (auto GroupIndex = 0; GroupIndex < numberOfGroups; GroupIndex++) {
        auto* group = new EoDbGroup;
        auto numberOfPrimitives = EoDb::ReadUInt16(*this);
        for (auto PrimitiveIndex = 0; PrimitiveIndex < numberOfPrimitives; PrimitiveIndex++) {
          if (EoDb::Read(*this, primitive, fileVersion)) {
            document->RegisterHandle(primitive);
            group->AddTail(primitive);
          }
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

/**
 * Reads the optional paper-space section and the trailing "EOF" marker.
 *
 * After ReadEntitiesSection, the file cursor is positioned immediately after
 * the kEndOfSection sentinel for the model-space groups. What follows depends
 * on the file version and content:
 *
 * - **V1 (AE2011)**: "EOF" string → end of file.
 * - **V2 (AE2026), no paper-space**: "EOF" string → end of file.
 * - **V2 (AE2026), with paper-space**: kPaperSpaceSection → paper-space
 *   layer table + entity groups → kEndOfSection → "EOF" string → end of file.
 *
 * The method peeks at the next uint16_t to distinguish kPaperSpaceSection
 * (0x0105) from the start of the "EOF" string (first two CP_ACP bytes
 * are 'E','O' → 0x4F45 little-endian), so the peek is unambiguous.
 *
 * @param document Pointer to the AeSysDoc that receives paper-space layers.
 */
void EoDbPegFile::ReadPaperSpaceSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  auto currentPosition = CFile::GetPosition();
  auto fileLength = CFile::GetLength();

  if (currentPosition >= fileLength) { return; }

  // Peek at the next uint16_t to check for kPaperSpaceSection before "EOF".
  auto peekPosition = CFile::GetPosition();
  auto peekedSentinel = EoDb::ReadUInt16(*this);

  if (peekedSentinel == EoDb::kPaperSpaceSection) {
    // --- Paper-space layer table ---
    if (EoDb::ReadUInt16(*this) != EoDb::kLayerTable) {
      throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kLayerTable in paper-space section.");
    }

    CString layerName;
    CString lineTypeName;
    auto numberOfLayers = EoDb::ReadUInt16(*this);

    for (auto n = 0; n < numberOfLayers; n++) {
      EoDb::Read(*this, layerName);
      auto tracingState = static_cast<std::uint16_t>(EoDb::ReadUInt16(*this));
      auto state = static_cast<std::uint16_t>(EoDb::ReadUInt16(*this));
      state |= std::to_underlying(EoDbLayer::State::isResident);

      if ((state & std::to_underlying(EoDbLayer::State::isInternal)) !=
          std::to_underlying(EoDbLayer::State::isInternal)) {
        if (layerName.Find('.') == -1) { layerName += L".jb1"; }
      }
      auto colorIndex = EoDb::ReadInt16(*this);
      EoDb::Read(*this, lineTypeName);

      std::uint64_t layerHandle{};
      std::uint64_t layerOwnerHandle{};
      EoDxfLineWeights::LineWeight layerLineWeight{EoDxfLineWeights::LineWeight::kLnWtByLwDefault};
      double layerLineTypeScale{1.0};
      if (fileVersion == EoDb::PegFileVersion::AE2026) {
        layerHandle = EoDb::ReadUInt64(*this);
        layerOwnerHandle = EoDb::ReadUInt64(*this);
        auto lineWeightDxfCode = EoDb::ReadInt16(*this);
        layerLineWeight = EoDxfLineWeights::DxfIndexToLineWeight(lineWeightDxfCode);
        layerLineTypeScale = EoDb::ReadDouble(*this);
      }

      if (document->FindLayerInSpace(layerName, EoDxf::Space::PaperSpace) == nullptr) {
        auto* layer = new EoDbLayer(layerName, state);
        layer->SetTracingState(tracingState);
        layer->SetColorIndex(colorIndex);
        layer->SetHandle(layerHandle);
        layer->SetOwnerHandle(layerOwnerHandle);
        layer->SetLineWeight(layerLineWeight);
        layer->SetLineTypeScale(layerLineTypeScale);

        EoDbLineType* lineType{};
        if (document->LineTypeTable()->Lookup(lineTypeName, lineType)) { layer->SetLineType(lineType); }
        document->AddLayerToSpace(layer, EoDxf::Space::PaperSpace);
      }
    }

    if (EoDb::ReadUInt16(*this) != EoDb::kEndOfTable) {
      throw std::runtime_error("Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable in paper-space section.");
    }

    // --- Paper-space entities ---
    if (EoDb::ReadUInt16(*this) != EoDb::kGroupsSection) {
      throw std::runtime_error(
          "Exception EoDbPegFile: Expecting sentinel EoDb::kGroupsSection in paper-space section.");
    }

    EoDbPrimitive* primitive{};
    auto& paperLayers = document->SpaceLayers(EoDxf::Space::PaperSpace);
    auto numberOfEntityLayers = EoDb::ReadUInt16(*this);

    for (auto n = 0; n < numberOfEntityLayers; n++) {
      auto* layer = (n < static_cast<int>(paperLayers.GetSize())) ? paperLayers.GetAt(n) : nullptr;

      if (layer == nullptr) {
        throw std::runtime_error("Exception EoDbPegFile: Paper-space layer table index out of range.");
      }

      auto numberOfGroups = EoDb::ReadUInt16(*this);

      if (layer->IsInternal()) {
        for (auto groupIndex = 0; groupIndex < numberOfGroups; groupIndex++) {
          auto* group = new EoDbGroup;
          auto numberOfPrimitives = EoDb::ReadUInt16(*this);
          for (auto primitiveIndex = 0; primitiveIndex < numberOfPrimitives; primitiveIndex++) {
            if (EoDb::Read(*this, primitive, fileVersion)) {
              document->RegisterHandle(primitive);
              group->AddTail(primitive);
            }
          }
          layer->AddTail(group);
        }
      } else {
        document->TracingLoadLayer(layer->Name(), layer);
      }
    }

    if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
      throw std::runtime_error(
          "Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection for paper-space entities.");
    }

    if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
      throw std::runtime_error(
          "Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection for paper-space section.");
    }
  } else {
    // Not a paper-space section — rewind past the peeked bytes.
    CFile::Seek(static_cast<LONGLONG>(peekPosition), CFile::begin);
  }

  // Consume the trailing "EOF" marker (present in both V1 and V2 files).
  currentPosition = CFile::GetPosition();
  if (currentPosition < fileLength) {
    CString eofMarker;
    EoDb::Read(*this, eofMarker);
  }
}

void EoDbPegFile::Unload(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  CFile::SetLength(0);
  CFile::SeekToBegin();

  WriteHeaderSection(document, fileVersion);
  WriteTablesSection(document, fileVersion);
  WriteBlocksSection(document, fileVersion);
  WriteEntitiesSection(document, fileVersion);
  WritePaperSpaceSection(document, fileVersion);
  EoDb::Write(*this, L"EOF");

  CFile::Flush();
}
/**
 * Writes the header section to the PEG file.
 *
 * AE2026+ format: each header variable is written as a triple:
 *   - variable name  (tab-terminated string)
 *   - type tag       (uint16_t discriminator: 0=double, 1=int, 2=wstring, 3=Point3d, 4=Vector3d)
 *   - value          (type-specific payload)
 *
 * The sentinel kEndOfSection (0x01ff) cannot collide with the first two bytes of a tab-terminated variable name
 * (which starts with '$' = 0x24), so ReadHeaderSection can distinguish legacy the files sentinel kEndOfSection immediately
 * after kHeaderSection) from V2 files (variable triples followed by kEndOfSection) by peeking at the next uint16_t.
 *
 * @param document Pointer to the AeSysDoc that owns the header section.
 * @param fileVersion The PEG file version to write.
 * @note For legacy files, only the kHeaderSection + kEndOfSection sentinels are written, and defaults are not
 * serialized.
 */
void EoDbPegFile::WriteHeaderSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kHeaderSection));

  if (fileVersion == EoDb::PegFileVersion::AE2011) {
    EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfSection));
    return;
  }

  EoDbHeaderSection& headerSection = document->HeaderSection();

  constexpr const wchar_t* kCurrentAeSysVersion = L"AE2026";
  headerSection.SetVariable(L"$AESVER", HeaderVariable(std::wstring(kCurrentAeSysVersion)));

  const auto& variables = headerSection.GetVariables();
  for (const auto& [name, value] : variables) {
    EoDb::Write(*this, CString(name.c_str()));

    std::visit(
        [&](auto&& arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, double>) {
            EoDb::WriteUInt16(*this, kTypeTagDouble);
            EoDb::WriteDouble(*this, arg);
          } else if constexpr (std::is_same_v<T, int>) {
            EoDb::WriteUInt16(*this, kTypeTagInt);
            EoDb::WriteInt32(*this, static_cast<std::int32_t>(arg));
          } else if constexpr (std::is_same_v<T, std::wstring>) {
            EoDb::WriteUInt16(*this, kTypeTagWString);
            EoDb::Write(*this, CString(arg.c_str()));
          } else if constexpr (std::is_same_v<T, EoGePoint3d>) {
            EoDb::WriteUInt16(*this, kTypeTagPoint3d);
            arg.Write(*this);
          } else if constexpr (std::is_same_v<T, EoGeVector3d>) {
            EoDb::WriteUInt16(*this, kTypeTagVector3d);
            arg.Write(*this);
          } else if constexpr (std::is_same_v<T, std::uint64_t>) {
            EoDb::WriteUInt16(*this, kTypeTagHandle);
            EoDb::WriteUInt64(*this, arg);
          } else if constexpr (std::is_same_v<T, bool>) {
            EoDb::WriteUInt16(*this, kTypeTagBool);
            EoDb::WriteUInt16(*this, arg ? 1 : 0);
          }
        },
        value);
  }
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfSection));
}

void EoDbPegFile::WriteTablesSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kTablesSection));

  WriteVPortTable(document, fileVersion);
  WriteLinetypeTable(document, fileVersion);
  WriteLayerTable(document, fileVersion);
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfSection));
}

void EoDbPegFile::WriteVPortTable(AeSysDoc* document, [[maybe_unused]] EoDb::PegFileVersion fileVersion) {
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kViewPortTable));

  if (fileVersion == EoDb::PegFileVersion::AE2026) {
    const auto& vportTable = document->VPortTable();
    EoDb::WriteUInt16(*this, static_cast<std::uint16_t>(vportTable.size()));
    for (const auto& entry : vportTable) { entry.Write(*this); }
  } else {
    EoDb::WriteUInt16(*this, std::uint16_t(0));
  }

  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfTable));
}

void EoDbPegFile::WriteLinetypeTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  auto* lineTypeTable = document->LineTypeTable();

  auto numberOfLinetypes = std::uint16_t(lineTypeTable->Size());

  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kLinetypeTable));
  EoDb::WriteUInt16(*this, numberOfLinetypes);

  CString name;
  EoDbLineType* linetype{};

  auto position = lineTypeTable->GetStartPosition();
  while (position) {
    lineTypeTable->GetNextAssoc(position, name, linetype);

    EoDb::Write(*this, name);
    EoDb::WriteUInt16(*this, std::uint16_t(0));
    EoDb::Write(*this, linetype->Description());

    auto numberOfDashes = linetype->GetNumberOfDashes();
    EoDb::WriteUInt16(*this, numberOfDashes);
    double patternLength = linetype->GetPatternLength();
    EoDb::WriteDouble(*this, patternLength);

    const auto& dashElements = linetype->DashElements();
    for (std::uint16_t i = 0; i < numberOfDashes; i++) { EoDb::WriteDouble(*this, dashElements[i]); }

    if (fileVersion == EoDb::PegFileVersion::AE2026) {
      EoDb::WriteUInt64(*this, linetype->Handle());
      EoDb::WriteUInt64(*this, linetype->OwnerHandle());
    }
  }
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfTable));
}

void EoDbPegFile::WriteLayerTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  auto& layers = document->SpaceLayers(EoDxf::Space::ModelSpace);
  int numberOfLayers = static_cast<int>(layers.GetSize());

  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kLayerTable));

  auto savedFilePosition = CFile::GetPosition();
  EoDb::WriteUInt16(*this, std::uint16_t(numberOfLayers));

  for (INT_PTR n = 0; n < layers.GetSize(); n++) {
    EoDbLayer* layer = layers.GetAt(n);
    if (layer->IsResident()) {
      EoDb::Write(*this, layer->Name());
      EoDb::WriteUInt16(*this, layer->GetTracingState());
      EoDb::WriteUInt16(*this, layer->GetState());
      EoDb::WriteInt16(*this, layer->ColorIndex());
      EoDb::Write(*this, layer->LineTypeName());
      if (fileVersion == EoDb::PegFileVersion::AE2026) {
        EoDb::WriteUInt64(*this, layer->Handle());
        EoDb::WriteUInt64(*this, layer->OwnerHandle());
        EoDb::WriteInt16(*this, EoDxfLineWeights::LineWeightToDxfIndex(layer->LineWeight()));
        EoDb::WriteDouble(*this, layer->LineTypeScale());
        std::uint16_t layerPropertyFlags{};
        if (layer->IsFrozen()) { layerPropertyFlags |= 0x01; }
        if (layer->IsLocked()) { layerPropertyFlags |= 0x02; }
        if (layer->PlottingFlag()) { layerPropertyFlags |= 0x04; }
        EoDb::WriteUInt16(*this, layerPropertyFlags);
        EoDb::WriteInt32(*this, layer->Color24());
      }
    } else {
      numberOfLayers--;
    }
  }
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfTable));

  if (numberOfLayers != static_cast<int>(layers.GetSize())) {
    auto currentFilePosition = CFile::GetPosition();
    CFile::Seek(static_cast<LONGLONG>(savedFilePosition), CFile::begin);
    EoDb::WriteUInt16(*this, std::uint16_t(numberOfLayers));
    CFile::Seek(static_cast<LONGLONG>(currentFilePosition), CFile::begin);
  }
}

void EoDbPegFile::WriteBlocksSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kBlocksSection));

  std::uint16_t numberOfBlocks = document->BlockTableSize();
  EoDb::WriteUInt16(*this, numberOfBlocks);

  CString name;
  EoDbBlock* block{};

  auto position = document->GetFirstBlockPosition();
  while (position != nullptr) {
    document->GetNextBlock(position, name, block);
    auto savedFilePosition = CFile::GetPosition();
    EoDb::WriteUInt16(*this, std::uint16_t(0));
    std::uint16_t numberOfPrimitives{};

    EoDb::Write(*this, name);
    EoDb::WriteUInt16(*this, block->BlockTypeFlags());
    block->BasePoint().Write(*this);

    if (fileVersion == EoDb::PegFileVersion::AE2026) {
      EoDb::WriteUInt64(*this, block->Handle());
      EoDb::WriteUInt64(*this, block->OwnerHandle());
    }

    auto primitivePosition = block->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = block->GetNext(primitivePosition);
      if (primitive->Write(*this)) {
        if (fileVersion == EoDb::PegFileVersion::AE2026) {
          EoDb::WriteUInt64(*this, primitive->Handle());
          EoDb::WriteUInt64(*this, primitive->OwnerHandle());
          EoDb::WriteInt16(*this, EoDxfLineWeights::LineWeightToDxfIndex(primitive->LineWeight()));
          EoDb::WriteDouble(*this, primitive->LineTypeScale());
        }
        numberOfPrimitives++;
      }
    }
    auto currentFilePosition = CFile::GetPosition();
    CFile::Seek(static_cast<LONGLONG>(savedFilePosition), CFile::begin);
    EoDb::WriteUInt16(*this, numberOfPrimitives);
    CFile::Seek(static_cast<LONGLONG>(currentFilePosition), CFile::begin);
  }

  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfSection));
}

void EoDbPegFile::WriteEntitiesSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kGroupsSection));

  auto& layers = document->SpaceLayers(EoDxf::Space::ModelSpace);

  // Count only resident layers to match WriteLayerTable's patched count.
  // Non-resident layers are not written to the layer table, so their entity
  // data must also be omitted to keep layer indices aligned on read.
  int numberOfResidentLayers = 0;
  for (INT_PTR n = 0; n < layers.GetSize(); n++) {
    if (layers.GetAt(n)->IsResident()) { numberOfResidentLayers++; }
  }
  EoDb::WriteUInt16(*this, std::uint16_t(numberOfResidentLayers));

  for (INT_PTR n = 0; n < layers.GetSize(); n++) {
    auto* layer = layers.GetAt(n);
    if (!layer->IsResident()) { continue; }

    if (layer->IsInternal()) {
      EoDb::WriteUInt16(*this, std::uint16_t(layer->GetCount()));

      auto position = layer->GetHeadPosition();
      while (position != nullptr) {
        auto* group = layer->GetNext(position);
        group->Write(*this, fileVersion);
      }
    } else {
      EoDb::WriteUInt16(*this, std::uint16_t(0));
    }
  }
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfSection));
}

/**
 * Writes the paper-space section before the "EOF" marker.
 *
 * AE2026 format: the paper-space section is a first-class section between the
 * model-space entities and the "EOF" marker. V1 readers (which stop after
 * ReadEntitiesSection) never encounter it because they do not read past the
 * groups section's kEndOfSection sentinel.
 *
 * Layout:
 *   kPaperSpaceSection (0x0105)
 *     kLayerTable → count → layer entries → kEndOfTable
 *     kGroupsSection → count → entity groups → kEndOfSection
 *   kEndOfSection
 *
 * If there are no paper-space layers, or the file version is AE2011, nothing
 * is written.
 *
 * @param document Pointer to the AeSysDoc that owns the paper-space layers.
 * @param fileVersion The PEG file version being written.
 */
void EoDbPegFile::WritePaperSpaceSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion) {
  if (fileVersion == EoDb::PegFileVersion::AE2011) { return; }

  auto& layers = document->SpaceLayers(EoDxf::Space::PaperSpace);

  if (layers.GetSize() == 0) { return; }

  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kPaperSpaceSection));

  // --- Paper-space layer table ---
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kLayerTable));

  int numberOfLayers = static_cast<int>(layers.GetSize());
  auto savedLayerCountPosition = CFile::GetPosition();
  EoDb::WriteUInt16(*this, std::uint16_t(numberOfLayers));

  for (INT_PTR n = 0; n < layers.GetSize(); n++) {
    EoDbLayer* layer = layers.GetAt(n);
    if (layer->IsResident()) {
      EoDb::Write(*this, layer->Name());
      EoDb::WriteUInt16(*this, layer->GetTracingState());
      EoDb::WriteUInt16(*this, layer->GetState());
      EoDb::WriteInt16(*this, layer->ColorIndex());
      EoDb::Write(*this, layer->LineTypeName());
      if (fileVersion == EoDb::PegFileVersion::AE2026) {
        EoDb::WriteUInt64(*this, layer->Handle());
        EoDb::WriteUInt64(*this, layer->OwnerHandle());
        EoDb::WriteInt16(*this, EoDxfLineWeights::LineWeightToDxfIndex(layer->LineWeight()));
        EoDb::WriteDouble(*this, layer->LineTypeScale());
        std::uint16_t layerPropertyFlags{};
        if (layer->IsFrozen()) { layerPropertyFlags |= 0x01; }
        if (layer->IsLocked()) { layerPropertyFlags |= 0x02; }
        if (layer->PlottingFlag()) { layerPropertyFlags |= 0x04; }
        EoDb::WriteUInt16(*this, layerPropertyFlags);
        EoDb::WriteInt32(*this, layer->Color24());
      }
    } else {
      numberOfLayers--;
    }
  }
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfTable));

  if (numberOfLayers != static_cast<int>(layers.GetSize())) {
    auto currentFilePosition = CFile::GetPosition();
    CFile::Seek(static_cast<LONGLONG>(savedLayerCountPosition), CFile::begin);
    EoDb::WriteUInt16(*this, std::uint16_t(numberOfLayers));
    CFile::Seek(static_cast<LONGLONG>(currentFilePosition), CFile::begin);
  }

  // --- Paper-space entities ---
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kGroupsSection));

  int numberOfResidentLayers = 0;
  for (INT_PTR n = 0; n < layers.GetSize(); n++) {
    if (layers.GetAt(n)->IsResident()) { numberOfResidentLayers++; }
  }
  EoDb::WriteUInt16(*this, std::uint16_t(numberOfResidentLayers));

  for (INT_PTR n = 0; n < layers.GetSize(); n++) {
    auto* layer = layers.GetAt(n);
    if (!layer->IsResident()) { continue; }

    if (layer->IsInternal()) {
      EoDb::WriteUInt16(*this, std::uint16_t(layer->GetCount()));

      auto position = layer->GetHeadPosition();
      while (position != nullptr) {
        auto* group = layer->GetNext(position);
        group->Write(*this, fileVersion);
      }
    } else {
      EoDb::WriteUInt16(*this, std::uint16_t(0));
    }
  }
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfSection));

  // End of paper-space section
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfSection));
}

bool EoDb::Read(CFile& file, EoDbPrimitive*& primitive, EoDb::PegFileVersion fileVersion) {
  switch (EoDb::ReadUInt16(file)) {
    case EoDb::kPointPrimitive:
      primitive = EoDbPoint::ReadFromPeg(file);
      break;
    case EoDb::kInsertPrimitive:
      primitive = EoDbBlockReference::ReadLegacyInsertPeg(file);
      break;
    case EoDb::kGroupReferencePrimitive:
      primitive = EoDbBlockReference::ReadFromPeg(file);
      break;
    case EoDb::kLinePrimitive:
      primitive = EoDbLine::ReadFromPeg(file);
      break;
    case EoDb::kPolygonPrimitive:
      primitive = EoDbPolygon::ReadFromPeg(file);
      break;
    case EoDb::kFacePrimitive:
      primitive = EoDbFace::ReadFromPeg(file);
      break;
    case EoDb::kEllipsePrimitive:
      primitive = EoDbConic::ReadFromLegacyEllipsePeg(file);
      break;
    case EoDb::kConicPrimitive:
      primitive = EoDbConic::ReadFromPeg(file);
      break;
    case EoDb::kSplinePrimitive:
      primitive = EoDbSpline::ReadFromPeg(file);
      break;
    case EoDb::kCSplinePrimitive:
      primitive = EoDbPolyline::ReadFromCSplinePeg(file);
      break;
    case EoDb::kPolylinePrimitive:
      primitive = EoDbPolyline::ReadFromPeg(file);
      break;
    case EoDb::kTextPrimitive:
      primitive = EoDbText::ReadFromPeg(file);
      break;
    case EoDb::kAttribPrimitive:
      primitive = EoDbAttrib::ReadFromPeg(file);
      break;
    case EoDb::kDimensionPrimitive:
      primitive = EoDbLabeledLine::ReadFromPeg(file);
      break;
    case EoDb::kTagPrimitive:
      primitive = EoDbPoint::ReadFromLegacyTagPeg(file);
      break;
    case EoDb::kViewportPrimitive:
      primitive = EoDbViewport::ReadFromPeg(file);
      break;

    default:
      app.WarningMessageBox(IDS_MSG_BAD_PRIM_TYPE);
      return false;
  }
  if (fileVersion == EoDb::PegFileVersion::AE2026) {
    primitive->SetHandle(EoDb::ReadUInt64(file));
    primitive->SetOwnerHandle(EoDb::ReadUInt64(file));
    primitive->SetLineWeight(EoDxfLineWeights::DxfIndexToLineWeight(EoDb::ReadInt16(file)));
    primitive->SetLineTypeScale(EoDb::ReadDouble(file));
    primitive->ReadV2Extension(file);
  }
  return true;
}

/**
 * Reads a string from the file until a tab character is encountered.
 *
 * @param file The file to read from.
 * @param string The string to store the read value.
 * @note Use the code page CP_ACP gives you the same pre-Unicode behavior, but now correctly represented in Unicode
 * CString. Will require adding a file format version number at the beginning of the file to read strings as CP_UTF8.
 */
void EoDb::Read(CFile& file, CString& string) {
  string.Empty();

  std::vector<char> buffer;
  char character;

  while (file.Read(&character, 1) == 1) {
    if (character == '\t') { break; }
    buffer.push_back(character);
  }
  if (buffer.empty()) { return; }

  int wideLength = MultiByteToWideChar(CP_ACP, 0, buffer.data(), static_cast<int>(buffer.size()), nullptr, 0);

  if (wideLength <= 0) { return; }

  wchar_t* wideBuffer = string.GetBuffer(wideLength);
  if (wideBuffer) {
    MultiByteToWideChar(CP_ACP, 0, buffer.data(), static_cast<int>(buffer.size()), wideBuffer, wideLength);
    string.ReleaseBuffer(wideLength);
  }
}

void EoDb::Read(CFile& file, double& number) { file.Read(&number, sizeof(double)); }
void EoDb::Read(CFile& file, std::int16_t& number) { file.Read(&number, sizeof(std::int16_t)); }

EoGePoint3d EoDb::ReadPoint3d(CFile& file) {
  EoGePoint3d point;
  point.Read(file);
  return point;
}

EoGeVector3d EoDb::ReadVector3d(CFile& file) {
  EoGeVector3d vector;
  vector.Read(file);
  return vector;
}

void EoDb::Read(CFile& file, std::uint16_t& number) { file.Read(&number, sizeof(std::uint16_t)); }

double EoDb::ReadDouble(CFile& file) {
  double number;
  file.Read(&number, sizeof(double));
  return number;
}

std::int8_t EoDb::ReadInt8(CFile& file) {
  std::int8_t number;
  file.Read(&number, sizeof(std::int8_t));
  return number;
}

std::int16_t EoDb::ReadInt16(CFile& file) {
  std::int16_t number;
  file.Read(&number, sizeof(std::int16_t));
  return number;
}

std::int32_t EoDb::ReadInt32(CFile& file) {
  std::int32_t number;
  file.Read(&number, sizeof(std::int32_t));
  return number;
}

std::uint16_t EoDb::ReadUInt16(CFile& file) {
  std::uint16_t number;
  file.Read(&number, sizeof(std::uint16_t));
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
      std::vector<char> buffer(bufferSize);
      WideCharToMultiByte(
          codePage, 0, string, wideLength, buffer.data(), static_cast<int>(bufferSize), nullptr, nullptr);
      file.Write(buffer.data(), static_cast<UINT>(bufferSize));
    }
  }
  file.Write("\t", 1);
}

void EoDb::WriteDouble(CFile& file, double number) { file.Write(&number, sizeof(double)); }
void EoDb::WriteInt8(CFile& file, std::int8_t number) { file.Write(&number, sizeof(std::int8_t)); }
void EoDb::WriteInt16(CFile& file, std::int16_t number) { file.Write(&number, sizeof(std::int16_t)); }
void EoDb::WriteInt32(CFile& file, std::int32_t number) { file.Write(&number, sizeof(std::int32_t)); }
void EoDb::WriteUInt16(CFile& file, std::uint16_t number) { file.Write(&number, sizeof(std::uint16_t)); }


std::uint64_t EoDb::ReadUInt64(CFile& file) {
  std::uint64_t number;
  file.Read(&number, sizeof(std::uint64_t));
  return number;
}

void EoDb::WriteUInt64(CFile& file, std::uint64_t number) { file.Write(&number, sizeof(std::uint64_t)); }
