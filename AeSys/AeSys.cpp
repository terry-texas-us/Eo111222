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
#include "Lex.h"
#include "MainFrm.h"
#include "PegColors.h"
#include "Resource.h"

#if defined(USING_DDE)
#include "Dde.h"
#include "ddeGItms.h"
#endif  // USING_DDE

ATOM WINAPI RegisterKeyPlanWindowClass(HINSTANCE instance);
ATOM WINAPI RegisterPreviewWindowClass(HINSTANCE instance);

namespace {

/** @brief Converts a multi-byte (UTF-8) string to a wide-character string.
* @param multiByte The multi-byte (UTF-8) string to convert.
* @return The converted wide-character string.
*/
std::wstring MultiByteToWString(const char* multiByte) {
  if (!multiByte) return {L""};
  int size = ::MultiByteToWideChar(CP_UTF8, 0, multiByte, -1, nullptr, 0);
  if (size == 0) return {L""};
  std::wstring string;
  string.resize(static_cast<size_t>(size) - 1);
  ::MultiByteToWideChar(CP_UTF8, 0, multiByte, -1, &string[0], size - 1);
  return string;
}

constexpr size_t numberOfPenWidths{16};
constexpr double defaultPenWidths[numberOfPenWidths] = {0.0,  0.0075, 0.015, 0.02,   0.03, 0.0075, 0.015, 0.0225,
                                                        0.03, 0.0075, 0.015, 0.0225, 0.03, 0.0075, 0.015, 0.0225};
}  // namespace

double penWidths[numberOfPenWidths] = {0.0,  0.0075, 0.015, 0.02,   0.03, 0.0075, 0.015, 0.0225,
                                       0.03, 0.0075, 0.015, 0.0225, 0.03, 0.0075, 0.015, 0.0225};
static void ResetPenWidthsToDefault() {
  std::copy(std::begin(defaultPenWidths), std::end(defaultPenWidths), penWidths);
}

namespace hatch {
double dXAxRefVecScal;
double dYAxRefVecScal;
double dOffAng;
int tableOffset[64]{};
float tableValue[1536]{};
}  // namespace hatch

auto* pColTbl = ColorPalette;

EoGsRenderState renderState;

BEGIN_MESSAGE_MAP(AeSys, CWinAppEx)
ON_COMMAND(ID_APP_ABOUT, &AeSys::OnAppAbout)
// Standard file based document commands
ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
ON_COMMAND(ID_FILE_OPEN, &AeSys::OnFileOpen)
// Standard print setup command
ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)

ON_COMMAND(ID_EDIT_CF_GROUPS, &AeSys::OnEditCfGroups)
ON_COMMAND(ID_EDIT_CF_IMAGE, &AeSys::OnEditCfImage)
ON_COMMAND(ID_EDIT_CF_TEXT, &AeSys::OnEditCfText)
ON_COMMAND(ID_FILE_RUN, &AeSys::OnFileRun)
ON_COMMAND(ID_HELP_CONTENTS, &AeSys::OnHelpContents)

ON_COMMAND(ID_MODE_LETTER, &AeSys::OnModeLetter)
ON_COMMAND(ID_MODE_REVISE, &AeSys::OnModeRevise)

ON_COMMAND(ID_TRAPCOMMANDS_HIGHLIGHT, &AeSys::OnTrapCommandsHighlight)
ON_COMMAND(ID_VIEW_MODEINFORMATION, &AeSys::OnViewModeInformation)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_UPDATE_COMMAND_UI(ID_EDIT_CF_GROUPS, &AeSys::OnUpdateEditCfGroups)
ON_UPDATE_COMMAND_UI(ID_EDIT_CF_IMAGE, &AeSys::OnUpdateEditCfImage)
ON_UPDATE_COMMAND_UI(ID_EDIT_CF_TEXT, &AeSys::OnUpdateEditCfText)
ON_UPDATE_COMMAND_UI(ID_VIEW_MODEINFORMATION, &AeSys::OnUpdateViewModeinformation)
ON_UPDATE_COMMAND_UI(ID_TRAPCOMMANDS_ADDGROUPS, &AeSys::OnUpdateTrapcommandsAddgroups)
ON_UPDATE_COMMAND_UI(ID_TRAPCOMMANDS_HIGHLIGHT, &AeSys::OnUpdateTrapcommandsHighlight)
#pragma warning(pop)
END_MESSAGE_MAP()

