#include "Stdafx.h"

#include <cstdint>
#include <filesystem>
#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbLayer.h"
#include "EoDbLineType.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDbTracingFile.h"
#include "EoDlgAttributePrompt.h"
#include "EoDlgBlocks.h"
#include "EoGeReferenceSystem.h"
#include "EoGsRenderState.h"
#include "Resource.h"

UINT AFXAPI HashKey(CString& string_) {
  auto string = static_cast<const wchar_t*>(string_);
  UINT hash{};
  while (*string) { hash = (hash << 5) + hash + *string++; }
  return hash;
}

// AeSysDoc

IMPLEMENT_DYNCREATE(AeSysDoc, CDocument)

BEGIN_MESSAGE_MAP(AeSysDoc, CDocument)
ON_COMMAND(ID_BLOCKS_LOAD, OnBlocksLoad)
ON_COMMAND(ID_BLOCKS_REMOVEUNUSED, OnBlocksRemoveUnused)
ON_COMMAND(ID_BLOCKS_UNLOAD, OnBlocksUnload)
ON_COMMAND(ID_TOOLS_EDIT_BLOCK_DEFINITION, OnToolsEditBlockDefinition)
ON_UPDATE_COMMAND_UI(ID_TOOLS_EDIT_BLOCK_DEFINITION, OnUpdateToolsEditBlockDefinition)
ON_COMMAND(ID_CLEAR_ACTIVELAYERS, OnClearActiveLayers)
ON_COMMAND(ID_CLEAR_ALLLAYERS, OnClearAllLayers)
ON_COMMAND(ID_CLEAR_ALLTRACINGS, OnClearAllTracings)
ON_COMMAND(ID_CLEAR_MAPPEDTRACINGS, OnClearMappedTracings)
ON_COMMAND(ID_CLEAR_VIEWEDTRACINGS, OnClearViewedTracings)
ON_COMMAND(ID_CLEAR_WORKINGLAYER, OnClearWorkingLayer)
ON_COMMAND(ID_EDIT_IMAGETOCLIPBOARD, OnEditImageToClipboard)
ON_COMMAND(ID_EDIT_SEGTOWORK, OnEditSegToWork)
ON_COMMAND(ID_EDIT_TRACE, OnEditTrace)
ON_COMMAND(ID_EDIT_TRAPCOPY, OnEditTrapCopy)
ON_COMMAND(ID_EDIT_TRAPCUT, OnEditTrapCut)
ON_COMMAND(ID_EDIT_TRAPDELETE, OnEditTrapDelete)
ON_COMMAND(ID_EDIT_TRAPPASTE, OnEditTrapPaste)
ON_COMMAND(ID_EDIT_TRAPQUIT, OnEditTrapQuit)
ON_COMMAND(ID_EDIT_TRAPWORK, OnEditTrapWork)
ON_COMMAND(ID_EDIT_TRAPWORKANDACTIVE, OnEditTrapWorkAndActive)
ON_COMMAND(ID_FILE, OnFile)
ON_COMMAND(ID_FILE_QUERY, OnFileQuery)
ON_COMMAND(ID_FILE_MANAGE_LAYERS, OnFileManageLayers)
ON_COMMAND(ID_FILE_MANAGE_BLOCKS, OnFileManageBlocks)
ON_COMMAND(ID_FILE_SEND_MAIL, CDocument::OnFileSendMail)
ON_COMMAND(ID_LAYER_ACTIVE, OnLayerActive)
ON_COMMAND(ID_LAYER_STATIC, OnLayerStatic)
ON_COMMAND(ID_LAYER_HIDDEN, OnLayerHidden)
ON_COMMAND(ID_LAYER_MELT, OnLayerMelt)
ON_COMMAND(ID_LAYER_WORK, OnLayerWork)
ON_COMMAND(ID_LAYERS_ACTIVEALL, OnLayersActiveAll)
ON_COMMAND(ID_LAYERS_STATICALL, OnLayersStaticAll)
ON_COMMAND(ID_LAYERS_REMOVEEMPTY, OnLayersRemoveEmpty)
ON_COMMAND(ID_MAINTENANCE_REMOVEEMPTYNOTES, OnMaintenanceRemoveEmptyNotes)
ON_COMMAND(ID_MAINTENANCE_REMOVEEMPTYGROUPS, OnMaintenanceRemoveEmptyGroups)
ON_COMMAND(ID_PENS_REMOVEUNUSEDSTYLES, OnPensRemoveUnusedStyles)
ON_COMMAND(ID_PENS_EDITCOLORS, OnPensEditColors)
ON_COMMAND(ID_PENS_LOADCOLORS, OnPensLoadColors)
ON_COMMAND(ID_PENS_TRANSLATE, OnPensTranslate)
ON_COMMAND(ID_PRIM_BREAK, OnPrimBreak)
ON_COMMAND(ID_PRIM_EXTRACTNUM, OnPrimExtractNum)
ON_COMMAND(ID_PRIM_EXTRACTSTR, OnPrimExtractStr)
ON_COMMAND(ID_TOOLS_PRIMITIVE_SNAPTOENDPOINT, OnToolsPrimitiveSnaptoendpoint)
ON_COMMAND(ID_PRIM_GOTOCENTERPOINT, OnPrimGotoCenterPoint)
ON_COMMAND(ID_TOOLS_PRIMITVE_DELETE, OnToolsPrimitiveDelete)
ON_COMMAND(ID_PRIM_MODIFY_ATTRIBUTES, OnPrimModifyAttributes)
ON_COMMAND(ID_TOOLS_GROUP_BREAK, OnToolsGroupBreak)
ON_COMMAND(ID_TOOLS_GROUP_DELETE, OnToolsGroupDelete)
ON_COMMAND(ID_TOOLS_GROUP_DELETELAST, OnToolsGroupDeletelast)
ON_COMMAND(ID_TOOLS_GROUP_EXCHANGE, OnToolsGroupExchange)
ON_COMMAND(ID_TOOLS_GROUP_UNDELETE, OnToolsGroupUndelete)
ON_COMMAND(ID_SETUP_PENCOLOR, OnSetupPenColor)
ON_COMMAND(ID_SETUP_LINETYPE, OnSetupLineType)
ON_COMMAND(ID_SETUP_FILL_HOLLOW, OnSetupFillHollow)
ON_COMMAND(ID_SETUP_FILL_SOLID, OnSetupFillSolid)
ON_COMMAND(ID_SETUP_FILL_PATTERN, OnSetupFillPattern)
ON_COMMAND(ID_SETUP_FILL_HATCH, OnSetupFillHatch)
ON_COMMAND(ID_SETUP_POINTSTYLE, OnSetupPointStyle)
ON_COMMAND(ID_SETUP_NOTE, OnSetupNote)
ON_COMMAND(ID_SETUP_SAVEPOINT, OnSetupSavePoint)
ON_COMMAND(ID_SETUP_GOTOPOINT, OnSetupGotoPoint)
ON_COMMAND(ID_SETUP_OPTIONS_DRAW, OnSetupOptionsDraw)
ON_COMMAND(ID_TRACING_MAP, OnTracingMap)
ON_COMMAND(ID_TRACING_VIEW, OnTracingView)
ON_COMMAND(ID_TRACING_CLOAK, OnTracingCloak)
ON_COMMAND(ID_TRACING_FUSE, OnTracingFuse)
ON_COMMAND(ID_TRACING_OPEN, OnTracingOpen)
ON_COMMAND(ID_TRAPCOMMANDS_COMPRESS, OnTrapCommandsCompress)
ON_COMMAND(ID_TRAPCOMMANDS_EXPAND, OnTrapCommandsExpand)
ON_COMMAND(ID_TRAPCOMMANDS_INVERT, OnTrapCommandsInvert)
ON_COMMAND(ID_TRAPCOMMANDS_SQUARE, OnTrapCommandsSquare)
ON_COMMAND(ID_TRAPCOMMANDS_QUERY, OnTrapCommandsQuery)
ON_COMMAND(ID_TRAPCOMMANDS_FILTER, OnTrapCommandsFilter)
ON_COMMAND(ID_TRAPCOMMANDS_BLOCK, OnTrapCommandsBlock)
ON_COMMAND(ID_TRAPCOMMANDS_UNBLOCK, OnTrapCommandsUnblock)
ON_COMMAND(ID_HELP_KEY, OnHelpKey)
ON_COMMAND(ID_VIEW_MODELSPACE, OnViewModelSpace)
ON_UPDATE_COMMAND_UI(ID_VIEW_MODELSPACE, OnUpdateViewModelSpace)
END_MESSAGE_MAP()

