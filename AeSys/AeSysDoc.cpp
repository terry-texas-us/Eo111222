#include "stdafx.h"

#include <Windows.h>
#include <afx.h>
#include <afxdlgs.h>
#include <afxmdichildwndex.h>
#include <afxmdiframewndex.h>
#include <afxmsg_.h>
#include <afxres.h>
#include <afxstr.h>
#include <afxver_.h>
#include <afxwin.h>
#include <algorithm>
#include <atltrace.h>
#include <atltypes.h>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>
#include <wchar.h>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbBlockFile.h"
#include "EoDbBlockReference.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbDimension.h"
#include "EoDbDrwInterface.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbJobFile.h"
#include "EoDbLayer.h"
#include "EoDbLine.h"
#include "EoDbLineType.h"
#include "EoDbMaskedPrimitive.h"
#include "EoDbPegFile.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDbTracingFile.h"
#include "EoDlgDrawOptions.h"
#include "EoDlgEditTrapCommandsQuery.h"
#include "EoDlgFileManage.h"
#include "EoDlgLineTypesSelection.h"
#include "EoDlgSelectGotoHomePoint.h"
#include "EoDlgSetHomePoint.h"
#include "EoDlgSetPastePosition.h"
#include "EoDlgSetupColor.h"
#include "EoDlgSetupHatch.h"
#include "EoDlgSetupNote.h"
#include "EoDlgTrapFilter.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeTransformMatrix.h"
#include "EoGeUniquePoint.h"
#include "EoGeVector3d.h"
#include "Hatch.h"
#include "Lex.h"
#include "PrimState.h"
#include "Resource.h"
#include "drw_base.h"
#include "libdxfrw.h"

#if defined(USING_ODA)
#include "DbBlockTable.h"
#include "DbBlockTableRecord.h"
#include "DbLayerTable.h"
#include "DbLayerTableRecord.h"
#include "DbLinetypeTable.h"
#include "DbLinetypeTableRecord.h"
#include "ColorMapping.h"
#include "EoDbDwgToPegFile.h"
#endif  // USING_ODA
#if defined(USING_DDE)
#include "ddeGItms.h"
#endif  // USING_DDE
UINT_PTR CALLBACK OFNHookProcFileTracing(HWND, UINT, WPARAM, LPARAM);

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
#if defined(USING_ODA)
ON_COMMAND(ID_SETUP_LAYERPROPERTIES, &AeSysDoc::OnSetupLayerproperties)
#endif  // USING_ODA
END_MESSAGE_MAP()

// AeSysDoc construction/destruction

AeSysDoc::AeSysDoc()
    : m_IdentifiedLayerName(),
      m_SaveAsType(EoDb::kUnknown),
      m_HeaderSection(),
      m_LineTypeTable(),
      m_ContinuousLineType(nullptr),
      m_BlocksTable(),
      m_LayerTable(),
      m_workLayer(nullptr),
      m_DeletedGroupList(),
      m_TrappedGroupList(),
      m_TrapPivotPoint(),
      m_NodalGroupList(),
      m_MaskedPrimitives(),
      m_UniquePoints() {}

AeSysDoc::~AeSysDoc() {}

void AeSysDoc::DeleteContents() {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysDoc<%p>)::DeleteContents()\n", this);

#if defined(USING_ODA)
  m_DatabasePtr.release();
#endif  // USING_ODA

  m_LineTypeTable.RemoveAll();

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
BOOL AeSysDoc::DoSave(LPCWSTR pathName, BOOL replace) {
#if defined(USING_ODA)
  m_SaveAsVersion = m_DatabasePtr->originalFileVersion();
#endif  // USING_ODA
  CString PathName = pathName;
  if (PathName.IsEmpty()) {
    CDocTemplate* Template = GetDocTemplate();
    ASSERT(Template != nullptr);

    PathName = m_strPathName;
    if (replace && PathName.IsEmpty()) {
      PathName = m_strTitle;

      int FirstBadCharacterIndex = PathName.FindOneOf(L" #%;/\\");
      if (FirstBadCharacterIndex != -1) { PathName.ReleaseBuffer(FirstBadCharacterIndex); }
      CString Extension;
      if (Template->GetDocString(Extension, CDocTemplate::filterExt) && !Extension.IsEmpty()) {
        ASSERT(Extension[0] == '.');
        PathName += Extension;
      }
    }
#if defined(USING_ODA)
    if (!DoPromptFileName(PathName, replace ? AFX_IDS_SAVEFILE : AFX_IDS_SAVEFILECOPY,
                          OFN_HIDEREADONLY | OFN_EXPLORER | OFN_PATHMUSTEXIST)) {
      return FALSE;
    }
#endif  // USING_ODA
  }
  if (!OnSaveDocument(PathName)) {
    if (pathName == nullptr) {
      TRY { CFile::Remove(PathName); }
      CATCH_ALL(e) {
        ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Warning: failed to delete file after failed SaveAs.\n");
        do { e->Delete(); } while (0);
      }
      END_CATCH_ALL
    }
    return FALSE;
  }
  if (replace) { SetPathName(PathName); }
  return TRUE;
}

/** @brief Sets up common entries in the line type and layer tables.
 *
 * This method initializes the line type table with standard line types
 * and creates the default working layer. It adds the working layer to the
 * layer table and sets the continuous line type for later use.
 */
void AeSysDoc::SetCommonTableEntries() {
  auto* lineType = new EoDbLineType(0, L"ByBlock", L"ByBlock", 0, nullptr);
  m_LineTypeTable.SetAt(L"ByBlock", lineType);
  lineType = new EoDbLineType(0, L"ByLayer", L"ByLayer", 0, nullptr);
  m_LineTypeTable.SetAt(L"ByLayer", lineType);
  lineType = new EoDbLineType(1, L"Continuous", L"Solid line", 0, nullptr);
  m_LineTypeTable.SetAt(L"Continuous", lineType);

  m_workLayer = new EoDbLayer(L"0", EoDbLayer::kIsResident | EoDbLayer::kIsInternal | EoDbLayer::kIsActive, lineType);
  m_ContinuousLineType = lineType;
  AddLayerTableLayer(m_workLayer);
}

BOOL AeSysDoc::OnNewDocument() {
  if (!CDocument::OnNewDocument()) { return FALSE; }

  SetCommonTableEntries();
  CString applicationPath = EoAppGetPathFromCommandLine();

  // TODO: Load standard line types from the text file rather than hardcoding them
  m_LineTypeTable.LoadLineTypesFromTxtFile(applicationPath + L"\\res\\LineTypes\\LineTypes.txt");
  //m_LineTypeTable.LoadLineTypesFromTxtFile(applicationPath + L"\\res\\LineTypes\\LineTypes-ACAD(scaled to AeSys).txt");
  //m_LineTypeTable.LoadLineTypesFromTxtFile(applicationPath + L"\\res\\LineTypes\\LineTypes-ISO128(scaled to AeSys).txt");

  m_SaveAsType = EoDb::kPeg;
  SetWorkLayer(GetLayerTableLayerAt(0));
  InitializeGroupAndPrimitiveEdit();

  return TRUE;
}
BOOL AeSysDoc::OnOpenDocument(LPCWSTR pathName) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"AeSysDoc<%p>::OnOpenDocument(%s)\n", this, pathName);

  SetCommonTableEntries();

  switch (AeSys::GetFileTypeFromPath(pathName)) {
    case EoDb::kDwg:
      break;
    case EoDb::kDxf:
    case EoDb::kDxb: {
      EoDbDrwInterface dxfInterface(this);
      dxfRW dxfReader(dxfInterface.WStringToString(pathName).data());
      //dxfReader.setDebug(static_cast<DRW::DBG_LEVEL>(1)); // messages to stdout only
      bool success = dxfReader.read(&dxfInterface, true);  // true for verbose output, false for silent
      if (success) {
        ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"DXF file loaded successfully.\n");
      } else {
        ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Error loading DXF file.\n");
      }
      // read dxf/dxb file and save pointer to the database
      // determine the version of the file (from header section) and set m_SaveAsType to EoDb::kDxf or EoDb::kDxb
      // create EoDbDrwInterface object and do conversion
      // set work layer to layer `0`
    } break;
    case EoDb::kPeg:
      try {
        EoDbPegFile file;
        if (file.Open(pathName, CFile::modeRead | CFile::shareDenyNone)) {
          SetCommonTableEntries();
          file.Load(this);
          m_SaveAsType = EoDb::kPeg;
        }
      } catch (const wchar_t* e) {
        app.WarningMessageBox(IDS_MSG_PEGFILE_OPEN_FAILURE, pathName);
        CString errorMessage = CString(pathName) + L" (" + CString(e) + L")";
        break;
      } catch (const std::runtime_error& e) {
        CString errorMessage = CString(pathName) + L" (" + CString(e.what()) + L")";
        app.WarningMessageBox(IDS_MSG_PEGFILE_OPEN_FAILURE, errorMessage);
        break;
      }
      break;
    case EoDb::kTracing:
    case EoDb::kJob:
      TracingOpen(pathName);
      break;

    case EoDb::kUnknown:
      // Let the base class handle it and probably fail
    default:
      return CDocument::OnOpenDocument(pathName);
  }
  return TRUE;
}
BOOL AeSysDoc::OnSaveDocument(LPCWSTR pathName) {
  BOOL ReturnStatus = FALSE;

  switch (m_SaveAsType) {
    case EoDb::kPeg: {
      WriteShadowFile();
      EoDbPegFile File;
      CFileException e;
      if (File.Open(pathName, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive, &e)) {
        File.Unload(this);
        ReturnStatus = TRUE;
      }
      break;
    }
    case EoDb::kTracing:
    case EoDb::kJob: {
      EoDbLayer* Layer = GetLayerTableLayer(pathName);
      if (Layer != 0) {
        CFile File(pathName, CFile::modeCreate | CFile::modeWrite);
        if (File == CFile::hFileNull) {
          app.WarningMessageBox(IDS_MSG_TRACING_WRITE_FAILURE, pathName);
          return FALSE;
        }
        if (m_SaveAsType == EoDb::kJob) {
          EoDbJobFile JobFile;
          JobFile.WriteHeader(File);
          JobFile.WriteLayer(File, Layer);
        } else if (m_SaveAsType == EoDb::kTracing) {
          EoDbTracingFile TracingFile;
          TracingFile.WriteHeader(File);
          TracingFile.WriteLayer(File, Layer);
        }
        app.AddStringToMessageList(IDS_MSG_TRACING_SAVE_SUCCESS, pathName);
        ReturnStatus = TRUE;
      }
      break;
    }
    case EoDb::kDwg:
    case EoDb::kDxf:
    case EoDb::kDxb:
#if defined(USING_ODA)
      m_DatabasePtr->writeFile(lpszPathName, OdDb::kDwg, OdDb::kDHL_CURRENT);
      ReturnStatus = TRUE;
#endif  // USING_ODA
      break;

    case EoDb::kUnknown:
      break;

    default:
      app.WarningMessageBox(IDS_MSG_NOTHING_TO_SAVE);
  }
  return ReturnStatus;
}
// AeSysDoc diagnostics

