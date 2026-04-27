#include "Stdafx.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "ChildFrm.h"
#include "Eo.h"
#include "EoApOptions.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"
#include "EoDlgModeLetter.h"
#include "EoDlgModeRevise.h"
#include "EoGePoint3d.h"
#include "EoGsRenderState.h"
#include "Hatch.h"
#include "Lex.h"
#include "MainFrm.h"
#include "Resource.h"

#ifdef USING_DDE
#include "Dde.h"
#include "ddeGItms.h"
namespace dde {
void RegisterAeSysTopics();
}
#endif

ATOM WINAPI RegisterKeyPlanWindowClass(HINSTANCE instance);
ATOM WINAPI RegisterPreviewWindowClass(HINSTANCE instance);

double penWidthsTable[numberOfPenWidths] =
    {0.0, 0.0075, 0.015, 0.02, 0.03, 0.0075, 0.015, 0.0225, 0.03, 0.0075, 0.015, 0.0225, 0.03, 0.0075, 0.015, 0.0225};

namespace {
/** @brief Converts a multi-byte (UTF-8) string to a wide-character string.
 * @param multiByte The multi-byte (UTF-8) string to convert.
 * @return The converted wide-character string.
 */
std::wstring MultiByteToWString(const char* multiByte) {
  if (!multiByte) { return {L""}; }
  const int size = MultiByteToWideChar(CP_UTF8, 0, multiByte, -1, nullptr, 0);
  if (size == 0) { return {L""}; }
  std::wstring string;
  string.resize(static_cast<size_t>(size) - 1);
  MultiByteToWideChar(CP_UTF8, 0, multiByte, -1, &string[0], size - 1);
  return string;
}

constexpr double defaultPenWidths[numberOfPenWidths] =
    {0.0, 0.0075, 0.015, 0.02, 0.03, 0.0075, 0.015, 0.0225, 0.03, 0.0075, 0.015, 0.0225, 0.03, 0.0075, 0.015, 0.0225};
}  // namespace

static void ResetPenWidthsToDefault() {
  std::copy(std::begin(defaultPenWidths), std::end(defaultPenWidths), penWidthsTable);
}

namespace hatch {
double dXAxRefVecScal;
double dYAxRefVecScal;
double dOffAng;
int tableOffset[maxPatterns]{};
float tableValue[maxTableValues]{};
}  // namespace hatch

auto* pColTbl = Eo::ColorPalette;

EoGsRenderState Gs::renderState;

BEGIN_MESSAGE_MAP(AeSys, CWinAppEx)
ON_COMMAND(ID_APP_ABOUT, &AeSys::OnAppAbout)
// Standard file based document commands
ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
ON_COMMAND(ID_FILE_OPEN, &AeSys::OnFileOpen)
ON_COMMAND(ID_FILE_SAVE_ALL, &AeSys::OnFileSaveAll)

ON_COMMAND(ID_EDIT_CF_GROUPS, &AeSys::OnEditCfGroups)
ON_COMMAND(ID_EDIT_CF_IMAGE, &AeSys::OnEditCfImage)
ON_COMMAND(ID_EDIT_CF_TEXT, &AeSys::OnEditCfText)
ON_COMMAND(ID_FILE_RUN, &AeSys::OnFileRun)
ON_COMMAND(ID_HELP_CONTENTS, &AeSys::OnHelpContents)

ON_COMMAND(ID_MODE_LETTER, &AeSys::OnModeLetter)
ON_COMMAND(ID_MODE_REVISE, &AeSys::OnModeRevise)

ON_COMMAND(ID_TRAPCOMMANDS_HIGHLIGHT, &AeSys::OnTrapCommandsHighlight)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_UPDATE_COMMAND_UI(ID_EDIT_CF_GROUPS, &AeSys::OnUpdateEditCfGroups)
ON_UPDATE_COMMAND_UI(ID_EDIT_CF_IMAGE, &AeSys::OnUpdateEditCfImage)
ON_UPDATE_COMMAND_UI(ID_EDIT_CF_TEXT, &AeSys::OnUpdateEditCfText)
ON_UPDATE_COMMAND_UI(ID_TRAPCOMMANDS_ADDGROUPS, &AeSys::OnUpdateTrapcommandsAddgroups)
ON_UPDATE_COMMAND_UI(ID_TRAPCOMMANDS_HIGHLIGHT, &AeSys::OnUpdateTrapcommandsHighlight)
#pragma warning(pop)
END_MESSAGE_MAP()

AeSys app;

