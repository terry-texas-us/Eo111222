#include "Stdafx.h"

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbLayer.h"
#include "EoDbPrimitive.h"
#include "EoGeTransformMatrix.h"
#include "EoDocCommand.h"

// --- EoDocCmdDeleteGroup ---

void EoDocCmdDeleteGroup::Discard(AeSysDoc* doc) noexcept {
  if (m_ownedGroup != nullptr) {
    doc->UnregisterGroupHandles(m_ownedGroup.get());
    m_ownedGroup->DeletePrimitivesAndRemoveAll();
  }
}

void EoDocCmdDeleteGroup::Execute(AeSysDoc* doc) {
  // Re-delete: take ownership back from the document layer.
  m_layer->Remove(m_group);
  doc->RemoveGroupFromAllViews(m_group);
  doc->UnregisterGroupHandles(m_group);
  doc->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_group);
  m_ownedGroup.reset(m_group);
}

void EoDocCmdDeleteGroup::Undo(AeSysDoc* doc) {
  // Restore: release ownership to the document layer.
  m_layer->AddTail(m_group);
  doc->AddGroupToAllViews(m_group);
  doc->RegisterGroupHandles(m_group);
  doc->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_group);
  m_ownedGroup.release();
}

// --- EoDocCmdDeleteLastGroup ---

void EoDocCmdDeleteLastGroup::Discard(AeSysDoc* doc) noexcept {
  if (m_ownedGroup != nullptr) {
    doc->UnregisterGroupHandles(m_ownedGroup.get());
    m_ownedGroup->DeletePrimitivesAndRemoveAll();
  }
}

void EoDocCmdDeleteLastGroup::Execute(AeSysDoc* doc) {
  // Re-delete: remove the tail of the work layer visible list.
  // On redo, m_group is already known — remove it directly from its layer.
  m_layer->Remove(m_group);
  doc->RemoveGroupFromAllViews(m_group);
  doc->UnregisterGroupHandles(m_group);
  doc->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_group);
  m_ownedGroup.reset(m_group);
}

void EoDocCmdDeleteLastGroup::Undo(AeSysDoc* doc) {
  m_layer->AddTail(m_group);
  doc->AddGroupToAllViews(m_group);
  doc->RegisterGroupHandles(m_group);
  doc->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_group);
  m_ownedGroup.release();
}

// --- EoDocCmdTrapCut ---

void EoDocCmdTrapCut::Discard(AeSysDoc* doc) noexcept {
  for (auto& entry : m_entries) {
    if (entry.owned != nullptr) {
      doc->UnregisterGroupHandles(entry.owned.get());
      entry.owned->DeletePrimitivesAndRemoveAll();
    }
  }
}

void EoDocCmdTrapCut::Execute(AeSysDoc* doc) {
  // Re-cut: remove each group from its origin layer, take ownership.
  for (auto& entry : m_entries) {
    entry.layer->Remove(entry.group);
    doc->RemoveGroupFromAllViews(entry.group);
    doc->UnregisterGroupHandles(entry.group);
    doc->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, entry.group);
    entry.owned.reset(entry.group);
  }
  doc->UpdateAllViews(nullptr, 0L, nullptr);
}

void EoDocCmdTrapCut::Undo(AeSysDoc* doc) {
  // Restore each group to its origin layer and re-trap it.
  for (auto& entry : m_entries) {
    entry.layer->AddTail(entry.group);
    doc->AddGroupToAllViews(entry.group);
    doc->RegisterGroupHandles(entry.group);
    entry.owned.release();
  }
  // Re-populate the trap with all restored groups so the user sees them highlighted.
  for (auto& entry : m_entries) {
    doc->AddGroupToTrap(entry.group);
  }
  doc->UpdateAllViews(nullptr, 0L, nullptr);
}

// --- EoDocCmdDeletePrimitive ---

