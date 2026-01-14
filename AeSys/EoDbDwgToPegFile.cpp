#include "stdafx.h"

#if defined(USING_ODA)
#include "DbDictionary.h"
#include "DbBlockTable.h"
#include "DbBlockTableRecord.h"
#include "DbLayerTable.h"
#include "DbLayerTableRecord.h"
#include "DbLine.h"
#include "DbLinetypeTable.h"
#include "DbLinetypeTableRecord.h"
#include "DbViewportTable.h"
#include "DbViewportTableRecord.h"

#include "EoDbDwgToPegFile.h"
#include "EntityToPrimitiveProtocolExtension.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTraceCategory traceOdDb(L"traceOdDb", 3);

EoDbDwgToPegFile::EoDbDwgToPegFile(OdDbDatabasePtr database) { m_DatabasePtr = database; }
void EoDbDwgToPegFile::ConvertToPeg(AeSysDoc* document) {
  if (m_DatabasePtr.isNull()) { return; }

  ConvertBlockTableToPeg(document);

  ConvertBlocksToPeg(document);
  ConvertEntitiesToPeg(document);
}
void EoDbDwgToPegFile::ConvertBlockTableToPeg(AeSysDoc* document) {
  OdDbBlockTablePtr Blocks = m_DatabasePtr->getBlockTableId().safeOpenObject();
  ATLTRACE2(traceOdDb, 0, L"%s   Loading block definitions ...\n", (LPCWSTR)Blocks->desc()->name());

  OdDbSymbolTableIteratorPtr Iterator = Blocks->newIterator();

  for (Iterator->start(); !Iterator->done(); Iterator->step()) {
    OdDbBlockTableRecordPtr Block = Iterator->getRecordId().safeOpenObject();
    ATLTRACE2(traceOdDb, 1, L"%s  %s\n", LPCWSTR(Block->desc()->name()), LPCWSTR(Block->getName()));

    if (Block->isAnonymous()) {
      //WCHAR szBuf[8];
      //errno_t Error = _itow_s(lBlockHeader, szBuf, 8, 10);
      //_tcscat(m_htb->blkh.name, szBuf);
      //adReplaceBlockheader(hdb, &m_htb->blkh);
    }
    if (Block->isFromExternalReference()) {
      //WCHAR NewFile[256];
      //WCHAR OldFile[256];
      //wcscpy(OldFile, (PCTSTR) Block->pathName());
      //ExamineFile(OldFile, 256, NewFile, 256);
      //Block->setPathName((PCTSTR) NewFile);
      ATLTRACE2(traceOdDb, 0, L"External reference to drawing: %s ignored.\n", (PCTSTR)Block->pathName());
    }
    EoDbBlock* pBlock;
    if (document->LookupBlock((PCTSTR)Block->getName(), pBlock)) {
      //WCHAR szBuf[8];
      //errno_t Error = _itow_s(lBlockHeader, szBuf, 8, 10);
      //_tcscat(m_htb->blkh.name, szBuf);
      //adReplaceBlockheader(hdb, &m_htb->blkh);
    }
    OdGePoint3d Origin = Block->origin();
    EoGePoint3d ptBase(Origin.x, Origin.y, Origin.z);
    pBlock = new EoDbBlock(0 /* m_htb->blkh.flag */, ptBase, (PCTSTR)Block->pathName());
    document->InsertBlock((PCTSTR)Block->getName(), pBlock);
  }
}