BOOL AeSys::InitInstance() {
  if (!AfxOleInit()) {
    AfxMessageBox(IDP_OLE_INIT_FAILED);
    return FALSE;
  }
  AfxEnableControlContainer();
  EnableHtmlHelp();

#ifdef _DEBUG
  CTrace::SetLevel(2);
#endif
  SetRegistryKey(L"Engineers Office");
  LoadStdProfileSettings();
  SetRegistryBase(L"Settings");

  m_Options.Load();

  // Apply persisted view background preference to the global state
  Eo::activeViewBackground = m_Options.m_viewBackground;
  Eo::SyncViewBackgroundColor();
  Eo::SyncAci7WithBackground();

  // Initialize managers
  InitContextMenuManager();
  InitKeyboardManager();
  InitTooltipManager();

  CMFCToolTipInfo params;
  params.m_bVislManagerTheme = TRUE;
  GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS(CMFCToolTipCtrl), &params);

  EnableUserTools(
      ID_TOOLS_ENTRY, ID_USER_TOOL1, ID_USER_TOOL10, RUNTIME_CLASS(CUserTool), IDR_MENU_ARGS, IDR_MENU_DIRS);

  // Initialize common controls required to enable visual styles.
  constexpr INITCOMMONCONTROLSEX initCommonControls{sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES};
  InitCommonControlsEx(&initCommonControls);

  CMFCButton::EnableWindowsTheming();

  CWinAppEx::InitInstance();

  // Register document templates

  auto* documentClass = RUNTIME_CLASS(AeSysDoc);
  auto* frameClass = RUNTIME_CLASS(CChildFrame);
  auto* viewClass = RUNTIME_CLASS(AeSysView);

  m_pegDocumentTemplate = new CMultiDocTemplate(IDR_PEGTYPE, documentClass, frameClass, viewClass);
  AddDocTemplate(m_pegDocumentTemplate);

  // Create main MDI Frame window
  auto mainFrame = new CMainFrame;
  if (!mainFrame || !mainFrame->LoadFrame(IDR_MAINFRAME)) {
    delete mainFrame;
    return FALSE;
  }
  m_pMainWnd = mainFrame;  // Set CWinApp::m_pMainWnd to the main frame window.
  mainFrame->DragAcceptFiles();

  CDC* deviceContext = mainFrame->GetDC();
  m_DeviceWidthInPixels = static_cast<double>(deviceContext->GetDeviceCaps(HORZRES));
  m_DeviceHeightInPixels = static_cast<double>(deviceContext->GetDeviceCaps(VERTRES));
  m_DeviceWidthInMillimeters = static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE));
  m_DeviceHeightInMillimeters = static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE));
  InitGbls(deviceContext);
  mainFrame->ReleaseDC(deviceContext);

  // Parse command line and process shell commands
  CCommandLineInfo commandLineInfo;
  ParseCommandLine(commandLineInfo);

  if (commandLineInfo.m_nShellCommand == CCommandLineInfo::FileNew) {
    if (!mainFrame->LoadMDIState(GetRegSectionPath())) { m_pegDocumentTemplate->OpenDocumentFile(nullptr); }
  } else {
    if (!ProcessShellCommand(commandLineInfo)) { return FALSE; }
  }
  // Safety net: ensure all toolbars are visible after restoring docking state.
  // A stale registry blob (DPI change, structural change) can leave toolbars hidden.
  mainFrame->EnsureToolbarsVisible();
  m_MainFrameMenuHandle = LoadMenuW(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));

  if (!RegisterKeyPlanWindowClass(GetInstance())) { return FALSE; }
  if (!RegisterPreviewWindowClass(GetInstance())) { return FALSE; }

  if (SetShadowFolderPath(L"AeSys Shadow Folder") != 0) {
    AddStringToMessageList(L"Failed to set shadow folder path.");
  }

  // Create Direct2D and DirectWrite factory singletons (Phase 6)
  HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
  if (FAILED(hr)) { ATLTRACE2(traceGeneral, 0, L"D2D1CreateFactory failed: 0x%08X\n", hr); }

  hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
      __uuidof(IDWriteFactory),
      reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));
  if (FAILED(hr)) { ATLTRACE2(traceGeneral, 0, L"DWriteCreateFactory failed: 0x%08X\n", hr); }

  const CString resourceFolder = App::ResourceFolderPath();
  LoadSimplexStrokeFont(resourceFolder + Eo::defaultStrokeFont);
  LoadHatchesFromFile(resourceFolder + L"Hatches\\DefaultSet.txt");
  // LoadPenColorsFromFile(ResourceFolder + L"Pens\\Colors\\Default.txt"));

#ifdef USING_DDE
  // Initialize DDE and register AeSys-specific topics, items, and commands
  dde::RegisterAeSysTopics();
#endif

  // This is the private data format used to pass EoGroups from one instance to another
  m_ClipboardFormatIdentifierForEoGroups = RegisterClipboardFormatW(L"EoGroups");

  GetMainWindow()->ShowWindow(m_nCmdShow);
  GetMainWindow()->UpdateWindow();

  // Testing code here

  return TRUE;
}

int AeSys::ExitInstance() {
  m_Options.Save();

#ifdef USING_DDE
  dde::Uninitialize();
#endif

  ReleaseSimplexStrokeFont();

  return CWinAppEx::ExitInstance();
}

/// <remarks> Processing occurs immediately before the framework loads the application state from the registry.
/// </remarks>
void AeSys::PreLoadState() {
  GetContextMenuManager()->AddMenu(L"My menu", IDR_CONTEXT_MENU);

  // @todo add another context menus here
}

int AeSys::ConfirmMessageBox(UINT stringResourceIdentifier, const CString& string) {
  const auto formatSpecification = App::LoadStringResource(stringResourceIdentifier);

  CString formattedResourceString;
  formattedResourceString.Format(formatSpecification, string.GetString());

  int nextToken{0};
  const CString message = formattedResourceString.Tokenize(L"\t", nextToken);
  const CString caption = formattedResourceString.Tokenize(L"\n", nextToken);

  return (MessageBoxW(nullptr, message, caption, MB_ICONINFORMATION | MB_YESNOCANCEL | MB_DEFBUTTON2));
}

void AeSys::AddStringToMessageList(const std::wstring& message) {
  auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());

  mainFrame->GetOutputPane().AddStringToMessageList(message);
  if (!mainFrame->GetOutputPane().IsWindowVisible()) { mainFrame->SetPaneText(0, message.c_str()); }
}