#ifdef _DEBUG
void AeSysDoc::AssertValid() const { CDocument::AssertValid(); }

void AeSysDoc::Dump(CDumpContext& dc) const { CDocument::Dump(dc); }
#endif  //_DEBUG

void AeSysDoc::AddTextBlock(LPWSTR pszText) {
  EoGePoint3d ptPvt = app.GetCursorPosition();

  EoDbFontDefinition fd;
  pstate.GetFontDef(fd);

  EoDbCharacterCellDefinition ccd;
  pstate.GetCharCellDef(ccd);

  EoGeReferenceSystem ReferenceSystem(ptPvt, ccd);

  LPWSTR NextToken = nullptr;
  LPWSTR pText = wcstok_s(pszText, L"\r", &NextToken);
  while (pText != 0) {
    if (wcslen(pText) > 0) {
      EoDbGroup* Group = new EoDbGroup(new EoDbText(fd, ReferenceSystem, pText));
      AddWorkLayerGroup(Group);
      UpdateAllViews(nullptr, EoDb::kGroup, Group);
    }
    ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 1.0, 0));
    pText = wcstok_s(0, L"\r", &NextToken);
    if (pText == 0) break;
    pText++;
  }
}
void AeSysDoc::DeletedGroupsRestore() {
  if (!DeletedGroupsIsEmpty()) {
    EoDbGroup* Group = DeletedGroupsRemoveTail();
    AddWorkLayerGroup(Group);
    UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
  }
}
void AeSysDoc::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  ptMin(FLT_MAX, FLT_MAX, FLT_MAX);
  ptMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(w);
    if (!Layer->IsOff()) { Layer->GetExtents(view, ptMin, ptMax, tm); }
  }
}
int AeSysDoc::NumberOfGroupsInWorkLayer() {
  INT_PTR iCount = 0;

  for (EoUInt16 i = 0; i < GetLayerTableSize(); i++) {
    EoDbLayer* layer = GetLayerTableLayerAt(i);
    if (layer->IsWork()) { iCount += layer->GetCount(); }
  }
  return static_cast<int>(iCount);
}
int AeSysDoc::NumberOfGroupsInActiveLayers() {
  INT_PTR count = 0;

  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(w);
    if (Layer->IsActive()) { count += Layer->GetCount(); }
  }
  return static_cast<int>(count);
}
void AeSysDoc::DisplayAllLayers(AeSysView* view, CDC* deviceContext) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysDoc<%p>::DisplayAllLayers(%p, %p)\n", this, view, deviceContext);

  try {
    bool IdentifyTrap = app.IsTrapHighlighted() && !IsTrapEmpty();

    RemoveAllGroupsFromAllViews();

    COLORREF BackgroundColor = deviceContext->GetBkColor();
    deviceContext->SetBkColor(ViewBackgroundColor);

    EoDbPolygon::SetSpecialPolygonStyle((EoInt16)(view->RenderAsWireframe() ? EoDb::kHollow : -1));
    int PrimitiveState = pstate.Save();

    for (int i = 0; i < GetLayerTableSize(); i++) {
      EoDbLayer* Layer = GetLayerTableLayerAt(i);
      Layer->Display(view, deviceContext, IdentifyTrap);
    }
    pstate.Restore(deviceContext, PrimitiveState);
    EoDbPolygon::SetSpecialPolygonStyle(-1);

    deviceContext->SetBkColor(BackgroundColor);
  } catch (CException* e) { e->Delete(); }
}
EoDbLayer* AeSysDoc::GetLayerTableLayer(const CString& strName) {
  int i = FindLayerTableLayer(strName);
  return (i < 0 ? (EoDbLayer*)0 : m_LayerTable.GetAt(i));
}
EoDbLayer* AeSysDoc::GetLayerTableLayerAt(int index) {
  return (index >= (int)m_LayerTable.GetSize() ? (EoDbLayer*)nullptr : m_LayerTable.GetAt(index));
}
int AeSysDoc::FindLayerTableLayer(const CString& layerName) const {
  for (EoUInt16 w = 0; w < m_LayerTable.GetSize(); w++) {
    EoDbLayer* Layer = m_LayerTable.GetAt(w);
    if (layerName.CompareNoCase(Layer->Name()) == 0) { return (w); }
  }
  return (-1);
}
void AeSysDoc::RemoveAllLayerTableLayers() {
  for (EoUInt16 LayerTableIndex = 0; LayerTableIndex < m_LayerTable.GetSize(); LayerTableIndex++) {
    EoDbLayer* Layer = m_LayerTable.GetAt(LayerTableIndex);
    if (Layer) {
      Layer->DeleteGroupsAndRemoveAll();
      delete Layer;
    }
  }
  m_LayerTable.RemoveAll();
}
void AeSysDoc::RemoveLayerTableLayerAt(int i) {
  EoDbLayer* Layer = GetLayerTableLayerAt(i);

  Layer->DeleteGroupsAndRemoveAll();
  delete Layer;

  m_LayerTable.RemoveAt(i);
}
void AeSysDoc::RemoveEmptyLayers() {
  for (int LayerTableIndex = GetLayerTableSize() - 1; LayerTableIndex > 0; LayerTableIndex--) {
    EoDbLayer* Layer = GetLayerTableLayerAt(LayerTableIndex);

    if (Layer && Layer->IsEmpty()) {
      Layer->DeleteGroupsAndRemoveAll();
      delete Layer;
      m_LayerTable.RemoveAt(LayerTableIndex);
    }
  }
}

