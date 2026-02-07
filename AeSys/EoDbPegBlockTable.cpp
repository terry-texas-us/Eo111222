#include "Stdafx.h"

#include "AeSysDoc.h"
#include "EoDbBlock.h"

int AeSysDoc::GetBlockReferenceCount(const CString& name) {
  int count = 0;

  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    auto* layer = GetLayerTableLayerAt(w);
    count += layer->GetBlockRefCount(name);
  }
  CString key;
  EoDbBlock* block{};

  auto position = m_BlocksTable.GetStartPosition();
  while (position != nullptr) {
    m_BlocksTable.GetNextAssoc(position, key, block);
    count += block->GetBlockRefCount(name);
  }
  return count;
}

bool AeSysDoc::LookupBlock(CString name, EoDbBlock*& block) {
  if (m_BlocksTable.Lookup(name, block)) { return true; }
  block = nullptr;
  return false;
}

void AeSysDoc::RemoveAllBlocks() {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"RemoveAllBlocks() - Count: %d\n",
            static_cast<int>(m_BlocksTable.GetCount()));
  CString name;
  EoDbBlock* block{};

  auto blockPosition = m_BlocksTable.GetStartPosition();
  while (blockPosition != nullptr) {
    m_BlocksTable.GetNextAssoc(blockPosition, name, block);
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"  Deleting block: %s with %d primitives\n", name.GetString(),
              static_cast<int>(block->GetCount()));
    block->DeletePrimitivesAndRemoveAll();
    delete block;
  }
  m_BlocksTable.RemoveAll();
}

void AeSysDoc::RemoveUnusedBlocks() {
  CString Name;
  EoDbBlock* Block{};

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
