#pragma once

class EoDbBlock;

class EoDbBlockFile : public CFile {
 public:
  EoDbBlockFile() {}
  EoDbBlockFile(const CString& pathName);
  EoDbBlockFile(const EoDbBlockFile&) = delete;
  EoDbBlockFile& operator=(const EoDbBlockFile&) = delete;

  ~EoDbBlockFile() override {}

  void ReadFile(const CString&, CBlocks& blocks);
  void ReadBlocks(CBlocks& blocks);

  /// @brief Reads and validates the header section of an EoDb block file.
  void ReadHeader();
  void WriteBlock(const CString& name, EoDbBlock* block);
  void WriteBlocks(CBlocks& blocks);
  void WriteFile(const CString& pathName, CBlocks& blocks);
  void WriteHeader();
};