AeSys::AeSys()
    : CustomLButtonDownCharacters{},
      CustomLButtonUpCharacters{L"{13}"},
      CustomRButtonDownCharacters{},
      CustomRButtonUpCharacters{L"{27}"},
      m_Options{},
      // Private members
      m_HomePoints{},
      m_pegDocumentTemplate{},
      m_traDocumentTemplate{},
      m_MainFrameMenuHandle{},
      m_SimplexStrokeFont{},
      m_DeviceHeightInMillimeters{},
      m_DeviceHeightInPixels{},
      m_DeviceWidthInMillimeters{},
      m_DeviceWidthInPixels{},
      m_DimensionAngle{45.0},
      m_DimensionLength{0.125},
      m_EngagedAngle{},
      m_EngagedLength{},
      m_ShadowFolderPath{L""},
      m_ClipboardFormatIdentifierForEoGroups{},
      m_Units{Eo::Units::Inches},
      m_ArchitecturalUnitsFractionPrecision{16},
      m_CurrentMode{},
      m_ModeResourceIdentifier{},
      m_PrimaryMode{ID_MODE_DRAW},
      m_TrapHighlightColor{},
      m_ClipboardDataEoGroups{true},
      m_ClipboardDataImage{},
      m_ClipboardDataText{true},
      m_TrapHighlighted{},
      m_TrapModeAddGroups{true},
      m_HighColorMode{},
      m_ModeInformationOverView{},
      m_NodalModeAddGroups{true} {
  // Detect color depth. 256 color toolbars can be used in the high or true color modes only (bits per pixel is > 8):
  CClientDC dc(AfxGetMainWnd());
  m_HighColorMode = dc.GetDeviceCaps(BITSPIXEL) > 8;
  EnableHtmlHelp();
}

AeSys app;

BOOL AeSys::InitInstance() {
  if (!AfxOleInit()) {
    AfxMessageBox(IDP_OLE_INIT_FAILED);
    return FALSE;
  }
  AfxEnableControlContainer();

  CTrace::SetLevel(2);
  SetRegistryKey(L"Engineers Office");
  LoadStdProfileSettings();
  SetRegistryBase(L"Settings");

  m_Options.Load();

  // Initialize managers
  InitContextMenuManager();
  InitKeyboardManager();
  InitTooltipManager();

  CMFCToolTipInfo params;
  params.m_bVislManagerTheme = TRUE;
  GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS(CMFCToolTipCtrl), &params);

  EnableUserTools(ID_TOOLS_ENTRY, ID_USER_TOOL1, ID_USER_TOOL10, RUNTIME_CLASS(CUserTool), IDR_MENU_ARGS,
                  IDR_MENU_DIRS);

  // Initialize common controls required to enable visual styles.
  INITCOMMONCONTROLSEX InitCtrls{};
  InitCtrls.dwSize = sizeof(InitCtrls);
  InitCtrls.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&InitCtrls);

  CMFCButton::EnableWindowsTheming();

  CWinAppEx::InitInstance();

#if defined(USING_Direct2D)
  if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_Direct2dFactory))) { return FALSE; }
#endif

  // Register document templates

  auto* documentClass = RUNTIME_CLASS(AeSysDoc);
  auto* frameClass = RUNTIME_CLASS(CChildFrame);
  auto* viewClass = RUNTIME_CLASS(AeSysView);

  m_pegDocumentTemplate = new CMultiDocTemplate(IDR_PEGTYPE, documentClass, frameClass, viewClass);
  AddDocTemplate(m_pegDocumentTemplate);

  m_traDocumentTemplate = new CMultiDocTemplate(IDR_TRATYPE, documentClass, frameClass, viewClass);
  AddDocTemplate(m_traDocumentTemplate);

  // Create main MDI Frame window
  CMainFrame* mainFrame = new CMainFrame;
  if (!mainFrame || !mainFrame->LoadFrame(IDR_MAINFRAME)) {
    delete mainFrame;
    return FALSE;
  }
  m_pMainWnd = mainFrame;  // Set CWinApp::m_pMainWnd to the main frame window.
  mainFrame->DragAcceptFiles();

  CDC* DeviceContext = mainFrame->GetDC();
  m_DeviceWidthInPixels = static_cast<double>(DeviceContext->GetDeviceCaps(HORZRES));
  m_DeviceHeightInPixels = static_cast<double>(DeviceContext->GetDeviceCaps(VERTRES));
  m_DeviceWidthInMillimeters = static_cast<double>(DeviceContext->GetDeviceCaps(HORZSIZE));
  m_DeviceHeightInMillimeters = static_cast<double>(DeviceContext->GetDeviceCaps(VERTSIZE));
  InitGbls(DeviceContext);
  mainFrame->ReleaseDC(DeviceContext);

  // Parse command line and process shell commands
  CCommandLineInfo commandLineInfo;
  ParseCommandLine(commandLineInfo);

  if (commandLineInfo.m_nShellCommand == CCommandLineInfo::FileNew) {
    if (!mainFrame->LoadMDIState(GetRegSectionPath())) { m_pegDocumentTemplate->OpenDocumentFile(nullptr); }
  } else {
    if (!ProcessShellCommand(commandLineInfo)) { return FALSE; }
  }
  m_MainFrameMenuHandle = LoadMenuW(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));

  if (!RegisterKeyPlanWindowClass(AeSys::GetInstance())) { return FALSE; }
  if (!RegisterPreviewWindowClass(AeSys::GetInstance())) { return FALSE; }

  if (SetShadowFolderPath(L"AeSys Shadow Folder") != 0) {
    AddStringToMessageList(L"Failed to set shadow folder path.");
  }

  CString resourceFolder = App::ResourceFolderPath();
  LoadSimplexStrokeFont(resourceFolder + Eo::defaultStrokeFont);
  LoadHatchesFromFile(resourceFolder + L"Hatches\\DefaultSet.txt");
  //LoadPenColorsFromFile(ResourceFolder + L"Pens\\Colors\\Default.txt"));

