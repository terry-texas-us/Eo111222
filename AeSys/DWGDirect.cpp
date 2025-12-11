#include "stdafx.h"
#include "AeSysDoc.h"

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

#include "Directory.h"
#include "DWGDirect.h"
#include "EntityToPrimitiveProtocolExtension.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTraceCategory traceOdDb(L"traceOdDb", 3);

CFileDWGDirect::CFileDWGDirect(OdDbDatabasePtr database)
{
	m_DatabasePtr = database;
}
void CFileDWGDirect::ConvertToPeg(AeSysDoc* document)
{
	if (m_DatabasePtr.isNull())
	{
		return;
	}
	ConvertHeaderSectionToPeg(document);
	ConvertViewportTableToPeg(document);
	ConvertLinetypesTableToPeg(document);
	ConvertLayerTableToPeg(document);
	
	ConvertBlockTableToPeg(document);
	
	ConvertBlocksToPeg(document);
	ConvertEntitiesToPeg(document);
}
void CFileDWGDirect::ConvertBlockTableToPeg(AeSysDoc* document)
{
	OdDbBlockTablePtr Blocks = m_DatabasePtr->getBlockTableId().safeOpenObject();
	ATLTRACE2(traceOdDb, 0, L"%s   Loading block definitions ...\n", (LPCWSTR) Blocks->desc()->name());

	OdDbSymbolTableIteratorPtr Iterator = Blocks->newIterator();
	
	for (Iterator->start(); !Iterator->done(); Iterator->step())
	{
		OdDbBlockTableRecordPtr Block = Iterator->getRecordId().safeOpenObject();
		ATLTRACE2(traceOdDb, 1, L"%s  %s\n", LPCWSTR(Block->desc()->name()), LPCWSTR(Block->getName()));

		if (Block->isAnonymous())
		{
			//WCHAR szBuf[8];
			//errno_t Error = _itow_s(lBlockHeader, szBuf, 8, 10);
			//_tcscat(m_htb->blkh.name, szBuf);
			//adReplaceBlockheader(hdb, &m_htb->blkh);
		}
		if (Block->isFromExternalReference())
		{
			//WCHAR NewFile[256];
			//WCHAR OldFile[256];
			//wcscpy(OldFile, (PCTSTR) Block->pathName());
			//Directory_ExamineFile(OldFile, 256, NewFile, 256);
			//Block->setPathName((PCTSTR) NewFile);
			ATLTRACE2(traceOdDb, 0, L"External reference to drawing: %s ignored.\n", (PCTSTR) Block->pathName());
		}
		EoDbBlock* pBlock;
		if (document->LookupBlock((PCTSTR) Block->getName(), pBlock))
		{
			//WCHAR szBuf[8];
			//errno_t Error = _itow_s(lBlockHeader, szBuf, 8, 10);
			//_tcscat(m_htb->blkh.name, szBuf);
			//adReplaceBlockheader(hdb, &m_htb->blkh);
		}
		OdGePoint3d Origin = Block->origin();
		EoGePoint3d ptBase(Origin.x, Origin.y, Origin.z);
		pBlock = new EoDbBlock(0 /* m_htb->blkh.flag */, ptBase, (PCTSTR) Block->pathName());
		document->InsertBlock((PCTSTR) Block->getName(), pBlock);
	}
}
void CFileDWGDirect::ConvertLayerTableToPeg(AeSysDoc* document)
{
	OdDbLayerTablePtr Layers = m_DatabasePtr->getLayerTableId().safeOpenObject();
	ATLTRACE2(traceOdDb, 0, L"%s   Loading layer definitions ...\n", (LPCWSTR) Layers->desc()->name());

	OdDbSymbolTableIteratorPtr Iterator = Layers->newIterator();
	
	for (Iterator->start(); !Iterator->done(); Iterator->step())
	{
		OdDbLayerTableRecordPtr Layer = Iterator->getRecordId().safeOpenObject();
		ATLTRACE2(traceOdDb, 1, L"%s  %s\n", (LPCWSTR) Layer->desc()->name(), (LPCWSTR) Layer->getName());
		if (document->FindLayerTableLayer((LPCWSTR) Layer->getName()) < 0)
		{
			EoDbLayer* pegLayer = new EoDbLayer((LPCWSTR) Layer->getName(), EoDbLayer::kIsResident | EoDbLayer::kIsInternal | EoDbLayer::kIsActive);
			
			pegLayer->SetColorIndex(EoInt16(abs(Layer->colorIndex())));
			
			OdDbObjectId LinetypeObjectId = Layer->linetypeObjectId();
			
	        OdDbLinetypeTableRecordPtr Linetype = LinetypeObjectId.safeOpenObject();
						
			EoDbLineType* LineType;
			document->LineTypeTable()->Lookup((LPCWSTR) Linetype->getName(), LineType);
			pegLayer->SetLineType(LineType);
						
			if (Layer->isFrozen() || Layer->isOff())
			{
				pegLayer->SetStateOff();
			}
			document->AddLayerTableLayer(pegLayer);
			
			ATLTRACE2(traceOdDb, 2, L"Line weight: %i\n", 	Layer->lineWeight());
			ATLTRACE2(traceOdDb, 2, L"Plot style name: %s\n", (PCTSTR) Layer->plotStyleName());
			ATLTRACE2(traceOdDb, 2, L"Plot style name object: %08.8lx\n", Layer->plotStyleNameId());
			ATLTRACE2(traceOdDb, 2, L"Layer is locked: %i\n", Layer->isLocked());
			ATLTRACE2(traceOdDb, 2, L"Layer is plottable: %i\n", Layer->isPlottable());
			ATLTRACE2(traceOdDb, 2, L"Viewport default: %i\n", Layer->VPDFLT());
			OdDbObjectId ObjectId = Layer->extensionDictionary(); 
			if (!ObjectId.isNull())
			{
				OdDbObjectPtr ObjectPtr = ObjectId.safeOpenObject();
				OdDbDictionaryPtr Dictionary = ObjectPtr;
			    
				OdDbDictionaryIteratorPtr Iterator = Dictionary->newIterator();
			    for (; !Iterator->done(); Iterator->next())
				{
					ATLTRACE2(traceOdDb, 2, L"Dictionary name: %s\n", (PCTSTR) Iterator->name());
				}
			}
		}
	}
}
void CFileDWGDirect::ConvertLinetypesTableToPeg(AeSysDoc* document)
{
	OdDbLinetypeTablePtr Linetypes = m_DatabasePtr->getLinetypeTableId().safeOpenObject();
	ATLTRACE2(traceOdDb, 0, L"%s   Loading line type definitions ...\n", (LPCWSTR) Linetypes->desc()->name());
	
	EoDbLineTypeTable* LineTypeTable = document->LineTypeTable();

	OdDbSymbolTableIteratorPtr Iterator = Linetypes->newIterator();
	
	for (Iterator->start(); !Iterator->done(); Iterator->step())
	{
		OdDbLinetypeTableRecordPtr Linetype = Iterator->getRecordId().safeOpenObject();
		ATLTRACE2(traceOdDb, 0, L"%s  %s\n", (LPCWSTR) Linetype->desc()->name(), (LPCWSTR) Linetype->getName());
		OdString LinetypeName = Linetype->getName();
		
		CString Name = LinetypeName;
		EoDbLineType* LineType;

		if (!LineTypeTable->Lookup(Name, LineType))
		{
			EoUInt16 NumberOfDashes = EoUInt16(Linetype->numDashes());
			//Linetype->patternLength();
			//Linetype->isScaledToFit();
			if (Linetype->extensionDictionary()) {}
		
			double* dDashLen = new double[NumberOfDashes];
			
			for (EoUInt16 n = 0; n < NumberOfDashes; n++)
			{
				dDashLen[n] = Linetype->dashLengthAt(n);
				//Linetype->shapeStyleAt(n);
				//Linetype->shapeNumberAt(n);
				//Linetype->textAt(n);
				//Linetype->shapeScaleAt(n);
				//Linetype->shapeOffsetAt(n);
				//Linetype->shapeRotationAt(n);
				//Linetype->shapeIsUcsOrientedAt(n);
			}
			LineType = new EoDbLineType(EoUInt16(_wtoi(Name)), Name, (LPCWSTR) Linetype->comments(), NumberOfDashes, dDashLen);
			LineTypeTable->SetAt(Name, LineType);
			
			delete [] dDashLen;
		}
	}
}
void CFileDWGDirect::ConvertViewportTableToPeg(AeSysDoc* /* document */)
{
	OdDbViewportTablePtr Viewports = m_DatabasePtr->getViewportTableId().safeOpenObject();
	ATLTRACE2(traceOdDb, 0, L"%s   Loading viewport definitions ...\n", (LPCWSTR) Viewports->desc()->name());
	
    OdDbSymbolTableIteratorPtr Iterator = Viewports->newIterator();
	
	for (Iterator->start(); !Iterator->done(); Iterator->step())
	{
		OdDbViewportTableRecordPtr Viewport = Iterator->getRecordId().safeOpenObject();
		ATLTRACE2(traceOdDb, 0, L"%s  %s\n", (LPCWSTR) Viewport->desc()->name(), (LPCWSTR) Viewport->getName());

		if (Viewport->extensionDictionary()) {}
	}
}


