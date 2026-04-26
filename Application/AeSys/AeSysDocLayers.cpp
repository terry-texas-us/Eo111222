#include "Stdafx.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbJobFile.h"
#include "EoDbLayer.h"
#include "EoDbTracingFile.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "Resource.h"

void AeSysDoc::GetExtents(
    AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& transformMatrix) {
  ptMin.Set(Eo::boundsMax, Eo::boundsMax, Eo::boundsMax);
  ptMax.Set(Eo::boundsMin, Eo::boundsMin, Eo::boundsMin);

  // In editor mode, compute extents from the temporary editing layer only
  if (IsInEditor()) {
    EoDbLayer* editLayer = IsEditingBlock() ? m_blockEditLayer : m_tracingEditLayer;
    if (editLayer != nullptr) {
      editLayer->GetExtents(view, ptMin, ptMax, transformMatrix);
      return;
    }
  }

  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (!layer->IsOff()) { layer->GetExtents(view, ptMin, ptMax, transformMatrix); }
  }
}

int AeSysDoc::NumberOfGroupsInWorkLayer() {
  // m_workLayer may be a temporary editor layer not registered in the layer table
  // (e.g. *BlockEdit or *TracingEdit). Count it directly to avoid returning zero
  // during an isolating editor session.
  if (m_workLayer != nullptr) { return static_cast<int>(m_workLayer->GetCount()); }

  int count{};
  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (layer->IsWork()) { count += static_cast<int>(layer->GetCount()); }
  }
  return count;
}

int AeSysDoc::NumberOfGroupsInActiveLayers() {
  // In an isolating editor session only the edit layer is visible/relevant;
  // active model-space layers are hidden and must not inflate the work count.
  if (IsInEditor()) { return 0; }

  int count{};

  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (layer->IsActive()) { count += static_cast<int>(layer->GetCount()); }
  }
  return static_cast<int>(count);
}

EoDbLayer* AeSysDoc::GetLayerTableLayer(const CString& name) {
  const auto i = FindLayerTableLayer(name);
  return (i < 0 ? nullptr : ActiveSpaceLayers().GetAt(i));
}
EoDbLayer* AeSysDoc::GetLayerTableLayerAt(int index) {
  auto& layers = ActiveSpaceLayers();
  return (index >= static_cast<int>(layers.GetSize()) ? nullptr : layers.GetAt(index));
}

int AeSysDoc::FindLayerTableLayer(const CString& name) const {
  const auto& layers = ActiveSpaceLayers();
  for (auto i = 0; i < layers.GetSize(); i++) {
    auto* layer = layers.GetAt(i);
    if (name.CompareNoCase(layer->Name()) == 0) { return i; }
  }
  return -1;
}

void AeSysDoc::RemoveAllLayerTableLayers() {
  auto clearLayers = [this](CLayers& layers) {
    for (INT_PTR i = 0; i < layers.GetSize(); i++) {
      auto* layer = layers.GetAt(i);
      if (layer) {
        UnregisterHandle(layer->Handle());
        layer->DeleteGroupsAndRemoveAll();
        delete layer;
      }
    }
    layers.RemoveAll();
  };
  clearLayers(m_modelSpaceLayers);
  for (auto& [handle, layers] : m_paperSpaceLayoutLayers) {
    clearLayers(layers);
  }
  m_paperSpaceLayoutLayers.clear();
  m_activeLayoutHandle = EoDxf::Handles::PaperSpaceBlockRecord;
}

void AeSysDoc::RemoveLayerTableLayerAt(int i) {
  auto& layers = ActiveSpaceLayers();
  auto* layer = layers.GetAt(i);

  auto position = layer->GetHeadPosition();
  while (position != nullptr) { UnregisterGroupHandles(layer->GetNext(position)); }

  UnregisterHandle(layer->Handle());
  layer->DeleteGroupsAndRemoveAll();
  delete layer;

  layers.RemoveAt(i);
}