#if defined(USING_DDE)
  // Initialize for using DDEML
  dde::Setup(AeSys::GetInstance());
#endif  // USING_DDE

  // This is the private data format used to pass EoGroups from one instance to another
  m_ClipboardFormatIdentifierForEoGroups = RegisterClipboardFormatW(L"EoGroups");

  GetMainWindow()->ShowWindow(m_nCmdShow);
  GetMainWindow()->UpdateWindow();

  // Testing code here

  return TRUE;
}

int AeSys::ExitInstance() {
  m_Options.Save();

#if defined(USING_DDE)
  dde::Uninitialize();
#endif  // USING_DDE

  ReleaseSimplexStrokeFont();

  return CWinAppEx::ExitInstance();
}
/// <remarks> Processing occurs immediately before the framework loads the application state from the registry. </remarks>
void AeSys::PreLoadState() {
  GetContextMenuManager()->AddMenu(L"My menu", IDR_CONTEXT_MENU);

  // TODO: add another context menus here
}

int AeSys::ConfirmMessageBox(UINT stringResourceIdentifier, const CString& string) {
  auto formatSpecification = App::LoadStringResource(stringResourceIdentifier);

  CString formattedResourceString;
  formattedResourceString.Format(formatSpecification, string.GetString());

  int nextToken{0};
  CString message = formattedResourceString.Tokenize(L"\t", nextToken);
  CString caption = formattedResourceString.Tokenize(L"\n", nextToken);

  return (MessageBoxW(0, message, caption, MB_ICONINFORMATION | MB_YESNOCANCEL | MB_DEFBUTTON2));
}

void AeSys::AddStringToMessageList(const CString& message) {
  auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());

  mainFrame->GetOutputPane().AddStringToMessageList(message);
  if (!mainFrame->GetOutputPane().IsWindowVisible()) { mainFrame->SetPaneText(1, message); }
}
void AeSys::AddStringToMessageList(const std::wstring& message) {
  auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());

  mainFrame->GetOutputPane().AddStringToMessageList(message);
  if (!mainFrame->GetOutputPane().IsWindowVisible()) { mainFrame->SetPaneText(1, message.c_str()); }
}
void AeSys::AddStringToMessageList(const wchar_t* message) {
  auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());

  mainFrame->GetOutputPane().AddStringToMessageList(std::wstring(message));
  if (!mainFrame->GetOutputPane().IsWindowVisible()) { mainFrame->SetPaneText(1, message); }
}

void AeSys::AddModeInformationToMessageList() {
  auto resourceString = App::LoadStringResource(static_cast<UINT>(m_CurrentMode));
  int nextToken{0};
  resourceString = resourceString.Tokenize(L"\n", nextToken);
  AddStringToMessageList(resourceString);
}

void AeSys::AddStringToMessageList(UINT stringResourceIdentifier) {
  auto resourceString = App::LoadStringResource(stringResourceIdentifier);

  AddStringToMessageList(resourceString);
}

void AeSys::AddStringToMessageList(UINT stringResourceIdentifier, const CString& string) {
  auto formatSpecification = App::LoadStringResource(stringResourceIdentifier);

  CString formattedResourceString;
  formattedResourceString.Format(formatSpecification, string.GetString());

  AddStringToMessageList(formattedResourceString);
}

void AeSys::WarningMessageBox(UINT stringResourceIdentifier) {
  auto resourceString = App::LoadStringResource(stringResourceIdentifier);

  int nextToken{0};
  CString message = resourceString.Tokenize(L"\t", nextToken);
  CString caption = resourceString.Tokenize(L"\n", nextToken);

  MessageBoxW(0, message, caption, MB_ICONWARNING | MB_OK);
}

void AeSys::WarningMessageBox(UINT stringResourceIdentifier, const CString& string) {
  auto formatSpecification = App::LoadStringResource(stringResourceIdentifier);

  CString formattedResourceString;
  formattedResourceString.Format(formatSpecification, string.GetString());

  int nextToken{0};
  CString message = formattedResourceString.Tokenize(L"\t", nextToken);
  CString caption = formattedResourceString.Tokenize(L"\n", nextToken);

  MessageBoxW(0, message, caption, MB_ICONWARNING | MB_OK);
}