void AeSys::AddStringToMessageList(const wchar_t* message) {
  auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());

  mainFrame->GetOutputPane().AddStringToMessageList(std::wstring(message));
  if (!mainFrame->GetOutputPane().IsWindowVisible()) { mainFrame->SetPaneText(0, message); }
}

void AeSys::AddStringToReportsList(const std::wstring& message) {
  auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());

  mainFrame->GetOutputPane().AddStringToReportsList(message);
  if (!mainFrame->GetOutputPane().IsWindowVisible()) { mainFrame->SetPaneText(0, message.c_str()); }
}

void AeSys::AddModeInformationToMessageList() {
  auto resourceString = App::LoadStringResource(static_cast<UINT>(m_CurrentMode));
  int nextToken{0};
  resourceString = resourceString.Tokenize(L"\n", nextToken);
  AddStringToMessageList(resourceString);
}

void AeSys::AddStringToMessageList(UINT stringResourceIdentifier) {
  const auto resourceString = App::LoadStringResource(stringResourceIdentifier);

  AddStringToMessageList(resourceString);
}

void AeSys::AddStringToMessageList(UINT stringResourceIdentifier, const CString& string) {
  const auto formatSpecification = App::LoadStringResource(stringResourceIdentifier);

  CString formattedResourceString;
  formattedResourceString.Format(formatSpecification, string.GetString());

  AddStringToMessageList(formattedResourceString);
}

void AeSys::WarningMessageBox(UINT stringResourceIdentifier) {
  const auto resourceString = App::LoadStringResource(stringResourceIdentifier);

  int nextToken{0};
  const CString message = resourceString.Tokenize(L"\t", nextToken);
  const CString caption = resourceString.Tokenize(L"\n", nextToken);
  MessageBoxW(nullptr, message, caption, MB_ICONWARNING | MB_OK);
}

void AeSys::WarningMessageBox(UINT stringResourceIdentifier, const CString& string) {
  const auto formatSpecification = App::LoadStringResource(stringResourceIdentifier);

  CString formattedResourceString;
  formattedResourceString.Format(formatSpecification, string.GetString());

  int nextToken{0};
  const CString message = formattedResourceString.Tokenize(L"\t", nextToken);
  const CString caption = formattedResourceString.Tokenize(L"\n", nextToken);

  MessageBoxW(nullptr, message, caption, MB_ICONWARNING | MB_OK);
}

void AeSys::UpdateMDITabs(BOOL resetMDIChild) {
  static_cast<CMainFrame*>(AfxGetMainWnd())->UpdateMDITabs(resetMDIChild);
}

void AeSys::OnTrapCommandsHighlight() {
  m_TrapHighlighted = !m_TrapHighlighted;
  // LPARAM lHint = m_TrapHighlighted ? EoDb::kGroupsSafeTrap : EoDb::kGroupsSafe;
  // UpdateAllViews(nullptr, lHint, &m_TrappedGroupList);
}

void AeSys::OnEditCfGroups() {
  m_ClipboardDataEoGroups = !m_ClipboardDataEoGroups;
}
void AeSys::OnEditCfImage() {
  m_ClipboardDataImage = !m_ClipboardDataImage;
}
void AeSys::OnEditCfText() {
  m_ClipboardDataText = !m_ClipboardDataText;
}

void AeSys::OnModeLetter() {
  EoDlgModeLetter dialog;
  dialog.DoModal();
}

void AeSys::OnModeRevise() {
  EoDlgModeRevise dialog;
  dialog.DoModal();
}

void AeSys::OnFileRun() {
  auto filter = App::LoadStringResource(IDS_OPENFILE_FILTER_APPS);

  CFileDialog dlg(TRUE, L"exe", L"*.exe", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, filter);
  dlg.m_ofn.lpstrTitle = L"Run Application";

  if (dlg.DoModal() == IDOK) {
    CString strFile = dlg.GetFileName();
    // Note: use of winexec should be replaced with createprocess
    // WinExec(strFile, SW_SHOW);
  }
}

void AeSys::OnHelpContents() {
  ::WinHelpW(GetSafeHwnd(), L"peg.hlp", HELP_CONTENTS, 0L);
}

void AeSys::LoadPenWidthsFromFile(const CString& fileName) {
  CStdioFile file;

  if (!file.Open(fileName, CFile::modeRead | CFile::typeText)) { return; }

  constexpr size_t bufferSize{128};
  wchar_t inputLine[bufferSize];

  while (file.ReadString(inputLine, bufferSize - 1) != nullptr) {
    if (inputLine[0] == L'\0' || inputLine[0] == L'#' || inputLine[0] == L';') { continue; }

    wchar_t* context{};
    const wchar_t* const penIndexText = wcstok_s(inputLine, L"=", &context);
    if (penIndexText == nullptr) { continue; }

    wchar_t* penIndexEnd{};
    const unsigned long penIndex = std::wcstoul(penIndexText, &penIndexEnd, 10);
    if (penIndexEnd == penIndexText) { continue; }

    const wchar_t* const penWidthText = wcstok_s(nullptr, L",\n", &context);
    if (penWidthText == nullptr) { continue; }

    wchar_t* penValueEnd{};
    const double penWidth = std::wcstod(penWidthText, &penValueEnd);
    if (penValueEnd == penWidthText) { continue; }

    if (static_cast<size_t>(penIndex) < numberOfPenWidths) { penWidthsTable[penIndex] = penWidth; }
  }
}

