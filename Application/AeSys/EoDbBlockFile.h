#pragma once

/**
 * @file EoDbBlockFile.cpp
 * @brief Implements the EoDbBlockFile class for reading and writing the AeSys
 *        block library file format (.blk).
 *
 * @section blk_purpose Purpose
 * The .blk file is a standalone block library exchange format. It allows the
 * user to export the current document's entire block table to a named file and
 * reload it into any other document. This is distinct from the block table
 * embedded inside a .peg document file (handled by EoDbPegFile).
 *
 * Entry points are the two "Blocks" menu commands in AeSysDocCommands.cpp:
 *   - OnBlocksLoad()   — file-open dialog → EoDbBlockFile::ReadFile()
 *   - OnBlocksUnload() — file-save dialog → EoDbBlockFile::WriteFile()
 *
 * @section blk_format Binary File Layout
 * All multi-byte integers are written in the platform-native byte order via
 * EoDb::ReadUInt16 / EoDb::WriteUInt16. Strings are written by EoDb::Read /
 * EoDb::Write (length-prefixed CString).
 *
 * @code
 * [kHeaderSection]          uint16  — section sentinel
 * [kEndOfSection]           uint16  — header is currently empty; reserved for
 *                                     future key-value metadata pairs
 *
 * [kBlocksSection]          uint16  — section sentinel
 *   blockTableSize          uint16  — number of block records that follow
 *   For each block:
 *     numberOfPrimitives    uint16  — written as 0 initially, then patched via
 *                                     a seek-back after primitives are serialized
 *                                     (only primitives where Write() == true counted)
 *     strName               CString — block name; used as the EoDbBlocks map key
 *     blockTypeFlags        uint16  — EoDbBlock block type flag values
 *     primitives[]          N × EoDbPrimitive binary records
 * [kEndOfSection]           uint16  — section sentinel
 * @endcode
 *
 * @note Block base-point origin is NOT persisted. All blocks are reconstructed
 *       with EoGePoint3d::kOrigin on load.
 * @note Primitive handles are NOT persisted. Fresh handles are assigned by the
 *       EoDbPrimitive constructor at load time (interactive / PEG V1 pipeline).
 */

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
