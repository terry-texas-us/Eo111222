#pragma once

#include <cwctype>
#include <string_view>
#include <utility>
#include <vector>

#include "EoDbGroup.h"
#include "EoDxfAttributes.h"
#include "EoGePoint3d.h"

class EoDbBlock : public EoDbGroup {
 private:
  std::uint16_t m_blockTypeFlags;  // block type flag values
                                   //		b0 set - anonymous block
                                   //		b1 set - block has attribute definitions
                                   //		b2 set - block is an external reference
                                   //		b3 set - not used
                                   //		b4 set - block is externally dependent
                                   //		b5 set - block is a resolved external reference
                                   //		b6 set - definition is referenced
  EoGePoint3d m_firstPoint;  // block base point
  CString m_xRefPathName;  // external reference (XRef) path name
  std::uint64_t m_handle{};
  std::uint64_t m_ownerHandle{};

  /// Attribute definitions parsed from DXF ATTDEF entities within this block.
  /// Stored as the original parsed DXF entity to preserve all base properties
  /// (handle, owner, layer, color, linetype, line weight, etc.) for round-trip
  /// DXF export without duplicating the entity property set.
  std::vector<EoDxfAttDef> m_attributeDefinitions;

 public:
  EoDbBlock() { m_blockTypeFlags = 0; }
  EoDbBlock(std::int16_t flags, EoGePoint3d basePoint);
  EoDbBlock(std::int16_t flags, EoGePoint3d basePoint, const CString& name);
  EoDbBlock& operator=(const EoDbBlock&) = delete;

  [[nodiscard]] EoGePoint3d BasePoint() const noexcept { return m_firstPoint; }
  [[nodiscard]] std::uint16_t BlockTypeFlags() const noexcept { return m_blockTypeFlags; }
  [[nodiscard]] bool HasAttributes() const noexcept { return (m_blockTypeFlags & 2) == 2; }
  [[nodiscard]] bool IsAnonymous() const noexcept { return (m_blockTypeFlags & 1) == 1; }
  [[nodiscard]] bool IsFromExternalReference() const noexcept { return (m_blockTypeFlags & 4) == 4; }
  void SetBlockTypeFlags(std::uint16_t flags) { m_blockTypeFlags = flags; }
  void SetBasePoint(EoGePoint3d basePoint) { m_firstPoint = std::move(basePoint); }

  [[nodiscard]] std::uint64_t Handle() const noexcept { return m_handle; }
  [[nodiscard]] std::uint64_t OwnerHandle() const noexcept { return m_ownerHandle; }
  void SetHandle(std::uint64_t handle) noexcept { m_handle = handle; }
  void SetOwnerHandle(std::uint64_t ownerHandle) noexcept { m_ownerHandle = ownerHandle; }

  /** @brief Checks if the given block name corresponds to a system block.
   *
   * This method checks if the provided block name is not empty and starts with an underscore character ('_').
   * System blocks in AutoCAD typically have names that begin with an underscore, indicating that they are reserved
   * for internal use by the software. If the block name meets these criteria, it is considered a system block.
   *
   * @param blockName The name of the block to check.
   * @return true if the block name indicates a system block, false otherwise.
   */
  [[nodiscard]] bool IsSystemBlock(const CString& blockName) const noexcept {
    if (blockName.IsEmpty()) { return false; }

    const wchar_t first = blockName[0];
    return first == L'_';
  }

  /** @brief Checks if the given block name corresponds to a model space block.
   *
   * This method performs a case-insensitive comparison of the first 12 characters of the provided block name
   * against the string "*Model_Space". If the block name starts with "*Model_Space" (ignoring case), it is
   * considered a model space block.
   *
   * @param blockName The name of the block to check.
   * @return true if the block name indicates a model space block, false otherwise.
   */
  [[nodiscard]] bool IsModelSpace(const std::wstring& blockName) const noexcept {
    constexpr std::wstring_view prefix = L"*Model_Space";

    if (blockName.size() < prefix.size()) { return false; }

    return std::equal(prefix.cbegin(), prefix.cend(), blockName.cbegin(), [](wchar_t a, wchar_t b) noexcept {
      return std::towlower(a) == std::towlower(b);
    });
  }

  /** @brief Checks if the given block name corresponds to a paper space block.
   *
   * This method performs a case-insensitive comparison of the first 12 characters of the provided block name
   * against the string "*Paper_Space". If the block name starts with "*Paper_Space" (ignoring case), it is
   * considered a paper space block.
   *
   * @param blockName The name of the block to check.
   * @return true if the block name indicates a paper space block, false otherwise.
   */
  [[nodiscard]] bool IsPaperSpace(const std::wstring& blockName) const noexcept {
    constexpr std::wstring_view prefix = L"*Paper_Space";

    if (blockName.size() < prefix.size()) { return false; }

    return std::equal(prefix.cbegin(), prefix.cend(), blockName.cbegin(), [](wchar_t a, wchar_t b) noexcept {
      return std::towlower(a) == std::towlower(b);
    });
  }

  /// @brief Appends an attribute definition to this block's ATTDEF catalog.
  /// Called during DXF import when an ATTDEF is encountered inside a BLOCK definition.
  void AddAttributeDefinition(EoDxfAttDef attributeDefinition) {
    m_attributeDefinitions.push_back(std::move(attributeDefinition));
  }

  /// @brief Returns the list of attribute definitions stored in this block.
  [[nodiscard]] const std::vector<EoDxfAttDef>& AttributeDefinitions() const noexcept { return m_attributeDefinitions; }

  /// @brief Finds an attribute definition by its tag name.
  /// @param tagName The attribute tag to search for (case-sensitive, no spaces).
  /// @return Pointer to the matching EoDxfAttDef, or nullptr if not found.
  [[nodiscard]] const EoDxfAttDef* FindAttributeDefinitionByTag(const std::wstring& tagName) const noexcept;
};

typedef CTypedPtrMap<CMapStringToOb, CString, EoDbBlock*> EoDbBlocks;