void AeSysDoc::LayerBlank(const CString& name) {
  EoDbLayer* Layer = GetLayerTableLayer(name);

  if (Layer == 0) {
    app.WarningMessageBox(IDS_LAYER_NOT_LOADED);
  } else if (Layer->IsResident()) {
    app.WarningMessageBox(IDS_MSG_LAYER_IS_RESIDENT, name);
  } else if (Layer->IsOpened()) {
    if (app.ConfirmMessageBox(IDS_MSG_CONFIRM_BLANK, name) == IDYES) {
      RemoveAllTrappedGroups();
      RemoveAllGroupsFromAllViews();
      ResetAllViews();
      m_DeletedGroupList.DeleteGroupsAndRemoveAll();

      SetWorkLayer(GetLayerTableLayerAt(0));
      m_SaveAsType = EoDb::kUnknown;

      UpdateAllViews(nullptr, EoDb::kLayerErase, Layer);
      RemoveLayerTableLayer(name);
    }
  } else {
    UpdateAllViews(nullptr, EoDb::kLayerErase, Layer);
    RemoveLayerTableLayer(name);
  }
}
bool AeSysDoc::LayerMelt(CString& strName) {
  EoDbLayer* Layer = GetLayerTableLayer(strName);
  if (Layer == 0) return false;

  bool bRetVal = false;

  CString Filter = EoAppLoadStringResource(IDS_OPENFILE_FILTER_TRACINGS);

  OPENFILENAME of;

  ::ZeroMemory(&of, sizeof(OPENFILENAME));
  of.lStructSize = sizeof(OPENFILENAME);
  of.hwndOwner = 0;
  of.hInstance = app.GetInstance();
  of.lpstrFilter = Filter;
  of.lpstrFile = new WCHAR[MAX_PATH];
  wcscpy_s(of.lpstrFile, MAX_PATH, strName);
  of.nMaxFile = MAX_PATH;
  of.lpstrTitle = L"Melt As";
  of.Flags = OFN_OVERWRITEPROMPT;
  of.lpstrDefExt = L"tra";

  if (GetSaveFileNameW(&of)) {
    strName = of.lpstrFile;

    EoDb::FileTypes FileType = AeSys::GetFileTypeFromPath(strName);
    if (FileType == EoDb::kTracing || FileType == EoDb::kJob) {
      CFile File(strName, CFile::modeWrite | CFile::modeCreate);
      if (File != CFile::hFileNull) {
        if (FileType == EoDb::kJob) {
          EoDbJobFile JobFile;
          JobFile.WriteHeader(File);
          JobFile.WriteLayer(File, Layer);
        } else {
          EoDbTracingFile TracingFile;
          TracingFile.WriteHeader(File);
          TracingFile.WriteLayer(File, Layer);
        }
        Layer->ClearStateFlag();
        Layer->MakeResident();
        Layer->SetStateStatic();
        Layer->SetTracingFlg(EoDbLayer::kTracingIsMapped);

        strName = strName.Mid(of.nFileOffset);
        Layer->SetName(strName);
        bRetVal = true;
      } else {
        app.WarningMessageBox(IDS_MSG_TRACING_WRITE_FAILURE, strName);
      }
    }
  }
  delete[] of.lpstrFile;
  return (bRetVal);
}
void AeSysDoc::RemoveLayerTableLayer(const CString& strName) {
  int i = FindLayerTableLayer(strName);

  if (i >= 0) RemoveLayerTableLayerAt(i);
}
void AeSysDoc::PenTranslation(EoUInt16 wCols, EoInt16* pColNew, EoInt16* pCol) {
  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(w);
    Layer->PenTranslation(wCols, pColNew, pCol);
  }
}
EoDbLayer* AeSysDoc::LayersSelUsingPoint(const EoGePoint3d& pt) {
  auto* activeView = AeSysView::GetActiveView();

  EoDbGroup* Group = activeView->SelectGroupAndPrimitive(pt);

  if (Group != 0) {
    for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
      EoDbLayer* Layer = GetLayerTableLayerAt(w);
      if (Layer->Find(Group)) { return (Layer); }
    }
  }
  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(w);

    if (Layer->SelectGroupUsingPoint(pt) != 0) { return (Layer); }
  }
  return 0;
}
int AeSysDoc::RemoveEmptyNotesAndDelete() {
  int iCount = 0;

  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(w);
    iCount += Layer->RemoveEmptyNotesAndDelete();
  }

  //Note: remove empty notes from blocks

  CString Key;
  EoDbBlock* Block;

  auto Position = m_BlocksTable.GetStartPosition();
  while (Position != nullptr) { m_BlocksTable.GetNextAssoc(Position, Key, Block); }
  return (iCount);
}
int AeSysDoc::RemoveEmptyGroups() {
  int iCount = 0;

  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(w);
    iCount += Layer->RemoveEmptyGroups();
  }

  //Note: remove empty groups from blocks

  CString Key;
  EoDbBlock* Block;

  auto Position = m_BlocksTable.GetStartPosition();
  while (Position != nullptr) { m_BlocksTable.GetNextAssoc(Position, Key, Block); }
  return (iCount);
}
// Work Layer interface
void AeSysDoc::AddWorkLayerGroup(EoDbGroup* group) {
  m_workLayer->AddTail(group);
  AddGroupToAllViews(group);
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::WorkCount);
  SetModifiedFlag(TRUE);
}
void AeSysDoc::AddWorkLayerGroups(EoDbGroupList* groups) {
  m_workLayer->AddTail(groups);
  AddGroupsToAllViews(groups);
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::WorkCount);
  SetModifiedFlag(TRUE);
}
/** @brief Retrieves the last group in the work layer.
  @return Pointer to the last EoDbGroup in the work layer, or nullptr if the work layer is not defined or contains no groups.
*/
EoDbGroup* AeSysDoc::GetLastWorkLayerGroup() const {
  if (m_workLayer == nullptr) { return nullptr; }
  auto position = m_workLayer->GetTailPosition();
  return (position != nullptr) ? static_cast<EoDbGroup*>(m_workLayer->GetPrev(position)) : nullptr;
}
void AeSysDoc::InitializeWorkLayer() {
  m_workLayer->DeleteGroupsAndRemoveAll();

  RemoveAllTrappedGroups();
  RemoveAllGroupsFromAllViews();
  ResetAllViews();
  m_DeletedGroupList.DeleteGroupsAndRemoveAll();
}
EoDbLayer* AeSysDoc::SetWorkLayer(EoDbLayer* layer) {
  EoDbLayer* previousWorkLayer = m_workLayer;
  m_workLayer = layer;
  m_workLayer->SetStateWork();

  // TODO: File Name and Work Layer display? (was appended to MenuBar File command)

  return previousWorkLayer;
}

// Locates the layer containing a group and removes it.
// The group itself is not deleted.
EoDbLayer* AeSysDoc::AnyLayerRemove(EoDbGroup* group) {
  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(w);
    if (Layer->IsWork() || Layer->IsActive()) {
      if (Layer->Remove(group) != 0) {
        AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::WorkCount);
        SetModifiedFlag(TRUE);

        return Layer;
      }
    }
  }
  return 0;
}

void AeSysDoc::TracingFuse(CString& nameAndLocation) {
  auto* layer = GetLayerTableLayer(nameAndLocation);
  if (layer != nullptr) {
    wchar_t title[MAX_PATH]{0};
    GetFileTitleW(nameAndLocation, title, MAX_PATH);
    wchar_t* context{nullptr};
    wchar_t* baseName = wcstok_s(title, L".", &context);
    nameAndLocation = baseName;
    layer->ClrTracingFlg();
    layer->ClearStateFlag();
    layer->MakeResident();
    layer->MakeInternal();
    layer->SetStateStatic();

    layer->SetName(baseName);
  }
}
bool AeSysDoc::TracingLoadLayer(const CString& pathName, EoDbLayer* layer) {
  EoDb::FileTypes FileType = AeSys::GetFileTypeFromPath(pathName);
  if (FileType != EoDb::kTracing && FileType != EoDb::kJob) { return false; }
  if (layer == 0) return false;

  bool bFileOpen = false;

  if (FileType == EoDb::kTracing) {
    CFileException e;
    CFile File(pathName, CFile::modeRead | CFile::shareDenyNone);
    if (File != CFile::hFileNull) {
      EoDbTracingFile TracingFile;
      TracingFile.ReadHeader(File);
      TracingFile.ReadLayer(File, layer);
      return true;
    }
  } else {
    CFile File(pathName, CFile::modeRead | CFile::shareDenyNone);
    if (File != 0) {
      EoDbJobFile JobFile;
      JobFile.ReadHeader(File);
      JobFile.ReadLayer(File, layer);
      return true;
    }
    app.WarningMessageBox(IDS_MSG_TRACING_OPEN_FAILURE, pathName);
  }
  return (bFileOpen);
}
bool AeSysDoc::TracingMap(const CString& pathName) {
  EoDb::FileTypes FileType = AeSys::GetFileTypeFromPath(pathName);
  if (FileType != EoDb::kTracing && FileType != EoDb::kJob) { return false; }
  bool bFileOpen = false;

  EoDbLayer* Layer = GetLayerTableLayer(pathName);

  if (Layer != 0) {
    if (Layer->IsOpened())
      app.WarningMessageBox(IDS_MSG_CLOSE_TRACING_FIRST, pathName);
    else
      bFileOpen = true;
  } else {
    Layer = new EoDbLayer(pathName, EoDbLayer::kIsStatic);

    bFileOpen = TracingLoadLayer(pathName, Layer);

    if (bFileOpen)
      AddLayerTableLayer(Layer);
    else
      delete Layer;
  }
  if (bFileOpen) {
    Layer->SetTracingFlg(EoDbLayer::kTracingIsMapped);
    UpdateAllViews(nullptr, EoDb::kLayerSafe, Layer);
  }
  return (bFileOpen);
}
bool AeSysDoc::TracingOpen(const CString& fileName) {
  // Opens tracing file.

  EoDbLayer* Layer = 0;

  int iLayId = FindLayerTableLayer(fileName);

  if (iLayId > 0) {  // already loaded
    Layer = GetLayerTableLayerAt(iLayId);
    Layer->ClearStateFlag(EoDbLayer::kIsResident);
  } else {  // create a new layer and append all the groups in the group file.

    Layer = new EoDbLayer(fileName, EoDbLayer::kIsWork | EoDbLayer::kIsActive);
    AddLayerTableLayer(Layer);

    TracingLoadLayer(fileName, Layer);

    AddGroupsToAllViews(Layer);
  }
  Layer->SetTracingFlg(EoDbLayer::kTracingIsOpened);

  m_SaveAsType = EoDb::kTracing;
  SetWorkLayer(Layer);

  UpdateAllViews(nullptr, 0L, nullptr);

  return true;
}
bool AeSysDoc::TracingView(const CString& pathName) {
  EoDb::FileTypes FileType = AeSys::GetFileTypeFromPath(pathName);
  if (FileType != EoDb::kTracing && FileType != EoDb::kJob) { return false; }
  bool bFileOpen = false;

  EoDbLayer* Layer = GetLayerTableLayer(pathName);

  if (Layer != 0) {
    if (Layer->IsOpened())
      app.WarningMessageBox(IDS_MSG_CLOSE_TRACING_FIRST, pathName);
    else
      bFileOpen = true;
  } else {
    Layer = new EoDbLayer(pathName, EoDbLayer::kIsStatic);

    bFileOpen = TracingLoadLayer(pathName, Layer);

    if (bFileOpen)
      AddLayerTableLayer(Layer);
    else
      delete Layer;
  }
  if (bFileOpen) {
    Layer->SetTracingFlg(EoDbLayer::kTracingIsViewed);
    UpdateAllViews(nullptr, EoDb::kLayerSafe, Layer);
  }
  return (bFileOpen);
}
void AeSysDoc::WriteShadowFile() {
  if (m_SaveAsType == EoDb::kPeg) {
    CString ShadowFilePath(app.ShadowFolderPath());
    ShadowFilePath += GetTitle();
    int nExt = ShadowFilePath.Find('.');
    if (nExt > 0) {
      CFileStatus fs;
      CFile::GetStatus(GetPathName(), fs);

      ShadowFilePath.Truncate(nExt);
      ShadowFilePath += fs.m_mtime.Format(L"_%Y%m%d%H%M");
      ShadowFilePath += L".peg";

      CFileException e;
      EoDbPegFile fp;
      if (!fp.Open(ShadowFilePath, CFile::modeWrite, &e)) {
        fp.Open(ShadowFilePath, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive, &e);
        fp.Unload(this);
        app.WarningMessageBox(IDS_MSG_FILE_SHADOWED_AS, ShadowFilePath);
        return;
      }
      app.WarningMessageBox(IDS_MSG_SHADOW_FILE_CREATE_FAILURE);
    }
  }
}
// AeSysDoc commands