void AeSysDoc::RemoveEmptyLayers() {
  auto& layers = ActiveSpaceLayers();
  for (int index = static_cast<int>(layers.GetSize()) - 1; index > 0; index--) {
    auto* layer = layers.GetAt(index);

    if (layer && layer->IsEmpty()) {
      UnregisterHandle(layer->Handle());
      layer->DeleteGroupsAndRemoveAll();
      delete layer;
      layers.RemoveAt(index);
    }
  }
}

CLayers& AeSysDoc::ActiveSpaceLayers() {
  return (m_activeSpace == EoDxf::Space::PaperSpace) ? PaperSpaceLayers() : m_modelSpaceLayers;
}

const CLayers& AeSysDoc::ActiveSpaceLayers() const {
  return (m_activeSpace == EoDxf::Space::PaperSpace) ? PaperSpaceLayers() : m_modelSpaceLayers;
}

CLayers& AeSysDoc::SpaceLayers(EoDxf::Space space) {
  return (space == EoDxf::Space::PaperSpace) ? PaperSpaceLayers() : m_modelSpaceLayers;
}

int AeSysDoc::GetLayerTableSize() const { return static_cast<int>(ActiveSpaceLayers().GetSize()); }

void AeSysDoc::AddLayerTableLayer(EoDbLayer* layer) {
  ActiveSpaceLayers().Add(layer);
  RegisterHandle(layer);
}

void AeSysDoc::AddLayerToSpace(EoDbLayer* layer, EoDxf::Space space) {
  SpaceLayers(space).Add(layer);
  RegisterHandle(layer);
}

void AeSysDoc::AddLayerToLayout(EoDbLayer* layer, std::uint64_t blockRecordHandle) {
  m_paperSpaceLayoutLayers[blockRecordHandle].Add(layer);
  RegisterHandle(layer);
}

EoDbLayer* AeSysDoc::FindLayerInSpace(const CString& name, EoDxf::Space space) {
  auto& layers = SpaceLayers(space);
  for (INT_PTR i = 0; i < layers.GetSize(); i++) {
    auto* layer = layers.GetAt(i);
    if (name.CompareNoCase(layer->Name()) == 0) { return layer; }
  }
  return nullptr;
}

EoDbLayer* AeSysDoc::FindLayerInLayout(const CString& name, std::uint64_t blockRecordHandle) {
  auto it = m_paperSpaceLayoutLayers.find(blockRecordHandle);
  if (it == m_paperSpaceLayoutLayers.end()) { return nullptr; }
  auto& layers = it->second;
  for (INT_PTR i = 0; i < layers.GetSize(); i++) {
    auto* layer = layers.GetAt(i);
    if (name.CompareNoCase(layer->Name()) == 0) { return layer; }
  }
  return nullptr;
}

const CLayers& AeSysDoc::PaperSpaceLayers() const {
  auto it = m_paperSpaceLayoutLayers.find(m_activeLayoutHandle);
  if (it != m_paperSpaceLayoutLayers.end()) { return it->second; }
  ATLTRACE2(traceGeneral, 1, L"Warning: PaperSpaceLayers() — active layout handle %I64X not found in layout map.\n",
      m_activeLayoutHandle);
  static const CLayers emptyLayers;
  return emptyLayers;
}

void AeSysDoc::LayerBlank(const CString& name) {
  auto* layer = GetLayerTableLayer(name);

  if (layer == nullptr) {
    app.WarningMessageBox(IDS_LAYER_NOT_LOADED);
  } else if (layer->IsResident()) {
    app.WarningMessageBox(IDS_MSG_LAYER_IS_RESIDENT, name);
  } else if (layer->IsOpened()) {
    if (app.ConfirmMessageBox(IDS_MSG_CONFIRM_BLANK, name) == IDYES) {
      RemoveAllTrappedGroups();
      RemoveAllGroupsFromAllViews();
      ResetAllViews();
      m_DeletedGroupList.DeleteGroupsAndRemoveAll();

      SetWorkLayer(GetLayerTableLayerAt(0));
      m_saveAsType = EoDb::FileTypes::Unknown;

      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      RemoveLayerTableLayer(name);
    }
  } else {
    UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
    RemoveLayerTableLayer(name);
  }
}