int AeSys::SetShadowFolderPath(const CString& folder) {
  wchar_t path[MAX_PATH]{};

  if (SHGetSpecialFolderPathW(GetSafeHwnd(), path, CSIDL_PERSONAL, TRUE)) {
    m_ShadowFolderPath = path;
  } else {
    m_ShadowFolderPath.Empty();
  }
  m_ShadowFolderPath += L"\\" + folder + L"\\";

  return (_wmkdir(m_ShadowFolderPath));
}

EoGePoint3d AeSys::GetCursorPosition() {
  auto* activeView = AeSysView::GetActiveView();
  return (activeView == nullptr) ? EoGePoint3d::kOrigin : activeView->GetCursorPosition();
}

void AeSys::LoadHatchesFromFile(const CString& fileName) {
  CFileException e;
  CStdioFile fl;

  if (!fl.Open(fileName, CFile::modeRead | CFile::typeText, &e)) { return; }

  wchar_t line[256]{};
  double dTotStrsLen{};
  int iNmbEnts{};
  int iNmbStrsId{};

  const wchar_t szValDel[] = L",\0";
  int iHatId{};
  int iNmbHatLns{};
  int iTblId{1};  // Reserve position 0 as uninitialized sentinel for tableOffset

  while (fl.ReadString(line, sizeof(line) / sizeof(wchar_t) - 1) != nullptr) {
    if (*line == '!') {
      // New Hatch index
      if (iHatId != 0) { hatch::tableValue[hatch::tableOffset[iHatId]] = static_cast<float>(iNmbHatLns); }
      ++iHatId;
      if (iHatId >= hatch::maxPatterns) {
        ATLTRACE(traceGeneral, 0, L"LoadHatchesFromFile: pattern count exceeds maxPatterns (%d)\n", hatch::maxPatterns);
        break;
      }
      hatch::tableOffset[iHatId] = iTblId++;
      iNmbHatLns = 0;
    } else {
      iNmbStrsId = iTblId;
      iTblId += 2;
      iNmbEnts = 0;
      dTotStrsLen = 0.;
      wchar_t* nextToken{};
      wchar_t* pTok = wcstok_s(line, szValDel, &nextToken);
      while (pTok != nullptr) {
        if (iTblId >= hatch::maxTableValues) {
          ATLTRACE(traceGeneral, 0, L"LoadHatchesFromFile: table value overflow at pattern %d\n", iHatId);
          break;
        }
        const volatile double tempValue = _wtof(pTok);
        hatch::tableValue[iTblId] = static_cast<float>(tempValue);
        iNmbEnts++;
        if (iNmbEnts >= 6) { dTotStrsLen = dTotStrsLen + hatch::tableValue[iTblId]; }
        iTblId++;
        pTok = wcstok_s(nullptr, szValDel, &nextToken);
      }
      hatch::tableValue[iNmbStrsId++] = static_cast<float>(iNmbEnts - 5);
      hatch::tableValue[iNmbStrsId] = static_cast<float>(dTotStrsLen);
      iNmbHatLns++;
    }
  }
}

EoGePoint3d AeSys::HomePointGet(int i) const noexcept {
  if (i >= 0 && i < 9) { return (m_HomePoints[i]); }
  return (EoGePoint3d::kOrigin);
}

void AeSys::HomePointSave(int i, const EoGePoint3d& pt) noexcept {
  if (i >= 0 && i < 9) { m_HomePoints[i] = pt; }
}

// Initializes all peg global sections to their default (startup) values.
void AeSys::InitGbls(CDC* deviceContext) {
  Gs::renderState.SetPolygonIntStyle(EoDb::PolygonStyle::Hollow);

  Gs::renderState.SetPolygonIntStyleId(1);

  hatch::dXAxRefVecScal = 0.1;
  hatch::dYAxRefVecScal = 0.1;
  hatch::dOffAng = 0.0;

  const EoDbCharacterCellDefinition characterCellDefinition{};
  Gs::renderState.SetCharacterCellDefinition(characterCellDefinition);

  const EoDbFontDefinition fontDefinition;
  Gs::renderState.SetFontDefinition(deviceContext, fontDefinition);

  SetUnits(Eo::Units::Inches);
  SetArchitecturalUnitsFractionPrecision(8);
  SetDimensionLength(0.125);
  SetDimensionAngle(45.0);

  m_TrapHighlighted = true;
  m_TrapHighlightColor = 15;

  Gs::renderState.SetPen(nullptr, deviceContext, Eo::defaultColor, 1);
  Gs::renderState.SetPointStyle(0);
}