void AeSysDoc::OnClearActiveLayers() {
  InitializeGroupAndPrimitiveEdit();
  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    EoDbLayer* Layer = GetLayerTableLayerAt(i);

    if (Layer->IsActive()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, Layer);
      Layer->DeleteGroupsAndRemoveAll();
    }
  }
}
void AeSysDoc::OnClearAllLayers() {
  InitializeGroupAndPrimitiveEdit();

  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    EoDbLayer* Layer = GetLayerTableLayerAt(i);

    if (Layer->IsInternal()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, Layer);
      Layer->DeleteGroupsAndRemoveAll();
    }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}
void AeSysDoc::OnClearWorkingLayer() {
  InitializeGroupAndPrimitiveEdit();
  InitializeWorkLayer();
}
void AeSysDoc::OnClearAllTracings() {
  InitializeGroupAndPrimitiveEdit();

  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    EoDbLayer* Layer = GetLayerTableLayerAt(i);

    if (!Layer->IsInternal()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, Layer);
      Layer->DeleteGroupsAndRemoveAll();
    }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}
void AeSysDoc::OnClearMappedTracings() {
  InitializeGroupAndPrimitiveEdit();
  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    EoDbLayer* Layer = GetLayerTableLayerAt(i);

    if (Layer->IsMapped()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, Layer);

      if (Layer->IsResident()) {
        Layer->ClrTracingFlg(EoDbLayer::kTracingIsMapped);
        Layer->SetStateOff();
      } else
        RemoveLayerTableLayerAt(i);
    }
  }
}
void AeSysDoc::OnClearViewedTracings() {
  InitializeGroupAndPrimitiveEdit();
  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    EoDbLayer* Layer = GetLayerTableLayerAt(i);

    if (Layer->IsViewed()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, Layer);

      if (Layer->IsResident()) {
        Layer->ClrTracingFlg(EoDbLayer::kTracingIsViewed);
        Layer->SetStateOff();
      } else
        RemoveLayerTableLayerAt(i);
    }
  }
}
void AeSysDoc::OnPrimBreak() {
  auto* activeView = AeSysView::GetActiveView();

  EoDbGroup* Group = activeView->SelectGroupAndPrimitive(activeView->GetCursorPosition());
  if (Group != 0 && activeView->EngagedPrimitive() != 0) {
    EoDbPrimitive* Primitive = activeView->EngagedPrimitive();

    EoInt16 nPenColor = Primitive->PenColor();
    EoInt16 LineType = Primitive->LineType();

    if (Primitive->Is(EoDb::kPolylinePrimitive)) {
      Group->FindAndRemovePrim(Primitive);

      EoDbPolyline* pPolyline = static_cast<EoDbPolyline*>(Primitive);

      EoGePoint3dArray Points;
      pPolyline->GetAllPts(Points);

      for (EoUInt16 w = 0; w < Points.GetSize() - 1; w++) Group->AddTail(new EoDbLine(nPenColor, LineType, Points[w], Points[w + 1]));

      if (pPolyline->IsLooped()) Group->AddTail(new EoDbLine(nPenColor, LineType, Points[Points.GetUpperBound()], Points[0]));

      delete Primitive;
      ResetAllViews();
    } else if (Primitive->Is(EoDb::kGroupReferencePrimitive)) {
      EoDbBlockReference* pSegRef = static_cast<EoDbBlockReference*>(Primitive);

      EoDbBlock* Block;

      if (LookupBlock(pSegRef->GetName(), Block) != 0) {
        Group->FindAndRemovePrim(Primitive);

        EoGePoint3d ptBase = Block->GetBasePt();

        EoGeTransformMatrix tm = pSegRef->BuildTransformMatrix(ptBase);

        EoDbGroup* pSegT = new EoDbGroup(*Block);
        pSegT->Transform(tm);
        Group->AddTail(pSegT);

        delete Primitive;
        ResetAllViews();
      }
    }
  }
}
void AeSysDoc::OnEditSegToWork() {
  EoGePoint3d pt = app.GetCursorPosition();

  EoDbLayer* Layer = LayersSelUsingPoint(pt);

  if (Layer != 0) {
    if (Layer->IsInternal()) {
      EoDbGroup* Group = Layer->SelectGroupUsingPoint(pt);

      if (Group != 0) {
        Layer->Remove(Group);
        UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);
        AddWorkLayerGroup(Group);
        UpdateAllViews(nullptr, EoDb::kGroup, Group);
      }
    }
  }
}
void AeSysDoc::OnFileQuery() {
  EoGePoint3d pt = app.GetCursorPosition();

  EoDbLayer* Layer = LayersSelUsingPoint(pt);

  if (Layer != 0) {
    CPoint CurrentPosition;
    ::GetCursorPos(&CurrentPosition);

    m_IdentifiedLayerName = Layer->Name();

    int MenuResource = (Layer->IsInternal()) ? IDR_LAYER : IDR_TRACING;

    HMENU LayerTracingMenu = ::LoadMenu(app.GetInstance(), MAKEINTRESOURCE(MenuResource));
    CMenu* SubMenu = CMenu::FromHandle(::GetSubMenu(LayerTracingMenu, 0));

    SubMenu->ModifyMenu(0, MF_BYPOSITION | MF_STRING, 0, m_IdentifiedLayerName);

    if (MenuResource == IDR_LAYER) {
      SubMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_WORK),
                             static_cast<UINT>(MF_BYCOMMAND | (Layer->IsWork() ? MF_CHECKED : MF_UNCHECKED)));
      SubMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_ACTIVE),
                             static_cast<UINT>(MF_BYCOMMAND | (Layer->IsActive() ? MF_CHECKED : MF_UNCHECKED)));
      SubMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_STATIC),
                             static_cast<UINT>(MF_BYCOMMAND | (Layer->IsStatic() ? MF_CHECKED : MF_UNCHECKED)));
      SubMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_HIDDEN),
                             static_cast<UINT>(MF_BYCOMMAND | (Layer->IsOff() ? MF_CHECKED : MF_UNCHECKED)));
    } else {
      SubMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_OPEN),
                             static_cast<UINT>(MF_BYCOMMAND | (Layer->IsOpened() ? MF_CHECKED : MF_UNCHECKED)));
      SubMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_MAP),
                             static_cast<UINT>(MF_BYCOMMAND | (Layer->IsMapped() ? MF_CHECKED : MF_UNCHECKED)));
      SubMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_VIEW),
                             static_cast<UINT>(MF_BYCOMMAND | (Layer->IsViewed() ? MF_CHECKED : MF_UNCHECKED)));
      SubMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_CLOAK),
                             static_cast<UINT>(MF_BYCOMMAND | (Layer->IsOff() ? MF_CHECKED : MF_UNCHECKED)));
    }
    SubMenu->TrackPopupMenuEx(0, CurrentPosition.x, CurrentPosition.y, AfxGetMainWnd(), 0);
    ::DestroyMenu(LayerTracingMenu);
  }
}
void AeSysDoc::OnLayerActive() {
  EoDbLayer* Layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (Layer == 0) {
  } else {
    if (Layer->IsWork()) {
      app.WarningMessageBox(IDS_MSG_LAYER_NO_ACTIVE, m_IdentifiedLayerName);
    } else {
      Layer->MakeStateActive();
      UpdateAllViews(nullptr, EoDb::kLayerSafe, Layer);
    }
  }
}
void AeSysDoc::OnLayerStatic() {
  EoDbLayer* Layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (Layer != 0) {
    if (Layer->IsWork()) {
      app.WarningMessageBox(IDS_MSG_LAYER_NO_STATIC, m_IdentifiedLayerName);
    } else {
      Layer->SetStateStatic();
      UpdateAllViews(nullptr, EoDb::kLayerSafe, Layer);
    }
  }
}
void AeSysDoc::OnLayerHidden() {
  EoDbLayer* Layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (Layer != 0) {
    if (Layer->IsWork()) {
      app.WarningMessageBox(IDS_MSG_LAYER_NO_HIDDEN, m_IdentifiedLayerName);
    } else {
      UpdateAllViews(nullptr, EoDb::kLayerErase, Layer);
      Layer->SetStateOff();
    }
  }
}
void AeSysDoc::OnLayerMelt() { LayerMelt(m_IdentifiedLayerName); }
void AeSysDoc::OnLayerWork() {
  EoDbLayer* Layer = GetLayerTableLayer(m_IdentifiedLayerName);

  SetWorkLayer(Layer);
}
void AeSysDoc::OnTracingMap() { TracingMap(m_IdentifiedLayerName); }
void AeSysDoc::OnTracingView() { TracingView(m_IdentifiedLayerName); }
void AeSysDoc::OnTracingCloak() {
  EoDbLayer* Layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (Layer->IsOpened()) {
    CFile File(m_IdentifiedLayerName, CFile::modeWrite | CFile::modeCreate);
    if (File != CFile::hFileNull) {
      EoDbJobFile JobFile;
      JobFile.WriteHeader(File);
      JobFile.WriteLayer(File, Layer);
      SetWorkLayer(GetLayerTableLayerAt(0));
      m_SaveAsType = EoDb::kUnknown;
      UpdateAllViews(nullptr, EoDb::kLayerErase, Layer);
      Layer->SetStateOff();
    } else {
      app.WarningMessageBox(IDS_MSG_TRACING_WRITE_FAILURE, m_IdentifiedLayerName);
    }
  }
}
void AeSysDoc::OnTracingFuse() { TracingFuse(m_IdentifiedLayerName); }
void AeSysDoc::OnTracingOpen() { TracingOpen(m_IdentifiedLayerName); }
void AeSysDoc::OnLayersActiveAll() {
  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(w);
    if (!Layer->IsWork()) { Layer->MakeStateActive(); }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}
