#pragma once

class EoDbBlock;

class EoDbBlockFile : public CFile {
 public:
  EoDbBlockFile() {}
  EoDbBlockFile(const CString& strPathName);

  virtual ~EoDbBlockFile() {}
  void ReadFile(const CString&, CBlocks& blks);
  void ReadBlocks(CBlocks& blks);
  void ReadHeader();
  void WriteBlock(const CString& strName, EoDbBlock* Block);
  void WriteBlocks(CBlocks& blks);
  void WriteFile(const CString& strPathName, CBlocks& blks);
  void WriteHeader();
};