void AeSys::EditColorPalette() {
  CHOOSECOLOR cc{};
  cc.lStructSize = sizeof(CHOOSECOLOR);
  cc.rgbResult = Eo::ColorPalette[Gs::renderState.Color()];
  cc.lpCustColors = Eo::ColorPalette;
  cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_SOLIDCOLOR;
  ::ChooseColor(&cc);
  cc.rgbResult = Eo::GrayPalette[Gs::renderState.Color()];
  cc.lpCustColors = Eo::GrayPalette;
  ::ChooseColor(&cc);

  MessageBoxW(nullptr,
      L"The background color is no longer associated with the pen Color Palette.",
      L"Deprecation Notice",
      MB_OK | MB_ICONINFORMATION);

  AeSysDoc::GetDoc()->UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSys::LoadPenColorsFromFile(const CString& fileName) {
  CStdioFile fl;

  if (fl.Open(fileName, CFile::modeRead | CFile::typeText)) {
    wchar_t pBuf[128]{};

    while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(wchar_t) - 1) != nullptr && _tcsnicmp(pBuf, L"<Colors>", 8) != 0);

    while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(wchar_t) - 1) != nullptr && *pBuf != '<') {
      wchar_t* nextToken = nullptr;
      const wchar_t* index = wcstok_s(pBuf, L"=", &nextToken);
      const auto colorIndex = _wtoi(index);

      if (colorIndex < 0 || colorIndex >= static_cast<int>(std::size(Eo::ColorPalette))) { continue; }
      const wchar_t* redColorPalette = wcstok_s(nullptr, L",", &nextToken);
      const wchar_t* greenColorPalette = wcstok_s(nullptr, L",", &nextToken);
      const wchar_t* blueColorPalette = wcstok_s(nullptr, L",", &nextToken);
      Eo::ColorPalette[colorIndex] = RGB(_wtoi(redColorPalette), _wtoi(greenColorPalette), _wtoi(blueColorPalette));

      if (colorIndex >= static_cast<int>(std::size(Eo::GrayPalette))) { continue; }
      const wchar_t* redGrayPalette = wcstok_s(nullptr, L",", &nextToken);
      const wchar_t* greenGrayPalette = wcstok_s(nullptr, L",", &nextToken);
      const wchar_t* blueGrayPalette = wcstok_s(nullptr, L"\n", &nextToken);
      Eo::GrayPalette[colorIndex] = RGB(_wtoi(redGrayPalette), _wtoi(greenGrayPalette), _wtoi(blueGrayPalette));
    }
  }
}

void AeSys::LoadModeResources(int mode, AeSysView* targetView) {
  BuildModifiedAcceleratorTable();

  m_CurrentMode = mode;
  AddModeInformationToMessageList();

  auto* activeView = targetView ? targetView : AeSysView::GetActiveView();
  if (activeView != nullptr) {
    activeView->SetModeCursor(m_CurrentMode);
    activeView->ModeLineDisplay();
    activeView->RubberBandingDisable();
  }
}

/// <remarks> Font stroke table encoded as follows:
/// b0 - b11  relative y displacement
/// b12 - b23 relative x displacement
/// b24 - b31 operation code (5 for line, else move)
///
/// v1: 16384 bytes, 96-entry offset table (chars 32-126 + sentinel), stroke data at int32[96].
/// v2: 16384 bytes, magic (-2) at int32[0], 225-entry offset table at int32[1],
///    224-entry advance width table at int32[226], stroke data at int32[450].
/// </remarks>
void AeSys::LoadSimplexStrokeFont(const CString& pathName) {
  HANDLE openHandle =
      CreateFile(pathName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (openHandle != INVALID_HANDLE_VALUE) {
    if (SetFilePointer(openHandle, 0, nullptr, FILE_BEGIN) != static_cast<DWORD>(-1)) {
      if (m_SimplexStrokeFont == nullptr) { m_SimplexStrokeFont = new char[Eo::strokeFontFileSizeInBytes]; }
      DWORD numberOfBytesRead;
      if (!ReadFile(openHandle, m_SimplexStrokeFont, Eo::strokeFontFileSizeInBytes, &numberOfBytesRead, nullptr)) {
        ReleaseSimplexStrokeFont();
      }
    }
    CloseHandle(openHandle);
  } else {
    HRSRC resourceHandle = FindResourceW(nullptr, MAKEINTRESOURCE(IDR_PEGSTROKEFONT), L"STROKEFONT");
    if (resourceHandle != nullptr) {
      const rsize_t resourceSize = SizeofResource(nullptr, resourceHandle);
      m_SimplexStrokeFont = new char[resourceSize];
      const LPVOID resource = LockResource(LoadResource(nullptr, resourceHandle));
      memcpy_s(m_SimplexStrokeFont, resourceSize, resource, resourceSize);
    }
  }
  if (m_SimplexStrokeFont != nullptr) {
    const auto* fontData = reinterpret_cast<const long*>(m_SimplexStrokeFont);
    m_StrokeFontVersion = (fontData[0] == Eo::strokeFontV2MagicNumber) ? 2 : 1;
  }
}

void AeSys::ReleaseSimplexStrokeFont() {
  if (m_SimplexStrokeFont != nullptr) { delete[] m_SimplexStrokeFont; }
}

void AeSys::OnUpdateEditCfGroups(CCmdUI* pCmdUI) {
  pCmdUI->SetCheck(m_ClipboardDataEoGroups);
}
void AeSys::OnUpdateEditCfImage(CCmdUI* pCmdUI) {
  pCmdUI->SetCheck(m_ClipboardDataImage);
}
void AeSys::OnUpdateEditCfText(CCmdUI* pCmdUI) {
  pCmdUI->SetCheck(m_ClipboardDataText);
}
void AeSys::OnUpdateTrapcommandsAddgroups(CCmdUI* pCmdUI) {
  pCmdUI->SetCheck(m_TrapModeAddGroups);
}
void AeSys::OnUpdateTrapcommandsHighlight(CCmdUI* pCmdUI) {
  pCmdUI->SetCheck(m_TrapHighlighted);
}

// Modifies the base accelerator table by defining the mode specific keys.
void AeSys::BuildModifiedAcceleratorTable() const {
  auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());

  HACCEL acceleratorTableHandle = mainFrame->m_hAccelTable;
  DestroyAcceleratorTable(acceleratorTableHandle);

  const HACCEL modeAcceleratorTableHandle =
      ::LoadAccelerators(GetInstance(), MAKEINTRESOURCE(m_ModeResourceIdentifier));
  const int modeAcceleratorTableEntries = CopyAcceleratorTableW(modeAcceleratorTableHandle, nullptr, 0);

  acceleratorTableHandle = ::LoadAccelerators(GetInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
  const int acceleratorTableEntries = CopyAcceleratorTableW(acceleratorTableHandle, nullptr, 0);

  const auto modifiedAcceleratorTable =
      new ACCEL[static_cast<size_t>(acceleratorTableEntries + modeAcceleratorTableEntries)];

  CopyAcceleratorTableW(modeAcceleratorTableHandle, modifiedAcceleratorTable, modeAcceleratorTableEntries);
  CopyAcceleratorTableW(
      acceleratorTableHandle, &modifiedAcceleratorTable[modeAcceleratorTableEntries], acceleratorTableEntries);

  mainFrame->m_hAccelTable =
      ::CreateAcceleratorTable(modifiedAcceleratorTable, acceleratorTableEntries + modeAcceleratorTableEntries);

  delete[] modifiedAcceleratorTable;
}

void AeSys::OnFileSaveAll() {
  POSITION templatePosition = GetFirstDocTemplatePosition();
  while (templatePosition != nullptr) {
    const auto* docTemplate = GetNextDocTemplate(templatePosition);
    POSITION docPosition = docTemplate->GetFirstDocPosition();
    while (docPosition != nullptr) {
      auto* document = docTemplate->GetNextDoc(docPosition);
      if (document->IsModified()) { document->DoFileSave(); }
    }
  }
}

void AeSys::OnFileOpen() {
  auto filter = App::LoadStringResource(IDS_OPENFILE_FILTER);

  constexpr DWORD flags(OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);

  CFileDialog fileDialog(TRUE, nullptr, nullptr, flags, filter);

  CString fileName;
  fileDialog.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);

  auto title = App::LoadStringResource(AFX_IDS_OPENFILE);
  fileDialog.m_ofn.lpstrTitle = title;

  const auto result = fileDialog.DoModal();
  fileName.ReleaseBuffer();

  if (result == IDOK) { OpenDocumentFile(fileName); }
}

