#pragma once

class EoDbBlock;

class EoDbBlockFile : public CFile {
 public:
  EoDbBlockFile() {}
  EoDbBlockFile(const CString& pathName);
  EoDbBlockFile(const EoDbBlockFile&) = delete;
  EoDbBlockFile& operator=(const EoDbBlockFile&) = delete;

  ~EoDbBlockFile() override {}

  void ReadFile(const CString&, EoDbBlocks& blocks);
  /**
   * @brief Reads block definitions from the file into the provided EoDbBlocks map.
   * 
   * This function expects the file to contain a blocks section, starting with the
   * EoDb::kBlocksSection sentinel and ending with the EoDb::kEndOfSection sentinel.
   * Each block definition includes its name, type flags, and associated primitives.
   *
   * @param blocks A reference to a EoDbBlocks map where the read blocks will be stored.
   * @throws CString If the expected sentinels are not found in the file.
   */
  void ReadBlocks(EoDbBlocks& blocks);

  /// @brief Reads and validates the header section of an EoDb block file.
  void ReadHeader();
  void WriteBlock(const CString& name, EoDbBlock* block);
  void WriteBlocks(EoDbBlocks& blocks);
  void WriteFile(const CString& pathName, EoDbBlocks& blocks);
  void WriteHeader();
};
