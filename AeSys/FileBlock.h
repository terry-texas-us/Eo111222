#pragma once

class EoDbBlock;

class CFileBlock : public CFile
{
public:
	CFileBlock() 
	{
	}
	CFileBlock(const CString& strPathName);
	
	virtual ~CFileBlock() 
	{
	}
	void	ReadFile(const CString&, CBlocks& blks);
	void	ReadBlocks(CBlocks& blks);
	void	ReadHeader();
	void	WriteBlock(const CString& strName, EoDbBlock* pBlock);
	void	WriteBlocks(CBlocks& blks);
	void	WriteFile(const CString& strPathName, CBlocks& blks);
	void	WriteHeader();
};