bool AeSysDoc::LayerMelt(CString& strName) {
  auto* layer = GetLayerTableLayer(strName);
  if (layer == nullptr) { return false; }

  bool bRetVal{};

  const auto filter = App::LoadStringResource(IDS_OPENFILE_FILTER_TRACINGS);

  OPENFILENAME of{};
  of.lStructSize = sizeof(OPENFILENAME);
  of.hInstance = AeSys::GetInstance();
  of.lpstrFilter = filter;
  of.lpstrFile = new wchar_t[MAX_PATH];
  wcscpy_s(of.lpstrFile, MAX_PATH, strName);
  of.nMaxFile = MAX_PATH;
  of.lpstrTitle = L"Melt As";
  of.Flags = OFN_OVERWRITEPROMPT;
  of.lpstrDefExt = L"tra";

  if (GetSaveFileNameW(&of)) {
    strName = of.lpstrFile;

    EoDb::FileTypes fileType = App::FileTypeFromPath(strName);
    if (fileType == EoDb::FileTypes::Tracing || fileType == EoDb::FileTypes::Job) {
      CFile file(strName, CFile::modeWrite | CFile::modeCreate);
      if (file != CFile::hFileNull) {
        if (fileType == EoDb::FileTypes::Job) {
          EoDbJobFile jobFile;
          jobFile.WriteHeader(file);
          jobFile.WriteLayer(file, layer);
        } else {
          EoDbTracingFile tracingFile;
          tracingFile.WriteHeader(file);
          tracingFile.WriteLayer(file, layer);
        }
        layer->ClearStateFlag();
        layer->MakeResident();
        layer->SetStateStatic();
        layer->SetTracingState(static_cast<std::uint16_t>(EoDbLayer::TracingState::isMapped));

        strName = strName.Mid(of.nFileOffset);
        layer->SetName(strName);
        bRetVal = true;
      } else {
        app.WarningMessageBox(IDS_MSG_TRACING_WRITE_FAILURE, strName);
      }
    }
  }
  delete[] of.lpstrFile;
  return bRetVal;
}
void AeSysDoc::RemoveLayerTableLayer(const CString& name) {
  const auto i = FindLayerTableLayer(name);

  if (i >= 0) { RemoveLayerTableLayerAt(i); }
}
void AeSysDoc::PenTranslation(std::uint16_t wCols, std::int16_t* pColNew, std::int16_t* pCol) {
  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    layer->PenTranslation(wCols, pColNew, pCol);
  }
}

EoDbLayer* AeSysDoc::LayersSelUsingPoint(const EoGePoint3d& pt) {
  auto* activeView = AeSysView::GetActiveView();

  auto* group = activeView->SelectGroupAndPrimitive(pt);

  if (group != nullptr) {
    for (auto i = 0; i < GetLayerTableSize(); i++) {
      auto* layer = GetLayerTableLayerAt(i);
      if (layer->Find(group)) { return layer; }
    }
  }
  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);

    if (layer->SelectGroupUsingPoint(pt) != nullptr) { return layer; }
  }
  return nullptr;
}

int AeSysDoc::RemoveEmptyNotesAndDelete() {
  int count{};

  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    count += layer->RemoveEmptyNotesAndDelete();
  }

  // Note: remove empty notes from blocks

  CString key;
  EoDbBlock* block{};

  auto position = m_BlocksTable.GetStartPosition();
  while (position != nullptr) { m_BlocksTable.GetNextAssoc(position, key, block); }
  return count;
}
int AeSysDoc::RemoveEmptyGroups() {
  int count{};

  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    count += layer->RemoveEmptyGroups();
  }

  // Note: remove empty groups from blocks

  CString key;
  EoDbBlock* block{};

  auto position = m_BlocksTable.GetStartPosition();
  while (position != nullptr) { m_BlocksTable.GetNextAssoc(position, key, block); }
  return count;
}