void AeSys::FormatAngle(CString& angleAsString, double angle, const int width, const int precision) {
  CString formatSpecification;
  formatSpecification.Format(L"%%%i.%if\u00B0", width, precision);
  angleAsString.Format(formatSpecification, Eo::RadianToDegree(angle));
}

void AeSys::FormatLength(CString& lengthAsString,
    Eo::Units units,
    double length,
    const int minWidth,
    const int precision) {
  constexpr size_t bufSize{32};
  auto lengthAsBuffer = lengthAsString.GetBufferSetLength(bufSize);

  if (units == Eo::Units::Architectural || units == Eo::Units::ArchitecturalS) {
    FormatLengthArchitectural(lengthAsBuffer, bufSize, units, length);
  } else if (units == Eo::Units::Engineering) {
    FormatLengthEngineering(lengthAsBuffer, bufSize, length, minWidth, precision);
  } else {
    FormatLengthSimple(lengthAsBuffer, bufSize, units, length, minWidth, precision);
  }
  lengthAsString.ReleaseBuffer();
}

void AeSys::FormatLengthArchitectural(wchar_t* lengthAsBuffer, const size_t bufSize, Eo::Units units, double length) const {
  wchar_t szBuf[16]{};

  double scaledLength = length * AeSysView::GetActiveView()->GetWorldScale();

  wcscpy_s(lengthAsBuffer, bufSize, (length >= 0.0) ? L" " : L"-");
  scaledLength = std::abs(scaledLength);

  int feet = static_cast<int>(scaledLength / 12.0);
  int inches = abs(static_cast<int>(fmod(scaledLength, 12.0)));

  const int fractionPrecision = GetArchitecturalUnitsFractionPrecision();
  int numerator = static_cast<int>(std::abs(fmod(scaledLength, 1.0)) * (double)(fractionPrecision) + 0.5);
  if (numerator == fractionPrecision) {
    if (inches == 11) {
      feet++;
      inches = 0;
    } else {
      inches++;
    }
    numerator = 0;
  }
  _itow_s(feet, szBuf, 16, 10);
  wcscat_s(lengthAsBuffer, bufSize, szBuf);
  wcscat_s(lengthAsBuffer, bufSize, L"'");

  _itow_s(inches, szBuf, 16, 10);
  wcscat_s(lengthAsBuffer, bufSize, szBuf);
  if (numerator > 0) {
    wcscat_s(lengthAsBuffer, bufSize, (units == Eo::Units::ArchitecturalS) ? L"\\S" : L"-");
    const int iGrtComDivisor = std::gcd(numerator, fractionPrecision);
    numerator /= iGrtComDivisor;
    const int denominator = fractionPrecision / iGrtComDivisor;
    _itow_s(numerator, szBuf, 16, 10);
    wcscat_s(lengthAsBuffer, bufSize, szBuf);
    wcscat_s(lengthAsBuffer, bufSize, L"/");
    _itow_s(denominator, szBuf, 16, 10);
    wcscat_s(lengthAsBuffer, bufSize, szBuf);
    if (units == Eo::Units::ArchitecturalS) { wcscat_s(lengthAsBuffer, bufSize, L";"); }
  }
  wcscat_s(lengthAsBuffer, bufSize, L"\"");
}