AeSysDoc::AeSysDoc() {
  EoDbPrimitive::SetHandleManager(&m_handleManager);
}

AeSysDoc::~AeSysDoc() = default;

void AeSysDoc::RegisterHandle(EoDbPrimitive* primitive) {
  const auto handle = primitive->Handle();
  if (handle != 0) { m_handleMap[handle] = HandleObject{primitive}; }
}

void AeSysDoc::RegisterHandle(EoDbLayer* layer) {
  const auto handle = layer->Handle();
  if (handle != 0) { m_handleMap[handle] = HandleObject{layer}; }
}

void AeSysDoc::RegisterHandle(EoDbLineType* lineType) {
  const auto handle = lineType->Handle();
  if (handle != 0) { m_handleMap[handle] = HandleObject{lineType}; }
}

void AeSysDoc::RegisterHandle(EoDbBlock* block) {
  const auto handle = block->Handle();
  if (handle != 0) { m_handleMap[handle] = HandleObject{block}; }
}

void AeSysDoc::UnregisterHandle(std::uint64_t handle) noexcept {
  if (handle != 0) { m_handleMap.erase(handle); }
}

const HandleObject* AeSysDoc::FindObjectByHandle(std::uint64_t handle) const {
  if (handle == 0) { return nullptr; }
  const auto it = m_handleMap.find(handle);
  return (it != m_handleMap.end()) ? &it->second : nullptr;
}