// Work Layer interface

void AeSysDoc::AddWorkLayerGroup(EoDbGroup* group) {
  if (m_workLayer == nullptr) {
    ATLTRACE2(traceGeneral, 1, L"AeSysDoc::AddWorkLayerGroup: m_workLayer is nullptr\n");
    return;
  }
  // In block edit mode, stamp primitives with layer "0" (the standard block content layer)
  const std::wstring layerName = IsInEditor() ? L"0" : std::wstring(m_workLayer->Name().GetString());
  auto position = group->GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = group->GetNext(position);
    if (primitive != nullptr && primitive->LayerName().empty()) { primitive->SetLayerName(layerName); }
  }
  RegisterGroupHandles(group);
  m_workLayer->AddTail(group);
  AddGroupToAllViews(group);
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::WorkCount);
  if (IsEditingTracing()) { m_tracingEditDirty = true; }
  SetModifiedFlag(TRUE);
}

void AeSysDoc::AddWorkLayerGroups(EoDbGroupList* groups) {
  if (m_workLayer == nullptr) {
    ATLTRACE2(traceGeneral, 1, L"AeSysDoc::AddWorkLayerGroups: m_workLayer is nullptr\n");
    return;
  }
  // Stamp primitives with the work layer's name for DXF export and diagnostics
  std::wstring layerName(m_workLayer->Name().GetString());
  auto position = groups->GetHeadPosition();
  while (position != nullptr) {
    auto* group = groups->GetNext(position);
    auto primPos = group->GetHeadPosition();
    while (primPos != nullptr) {
      auto* primitive = group->GetNext(primPos);
      if (primitive != nullptr && primitive->LayerName().empty()) { primitive->SetLayerName(layerName); }
    }
    RegisterGroupHandles(group);
  }
  m_workLayer->AddTail(groups);
  AddGroupsToAllViews(groups);
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::WorkCount);
  SetModifiedFlag(TRUE);
}

EoDbGroup* AeSysDoc::GetLastWorkLayerGroup() const {
  if (m_workLayer == nullptr) { return nullptr; }
  auto position = m_workLayer->GetTailPosition();
  return (position != nullptr) ? static_cast<EoDbGroup*>(m_workLayer->GetPrev(position)) : nullptr;
}

void AeSysDoc::InitializeWorkLayer() {
  if (m_workLayer == nullptr) {
    ATLTRACE2(traceGeneral, 1, L"AeSysDoc::InitializeWorkLayer: m_workLayer is nullptr\n");
    return;
  }
  auto position = m_workLayer->GetHeadPosition();
  while (position != nullptr) { UnregisterGroupHandles(m_workLayer->GetNext(position)); }

  m_workLayer->DeleteGroupsAndRemoveAll();

  RemoveAllTrappedGroups();
  RemoveAllGroupsFromAllViews();
  ResetAllViews();

  position = m_DeletedGroupList.GetHeadPosition();
  while (position != nullptr) { UnregisterGroupHandles(m_DeletedGroupList.GetNext(position)); }

  m_DeletedGroupList.DeleteGroupsAndRemoveAll();
}
EoDbLayer* AeSysDoc::SetWorkLayer(EoDbLayer* layer) {
  if (layer == nullptr) {
    ATLTRACE2(traceGeneral, 1, L"AeSysDoc::SetWorkLayer called with nullptr\n");
    return m_workLayer;
  }
  EoDbLayer* previousWorkLayer = m_workLayer;
  if (m_workLayer != nullptr && m_workLayer != layer) {
    m_workLayer->SetStateActive();  // demote previous work layer
  }
  m_workLayer = layer;
  m_workLayer->SetStateWork();

  // @todo File Name and Work Layer display? (was appended to MenuBar File command)

  return previousWorkLayer;
}