void CFileDWGDirect::ConvertBlocksToPeg(AeSysDoc* document)
{
	OdDbBlockTablePtr Blocks = m_DatabasePtr->getBlockTableId().safeOpenObject();
	ATLTRACE2(traceOdDb, 0, L"%s   Loading block definitions ...\n", LPCWSTR(Blocks->desc()->name()));

	OdDbSymbolTableIteratorPtr Iterator = Blocks->newIterator();
	
	for (Iterator->start(); !Iterator->done(); Iterator->step())
	{
		OdDbBlockTableRecordPtr Block = Iterator->getRecordId().safeOpenObject();
		if (!Block->isLayout())
		{
			ATLTRACE2(traceOdDb, 0, L"%s  %s\n", LPCWSTR(Block->desc()->name()), LPCWSTR(Block->getName()));
			ConvertBlockToPeg(Block, document);
		}
	}
}
void CFileDWGDirect::ConvertBlockToPeg(OdDbBlockTableRecordPtr Block, AeSysDoc* document)
{
	EoDbBlock* pBlock;
	document->LookupBlock((PCTSTR) Block->getName(), pBlock);
	
	ProtocolExtension_ConvertEntityToPegPrimitive ProtocolExtensions;
    ProtocolExtensions.Initialize();
	CString BlockName((PCTSTR) Block->getName()); 
	ATLTRACE2(traceOdDb, 0, L"Loading Block %s entity definitions ...\n", (PCTSTR) Block->getName());
			
	if (Block->isFromExternalReference())
	{
		// external reference blocks have no entities. It points to a block in an external
		// drawing. either the external drawing's *Model_space or any non-layout block.
	}
	OdDbObjectIteratorPtr EntityIterator = Block->newIterator();
	   
    for (; !EntityIterator->done(); EntityIterator->step())
    {
		OdDbObjectId EntityObjectId = EntityIterator->objectId();
		OdDbEntityPtr Entity = EntityObjectId.safeOpenObject();
		
		OdSmartPtr<EoDbConvertEntityToPrimitive> EntityConverter = Entity;
		EntityConverter->Convert(Entity, pBlock);
		if (Entity->extensionDictionary()) 
		{
			ATLTRACE2(traceOdDb, 0, L"Entity extension dictionary not loaded\n");
		}
	}
	OdDbObjectId ObjectId = Block->extensionDictionary(); 
	if (!ObjectId.isNull())
	{
		OdDbObjectPtr ObjectPtr = ObjectId.safeOpenObject();
		OdDbDictionaryPtr Dictionary = ObjectPtr;
	    
		OdDbDictionaryIteratorPtr Iterator = Dictionary->newIterator();
	    for (; !Iterator->done(); Iterator->next())
		{
			ATLTRACE2(traceOdDb, 0, L"Dictionary name: %s\n", (PCTSTR) Iterator->name());
		}
	}
}
void CFileDWGDirect::ConvertEntitiesToPeg(AeSysDoc* document)
{
	ProtocolExtension_ConvertEntityToPegPrimitive ProtocolExtensions;
    ProtocolExtensions.Initialize();

	OdDbBlockTablePtr Blocks = m_DatabasePtr->getBlockTableId().safeOpenObject();
	OdDbBlockTableRecordPtr Modelspace = Blocks->getModelSpaceId().safeOpenObject();

	ATLTRACE2(traceOdDb, 0, L"%s   Loading Layout Object definitions ...\n", (LPCWSTR) Modelspace->desc()->name());
	ATLTRACE2(traceOdDb, 0, L"Loading %s entity definitions ...\n", (PCTSTR) Modelspace->getName());

	int EntitiesNotLoaded = 0;
		
	OdDbObjectIteratorPtr EntityIterator = Modelspace->newIterator();
	   
    for (; !EntityIterator->done(); EntityIterator->step())
    {
		OdDbObjectId EntityObjectId = EntityIterator->objectId();
		OdDbEntityPtr Entity = EntityObjectId.safeOpenObject();
		
		EoDbLayer* Layer = document->GetLayerTableLayer((PCTSTR) Entity->layer());

		EoDbGroup* Group = new EoDbGroup();
		OdSmartPtr<EoDbConvertEntityToPrimitive> EntityConverter = Entity;
		EntityConverter->Convert(Entity, Group);

		if (Group->IsEmpty())
		{
			delete Group;
			EntitiesNotLoaded++;
		}
		else
		{
			Layer->AddTail(Group);
		}
	}
	ATLTRACE2(traceOdDb, 0, L"      %d Modelspace entitities not loaded\n", EntitiesNotLoaded);
	
	OdDbObjectId ObjectId = Modelspace->extensionDictionary(); 
	
	if (!ObjectId.isNull())
	{
		OdDbObjectPtr ObjectPtr = ObjectId.safeOpenObject();
		OdDbDictionaryPtr Dictionary = ObjectPtr;
	    
		OdDbDictionaryIteratorPtr Iterator = Dictionary->newIterator();
	    for (; !Iterator->done(); Iterator->next())
		{
			ATLTRACE2(traceOdDb, 0, L"Dictionary name: %s\n", (PCTSTR) Iterator->name());
		}
	}

	EntitiesNotLoaded = 0;
	OdDbBlockTableRecordPtr Paperspace = Blocks->getPaperSpaceId().safeOpenObject();
	ATLTRACE2(traceOdDb, 0, L"Loading %s entity definitions ...\n", (PCTSTR) Paperspace->getName());
	
	EntityIterator = Paperspace->newIterator();
	for (; !EntityIterator->done(); EntityIterator->step())
    {
		OdDbObjectId EntityObjectId = EntityIterator->objectId();
		OdDbEntityPtr Entity = EntityObjectId.safeOpenObject();
		
		/* EoDbLayer* Layer = */ document->GetLayerTableLayer((PCTSTR) Entity->layer());

		EoDbGroup* Group = new EoDbGroup();
		OdSmartPtr<EoDbConvertEntityToPrimitive> EntityConverter = Entity;
		EntityConverter->Convert(Entity, Group);

		if (Group->IsEmpty())
		{
			delete Group;
			EntitiesNotLoaded++;
		}
		else
		{
			//Layer->AddTail(Group);
		}
	}
	ATLTRACE2(traceOdDb, 0, L"      %d Paperspace entitities not loaded\n", EntitiesNotLoaded);
}

