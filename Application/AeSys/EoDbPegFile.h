#pragma once

#include <vector>

class AeSysDoc;

class EoDbPegFile : public CFile {
 public:
  EoDbPegFile() {};
  EoDbPegFile(const EoDbPegFile&) = delete;
  EoDbPegFile& operator=(const EoDbPegFile&) = delete;

  virtual ~EoDbPegFile() {}
  void Load(AeSysDoc* document);
  void ReadBlocksSection(AeSysDoc* document);
  void ReadEntitiesSection(AeSysDoc* document);
  void ReadHeaderSection(AeSysDoc* document);
  void ReadLayerTable(AeSysDoc* document);
  void ReadLinetypesTable(AeSysDoc* document);
  void ReadLinetypeDefinition(
      std::vector<double>& dashLength, CString& name, CString& description, std::uint16_t& definitionLength);
  void ReadPaperSpaceExtension(AeSysDoc* document);
  void ReadTablesSection(AeSysDoc* document);
  void ReadViewportTable(AeSysDoc* document);
  void Unload(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteBlocksSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteEntitiesSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteHeaderSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteLayerTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteLinetypeTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WritePaperSpaceExtension(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteTablesSection(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
  void WriteVPortTable(AeSysDoc* document, EoDb::PegFileVersion fileVersion);
};
