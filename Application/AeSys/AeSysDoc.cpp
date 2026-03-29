#include "Stdafx.h"

#include <cstdint>
#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbLayer.h"
#include "EoDbLineType.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoGePoint3d.h"
#include "EoGeReferenceSystem.h"
#include "EoGsRenderState.h"
#include "Resource.h"

UINT AFXAPI HashKey(CString& str) {
  LPCWSTR pStr = (LPCWSTR)str;
  UINT nHash = 0;
  while (*pStr) { nHash = (nHash << 5) + nHash + *pStr++; }
  return nHash;
}

// AeSysDoc

IMPLEMENT_DYNCREATE(AeSysDoc, CDocument)

BEGIN_MESSAGE_MAP(AeSysDoc, CDocument)
ON_COMMAND(ID_BLOCKS_LOAD, OnBlocksLoad)
ON_COMMAND(ID_BLOCKS_REMOVEUNUSED, OnBlocksRemoveUnused)
ON_COMMAND(ID_BLOCKS_UNLOAD, OnBlocksUnload)
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
ON_COMMAND(ID_FILE_MANAGE, OnFileManage)
ON_COMMAND(ID_FILE_SEND_MAIL, CDocument::OnFileSendMail)
ON_COMMAND(ID_FILE_TRACING, OnFileTracing)
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


AeSysDoc::AeSysDoc() { EoDbPrimitive::SetHandleManager(&m_handleManager); }

AeSysDoc::~AeSysDoc() {}

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

const HandleObject* AeSysDoc::FindObjectByHandle(std::uint64_t handle) const noexcept {
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

void AeSysDoc::RegisterGroupHandles(EoDbGroup* group) {
  auto position = group->GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = group->GetNext(position);
    RegisterHandle(primitive);
  }
}

void AeSysDoc::UnregisterGroupHandles(EoDbGroup* group) {
  auto position = group->GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = group->GetNext(position);
    UnregisterHandle(primitive->Handle());
  }
}

void AeSysDoc::DeleteContents() {
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
  m_vportTable.clear();

  RemoveAllBlocks();
  RemoveAllLayerTableLayers();
  m_workLayer = nullptr;
  DeletedGroupsRemoveGroups();

  RemoveAllTrappedGroups();
  RemoveAllGroupsFromAllViews();

  DeleteNodalResources();

  ResetAllViews();
  CDocument::DeleteContents();
}

void AeSysDoc::AddTextBlock(LPWSTR textBlock) {
  auto cursorPosition = app.GetCursorPosition();

  const auto& fontDefinition = renderState.FontDefinition();
  auto characterCellDefinition = renderState.CharacterCellDefinition();

  EoGeReferenceSystem ReferenceSystem(cursorPosition, characterCellDefinition);

  LPWSTR nextToken = nullptr;
  LPWSTR text = wcstok_s(textBlock, L"\r", &nextToken);
  while (text != nullptr) {
    if (wcslen(text) > 0) {
      auto* group = new EoDbGroup(new EoDbText(fontDefinition, ReferenceSystem, std::wstring(text)));
      AddWorkLayerGroup(group);
      UpdateAllViews(nullptr, EoDb::kGroup, group);
    }
    ReferenceSystem.SetOrigin(text_GetNewLinePos(fontDefinition, ReferenceSystem, 1.0, 0));
    text = wcstok_s(0, L"\r", &nextToken);
    if (text == nullptr) { break; }
    text++;
  }
}

void AeSysDoc::DeletedGroupsRestore() {
  if (!DeletedGroupsIsEmpty()) {
    auto* Group = DeletedGroupsRemoveTail();
    AddWorkLayerGroup(Group);
    UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
  }
}

// Returns a pointer to the currently active document.
AeSysDoc* AeSysDoc::GetDoc() {
  auto* frame = static_cast<CMDIFrameWndEx*>(AfxGetMainWnd());
  if (frame == nullptr) { return nullptr; }
  auto* child = static_cast<CMDIChildWndEx*>(frame->MDIGetActive());

  return (child == nullptr) ? nullptr : static_cast<AeSysDoc*>(child->GetActiveDocument());
}