void AeSys::UpdateMDITabs(BOOL resetMDIChild) { ((CMainFrame*)AfxGetMainWnd())->UpdateMDITabs(resetMDIChild); }
void AeSys::OnTrapCommandsHighlight() {
  m_TrapHighlighted = !m_TrapHighlighted;
  //LPARAM lHint = m_TrapHighlighted ? EoDb::kGroupsSafeTrap : EoDb::kGroupsSafe;
  //UpdateAllViews(nullptr, lHint, &m_TrappedGroupList);
}
void AeSys::OnEditCfGroups() { m_ClipboardDataEoGroups = !m_ClipboardDataEoGroups; }
void AeSys::OnEditCfImage() { m_ClipboardDataImage = !m_ClipboardDataImage; }
void AeSys::OnEditCfText() { m_ClipboardDataText = !m_ClipboardDataText; }

void AeSys::OnModeLetter() {
  EoDlgModeLetter Dialog;
  Dialog.DoModal();
}

void AeSys::OnModeRevise() {
  EoDlgModeRevise Dialog;
  Dialog.DoModal();
}

void AeSys::OnFileRun() {
  auto filter = App::LoadStringResource(IDS_OPENFILE_FILTER_APPS);

  CFileDialog dlg(TRUE, L"exe", L"*.exe", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, filter);
  dlg.m_ofn.lpstrTitle = L"Run Application";

  if (dlg.DoModal() == IDOK) {
    CString strFile = dlg.GetFileName();
    //Note: use of winexec should be replaced with createprocess
    //WinExec(strFile, SW_SHOW);
  }
}

void AeSys::OnHelpContents() { ::WinHelpW(GetSafeHwnd(), L"peg.hlp", HELP_CONTENTS, 0L); }

[[nodiscard]] double AeSys::LineWeight(std::int16_t penIndex) { return (penWidths[penIndex]); }

void AeSys::LoadPenWidthsFromFile(const CString& fileName) {
  CStdioFile file;

  if (!file.Open(fileName, CFile::modeRead | CFile::typeText)) { return; }

  constexpr size_t bufferSize{128};
  wchar_t inputLine[bufferSize];

  while (file.ReadString(inputLine, bufferSize - 1) != 0) {
    if (inputLine[0] == L'\0' || inputLine[0] == L'#' || inputLine[0] == L';') { continue; }

    wchar_t* context = nullptr;
    wchar_t* penIndexText = wcstok_s(inputLine, L"=", &context);
    if (penIndexText == nullptr) { continue; }

    wchar_t* penIndexEnd = nullptr;
    unsigned long penIndex = std::wcstoul(penIndexText, &penIndexEnd, 10);
    if (penIndexEnd == penIndexText) { continue; }

    wchar_t* penWidthText = wcstok_s(nullptr, L",\n", &context);
    if (penWidthText == nullptr) { continue; }

    wchar_t* penValueEnd = nullptr;
    double penWidth = std::wcstod(penWidthText, &penValueEnd);
    if (penValueEnd == penWidthText) { continue; }

    if (static_cast<size_t>(penIndex) < numberOfPenWidths) { penWidths[penIndex] = penWidth; }
  }
}

[[nodiscard]] int AeSys::SetShadowFolderPath(const CString& folder) {
  wchar_t path[MAX_PATH]{};

  if (SHGetSpecialFolderPathW(AeSys::GetSafeHwnd(), path, CSIDL_PERSONAL, TRUE)) {
    m_ShadowFolderPath = path;
  } else {
    m_ShadowFolderPath.Empty();
  }
  m_ShadowFolderPath += L"\\" + folder + L"\\";

  return (_wmkdir(m_ShadowFolderPath));
}

[[nodiscard]] EoGePoint3d AeSys::GetCursorPosition() {
  auto* activeView = AeSysView::GetActiveView();
  return (activeView == nullptr) ? EoGePoint3d::kOrigin : activeView->GetCursorPosition();
}

[[nodiscard]] HINSTANCE AeSys::GetInstance() { return AfxGetInstanceHandle(); }

[[nodiscard]] CWnd* AeSys::GetMainWindow() { return AfxGetMainWnd(); }

[[nodiscard]] HWND AeSys::GetSafeHwnd() { return (AfxGetMainWnd()->GetSafeHwnd()); }

void AeSys::SetCursorPosition(EoGePoint3d pt) {
  auto* activeView = AeSysView::GetActiveView();
  activeView->SetCursorPosition(pt);
}