void AeSysDoc::OnLayersStaticAll() {
  for (EoUInt16 w = 0; w < GetLayerTableSize(); w++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(w);
    if (!Layer->IsWork()) { Layer->SetStateStatic(); }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}
void AeSysDoc::OnLayersRemoveEmpty() { RemoveEmptyLayers(); }
void AeSysDoc::OnToolsGroupUndelete() { DeletedGroupsRestore(); }
void AeSysDoc::OnPensRemoveUnusedStyles() { m_LineTypeTable.RemoveUnused(); }
void AeSysDoc::OnBlocksLoad() {
  CFileDialog dlg(TRUE, L"blk", L"*.blk");
  dlg.m_ofn.lpstrTitle = L"Load Blocks";

  if (dlg.DoModal() == IDOK) {
    if ((dlg.m_ofn.Flags & OFN_EXTENSIONDIFFERENT) == 0) {
      EoDbBlockFile fb;
      fb.ReadFile(dlg.GetPathName(), m_BlocksTable);
    } else
      app.WarningMessageBox(IDS_MSG_FILE_TYPE_ERROR);
  }
}
void AeSysDoc::OnBlocksRemoveUnused() { RemoveUnusedBlocks(); }
void AeSysDoc::OnBlocksUnload() {
  CFileDialog dlg(FALSE, L"blk", L"*.blk");
  dlg.m_ofn.lpstrTitle = L"Unload Blocks As";

  if (dlg.DoModal() == IDOK) {
    if ((dlg.m_ofn.Flags & OFN_EXTENSIONDIFFERENT) == 0) {
      EoDbBlockFile fb;
      fb.WriteFile(dlg.GetPathName(), m_BlocksTable);
    }
  }
}
void AeSysDoc::OnEditImageToClipboard() {
  auto* activeView = AeSysView::GetActiveView();

  HDC hdcEMF = ::CreateEnhMetaFile(0, 0, 0, 0);
  DisplayAllLayers(activeView, CDC::FromHandle(hdcEMF));
  HENHMETAFILE hemf = ::CloseEnhMetaFile(hdcEMF);

  ::OpenClipboard(nullptr);
  ::EmptyClipboard();
  ::SetClipboardData(CF_ENHMETAFILE, hemf);
  ::CloseClipboard();
}
void AeSysDoc::OnEditTrace() {
  if (::OpenClipboard(nullptr)) {
    WCHAR sBuf[16];

    UINT ClipboardFormat;
    UINT Format = 0;

    while ((ClipboardFormat = EnumClipboardFormats(Format)) != 0) {
      GetClipboardFormatName(ClipboardFormat, sBuf, 16);

      if (wcscmp(sBuf, L"EoGroups") == 0) {
        HGLOBAL ClipboardDataHandle = GetClipboardData(ClipboardFormat);
        if (ClipboardDataHandle != 0) {
          CMemFile MemFile;
          EoGeVector3d vTrns;

          LPCSTR ClipboardData = (LPCSTR)GlobalLock(ClipboardDataHandle);
          if (ClipboardData != nullptr) {
            DWORD dwSizeOfBuffer = *((DWORD*)ClipboardData);
            MemFile.Write(ClipboardData, UINT(dwSizeOfBuffer));
            GlobalUnlock(ClipboardDataHandle);

            MemFile.Seek(96, CFile::begin);
            EoDbJobFile JobFile;
            JobFile.ReadMemFile(MemFile, vTrns);
          }
          break;
        }
      }
      Format = ClipboardFormat;
    }
    CloseClipboard();
  } else {
    app.WarningMessageBox(IDS_MSG_CLIPBOARD_LOCKED);
  }
}
void AeSysDoc::OnEditTrapDelete() {
  DeleteAllTrappedGroups();
  UpdateAllViews(nullptr, 0L, nullptr);
  OnEditTrapQuit();
}
void AeSysDoc::OnEditTrapQuit() {
  RemoveAllTrappedGroups();
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
void AeSysDoc::OnEditTrapCopy() {
  auto* activeView = AeSysView::GetActiveView();
  CopyTrappedGroupsToClipboard(activeView);
}
void AeSysDoc::OnEditTrapCut() {
  auto* activeView = AeSysView::GetActiveView();
  CopyTrappedGroupsToClipboard(activeView);
  DeleteAllTrappedGroups();
  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnEditTrapPaste() {
  if (::OpenClipboard(nullptr)) {
    UINT clipboardFormat = app.ClipboardFormatIdentifierForEoGroups();

    if (IsClipboardFormatAvailable(clipboardFormat)) {
      EoDlgSetPastePosition dialog;
      if (dialog.DoModal() == IDOK) {
        HGLOBAL globalHandle = GetClipboardData(clipboardFormat);
        if (globalHandle != 0) {
          EoGePoint3d minPoint;

          EoGePoint3d pivotPoint(app.GetCursorPosition());
          SetTrapPivotPoint(pivotPoint);

          LPCSTR buffer = (LPCSTR)GlobalLock(globalHandle);

          DWORD sizeOfBuffer{0};
          if (buffer == nullptr) {
            GlobalUnlock(globalHandle);
            CloseClipboard();
            app.WarningMessageBox(IDS_MSG_CLIPBOARD_LOCKED);
            return;
          }
          sizeOfBuffer = *((DWORD*)buffer);

          CMemFile memoryFile;
          memoryFile.Write(buffer, UINT(sizeOfBuffer));

          memoryFile.Seek(sizeof(DWORD), CFile::begin);
          minPoint.Read(memoryFile);

          EoGeVector3d translateVector(minPoint, pivotPoint);

          GlobalUnlock(globalHandle);

          memoryFile.Seek(96, CFile::begin);
          EoDbJobFile jobFile;
          jobFile.ReadMemFile(memoryFile, translateVector);
        }
      }
    } else if (IsClipboardFormatAvailable(CF_TEXT)) {
      HGLOBAL clipboardDataHandle = GetClipboardData(CF_TEXT);

      LPWSTR clipboardText = new WCHAR[GlobalSize(clipboardDataHandle)];

      LPCWSTR clipboardData = (LPCWSTR)GlobalLock(clipboardDataHandle);
      lstrcpyW(clipboardText, clipboardData);
      GlobalUnlock(clipboardDataHandle);

      AddTextBlock(clipboardText);

      delete[] clipboardText;
    }
    CloseClipboard();
  } else
    app.WarningMessageBox(IDS_MSG_CLIPBOARD_LOCKED);
}

void AeSysDoc::OnEditTrapWork() {
  RemoveAllTrappedGroups();
  AddGroupsToTrap(GetWorkLayer());
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
void AeSysDoc::OnEditTrapWorkAndActive() {
  RemoveAllTrappedGroups();

  for (int i = 0; i < GetLayerTableSize(); i++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(i);
    if (Layer->IsWork() || Layer->IsActive()) { AddGroupsToTrap(Layer); }
  }
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
void AeSysDoc::OnTrapCommandsCompress() { CompressTrappedGroups(); }
void AeSysDoc::OnTrapCommandsExpand() { ExpandTrappedGroups(); }
void AeSysDoc::OnTrapCommandsInvert() {
  int iTblSize = GetLayerTableSize();
  for (int i = 0; i < iTblSize; i++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(i);
    if (Layer->IsWork() || Layer->IsActive()) {
      auto LayerPosition = Layer->GetHeadPosition();
      while (LayerPosition != 0) {
        EoDbGroup* Group = Layer->GetNext(LayerPosition);
        auto GroupPosition = FindTrappedGroup(Group);
        if (GroupPosition != 0) {
          m_TrappedGroupList.RemoveAt(GroupPosition);
        } else {
          AddGroupToTrap(Group);
        }
      }
    }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}
void AeSysDoc::OnTrapCommandsSquare() {
  auto* activeView = AeSysView::GetActiveView();
  SquareTrappedGroups(activeView);
}
void AeSysDoc::OnTrapCommandsQuery() {
  EoDlgEditTrapCommandsQuery Dialog;

  if (Dialog.DoModal() == IDOK) {}
}
void AeSysDoc::OnTrapCommandsFilter() {
  EoDlgTrapFilter Dialog(this);
  if (Dialog.DoModal() == IDOK) {}
}
void AeSysDoc::OnTrapCommandsBlock() {
  if (m_TrappedGroupList.GetCount() == 0) return;

  EoDbBlock* Block;
  EoUInt16 w = BlockTableSize();
  WCHAR szBlkNam[16];

  do { swprintf_s(szBlkNam, 16, L"_%.3i", ++w); } while (LookupBlock(szBlkNam, Block));

  Block = new EoDbBlock;

  auto Position = GetFirstTrappedGroupPosition();
  while (Position != 0) {
    EoDbGroup* Group = GetNextTrappedGroup(Position);

    EoDbGroup* pSeg2 = new EoDbGroup(*Group);

    Block->AddTail(pSeg2);

    pSeg2->RemoveAll();

    delete pSeg2;
  }
  Block->SetBasePt(m_TrapPivotPoint);
  InsertBlock(CString(szBlkNam), Block);
}
void AeSysDoc::OnTrapCommandsUnblock() { m_TrappedGroupList.BreakSegRefs(); }
void AeSysDoc::OnSetupPenColor() {
  EoDlgSetupColor Dialog;
  Dialog.m_ColorIndex = static_cast<EoUInt16>(pstate.PenColor());

  if (Dialog.DoModal() == IDOK) {
    pstate.SetPenColor(nullptr, static_cast<EoInt16>(Dialog.m_ColorIndex));

    AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Pen);
  }
}

/** @brief Handles the command to set up the line type for drawing.
  This function checks if there are any line types defined in the document's line type table.
  If none are defined, it displays a warning message. Otherwise, it opens a dialog to allow
  the user to select a line type. If the user selects a line type and confirms, it updates
  the current drawing state with the selected line type and refreshes the view to reflect
  the change.
*/
void AeSysDoc::OnSetupLineType() {
  if (m_LineTypeTable.IsEmpty()) {
    app.WarningMessageBox(IDS_MSG_NO_LINETYPES_DEFINED);
    return;
  }
  EoDlgLineTypesSelection dialog(m_LineTypeTable);

  EoDbLineType* currentLineType{nullptr};

  m_LineTypeTable.LookupUsingLegacyIndex(static_cast<EoUInt16>(pstate.LineType()), currentLineType);
  dialog.SetSelectedLineType(currentLineType);

  if (dialog.DoModal() != IDOK) { return; }

  EoDbLineType* selectedLineType = dialog.GetSelectedLineType();
  if (selectedLineType == nullptr) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"AeSysDoc::OnSetupLineType: No line type selected.\n");
    return;
  }
  pstate.SetLineType(nullptr, static_cast<EoInt16>(selectedLineType->Index()));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Line);
}

void AeSysDoc::OnSetupFillHollow() { pstate.SetPolygonIntStyle(EoDb::kHollow); }
void AeSysDoc::OnSetupFillSolid() { pstate.SetPolygonIntStyle(EoDb::kSolid); }
void AeSysDoc::OnSetupFillPattern() {}
void AeSysDoc::OnSetupFillHatch() {
  EoDlgSetupHatch Dialog;
  Dialog.m_HatchXScaleFactor = hatch::dXAxRefVecScal;
  Dialog.m_HatchYScaleFactor = hatch::dYAxRefVecScal;
  Dialog.m_HatchRotationAngle = Eo::RadianToDegree(hatch::dOffAng);

  if (Dialog.DoModal() == IDOK) {
    pstate.SetPolygonIntStyle(EoDb::kHatch);
    hatch::dXAxRefVecScal = std::max(0.01, Dialog.m_HatchXScaleFactor);
    hatch::dYAxRefVecScal = std::max(0.01, Dialog.m_HatchYScaleFactor);
    hatch::dOffAng = Eo::DegreeToRadian(Dialog.m_HatchRotationAngle);
  }
}
void AeSysDoc::OnSetupNote() {
  EoDbFontDefinition FontDefinition;
  pstate.GetFontDef(FontDefinition);

  EoDlgSetupNote Dialog(&FontDefinition);

  EoDbCharacterCellDefinition CCD;
  pstate.GetCharCellDef(CCD);

  Dialog.m_TextHeight = CCD.ChrHgtGet();
  Dialog.m_TextRotationAngle = Eo::RadianToDegree(CCD.TextRotAngGet());
  Dialog.m_TextExpansionFactor = CCD.ChrExpFacGet();
  Dialog.m_CharacterSlantAngle = Eo::RadianToDegree(CCD.ChrSlantAngGet());

  if (Dialog.DoModal() == IDOK) {
    CCD.ChrHgtSet(Dialog.m_TextHeight);
    CCD.TextRotAngSet(Eo::DegreeToRadian(Dialog.m_TextRotationAngle));
    CCD.ChrExpFacSet(Dialog.m_TextExpansionFactor);
    CCD.ChrSlantAngSet(Eo::DegreeToRadian(Dialog.m_CharacterSlantAngle));
    pstate.SetCharCellDef(CCD);

    auto* activeView = AeSysView::GetActiveView();
    CDC* DeviceContext = (activeView == nullptr) ? nullptr : activeView->GetDC();

    pstate.SetFontDef(DeviceContext, FontDefinition);
  }
}
void AeSysDoc::OnToolsGroupBreak() {
  auto* activeView = AeSysView::GetActiveView();

  activeView->BreakAllPolylines();
  activeView->BreakAllSegRefs();
}

void AeSysDoc::OnToolsGroupDelete() {
  auto* activeView = AeSysView::GetActiveView();

  EoGePoint3d pt = activeView->GetCursorPosition();

  EoDbGroup* Group = activeView->SelectGroupAndPrimitive(pt);

  if (Group != 0) {
    AnyLayerRemove(Group);
    RemoveGroupFromAllViews(Group);
    if (RemoveTrappedGroup(Group) != nullptr) { activeView->UpdateStateInformation(AeSysView::TrapCount); }
    UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);
    DeletedGroupsAddTail(Group);
    app.AddStringToMessageList(IDS_SEG_DEL_TO_RESTORE);
  }
}
void AeSysDoc::OnToolsGroupDeletelast() {
  auto* activeView = AeSysView::GetActiveView();

  activeView->DeleteLastGroup();
}
void AeSysDoc::OnToolsGroupExchange() {
  if (!DeletedGroupsIsEmpty()) {
    EoDbGroup* TailGroup = DeletedGroupsRemoveTail();
    EoDbGroup* HeadGroup = DeletedGroupsRemoveHead();
    DeletedGroupsAddTail(HeadGroup);
    DeletedGroupsAddHead(TailGroup);
  }
}
void AeSysDoc::OnToolsPrimitiveSnaptoendpoint() {
  auto* activeView = AeSysView::GetActiveView();

  EoGePoint4d ptView(activeView->GetCursorPosition());
  activeView->ModelViewTransformPoint(ptView);

  if (activeView->GroupIsEngaged()) {
    EoDbPrimitive* Primitive = activeView->EngagedPrimitive();

    if (Primitive->PvtOnCtrlPt(activeView, ptView)) {
      EoGePoint3d ptEng = activeView->DetPt();
      Primitive->AddReportToMessageList(ptEng);
      activeView->SetCursorPosition(ptEng);
      return;
    }
    // Did not pivot on engaged primitive
    if (Primitive->IsPointOnControlPoint(activeView, ptView)) { EoDbGroup::SetPrimitiveToIgnore(Primitive); }
  }
  if (activeView->SelSegAndPrimAtCtrlPt(ptView) != 0) {
    EoGePoint3d ptEng = activeView->DetPt();
    activeView->EngagedPrimitive()->AddReportToMessageList(ptEng);
    activeView->SetCursorPosition(ptEng);
  }
  EoDbGroup::SetPrimitiveToIgnore(static_cast<EoDbPrimitive*>(nullptr));
}
void AeSysDoc::OnPrimGotoCenterPoint() {
  auto* activeView = AeSysView::GetActiveView();
  if (activeView->GroupIsEngaged()) {
    EoGePoint3d pt = activeView->EngagedPrimitive()->GetCtrlPt();
    activeView->SetCursorPosition(pt);
  }
}
void AeSysDoc::OnToolsPrimitiveDelete() {
  EoGePoint3d pt = app.GetCursorPosition();

  auto* activeView = AeSysView::GetActiveView();

  EoDbGroup* Group = activeView->SelectGroupAndPrimitive(pt);

  if (Group != 0) {
    auto Position = FindTrappedGroup(Group);

    LPARAM lHint = (Position != 0) ? EoDb::kGroupEraseSafeTrap : EoDb::kGroupEraseSafe;
    // erase entire group even if group has more than one primitive
    UpdateAllViews(nullptr, lHint, Group);

    if (Group->GetCount() > 1) {  // remove primitive from group
      EoDbPrimitive* Primitive = activeView->EngagedPrimitive();
      Group->FindAndRemovePrim(Primitive);
      lHint = (Position != 0) ? EoDb::kGroupSafeTrap : EoDb::kGroupSafe;
      // display the group with the primitive removed
      UpdateAllViews(nullptr, lHint, Group);
      // new group required to allow primitive to be placed into deleted group list
      Group = new EoDbGroup(Primitive);
    } else {  // deleting an entire group
      AnyLayerRemove(Group);
      RemoveGroupFromAllViews(Group);

      if (RemoveTrappedGroup(Group) != 0) { activeView->UpdateStateInformation(AeSysView::TrapCount); }
    }
    DeletedGroupsAddTail(Group);
    app.AddStringToMessageList(IDS_MSG_PRIM_ADDED_TO_DEL_GROUPS);
  }
}
void AeSysDoc::OnPrimModifyAttributes() {
  auto* activeView = AeSysView::GetActiveView();

  EoGePoint3d pt = activeView->GetCursorPosition();

  EoDbGroup* Group = activeView->SelectGroupAndPrimitive(pt);

  if (Group != 0) {
    activeView->EngagedPrimitive()->ModifyState();
    UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, activeView->EngagedPrimitive());
  }
}
void AeSysDoc::OnSetupSavePoint() {
  EoDlgSetHomePoint Dialog(AeSysView::GetActiveView());

  if (Dialog.DoModal() == IDOK) {}
}
void AeSysDoc::OnSetupGotoPoint() {
  EoDlgSelectGotoHomePoint Dialog(AeSysView::GetActiveView());

  if (Dialog.DoModal() == IDOK) {}
}
void AeSysDoc::OnSetupOptionsDraw() {
  EoDlgDrawOptions Dialog;

  if (Dialog.DoModal() == IDOK) { AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::All); }
}
void AeSysDoc::OnFileManage() {
  EoDlgFileManage dlg(this);

  if (dlg.DoModal() == IDOK) {}
}
void AeSysDoc::OnFileTracing() {
  static DWORD filterIndex = 1;

  CString filter = EoAppLoadStringResource(IDS_OPENFILE_FILTER_TRACINGS);

  wchar_t fileBuffer[MAX_PATH]{0};

  OPENFILENAME of;
  ::ZeroMemory(&of, sizeof(OPENFILENAME));
  of.lStructSize = sizeof(OPENFILENAME);
  of.hwndOwner = AfxGetMainWnd() ? AfxGetMainWnd()->GetSafeHwnd() : nullptr;
  of.hInstance = app.GetInstance();
  of.lpstrFilter = L"Tracing Files (*.tra)\0*.tra\0\0";
  of.lpstrFile = fileBuffer;
  of.nMaxFile = MAX_PATH;
  of.lpstrTitle = L"Tracing File";
  of.Flags = OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
  of.lpstrDefExt = L"tra";
  of.lpfnHook = OFNHookProcFileTracing;
  of.lpTemplateName = MAKEINTRESOURCE(IDD_TRACING_EX);
  of.nFilterIndex = filterIndex;

  if (GetOpenFileNameW(&of)) {
    filterIndex = of.nFilterIndex;

    TracingOpen(of.lpstrFile);
  }
}

