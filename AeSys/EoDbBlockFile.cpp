#include "Stdafx.h"

#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbBlockFile.h"
#include "EoDbPrimitive.h"
#include "EoGePoint3d.h"

// EoDb::kHeaderSection sentinel				EoUInt16 0x0101
//		{0 or more key-value pairs}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff
// EoDb::kBlocksSection sentinel				EoUInt16 0x0103
//		Number of blocks						EoUInt16
//		{0 or more block definitions}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff

EoDbBlockFile::EoDbBlockFile(const CString& pathName) { CFile::Open(pathName, modeReadWrite | shareDenyNone); }

void EoDbBlockFile::ReadBlocks(EoDbBlocks& blocks) {
  if (EoDb::ReadUInt16(*this) != EoDb::kBlocksSection) {
    throw L"Exception EoDbBlockFile: Expecting sentinel EoDb::kBlocksSection.";
  }
  CString strName;

  EoDbPrimitive* primitive{};

  auto blockTableSize = EoDb::ReadUInt16(*this);
  for (int BlockTableIndex = 0; BlockTableIndex < blockTableSize; BlockTableIndex++) {
    auto numberOfPrimitives = EoDb::ReadUInt16(*this);

    EoDb::Read(*this, strName);
    auto wBlkTypFlgs = EoDb::ReadUInt16(*this);

    auto* block = new EoDbBlock(wBlkTypFlgs, EoGePoint3d::kOrigin);

    for (auto i = 0; i < numberOfPrimitives; i++) {
      EoDb::Read(*this, primitive);
      block->AddTail(primitive);
    }
    blocks[strName] = block;
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
    throw L"Exception EoDbBlockFile: Expecting sentinel EoDb::kEndOfSection.";
  }
}

void EoDbBlockFile::ReadFile(const CString& strPathName, EoDbBlocks& blocks) {
  CFileException e;

  if (CFile::Open(strPathName, modeRead | shareDenyNone, &e)) {
    ReadHeader();
    ReadBlocks(blocks);
  }
}

void EoDbBlockFile::ReadHeader() {
  if (EoDb::ReadUInt16(*this) != EoDb::kHeaderSection) {
    throw L"Exception EoDbBlockFile: Expecting sentinel EoDb::kHeaderSection.";
  }

  // with addition of info will loop key-value pairs till EoDb::kEndOfSection sentinel

  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
    throw L"Exception EoDbBlockFile: Expecting sentinel EoDb::kEndOfSection.";
  }
}

void EoDbBlockFile::WriteBlock(const CString& strName, EoDbBlock* block) {
  EoUInt16 wPrims{};

  auto countPosition = GetPosition();

  EoDb::Write(*this, wPrims);
  EoDb::Write(*this, strName);
  EoDb::Write(*this, block->BlockTypeFlags());

  auto BlockPosition = block->GetHeadPosition();
  while (BlockPosition != nullptr) {
    auto* primitive = block->GetNext(BlockPosition);
    if (primitive->Write(*this)) wPrims++;
  }
  auto dwPosition = GetPosition();
  Seek(static_cast<LONGLONG>(countPosition), begin);
  EoDb::Write(*this, wPrims);
  Seek(static_cast<LONGLONG>(dwPosition), begin);
}

void EoDbBlockFile::WriteBlocks(EoDbBlocks& blocks) {
  EoDb::Write(*this, EoUInt16(EoDb::kBlocksSection));
  EoDb::Write(*this, EoUInt16(blocks.GetSize()));

  CString Key;
  EoDbBlock* Block{};

  auto position = blocks.GetStartPosition();
  while (position != nullptr) {
    blocks.GetNextAssoc(position, Key, Block);
    WriteBlock(Key, Block);
  }
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}
void EoDbBlockFile::WriteFile(const CString& strPathName, EoDbBlocks& blocks) {
  CFileException e;

  CFile::Open(strPathName, modeCreate | modeWrite, &e);

  WriteHeader();
  WriteBlocks(blocks);
}
void EoDbBlockFile::WriteHeader() {
  EoDb::Write(*this, EoUInt16(EoDb::kHeaderSection));

  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}
