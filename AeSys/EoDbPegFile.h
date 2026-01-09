#pragma once
#include <afx.h>
#include <afxstr.h>
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
  void ReadLinetypeDefinition(std::vector<double>& dashLength, CString& name, CString& description, EoUInt16& definitionLength);
  void ReadTablesSection(AeSysDoc* document);
  void ReadViewportTable(AeSysDoc* document);
  void Unload(AeSysDoc* document);
  void WriteBlocksSection(AeSysDoc* document);
  void WriteEntitiesSection(AeSysDoc* document);
  void WriteHeaderSection(AeSysDoc* document);
  void WriteLayerTable(AeSysDoc* document);
  void WriteLinetypeTable(AeSysDoc* document);
  void WriteTablesSection(AeSysDoc* document);
  void WriteVPortTable(AeSysDoc* document);
};
