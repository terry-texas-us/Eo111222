#include "Stdafx.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
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
#include "EoDlgSetupPointStyle.h"
#include "EoDlgTrapFilter.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeTransformMatrix.h"
#include "EoGeUniquePoint.h"
#include "EoGeVector3d.h"
#include "EoGsRenderState.h"
#include "Hatch.h"
#include "Lex.h"
#include "Resource.h"
#include "drw_base.h"
#include "libdxfrw.h"

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
END_MESSAGE_MAP()

AeSysDoc::AeSysDoc()
    : m_HeaderSection{},
      m_LineTypeTable{},
      m_BlocksTable{},
      m_DeletedGroupList{},
      m_trappedGroups{},
      m_NodalGroupList{},
      m_MaskedPrimitives{},
      m_UniquePoints{},
      m_LayerTable{},
      m_trapPivotPoint{},
      m_ContinuousLineType{},
      m_workLayer{},
      m_IdentifiedLayerName{},
      m_SaveAsType{EoDb::FileTypes::Unknown} {}

AeSysDoc::~AeSysDoc() {}

void AeSysDoc::DeleteContents() {
  ATLTRACE2(traceGeneral, 0, L"AeSysDoc<%p>::DeleteContents() - BlockTableSize: %d\n", this, BlockTableSize());

  // TODO: Release EoDbDrwInterface resources if any

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
  CString PathName = pathName;
  if (PathName.IsEmpty()) {
    auto* Template = GetDocTemplate();
    assert(Template != nullptr);

    PathName = m_strPathName;
    if (replace && PathName.IsEmpty()) {
      PathName = m_strTitle;

      int FirstBadCharacterIndex = PathName.FindOneOf(L" #%;/\\");
      if (FirstBadCharacterIndex != -1) { PathName.ReleaseBuffer(FirstBadCharacterIndex); }
      CString Extension;
      if (Template->GetDocString(Extension, CDocTemplate::filterExt) && !Extension.IsEmpty()) {
        assert(Extension[0] == '.');
        PathName += Extension;
      }
    }
    /// @todo Implement a EoDbDrwInterface Save As dialog if needed
  }
  if (!OnSaveDocument(PathName)) {
    if (pathName == nullptr) {
      TRY { CFile::Remove(PathName); }
      CATCH_ALL(e) {
        ATLTRACE2(traceGeneral, 0, L"Warning: failed to delete file after failed SaveAs.\n");
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

  auto applicationPath = App::PathFromCommandLine();

  // TODO: Peg uses index for line types, need to map names to indexes (index 0 to 41 have been hard coded in Peg).
  //       Need to ensure that line types loaded here match those indexes.
  // m_LineTypeTable.LoadLineTypesFromTxtFile(applicationPath + L"\\res\\LineTypes\\LineTypes.txt");
  // m_LineTypeTable.LoadLineTypesFromTxtFile(applicationPath + L"\\res\\LineTypes\\LineTypes-ACAD(scaled to AeSys).txt");
  // m_LineTypeTable.LoadLineTypesFromTxtFile(applicationPath + L"\\res\\LineTypes\\LineTypes-ISO128(scaled to AeSys).txt");
}

BOOL AeSysDoc::OnNewDocument() {
  if (!CDocument::OnNewDocument()) { return FALSE; }

  SetCommonTableEntries();

  m_SaveAsType = EoDb::FileTypes::Peg;
  SetWorkLayer(GetLayerTableLayerAt(0));
  InitializeGroupAndPrimitiveEdit();

  return TRUE;
}
BOOL AeSysDoc::OnOpenDocument(LPCWSTR pathName) {
  ATLTRACE2(traceGeneral, 0, L"AeSysDoc<%p>::OnOpenDocument(%s)\n", this, pathName);

  switch (App::FileTypeFromPath(pathName)) {
    case EoDb::FileTypes::Dwg:
      break;
    case EoDb::FileTypes::Dxf:
    case EoDb::FileTypes::Dxb: {
      EoDbDrwInterface dxfInterface(this);
      dxfRW dxfReader(Eo::WStringToMultiByte(pathName).data());
      dxfReader.setDebug(static_cast<DRW::DebugTraceLevel>(DRW::none));
      SetCommonTableEntries();
      bool success = dxfReader.read(&dxfInterface, true);  // true for verbose output, false for silent
      if (success) {
        ATLTRACE2(traceGeneral, 0, L"3dFace: %d.\n", dxfInterface.countOf3dFace);
        ATLTRACE2(traceGeneral, 0, L"Arc: %d.\n", dxfInterface.countOfArc);
        ATLTRACE2(traceGeneral, 0, L"Circle: %d.\n", dxfInterface.countOfCircle);
        ATLTRACE2(traceGeneral, 0, L"DimAlign: %d.\n", dxfInterface.countOfDimAlign);
        ATLTRACE2(traceGeneral, 0, L"DimLinear: %d.\n", dxfInterface.countOfDimLinear);
        ATLTRACE2(traceGeneral, 0, L"DimAngular: %d.\n", dxfInterface.countOfDimAngular);
        ATLTRACE2(traceGeneral, 0, L"DimAngular3P: %d.\n", dxfInterface.countOfDimAngular3P);
        ATLTRACE2(traceGeneral, 0, L"DimDiametric: %d.\n", dxfInterface.countOfDimDiametric);
        ATLTRACE2(traceGeneral, 0, L"DimLinear: %d.\n", dxfInterface.countOfDimLinear);
        ATLTRACE2(traceGeneral, 0, L"DimOrdinate: %d.\n", dxfInterface.countOfDimOrdinate);
        ATLTRACE2(traceGeneral, 0, L"DimRadial: %d.\n", dxfInterface.countOfDimRadial);
        ATLTRACE2(traceGeneral, 0, L"Ellipse: %d.\n", dxfInterface.countOfEllipse);
        ATLTRACE2(traceGeneral, 0, L"Hatch: %d.\n", dxfInterface.countOfHatch);
        ATLTRACE2(traceGeneral, 0, L"Image: %d.\n", dxfInterface.countOfImage);
        ATLTRACE2(traceGeneral, 0, L"Insert: %d.\n", dxfInterface.countOfInsert);
        ATLTRACE2(traceGeneral, 0, L"Knot: %d.\n", dxfInterface.countOfKnot);
        ATLTRACE2(traceGeneral, 0, L"Line: %d.\n", dxfInterface.countOfLine);
        ATLTRACE2(traceGeneral, 0, L"LWPolyline: %d.\n", dxfInterface.countOfLWPolyline);
        ATLTRACE2(traceGeneral, 0, L"MText: %d.\n", dxfInterface.countOfMText);
        ATLTRACE2(traceGeneral, 0, L"Point: %d.\n", dxfInterface.countOfPoint);
        ATLTRACE2(traceGeneral, 0, L"Polyline: %d.\n", dxfInterface.countOfPolyline);
        ATLTRACE2(traceGeneral, 0, L"Ray: %d.\n", dxfInterface.countOfRay);
        ATLTRACE2(traceGeneral, 0, L"Solid: %d.\n", dxfInterface.countOfSolid);
        ATLTRACE2(traceGeneral, 0, L"Spline: %d.\n", dxfInterface.countOfSpline);
        ATLTRACE2(traceGeneral, 0, L"Text: %d.\n", dxfInterface.countOfText);
        ATLTRACE2(traceGeneral, 0, L"Trace: %d.\n", dxfInterface.countOfTrace);
        ATLTRACE2(traceGeneral, 0, L"Viewport: %d.\n", dxfInterface.countOfViewport);
      } else {
        ATLTRACE2(traceGeneral, 0, L"Error loading DXF file.\n");
      }
      // read dxf/dxb file and save pointer to the database
      // determine the version of the file (from header section) and set m_SaveAsType to EoDb::kDxf or EoDb::kDxb
      // create EoDbDrwInterface object and do conversion
      // set work layer to layer `0`
    } break;
    case EoDb::FileTypes::Peg:
      try {
        EoDbPegFile file;
        if (file.Open(pathName, CFile::modeRead | CFile::shareDenyNone)) {
          SetCommonTableEntries();
          file.Load(this);
          m_SaveAsType = EoDb::FileTypes::Peg;
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
    case EoDb::FileTypes::Tracing:
    case EoDb::FileTypes::Job:
      TracingOpen(pathName);
      break;

    case EoDb::FileTypes::Unknown:
      // Let the base class handle it and probably fail
    default:
      return CDocument::OnOpenDocument(pathName);
  }
  return TRUE;
}
BOOL AeSysDoc::OnSaveDocument(LPCWSTR pathName) {
  BOOL ReturnStatus = FALSE;

  switch (m_SaveAsType) {
    case EoDb::FileTypes::Peg: {
      WriteShadowFile();
      EoDbPegFile File;
      CFileException e;
      if (File.Open(pathName, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive, &e)) {
        File.Unload(this);
        ReturnStatus = TRUE;
      }
      break;
    }
    case EoDb::FileTypes::Tracing:
    case EoDb::FileTypes::Job: {
      auto* layer = GetLayerTableLayer(pathName);
      if (layer != nullptr) {
        CFile File(pathName, CFile::modeCreate | CFile::modeWrite);
        if (File == CFile::hFileNull) {
          app.WarningMessageBox(IDS_MSG_TRACING_WRITE_FAILURE, pathName);
          return FALSE;
        }
        if (m_SaveAsType == EoDb::FileTypes::Job) {
          EoDbJobFile JobFile;
          JobFile.WriteHeader(File);
          JobFile.WriteLayer(File, layer);
        } else if (m_SaveAsType == EoDb::FileTypes::Tracing) {
          EoDbTracingFile TracingFile;
          TracingFile.WriteHeader(File);
          TracingFile.WriteLayer(File, layer);
        }
        app.AddStringToMessageList(IDS_MSG_TRACING_SAVE_SUCCESS, pathName);
        ReturnStatus = TRUE;
      }
      break;
    }
    case EoDb::FileTypes::Dwg:
    case EoDb::FileTypes::Dxf:
    case EoDb::FileTypes::Dxb:
      // TODO: Implement DXF/DXB saving using EoDbDrwInterface and libdxfrw
      break;

    case EoDb::FileTypes::Unknown:
      break;

    default:
      app.WarningMessageBox(IDS_MSG_NOTHING_TO_SAVE);
  }
  return ReturnStatus;
}

void AeSysDoc::AddTextBlock(LPWSTR pszText) {
  auto cursorPosition = app.GetCursorPosition();

  const auto& fontDefinition = renderState.FontDefinition();
  auto characterCellDefinition = renderState.CharacterCellDefinition();

  EoGeReferenceSystem ReferenceSystem(cursorPosition, characterCellDefinition);

  LPWSTR NextToken = nullptr;
  LPWSTR pText = wcstok_s(pszText, L"\r", &NextToken);
  while (pText != 0) {
    if (wcslen(pText) > 0) {
      auto* Group = new EoDbGroup(new EoDbText(fontDefinition, ReferenceSystem, pText));
      AddWorkLayerGroup(Group);
      UpdateAllViews(nullptr, EoDb::kGroup, Group);
    }
    ReferenceSystem.SetOrigin(text_GetNewLinePos(fontDefinition, ReferenceSystem, 1.0, 0));
    pText = wcstok_s(0, L"\r", &NextToken);
    if (pText == 0) break;
    pText++;
  }
}

void AeSysDoc::DeletedGroupsRestore() {
  if (!DeletedGroupsIsEmpty()) {
    auto* Group = DeletedGroupsRemoveTail();
    AddWorkLayerGroup(Group);
    UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
  }
}
void AeSysDoc::GetExtents(
    AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& transformMatrix) {
  ptMin(Eo::boundsMax, Eo::boundsMax, Eo::boundsMax);
  ptMax(Eo::boundsMin, Eo::boundsMin, Eo::boundsMin);

  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (!layer->IsOff()) { layer->GetExtents(view, ptMin, ptMax, transformMatrix); }
  }
}

int AeSysDoc::NumberOfGroupsInWorkLayer() {
  INT_PTR count{};

  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (layer->IsWork()) { count += layer->GetCount(); }
  }
  return static_cast<int>(count);
}

int AeSysDoc::NumberOfGroupsInActiveLayers() {
  INT_PTR count{};

  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (layer->IsActive()) { count += layer->GetCount(); }
  }
  return static_cast<int>(count);
}

void AeSysDoc::DisplayAllLayers(AeSysView* view, CDC* deviceContext) {
  ATLTRACE2(traceGeneral, 3, L"AeSysDoc<%p>::DisplayAllLayers(%p, %p)\n", this, view, deviceContext);

  try {
    bool identifyTrap = app.IsTrapHighlighted() && !IsTrapEmpty();

    RemoveAllGroupsFromAllViews();

    auto backgroundColor = deviceContext->GetBkColor();
    deviceContext->SetBkColor(ViewBackgroundColor);

    EoDbPolygon::SetSpecialPolygonStyle(
        view->RenderAsWireframe() ? EoDb::PolygonStyle::Hollow : EoDb::PolygonStyle::Special);
    int savedRenderState = renderState.Save();

    for (int i = 0; i < GetLayerTableSize(); i++) {
      auto* layer = GetLayerTableLayerAt(i);
      layer->Display(view, deviceContext, identifyTrap);
    }
    renderState.Restore(deviceContext, savedRenderState);
    EoDbPolygon::SetSpecialPolygonStyle(EoDb::PolygonStyle::Special);

    deviceContext->SetBkColor(backgroundColor);
  } catch (CException* e) { e->Delete(); }
}

[[nodiscard]] EoDbLayer* AeSysDoc::GetLayerTableLayer(const CString& layerName) {
  auto i = FindLayerTableLayer(layerName);
  return (i < 0 ? nullptr : m_LayerTable.GetAt(i));
}
EoDbLayer* AeSysDoc::GetLayerTableLayerAt(int index) {
  return (index >= (int)m_LayerTable.GetSize() ? nullptr : m_LayerTable.GetAt(index));
}

int AeSysDoc::FindLayerTableLayer(const CString& layerName) const {
  for (auto i = 0; i < m_LayerTable.GetSize(); i++) {
    auto* layer = m_LayerTable.GetAt(i);
    if (layerName.CompareNoCase(layer->Name()) == 0) { return i; }
  }
  return -1;
}

void AeSysDoc::RemoveAllLayerTableLayers() {
  for (std::uint16_t layerTableIndex = 0; layerTableIndex < m_LayerTable.GetSize(); layerTableIndex++) {
    auto* layer = m_LayerTable.GetAt(layerTableIndex);
    if (layer) {
      layer->DeleteGroupsAndRemoveAll();
      delete layer;
    }
  }
  m_LayerTable.RemoveAll();
}

void AeSysDoc::RemoveLayerTableLayerAt(int i) {
  auto* layer = GetLayerTableLayerAt(i);

  layer->DeleteGroupsAndRemoveAll();
  delete layer;

  m_LayerTable.RemoveAt(i);
}

void AeSysDoc::RemoveEmptyLayers() {
  for (int index = GetLayerTableSize() - 1; index > 0; index--) {
    auto* layer = GetLayerTableLayerAt(index);

    if (layer && layer->IsEmpty()) {
      layer->DeleteGroupsAndRemoveAll();
      delete layer;
      m_LayerTable.RemoveAt(index);
    }
  }
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
      m_SaveAsType = EoDb::FileTypes::Unknown;

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

  auto Filter = App::LoadStringResource(IDS_OPENFILE_FILTER_TRACINGS);

  OPENFILENAME of{};
  of.lStructSize = sizeof(OPENFILENAME);
  of.hInstance = AeSys::GetInstance();
  of.lpstrFilter = Filter;
  of.lpstrFile = new wchar_t[MAX_PATH];
  wcscpy_s(of.lpstrFile, MAX_PATH, strName);
  of.nMaxFile = MAX_PATH;
  of.lpstrTitle = L"Melt As";
  of.Flags = OFN_OVERWRITEPROMPT;
  of.lpstrDefExt = L"tra";

  if (GetSaveFileNameW(&of)) {
    strName = of.lpstrFile;

    EoDb::FileTypes FileType = App::FileTypeFromPath(strName);
    if (FileType == EoDb::FileTypes::Tracing || FileType == EoDb::FileTypes::Job) {
      CFile File(strName, CFile::modeWrite | CFile::modeCreate);
      if (File != CFile::hFileNull) {
        if (FileType == EoDb::FileTypes::Job) {
          EoDbJobFile JobFile;
          JobFile.WriteHeader(File);
          JobFile.WriteLayer(File, layer);
        } else {
          EoDbTracingFile TracingFile;
          TracingFile.WriteHeader(File);
          TracingFile.WriteLayer(File, layer);
        }
        layer->ClearStateFlag();
        layer->MakeResident();
        layer->SetStateStatic();
        layer->SetTracingFlg(EoDbLayer::kTracingIsMapped);

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
void AeSysDoc::RemoveLayerTableLayer(const CString& strName) {
  int i = FindLayerTableLayer(strName);

  if (i >= 0) RemoveLayerTableLayerAt(i);
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

    if (layer->SelectGroupUsingPoint(pt) != 0) { return layer; }
  }
  return nullptr;
}

int AeSysDoc::RemoveEmptyNotesAndDelete() {
  int count{};

  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    count += layer->RemoveEmptyNotesAndDelete();
  }

  //Note: remove empty notes from blocks

  CString Key;
  EoDbBlock* Block{};

  auto position = m_BlocksTable.GetStartPosition();
  while (position != nullptr) { m_BlocksTable.GetNextAssoc(position, Key, Block); }
  return count;
}
int AeSysDoc::RemoveEmptyGroups() {
  int count{};

  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    count += layer->RemoveEmptyGroups();
  }

  //Note: remove empty groups from blocks

  CString Key;
  EoDbBlock* Block{};

  auto position = m_BlocksTable.GetStartPosition();
  while (position != nullptr) { m_BlocksTable.GetNextAssoc(position, Key, Block); }
  return count;
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
[[nodiscard]] EoDbGroup* AeSysDoc::GetLastWorkLayerGroup() const {
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
  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (layer->IsWork() || layer->IsActive()) {
      if (layer->Remove(group) != 0) {
        AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::WorkCount);
        SetModifiedFlag(TRUE);

        return layer;
      }
    }
  }
  return nullptr;
}

void AeSysDoc::TracingFuse(CString& nameAndLocation) {
  auto* layer = GetLayerTableLayer(nameAndLocation);
  if (layer != nullptr) {
    wchar_t title[MAX_PATH]{};
    GetFileTitleW(nameAndLocation, title, MAX_PATH);
    wchar_t* context{};
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
  EoDb::FileTypes FileType = App::FileTypeFromPath(pathName);
  if (FileType != EoDb::FileTypes::Tracing && FileType != EoDb::FileTypes::Job) { return false; }
  if (layer == nullptr) { return false; }

  bool bFileOpen = false;

  if (FileType == EoDb::FileTypes::Tracing) {
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
  return bFileOpen;
}

bool AeSysDoc::TracingMap(const CString& pathName) {
  EoDb::FileTypes FileType = App::FileTypeFromPath(pathName);
  if (FileType != EoDb::FileTypes::Tracing && FileType != EoDb::FileTypes::Job) { return false; }
  bool fileOpen{};

  auto* layer = GetLayerTableLayer(pathName);

  if (layer != nullptr) {
    if (layer->IsOpened())
      app.WarningMessageBox(IDS_MSG_CLOSE_TRACING_FIRST, pathName);
    else
      fileOpen = true;
  } else {
    layer = new EoDbLayer(pathName, EoDbLayer::kIsStatic);

    fileOpen = TracingLoadLayer(pathName, layer);

    if (fileOpen)
      AddLayerTableLayer(layer);
    else
      delete layer;
  }
  if (fileOpen) {
    layer->SetTracingFlg(EoDbLayer::kTracingIsMapped);
    UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);
  }
  return fileOpen;
}

bool AeSysDoc::TracingOpen(const CString& fileName) {
  // Opens tracing file.

  EoDbLayer* layer{};

  int iLayId = FindLayerTableLayer(fileName);

  if (iLayId > 0) {  // already loaded
    layer = GetLayerTableLayerAt(iLayId);
    layer->ClearStateFlag(EoDbLayer::kIsResident);
  } else {  // create a new layer and append all the groups in the group file.

    layer = new EoDbLayer(fileName, EoDbLayer::kIsWork | EoDbLayer::kIsActive);
    AddLayerTableLayer(layer);

    TracingLoadLayer(fileName, layer);

    AddGroupsToAllViews(layer);
  }
  layer->SetTracingFlg(EoDbLayer::kTracingIsOpened);

  m_SaveAsType = EoDb::FileTypes::Tracing;
  SetWorkLayer(layer);

  UpdateAllViews(nullptr, 0L, nullptr);

  return true;
}

bool AeSysDoc::TracingView(const CString& pathName) {
  EoDb::FileTypes fileType = App::FileTypeFromPath(pathName);
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
    layer = new EoDbLayer(pathName, EoDbLayer::kIsStatic);
    fileOpen = TracingLoadLayer(pathName, layer);
    if (fileOpen) {
      AddLayerTableLayer(layer);
    } else {
      delete layer;
    }
  }
  if (fileOpen) {
    layer->SetTracingFlg(EoDbLayer::kTracingIsViewed);
    UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);
  }
  return fileOpen;
}

void AeSysDoc::WriteShadowFile() {
  if (m_SaveAsType == EoDb::FileTypes::Peg) {
    CString shadowFilePath(app.ShadowFolderPath());
    shadowFilePath += GetTitle();
    int nExt = shadowFilePath.Find('.');
    if (nExt > 0) {
      CFileStatus fs;
      CFile::GetStatus(GetPathName(), fs);

      shadowFilePath.Truncate(nExt);
      shadowFilePath += fs.m_mtime.Format(L"_%Y%m%d%H%M");
      shadowFilePath += L".peg";

      CFileException e;
      EoDbPegFile fp;
      if (!fp.Open(shadowFilePath, CFile::modeWrite, &e)) {
        fp.Open(shadowFilePath, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive, &e);
        fp.Unload(this);
        app.WarningMessageBox(IDS_MSG_FILE_SHADOWED_AS, shadowFilePath);
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
    auto* layer = GetLayerTableLayerAt(i);

    if (layer->IsActive()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      layer->DeleteGroupsAndRemoveAll();
    }
  }
}

void AeSysDoc::OnClearAllLayers() {
  InitializeGroupAndPrimitiveEdit();

  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    auto* layer = GetLayerTableLayerAt(i);

    if (layer->IsInternal()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      layer->DeleteGroupsAndRemoveAll();
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
    auto* layer = GetLayerTableLayerAt(i);

    if (!layer->IsInternal()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      layer->DeleteGroupsAndRemoveAll();
    }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnClearMappedTracings() {
  InitializeGroupAndPrimitiveEdit();
  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    auto* layer = GetLayerTableLayerAt(i);

    if (layer->IsMapped()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);

      if (layer->IsResident()) {
        layer->ClrTracingFlg(EoDbLayer::kTracingIsMapped);
        layer->SetStateOff();
      } else {
        RemoveLayerTableLayerAt(i);
      }
    }
  }
}

void AeSysDoc::OnClearViewedTracings() {
  InitializeGroupAndPrimitiveEdit();
  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    auto* layer = GetLayerTableLayerAt(i);

    if (layer->IsViewed()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);

      if (layer->IsResident()) {
        layer->ClrTracingFlg(EoDbLayer::kTracingIsViewed);
        layer->SetStateOff();
      } else {
        RemoveLayerTableLayerAt(i);
      }
    }
  }
}

void AeSysDoc::OnPrimBreak() {
  auto* activeView = AeSysView::GetActiveView();

  auto* group = activeView->SelectGroupAndPrimitive(activeView->GetCursorPosition());
  if (group != nullptr && activeView->EngagedPrimitive() != nullptr) {
    auto* primitive = activeView->EngagedPrimitive();

    if (primitive->Is(EoDb::kPolylinePrimitive)) {
      group->FindAndRemovePrim(primitive);

      auto* polyline = static_cast<EoDbPolyline*>(primitive);

      EoGePoint3dArray points;
      polyline->GetAllPoints(points);

      auto color = primitive->Color();
      auto lineTypeIndex = primitive->LineTypeIndex();

      for (auto i = 0; i < points.GetSize() - 1; i++) {
        group->AddTail(EoDbLine::CreateLine(points[i], points[i + 1])->WithProperties(color, lineTypeIndex));
      }
      if (polyline->IsLooped()) {
        group->AddTail(
            EoDbLine::CreateLine(points[points.GetUpperBound()], points[0])->WithProperties(color, lineTypeIndex));
      }
      delete primitive;
      ResetAllViews();
    } else if (primitive->Is(EoDb::kGroupReferencePrimitive)) {
      auto* blockReference = static_cast<EoDbBlockReference*>(primitive);

      EoDbBlock* block{};

      if (LookupBlock(blockReference->BlockName(), block) != 0) {
        group->FindAndRemovePrim(primitive);

        auto transformMatrix = blockReference->BuildTransformMatrix(block->BasePoint());

        EoDbGroup* pSegT = new EoDbGroup(*block);
        pSegT->Transform(transformMatrix);
        group->AddTail(pSegT);

        delete primitive;
        ResetAllViews();
      }
    }
  }
}

void AeSysDoc::OnEditSegToWork() {
  auto cursorPosition = app.GetCursorPosition();

  auto* layer = LayersSelUsingPoint(cursorPosition);
  if (layer == nullptr) { return; }

  if (layer->IsInternal()) {
    auto* group = layer->SelectGroupUsingPoint(cursorPosition);

    if (group == nullptr) { return; }
    layer->Remove(group);
    UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);
    AddWorkLayerGroup(group);
    UpdateAllViews(nullptr, EoDb::kGroup, group);
  }
}

