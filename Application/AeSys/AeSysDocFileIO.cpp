#include "Stdafx.h"

#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbDxfInterface.h"
#include "EoDbJobFile.h"
#include "EoDbLayer.h"
#include "EoDbLineType.h"
#include "EoDbPegFile.h"
#include "EoDbTracingFile.h"
#include "EoDbVPortTableEntry.h"
#include "EoDxfBase.h"
#include "EoDxfObjects.h"
#include "EoDxfRead.h"
#include "EoDxfWrite.h"
#include "EoOdaConverter.h"
#include "Resource.h"

namespace {
void ReportDxfImportStatistics(EoDbDxfInterface& dxfInterface) {
  app.AddStringToReportsList(std::format(L"3dFace: {}", dxfInterface.countOf3dFace));
  app.AddStringToReportsList(std::format(L"AcadProxyEntity: {}", dxfInterface.countOfAcadProxyEntity));
  app.AddStringToReportsList(std::format(L"Arc: {}", dxfInterface.countOfArc));
  app.AddStringToReportsList(std::format(L"AttDef: {}", dxfInterface.countOfAttDef));
  app.AddStringToReportsList(std::format(L"Attrib: {}", dxfInterface.countOfAttrib));
  app.AddStringToReportsList(std::format(L"Circle: {}", dxfInterface.countOfCircle));
  app.AddStringToReportsList(std::format(L"DimAlign: {}", dxfInterface.countOfDimAlign));
  app.AddStringToReportsList(std::format(L"DimLinear: {}", dxfInterface.countOfDimLinear));
  app.AddStringToReportsList(std::format(L"DimAngular: {}", dxfInterface.countOfDimAngular));
  app.AddStringToReportsList(std::format(L"DimAngular3P: {}", dxfInterface.countOfDimAngular3P));
  app.AddStringToReportsList(std::format(L"DimDiametric: {}", dxfInterface.countOfDimDiametric));
  app.AddStringToReportsList(std::format(L"DimOrdinate: {}", dxfInterface.countOfDimOrdinate));
  app.AddStringToReportsList(std::format(L"DimRadial: {}", dxfInterface.countOfDimRadial));
  app.AddStringToReportsList(std::format(L"Ellipse: {}", dxfInterface.countOfEllipse));
  app.AddStringToReportsList(std::format(L"Hatch: {}", dxfInterface.countOfHatch));
  app.AddStringToReportsList(std::format(L"Image: {}", dxfInterface.countOfImage));
  app.AddStringToReportsList(std::format(L"Insert: {}", dxfInterface.countOfInsert));
  app.AddStringToReportsList(std::format(L"Knot: {}", dxfInterface.countOfKnot));
  app.AddStringToReportsList(std::format(L"Line: {}", dxfInterface.countOfLine));
  app.AddStringToReportsList(std::format(L"LWPolyline: {}", dxfInterface.countOfLWPolyline));
  app.AddStringToReportsList(std::format(L"MText: {}", dxfInterface.countOfMText));
  app.AddStringToReportsList(std::format(L"Point: {}", dxfInterface.countOfPoint));
  app.AddStringToReportsList(std::format(L"PolygonMesh: {}", dxfInterface.countOfPolygonMesh));
  app.AddStringToReportsList(std::format(L"PolylineMesh: {}", dxfInterface.countOfPolylineMesh));
  app.AddStringToReportsList(std::format(L"2DPolyline: {}", dxfInterface.countOf2DPolyline));
  app.AddStringToReportsList(std::format(L"3DPolyline: {}", dxfInterface.countOf3DPolyline));
  app.AddStringToReportsList(std::format(L"Ray: {}", dxfInterface.countOfRay));
  app.AddStringToReportsList(std::format(L"Solid: {}", dxfInterface.countOfSolid));
  app.AddStringToReportsList(std::format(L"Spline: {}", dxfInterface.countOfSpline));
  app.AddStringToReportsList(std::format(L"Text: {}", dxfInterface.countOfText));
  app.AddStringToReportsList(std::format(L"Trace: {}", dxfInterface.countOfTrace));
  app.AddStringToReportsList(std::format(L"Viewport: {}", dxfInterface.countOfViewport));
}

}

