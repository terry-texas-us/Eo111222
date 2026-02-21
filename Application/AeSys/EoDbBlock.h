#pragma once

#include <utility>

#include "EoDbGroup.h"
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
  EoGePoint3d m_basePoint;    // block base point
  CString m_xRefPathName;     // external reference (XRef) path name

 public:
  EoDbBlock() { m_blockTypeFlags = 0; }
  EoDbBlock(std::uint16_t flags, EoGePoint3d basePoint);
  EoDbBlock(std::uint16_t flags, EoGePoint3d basePoint, const CString& name);
  EoDbBlock& operator=(const EoDbBlock&) = delete;

  const EoGePoint3d& BasePoint() const noexcept { return m_basePoint; }
  std::uint16_t BlockTypeFlags() const noexcept { return m_blockTypeFlags; }
  bool HasAttributes() const { return (m_blockTypeFlags & 2) == 2; }
  bool IsAnonymous() const { return (m_blockTypeFlags & 1) == 1; }
  bool IsFromExternalReference() const { return (m_blockTypeFlags & 4) == 4; }
  void SetBlockTypeFlags(std::uint16_t flags) { m_blockTypeFlags = flags; }
  void SetBasePoint(EoGePoint3d basePoint) { m_basePoint = std::move(basePoint); }
};

typedef CTypedPtrMap<CMapStringToOb, CString, EoDbBlock*> EoDbBlocks;