void AeSysDoc::OnFileQuery() {
  auto cursorPosition = app.GetCursorPosition();

  auto* layer = LayersSelUsingPoint(cursorPosition);

  if (layer != nullptr) {
    CPoint currentPosition;
    ::GetCursorPos(&currentPosition);

    m_IdentifiedLayerName = layer->Name();

    int MenuResource = (layer->IsInternal()) ? IDR_LAYER : IDR_TRACING;

    auto layerTracingMenu = ::LoadMenuW(AeSys::GetInstance(), MAKEINTRESOURCE(MenuResource));
    auto* subMenu = CMenu::FromHandle(::GetSubMenu(layerTracingMenu, 0));

    subMenu->ModifyMenu(0, MF_BYPOSITION | MF_STRING, 0, m_IdentifiedLayerName);

    if (MenuResource == IDR_LAYER) {
      subMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_WORK),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsWork() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_ACTIVE),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsActive() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_STATIC),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsStatic() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_HIDDEN),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsOff() ? MF_CHECKED : MF_UNCHECKED)));
    } else {
      subMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_OPEN),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsOpened() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_MAP),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsMapped() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_VIEW),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsViewed() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_CLOAK),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsOff() ? MF_CHECKED : MF_UNCHECKED)));
    }
    subMenu->TrackPopupMenuEx(0, currentPosition.x, currentPosition.y, AfxGetMainWnd(), 0);
    ::DestroyMenu(layerTracingMenu);
  }
}