void AeSysDoc::OnMaintenanceRemoveEmptyNotes() {
  int NumberOfEmptyNotes = RemoveEmptyNotesAndDelete();
  int NumberOfEmptyGroups = RemoveEmptyGroups();
  CString str;
  str.Format(L"%d notes were removed resulting in %d empty groups which were also removed.", NumberOfEmptyNotes, NumberOfEmptyGroups);
  app.AddStringToMessageList(str);
}
void AeSysDoc::OnMaintenanceRemoveEmptyGroups() {
  int NumberOfEmptyGroups = RemoveEmptyGroups();
  CString str;
  str.Format(L"%d were removed.", NumberOfEmptyGroups);
  app.AddStringToMessageList(str);
}
void AeSysDoc::OnPensEditColors() { app.EditColorPalette(); }

void AeSysDoc::OnPensLoadColors() {
  CString filter = EoAppLoadStringResource(IDS_OPENFILE_FILTER_PENCOLORS);
  CString title = EoAppLoadStringResource(IDS_OPENFILE_LOAD_PENCOLORS_TITLE);
  CString initialDir = EoAppGetPathFromCommandLine();

  CString file;
  file.GetBufferSetLength(MAX_PATH);

  OPENFILENAME of;
  ::ZeroMemory(&of, sizeof(OPENFILENAME));
  of.lStructSize = sizeof(OPENFILENAME);
  of.hwndOwner = 0;
  of.hInstance = app.GetInstance();
  of.lpstrFilter = filter;
  of.lpstrFile = file.GetBuffer();
  of.nMaxFile = MAX_PATH;
  of.lpstrTitle = title;
  of.lpstrInitialDir = initialDir;
  of.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
  of.lpstrDefExt = L"txt";

  if (GetOpenFileNameW(&of)) {
    if ((of.Flags & OFN_EXTENSIONDIFFERENT) == 0) {
      app.LoadPenColorsFromFile(of.lpstrFile);
      UpdateAllViews(nullptr, 0L, nullptr);
    } else {
      app.WarningMessageBox(IDS_MSG_FILE_TYPE_ERROR);
    }
  } else {
    auto error = CommDlgExtendedError();
    if (error != 0) { app.WarningMessageBox(IDS_MSG_OPENFILE_DIALOG_ERROR); }
  }
}

