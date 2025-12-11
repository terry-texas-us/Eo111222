#include "stdafx.h"
#include "AeSys.h"
#include "FileTracing.h"

void CFileTracing::ReadHeader(CFile& file)
{
	if (EoDb::ReadUInt16(file) != EoDb::kHeaderSection)
		throw L"Exception CFileTracing: Expecting sentinel EoDb::kHeaderSection.";
	
	// 	with addition of info here will loop key-value pairs till EoDb::kEndOfSection sentinel

	if (EoDb::ReadUInt16(file) != EoDb::kEndOfSection)
		throw L"Exception CFileTracing: Expecting sentinel EoDb::kEndOfSection.";
}
bool CFileTracing::ReadLayer(CFile& file, EoDbLayer* layer)
{
	if (EoDb::ReadUInt16(file) != EoDb::kGroupsSection)
		throw L"Exception CFileTracing: Expecting sentinel EoDb::kGroupsSection.";
	
	EoUInt16 NumberOfGroups = EoDb::ReadUInt16(file);

	for (EoUInt16 n = 0; n < NumberOfGroups; n++)
	{
		EoDbGroup* Group = ReadGroup(file);
		layer->AddTail(Group); 
	}
	if (EoDb::ReadUInt16(file) != EoDb::kEndOfSection)
		throw L"Exception CFileTracing: Expecting sentinel EoDb::kEndOfSection.";

	return true;
}
EoDbGroup* CFileTracing::ReadGroup(CFile &file)
{
	EoUInt16 NumberOfPrimitives = EoDb::ReadUInt16(file);

	EoDbGroup* Group = new EoDbGroup;
	EoDbPrimitive* Primitive;

	for (EoUInt16 n = 0; n < NumberOfPrimitives; n++)
	{
		EoDb::Read(file, Primitive);
		Group->AddTail(Primitive);
	}
	return Group;
}
void CFileTracing::WriteHeader(CFile& file)
{
	EoDb::Write(file, EoUInt16(EoDb::kHeaderSection));

	EoDb::Write(file, EoUInt16(EoDb::kEndOfSection));
}
void CFileTracing::WriteLayer(CFile& file, EoDbLayer* layer)
{
	EoDb::Write(file, EoUInt16(EoDb::kGroupsSection));

	EoDb::Write(file, EoUInt16(layer->GetCount()));
		
	POSITION position = layer->GetHeadPosition();
	while (position != 0)
	{
		EoDbGroup* Group = layer->GetNext(position);
		Group->Write(file);
	}
	EoDb::Write(file, EoUInt16(EoDb::kEndOfSection));
	app.AddStringToMessageList(IDS_MSG_TRACING_SAVE_SUCCESS, layer->Name());
}