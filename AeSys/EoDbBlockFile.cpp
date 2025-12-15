#include "stdafx.h"

#include "EoDbBlockFile.h"
#include "EoDbPrimitive.h"

// EoDb::kHeaderSection sentinel				EoUInt16 0x0101
//		{0 or more key-value pairs}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff
// EoDb::kBlocksSection sentinel				EoUInt16 0x0103
//		Number of blocks						EoUInt16
//		{0 or more block definitions}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff

EoDbBlockFile::EoDbBlockFile(const CString& pathName) { CFile::Open(pathName, modeReadWrite | shareDenyNone); }
void EoDbBlockFile::ReadBlocks(CBlocks& blocks) {
  if (EoDb::ReadUInt16(*this) != EoDb::kBlocksSection) {
    throw L"Exception EoDbBlockFile: Expecting sentinel EoDb::kBlocksSection.";
  }

  CString strName;

  EoDbPrimitive* Primitive;

  EoUInt16 BlockTableSize = EoDb::ReadUInt16(*this);
  for (int BlockTableIndex = 0; BlockTableIndex < BlockTableSize; BlockTableIndex++) {
    int iPrims = EoDb::ReadUInt16(*this);

    EoDb::Read(*this, strName);
    EoUInt16 wBlkTypFlgs = EoDb::ReadUInt16(*this);

    auto* Block = new EoDbBlock(wBlkTypFlgs, EoGePoint3d::kOrigin);

    for (int i = 0; i < iPrims; i++) {
      EoDb::Read(*this, Primitive);
      Block->AddTail(Primitive);
    }
    blocks[strName] = Block;
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection) {
    throw L"Exception EoDbBlockFile: Expecting sentinel EoDb::kEndOfSection.";
  }
}
void EoDbBlockFile::ReadFile(const CString& strPathName, CBlocks& blks) {
  CFileException e;

  if (CFile::Open(strPathName, modeRead | shareDenyNone, &e)) {
    ReadHeader();
    ReadBlocks(blks);
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
  EoUInt16 wPrims = 0;

  ULONGLONG dwCountPosition = GetPosition();

  EoDb::Write(*this, wPrims);
  EoDb::Write(*this, strName);
  EoDb::Write(*this, block->GetBlkTypFlgs());

  POSITION BlockPosition = block->GetHeadPosition();
  while (BlockPosition != nullptr) {
    EoDbPrimitive* Primitive = block->GetNext(BlockPosition);
    if (Primitive->Write(*this)) wPrims++;
  }
  ULONGLONG dwPosition = GetPosition();
  Seek(static_cast<LONGLONG>(dwCountPosition), begin);
  EoDb::Write(*this, wPrims);
  Seek(static_cast<LONGLONG>(dwPosition), begin);
}
void EoDbBlockFile::WriteBlocks(CBlocks& blocks) {
  EoDb::Write(*this, EoUInt16(EoDb::kBlocksSection));
  EoDb::Write(*this, EoUInt16(blocks.GetSize()));

  CString Key;
  EoDbBlock* Block;

  POSITION Position = blocks.GetStartPosition();
  while (Position != nullptr) {
    blocks.GetNextAssoc(Position, Key, Block);
    WriteBlock(Key, Block);
  }
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}
void EoDbBlockFile::WriteFile(const CString& strPathName, CBlocks& blks) {
  CFileException e;

  CFile::Open(strPathName, modeCreate | modeWrite, &e);

  WriteHeader();
  WriteBlocks(blks);
}
void EoDbBlockFile::WriteHeader() {
  EoDb::Write(*this, EoUInt16(EoDb::kHeaderSection));

  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}
