#include "Stdafx.h"

#include "AeSys.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbLayer.h"
#include "EoDbPrimitive.h"
#include "EoDbTracingFile.h"
#include "Resource.h"

void EoDbTracingFile::ReadHeader(CFile& file) {
  if (EoDb::ReadUInt16(file) != EoDb::kHeaderSection)
    throw L"Exception EoDbTracingFile: Expecting sentinel EoDb::kHeaderSection.";

  // 	with addition of info here will loop key-value pairs till EoDb::kEndOfSection sentinel

  if (EoDb::ReadUInt16(file) != EoDb::kEndOfSection)
    throw L"Exception EoDbTracingFile: Expecting sentinel EoDb::kEndOfSection.";
}
bool EoDbTracingFile::ReadLayer(CFile& file, EoDbLayer* layer) {
  if (EoDb::ReadUInt16(file) != EoDb::kGroupsSection)
    throw L"Exception EoDbTracingFile: Expecting sentinel EoDb::kGroupsSection.";

  EoUInt16 NumberOfGroups = EoDb::ReadUInt16(file);

  for (EoUInt16 n = 0; n < NumberOfGroups; n++) {
    EoDbGroup* Group = ReadGroup(file);
    layer->AddTail(Group);
  }
  if (EoDb::ReadUInt16(file) != EoDb::kEndOfSection)
    throw L"Exception EoDbTracingFile: Expecting sentinel EoDb::kEndOfSection.";

  return true;
}
EoDbGroup* EoDbTracingFile::ReadGroup(CFile& file) {
  EoUInt16 NumberOfPrimitives = EoDb::ReadUInt16(file);

  EoDbGroup* Group = new EoDbGroup;
  EoDbPrimitive* Primitive;

  for (EoUInt16 n = 0; n < NumberOfPrimitives; n++) {
    EoDb::Read(file, Primitive);
    Group->AddTail(Primitive);
  }
  return Group;
}
void EoDbTracingFile::WriteHeader(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kHeaderSection));

  EoDb::Write(file, EoUInt16(EoDb::kEndOfSection));
}
void EoDbTracingFile::WriteLayer(CFile& file, EoDbLayer* layer) {
  EoDb::Write(file, EoUInt16(EoDb::kGroupsSection));

  EoDb::Write(file, EoUInt16(layer->GetCount()));

  auto position = layer->GetHeadPosition();
  while (position != 0) {
    EoDbGroup* Group = layer->GetNext(position);
    Group->Write(file);
  }
  EoDb::Write(file, EoUInt16(EoDb::kEndOfSection));
  app.AddStringToMessageList(IDS_MSG_TRACING_SAVE_SUCCESS, layer->Name());
}
