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

  auto numberOfGroups = EoDb::ReadUInt16(file);

  for (EoUInt16 n = 0; n < numberOfGroups; n++) {
    auto* group = ReadGroup(file);
    layer->AddTail(group);
  }
  if (EoDb::ReadUInt16(file) != EoDb::kEndOfSection) {
    throw L"Exception EoDbTracingFile: Expecting sentinel EoDb::kEndOfSection.";
  }
  return true;
}

EoDbGroup* EoDbTracingFile::ReadGroup(CFile& file) {
  auto numberOfPrimitives = EoDb::ReadUInt16(file);

  auto* group = new EoDbGroup;
  EoDbPrimitive* primitive{};

  for (EoUInt16 n = 0; n < numberOfPrimitives; n++) {
    EoDb::Read(file, primitive);
    group->AddTail(primitive);
  }
  return group;
}

void EoDbTracingFile::WriteHeader(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kHeaderSection));

  EoDb::Write(file, EoUInt16(EoDb::kEndOfSection));
}
void EoDbTracingFile::WriteLayer(CFile& file, EoDbLayer* layer) {
  EoDb::Write(file, EoUInt16(EoDb::kGroupsSection));

  EoDb::Write(file, EoUInt16(layer->GetCount()));

  auto position = layer->GetHeadPosition();
  while (position != nullptr) {
    auto* group = layer->GetNext(position);
    group->Write(file);
  }
  EoDb::Write(file, EoUInt16(EoDb::kEndOfSection));
  app.AddStringToMessageList(IDS_MSG_TRACING_SAVE_SUCCESS, layer->Name());
}