// Loads the hatch table.
void AeSys::LoadHatchesFromFile(const CString& fileName) {
  CFileException e;
  CStdioFile fl;

  if (!fl.Open(fileName, CFile::modeRead | CFile::typeText, &e)) { return; }

  wchar_t line[128]{};
  double dTotStrsLen{};
  int iNmbEnts{};
  int iNmbStrsId{};

  wchar_t szValDel[] = L",\0";
  int iHatId{};
  int iNmbHatLns{};
  int iTblId{};

  while (fl.ReadString(line, sizeof(line) / sizeof(wchar_t) - 1) != 0) {
    if (*line == '!') {  // New Hatch index
      if (iHatId != 0) { hatch::tableValue[hatch::tableOffset[iHatId]] = float(iNmbHatLns); }
      hatch::tableOffset[++iHatId] = iTblId++;
      iNmbHatLns = 0;
    } else {
      iNmbStrsId = iTblId;
      iTblId += 2;
      iNmbEnts = 0;
      dTotStrsLen = 0.;
      LPWSTR NextToken = nullptr;
      LPWSTR pTok = wcstok_s(line, szValDel, &NextToken);
      while (pTok != 0) {
        volatile double tempValue = _wtof(pTok);
        hatch::tableValue[iTblId] = static_cast<float>(tempValue);
        iNmbEnts++;
        if (iNmbEnts >= 6) { dTotStrsLen = dTotStrsLen + hatch::tableValue[iTblId]; }
        iTblId++;
        pTok = wcstok_s(0, szValDel, &NextToken);
      }
      hatch::tableValue[iNmbStrsId++] = float(iNmbEnts - 5);
      hatch::tableValue[iNmbStrsId] = float(dTotStrsLen);
      iNmbHatLns++;
    }
  }
}

[[nodiscard]] EoGePoint3d AeSys::HomePointGet(int i) const {
  if (i >= 0 && i < 9) return (m_HomePoints[i]);

  return (EoGePoint3d::kOrigin);
}
void AeSys::HomePointSave(int i, const EoGePoint3d& pt) {
  if (i >= 0 && i < 9) m_HomePoints[i] = pt;
}

//Initializes all peg global sections to their default (startup) values.
void AeSys::InitGbls(CDC* deviceContext) {
  renderState.SetPolygonIntStyle(EoDb::PolygonStyle::Hollow);

  renderState.SetPolygonIntStyleId(1);

  hatch::dXAxRefVecScal = 0.1;
  hatch::dYAxRefVecScal = 0.1;
  hatch::dOffAng = 0.0;

  EoDbCharacterCellDefinition characterCellDefinition{};
  renderState.SetCharacterCellDefinition(characterCellDefinition);

  EoDbFontDefinition fontDefinition;
  renderState.SetFontDefinition(deviceContext, fontDefinition);

  SetUnits(Eo::Units::Inches);
  SetArchitecturalUnitsFractionPrecision(8);
  SetDimensionLength(0.125);
  SetDimensionAngle(45.0);

  m_TrapHighlighted = true;
  m_TrapHighlightColor = 15;

  //Document->InitializeGroupAndPrimitiveEdit();
  renderState.SetPen(nullptr, deviceContext, 1, 1);
  renderState.SetPointStyle(0);
}
void AeSys::EditColorPalette() {
  CHOOSECOLOR cc{};
  cc.lStructSize = sizeof(CHOOSECOLOR);
  cc.rgbResult = ColorPalette[renderState.Color()];
  cc.lpCustColors = ColorPalette;
  cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_SOLIDCOLOR;
  ::ChooseColor(&cc);
  cc.rgbResult = GreyPalette[renderState.Color()];
  cc.lpCustColors = GreyPalette;
  ::ChooseColor(&cc);

  MessageBoxW(nullptr, L"The background color is no longer associated with the pen Color Palette.",
              L"Deprecation Notice", MB_OK | MB_ICONINFORMATION);

  AeSysDoc::GetDoc()->UpdateAllViews(nullptr, 0L, nullptr);
}
// Loads the color table.
void AeSys::LoadPenColorsFromFile(const CString& strFileName) {
  CStdioFile fl;

  if (fl.Open(strFileName, CFile::modeRead | CFile::typeText)) {
    wchar_t pBuf[128]{};
    LPWSTR pId, pRed, pGreen, pBlue;

    while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(wchar_t) - 1) != 0 && _tcsnicmp(pBuf, L"<Colors>", 8) != 0);

    while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(wchar_t) - 1) != 0 && *pBuf != '<') {
      LPWSTR NextToken = nullptr;
      pId = wcstok_s(pBuf, L"=", &NextToken);
      pRed = wcstok_s(0, L",", &NextToken);
      pGreen = wcstok_s(0, L",", &NextToken);
      pBlue = wcstok_s(0, L",", &NextToken);
      ColorPalette[_wtoi(pId)] = RGB(_wtoi(pRed), _wtoi(pGreen), _wtoi(pBlue));
      pRed = wcstok_s(0, L",", &NextToken);
      pGreen = wcstok_s(0, L",", &NextToken);
      pBlue = wcstok_s(0, L"\n", &NextToken);
      GreyPalette[_wtoi(pId)] = RGB(_wtoi(pRed), _wtoi(pGreen), _wtoi(pBlue));
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
/// The font is exactly 16384 bytes and defines a 96 character font set with a maximum of 4096 stokes
/// </remarks>
void AeSys::LoadSimplexStrokeFont(const CString& pathName) {
  HANDLE OpenHandle = CreateFile(pathName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (OpenHandle != INVALID_HANDLE_VALUE) {
    if (SetFilePointer(OpenHandle, 0, 0, FILE_BEGIN) != (DWORD)-1) {
      if (m_SimplexStrokeFont == 0) { m_SimplexStrokeFont = new char[16384]; }
      DWORD NumberOfBytesRead;
      if (!ReadFile(OpenHandle, m_SimplexStrokeFont, 16384U, &NumberOfBytesRead, 0)) { ReleaseSimplexStrokeFont(); }
    }
    CloseHandle(OpenHandle);
  } else {
    HRSRC ResourceHandle = FindResourceW(nullptr, MAKEINTRESOURCE(IDR_PEGSTROKEFONT), L"STROKEFONT");
    if (ResourceHandle != nullptr) {
      rsize_t ResourceSize = SizeofResource(nullptr, ResourceHandle);
      m_SimplexStrokeFont = new char[ResourceSize];
      LPVOID Resource = LockResource(LoadResource(nullptr, ResourceHandle));
      memcpy_s(m_SimplexStrokeFont, ResourceSize, Resource, ResourceSize);
    }
  }
}
void AeSys::ReleaseSimplexStrokeFont() {
  if (m_SimplexStrokeFont != 0) { delete[] m_SimplexStrokeFont; }
}
void AeSys::OnViewModeInformation() {
  m_ModeInformationOverView = !m_ModeInformationOverView;
  AeSysDoc::GetDoc()->UpdateAllViews(nullptr, 0L, nullptr);
}
void AeSys::OnUpdateEditCfGroups(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ClipboardDataEoGroups); }
void AeSys::OnUpdateEditCfImage(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ClipboardDataImage); }
void AeSys::OnUpdateEditCfText(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ClipboardDataText); }
void AeSys::OnUpdateTrapcommandsAddgroups(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_TrapModeAddGroups); }
void AeSys::OnUpdateTrapcommandsHighlight(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_TrapHighlighted); }
void AeSys::OnUpdateViewModeinformation(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ModeInformationOverView); }