void EoDocCmdDeletePrimitive::Discard(AeSysDoc* doc) noexcept {
  if (m_sourceGroup == nullptr) {
    // Solo: wrapper IS the original group — free its primitives then the group.
    if (m_wrapperGroup != nullptr) {
      doc->UnregisterGroupHandles(m_wrapperGroup.get());
      m_wrapperGroup->DeletePrimitivesAndRemoveAll();
    }
  } else {
    // Multi: wrapper holds one extracted primitive (on redo branch) or is empty (on undo branch).
    if (m_primitive != nullptr && m_wrapperGroup != nullptr && !m_wrapperGroup->IsEmpty()) {
      doc->UnregisterHandle(m_primitive->Handle());
      m_wrapperGroup->DeletePrimitivesAndRemoveAll();
    }
  }
}

void EoDocCmdDeletePrimitive::Execute(AeSysDoc* doc) {
  if (m_sourceGroup == nullptr) {
    // Solo re-delete: remove the whole group from its layer.
    m_layer->Remove(m_rawWrapper);
    doc->RemoveGroupFromAllViews(m_rawWrapper);
    doc->UnregisterGroupHandles(m_rawWrapper);
    doc->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_rawWrapper);
    m_wrapperGroup.reset(m_rawWrapper);
  } else {
    // Multi re-delete: move primitive from sourceGroup back into the wrapper.
    doc->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_sourceGroup);
    m_sourceGroup->FindAndRemovePrim(m_primitive);
    doc->UnregisterHandle(m_primitive->Handle());
    m_wrapperGroup->AddTail(m_primitive);
    doc->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_sourceGroup);
  }
}

void EoDocCmdDeletePrimitive::Undo(AeSysDoc* doc) {
  if (m_sourceGroup == nullptr) {
    // Solo restore: add group back to its layer.
    m_layer->AddTail(m_rawWrapper);
    doc->AddGroupToAllViews(m_rawWrapper);
    doc->RegisterGroupHandles(m_rawWrapper);
    doc->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_rawWrapper);
    m_wrapperGroup.release();
  } else {
    // Multi restore: move primitive from wrapper back into sourceGroup.
    doc->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_sourceGroup);
    m_wrapperGroup->FindAndRemovePrim(m_primitive);
    doc->RegisterHandle(m_primitive);
    m_sourceGroup->AddTail(m_primitive);
    doc->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_sourceGroup);
    // Wrapper stays owned by command (empty) — ready for re-Execute.
  }
}

// --- EoDocCmdAddGroup ---

void EoDocCmdAddGroup::Execute(AeSysDoc* doc) {
  // Redo: re-add the group to the layer and all views.
  m_layer->AddTail(m_group);
  doc->AddGroupToAllViews(m_group);
  doc->RegisterGroupHandles(m_group);
  doc->UpdateAllViews(nullptr, EoDb::kGroupSafe, m_group);
  m_ownedGroup.release();
}

void EoDocCmdAddGroup::Undo(AeSysDoc* doc) {
  m_layer->Remove(m_group);
  doc->RemoveGroupFromAllViews(m_group);
  doc->UnregisterGroupHandles(m_group);
  doc->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, m_group);
  m_ownedGroup.reset(m_group);
}

void EoDocCmdAddGroup::Discard(AeSysDoc* doc) noexcept {
  if (m_ownedGroup) {
    doc->UnregisterGroupHandles(m_group);
    m_ownedGroup->DeletePrimitivesAndRemoveAll();
  }
}

// --- EoDocCmdTransformGroups ---

static void ApplyTransformToGroups(AeSysDoc* doc,
    const std::vector<EoDbGroup*>& groups,
    const EoGeTransformMatrix& matrix) {
  for (auto* group : groups) {
    doc->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);
    group->Transform(matrix);
    doc->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
  }
}

void EoDocCmdTransformGroups::Execute(AeSysDoc* doc) {
  ApplyTransformToGroups(doc, m_groups, m_forwardMatrix);
}

void EoDocCmdTransformGroups::Undo(AeSysDoc* doc) {
  ApplyTransformToGroups(doc, m_groups, m_inverseMatrix);
}