void EoDbDwgToPegFile::ConvertBlocksToPeg(AeSysDoc* document) {
  OdDbBlockTablePtr Blocks = m_DatabasePtr->getBlockTableId().safeOpenObject();
  ATLTRACE2(traceOdDb, 0, L"%s   Loading block definitions ...\n", LPCWSTR(Blocks->desc()->name()));

  OdDbSymbolTableIteratorPtr Iterator = Blocks->newIterator();

  for (Iterator->start(); !Iterator->done(); Iterator->step()) {
    OdDbBlockTableRecordPtr Block = Iterator->getRecordId().safeOpenObject();
    if (!Block->isLayout()) {
      ATLTRACE2(traceOdDb, 0, L"%s  %s\n", LPCWSTR(Block->desc()->name()), LPCWSTR(Block->getName()));
      ConvertBlockToPeg(Block, document);
    }
  }
}
void EoDbDwgToPegFile::ConvertBlockToPeg(OdDbBlockTableRecordPtr Block, AeSysDoc* document) {
  EoDbBlock* pBlock;
  document->LookupBlock((PCTSTR)Block->getName(), pBlock);

  ProtocolExtension_ConvertEntityToPegPrimitive ProtocolExtensions(document);
  ProtocolExtensions.Initialize();
  CString BlockName((PCTSTR)Block->getName());
  ATLTRACE2(traceOdDb, 0, L"Loading Block %s entity definitions ...\n", (PCTSTR)Block->getName());

  if (Block->isFromExternalReference()) {
    // external reference blocks have no entities. It points to a block in an external
    // drawing. either the external drawing's *Model_space or any non-layout block.
  }
  OdDbObjectIteratorPtr EntityIterator = Block->newIterator();

  for (; !EntityIterator->done(); EntityIterator->step()) {
    OdDbObjectId EntityObjectId = EntityIterator->objectId();
    OdDbEntityPtr Entity = EntityObjectId.safeOpenObject();

    OdSmartPtr<EoDbConvertEntityToPrimitive> EntityConverter = Entity;
    EntityConverter->Convert(Entity, pBlock);
    if (Entity->extensionDictionary()) { ATLTRACE2(traceOdDb, 0, L"Entity extension dictionary not loaded\n"); }
  }
  OdDbObjectId ObjectId = Block->extensionDictionary();
  if (!ObjectId.isNull()) {
    OdDbObjectPtr ObjectPtr = ObjectId.safeOpenObject();
    OdDbDictionaryPtr Dictionary = ObjectPtr;

    OdDbDictionaryIteratorPtr Iterator = Dictionary->newIterator();
    for (; !Iterator->done(); Iterator->next()) {
      ATLTRACE2(traceOdDb, 0, L"Dictionary name: %s\n", (PCTSTR)Iterator->name());
    }
  }
}
void EoDbDwgToPegFile::ConvertEntitiesToPeg(AeSysDoc* document) {
  ProtocolExtension_ConvertEntityToPegPrimitive ProtocolExtensions(document);
  ProtocolExtensions.Initialize();

  OdDbBlockTablePtr Blocks = m_DatabasePtr->getBlockTableId().safeOpenObject();
  OdDbBlockTableRecordPtr Modelspace = Blocks->getModelSpaceId().safeOpenObject();

  ATLTRACE2(traceOdDb, 0, L"%s   Loading Layout Object definitions ...\n", (LPCWSTR)Modelspace->desc()->name());
  ATLTRACE2(traceOdDb, 0, L"Loading %s entity definitions ...\n", (PCTSTR)Modelspace->getName());

  int EntitiesNotLoaded = 0;

  OdDbObjectIteratorPtr EntityIterator = Modelspace->newIterator();

  for (; !EntityIterator->done(); EntityIterator->step()) {
    OdDbObjectId EntityObjectId = EntityIterator->objectId();
    OdDbEntityPtr Entity = EntityObjectId.safeOpenObject();

    EoDbLayer* Layer = document->GetLayerTableLayer((PCTSTR)Entity->layer());

    EoDbGroup* Group = new EoDbGroup();
    OdSmartPtr<EoDbConvertEntityToPrimitive> EntityConverter = Entity;
    EntityConverter->Convert(Entity, Group);

    if (Group->IsEmpty()) {
      delete Group;
      EntitiesNotLoaded++;
    } else {
      Layer->AddTail(Group);
    }
  }
  ATLTRACE2(traceOdDb, 0, L"      %d Modelspace entitities not loaded\n", EntitiesNotLoaded);

  OdDbObjectId ObjectId = Modelspace->extensionDictionary();

  if (!ObjectId.isNull()) {
    OdDbObjectPtr ObjectPtr = ObjectId.safeOpenObject();
    OdDbDictionaryPtr Dictionary = ObjectPtr;

    OdDbDictionaryIteratorPtr Iterator = Dictionary->newIterator();
    for (; !Iterator->done(); Iterator->next()) {
      ATLTRACE2(traceOdDb, 0, L"Dictionary name: %s\n", (PCTSTR)Iterator->name());
    }
  }

  EntitiesNotLoaded = 0;
  OdDbBlockTableRecordPtr Paperspace = Blocks->getPaperSpaceId().safeOpenObject();
  ATLTRACE2(traceOdDb, 0, L"Loading %s entity definitions ...\n", (PCTSTR)Paperspace->getName());

  EntityIterator = Paperspace->newIterator();
  for (; !EntityIterator->done(); EntityIterator->step()) {
    OdDbObjectId EntityObjectId = EntityIterator->objectId();
    OdDbEntityPtr Entity = EntityObjectId.safeOpenObject();

    /* EoDbLayer* Layer = */ document->GetLayerTableLayer((PCTSTR)Entity->layer());

    EoDbGroup* Group = new EoDbGroup();
    OdSmartPtr<EoDbConvertEntityToPrimitive> EntityConverter = Entity;
    EntityConverter->Convert(Entity, Group);

    if (Group->IsEmpty()) {
      delete Group;
      EntitiesNotLoaded++;
    } else {
      //Layer->AddTail(Group);
    }
  }
  ATLTRACE2(traceOdDb, 0, L"      %d Paperspace entitities not loaded\n", EntitiesNotLoaded);
}