void ConvertPrimitiveData(const EoDbPrimitive* primitive, OdDbBlockTableRecordPtr block, OdDbEntity* entity)
{
	OdDbDatabase* Database = entity->database();
	entity->setDatabaseDefaults(Database);
	
	entity->setColorIndex(primitive->PenColor());
	
	OdDbLinetypeTablePtr Linetypes = Database->getLinetypeTableId().safeOpenObject(OdDb::kForWrite);
	OdDbObjectId Linetype = 0;
	if (primitive->LineType() == EoDbPrimitive::LINETYPE_BYLAYER)
	{
		Linetype = Linetypes->getLinetypeByLayerId();
	}
	else if (primitive->LineType() == EoDbPrimitive::LINETYPE_BYBLOCK)
	{
		Linetype = Linetypes->getLinetypeByBlockId();
	}
	else
	{
		EoDbLineTypeTable* LineTypeTable = AeSysDoc::GetDoc()->LineTypeTable(); 

		EoDbLineType* LineType;
		LineTypeTable->__Lookup(primitive->LineType(), LineType);

		OdString Name = LineType->Name();
		Linetype = Linetypes->getAt(Name);
	}
	entity->setLinetype(Linetype);
}
OdDbEntity* EoDbLine::Convert(const OdDbObjectId& blockTableRecord)
{
	OdDbBlockTableRecordPtr Block = blockTableRecord.safeOpenObject(OdDb::kForWrite);

	OdDbLinePtr LineEntity = OdDbLine::createObject();
	Block->appendOdDbEntity(LineEntity);

	ConvertPrimitiveData(this, Block, LineEntity);
	
	LineEntity->setStartPoint(OdGePoint3d(m_ln[0].x, m_ln[0].y, m_ln[0].z));
	LineEntity->setEndPoint(OdGePoint3d(m_ln[1].x, m_ln[1].y, m_ln[1].z));
	return LineEntity;
}
OdDbEntity* EoDbEllipse::Convert(const OdDbObjectId&)
{
	return 0;
}
OdDbEntity* EoDbPolyline::Convert(const OdDbObjectId&)
{
	return 0;
}
OdDbEntity* EoDbPolygon::Convert(const OdDbObjectId&)
{
	return 0;
}
OdDbEntity* EoDbText::Convert(const OdDbObjectId&)
{
	return 0;
}
OdDbEntity* EoDbSpline::Convert(const OdDbObjectId&)
{
	return 0;
}
OdDbEntity* EoDbPoint::Convert(const OdDbObjectId&)
{
	return 0;
}
OdDbEntity* EoDbDimension::Convert(const OdDbObjectId&)
{
	return 0;
}
OdDbEntity* EoDbBlockReference::Convert(const OdDbObjectId&)
{
	return 0;
}
#endif // USING_ODA