// Modifies the base accelerator table by defining the mode specific keys.
void AeSys::BuildModifiedAcceleratorTable() {
  CMainFrame* MainFrame = (CMainFrame*)AfxGetMainWnd();

  HACCEL AcceleratorTableHandle = MainFrame->m_hAccelTable;
  ::DestroyAcceleratorTable(AcceleratorTableHandle);

  HACCEL ModeAcceleratorTableHandle =
      ::LoadAccelerators(AeSys::GetInstance(), MAKEINTRESOURCE(m_ModeResourceIdentifier));
  int ModeAcceleratorTableEntries = CopyAcceleratorTableW(ModeAcceleratorTableHandle, nullptr, 0);

  AcceleratorTableHandle = ::LoadAccelerators(AeSys::GetInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
  int AcceleratorTableEntries = CopyAcceleratorTableW(AcceleratorTableHandle, nullptr, 0);

  LPACCEL ModifiedAcceleratorTable =
      new ACCEL[static_cast<size_t>(AcceleratorTableEntries + ModeAcceleratorTableEntries)];

  CopyAcceleratorTableW(ModeAcceleratorTableHandle, ModifiedAcceleratorTable, ModeAcceleratorTableEntries);
  CopyAcceleratorTableW(AcceleratorTableHandle, &ModifiedAcceleratorTable[ModeAcceleratorTableEntries],
                        AcceleratorTableEntries);

  MainFrame->m_hAccelTable =
      ::CreateAcceleratorTable(ModifiedAcceleratorTable, AcceleratorTableEntries + ModeAcceleratorTableEntries);

  delete[] ModifiedAcceleratorTable;
}

void AeSys::OnFileOpen() {
  auto filter = App::LoadStringResource(IDS_OPENFILE_FILTER);

  constexpr DWORD flags(OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);

  CFileDialog fileDialog(TRUE, nullptr, nullptr, flags, filter);

  CString fileName;
  fileDialog.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);

  auto title = App::LoadStringResource(AFX_IDS_OPENFILE);
  fileDialog.m_ofn.lpstrTitle = title;

  auto result = fileDialog.DoModal();
  fileName.ReleaseBuffer();

  if (result == IDOK) { OpenDocumentFile(fileName); }
}

void AeSys::FormatAngle(CString& angleAsString, const double angle, const int width, const int precision) {
  CString FormatSpecification;
  FormatSpecification.Format(L"%%%i.%if\u00B0", width, precision);
  angleAsString.Format(FormatSpecification, Eo::RadianToDegree(angle));
}

