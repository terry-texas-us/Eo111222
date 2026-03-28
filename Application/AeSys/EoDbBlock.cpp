#include "Stdafx.h"

#include "EoDbBlock.h"
#include "EoGePoint3d.h"

EoDbBlock::EoDbBlock(std::int16_t blockTypeFlags, EoGePoint3d basePoint) {
  m_blockTypeFlags = blockTypeFlags;
  m_firstPoint = basePoint;
}

EoDbBlock::EoDbBlock(std::int16_t blockTypeFlags, EoGePoint3d basePoint, const CString& xRefPathName) {
  m_blockTypeFlags = blockTypeFlags;
  m_firstPoint = basePoint;
  m_xRefPathName = xRefPathName;
}

const EoDxfAttDef* EoDbBlock::FindAttributeDefinitionByTag(const std::wstring& tagName) const noexcept {
  for (const auto& attributeDefinition : m_attributeDefinitions) {
    if (attributeDefinition.m_tagString == tagName) {
      return &attributeDefinition;
    }
  }
  return nullptr;
}
