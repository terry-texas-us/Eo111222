#pragma once

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

  /// @brief Appends an attribute definition to this block's ATTDEF catalog.
  /// Called during DXF import when an ATTDEF is encountered inside a BLOCK definition.
  void AddAttributeDefinition(EoDxfAttDef attributeDefinition) {
    m_attributeDefinitions.push_back(std::move(attributeDefinition));
  }

  /// @brief Returns the list of attribute definitions stored in this block.
  [[nodiscard]] const std::vector<EoDxfAttDef>& AttributeDefinitions() const noexcept {
    return m_attributeDefinitions;
  }

  /// @brief Finds an attribute definition by its tag name.
  /// @param tagName The attribute tag to search for (case-sensitive, no spaces).
  /// @return Pointer to the matching EoDxfAttDef, or nullptr if not found.
  [[nodiscard]] const EoDxfAttDef* FindAttributeDefinitionByTag(const std::wstring& tagName) const noexcept;
};

typedef CTypedPtrMap<CMapStringToOb, CString, EoDbBlock*> EoDbBlocks;