/** @brief Maps a 1-based CFileDialog filter index from IDS_SAVEFILE_FILTER to a file type.
 *
 * IDS_SAVEFILE_FILTER order:
 *   1 = Drawing AE2026 (*.peg)
 *   2 = Drawing AE2011 (*.peg)
 *   3 = ASCII DXF (2018) (*.dxf)
 *   4 = ASCII DXF (2013) (*.dxf)
 *   5 = Binary DXF (2018) (*.dxf)
 *   6 = Binary DXF (2013) (*.dxf)
 *   7 = DWG 2018 (*.dwg)
 *   8 = DWG 2013 (*.dwg)
 */
static EoDb::FileTypes FileTypeFromFilterIndex(DWORD filterIndex) noexcept {
  switch (filterIndex) {
    case 1:
      return EoDb::FileTypes::Peg;
    case 2:
      return EoDb::FileTypes::Peg11;
    case 3:
    case 4:
      return EoDb::FileTypes::Dxf;
    case 5:
    case 6:
      return EoDb::FileTypes::Dxb;
    case 7:
    case 8:
      return EoDb::FileTypes::Dwg;
    default:
      return EoDb::FileTypes::Unknown;
  }
}

/** @brief Maps a 1-based CFileDialog filter index to a DXF version.
 *
 * Odd DXF/DWG indices (3, 5, 7) select 2018 (AC1032);
 * even indices (4, 6, 8) select 2013 (AC1027).
 * Non-DXF indices return AC1032 as a safe default.
 */
static EoDxf::Version DxfVersionFromFilterIndex(DWORD filterIndex) noexcept {
  switch (filterIndex) {
    case 4:
    case 6:
    case 8:
      return EoDxf::Version::AC1027;
    default:
      return EoDxf::Version::AC1032;
  }
}

/** @brief Maps the current m_saveAsType to a 1-based filter index for CFileDialog pre-selection.
 *
 * When multiple filter entries map to the same file type (e.g., DXF has both ASCII and Binary
 * variants), this returns the first matching entry for that type.
 */
static DWORD FilterIndexFromFileType(EoDb::FileTypes fileType) noexcept {
  switch (fileType) {
    case EoDb::FileTypes::Peg:
      return 1;
    case EoDb::FileTypes::Peg11:
      return 2;
    case EoDb::FileTypes::Dxf:
      return 3;
    case EoDb::FileTypes::Dxb:
      return 5;
    case EoDb::FileTypes::Dwg:
      return 7;
    default:
      return 1;
  }
}

/** @brief Returns the default file extension (without dot) for a given file type. */
static LPCWSTR DefaultExtensionForFileType(EoDb::FileTypes fileType) noexcept {
  switch (fileType) {
    case EoDb::FileTypes::Peg:
    case EoDb::FileTypes::Peg11:
      return L"peg";
    case EoDb::FileTypes::Dxf:
    case EoDb::FileTypes::Dxb:
      return L"dxf";
    case EoDb::FileTypes::Dwg:
      return L"dwg";
    default:
      return L"peg";
  }
}

