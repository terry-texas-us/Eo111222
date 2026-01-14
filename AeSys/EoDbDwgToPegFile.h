#pragma once
#if defined(USING_ODA)

#include <io.h>

class EoDbDwgToPegFile {
 public:
  OdDbDatabasePtr m_DatabasePtr;

 public:
  EoDbDwgToPegFile(OdDbDatabasePtr database);

  ~EoDbDwgToPegFile() {};
  void ConvertToPeg(AeSysDoc* document);
  void ConvertBlockTableToPeg(AeSysDoc* document);
  void ConvertBlocksToPeg(AeSysDoc* document);
  void ConvertEntitiesToPeg(AeSysDoc* document);
  void ConvertBlockToPeg(OdDbBlockTableRecordPtr block, AeSysDoc* document);
};
void ExamineFile(LPWSTR oldFile, const int oldFileBufferSize, LPWSTR newFile, const int newFileBufferSize);

#endif