EoDbPrimitive* AeSysDoc::FindPrimitiveByHandle(std::uint64_t handle) const noexcept {
  const auto* object = FindObjectByHandle(handle);
  if (object == nullptr) { return nullptr; }
  const auto* pointer = std::get_if<EoDbPrimitive*>(object);
  return pointer ? *pointer : nullptr;
}

EoDbLayer* AeSysDoc::FindLayerByHandle(std::uint64_t handle) const noexcept {
  const auto* object = FindObjectByHandle(handle);
  if (object == nullptr) { return nullptr; }
  const auto* pointer = std::get_if<EoDbLayer*>(object);
  return pointer ? *pointer : nullptr;
}

EoDbLineType* AeSysDoc::FindLineTypeByHandle(std::uint64_t handle) const noexcept {
  const auto* object = FindObjectByHandle(handle);
  if (object == nullptr) { return nullptr; }
  const auto* pointer = std::get_if<EoDbLineType*>(object);
  return pointer ? *pointer : nullptr;
}

EoDbBlock* AeSysDoc::FindBlockByHandle(std::uint64_t handle) const noexcept {
  const auto* object = FindObjectByHandle(handle);
  if (object == nullptr) { return nullptr; }
  const auto* pointer = std::get_if<EoDbBlock*>(object);
  return pointer ? *pointer : nullptr;
}

void AeSysDoc::RegisterGroupHandles(const EoDbGroup* group) {
  auto position = group->GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = group->GetNext(position);
    RegisterHandle(primitive);
  }
}

void AeSysDoc::UnregisterGroupHandles(const EoDbGroup* group) {
  auto position = group->GetHeadPosition();
  while (position != nullptr) {
    const auto* primitive = group->GetNext(position);
    UnregisterHandle(primitive->Handle());
  }
}