// Locates the layer containing a group and removes it.
// The group itself is not deleted. Searches both model and all paper-space layout tables.
// Also checks the active edit layer (tracing/block) which is not registered in the layer arrays.
EoDbLayer* AeSysDoc::AnyLayerRemove(EoDbGroup* group) {
  // In editor mode the active edit layer is not in the registered layer arrays — check it first.
  if (IsInEditor()) {
    EoDbLayer* editLayer = IsEditingBlock() ? m_blockEditLayer : m_tracingEditLayer;
    if (editLayer != nullptr && editLayer->Remove(group) != nullptr) {
      AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::WorkCount);
      if (IsEditingTracing()) { m_tracingEditDirty = true; }
      SetModifiedFlag(TRUE);
      return editLayer;
    }
  }
  auto searchLayers = [&](CLayers& layers) -> EoDbLayer* {
    for (INT_PTR i = 0; i < layers.GetSize(); i++) {
      auto* layer = layers.GetAt(i);
      if (layer->IsWork() || layer->IsActive()) {
        if (layer->Remove(group) != nullptr) {
          AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::WorkCount);
          SetModifiedFlag(TRUE);
          return layer;
        }
      }
    }
    return nullptr;
  };
  if (auto* found = searchLayers(m_modelSpaceLayers)) { return found; }
  for (auto& [handle, layers] : m_paperSpaceLayoutLayers) {
    if (auto* found = searchLayers(layers)) { return found; }
  }
  return nullptr;
}

void AeSysDoc::MoveTrappedGroupsToSpace(EoDxf::Space targetSpace) {
  if (m_trappedGroups.IsEmpty()) { return; }

  // Collect groups into a local list so we can iterate safely while modifying the trap.
  std::vector<EoDbGroup*> groupsToMove;
  auto position = m_trappedGroups.GetHeadPosition();
  while (position != nullptr) { groupsToMove.push_back(m_trappedGroups.GetNext(position)); }

  for (auto* group : groupsToMove) {
    // Determine which layer name to use from the first primitive in the group
    CString targetLayerName(L"0");
    auto primPosition = group->GetHeadPosition();
    if (primPosition != nullptr) {
      const auto* primitive = group->GetNext(primPosition);
      if (primitive != nullptr && !primitive->LayerName().empty()) {
        targetLayerName = primitive->LayerName().c_str();
      }
    }

    // Remove the group from its current layer (searches both model and paper spaces).
    // AnyLayerRemove only searches work/active layers — use a broader search.
    bool removed = false;
    auto removeFromLayers = [&](CLayers& layers) -> bool {
      for (INT_PTR i = 0; i < layers.GetSize(); i++) {
        auto* layer = layers.GetAt(i);
        if (layer->Remove(group) != nullptr) { return true; }
      }
      return false;
    };
    removed = removeFromLayers(m_modelSpaceLayers);
    if (!removed) {
      for (auto& [handle, layers] : m_paperSpaceLayoutLayers) {
        if (removeFromLayers(layers)) {
          removed = true;
          break;
        }
      }
    }
    if (!removed) { continue; }  // Group not found in any layer — skip

    // Find or create the target layer in the target space
    EoDbLayer* targetLayer = nullptr;
    if (targetSpace == EoDxf::Space::PaperSpace) {
      targetLayer = FindLayerInLayout(targetLayerName, m_activeLayoutHandle);
      if (targetLayer == nullptr) {
        targetLayer = new EoDbLayer(targetLayerName, EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive);
        AddLayerToLayout(targetLayer, m_activeLayoutHandle);
      }
    } else {
      targetLayer = FindLayerInSpace(targetLayerName, EoDxf::Space::ModelSpace);
      if (targetLayer == nullptr) {
        targetLayer = new EoDbLayer(targetLayerName, EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive);
        AddLayerToSpace(targetLayer, EoDxf::Space::ModelSpace);
      }
    }

    targetLayer->AddTail(group);
    m_trappedGroups.Remove(group);
  }

  RemoveAllGroupsFromAllViews();
  ResetAllViews();
  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) { activeView->UpdateStateInformation(AeSysView::WorkCount); }
  SetModifiedFlag(TRUE);
}