BOOL AeSysDoc::DoSave(LPCWSTR pathName, BOOL replace) {
  CString selectedPath = pathName;

  // Determine whether a SaveAs dialog is needed:
  // - pathName is null/empty (first save or explicit SaveAs invocation)
  // - m_saveAsType is Unknown (document has no established save format)
  // - path extension doesn't match the target save type (e.g., DXF loaded with PEG save type)
  bool needsDialog = selectedPath.IsEmpty() || m_saveAsType == EoDb::FileTypes::Unknown;

  if (!needsDialog && !selectedPath.IsEmpty() && m_saveAsType != EoDb::FileTypes::Unknown) {
    const auto* expectedExtension = DefaultExtensionForFileType(m_saveAsType);
    std::filesystem::path currentPath(static_cast<LPCWSTR>(selectedPath));
    auto currentExtension = currentPath.extension().wstring();
    if (!currentExtension.empty() && currentExtension[0] == L'.') {
      currentExtension = currentExtension.substr(1);
    }
    needsDialog = (_wcsicmp(currentExtension.c_str(), expectedExtension) != 0);
  }

  if (needsDialog) {
    // Build a default file name from the document title when no path exists
    CString defaultFileName = m_strPathName;
    if (defaultFileName.IsEmpty()) {
      defaultFileName = m_strTitle;
      // Strip characters that are problematic in file names
      const int firstBadCharacterIndex = defaultFileName.FindOneOf(L" #%;/\\");
      if (firstBadCharacterIndex != -1) { defaultFileName.ReleaseBuffer(firstBadCharacterIndex); }
    }

    // Replace the extension to match the target save type so the dialog pre-fills correctly.
    // This ensures switching from DXF to PEG (or vice versa) shows the right filename.
    if (!defaultFileName.IsEmpty() && m_saveAsType != EoDb::FileTypes::Unknown) {
      std::filesystem::path defaultPath(static_cast<LPCWSTR>(defaultFileName));
      defaultPath.replace_extension(DefaultExtensionForFileType(m_saveAsType));
      defaultFileName = defaultPath.wstring().c_str();
    }

    // Extract the directory for the initial dialog location
    CString initialDirectory;
    if (!m_strPathName.IsEmpty()) {
      const int lastSeparatorIndex = m_strPathName.ReverseFind(L'\\');
      if (lastSeparatorIndex >= 0) { initialDirectory = m_strPathName.Left(lastSeparatorIndex); }
    }

    const auto filterString = App::LoadStringResource(IDS_SAVEFILE_FILTER);
    const auto initialFilterIndex = FilterIndexFromFileType(m_saveAsType);

    CFileDialog saveDialog(FALSE,  // FALSE = Save As dialog
        DefaultExtensionForFileType(m_saveAsType), defaultFileName,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, filterString, AfxGetMainWnd());

    saveDialog.m_ofn.nFilterIndex = initialFilterIndex;
    if (!initialDirectory.IsEmpty()) { saveDialog.m_ofn.lpstrInitialDir = initialDirectory; }

    if (saveDialog.DoModal() != IDOK) { return FALSE; }

    selectedPath = saveDialog.GetPathName();
    const auto chosenFilterIndex = saveDialog.m_ofn.nFilterIndex;

    // Update the save format from the user's filter selection
    const EoDb::FileTypes chosenFileType = FileTypeFromFilterIndex(chosenFilterIndex);
    if (chosenFileType != EoDb::FileTypes::Unknown) {
      m_saveAsType = chosenFileType;
      m_dxfVersion = DxfVersionFromFilterIndex(chosenFilterIndex);
    } else {
      // Fall back to extension-based detection
      m_saveAsType = App::FileTypeFromPath(selectedPath);
    }

    if (m_saveAsType == EoDb::FileTypes::Unknown) {
      ATLTRACE2(
          traceGeneral, 1, L"DoSave: unable to determine file type for '%s'\n", static_cast<LPCWSTR>(selectedPath));
      return FALSE;
    }
  }

  if (!OnSaveDocument(selectedPath)) {
    // If this was a new file (no prior path), clean up the partially created file
    if (pathName == nullptr || CString(pathName).IsEmpty()) {
      TRY { CFile::Remove(selectedPath); }
      CATCH_ALL(e) {
        ATLTRACE2(traceGeneral, 3, L"Warning: failed to delete file after failed SaveAs.\n");
        do { e->Delete(); } while (0);
      }
      END_CATCH_ALL
    }
    return FALSE;
  }

  if (replace) { SetPathName(selectedPath); }
  SetModifiedFlag(FALSE);
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
  lineType = new EoDbLineType(1, L"CONTINUOUS", L"Solid line", 0, nullptr);
  m_LineTypeTable.SetAt(L"CONTINUOUS", lineType);
  m_continuousLineType = lineType;

  constexpr EoDbLayer::State commonState =
      EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive;

  m_workLayer = new EoDbLayer(L"0", commonState);
  m_workLayer->SetColorIndex(7);  // ACI 7 — white on dark background, black on light background
  m_workLayer->SetLineType(lineType);
  AddLayerToSpace(m_workLayer, EoDxf::Space::ModelSpace);

  auto* paperSpaceLayer0 = new EoDbLayer(L"0", commonState);
  paperSpaceLayer0->SetColorIndex(7);
  paperSpaceLayer0->SetLineType(lineType);
  AddLayerToSpace(paperSpaceLayer0, EoDxf::Space::PaperSpace);

  // Create the default *ACTIVE viewport table entry if none exists
  if (m_vportTable.empty()) {
    m_vportTable.emplace_back();  // EoDbVPortTableEntry defaults to *ACTIVE with sensible values
  }

  // Create the default "Standard" text style if none exists
  if (m_textStyleTable.empty()) {
    m_textStyleTable.emplace_back();  // EoDbTextStyle defaults to "Standard" with DXF-compatible values
  }

  auto applicationPath = App::PathFromCommandLine();

  // @todo Peg uses index for line types, need to map names to indexes (index 0 to 41 have been hard coded in Peg).
  //       Need to ensure that line types loaded here match those indexes.
  // m_LineTypeTable.LoadLineTypesFromTxtFile(applicationPath + L"\\res\\LineTypes\\LineTypes.txt");
  // m_LineTypeTable.LoadLineTypesFromTxtFile(applicationPath + L"\\res\\LineTypes\\LineTypes-ACAD(scaled to
  // AeSys).txt"); m_LineTypeTable.LoadLineTypesFromTxtFile(applicationPath +
  // L"\\res\\LineTypes\\LineTypes-ISO128(scaled to AeSys).txt");
}