void AeSysDoc::OnLayerActive() {
  auto* layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (layer == nullptr) {
  } else {
    if (layer->IsWork()) {
      app.WarningMessageBox(IDS_MSG_LAYER_NO_ACTIVE, m_IdentifiedLayerName);
    } else {
      layer->MakeStateActive();
      UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);
    }
  }
}

void AeSysDoc::OnLayerStatic() {
  auto* layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (layer != nullptr) {
    if (layer->IsWork()) {
      app.WarningMessageBox(IDS_MSG_LAYER_NO_STATIC, m_IdentifiedLayerName);
    } else {
      layer->SetStateStatic();
      UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);
    }
  }
}

void AeSysDoc::OnLayerHidden() {
  auto* layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (layer != nullptr) {
    if (layer->IsWork()) {
      app.WarningMessageBox(IDS_MSG_LAYER_NO_HIDDEN, m_IdentifiedLayerName);
    } else {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      layer->SetStateOff();
    }
  }
}

void AeSysDoc::OnLayerMelt() { LayerMelt(m_IdentifiedLayerName); }

void AeSysDoc::OnLayerWork() {
  auto* layer = GetLayerTableLayer(m_IdentifiedLayerName);

  SetWorkLayer(layer);
}

void AeSysDoc::OnTracingMap() { TracingMap(m_IdentifiedLayerName); }