void AeSys::FormatLength(CString& lengthAsString, Eo::Units units, const double length, const int minWidth,
                         const int precision) {
  const size_t bufSize{32};
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

void AeSys::FormatLengthArchitectural(LPWSTR lengthAsBuffer, const size_t bufSize, Eo::Units units,
                                      const double length) {
  wchar_t szBuf[16]{};

  double ScaledLength = length * AeSysView::GetActiveView()->GetWorldScale();

  wcscpy_s(lengthAsBuffer, bufSize, (length >= 0.0) ? L" " : L"-");
  ScaledLength = fabs(ScaledLength);

  int Feet = int(ScaledLength / 12.0);
  int Inches = abs(int(fmod(ScaledLength, 12.0)));

  int fractionPrecision = GetArchitecturalUnitsFractionPrecision();
  int numerator = int(fabs(fmod(ScaledLength, 1.0)) * (double)(fractionPrecision) + 0.5);

  if (numerator == fractionPrecision) {
    if (Inches == 11) {
      Feet++;
      Inches = 0;
    } else {
      Inches++;
    }
    numerator = 0;
  }
  _itow_s(Feet, szBuf, 16, 10);
  wcscat_s(lengthAsBuffer, bufSize, szBuf);
  wcscat_s(lengthAsBuffer, bufSize, L"'");

  _itow_s(Inches, szBuf, 16, 10);
  wcscat_s(lengthAsBuffer, bufSize, szBuf);
  if (numerator > 0) {
    wcscat_s(lengthAsBuffer, static_cast<size_t>(bufSize), (units == Eo::Units::ArchitecturalS) ? L"\\S" : L"-");
    int iGrtComDivisor = std::gcd(numerator, fractionPrecision);
    numerator /= iGrtComDivisor;
    int Denominator = fractionPrecision / iGrtComDivisor;
    _itow_s(numerator, szBuf, 16, 10);
    wcscat_s(lengthAsBuffer, bufSize, szBuf);
    wcscat_s(lengthAsBuffer, bufSize, L"/");
    _itow_s(Denominator, szBuf, 16, 10);
    wcscat_s(lengthAsBuffer, bufSize, szBuf);
    if (units == Eo::Units::ArchitecturalS) wcscat_s(lengthAsBuffer, bufSize, L";");
  }
  wcscat_s(lengthAsBuffer, bufSize, L"\"");
}

void AeSys::FormatLengthEngineering(LPWSTR lengthAsBuffer, const size_t bufSize, const double length, const int width,
                                    const int precision) {
  wchar_t szBuf[16]{};

  double ScaledLength = length * AeSysView::GetActiveView()->GetWorldScale();

  wcscpy_s(lengthAsBuffer, bufSize, (length >= 0.0) ? L" " : L"-");
  ScaledLength = fabs(ScaledLength);

  int Precision = (ScaledLength >= 1.0) ? precision - int(log10(ScaledLength)) - 1 : precision;

  if (Precision >= 0) {
    _itow_s(int(ScaledLength / 12.0), szBuf, 16, 10);
    wcscat_s(lengthAsBuffer, bufSize, szBuf);
    ScaledLength = fmod(ScaledLength, 12.0);
    wcscat_s(lengthAsBuffer, bufSize, L"'");

    _itow_s(int(ScaledLength), szBuf, 16, 10);
    wcscat_s(lengthAsBuffer, bufSize, szBuf);

    if (Precision > 0) {
      CString FormatSpecification;
      FormatSpecification.Format(L"%%%i.%if", width, Precision);

      CString FractionalInches;
      FractionalInches.Format(FormatSpecification, ScaledLength);
      int DecimalPointPosition = FractionalInches.Find('.');
      FractionalInches = FractionalInches.Mid(DecimalPointPosition) + L"\"";

      wcscat_s(lengthAsBuffer, bufSize, FractionalInches);
    }
  }
}
void AeSys::FormatLengthSimple(LPWSTR lengthAsBuffer, const size_t bufSize, Eo::Units units, const double length,
                               const int width, const int precision) {
  double ScaledLength = length * AeSysView::GetActiveView()->GetWorldScale();

  CString formatSpecification;
  formatSpecification.Format(L"%%%i.%if", width, precision);
  CString formatted{};

  switch (units) {
    case Eo::Units::Feet:
      formatSpecification.Append(L"'");
      formatted.Format(formatSpecification, ScaledLength / 12.0);
      break;
    case Eo::Units::Inches:
      formatSpecification.Append(L"\"");
      formatted.Format(formatSpecification, ScaledLength);
      break;
    case Eo::Units::Meters:
      formatSpecification.Append(L"m");
      formatted.Format(formatSpecification, ScaledLength * 0.0254);
      break;
    case Eo::Units::Millimeters:
      formatSpecification.Append(L"mm");
      formatted.Format(formatSpecification, ScaledLength * 25.4);
      break;
    case Eo::Units::Centimeters:
      formatSpecification.Append(L"cm");
      formatted.Format(formatSpecification, ScaledLength * 2.54);
      break;
    case Eo::Units::Decimeters:
      formatSpecification.Append(L"dm");
      formatted.Format(formatSpecification, ScaledLength * 0.254);
      break;
    case Eo::Units::Kilometers:
      formatSpecification.Append(L"km");
      formatted.Format(formatSpecification, ScaledLength * 0.0000254);
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
    @param[in, out] end Pointer to the character in inputLine immediately following the feet portion, updated to point to the character after the optional inches and fraction portion.
    @return The total length in inches after adding optional inches and fraction.
    @note Fraction is validated by lex::Scan typing token as AcrchitecturalUnitsLengthToken.
    @throws wchar_t* If an error occurs during parsing, an error message is thrown.
*/
static double AddOptionalInches(wchar_t* inputLine, double feetLength, wchar_t* end) {
  wchar_t token[32]{};
  int linePosition{};
  int tokenType = lex::Scan(token, inputLine, linePosition);

  double totalLength = feetLength;

  // Check if end of feet portion is end of inputLine and investigate inches portion if not
  if (end[1] != L'\0') {  // Parse inches portion
    double inches = wcstod(&end[1], &end);
    if (end == nullptr) { throw L"Invalid inches value in length string."; }
    // Add/subtract inches to totallength
    totalLength += std::copysign(inches, totalLength);
    if (tokenType == lex::ArchitecturalUnitsLengthToken) {
      // the inches component possibly looks like 1'2-3/4". end should be `-` character.If so, parse and add/subtract that also
      if (*end == L'-') {
        wchar_t* fractionEnd{};
        double numerator = wcstod(&end[1], &fractionEnd);
        if (fractionEnd == &end[1]) { throw L"Invalid fraction in length string."; }  // allowing 0.0 numerator here
        end = fractionEnd;
        double denominator = wcstod(&end[1], &fractionEnd);
        if (fractionEnd == &end[1] || denominator == 0.0) { throw L"Invalid fraction denominator in length string."; }
        double fraction = numerator / denominator;
        // Add/subtract fraction to totallength
        totalLength += std::copysign(fraction, totalLength);
        end = fractionEnd;
      }
    }
  }
  if (*end == L'\"') end++;  // the inches component had the optional `"` character, skip it
  return totalLength;
}

[[nodiscard]] double AeSys::ParseLength(wchar_t* inputLine) {
  wchar_t* end{};

  // Parse the leading numeric portion of the string or possible the numerator of a fraction
  double length = wcstod(inputLine, &end);

  if (end == inputLine) { throw L"Invalid length format."; }
  // The only valid case for a leading fraction is a variation of SimpleUnitsLengthToken `{sign}{fraction}(\'|\"|{metric_units})`
  if (*end == L'/') {
    wchar_t* fractionEnd{};
    double denominator = wcstod(&end[1], &fractionEnd);
    if (fractionEnd == &end[1] || denominator == 0.0) { throw L"Invalid length format."; }
    length = length / denominator;
    end = fractionEnd;
  }

  switch (toupper(static_cast<int>(end[0]))) {
    case '\'':  // Feet optional inches
      length *= 12.0;
      length = AddOptionalInches(inputLine, length, end);
      break;

    case 'M':
      if (toupper(static_cast<int>(end[1])) == 'M') {
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
[[nodiscard]] double AeSys::ParseLength(Eo::Units units, wchar_t* inputLine) {
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

    if (iTyp == lex::ArchitecturalUnitsLengthToken || iTyp == lex::EngineeringUnitsLengthToken ||
        iTyp == lex::SimpleUnitsLengthToken) {
      return (length[0]);
    } else {
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
    }
    return (length[0]);

  } catch (const wchar_t* errorMessage) {
    ::MessageBoxW(0, errorMessage, 0, MB_ICONWARNING | MB_OK);
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
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual void OnOK();
};

EoDlgAbout::EoDlgAbout() : CDialogEx(EoDlgAbout::IDD) {}
void EoDlgAbout::DoDataExchange(CDataExchange* dataExchange) { CDialogEx::DoDataExchange(dataExchange); }
void EoDlgAbout::OnOK() { CDialogEx::OnOK(); }

void AeSys::OnAppAbout() {
  EoDlgAbout dlg;
  dlg.DoModal();
}

namespace App {
[[nodiscard]] EoDb::FileTypes FileTypeFromPath(const CString& pathName) {
  EoDb::FileTypes type(EoDb::FileTypes::Unknown);

  int dotPosition = pathName.ReverseFind(L'.');
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

[[nodiscard]] CString PathFromCommandLine() {
  CString pathName = ::GetCommandLineW();
  int lastPathDelimiter = pathName.ReverseFind(L'\\');
  pathName = pathName.Mid(1, lastPathDelimiter - 1);

  return pathName;
}

[[nodiscard]] CString LoadStringResource(UINT resourceIdentifier) {
  CString resourceString;

  const BOOL success = resourceString.LoadStringW(resourceIdentifier);
#ifdef _DEBUG
  assert(success == TRUE && "Failed to load string resource");
#else
  (void)success;  // Suppress unused variable warning in release
#endif
  return resourceString;
}

[[nodiscard]] CString ResourceFolderPath() {
  auto applicationPath = PathFromCommandLine();
  return applicationPath + L"\\res\\";
}

}  // namespace App
