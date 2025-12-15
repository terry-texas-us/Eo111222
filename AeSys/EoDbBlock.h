#pragma once

#include "EoDbGroup.h"

class EoDbBlock : public EoDbGroup {
 private:
  EoUInt16 m_wBlkTypFlgs;     // block type flag values
                              //		b0 set - anonymous block
                              //		b1 set - block has attribute definitions
                              //		b2 set - block is an external reference
                              //		b3 set - not used
                              //		b4 set - block is externally dependent
                              //		b5 set - block is a resolved external reference
                              //		b6 set - definition is referenced
  EoGePoint3d m_ptBase;       // block base point
  CString m_strXRefPathName;  // external reference (XRef) path name

 public:
  EoDbBlock() { m_wBlkTypFlgs = 0; }
  EoDbBlock(EoUInt16 flags, EoGePoint3d);
  EoDbBlock(EoUInt16 flags, EoGePoint3d, const CString&);
  EoDbBlock& operator=(const EoDbBlock&) = delete;

  EoGePoint3d GetBasePt() { return m_ptBase; }
  EoUInt16 GetBlkTypFlgs() { return m_wBlkTypFlgs; }
  bool HasAttributes() { return (m_wBlkTypFlgs & 2) == 2; }
  bool IsAnonymous() { return (m_wBlkTypFlgs & 1) == 1; }
  bool IsFromExternalReference() { return (m_wBlkTypFlgs & 4) == 4; }
  void SetBlkTypFlgs(EoUInt16 flags) { m_wBlkTypFlgs = flags; }
  void SetBasePt(EoGePoint3d& pt) { m_ptBase = pt; }
};

typedef CTypedPtrMap<CMapStringToOb, CString, EoDbBlock*> CBlocks;