void AeSysDoc::DeleteContents() {
  // Clean up any active editor session without committing
  if (m_editorMode == EditorMode::Block) {
    if (m_blockEditLayer != nullptr) {
      m_blockEditLayer->DeleteGroupsAndRemoveAll();
      delete m_blockEditLayer;
      m_blockEditLayer = nullptr;
    }
    m_editingBlock = nullptr;
    m_editingBlockName.Empty();
    m_blockEditSnapshot.clear();
  } else if (m_editorMode == EditorMode::Tracing) {
    if (m_tracingEditLayer != nullptr) {
      m_tracingEditLayer->DeleteGroupsAndRemoveAll();
      delete m_tracingEditLayer;
      m_tracingEditLayer = nullptr;
    }
    m_embeddedTracingLayer = nullptr;  // Layer is owned by the layer table, deleted below
    m_editingTracingPath.Empty();
    m_savedTracingLayerVisibility.clear();
    // When a standalone tracing session document is closed directly (e.g. MDI tab X),
    // ExitTracingEditMode is never called — restore the layout tab bar TRACING state on
    // whatever view currently has it (the host document's active view).
    if (m_isTracingSession) {
      m_isTracingSession = false;
      auto* hostView = AeSysView::GetActiveView();
      if (hostView != nullptr) {
        hostView->LayoutTabBar().UpdateBlockEditState(false);
        hostView->LayoutTabBar().PopulateFromDocument(hostView->GetDocument());
      }
    }
  }
  m_editorMode = EditorMode::None;
  m_savedEditorWorkLayer = nullptr;
  m_savedEditorTitle.Empty();

  m_handleManager.Reset();
  m_handleMap.clear();
  m_originalDwgPath.clear();
  ATLTRACE2(traceGeneral, 3, L"AeSysDoc<%p>::DeleteContents() - BlockTableSize: %d\n", this, BlockTableSize());

  // @todo Release EoDbDxfInterface resources if any

  m_LineTypeTable.RemoveAll();
  m_appIdTable.clear();
  m_classTable.clear();
  m_dimStyleTable.clear();
  m_textStyleTable.clear();
  m_unsupportedObjects.clear();
  m_layouts.clear();
  m_vportTable.clear();

  RemoveAllBlocks();
  m_workLayer = nullptr;  // Null before layer deletion to prevent dangling access during repaints
  RemoveAllLayerTableLayers();
  DeletedGroupsRemoveGroups();

  RemoveAllTrappedGroups();
  RemoveAllGroupsFromAllViews();

  DeleteNodalResources();

  ResetAllViews();
  CDocument::DeleteContents();
}

void AeSysDoc::AddTextBlock(wchar_t* textBlock) {
  const auto cursorPosition = app.GetCursorPosition();

  const auto& fontDefinition = Gs::renderState.FontDefinition();
  const auto characterCellDefinition = Gs::renderState.CharacterCellDefinition();

  EoGeReferenceSystem referenceSystem(cursorPosition, characterCellDefinition);
  wchar_t* nextToken = nullptr;
  wchar_t* text = wcstok_s(textBlock, L"\r", &nextToken);
  while (text != nullptr) {
    if (wcslen(text) > 0) {
      auto* group = new EoDbGroup(new EoDbText(fontDefinition, referenceSystem, std::wstring(text)));
      AddWorkLayerGroup(group);
      UpdateAllViews(nullptr, EoDb::kGroup, group);
    }
    referenceSystem.SetOrigin(text_GetNewLinePos(fontDefinition, referenceSystem, 1.0, 0));
    text = wcstok_s(nullptr, L"\r", &nextToken);
    if (text == nullptr) { break; }
    text++;
  }
}

void AeSysDoc::DeletedGroupsRestore() {
  if (DeletedGroupsIsEmpty()) { return; }
  auto* group = DeletedGroupsRemoveTail();
  AddWorkLayerGroup(group);
  UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

// Returns a pointer to the currently active document.
AeSysDoc* AeSysDoc::GetDoc() {
  const auto* frame = static_cast<CMDIFrameWndEx*>(AfxGetMainWnd());
  if (frame == nullptr) { return nullptr; }
  auto* child = static_cast<CMDIChildWndEx*>(frame->MDIGetActive());

  return (child == nullptr) ? nullptr : static_cast<AeSysDoc*>(child->GetActiveDocument());
}

bool AeSysDoc::EnterBlockEditMode(const CString& blockName) {
  EoDbBlock* block{};
  if (!LookupBlock(blockName, block)) { return false; }

  m_editingBlock = block;
  m_editingBlockName = blockName;

  // Save current state
  m_savedEditorWorkLayer = m_workLayer;
  m_savedEditorSpace = m_activeSpace;

  // Create a temporary editing layer with deep-copied primitives from the block
  m_blockEditLayer = new EoDbLayer(CString(L"*BlockEdit"), EoDbLayer::State::isWork);
  m_blockEditLayer->SetColorIndex(7);
  m_blockEditLayer->SetLineType(m_continuousLineType);

  // Deep-copy block primitives into the temp layer — one group per primitive,
  // matching the normal layer→group→primitive structure so selection works.
  auto primPosition = block->GetHeadPosition();
  while (primPosition != nullptr) {
    EoDbPrimitive* copy = nullptr;
    block->GetNext(primPosition)->Copy(copy);
    auto* editGroup = new EoDbGroup(copy);
    m_blockEditLayer->AddTail(editGroup);
  }

  // Switch to model space and set the temp layer as work layer
  m_activeSpace = EoDxf::Space::ModelSpace;
  m_editorMode = EditorMode::Block;

  if (m_savedEditorWorkLayer != nullptr) { m_savedEditorWorkLayer->SetStateActive(); }
  m_workLayer = m_blockEditLayer;

  // Update document title to indicate block edit mode
  m_savedEditorTitle = GetTitle();
  CString editTitle;
  editTitle.Format(L"%s [Editing: %s]", m_savedEditorTitle.GetString(), blockName.GetString());
  SetTitle(editTitle);

  CString message;
  message.Format(L"Editing block definition: %s", blockName.GetString());
  app.AddStringToMessageList(message);

  // Zoom to block extents so the user can see the block content
  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) {
    activeView->LayoutTabBar().UpdateBlockEditState(true, blockName);
    activeView->ModelViewInitialize();
    activeView->OnWindowBest();
  }
  UpdateAllViews(nullptr);
  return true;
}

