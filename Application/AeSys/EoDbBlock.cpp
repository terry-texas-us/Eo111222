#include "Stdafx.h"

#include "EoDbBlock.h"
#include "EoGePoint3d.h"

EoDbBlock::EoDbBlock(std::uint16_t blockTypeFlags, EoGePoint3d basePoint) {
  m_blockTypeFlags = blockTypeFlags;
  m_firstPoint = basePoint;
}

EoDbBlock::EoDbBlock(std::uint16_t blockTypeFlags, EoGePoint3d basePoint, const CString& xRefPathName) {
  m_blockTypeFlags = blockTypeFlags;
  m_firstPoint = basePoint;
  m_xRefPathName = xRefPathName;
}