void AeSysDoc::TracingFuse(CString& nameAndLocation) {
  auto* layer = GetLayerTableLayer(nameAndLocation);
  if (layer != nullptr) {
    wchar_t title[MAX_PATH]{};
    GetFileTitleW(nameAndLocation, title, MAX_PATH);
    wchar_t* context{};
    wchar_t* baseName = wcstok_s(title, L".", &context);
    nameAndLocation = baseName;
    layer->ClearTracingStateBit();
    layer->ClearStateFlag();
    layer->MakeResident();
    layer->MakeInternal();
    layer->SetStateStatic();

    layer->SetName(baseName);
  }
}
/// Inserts a .tra file as a locked XREF-style tracing layer in the current document. The layer is named `|stem` for
/// `stem.tra`, loaded with the file's primitives, set to locked state, and added to the model-space layer table.
void AeSysDoc::InsertTracingLayer(const std::wstring& absolutePath) {
  const std::filesystem::path filePath(absolutePath);
  const CString layerName = CString(L"|") + filePath.stem().wstring().c_str();

  // Reject duplicate — same tracing already referenced in this document
  if (FindLayerInSpace(layerName, EoDxf::Space::ModelSpace) != nullptr) {
    CString message;
    message.Format(L"Tracing layer '%s' is already inserted.", layerName.GetString());
    app.AddStringToMessageList(message);
    return;
  }

  auto* layer = new EoDbLayer(layerName, EoDbLayer::State::isStatic);
  layer->SetColorIndex(7);
  layer->SetLineType(m_continuousLineType);
  layer->SetLocked(true);
  layer->SetTracingFilePath(absolutePath);
  layer->MakeResident();
  layer->MakeInternal();  // Prevents .jb1 suffix on PEG V1 read; marks as externally referenced

  const CString pathCs(absolutePath.c_str());
  if (!TracingLoadLayer(pathCs, layer)) {
    CString message;
    message.Format(L"Failed to load tracing file: %s", pathCs.GetString());
    app.AddStringToMessageList(message);
    delete layer;
    return;
  }

  AddLayerToSpace(layer, EoDxf::Space::ModelSpace);

  CString message;
  message.Format(L"Inserted tracing layer '%s' from: %s", layerName.GetString(), pathCs.GetString());
  app.AddStringToMessageList(message);

  UpdateAllViews(nullptr);
}

/// Reloads primitives for a tracing layer from its associated .tra file on disk.
/// Existing primitives are cleared first. Logs to the output pane if the file is missing.
void AeSysDoc::ReloadTracingLayer(EoDbLayer* layer) {
  if (layer == nullptr || !layer->IsTracingLayer()) { return; }
  if (layer->TracingFilePath().empty()) { return; }

  const CString pathCs(layer->TracingFilePath().c_str());

  // Clear existing transient primitives
  layer->DeleteGroupsAndRemoveAll();

  if (!TracingLoadLayer(pathCs, layer)) {
    CString message;
    message.Format(L"Tracing file not found (layer '%s'): %s — layer will be empty.",
        layer->Name().GetString(), pathCs.GetString());
    app.AddStringToMessageList(message);
  }
}

bool AeSysDoc::TracingLoadLayer(const CString& pathName, EoDbLayer* layer) {
  const EoDb::FileTypes fileType = App::FileTypeFromPath(pathName);
  if (fileType != EoDb::FileTypes::Tracing && fileType != EoDb::FileTypes::Job) { return false; }
  if (layer == nullptr) { return false; }

  constexpr bool bFileOpen{};

  if (fileType == EoDb::FileTypes::Tracing) {
    CFileException e;
    CFile file(pathName, CFile::modeRead | CFile::shareDenyNone);
    if (file != CFile::hFileNull) {
      EoDbTracingFile tracingFile;
      tracingFile.ReadHeader(file);
      tracingFile.ReadLayer(file, layer);
      return true;
    }
  } else {
    CFile file(pathName, CFile::modeRead | CFile::shareDenyNone);
    if (file != nullptr) {
      EoDbJobFile jobFile;
      jobFile.ReadHeader(file);
      jobFile.ReadLayer(file, layer);
      return true;
    }
    app.WarningMessageBox(IDS_MSG_TRACING_OPEN_FAILURE, pathName);
  }
  return bFileOpen;
}

