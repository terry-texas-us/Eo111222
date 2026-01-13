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