void AeSysDoc::ExitBlockEditMode(bool commit) {
  if (m_editorMode != EditorMode::Block || m_editingBlock == nullptr || m_blockEditLayer == nullptr) { return; }

  if (commit) {
    // Replace the original block's primitives with edited content
    // First unregister old handles
    UnregisterGroupHandles(m_editingBlock);
    m_editingBlock->DeletePrimitivesAndRemoveAll();

    // Move primitives from the edit layer's group(s) into the block
    auto position = m_blockEditLayer->GetHeadPosition();
    while (position != nullptr) {
      auto* group = m_blockEditLayer->GetNext(position);
      auto primPosition = group->GetHeadPosition();
      while (primPosition != nullptr) {
        auto* primitive = group->GetNext(primPosition);
        m_editingBlock->AddTail(primitive);
        RegisterHandle(primitive);
      }
      // Clear the group without deleting primitives (they're now owned by the block)
      group->RemoveAll();
    }

    CString message;
    message.Format(L"Block definition '%s' updated.", m_editingBlockName.GetString());
    app.AddStringToMessageList(message);
  } else {
    // Cancel — discard edits
    auto position = m_blockEditLayer->GetHeadPosition();
    while (position != nullptr) {
      auto* group = m_blockEditLayer->GetNext(position);
      group->DeletePrimitivesAndRemoveAll();
    }
    app.AddStringToMessageList(L"Block edit cancelled.");
  }

  // Clean up the temp layer
  m_blockEditLayer->DeleteGroupsAndRemoveAll();
  delete m_blockEditLayer;
  m_blockEditLayer = nullptr;

  // Restore state
  m_workLayer = m_savedEditorWorkLayer;
  if (m_workLayer != nullptr) { m_workLayer->SetStateWork(); }
  m_activeSpace = m_savedEditorSpace;

  m_editorMode = EditorMode::None;
  m_editingBlock = nullptr;
  m_editingBlockName.Empty();
  m_savedEditorWorkLayer = nullptr;
  m_blockEditSnapshot.clear();

  // Restore document title
  SetTitle(m_savedEditorTitle);
  m_savedEditorTitle.Empty();

  // Refresh cursor and zoom to restored content
  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) {
    activeView->LayoutTabBar().UpdateBlockEditState(false);
    activeView->LayoutTabBar().PopulateFromDocument(this);
    activeView->OnWindowBest();
  }
  UpdateAllViews(nullptr);
}

void AeSysDoc::OnToolsEditBlockDefinition() {
  EoDlgBlocks dlg(this);
  dlg.DoModal();
}