void AeSys::FormatLengthEngineering(wchar_t* lengthAsBuffer,
    const size_t bufSize,
    double length,
    const int width,
    const int precision) {
  wchar_t szBuf[16]{};

  double scaledLength = length * AeSysView::GetActiveView()->GetWorldScale();

  wcscpy_s(lengthAsBuffer, bufSize, (length >= 0.0) ? L" " : L"-");
  scaledLength = std::abs(scaledLength);

  const int adjustedPrecision =
      (scaledLength >= 1.0) ? precision - static_cast<int>(log10(scaledLength)) - 1 : precision;

  if (adjustedPrecision >= 0) {
    _itow_s(static_cast<int>(scaledLength / 12.0), szBuf, 16, 10);
    wcscat_s(lengthAsBuffer, bufSize, szBuf);
    scaledLength = fmod(scaledLength, 12.0);
    wcscat_s(lengthAsBuffer, bufSize, L"'");

    _itow_s(static_cast<int>(scaledLength), szBuf, 16, 10);
    wcscat_s(lengthAsBuffer, bufSize, szBuf);

    if (adjustedPrecision > 0) {
      CString formatSpecification;
      formatSpecification.Format(L"%%%i.%if", width, adjustedPrecision);

      CString fractionalInches;
      fractionalInches.Format(formatSpecification, scaledLength);
      const int decimalPointPosition = fractionalInches.Find('.');
      fractionalInches = fractionalInches.Mid(decimalPointPosition) + L"\"";

      wcscat_s(lengthAsBuffer, bufSize, fractionalInches);
    }
  }
}

void AeSys::FormatLengthSimple(wchar_t* lengthAsBuffer,
    const size_t bufSize,
    Eo::Units units,
    double length,
    const int width,
    const int precision) {
  const double scaledLength = length * AeSysView::GetActiveView()->GetWorldScale();

  CString formatSpecification;
  formatSpecification.Format(L"%%%i.%if", width, precision);
  CString formatted{};

  switch (units) {
    case Eo::Units::Feet:
      formatSpecification.Append(L"'");
      formatted.Format(formatSpecification, scaledLength / 12.0);
      break;
    case Eo::Units::Inches:
      formatSpecification.Append(L"\"");
      formatted.Format(formatSpecification, scaledLength);
      break;
    case Eo::Units::Meters:
      formatSpecification.Append(L"m");
      formatted.Format(formatSpecification, scaledLength * 0.0254);
      break;
    case Eo::Units::Millimeters:
      formatSpecification.Append(L"mm");
      formatted.Format(formatSpecification, scaledLength * 25.4);
      break;
    case Eo::Units::Centimeters:
      formatSpecification.Append(L"cm");
      formatted.Format(formatSpecification, scaledLength * 2.54);
      break;
    case Eo::Units::Decimeters:
      formatSpecification.Append(L"dm");
      formatted.Format(formatSpecification, scaledLength * 0.254);
      break;
    case Eo::Units::Kilometers:
      formatSpecification.Append(L"km");
      formatted.Format(formatSpecification, scaledLength * 0.0000254);
      break;

    case Eo::Units::Architectural:
    case Eo::Units::ArchitecturalS:
    case Eo::Units::Engineering:
      [[fallthrough]];  // Handled by specialized implementations;
    default:
      break;
  }
  wcsncpy_s(lengthAsBuffer, bufSize, formatted, _TRUNCATE);
}

/** @brief Adds optional inches and fraction to a length given in feet.
    @param inputLine The original input line containing the length string.
    @param feetLength The length in feet.
    @param[in, out] end Pointer to the character in inputLine immediately following the feet portion, updated to point
   to the character after the optional inches and fraction portion.
    @return The total length in inches after adding optional inches and fraction.
    @note Fraction is validated by lex::Scan typing token as AcrchitecturalUnitsLengthToken.
    @throws wchar_t* If an error occurs during parsing, an error message is thrown.
*/
static double AddOptionalInches(const wchar_t* inputLine, double feetLength, wchar_t* end) {
  wchar_t token[32]{};
  int linePosition{};
  const int tokenType = lex::Scan(token, inputLine, linePosition);

  double totalLength = feetLength;

  // Check if end of feet portion is end of inputLine and investigate inches portion if not
  if (end[1] != L'\0') {
    // Parse inches portion
    const double inches = wcstod(&end[1], &end);
    if (end == nullptr) { throw L"Invalid inches value in length string."; }
    // Add/subtract inches to totallength
    totalLength += std::copysign(inches, totalLength);
    if (tokenType == lex::ArchitecturalUnitsLengthToken) {
      // the inches component possibly looks like 1'2-3/4". end should be `-` character.If so, parse and add/subtract
      // that also
      if (*end == L'-') {
        wchar_t* fractionEnd{};
        const double numerator = wcstod(&end[1], &fractionEnd);
        if (fractionEnd == &end[1]) { throw L"Invalid fraction in length string."; }  // allowing 0.0 numerator here
        end = fractionEnd;
        const double denominator = wcstod(&end[1], &fractionEnd);
        if (fractionEnd == &end[1] || denominator == 0.0) { throw L"Invalid fraction denominator in length string."; }
        const double fraction = numerator / denominator;
        // Add/subtract fraction to totallength
        totalLength += std::copysign(fraction, totalLength);
        end = fractionEnd;
      }
    }
  }
  if (*end == L'\"') {
    end++;  // the inches component had the optional `"` character, skip it
  }
  return totalLength;
}

