#include "Stdafx.h"

#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbBlockFile.h"
#include "EoDbPrimitive.h"
#include "EoGePoint3d.h"

// EoDb::kHeaderSection sentinel				std::uint16_t 0x0101
//		{0 or more key-value pairs}
// EoDb::kEndOfSection sentinel					std::uint16_t 0x01ff
// EoDb::kBlocksSection sentinel				std::uint16_t 0x0103
//		Number of blocks						std::uint16_t
//		{0 or more block definitions}
// EoDb::kEndOfSection sentinel					std::uint16_t 0x01ff

EoDbBlockFile::EoDbBlockFile(const CString& pathName) { CFile::Open(pathName, modeReadWrite | shareDenyNone); }

void EoDbBlockFile::ReadBlocks(EoDbBlocks& blocks) {
  if (EoDb::ReadUInt16(*this) != EoDb::kBlocksSection) {
    throw L"Exception EoDbBlockFile: Expecting sentinel EoDb::kBlocksSection.";
  }
  CString strName;

  EoDbPrimitive* primitive{};

  const auto blockTableSize = EoDb::ReadUInt16(*this);
  for (int BlockTableIndex = 0; BlockTableIndex < blockTableSize; BlockTableIndex++) {
    const auto numberOfPrimitives = EoDb::ReadUInt16(*this);

    EoDb::Read(*this, strName);
    const auto wBlkTypFlgs = EoDb::ReadUInt16(*this);

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
  std::uint16_t wPrims{};

  const auto countPosition = GetPosition();

  EoDb::WriteUInt16(*this, wPrims);
  EoDb::Write(*this, strName);
  EoDb::WriteUInt16(*this, block->BlockTypeFlags());

  auto BlockPosition = block->GetHeadPosition();
  while (BlockPosition != nullptr) {
    auto* primitive = block->GetNext(BlockPosition);
    if (primitive->Write(*this)) { wPrims++; }
  }
  const auto dwPosition = GetPosition();
  Seek(static_cast<LONGLONG>(countPosition), begin);
  EoDb::WriteUInt16(*this, wPrims);
  Seek(static_cast<LONGLONG>(dwPosition), begin);
}

void EoDbBlockFile::WriteBlocks(EoDbBlocks& blocks) {
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kBlocksSection));
  EoDb::WriteUInt16(*this, std::uint16_t(blocks.GetSize()));

  CString Key;
  EoDbBlock* Block{};

  auto position = blocks.GetStartPosition();
  while (position != nullptr) {
    blocks.GetNextAssoc(position, Key, Block);
    WriteBlock(Key, Block);
  }
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfSection));
}
void EoDbBlockFile::WriteFile(const CString& strPathName, EoDbBlocks& blocks) {
  CFileException e;

  CFile::Open(strPathName, modeCreate | modeWrite, &e);

  WriteHeader();
  WriteBlocks(blocks);
}
void EoDbBlockFile::WriteHeader() {
  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kHeaderSection));

  EoDb::WriteUInt16(*this, std::uint16_t(EoDb::kEndOfSection));
}