void AeSysDoc::OnToolsSaveAsBlockEdit() {
  if (m_editorMode != EditorMode::Block || m_blockEditLayer == nullptr) { return; }

  // Prompt for a new block name using the attribute prompt dialog
  EoDlgAttributePrompt dlg;
  dlg.m_blockName = m_editingBlockName;
  dlg.m_tagName = L"Block Name";
  dlg.m_promptString = L"Enter new block name:";
  dlg.m_defaultValue = m_editingBlockName;
  if (dlg.DoModal() != IDOK || dlg.m_enteredValue.IsEmpty()) { return; }

  const CString newName = dlg.m_enteredValue;

  // Check if name already exists (and is not the current block)
  EoDbBlock* existing = nullptr;
  if (LookupBlock(newName, existing) && existing != m_editingBlock) {
    CString msg;
    msg.Format(L"Block '%s' already exists. Overwrite?", newName.GetString());
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) { return; }
    // Remove old block content
    existing->DeletePrimitivesAndRemoveAll();
  }

  if (existing == nullptr || existing == m_editingBlock) {
    // Create a new block
    existing = new EoDbBlock;
    m_BlocksTable.SetAt(newName, existing);
  }

  // Copy primitives from the edit layer into the new block
  auto position = m_blockEditLayer->GetHeadPosition();
  while (position != nullptr) {
    const auto* group = m_blockEditLayer->GetNext(position);
    auto primPosition = group->GetHeadPosition();
    while (primPosition != nullptr) {
      auto* primitive = group->GetNext(primPosition);
      EoDbPrimitive* copy = nullptr;
      primitive->Copy(copy);
      existing->AddTail(copy);
      RegisterHandle(copy);
    }
  }

  CString message;
  message.Format(L"Block saved as '%s'.", newName.GetString());
  app.AddStringToMessageList(message);
}

// --- Tracing Editor ---

bool AeSysDoc::EnterEmbeddedTracingEditMode(EoDbLayer* tracingLayer) {
  if (tracingLayer == nullptr || !tracingLayer->IsTracingLayer()) { return false; }
  if (m_editorMode != EditorMode::None) { return false; }

  // Save host document state
  m_savedEditorWorkLayer = m_workLayer;
  m_savedEditorSpace = m_activeSpace;
  m_savedEditorTitle = GetTitle();
  m_embeddedTracingLayer = tracingLayer;
  m_editingTracingPath = CString(tracingLayer->TracingFilePath().c_str());

  m_tracingEditDirty = false;

  // Unlock and activate the tracing layer for editing
  tracingLayer->SetLocked(false);
  tracingLayer->SetStateWork();
  if (m_savedEditorWorkLayer != nullptr) { m_savedEditorWorkLayer->SetStateActive(); }
  m_workLayer = tracingLayer;
  m_activeSpace = EoDxf::Space::ModelSpace;
  m_editorMode = EditorMode::Tracing;

  // Hide all model-space layers except the tracing layer being edited
  m_savedTracingLayerVisibility.clear();
  auto& modelLayers = SpaceLayers(EoDxf::Space::ModelSpace);
  for (int i = 0; i < static_cast<int>(modelLayers.GetSize()); ++i) {
    auto* layer = modelLayers.GetAt(i);
    if (layer == tracingLayer) { continue; }
    const auto savedState = layer->GetState();
    m_savedTracingLayerVisibility.emplace_back(layer, savedState);
    if (!layer->IsOff()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      layer->SetStateOff();
    }
  }

  // Update document title to reflect tracing edit mode
  std::filesystem::path filePath(static_cast<const wchar_t*>(m_editingTracingPath));
  const CString tracingName = filePath.stem().wstring().c_str();
  CString editTitle;
  editTitle.Format(L"[|%s]", tracingName.GetString());
  SetTitle(editTitle);

  CString message;
  message.Format(L"Editing embedded tracing: %s", m_editingTracingPath.GetString());
  app.AddStringToMessageList(message);

  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) {
    activeView->LayoutTabBar().UpdateBlockEditState(true, tracingName, L"TRACING");
    activeView->ModelViewInitialize();
    activeView->OnWindowBest();
  }
  UpdateAllViews(nullptr);
  return true;
}

