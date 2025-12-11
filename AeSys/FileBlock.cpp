#include "stdafx.h"

#include "FileBlock.h"

// EoDb::kHeaderSection sentinel				EoUInt16 0x0101
//		{0 or more key-value pairs}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff
// EoDb::kBlocksSection sentinel				EoUInt16 0x0103
//		Number of blocks						EoUInt16
//		{0 or more block definitions}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff

CFileBlock::CFileBlock(const CString& strPathName)
{
	CFile::Open(strPathName, modeReadWrite | shareDenyNone);
}
void CFileBlock::ReadBlocks(CBlocks& blocks)
{
	if (EoDb::ReadUInt16(*this) != EoDb::kBlocksSection)
		throw L"Exception CFileBlock: Expecting sentinel EoDb::kBlocksSection.";  
	
	CString strName;

	EoDbPrimitive* Primitive;

	EoUInt16 BlockTableSize = EoDb::ReadUInt16(*this);
	for (int BlockTableIndex = 0; BlockTableIndex < BlockTableSize; BlockTableIndex++)
	{
		int iPrims = EoDb::ReadUInt16(*this);

		EoDb::Read(*this, strName);
		EoUInt16 wBlkTypFlgs = EoDb::ReadUInt16(*this);
	
		EoDbBlock* Block = new EoDbBlock(wBlkTypFlgs, EoGePoint3d::kOrigin);

		for (int i = 0; i < iPrims; i++)
		{
			EoDb::Read(*this, Primitive);
			Block->AddTail(Primitive); 
		}
		blocks[strName] = Block;
	}
	if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection)
		throw L"Exception CFileBlock: Expecting sentinel EoDb::kEndOfSection.";  
}
void CFileBlock::ReadFile(const CString& strPathName, CBlocks& blks)
{
	CFileException e;

	if (CFile::Open(strPathName, modeRead | shareDenyNone, &e))
	{
		ReadHeader();
		ReadBlocks(blks);
	}
}
void CFileBlock::ReadHeader()
{
	if (EoDb::ReadUInt16(*this) != EoDb::kHeaderSection)
		throw L"Exception CFileBlock: Expecting sentinel EoDb::kHeaderSection.";
	
	// 	with addition of info where will loop key-value pairs till EoDb::kEndOfSection sentinel

	if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection)
		throw L"Exception CFileBlock: Expecting sentinel EoDb::kEndOfSection.";
}
void CFileBlock::WriteBlock(const CString& strName, EoDbBlock* pBlock)
{
	EoUInt16 wPrims = 0;
	
	ULONGLONG dwCountPosition = GetPosition();
	
	EoDb::Write(*this, wPrims);	
	EoDb::Write(*this, strName);
	EoDb::Write(*this, pBlock->GetBlkTypFlgs());
	
	POSITION pos = pBlock->GetHeadPosition();
	while (pos != 0)
	{
		EoDbPrimitive* pPrim = pBlock->GetNext(pos);
		if (pPrim->Write(*this))
			wPrims++;
	}
	ULONGLONG dwPosition = GetPosition();
	Seek(dwCountPosition, begin);
	EoDb::Write(*this, wPrims);
	Seek(dwPosition, begin);
}
void CFileBlock::WriteBlocks(CBlocks& blks)
{
	EoDb::Write(*this, EoUInt16(EoDb::kBlocksSection));
	EoDb::Write(*this, EoUInt16(blks.GetSize()));

	CString strKey;
	EoDbBlock* pBlock;

	POSITION pos = blks.GetStartPosition();
	while (pos != NULL)
	{
		blks.GetNextAssoc(pos, strKey, pBlock);
		WriteBlock(strKey, pBlock);
	}
	EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}
void CFileBlock::WriteFile(const CString& strPathName, CBlocks& blks)
{
	CFileException e;
	
	CFile::Open(strPathName, modeCreate | modeWrite, &e);

	WriteHeader();
	WriteBlocks(blks);
}
void CFileBlock::WriteHeader()
{
	EoDb::Write(*this, EoUInt16(EoDb::kHeaderSection));

	EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}