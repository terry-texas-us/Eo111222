#include "stdafx.h"

#include "EoDbBlock.h"

EoDbBlock::EoDbBlock(EoUInt16 wBlkTypFlgs, EoGePoint3d ptBase) {
  m_wBlkTypFlgs = wBlkTypFlgs;
  m_ptBase = ptBase;
}
EoDbBlock::EoDbBlock(EoUInt16 wBlkTypFlgs, EoGePoint3d ptBase, const CString& strXRefPathName) {
  m_wBlkTypFlgs = wBlkTypFlgs;
  m_ptBase = ptBase;
  m_strXRefPathName = strXRefPathName;
}