bool AeSysDoc::EnterTracingEditMode(const CString& pathName) {
  if (m_editorMode != EditorMode::None) { return false; }

  // Save current state
  m_savedEditorWorkLayer = m_workLayer;
  m_savedEditorSpace = m_activeSpace;
  m_editingTracingPath = pathName;

  // Create a temporary editing layer and load the .tra content into it
  m_tracingEditLayer = new EoDbLayer(CString(L"*TracingEdit"), EoDbLayer::State::isWork);
  m_tracingEditLayer->SetColorIndex(7);
  m_tracingEditLayer->SetLineType(m_continuousLineType);

  if (!TracingLoadLayer(pathName, m_tracingEditLayer)) {
    // File doesn't exist yet or failed to load — start with empty layer for new tracing creation
    app.AddStringToMessageList(L"Starting new tracing file.");
  }

  m_tracingEditDirty = false;

  // Switch to model space and set the temp layer as work layer
  m_activeSpace = EoDxf::Space::ModelSpace;
  m_editorMode = EditorMode::Tracing;

  if (m_savedEditorWorkLayer != nullptr) { m_savedEditorWorkLayer->SetStateActive(); }
  m_workLayer = m_tracingEditLayer;

  // Update document title to indicate tracing edit mode
  m_savedEditorTitle = GetTitle();
  const std::filesystem::path filePath(static_cast<const wchar_t*>(pathName));
  const CString tracingName = filePath.stem().wstring().c_str();
  CString editTitle;
  editTitle.Format(L"[|%s]", tracingName.GetString());
  SetTitle(editTitle);

  CString message;
  message.Format(L"Editing tracing: %s", pathName.GetString());
  app.AddStringToMessageList(message);

  // Zoom to tracing extents
  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) {
    activeView->LayoutTabBar().UpdateBlockEditState(true, tracingName, L"TRACING");
    activeView->ModelViewInitialize();
    activeView->OnWindowBest();
  }
  UpdateAllViews(nullptr);
  return true;
}

void AeSysDoc::ExitTracingEditMode(bool commit) {
  if (m_editorMode != EditorMode::Tracing) { return; }
  const bool isEmbedded = (m_embeddedTracingLayer != nullptr);
  if (!isEmbedded && m_tracingEditLayer == nullptr) { return; }

  // For standalone sessions, set closing flag early to suppress repaints during cleanup
  if (m_isTracingSession) { m_isClosing = true; }

  if (isEmbedded) {
    // Embedded mode — re-lock the layer and return it to static state
    if (!commit) {
      // Discard unsaved edits by reloading from disk
      ReloadTracingLayer(m_embeddedTracingLayer);
      app.AddStringToMessageList(L"Embedded tracing edit cancelled — changes discarded.");
    }
    m_embeddedTracingLayer->SetLocked(true);
    m_embeddedTracingLayer->SetStateStatic();
    m_embeddedTracingLayer = nullptr;
  } else {
    // Solo mode: save was already performed by the caller before ExitTracingEditMode;
    // unconditionally discard the temp layer regardless of commit flag.
    if (commit) {
      app.AddStringToMessageList(L"Tracing saved.");
    } else {
      app.AddStringToMessageList(L"Tracing edit cancelled.");
    }

    // Clean up the temp layer
    m_tracingEditLayer->DeleteGroupsAndRemoveAll();
    delete m_tracingEditLayer;
    m_tracingEditLayer = nullptr;
  }

  // Restore state
  m_workLayer = m_savedEditorWorkLayer;
  if (m_workLayer != nullptr) { m_workLayer->SetStateWork(); }
  m_activeSpace = m_savedEditorSpace;

  // Restore visibility of layers that were hidden during tracing edit
  for (auto& [layer, savedState] : m_savedTracingLayerVisibility) {
    if (savedState & static_cast<std::uint16_t>(EoDbLayer::State::isOff)) {
      layer->SetStateOff();
    } else if (savedState & static_cast<std::uint16_t>(EoDbLayer::State::isStatic)) {
      layer->SetStateStatic();
    } else {
      layer->SetStateActive();
    }
  }
  m_savedTracingLayerVisibility.clear();

  m_editorMode = EditorMode::None;
  m_editingTracingPath.Empty();
  m_savedEditorWorkLayer = nullptr;

  // Restore document title
  SetTitle(m_savedEditorTitle);
  m_savedEditorTitle.Empty();

  // If this was a standalone .tra session, close the document immediately
  if (m_isTracingSession) {
    // Reset the layout tab bar before the window closes so it does not show
    // a stale TRACING state if the tab bar is briefly repainted during teardown
    // or inherited by the next activated document's view.
    auto* closingView = AeSysView::GetActiveView();
    if (closingView != nullptr) { closingView->LayoutTabBar().UpdateBlockEditState(false); }
    m_isTracingSession = false;
    m_workLayer = nullptr;  // Prevent dangling access during async close teardown
    SetModifiedFlag(FALSE);  // Prevent "Save changes?" prompt on close
    auto pos = GetFirstViewPosition();
    if (pos != nullptr) {
      const auto* view = GetNextView(pos);
      if (view != nullptr && view->GetParentFrame() != nullptr) { view->GetParentFrame()->PostMessageW(WM_CLOSE); }
    }
    return;
  }

  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) {
    activeView->LayoutTabBar().UpdateBlockEditState(false);
    activeView->LayoutTabBar().PopulateFromDocument(this);
    activeView->OnWindowBest();
  }

  UpdateAllViews(nullptr);
}