void AeSysDoc::OnTracingView() { TracingView(m_IdentifiedLayerName); }

void AeSysDoc::OnTracingCloak() {
  auto* layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (layer->IsOpened()) {
    CFile File(m_IdentifiedLayerName, CFile::modeWrite | CFile::modeCreate);
    if (File != CFile::hFileNull) {
      EoDbJobFile JobFile;
      JobFile.WriteHeader(File);
      JobFile.WriteLayer(File, layer);
      SetWorkLayer(GetLayerTableLayerAt(0));
      m_SaveAsType = EoDb::FileTypes::Unknown;
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      layer->SetStateOff();
    } else {
      app.WarningMessageBox(IDS_MSG_TRACING_WRITE_FAILURE, m_IdentifiedLayerName);
    }
  }
}

void AeSysDoc::OnTracingFuse() { TracingFuse(m_IdentifiedLayerName); }
void AeSysDoc::OnTracingOpen() { TracingOpen(m_IdentifiedLayerName); }

void AeSysDoc::OnLayersActiveAll() {
  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (!layer->IsWork()) { layer->MakeStateActive(); }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnLayersStaticAll() {
  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (!layer->IsWork()) { layer->SetStateStatic(); }
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
    wchar_t sBuf[16]{};

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

          DWORD sizeOfBuffer{};
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

      LPWSTR clipboardText = new wchar_t[GlobalSize(clipboardDataHandle)];

      LPCWSTR clipboardData = (LPCWSTR)GlobalLock(clipboardDataHandle);
      if (clipboardData != nullptr) {
        lstrcpyW(clipboardText, clipboardData);
      } else {
        clipboardText[0] = L'\0';
      }
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
    auto* layer = GetLayerTableLayerAt(i);
    if (layer->IsWork() || layer->IsActive()) { AddGroupsToTrap(layer); }
  }
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}

