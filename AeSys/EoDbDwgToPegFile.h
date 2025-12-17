#pragma once

#include <io.h>

#if defined(USING_ODA)

class EoDbDwgToPegFile {
 public:
  OdDbDatabasePtr m_DatabasePtr;

 public:
  /// <summary>Create and initialize a default database</summary>
  EoDbDwgToPegFile(OdDbDatabasePtr database);

  ~EoDbDwgToPegFile() {};
  void ConvertToPeg(AeSysDoc* document);

  void ConvertHeaderSectionToPeg(AeSysDoc* /* document */) {};
  void ConvertViewportTableToPeg(AeSysDoc* document);
  void ConvertLinetypesTableToPeg(AeSysDoc* document);
  void ConvertLayerTableToPeg(AeSysDoc* document);

  /// <summary>Load all block containers, local or external. An external reference contains the name and the filename of the external drawing. Local blocks containers are an unordered list of drawing entities. The two type of local block containers are layout and non-layout.</summary>
  void ConvertBlockTableToPeg(AeSysDoc* document);
  void ConvertBlocksToPeg(AeSysDoc* document);
  void ConvertEntitiesToPeg(AeSysDoc* document);
  void ConvertBlockToPeg(OdDbBlockTableRecordPtr block, AeSysDoc* document);
};

/// <summary>
/// the default file examine -- if the file isn't pathed to the right place, then search the ACAD environment variable for the oldFile
/// </summary>
void ExamineFile(LPWSTR oldFile, const int oldFileBufferSize, LPWSTR newFile, const int newFileBufferSize);

#endif  // USING_ODA