void ConvertPrimitiveData(const EoDbPrimitive* primitive, OdDbBlockTableRecordPtr block, OdDbEntity* entity) {
  OdDbDatabase* Database = entity->database();
  entity->setDatabaseDefaults(Database);

  entity->setColorIndex(primitive->PenColor());

  OdDbLinetypeTablePtr Linetypes = Database->getLinetypeTableId().safeOpenObject(OdDb::kForWrite);
  OdDbObjectId Linetype = 0;
  if (primitive->LineType() == EoDbPrimitive::LINETYPE_BYLAYER) {
    Linetype = Linetypes->getLinetypeByLayerId();
  } else if (primitive->LineType() == EoDbPrimitive::LINETYPE_BYBLOCK) {
    Linetype = Linetypes->getLinetypeByBlockId();
  } else {
    EoDbLineTypeTable* LineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();

    EoDbLineType* LineType;
    LineTypeTable->LookupUsingLegacyIndex(primitive->LineType(), LineType);

    OdString Name = LineType->Name();
    Linetype = Linetypes->getAt(Name);
  }
  entity->setLinetype(Linetype);
}
OdDbEntity* EoDbLine::Convert(const OdDbObjectId& blockTableRecord) {
  OdDbBlockTableRecordPtr Block = blockTableRecord.safeOpenObject(OdDb::kForWrite);

  OdDbLinePtr LineEntity = OdDbLine::createObject();
  Block->appendOdDbEntity(LineEntity);

  ConvertPrimitiveData(this, Block, LineEntity);

  LineEntity->setStartPoint(OdGePoint3d(m_ln[0].x, m_ln[0].y, m_ln[0].z));
  LineEntity->setEndPoint(OdGePoint3d(m_ln[1].x, m_ln[1].y, m_ln[1].z));
  return LineEntity;
}
OdDbEntity* EoDbEllipse::Convert(const OdDbObjectId&) { return 0; }
OdDbEntity* EoDbPolyline::Convert(const OdDbObjectId&) { return 0; }
OdDbEntity* EoDbPolygon::Convert(const OdDbObjectId&) { return 0; }
OdDbEntity* EoDbText::Convert(const OdDbObjectId&) { return 0; }
OdDbEntity* EoDbSpline::Convert(const OdDbObjectId&) { return 0; }
OdDbEntity* EoDbPoint::Convert(const OdDbObjectId&) { return 0; }
OdDbEntity* EoDbDimension::Convert(const OdDbObjectId&) { return 0; }
OdDbEntity* EoDbBlockReference::Convert(const OdDbObjectId&) { return 0; }

void ExamineFile(LPWSTR oldFile, const int oldFileBufferSize, LPWSTR newFile, const int newFileBufferSize) {
  const int kExistenceOnly = 0;

  static WCHAR pathchar = '\\';
  static WCHAR oldpathchar = '/';

  // replace path characters, if present, with proper ones for platform
  for (int i = 0; i < int(wcslen(oldFile)); i++) {
    if (oldFile[i] == oldpathchar) oldFile[i] = pathchar;
  }
  if (wcschr(oldFile, pathchar) != 0) {  // file has some degree of path designation
    if (_waccess_s(oldFile, kExistenceOnly) == 0) {
      wcscpy_s(newFile, newFileBufferSize, oldFile);
      return;
    }
    // strip the path
    LPWSTR pLast = _tcsrchr(oldFile, pathchar);
    wcscpy_s(oldFile, oldFileBufferSize, ++pLast);
  }
  wcscpy_s(newFile, newFileBufferSize, oldFile);

  if (_waccess_s(oldFile, kExistenceOnly) != 0) {  // file is not in current directory
    if (wcschr(oldFile, ':') != 0) {               // path with drive id
      return;
    }
    size_t RequiredSize;
    _wgetenv_s(&RequiredSize, nullptr, 0, L"ACAD");

    if (RequiredSize == 0) {  // no ACAD environment to search
      return;
    }
    LPWSTR envptr = new WCHAR[RequiredSize];

    _wgetenv_s(&RequiredSize, envptr, RequiredSize, L"ACAD");

    LPWSTR cptr = envptr;
    WCHAR testpath[256];
    WCHAR holdch;
    do {
      while (*cptr != ';' && *cptr != 0) { cptr++; }
      holdch = *cptr;  // grab terminating character
      *cptr = 0;       // null it out for wcscpy_s
      wcscpy_s(testpath, 256, envptr);
      *cptr = holdch;
      int iTest = (int)wcslen(testpath);
      if (testpath[iTest - 1] != pathchar) {  // append path character
        testpath[iTest] = pathchar;
        testpath[iTest + 1] = 0;
      }
      wcscat_s(testpath, 256, oldFile);

      if (_waccess_s(testpath, kExistenceOnly) == 0) {
        wcscpy_s(newFile, newFileBufferSize, testpath);
        return;
      }
      cptr++;
      envptr = cptr;
    } while (holdch == ';');  // terminator is 0 for end of env string

    delete[] envptr;
  }
}
#endif
