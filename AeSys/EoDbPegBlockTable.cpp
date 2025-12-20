#include "stdafx.h"
#include "AeSysDoc.h"

int AeSysDoc::GetBlockReferenceCount(const CString& name) {
  int Count = 0;

  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(w);
    Count += Layer->GetBlockRefCount(name);
  }
  CString Key;
  EoDbBlock* Block;

  auto Position = m_BlocksTable.GetStartPosition();
  while (Position != nullptr) {
    m_BlocksTable.GetNextAssoc(Position, Key, Block);
    Count += Block->GetBlockRefCount(name);
  }
  return (Count);
}
bool AeSysDoc::LookupBlock(CString name, EoDbBlock*& block) {
  if (m_BlocksTable.Lookup(name, block)) { return true; }
  block = nullptr;
  return false;
}
void AeSysDoc::RemoveAllBlocks() {
  CString Name;
  EoDbBlock* Block;

  auto BlockPosition = m_BlocksTable.GetStartPosition();
  while (BlockPosition != nullptr) {
    m_BlocksTable.GetNextAssoc(BlockPosition, Name, Block);
    Block->DeletePrimitivesAndRemoveAll();
    delete Block;
  }
  m_BlocksTable.RemoveAll();
}
void AeSysDoc::RemoveUnusedBlocks() {
  CString Name;
  EoDbBlock* Block;

  auto BlockPosition = m_BlocksTable.GetStartPosition();
  while (BlockPosition != nullptr) {
    m_BlocksTable.GetNextAssoc(BlockPosition, Name, Block);
    if (GetBlockReferenceCount(Name) == 0) {
      //Note: Deletion by key may cause loop problems
      m_BlocksTable.RemoveKey(Name);
      Block->DeletePrimitivesAndRemoveAll();
      delete Block;
    }
  }
}