void AeSysDoc::OnTrapCommandsCompress() { CompressTrappedGroups(); }

void AeSysDoc::OnTrapCommandsExpand() {
  try {
    ExpandTrappedGroups();
  } catch (...) { ATLTRACE2(traceGeneral, 1, L"AeSysDoc::OnTrapCommandsExpand: Failed to expand trapped groups.\n"); }
}

void AeSysDoc::OnTrapCommandsInvert() {
  int layerTableSize = GetLayerTableSize();
  for (int i = 0; i < layerTableSize; i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (layer->IsWork() || layer->IsActive()) {
      auto LayerPosition = layer->GetHeadPosition();
      while (LayerPosition != 0) {
        auto* Group = layer->GetNext(LayerPosition);
        auto GroupPosition = FindTrappedGroup(Group);
        if (GroupPosition != nullptr) {
          m_trappedGroups.RemoveAt(GroupPosition);
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
  if (m_trappedGroups.GetCount() == 0) { return; }

  EoDbBlock* block{};
  auto blockTableSize = BlockTableSize();
  wchar_t name[16]{};

  do { swprintf_s(name, 16, L"_%.3i", ++blockTableSize); } while (LookupBlock(name, block));

  block = new EoDbBlock;

  auto position = GetFirstTrappedGroupPosition();
  while (position != nullptr) {
    auto* group = GetNextTrappedGroup(position);

    auto* newGroup = new EoDbGroup(*group);

    block->AddTail(newGroup);

    newGroup->RemoveAll();

    delete newGroup;
  }
  block->SetBasePoint(m_trapPivotPoint);
  InsertBlock(CString(name), block);
}

void AeSysDoc::OnTrapCommandsUnblock() { m_trappedGroups.BreakSegRefs(); }
void AeSysDoc::OnSetupPenColor() {
  EoDlgSetupColor Dialog;
  Dialog.m_ColorIndex = static_cast<std::uint16_t>(renderState.Color());

  if (Dialog.DoModal() == IDOK) {
    renderState.SetColor(nullptr, static_cast<std::int16_t>(Dialog.m_ColorIndex));

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

  EoDbLineType* currentLineType{};

  m_LineTypeTable.LookupUsingLegacyIndex(static_cast<std::uint16_t>(renderState.LineTypeIndex()), currentLineType);
  dialog.SetSelectedLineType(currentLineType);

  if (dialog.DoModal() != IDOK) { return; }

  EoDbLineType* selectedLineType = dialog.GetSelectedLineType();
  if (selectedLineType == nullptr) {
    ATLTRACE2(traceGeneral, 1, L"AeSysDoc::OnSetupLineType: No line type selected.\n");
    return;
  }
  renderState.SetLineType(nullptr, static_cast<std::int16_t>(selectedLineType->Index()));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Line);
}

void AeSysDoc::OnSetupFillHollow() { renderState.SetPolygonIntStyle(EoDb::PolygonStyle::Hollow); }
void AeSysDoc::OnSetupFillSolid() { renderState.SetPolygonIntStyle(EoDb::PolygonStyle::Solid); }
void AeSysDoc::OnSetupFillPattern() {}
void AeSysDoc::OnSetupFillHatch() {
  EoDlgSetupHatch Dialog;
  Dialog.m_HatchXScaleFactor = hatch::dXAxRefVecScal;
  Dialog.m_HatchYScaleFactor = hatch::dYAxRefVecScal;
  Dialog.m_HatchRotationAngle = Eo::RadianToDegree(hatch::dOffAng);

  if (Dialog.DoModal() == IDOK) {
    renderState.SetPolygonIntStyle(EoDb::PolygonStyle::Hatch);
    hatch::dXAxRefVecScal = std::max(0.01, Dialog.m_HatchXScaleFactor);
    hatch::dYAxRefVecScal = std::max(0.01, Dialog.m_HatchYScaleFactor);
    hatch::dOffAng = Eo::DegreeToRadian(Dialog.m_HatchRotationAngle);
  }
}

void AeSysDoc::OnSetupNote() {
  EoDbFontDefinition fontDefinition = renderState.FontDefinition();

  EoDlgSetupNote Dialog(&fontDefinition);

  auto characterCellDefinition = renderState.CharacterCellDefinition();

  Dialog.m_height = characterCellDefinition.Height();
  Dialog.m_rotationAngle = Eo::RadianToDegree(characterCellDefinition.RotationAngle());
  Dialog.m_expansionFactor = characterCellDefinition.ExpansionFactor();
  Dialog.m_slantAngle = Eo::RadianToDegree(characterCellDefinition.SlantAngle());

  if (Dialog.DoModal() == IDOK) {
    characterCellDefinition.SetHeight(Dialog.m_height);
    characterCellDefinition.SetRotationAngle(Eo::DegreeToRadian(Dialog.m_rotationAngle));
    characterCellDefinition.SetExpansionFactor(Dialog.m_expansionFactor);
    characterCellDefinition.SetSlantAngle(Eo::DegreeToRadian(Dialog.m_slantAngle));
    renderState.SetCharacterCellDefinition(characterCellDefinition);

    auto* activeView = AeSysView::GetActiveView();
    CDC* DeviceContext = (activeView == nullptr) ? nullptr : activeView->GetDC();

    renderState.SetFontDefinition(DeviceContext, fontDefinition);
  }
}

void AeSysDoc::OnSetupPointStyle() {
  CDlgSetPointStyle dlg;
  // Preload dialog from global state and document
  dlg.m_pointStyle = renderState.PointStyle();
  dlg.m_pointSize = GetPointSize();

  if (dlg.DoModal() == IDOK) {
    // Apply to global primitive state
    renderState.SetPointStyle(static_cast<short>(dlg.m_pointStyle));
    // Store into document
    SetPointSize(dlg.m_pointSize);

    UpdateAllViews(nullptr, 0L, nullptr);
  }
}

void AeSysDoc::OnToolsGroupBreak() {
  auto* activeView = AeSysView::GetActiveView();

  activeView->BreakAllPolylines();
  activeView->BreakAllSegRefs();
}

void AeSysDoc::OnToolsGroupDelete() {
  auto* activeView = AeSysView::GetActiveView();
  auto cursorPosition = activeView->GetCursorPosition();

  auto* Group = activeView->SelectGroupAndPrimitive(cursorPosition);

  if (Group != nullptr) {
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
    auto* primitive = activeView->EngagedPrimitive();

    if (primitive->PivotOnControlPoint(activeView, ptView)) {
      EoGePoint3d ptEng = activeView->DetPt();
      primitive->AddReportToMessageList(ptEng);
      activeView->SetCursorPosition(ptEng);
      return;
    }
    // Did not pivot on engaged primitive
    if (primitive->IsPointOnControlPoint(activeView, ptView)) { EoDbGroup::SetPrimitiveToIgnore(primitive); }
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
    EoGePoint3d pt = activeView->EngagedPrimitive()->GetControlPoint();
    activeView->SetCursorPosition(pt);
  }
}

void AeSysDoc::OnToolsPrimitiveDelete() {
  EoGePoint3d pt = app.GetCursorPosition();

  auto* activeView = AeSysView::GetActiveView();

  auto* group = activeView->SelectGroupAndPrimitive(pt);

  if (group != 0) {
    auto position = FindTrappedGroup(group);

    LPARAM lHint = (position != nullptr) ? EoDb::kGroupEraseSafeTrap : EoDb::kGroupEraseSafe;
    // erase entire group even if group has more than one primitive
    UpdateAllViews(nullptr, lHint, group);

    if (group->GetCount() > 1) {  // remove primitive from group
      auto* primitive = activeView->EngagedPrimitive();
      group->FindAndRemovePrim(primitive);
      lHint = (position != nullptr) ? EoDb::kGroupSafeTrap : EoDb::kGroupSafe;
      // display the group with the primitive removed
      UpdateAllViews(nullptr, lHint, group);
      // new group required to allow primitive to be placed into deleted group list
      group = new EoDbGroup(primitive);
    } else {  // deleting an entire group
      AnyLayerRemove(group);
      RemoveGroupFromAllViews(group);

      if (RemoveTrappedGroup(group) != 0) { activeView->UpdateStateInformation(AeSysView::TrapCount); }
    }
    DeletedGroupsAddTail(group);
    app.AddStringToMessageList(IDS_MSG_PRIM_ADDED_TO_DEL_GROUPS);
  }
}

void AeSysDoc::OnPrimModifyAttributes() {
  auto* activeView = AeSysView::GetActiveView();

  auto cursorPosition = activeView->GetCursorPosition();

  auto* Group = activeView->SelectGroupAndPrimitive(cursorPosition);

  if (Group != nullptr) {
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

  auto filter = App::LoadStringResource(IDS_OPENFILE_FILTER_TRACINGS);

  wchar_t fileBuffer[MAX_PATH]{};

  OPENFILENAME of{};
  of.lStructSize = sizeof(OPENFILENAME);
  of.hwndOwner = AfxGetMainWnd() ? AfxGetMainWnd()->GetSafeHwnd() : nullptr;
  of.hInstance = AeSys::GetInstance();
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
  str.Format(L"%d notes were removed resulting in %d empty groups which were also removed.", NumberOfEmptyNotes,
      NumberOfEmptyGroups);
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
  auto filter = App::LoadStringResource(IDS_OPENFILE_FILTER_PENCOLORS);
  auto title = App::LoadStringResource(IDS_OPENFILE_LOAD_PENCOLORS_TITLE);
  auto initialDir = App::PathFromCommandLine();

  CString file;
  file.GetBufferSetLength(MAX_PATH);

  OPENFILENAME of{};
  of.lStructSize = sizeof(OPENFILENAME);
  of.hInstance = AeSys::GetInstance();
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

  if (fl.Open(App::PathFromCommandLine() + L"\\Pens\\xlate.txt", CFile::modeRead | CFile::typeText)) {
    wchar_t pBuf[128]{};
    std::uint16_t wCols{};

    while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(wchar_t) - 1) != 0) { wCols++; }

    if (wCols > 0) {
      auto* pColNew = new std::int16_t[wCols];
      auto* pCol = new std::int16_t[wCols];

      std::uint16_t w{};

      fl.SeekToBegin();

      LPWSTR NextToken;
      while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(wchar_t) - 1) != 0) {
        NextToken = nullptr;
        pCol[w] = std::int16_t(_wtoi(wcstok_s(pBuf, L",", &NextToken)));
        pColNew[w++] = std::int16_t(_wtoi(wcstok_s(0, L"\n", &NextToken)));
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
    int iTokId{};

    try {
      lex::Parse(number);
      lex::EvalTokenStream(&iTokId, &lDef, &iTyp, value);

      if (iTyp != lex::ArchitecturalUnitsLengthToken && iTyp != lex::EngineeringUnitsLengthToken &&
          iTyp != lex::SimpleUnitsLengthToken) {
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

  auto cursorPosition = activeView->GetCursorPosition();

  if (activeView->SelectGroupAndPrimitive(cursorPosition)) {
    auto* primitive = activeView->EngagedPrimitive();

    CString String;

    if (primitive->Is(EoDb::kTextPrimitive)) {
      String = static_cast<EoDbText*>(primitive)->Text();
    } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
      String = static_cast<EoDbDimension*>(primitive)->Text();
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
      WinHelpW(AeSys::GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"DRAW"));
      break;

    case ID_MODE_EDIT: {
      WinHelpW(AeSys::GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"EDIT"));
      break;
    }
    case ID_MODE_TRAP:
    case ID_MODE_TRAPR: {
      WinHelpW(AeSys::GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"TRAP"));
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
    auto position = UniquePointPosition;
    EoGeUniquePoint* UniquePoint = GetNextUniquePoint(UniquePointPosition);
    if (point == UniquePoint->m_Point) {
      References = --(UniquePoint->m_References);

      if (References == 0) {
        RemoveUniquePointAt(position);
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