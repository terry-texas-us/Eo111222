#include "Stdafx.h"

#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"

int AeSysDoc::GetBlockReferenceCount(const CString& name) {
  int count = 0;

  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
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
  ATLTRACE2(traceGeneral, 3, L"RemoveAllBlocks() - Count: %d\n", static_cast<int>(m_BlocksTable.GetCount()));
  CString name;
  EoDbBlock* block{};

  auto blockPosition = m_BlocksTable.GetStartPosition();
  while (blockPosition != nullptr) {
    m_BlocksTable.GetNextAssoc(blockPosition, name, block);
    ATLTRACE2(traceGeneral, 3, L"  Deleting block: %s with %d primitives\n", name.GetString(),
        static_cast<int>(block->GetCount()));
    UnregisterHandle(block->Handle());
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
      // Note: Deletion by key may cause loop problems
      m_BlocksTable.RemoveKey(Name);
      UnregisterHandle(Block->Handle());
      UnregisterGroupHandles(Block);
      Block->DeletePrimitivesAndRemoveAll();
      delete Block;
    }
  }
}

bool AeSysDoc::RenameBlock(const CString& oldName, const CString& newName) {
  if (newName.IsEmpty()) { return false; }
  if (newName[0] == L'*') { return false; }  // reserved system/anonymous prefix
  if (oldName == newName) { return true; }

  EoDbBlock* block{};
  if (!m_BlocksTable.Lookup(oldName, block)) { return false; }

  EoDbBlock* existingBlock{};
  if (m_BlocksTable.Lookup(newName, existingBlock)) { return false; }  // name already taken

  // Re-key the block table entry
  m_BlocksTable.RemoveKey(oldName);
  m_BlocksTable.SetAt(newName, block);

  // Helper: patches EoDbBlockReference primitives inside a single EoDbGroup
  auto patchGroup = [&](EoDbGroup* group) {
    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);
      if (primitive->Is(EoDb::kGroupReferencePrimitive)) {
        auto* blockRef = static_cast<EoDbBlockReference*>(primitive);
        if (blockRef->BlockName() == oldName) { blockRef->SetName(newName); }
      }
    }
  };

  // Helper: patches all groups in a layer (EoDbGroupList)
  auto patchGroupList = [&](EoDbGroupList& groupList) {
    auto groupPosition = groupList.GetHeadPosition();
    while (groupPosition != nullptr) {
      patchGroup(groupList.GetNext(groupPosition));
    }
  };

  // Patch model-space layers
  for (INT_PTR i = 0; i < m_modelSpaceLayers.GetSize(); i++) {
    patchGroupList(*m_modelSpaceLayers.GetAt(i));
  }

  // Patch all paper-space layout layers
  for (auto& [handle, layers] : m_paperSpaceLayoutLayers) {
    for (INT_PTR i = 0; i < layers.GetSize(); i++) {
      patchGroupList(*layers.GetAt(i));
    }
  }

  // Patch block definitions (nested INSERT inside another block)
  CString key;
  EoDbBlock* blk{};
  auto blockPosition = m_BlocksTable.GetStartPosition();
  while (blockPosition != nullptr) {
    m_BlocksTable.GetNextAssoc(blockPosition, key, blk);
    patchGroup(blk);
  }

  SetModifiedFlag();
  return true;
}