bool AeSysDoc::SaveTracingLayerToFile(const CString& filePath, EoDbLayer* layer) {
  if (filePath.IsEmpty() || layer == nullptr) { return false; }

  CFile file;
  CFileException ex;
  if (!file.Open(filePath, CFile::modeCreate | CFile::modeWrite, &ex)) {
    CString msg;
    msg.Format(L"Failed to save tracing file: %s", filePath.GetString());
    app.AddStringToMessageList(msg);
    return false;
  }

  EoDbTracingFile tracingFile;
  tracingFile.WriteHeader(file);
  tracingFile.WriteLayer(file, layer);
  file.Close();
  return true;
}

void AeSysDoc::OnToolsSaveTracingEdit() {
  if (m_editorMode != EditorMode::Tracing) { return; }

  const bool isEmbedded = (m_embeddedTracingLayer != nullptr);
  EoDbLayer* layerToSave = isEmbedded ? m_embeddedTracingLayer : m_tracingEditLayer;
  if (layerToSave == nullptr) { return; }

  // If we somehow have no path yet, treat Save as Save As (common UX)
  if (m_editingTracingPath.IsEmpty()) {
    OnToolsSaveAsTracingEdit();
    return;
  }

  if (SaveTracingLayerToFile(m_editingTracingPath, layerToSave)) {
    m_tracingEditDirty = false;
    CString message;
    message.Format(L"Tracing saved: %s", m_editingTracingPath.GetString());
    app.AddStringToMessageList(message);
  }

  if (isEmbedded) {
    // Embedded mode — save and continue editing; do not exit the editor
    return;
  }

  ExitTracingEditMode(true);  // Solo mode — exit after save
}

void AeSysDoc::OnToolsSaveAsTracingEdit() {
  if (m_editorMode != EditorMode::Tracing) { return; }

  const bool isEmbedded = (m_embeddedTracingLayer != nullptr);
  EoDbLayer* layerToSave = isEmbedded ? m_embeddedTracingLayer : m_tracingEditLayer;
  if (layerToSave == nullptr) { return; }

  CFileDialog saveDialog(FALSE,
      L"tra",
      m_editingTracingPath,
      OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
      L"Tracing Files (*.tra)|*.tra||",
      AfxGetMainWnd());

  if (saveDialog.DoModal() != IDOK) { return; }

  const CString newPath = saveDialog.GetPathName();

  if (SaveTracingLayerToFile(newPath, layerToSave)) {
    m_tracingEditDirty = false;
    CString message;
    message.Format(L"Tracing saved as: %s", newPath.GetString());
    app.AddStringToMessageList(message);

    // Update the path reference — applies to both modes
    m_editingTracingPath = newPath;
    if (isEmbedded && m_embeddedTracingLayer != nullptr) {
      m_embeddedTracingLayer->SetTracingFilePath(newPath.GetString());
    }
  }

  if (isEmbedded) {
    // Embedded SaveAs — continue editing at the new path
    return;
  }

  ExitTracingEditMode(true);  // Solo mode — exit after Save As
}

void AeSysDoc::OnToolsExitTracingEdit() {
  ExitTracingEditMode(true);
}

void AeSysDoc::OnToolsCancelTracingEdit() {
  if (m_tracingEditDirty) {
    const std::filesystem::path filePath(static_cast<const wchar_t*>(m_editingTracingPath));
    const CString tracingName = filePath.stem().wstring().c_str();
    CString prompt;
    prompt.Format(L"Save changes to '%s'?", tracingName.GetString());
    const int result = AfxMessageBox(prompt, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (result == IDCANCEL) { return; }
    if (result == IDYES) {
      OnToolsSaveTracingEdit();
      if (m_tracingEditDirty) { return; }  // save failed — stay in editor
    }
    // IDNO falls through to discard
  }
  ExitTracingEditMode(false);
}

void AeSysDoc::OnUpdateToolsEditBlockDefinition(CCmdUI* cmdUI) {
  cmdUI->Enable(!IsInEditor() && !BlockTableIsEmpty());
}