void AeSysDoc::OnPensTranslate() {
  CStdioFile fl;

  if (fl.Open(EoAppGetPathFromCommandLine() + L"\\Pens\\xlate.txt", CFile::modeRead | CFile::typeText)) {
    WCHAR pBuf[128];
    EoUInt16 wCols = 0;

    while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(WCHAR) - 1) != 0) wCols++;

    if (wCols > 0) {
      EoInt16* pColNew = new EoInt16[wCols];
      EoInt16* pCol = new EoInt16[wCols];

      EoUInt16 w = 0;

      fl.SeekToBegin();

      LPWSTR NextToken;
      while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(WCHAR) - 1) != 0) {
        NextToken = nullptr;
        pCol[w] = EoInt16(_wtoi(wcstok_s(pBuf, L",", &NextToken)));
        pColNew[w++] = EoInt16(_wtoi(wcstok_s(0, L"\n", &NextToken)));
      }
      PenTranslation(wCols, pColNew, pCol);

      delete[] pColNew;
      delete[] pCol;
    }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}
void AeSysDoc::OnFile() {
  CPoint Position(8, 8);

  AfxGetApp()->GetMainWnd()->ClientToScreen(&Position);
  CMenu* FileSubMenu = CMenu::FromHandle(app.GetSubMenu(0));
  FileSubMenu->TrackPopupMenuEx(TPM_LEFTALIGN, Position.x, Position.y, AfxGetMainWnd(), 0);
}

void AeSysDoc::OnPrimExtractNum() {
  auto* activeView = AeSysView::GetActiveView();

  auto cursorPosition = activeView->GetCursorPosition();

  if (activeView->SelectGroupAndPrimitive(cursorPosition)) {
    auto* primitive = activeView->EngagedPrimitive();

    CString number;

    if (primitive->Is(EoDb::kTextPrimitive)) {
      number = static_cast<EoDbText*>(primitive)->Text();
    } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
      number = static_cast<EoDbDimension*>(primitive)->Text();
    } else {
      return;
    }
    double value[32]{};
    int iTyp{};
    long lDef{};
    int iTokId{0};

    try {
      lex::Parse(number);
      lex::EvalTokenStream(&iTokId, &lDef, &iTyp, value);

      if (iTyp != lex::ArchitecturalUnitsLengthToken && iTyp != lex::EngineeringUnitsLengthToken && iTyp != lex::SimpleUnitsLengthToken) {
        lex::ConvertValTyp(iTyp, lex::RealToken, &lDef, value);
      }
      wchar_t Message[64]{};
      swprintf_s(Message, 64, L"%10.4f ", value[0]);
      wcscat_s(Message, 64, L"was extracted from drawing");
      app.AddStringToMessageList(std::wstring(Message));
    } catch (...) {
      app.WarningMessageBox(IDS_MSG_INVALID_NUMBER_EXTRACT);
      return;
    }
#if defined(USING_DDE)
    app.SetExtractedNumber(dVal[0]);
    dde::PostAdvise(dde::ExtNumInfo);
#endif  // USING_DDE
  }
}

void AeSysDoc::OnPrimExtractStr() {
  auto* activeView = AeSysView::GetActiveView();

  EoGePoint3d pt = activeView->GetCursorPosition();

  if (activeView->SelectGroupAndPrimitive(pt)) {
    EoDbPrimitive* Primitive = activeView->EngagedPrimitive();

    CString String;

    if (Primitive->Is(EoDb::kTextPrimitive)) {
      String = static_cast<EoDbText*>(Primitive)->Text();
    } else if (Primitive->Is(EoDb::kDimensionPrimitive)) {
      String = static_cast<EoDbDimension*>(Primitive)->Text();
    } else {
      return;
    }
    String += L" was extracted from drawing";
    app.AddStringToMessageList(String);
#if defined(USING_DDE)
    app.SetExtractedString(String);
    dde::PostAdvise(dde::ExtStrInfo);
#endif  // USING_DDE
  }
  return;
}
// Returns a pointer to the currently active document.
AeSysDoc* AeSysDoc::GetDoc() {
  auto* frame = static_cast<CMDIFrameWndEx*>(AfxGetMainWnd());
  if (frame == nullptr) { return nullptr; }
  auto* child = static_cast<CMDIChildWndEx*>(frame->MDIGetActive());

  return (child == nullptr) ? nullptr : static_cast<AeSysDoc*>(child->GetActiveDocument());
}
void AeSysDoc::AddGroupToAllViews(EoDbGroup* group) {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != 0) {
    auto* view = static_cast<AeSysView*>(GetNextView(viewPosition));
    view->AddGroup(group);
  }
}
void AeSysDoc::AddGroupsToAllViews(EoDbGroupList* groups) {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != 0) {
    auto* view = static_cast<AeSysView*>(GetNextView(viewPosition));
    view->AddGroups(groups);
  }
}
void AeSysDoc::RemoveAllGroupsFromAllViews() {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != 0) {
    auto* view = static_cast<AeSysView*>(GetNextView(viewPosition));
    view->RemoveAllGroups();
  }
}
void AeSysDoc::RemoveGroupFromAllViews(EoDbGroup* group) {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != 0) {
    auto* view = static_cast<AeSysView*>(GetNextView(viewPosition));
    view->RemoveGroup(group);
  }
}
void AeSysDoc::ResetAllViews() {
  auto viewPosition = GetFirstViewPosition();
  while (viewPosition != 0) {
    auto* view = static_cast<AeSysView*>(GetNextView(viewPosition));
    view->ResetView();
  }
}
void AeSysDoc::OnHelpKey() {
  switch (app.CurrentMode()) {
    case ID_MODE_DRAW:
      WinHelpW(app.GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"DRAW"));
      break;

    case ID_MODE_EDIT: {
      WinHelpW(app.GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"EDIT"));
      break;
    }
    case ID_MODE_TRAP:
    case ID_MODE_TRAPR: {
      WinHelpW(app.GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"TRAP"));
      break;
    }
  }
}

void AeSysDoc::DeleteNodalResources() {
  auto UniquePointPosition = GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) { delete GetNextUniquePoint(UniquePointPosition); }
  RemoveAllUniquePoints();
  auto MaskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (MaskedPrimitivePosition != 0) { delete GetNextMaskedPrimitive(MaskedPrimitivePosition); }
  RemoveAllMaskedPrimitives();
  RemoveAllNodalGroups();
}
void AeSysDoc::UpdateNodalList(EoDbGroup* group, EoDbPrimitive* primitive, DWORD mask, int bit, EoGePoint3d point) {
  if (app.m_NodalModeAddGroups) {
    if (!btest(mask, bit)) {
      if (!FindNodalGroup(group)) { AddNodalGroup(group); }
      AddPrimitiveBit(primitive, bit);
      if (AddUniquePoint(point) == 1) {
        EoDbPoint PointPrimitive(252, 8, point);
        UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, &PointPrimitive);
      }
    }
  } else {
    if (btest(mask, bit)) {
      RemovePrimitiveBit(primitive, bit);

      if (RemoveUniquePoint(point) == 0) {
        EoDbPoint PointPrimitive(252, 8, point);
        UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, &PointPrimitive);
      }
    }
  }
}

