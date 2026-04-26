#pragma once

#include <cstdint>
#include <vector>

class AeSysDoc;

class EoDbPegFile : public CFile {
 public:
  EoDbPegFile() {};
  EoDbPegFile(const EoDbPegFile&) = delete;
  EoDbPegFile& operator=(const EoDbPegFile&) = delete;

  virtual ~EoDbPegFile() {}

  [[nodiscard]] CString FileName() const { return m_strFileName; }

  /** @brief Loads the PEG file data into the provided AeSysDoc object.
   *
   * This method orchestrates the loading process by sequentially reading each section of the PEG file:
   * 1. Header Section: Reads header variables, using a peek-ahead to distinguish between legacy and V2 formats.
   * 2. Tables Section: Reads viewport, linetype, layer, text style, and optionally layout tables based on file version.
   * 3. Blocks Section: Reads block definitions and their associated entities.
   * 4. Entities Section: Reads model space entities and populates the document's model space layers.
   * 5. Paper Space Section: Reads paper space layouts, their layers, and entities.
   *
   * The method uses the file version (determined from the header) to handle differences in expected sections and
   * formats.
   *
   * @param document Pointer to the AeSysDoc object where the PEG file data will be loaded.
   * @throws std::runtime_error if any expected section sentinels are not found or if unknown data formats are
   * encountered.
   */
  void Load(AeSysDoc* document);
  void ReadBlocksSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void ReadEntitiesSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);

  /** @brief Reads the header section from the PEG file into the document's header section.
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
  void ReadHeaderSection(AeSysDoc* document);

  /** @brief Reads the layer table from the PEG file into the document's layer table.
   * @param document Pointer to the AeSysDoc object where the layer table will be populated.
   * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kLayerTable." if the expected sentinel is not found.
   * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable." if the expected end of table sentinel is
   * not found.
   */
  void ReadLayerTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void ReadLayoutTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);

  /** @brief Reads the linetype table from the PEG file into the document's linetype table.
   * @param document Pointer to the AeSysDoc object where the linetype table will be populated.
   * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kLinetypeTable." if the expected sentinel is not found.
   * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable." if the expected end of table sentinel is
   * not found.
   */
  void ReadLinetypesTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);

  /** @brief Reads a linetype definition from the PEG file.
   * @param dashLength A reference to a vector that will be populated with the dash lengths of the linetype.
   * @param name A reference to a CString that will be populated with the name of the linetype.
   * @param description A reference to a CString that will be populated with the description of the linetype.
   * @param definitionLength A reference to an std::uint16_t that will be set to the number of dash elements in the
   * linetype.
   */
  void ReadLinetypeDefinition(std::vector<double>& dashLength,
      CString& name,
      CString& description,
      std::uint16_t& definitionLength);
  void ReadPaperSpaceSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void ReadPaperSpaceLayoutLayers(AeSysDoc* document, EoDb::PegFileVersion fileVersion, std::uint64_t layoutHandle);
  void ReadPaperSpaceLayoutEntities(AeSysDoc* document, EoDb::PegFileVersion fileVersion, std::uint64_t layoutHandle);

  /** @brief Reads the tables section from the PEG file, including viewport, linetype, layer, text style, and optionally
   * layout tables.
   *
   * The presence of the layout table is determined by peeking for its sentinel before the kEndOfSection sentinel.
   *
   * @param document Pointer to the AeSysDoc object where the tables will be populated.
   * @param fileVersion The version of the PEG file being read, which may affect which tables are expected and how they
   * are read.
   * @throws std::runtime_error if the expected kTablesSection sentinel is not found.
   * @throws std::runtime_error if the expected kEndOfSection sentinel is not found at the end of the section.
   */
  void ReadTablesSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void ReadTextStyleTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);

  /** @brief Reads the viewport table from the PEG file into the document's viewport table.
   * @param document Pointer to the AeSysDoc object where the viewport table will be populated.
   * @param fileVersion The version of the PEG file being read, used to determine expected data format.
   * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kViewPortTable." if the expected sentinel is not found.
   * @throws L"Exception EoDbPegFile: Expecting number of viewports to be 0." if a legacy file contains non-zero
   * viewports.
   * @throws L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable." if the expected end of table sentinel is
   * not found.
   */
  void ReadViewportTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void Unload(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteBlocksSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteEntitiesSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteHeaderSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteLayerTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteLayoutTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteLinetypeTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WritePaperSpaceSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteTablesSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteTextStyleTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteVPortTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
};