double AeSys::ParseLength(const wchar_t* inputLine) {
  wchar_t* end{};

  // Parse the leading numeric portion of the string or possible the numerator of a fraction
  double length = wcstod(inputLine, &end);

  if (end == inputLine) { throw L"Invalid length format."; }
  // The only valid case for a leading fraction is a variation of SimpleUnitsLengthToken
  // `{sign}{fraction}(\'|\"|{metric_units})`
  if (*end == L'/') {
    wchar_t* fractionEnd{};
    const double denominator = wcstod(&end[1], &fractionEnd);
    if (fractionEnd == &end[1] || denominator == 0.0) { throw L"Invalid length format."; }
    length = length / denominator;
    end = fractionEnd;
  }

  switch (toupper(end[0])) {
    case '\'':  // Feet optional inches
      length *= 12.0;
      length = AddOptionalInches(inputLine, length, end);
      break;

    case 'M':
      if (toupper(end[1]) == 'M') {
        length *= 0.03937007874015748;
      } else {
        length *= 39.37007874015748;
      }
      break;

    case 'C':
      length *= 0.3937007874015748;
      break;

    case 'D':
      length *= 3.937007874015748;
      break;

    case 'K':
      length *= 39370.07874015748;
  }
  return (length / AeSysView::GetActiveView()->GetWorldScale());
}

/** @brief Parses a length string according to the current units setting.
    @param units The units to use for parsing.
    @param inputLine The length string to parse.
    @return The parsed length in internal units (inches).
    @throws wchar_t* If an error occurs during parsing, an error message is thrown.
*/
double AeSys::ParseLength(Eo::Units units, const wchar_t* inputLine) {
  try {
    int iTokId{};
    long lDef{};
    int iTyp{};
    double length[32]{};

    lex::Parse(inputLine);

    try {
      lex::EvalTokenStream(&iTokId, &lDef, &iTyp, length);
    } catch (const std::domain_error& e) {
      app.AddStringToMessageList(MultiByteToWString(e.what()));
      return (0.0);
    } catch (const std::invalid_argument& e) {
      app.AddStringToMessageList(MultiByteToWString(e.what()));
      return (0.0);
    } catch (const wchar_t* errorMessage) { app.AddStringToMessageList(std::wstring(errorMessage)); }

    if (iTyp == lex::ArchitecturalUnitsLengthToken || iTyp == lex::EngineeringUnitsLengthToken
        || iTyp == lex::SimpleUnitsLengthToken) {
      return (length[0]);
    }
    lex::ConvertValTyp(iTyp, lex::RealToken, &lDef, length);

    switch (units) {
      case Eo::Units::Architectural:
      case Eo::Units::Engineering:
      case Eo::Units::Feet:
        length[0] *= 12.0;
        break;

      case Eo::Units::Meters:
        length[0] *= 39.37007874015748;
        break;

      case Eo::Units::Millimeters:
        length[0] *= 0.03937007874015748;
        break;

      case Eo::Units::Centimeters:
        length[0] *= 0.3937007874015748;
        break;

      case Eo::Units::Decimeters:
        length[0] *= 3.937007874015748;
        break;

      case Eo::Units::Kilometers:
        length[0] *= 39370.07874015748;
        break;

      case Eo::Units::ArchitecturalS:
      case Eo::Units::Inches:
        break;
    }
    length[0] /= AeSysView::GetActiveView()->GetWorldScale();
    return (length[0]);
  } catch (const wchar_t* errorMessage) {
    MessageBoxW(nullptr, errorMessage, nullptr, MB_ICONWARNING | MB_OK);
    return (0.0);
  }
}

// EoDlgAbout dialog used for App About

class EoDlgAbout : public CDialogEx {
 public:
  EoDlgAbout();
  EoDlgAbout(const EoDlgAbout&) = delete;
  EoDlgAbout& operator=(const EoDlgAbout&) = delete;

  enum { IDD = IDD_ABOUTBOX };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  void OnOK() override;
};

EoDlgAbout::EoDlgAbout() : CDialogEx(IDD) {}

void EoDlgAbout::DoDataExchange(CDataExchange* dataExchange) {
  CDialogEx::DoDataExchange(dataExchange);
}
void EoDlgAbout::OnOK() {
  CDialogEx::OnOK();
}

void AeSys::OnAppAbout() {
  EoDlgAbout dlg;
  dlg.DoModal();
}

namespace App {
EoDb::FileTypes FileTypeFromPath(const CString& pathName) {
  auto type(EoDb::FileTypes::Unknown);

  const int dotPosition = pathName.ReverseFind(L'.');
  if (dotPosition != -1 && dotPosition < pathName.GetLength() - 1) {
    CString extension = pathName.Mid(dotPosition + 1);
    extension.MakeLower();

    if (extension == L"peg") {
      type = EoDb::FileTypes::Peg;
    } else if (extension == L"tra") {
      type = EoDb::FileTypes::Tracing;
    } else if (extension == L"jb1") {
      type = EoDb::FileTypes::Job;
    } else if (extension == L"dwg") {
      type = EoDb::FileTypes::Dwg;
    } else if (extension == L"dxf") {
      type = EoDb::FileTypes::Dxf;
    } else if (extension == L"dxb") {
      type = EoDb::FileTypes::Dxb;
    }
  }
  return type;
}

CString PathFromCommandLine() {
  CString pathName = GetCommandLineW();
  const int lastPathDelimiter = pathName.ReverseFind(L'\\');
  pathName = pathName.Mid(1, lastPathDelimiter - 1);

  return pathName;
}

CString LoadStringResource(UINT resourceIdentifier) {
  CString resourceString;

  const BOOL success = resourceString.LoadStringW(resourceIdentifier);
#ifdef _DEBUG
  assert(success == TRUE && "Failed to load string resource");
#else
  (void)success;  // Suppress unused variable warning in release
#endif
  return resourceString;
}

CString ResourceFolderPath() {
  const auto applicationPath = PathFromCommandLine();
  return applicationPath + L"\\res\\";
}
}  // namespace App