int AeSysDoc::AddUniquePoint(const EoGePoint3d& point) {
  auto UniquePointPosition = GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    EoGeUniquePoint* UniquePoint = GetNextUniquePoint(UniquePointPosition);
    if (point == UniquePoint->m_Point) {
      (UniquePoint->m_References)++;
      return (UniquePoint->m_References);
    }
  }
  AddUniquePoint(new EoGeUniquePoint(point, 1));
  return (1);
}
void AeSysDoc::DisplayUniquePoints() {
  EoDbGroup Group;
  auto UniquePointPosition = GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    EoGeUniquePoint* UniquePoint = GetNextUniquePoint(UniquePointPosition);
    Group.AddTail(new EoDbPoint(252, 8, UniquePoint->m_Point));
  }
  UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &Group);
  Group.DeletePrimitivesAndRemoveAll();
}
int AeSysDoc::RemoveUniquePoint(const EoGePoint3d& point) {
  int References = 0;

  auto UniquePointPosition = GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    auto Position = UniquePointPosition;
    EoGeUniquePoint* UniquePoint = GetNextUniquePoint(UniquePointPosition);
    if (point == UniquePoint->m_Point) {
      References = --(UniquePoint->m_References);

      if (References == 0) {
        RemoveUniquePointAt(Position);
        delete UniquePoint;
      }
      break;
    }
  }
  return References;
}
void AeSysDoc::AddPrimitiveBit(EoDbPrimitive* primitive, int bit) {
  EoDbMaskedPrimitive* MaskedPrimitive = 0;

  auto MaskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (MaskedPrimitivePosition != 0) {
    auto posCur = MaskedPrimitivePosition;
    MaskedPrimitive = GetNextMaskedPrimitive(MaskedPrimitivePosition);
    if (MaskedPrimitive->GetPrimitive() == primitive) {
      MaskedPrimitivePosition = posCur;
      break;
    }
  }
  if (MaskedPrimitivePosition == 0) {
    MaskedPrimitive = new EoDbMaskedPrimitive(primitive, 0);
    AddMaskedPrimitive(MaskedPrimitive);
  }
  MaskedPrimitive->SetMaskBit(bit);
}
void AeSysDoc::RemovePrimitiveBit(EoDbPrimitive* primitive, int bit) {
  EoDbMaskedPrimitive* MaskedPrimitive = 0;

  auto MaskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (MaskedPrimitivePosition != 0) {
    auto posCur = MaskedPrimitivePosition;
    MaskedPrimitive = GetNextMaskedPrimitive(MaskedPrimitivePosition);
    if (MaskedPrimitive->GetPrimitive() == primitive) {
      MaskedPrimitivePosition = posCur;
      break;
    }
  }
  if (MaskedPrimitivePosition != 0) { MaskedPrimitive->ClearMaskBit(bit); }
}
DWORD AeSysDoc::GetPrimitiveMask(EoDbPrimitive* primitive) {
  EoDbMaskedPrimitive* MaskedPrimitive = 0;

  auto MaskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (MaskedPrimitivePosition != 0) {
    auto posCur = MaskedPrimitivePosition;
    MaskedPrimitive = GetNextMaskedPrimitive(MaskedPrimitivePosition);
    if (MaskedPrimitive->GetPrimitive() == primitive) {
      MaskedPrimitivePosition = posCur;
      break;
    }
  }
  return ((MaskedPrimitivePosition != 0) ? MaskedPrimitive->GetMask() : 0UL);
}

#if defined(USING_ODA)
void AeSysDoc::ConvertPegDocument() {
  ConvertBlockTable();
  ConvertGroupsInLayers();
  ConvertGroupsInBlocks();
}

void AeSysDoc::ConvertBlockTable() {
  OdDbBlockTablePtr Blocks = m_DatabasePtr->getBlockTableId().safeOpenObject(OdDb::kForWrite);

  CString Name;
  EoDbBlock* PegBlock;

  auto position = GetFirstBlockPosition();
  while (position != 0) {
    GetNextBlock(position, Name, PegBlock);
    if (Blocks->getAt(OdString(Name)).isNull()) {
      OdDbBlockTableRecordPtr Block = OdDbBlockTableRecord::createObject();
      Block->setName(OdString(Name));
      Blocks->add(Block);
    }
  }
}
void AeSysDoc::ConvertGroupsInBlocks() {
  OdDbBlockTablePtr Blocks = m_DatabasePtr->getBlockTableId().safeOpenObject(OdDb::kForWrite);

  CString Key;
  EoDbBlock* Block;

  auto position = GetFirstBlockPosition();
  while (position != 0) {
    GetNextBlock(position, Key, Block);

    WCHAR szName[64];
    wcscpy_s(szName, 64, Key);

    ConvertGroup(Block, Blocks->getAt(szName));
  }
}
void AeSysDoc::ConvertGroupsInLayers() {
  OdDbLayerTablePtr Layers = m_DatabasePtr->getLayerTableId().safeOpenObject(OdDb::kForWrite);
  OdDbObjectId ModelSpace = m_DatabasePtr->getModelSpaceId();

  int NumberOfLayers = GetLayerTableSize();

  for (int n = 0; n < NumberOfLayers; n++) {
    EoDbLayer* Layer = GetLayerTableLayerAt(n);

    OdDbObjectId LayerRecord = Layers->getAt((LPCWSTR)Layer->Name());
    m_DatabasePtr->setCLAYER(LayerRecord);

    auto position = Layer->GetHeadPosition();
    while (position != 0) {
      EoDbGroup* Group = Layer->GetNext(position);
      ConvertGroup(Group, ModelSpace);
    }
  }
  m_DatabasePtr->setCLAYER(m_DatabasePtr->getLayerZeroId());
}
void AeSysDoc::ConvertGroup(EoDbGroup* group, const OdDbObjectId& modelSpace) {
  auto position = group->GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = group->GetNext(position);
    /* OdDbEntity* Entity = */ Primitive->Convert(modelSpace);
  }
}

void AeSysDoc::OnSetupLayerproperties() {
  EoDlgLayerPropertiesManager LayerPropertiesManager(m_DatabasePtr);
  ;

  if (IDOK == LayerPropertiesManager.DoModal()) { UpdateAllViews(0); }
}

BOOL AeSysDoc::DoPromptFileName(CString& fileName, UINT titleResourceIdentifier, DWORD flags) {
  OdDb::DwgVersion DrawingVersion(m_DatabasePtr->originalFileVersion());
  CString Extension = fileName.Right(3);

  DWORD FilterIndex(1);

  if (Extension.CompareNoCase(L"peg") == 0) { FilterIndex = 1; }
  if ((DrawingVersion == OdDb::vAC24 || DrawingVersion == OdDb::kDHL_1024) && Extension.CompareNoCase(L"dxf") == 0) { FilterIndex = 2; }
  if ((DrawingVersion == OdDb::vAC21 || DrawingVersion == OdDb::kDHL_1021) && Extension.CompareNoCase(L"dxf") == 0) { FilterIndex = 3; }
  if ((DrawingVersion == OdDb::kDHL_2400a || DrawingVersion == OdDb::kDHL_1024) && Extension.CompareNoCase(L"dxf") == 0) {
    FilterIndex = 4;
  }
  if ((DrawingVersion == OdDb::kDHL_2100a || DrawingVersion == OdDb::kDHL_1021) && Extension.CompareNoCase(L"dxf") == 0) {
    FilterIndex = 5;
  }
  if (DrawingVersion == OdDb::vAC24 && Extension.CompareNoCase(L"dwg") == 0) { FilterIndex = 6; }
  if (DrawingVersion == OdDb::vAC21 && Extension.CompareNoCase(L"dwg") == 0) { FilterIndex = 7; }
  if (fileName.Find('.') != -1) { fileName = fileName.Left(fileName.Find('.')); }
  CString Filter = EoAppLoadStringResource(IDS_SAVEFILE_FILTER);

  CFileDialog FileDialog(FALSE, nullptr, fileName.GetBuffer(_MAX_PATH), flags, Filter);

  FileDialog.m_ofn.nFilterIndex = FilterIndex;

  CString Title = EoAppLoadStringResource(titleResourceIdentifier);
  FileDialog.m_ofn.lpstrTitle = Title;

  int Result = FileDialog.DoModal();
  fileName.ReleaseBuffer();

  FilterIndex = FileDialog.m_ofn.nFilterIndex;

  if (fileName.Find('.') == -1) {
    if (FilterIndex == 1) {
      fileName += L".peg";
    } else if (FilterIndex <= 5) {
      fileName += L".dxf";
    } else if (FilterIndex <= 7) {
      fileName += L".dwg";
    }
  }
  if (FilterIndex == 1) {
    m_SaveAsType = EoDb::kPeg;
  } else if (FilterIndex <= 3) {
    m_SaveAsType = EoDb::kDxf;
  } else if (FilterIndex <= 5) {
    m_SaveAsType = EoDb::kDxb;
  } else {
    m_SaveAsType = EoDb::kDwg;
  }
  switch (FilterIndex) {
    case 1:
      m_SaveAsVersion = OdDb::kDHL_Unknown;
      break;
    case 2:
    case 4:
    case 6:
      m_SaveAsVersion = OdDb::vAC24;
      break;
    case 3:
    case 5:
    case 7:
      m_SaveAsVersion = OdDb::vAC21;
      break;
    default:
      m_SaveAsVersion = m_DatabasePtr->originalFileVersion();
      break;
  }
  return Result == IDOK;
}
#endif  // USING_ODA