BOOL AeSysDoc::OnNewDocument() {
  if (!CDocument::OnNewDocument()) { return FALSE; }

  SetCommonTableEntries();

  // New documents always get a default "Layout1" paper-space layout
  if (m_layouts.empty()) {
    EoDxfLayout defaultLayout;
    defaultLayout.m_layoutName = L"Layout1";
    defaultLayout.m_tabOrder = 1;
    defaultLayout.m_blockRecordHandle = EoDxf::Handles::PaperSpaceBlockRecord;
    m_layouts.push_back(std::move(defaultLayout));
  }

  m_saveAsType = EoDb::FileTypes::Peg;
  SetWorkLayer(GetLayerTableLayerAt(0));
  InitializeGroupAndPrimitiveEdit();

  return TRUE;
}
BOOL AeSysDoc::OnOpenDocument(LPCWSTR pathName) {
  ATLTRACE2(traceGeneral, 3, L"AeSysDoc<%p>::OnOpenDocument(%s)\n", this, pathName);

  switch (App::FileTypeFromPath(pathName)) {
    case EoDb::FileTypes::Dwg: {
      auto tempDwgFolder = EoOdaConverter::CreateTempFolder(L"_dwg");
      if (tempDwgFolder.empty()) {
        app.AddStringToMessageList(L"Failed to create temporary DWG folder for conversion.");
        break;
      }
      auto tempDxfFolder = EoOdaConverter::CreateTempFolder(L"_dxf");
      if (tempDxfFolder.empty()) {
        app.AddStringToMessageList(L"Failed to create temporary DXF folder for conversion.");
        EoOdaConverter::DeleteTempFolder(tempDwgFolder);
        break;
      }
      auto dxfPath = EoOdaConverter::ConvertDwgToDxf(pathName, tempDwgFolder, tempDxfFolder);
      if (dxfPath.empty()) {
        EoOdaConverter::DeleteTempFolder(tempDwgFolder);
        EoOdaConverter::DeleteTempFolder(tempDxfFolder);
        app.AddStringToMessageList(L"ODA File Converter failed to convert DWG to DXF.");
        break;
      }
      {
        EoDbDxfInterface dxfInterface(this);
        EoDxfRead dxfReader(dxfPath.c_str());
        SetCommonTableEntries();
        bool success = dxfReader.Read(&dxfInterface, true);
        if (!success) {
          EoOdaConverter::DeleteTempFolder(tempDwgFolder);
          EoOdaConverter::DeleteTempFolder(tempDxfFolder);
          app.AddStringToReportsList(L"Error loading converted DXF file.");
          break;
        }
        app.AddStringToReportsList(std::format(L"DWG opened via ODAFileConverter: {}", pathName));
        ReportDxfImportStatistics(dxfInterface);
        ResolveDynamicBlockReferences();
      }
      EoOdaConverter::DeleteTempFolder(tempDwgFolder);
      EoOdaConverter::DeleteTempFolder(tempDxfFolder);
      m_originalDwgPath = pathName;
      m_saveAsType = EoDb::FileTypes::Dwg;
      SetWorkLayer(GetLayerTableLayerAt(0));
      InitializeGroupAndPrimitiveEdit();
    } break;
    case EoDb::FileTypes::Dxf:
    case EoDb::FileTypes::Dxb: {
      EoDbDxfInterface dxfInterface(this);
      EoDxfRead dxfReader(pathName);
      SetCommonTableEntries();
      bool success = dxfReader.Read(&dxfInterface, true);  // true for verbose output, false for silent
      if (success) {
        app.AddStringToReportsList(std::format(L"DXF file `{}` opened successfully", pathName));
        ReportDxfImportStatistics(dxfInterface);
        ResolveDynamicBlockReferences();
      } else {
        app.AddStringToReportsList(L"Error loading DXF file.");
      }
      m_saveAsType = EoDb::FileTypes::Peg;  // Set to Peg to allow saving back to Peg format after loading DXF/DXB, can
                                            // be changed to Dxf/Dxb if we want to save back in the same format
      SetWorkLayer(GetLayerTableLayerAt(0));
      InitializeGroupAndPrimitiveEdit();
    } break;
    case EoDb::FileTypes::Peg:
      try {
        EoDbPegFile file;
        if (file.Open(pathName, CFile::modeRead | CFile::shareDenyNone)) {
          SetCommonTableEntries();
          file.Load(this);

          // PEG V1 files have no layout data — create a default "Layout1" if none was loaded
          if (m_layouts.empty()) {
            EoDxfLayout defaultLayout;
            defaultLayout.m_layoutName = L"Layout1";
            defaultLayout.m_tabOrder = 1;
            defaultLayout.m_blockRecordHandle = EoDxf::Handles::PaperSpaceBlockRecord;
            m_layouts.push_back(std::move(defaultLayout));
          }

          m_saveAsType = EoDb::FileTypes::Peg;
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
      m_isTracingSession = true;
      SetCommonTableEntries();
      if (!EnterTracingEditMode(pathName)) {
        m_isTracingSession = false;
        return FALSE;
      }
      break;

    case EoDb::FileTypes::Unknown:
      // Let the base class handle it and probably fail
    default:
      return CDocument::OnOpenDocument(pathName);
  }
  return TRUE;
}

BOOL AeSysDoc::OnSaveDocument(LPCWSTR pathName) {
  BOOL returnStatus{};

  switch (m_saveAsType) {
    case EoDb::FileTypes::Peg:
    case EoDb::FileTypes::Peg11: {
      auto fileVersion =
          (m_saveAsType == EoDb::FileTypes::Peg) ? EoDb::PegFileVersion::AE2026 : EoDb::PegFileVersion::AE2011;
      WriteShadowFile(fileVersion);
      EoDbPegFile file;
      CFileException e;
      if (file.Open(pathName, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive, &e)) {
        file.Unload(this, fileVersion);
        returnStatus = TRUE;
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
        if (m_saveAsType == EoDb::FileTypes::Job) {
          EoDbJobFile JobFile;
          JobFile.WriteHeader(File);
          JobFile.WriteLayer(File, layer);
        } else if (m_saveAsType == EoDb::FileTypes::Tracing) {
          EoDbTracingFile TracingFile;
          TracingFile.WriteHeader(File);
          TracingFile.WriteLayer(File, layer);
        }
        app.AddStringToMessageList(IDS_MSG_TRACING_SAVE_SUCCESS, pathName);
        returnStatus = TRUE;
      }
      break;
    }
    case EoDb::FileTypes::Dwg: {
      // Export as DXF to a temp folder, then convert to DWG via ODAFileConverter
      auto tempFolder = EoOdaConverter::CreateTempFolder();
      if (tempFolder.empty()) {
        app.AddStringToMessageList(L"Failed to create temporary folder for DWG export.");
        break;
      }
      // Derive the .dxf file name from the target .dwg path
      std::filesystem::path dwgTarget{pathName};
      auto dxfFileName = dwgTarget.stem().wstring() + L".dxf";
      auto dxfPath = std::filesystem::path(tempFolder) / dxfFileName;

      {
        const bool isBinaryDxf = false;
        EoDbDxfInterface dxfInterface(this);
        EoDxfWrite dxfWriter(dxfPath.c_str());
        dxfInterface.SetDxfWriter(&dxfWriter);
        if (!dxfWriter.Write(&dxfInterface, m_dxfVersion, isBinaryDxf)) {
          EoOdaConverter::DeleteTempFolder(tempFolder);
          app.WarningMessageBox(IDS_MSG_DXF_SAVE_FAILURE, pathName);
          break;
        }
      }

      auto dwgOutputFolder = dwgTarget.parent_path().wstring();
      auto resultPath = EoOdaConverter::ConvertDxfToDwg(tempFolder, dxfFileName, dwgOutputFolder);
      EoOdaConverter::DeleteTempFolder(tempFolder);

      if (resultPath.empty()) {
        app.AddStringToMessageList(L"ODA File Converter failed to convert DXF to DWG.");
        break;
      }
      m_originalDwgPath = pathName;
      app.AddStringToMessageList(IDS_MSG_DWG_SAVE_SUCCESS, pathName);
      returnStatus = TRUE;
    } break;
    case EoDb::FileTypes::Dxf:
    case EoDb::FileTypes::Dxb: {
      const bool isBinaryDxf = (m_saveAsType == EoDb::FileTypes::Dxb);
      EoDbDxfInterface dxfInterface(this);
      EoDxfWrite dxfWriter(pathName);
      dxfInterface.SetDxfWriter(&dxfWriter);
      if (dxfWriter.Write(&dxfInterface, m_dxfVersion, isBinaryDxf)) {
        app.AddStringToMessageList(IDS_MSG_DXF_SAVE_SUCCESS, pathName);
        returnStatus = TRUE;
      } else {
        app.WarningMessageBox(IDS_MSG_DXF_SAVE_FAILURE, pathName);
      }
    } break;

    case EoDb::FileTypes::Unknown:
      break;

    default:
      app.WarningMessageBox(IDS_MSG_NOTHING_TO_SAVE);
  }
  return returnStatus;
}

/**
 * If the current file is a PEG file, creates a shadow copy of the file in the application's shadow folder.
 * The shadow file is named using the original file's name and last modified timestamp to ensure uniqueness.
 * If the shadow file cannot be created, displays a warning message to the user.
 *
 * @param fileVersion The version of the PEG file format to use when writing the shadow file.
 */
void AeSysDoc::WriteShadowFile(EoDb::PegFileVersion fileVersion) {
  if (m_saveAsType != EoDb::FileTypes::Peg && m_saveAsType != EoDb::FileTypes::Peg11) { return; }

  CString shadowFilePath(app.ShadowFolderPath());
  shadowFilePath += GetTitle();
  const int dotPosition = shadowFilePath.Find('.');
  if (dotPosition <= 0) { return; }

  CFileStatus fileStatus;
  CFile::GetStatus(GetPathName(), fileStatus);

  shadowFilePath.Truncate(dotPosition);
  shadowFilePath += fileStatus.m_mtime.Format(L"_%Y%m%d%H%M");
  shadowFilePath += L".peg";

  CFileException e;
  EoDbPegFile fp;
  if (!fp.Open(shadowFilePath, CFile::modeWrite, &e)) {
    fp.Open(shadowFilePath, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive, &e);
    fp.Unload(this, fileVersion);
    app.WarningMessageBox(IDS_MSG_FILE_SHADOWED_AS, shadowFilePath);
    return;
  }
  app.WarningMessageBox(IDS_MSG_SHADOW_FILE_CREATE_FAILURE);
}

void AeSysDoc::ResolveDynamicBlockReferences() {
  const auto& unsupportedObjects = m_unsupportedObjects;
  if (unsupportedObjects.empty()) { return; }

  // Phase 1: Build handle → unsupported object index.
  // Handles in unsupported objects are stored as std::wstring hex values (group code 5 falls in the
  // 0–9 wstring range in CreateRawRecordValue).
  std::unordered_map<std::uint64_t, const EoDxfUnsupportedObject*> objectsByHandle;
  objectsByHandle.reserve(unsupportedObjects.size());
  for (const auto& obj : unsupportedObjects) {
    for (const auto& value : obj.m_values) {
      if (value.Code() == 5) {
        if (const auto* hexString = value.GetIf<std::wstring>()) {
          auto handle = std::wcstoull(hexString->c_str(), nullptr, 16);
          if (handle != 0) { objectsByHandle[handle] = &obj; }
        }
        break;
      }
    }
  }
  if (objectsByHandle.empty()) { return; }

  // Phase 2: Build block BLOCK_RECORD handle → block name map from the block table.
  std::unordered_map<std::uint64_t, CString> blockNameByHandle;
  {
    POSITION pos = m_BlocksTable.GetStartPosition();
    CString blockName;
    EoDbBlock* block = nullptr;
    while (pos != nullptr) {
      m_BlocksTable.GetNextAssoc(pos, blockName, block);
      if (block != nullptr && block->Handle() != 0) { blockNameByHandle[block->Handle()] = blockName; }
    }
  }

  // Phase 3: Lambda to resolve a single INSERT's extension dictionary chain.
  // Chain: INSERT.extensionDictionaryHandle → DICTIONARY → scan entries for
  // ACDB_BLOCKREPRESENTATION_DATA → group code 340 → anonymous block BLOCK_RECORD handle → block name.
  auto resolveInsert = [&](EoDbBlockReference* insert) -> bool {
    const auto extDictHandle = insert->ExtensionDictionaryHandle();
    if (extDictHandle == 0) { return false; }

    // Step A: Find the DICTIONARY object at extDictHandle.
    const auto dictIt = objectsByHandle.find(extDictHandle);
    if (dictIt == objectsByHandle.end()) { return false; }
    const auto* dictObject = dictIt->second;
    if (dictObject->m_objectType != L"DICTIONARY") { return false; }

    // Step B: Scan DICTIONARY entries (code 3 = key, code 350/360 = value handle).
    // Find entries pointing to ACDB_BLOCKREPRESENTATION_DATA objects.
    for (const auto& value : dictObject->m_values) {
      if (value.Code() != 350 && value.Code() != 360) { continue; }
      const auto* entryHexString = value.GetIf<std::wstring>();
      if (entryHexString == nullptr) { continue; }

      auto entryHandle = std::wcstoull(entryHexString->c_str(), nullptr, 16);
      if (entryHandle == 0) { continue; }

      // Step C: Follow to the referenced object.
      auto entryIt = objectsByHandle.find(entryHandle);
      if (entryIt == objectsByHandle.end()) { continue; }
      const auto* entryObject = entryIt->second;

      // Accept ACDB_BLOCKREPRESENTATION_DATA (the standard dynamic block data object).
      if (entryObject->m_objectType != L"ACDB_BLOCKREPRESENTATION_DATA") { continue; }

      // Step D: Find group code 340 (block table record handle) in this object.
      for (const auto& entryValue : entryObject->m_values) {
        if (entryValue.Code() != 340) { continue; }
        const auto* blockHandleHex = entryValue.GetIf<std::wstring>();
        if (blockHandleHex == nullptr) { continue; }

        auto blockRecordHandle = std::wcstoull(blockHandleHex->c_str(), nullptr, 16);
        if (blockRecordHandle == 0) { continue; }

        // Step E: Lookup block name from the handle → name map.
        auto blockIt = blockNameByHandle.find(blockRecordHandle);
        if (blockIt == blockNameByHandle.end()) { continue; }

        const CString& resolvedName = blockIt->second;
        if (resolvedName != insert->BlockName()) {
          insert->SetName(resolvedName);
          return true;
        }
        return false;
      }
    }
    return false;
  };

  // Phase 4: Lambda to process all INSERTs in a CLayers collection.
  int resolvedCount = 0;
  auto processLayers = [&](CLayers& layers) {
    for (INT_PTR i = 0; i < layers.GetSize(); i++) {
      auto* layer = layers[i];
      if (layer == nullptr) { continue; }
      auto groupPosition = layer->GetHeadPosition();
      while (groupPosition != nullptr) {
        auto* const group = layer->GetNext(groupPosition);
        if (group == nullptr) { continue; }
        auto primPosition = group->GetHeadPosition();
        while (primPosition != nullptr) {
          auto* const primitive = group->GetNext(primPosition);
          if (primitive == nullptr) { continue; }
          if (!primitive->Is(EoDb::kGroupReferencePrimitive)) { continue; }
          auto* insert = static_cast<EoDbBlockReference*>(primitive);
          if (insert->ExtensionDictionaryHandle() == 0) { continue; }
          if (resolveInsert(insert)) { resolvedCount++; }
        }
      }
    }
  };

  // Phase 5: Process model space and all paper-space layouts.
  processLayers(m_modelSpaceLayers);
  for (auto& [layoutHandle, layers] : m_paperSpaceLayoutLayers) { processLayers(layers); }

  if (resolvedCount > 0) {
    CString message;
    message.Format(L"Resolved %d dynamic block reference(s) to anonymous block definitions.", resolvedCount);
    app.AddStringToMessageList(message);
  }
}