bool AeSysDoc::TracingMap(const CString& pathName) {
  const auto fileType = App::FileTypeFromPath(pathName);
  if (fileType != EoDb::FileTypes::Tracing && fileType != EoDb::FileTypes::Job) { return false; }
  bool fileOpen{};

  auto* layer = GetLayerTableLayer(pathName);

  if (layer != nullptr) {
    if (layer->IsOpened()) {
      app.WarningMessageBox(IDS_MSG_CLOSE_TRACING_FIRST, pathName);
    } else {
      fileOpen = true;
    }
  } else {
    layer = new EoDbLayer(pathName, EoDbLayer::State::isStatic);

    fileOpen = TracingLoadLayer(pathName, layer);

    if (fileOpen) {
      AddLayerTableLayer(layer);
    } else {
      delete layer;
    }
  }
  if (fileOpen) {
    layer->SetTracingState(static_cast<std::uint16_t>(EoDbLayer::TracingState::isMapped));
    UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);
  }
  return fileOpen;
}

bool AeSysDoc::TracingOpen(const CString& pathName) {
  EoDbLayer* layer{};

  const auto index = FindLayerTableLayer(pathName);

  if (index > 0) {  // already loaded
    layer = GetLayerTableLayerAt(index);
    layer->ClearStateFlag(std::to_underlying(EoDbLayer::State::isResident));
  } else {
    // create a new layer and append all the groups in the group file.

    layer = new EoDbLayer(pathName, EoDbLayer::State::isWork | EoDbLayer::State::isActive);
    AddLayerTableLayer(layer);

    TracingLoadLayer(pathName, layer);
    AddGroupsToAllViews(layer);
  }
  layer->SetTracingState(static_cast<std::uint16_t>(EoDbLayer::TracingState::isOpened));

  // Only adopt Tracing as the document save type when the document has no other
  // committed type (e.g. opened standalone via File > Open). When TracingOpen is
  // called as an overlay command on an existing .peg/.peg11 document, preserve
  // the original type so Ctrl+S / Save All continue to save the host document
  // correctly.
  if (m_saveAsType == EoDb::FileTypes::Unknown || m_saveAsType == EoDb::FileTypes::Tracing ||
      m_saveAsType == EoDb::FileTypes::Job) {
    m_saveAsType = EoDb::FileTypes::Tracing;
  }
  SetWorkLayer(layer);

  UpdateAllViews(nullptr, 0L, nullptr);

  return true;
}

bool AeSysDoc::TracingView(const CString& pathName) {
  const EoDb::FileTypes fileType = App::FileTypeFromPath(pathName);
  if (fileType != EoDb::FileTypes::Tracing && fileType != EoDb::FileTypes::Job) { return false; }
  bool fileOpen{};

  auto* layer = GetLayerTableLayer(pathName);

  if (layer != nullptr) {
    if (layer->IsOpened()) {
      app.WarningMessageBox(IDS_MSG_CLOSE_TRACING_FIRST, pathName);
    } else {
      fileOpen = true;
    }
  } else {
    layer = new EoDbLayer(pathName, EoDbLayer::State::isStatic);
    fileOpen = TracingLoadLayer(pathName, layer);
    if (fileOpen) {
      AddLayerTableLayer(layer);
    } else {
      delete layer;
    }
  }
  if (fileOpen) {
    layer->SetTracingState(static_cast<std::uint16_t>(EoDbLayer::TracingState::isViewed));
    UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);
  }
  return fileOpen;
}
