#include "Stdafx.h"

#include <ShlObj_core.h>
#include <Windows.h>
#include <afx.h>
#include <afxbutton.h>
#include <afxdialogex.h>
#include <afxdisp.h>
#include <afxdlgs.h>
#include <afxmsg_.h>
#include <afxres.h>
#include <afxstr.h>
#include <afxtooltipctrl.h>
#include <afxtooltipmanager.h>
#include <afxusertool.h>
#include <afxwin.h>
#include <afxwinappex.h>
#include <atlsimpstr.h>
#include <atltrace.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <corecrt.h>
#include <cstdlib>
#include <direct.h>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string.h>
#include <tchar.h>
#include <wchar.h>

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
#include "Lex.h"
#include "MainFrm.h"
#include "PegColors.h"
#include "PrimState.h"
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
constexpr double defaultPenWidths[numberOfPenWidths] = {0.0,  0.0075, 0.015, 0.02, 0.03, 0.0075, 0.015, 0.0225,
                                                        0.03, 0.0075, 0.015, 0.0225, 0.03, 0.0075, 0.015, 0.0225};
}  // namespace
double penWidths[numberOfPenWidths] = {0.0, 0.0075, 0.015, 0.02, 0.03, 0.0075, 0.015, 0.0225, 0.03, 0.0075, 0.015, 0.0225, 0.03, 0.0075, 0.015, 0.0225};
static void ResetPenWidthsToDefault() { std::copy(std::begin(defaultPenWidths), std::end(defaultPenWidths), penWidths); }

namespace hatch {
double dXAxRefVecScal;
double dYAxRefVecScal;
double dOffAng;
int iTableOffset[64];
float fTableValue[1536];
}  // namespace hatch

COLORREF* pColTbl = ColorPalette;

CPrimState pstate;

// This is a legacy feature. All values are empty strings now for normal MouseButton command processing.
// User may still change through user interface, so must not assume empty.

CString AeSys::CustomLButtonDownCharacters(L"");
CString AeSys::CustomLButtonUpCharacters(L"{13}");
CString AeSys::CustomRButtonDownCharacters(L"");
CString AeSys::CustomRButtonUpCharacters(L"{27}");

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
ON_COMMAND(ID_MODE_ANNOTATE, &AeSys::OnModeAnnotate)
ON_COMMAND(ID_MODE_CUT, &AeSys::OnModeCut)
ON_COMMAND(ID_MODE_DIMENSION, &AeSys::OnModeDimension)
ON_COMMAND(ID_MODE_DRAW, OnModeDraw)
ON_COMMAND(ID_MODE_DRAW2, &AeSys::OnModeDraw2)
ON_COMMAND(ID_MODE_EDIT, &AeSys::OnModeEdit)
ON_COMMAND(ID_MODE_FIXUP, &AeSys::OnModeFixup)
ON_COMMAND(ID_MODE_LETTER, &AeSys::OnModeLetter)
ON_COMMAND(ID_MODE_LPD, &AeSys::OnModeLPD)
ON_COMMAND(ID_MODE_NODAL, &AeSys::OnModeNodal)
ON_COMMAND(ID_MODE_PIPE, &AeSys::OnModePipe)
ON_COMMAND(ID_MODE_POWER, &AeSys::OnModePower)
ON_COMMAND(ID_MODE_REVISE, &AeSys::OnModeRevise)
ON_COMMAND(ID_MODE_TRAP, &AeSys::OnModeTrap)
ON_COMMAND(ID_TRAPCOMMANDS_ADDGROUPS, &AeSys::OnTrapCommandsAddGroups)
ON_COMMAND(ID_TRAPCOMMANDS_HIGHLIGHT, &AeSys::OnTrapCommandsHighlight)
ON_COMMAND(ID_VIEW_MODEINFORMATION, &AeSys::OnViewModeInformation)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_UPDATE_COMMAND_UI(ID_EDIT_CF_GROUPS, &AeSys::OnUpdateEditCfGroups)
ON_UPDATE_COMMAND_UI(ID_EDIT_CF_IMAGE, &AeSys::OnUpdateEditCfImage)
ON_UPDATE_COMMAND_UI(ID_EDIT_CF_TEXT, &AeSys::OnUpdateEditCfText)
ON_UPDATE_COMMAND_UI(ID_MODE_ANNOTATE, &AeSys::OnUpdateModeAnnotate)
ON_UPDATE_COMMAND_UI(ID_MODE_CUT, &AeSys::OnUpdateModeCut)
ON_UPDATE_COMMAND_UI(ID_MODE_DIMENSION, &AeSys::OnUpdateModeDimension)
ON_UPDATE_COMMAND_UI(ID_MODE_DRAW, &AeSys::OnUpdateModeDraw)
ON_UPDATE_COMMAND_UI(ID_MODE_DRAW2, &AeSys::OnUpdateModeDraw2)
ON_UPDATE_COMMAND_UI(ID_MODE_EDIT, &AeSys::OnUpdateModeEdit)
ON_UPDATE_COMMAND_UI(ID_MODE_FIXUP, &AeSys::OnUpdateModeFixup)
ON_UPDATE_COMMAND_UI(ID_MODE_LPD, &AeSys::OnUpdateModeLpd)
ON_UPDATE_COMMAND_UI(ID_MODE_NODAL, &AeSys::OnUpdateModeNodal)
ON_UPDATE_COMMAND_UI(ID_MODE_PIPE, &AeSys::OnUpdateModePipe)
ON_UPDATE_COMMAND_UI(ID_MODE_POWER, &AeSys::OnUpdateModePower)
ON_UPDATE_COMMAND_UI(ID_MODE_TRAP, &AeSys::OnUpdateModeTrap)
ON_UPDATE_COMMAND_UI(ID_VIEW_MODEINFORMATION, &AeSys::OnUpdateViewModeinformation)
ON_UPDATE_COMMAND_UI(ID_TRAPCOMMANDS_ADDGROUPS, &AeSys::OnUpdateTrapcommandsAddgroups)
ON_UPDATE_COMMAND_UI(ID_TRAPCOMMANDS_HIGHLIGHT, &AeSys::OnUpdateTrapcommandsHighlight)
#pragma warning(pop)
END_MESSAGE_MAP()

// AeSys construction

AeSys::AeSys() {
  m_PegDocTemplate = nullptr;
  m_TracingDocTemplate = nullptr;

  EnableHtmlHelp();

  // Detect color depth. 256 color toolbars can be used in the high or true color modes only (bits per pixel is > 8):
  CClientDC dc(AfxGetMainWnd());
  m_HighColorMode = dc.GetDeviceCaps(BITSPIXEL) > 8;

  m_ClipboardDataImage = false;
  m_ClipboardDataEoGroups = true;
  m_ClipboardDataText = true;
  m_ModeInformationOverView = false;
  m_TrapModeAddGroups = true;
  m_NodalModeAddGroups = true;
  m_ClipboardFormatIdentifierForEoGroups = 0;
  m_EngagedLength = 0.0;
  m_EngagedAngle = 0.0;
  m_DimensionLength = 0.125;
  m_DimensionAngle = 45.;
  m_Units = kInches;
  m_ArchitecturalUnitsFractionPrecision = 16;
  m_SimplexStrokeFont = 0;
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

  EnableUserTools(ID_TOOLS_ENTRY, ID_USER_TOOL1, ID_USER_TOOL10, RUNTIME_CLASS(CUserTool), IDR_MENU_ARGS, IDR_MENU_DIRS);

  // Initialize common controls required to enable visual styles.
  INITCOMMONCONTROLSEX InitCtrls{};
  InitCtrls.dwSize = sizeof(InitCtrls);
  InitCtrls.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&InitCtrls);

  CMFCButton::EnableWindowsTheming();

  CWinAppEx::InitInstance();

#if defined(USING_Direct2D)
  if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_Direct2dFactory))) { return FALSE; }
#endif  // USING_Direct2D

  // Register document templates

  auto* documentClass = RUNTIME_CLASS(AeSysDoc);
  auto* frameClass = RUNTIME_CLASS(CChildFrame);
  auto* viewClass = RUNTIME_CLASS(AeSysView);

  m_PegDocTemplate = new CMultiDocTemplate(IDR_AESYSTYPE, documentClass, frameClass, viewClass);
  AddDocTemplate(m_PegDocTemplate);

  m_TracingDocTemplate = new CMultiDocTemplate(IDR_TRACINGTYPE, documentClass, frameClass, viewClass);
  AddDocTemplate(m_TracingDocTemplate);

  // Create main MDI Frame window
  CMainFrame* mainFrame = new CMainFrame;
  if (!mainFrame || !mainFrame->LoadFrame(IDR_MAINFRAME)) {
    delete mainFrame;
    return FALSE;
  }
  m_pMainWnd = mainFrame;
  mainFrame->DragAcceptFiles();

  CDC* DeviceContext = mainFrame->GetDC();
  m_DeviceWidthInPixels = static_cast<double>(DeviceContext->GetDeviceCaps(HORZRES));
  m_DeviceHeightInPixels = static_cast<double>(DeviceContext->GetDeviceCaps(VERTRES));
  m_DeviceWidthInMillimeters = static_cast<double>(DeviceContext->GetDeviceCaps(HORZSIZE));
  m_DeviceHeightInMillimeters = static_cast<double>(DeviceContext->GetDeviceCaps(VERTSIZE));
  InitGbls(DeviceContext);
  mainFrame->ReleaseDC(DeviceContext);

  // Parse command line and process shell commands
  CCommandLineInfo CommandLineInfo;
  ParseCommandLine(CommandLineInfo);

  if (CommandLineInfo.m_nShellCommand == CCommandLineInfo::FileNew) {
    if (!mainFrame->LoadMDIState(GetRegSectionPath())) { m_PegDocTemplate->OpenDocumentFile(nullptr); }
  } else {
    if (!ProcessShellCommand(CommandLineInfo)) { return FALSE; }
  }
  m_MainFrameMenuHandle = LoadMenuW(m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));

  if (!RegisterKeyPlanWindowClass(m_hInstance)) { return FALSE; }
  if (!RegisterPreviewWindowClass(m_hInstance)) { return FALSE; }
  SetShadowFolderPath(L"AeSys Shadow Folder");

  CString ResourceFolder = ResourceFolderPath();
  LoadSimplexStrokeFont(ResourceFolder + L"Simplex.psf");
  LoadHatchesFromFile(ResourceFolder + L"Hatches\\DefaultSet.txt");
  //LoadPenColorsFromFile(ResourceFolder + L"Pens\\Colors\\Default.txt"));

#if defined(USING_DDE)
  // Initialize for using DDEML
  dde::Setup(m_hInstance);
#endif  // USING_DDE

  OnModeDraw();

  // This is the private data format used to pass EoGroups from one instance to another
  m_ClipboardFormatIdentifierForEoGroups = RegisterClipboardFormatW(L"EoGroups");

  m_pMainWnd->ShowWindow(m_nCmdShow);
  m_pMainWnd->UpdateWindow();

  return TRUE;
}

// AeSys message handlers
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
  CString FormatSpecification = EoAppLoadStringResource(stringResourceIdentifier);

  CString FormattedResourceString;
  FormattedResourceString.Format(FormatSpecification, string.GetString());

  int NextToken = 0;
  CString Message = FormattedResourceString.Tokenize(L"\t", NextToken);
  CString Caption = FormattedResourceString.Tokenize(L"\n", NextToken);

  return (MessageBoxW(0, Message, Caption, MB_ICONINFORMATION | MB_YESNOCANCEL | MB_DEFBUTTON2));
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
void AeSys::AddModeInformationToMessageList() {
  CString ResourceString = EoAppLoadStringResource(static_cast<UINT>(m_CurrentMode));
  int NextToken = 0;
  ResourceString = ResourceString.Tokenize(L"\n", NextToken);
  AddStringToMessageList(ResourceString);
}
void AeSys::AddStringToMessageList(UINT stringResourceIdentifier) {
  CString ResourceString = EoAppLoadStringResource(stringResourceIdentifier);

  AddStringToMessageList(ResourceString);
}
void AeSys::AddStringToMessageList(UINT stringResourceIdentifier, const CString& string) {
  CString FormatSpecification = EoAppLoadStringResource(stringResourceIdentifier);

  CString FormattedResourceString;
  FormattedResourceString.Format(FormatSpecification, string.GetString());

  AddStringToMessageList(FormattedResourceString);
}
void AeSys::WarningMessageBox(UINT stringResourceIdentifier) {
  CString ResourceString = EoAppLoadStringResource(stringResourceIdentifier);

  int NextToken = 0;
  CString Message = ResourceString.Tokenize(L"\t", NextToken);
  CString Caption = ResourceString.Tokenize(L"\n", NextToken);

  MessageBoxW(0, Message, Caption, MB_ICONWARNING | MB_OK);
}
void AeSys::WarningMessageBox(UINT stringResourceIdentifier, const CString& string) {
  CString FormatSpecification = EoAppLoadStringResource(stringResourceIdentifier);

  CString FormattedResourceString;
  FormattedResourceString.Format(FormatSpecification, string.GetString());

  int NextToken = 0;
  CString Message = FormattedResourceString.Tokenize(L"\t", NextToken);
  CString Caption = FormattedResourceString.Tokenize(L"\n", NextToken);

  MessageBoxW(0, Message, Caption, MB_ICONWARNING | MB_OK);
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
void AeSys::OnModeAnnotate() {
  m_ModeResourceIdentifier = IDR_ANNOTATE_MODE;
  m_PrimaryMode = ID_MODE_ANNOTATE;
  LoadModeResources(ID_MODE_ANNOTATE);
}
void AeSys::OnModeCut() {
  m_ModeResourceIdentifier = IDR_CUT_MODE;
  m_PrimaryMode = ID_MODE_CUT;
  LoadModeResources(ID_MODE_CUT);
}
void AeSys::OnModeDimension() {
  m_ModeResourceIdentifier = IDR_DIMENSION_MODE;
  m_PrimaryMode = ID_MODE_DIMENSION;
  LoadModeResources(ID_MODE_DIMENSION);
}
void AeSys::OnModeDraw() {
  m_ModeResourceIdentifier = IDR_DRAW_MODE;
  m_PrimaryMode = ID_MODE_DRAW;
  LoadModeResources(ID_MODE_DRAW);
}
void AeSys::OnModeDraw2() {
  m_ModeResourceIdentifier = IDR_DRAW2_MODE;
  m_PrimaryMode = ID_MODE_DRAW2;
  LoadModeResources(ID_MODE_DRAW2);
}
void AeSys::OnModeEdit() {
  m_ModeResourceIdentifier = IDR_EDIT_MODE;
  LoadModeResources(ID_MODE_EDIT);
}
void AeSys::OnModeFixup() {
  m_ModeResourceIdentifier = IDR_FIXUP_MODE;
  LoadModeResources(ID_MODE_FIXUP);
}
void AeSys::OnModeLetter() {
  EoDlgModeLetter Dialog;
  Dialog.DoModal();
}
void AeSys::OnModeLPD() {
  m_ModeResourceIdentifier = IDR_LPD_MODE;
  LoadModeResources(ID_MODE_LPD);
}
void AeSys::OnModeNodal() {
  m_ModeResourceIdentifier = IDR_NODAL_MODE;
  LoadModeResources(ID_MODE_NODAL);
}
void AeSys::OnModePipe() {
  m_ModeResourceIdentifier = IDR_PIPE_MODE;
  LoadModeResources(ID_MODE_PIPE);
}
void AeSys::OnModePower() {
  m_ModeResourceIdentifier = IDR_POWER_MODE;
  LoadModeResources(ID_MODE_POWER);
}
void AeSys::OnModeRevise() {
  EoDlgModeRevise Dialog;
  Dialog.DoModal();
}
void AeSys::OnModeTrap() {
  if (m_TrapModeAddGroups) {
    m_ModeResourceIdentifier = IDR_TRAP_MODE;
    LoadModeResources(ID_MODE_TRAP);
  } else {
    m_ModeResourceIdentifier = IDR_TRAPR_MODE;
    LoadModeResources(ID_MODE_TRAPR);
  }
}
void AeSys::OnTrapCommandsAddGroups() {
  m_TrapModeAddGroups = !m_TrapModeAddGroups;
  m_CurrentMode = m_TrapModeAddGroups ? ID_MODE_TRAP : ID_MODE_TRAPR;

  OnModeTrap();
}
void AeSys::OnFileRun() {
  CString Filter = EoAppLoadStringResource(IDS_OPENFILE_FILTER_APPS);

  CFileDialog dlg(TRUE, L"exe", L"*.exe", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, Filter);
  dlg.m_ofn.lpstrTitle = L"Run Application";

  if (dlg.DoModal() == IDOK) {
    CString strFile = dlg.GetFileName();
    //Note: use of winexec should be replaced with createprocess
    //WinExec(strFile, SW_SHOW);
  }
}
void AeSys::OnHelpContents() { ::WinHelpW(GetSafeHwnd(), L"peg.hlp", HELP_CONTENTS, 0L); }

double AeSys::PenWidthsGet(EoInt16 penIndex) { return (penWidths[penIndex]); }

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

EoDb::FileTypes AeSys::GetFileTypeFromPath(const CString& pathName) {
  EoDb::FileTypes Type(EoDb::kUnknown);
  CString Extension = pathName.Right(3);

  if (!Extension.IsEmpty()) {
    if (Extension.CompareNoCase(L"peg") == 0) {
      Type = EoDb::kPeg;
    } else if (Extension.CompareNoCase(L"tra") == 0) {
      Type = EoDb::kTracing;
    } else if (Extension.CompareNoCase(L"jb1") == 0) {
      Type = EoDb::kJob;
    } else if (Extension.CompareNoCase(L"dwg") == 0) {
      Type = EoDb::kDwg;
    } else if (Extension.CompareNoCase(L"dxf") == 0) {
      Type = EoDb::kDxf;
    } else if (Extension.CompareNoCase(L"dxb") == 0) {
      Type = EoDb::kDxb;
    }
  }
  return Type;
}
CString AeSys::ResourceFolderPath() {
  CString ApplicationPath = EoAppGetPathFromCommandLine();
  return ApplicationPath + L"\\res\\";
}
int AeSys::SetShadowFolderPath(const CString& folder) {
  WCHAR Path[MAX_PATH];

  if (SHGetSpecialFolderPathW(m_pMainWnd->GetSafeHwnd(), Path, CSIDL_PERSONAL, TRUE)) {
    m_ShadowFolderPath = Path;
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
void AeSys::SetCursorPosition(EoGePoint3d pt) {
  auto* activeView = AeSysView::GetActiveView();
  activeView->SetCursorPosition(pt);
}
// Loads the hatch table.
void AeSys::LoadHatchesFromFile(const CString& strFileName) {
  CFileException e;
  CStdioFile fl;

  if (!fl.Open(strFileName, CFile::modeRead | CFile::typeText, &e)) return;

  WCHAR szLn[128];
  double dTotStrsLen;
  int iNmbEnts, iNmbStrsId;

  WCHAR szValDel[] = L",\0";
  int iHatId = 0;
  int iNmbHatLns = 0;
  int iTblId = 0;

  while (fl.ReadString(szLn, sizeof(szLn) / sizeof(WCHAR) - 1) != 0) {
    if (*szLn == '!') {  // New Hatch index
      if (iHatId != 0) hatch::fTableValue[hatch::iTableOffset[iHatId]] = float(iNmbHatLns);
      hatch::iTableOffset[++iHatId] = iTblId++;
      iNmbHatLns = 0;
    } else {
      iNmbStrsId = iTblId;
      iTblId += 2;
      iNmbEnts = 0;
      dTotStrsLen = 0.;
      LPWSTR NextToken = nullptr;
      LPWSTR pTok = wcstok_s(szLn, szValDel, &NextToken);
      while (pTok != 0) {
        volatile double tempValue = _wtof(pTok);
        hatch::fTableValue[iTblId] = static_cast<float>(tempValue);
        iNmbEnts++;
        if (iNmbEnts >= 6) dTotStrsLen = dTotStrsLen + hatch::fTableValue[iTblId];
        iTblId++;
        pTok = wcstok_s(0, szValDel, &NextToken);
      }
      hatch::fTableValue[iNmbStrsId++] = float(iNmbEnts - 5);
      hatch::fTableValue[iNmbStrsId] = float(dTotStrsLen);
      iNmbHatLns++;
    }
  }
}
EoGePoint3d AeSys::HomePointGet(int i) const {
  if (i >= 0 && i < 9) return (m_HomePoints[i]);

  return (EoGePoint3d::kOrigin);
}
void AeSys::HomePointSave(int i, const EoGePoint3d& pt) {
  if (i >= 0 && i < 9) m_HomePoints[i] = pt;
}
//Initializes all peg global sections to their default (startup) values.
void AeSys::InitGbls(CDC* deviceContext) {
  pstate.SetPolygonIntStyle(EoDb::kHollow);

  pstate.SetPolygonIntStyleId(1);

  hatch::dXAxRefVecScal = 0.1;
  hatch::dYAxRefVecScal = 0.1;
  hatch::dOffAng = 0.;

  EoDbCharacterCellDefinition ccd;
  pstate.SetCharCellDef(ccd);

  EoDbFontDefinition fd;
  pstate.SetFontDef(deviceContext, fd);

  SetUnits(kInches);
  SetArchitecturalUnitsFractionPrecision(8);
  SetDimensionLength(0.125);
  SetDimensionAngle(45.0);

  m_TrapHighlighted = true;
  m_TrapHighlightColor = 15;

  //Document->InitializeGroupAndPrimitiveEdit();
  pstate.SetPen(nullptr, deviceContext, 1, 1);
  pstate.SetPointStyle(0);
}
void AeSys::EditColorPalette() {
  CHOOSECOLOR cc;

  ::ZeroMemory(&cc, sizeof(CHOOSECOLOR));
  cc.lStructSize = sizeof(CHOOSECOLOR);

  cc.rgbResult = ColorPalette[pstate.PenColor()];
  cc.lpCustColors = ColorPalette;
  cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_SOLIDCOLOR;
  ::ChooseColor(&cc);

  cc.rgbResult = GreyPalette[pstate.PenColor()];
  cc.lpCustColors = GreyPalette;
  ::ChooseColor(&cc);

  MessageBoxW(nullptr, L"The background color is no longer associated with the pen Color Palette.", L"Deprecation Notice", MB_OK | MB_ICONINFORMATION);

  AeSysDoc::GetDoc()->UpdateAllViews(nullptr, 0L, nullptr);
}
// Loads the color table.
void AeSys::LoadPenColorsFromFile(const CString& strFileName) {
  CStdioFile fl;

  if (fl.Open(strFileName, CFile::modeRead | CFile::typeText)) {
    WCHAR pBuf[128];
    LPWSTR pId, pRed, pGreen, pBlue;

    while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(WCHAR) - 1) != 0 && _tcsnicmp(pBuf, L"<Colors>", 8) != 0);

    while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(WCHAR) - 1) != 0 && *pBuf != '<') {
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
void AeSys::LoadModeResources(int mode) {
  BuildModifiedAcceleratorTable();

  m_CurrentMode = mode;
  AddModeInformationToMessageList();

  auto* activeView = AeSysView::GetActiveView();
  if (activeView != 0) {
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
COLORREF AppGetTextCol() { return (~(ViewBackgroundColor | 0xff000000)); }
void AeSys::OnUpdateModeDraw(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_DRAW); }
void AeSys::OnUpdateModeAnnotate(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_ANNOTATE); }
void AeSys::OnUpdateModeTrap(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_TRAP); }
void AeSys::OnUpdateModeEdit(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_EDIT); }
void AeSys::OnUpdateModeDraw2(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_DRAW2); }
void AeSys::OnUpdateModeDimension(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_DIMENSION); }
void AeSys::OnUpdateModeCut(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_CUT); }
void AeSys::OnUpdateModeNodal(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_NODAL); }
void AeSys::OnUpdateModeFixup(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_FIXUP); }
void AeSys::OnUpdateModeLpd(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_LPD); }
void AeSys::OnUpdateModePower(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_POWER); }
void AeSys::OnUpdateModePipe(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_CurrentMode == ID_MODE_PIPE); }
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

  HACCEL ModeAcceleratorTableHandle = ::LoadAccelerators(m_hInstance, MAKEINTRESOURCE(m_ModeResourceIdentifier));
  int ModeAcceleratorTableEntries = CopyAcceleratorTableW(ModeAcceleratorTableHandle, nullptr, 0);

  AcceleratorTableHandle = ::LoadAccelerators(m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
  int AcceleratorTableEntries = CopyAcceleratorTableW(AcceleratorTableHandle, nullptr, 0);

  LPACCEL ModifiedAcceleratorTable = new ACCEL[static_cast<size_t>(AcceleratorTableEntries + ModeAcceleratorTableEntries)];

  CopyAcceleratorTableW(ModeAcceleratorTableHandle, ModifiedAcceleratorTable, ModeAcceleratorTableEntries);
  CopyAcceleratorTableW(AcceleratorTableHandle, &ModifiedAcceleratorTable[ModeAcceleratorTableEntries], AcceleratorTableEntries);

  MainFrame->m_hAccelTable = ::CreateAcceleratorTable(ModifiedAcceleratorTable, AcceleratorTableEntries + ModeAcceleratorTableEntries);

  delete[] ModifiedAcceleratorTable;
}
void AeSys::OnFileOpen() {
  CString Filter = EoAppLoadStringResource(IDS_OPENFILE_FILTER);

  DWORD Flags(OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);

  CFileDialog FileDialog(TRUE, nullptr, nullptr, Flags, Filter);

  CString FileName;
  FileDialog.m_ofn.lpstrFile = FileName.GetBuffer(_MAX_PATH);

  CString Title = EoAppLoadStringResource(AFX_IDS_OPENFILE);
  FileDialog.m_ofn.lpstrTitle = Title;

  INT_PTR result = FileDialog.DoModal();
  FileName.ReleaseBuffer();

  if (result == IDOK) { OpenDocumentFile(FileName); }
}
int AeSys::GreatestCommonDivisor(const int number1, const int number2) {
  int ReturnValue = abs(number1);
  int Divisor = abs(number2);
  while (Divisor != 0) {
    int Remainder = ReturnValue % Divisor;
    ReturnValue = Divisor;
    Divisor = Remainder;
  }
  return (ReturnValue);
}

void AeSys::FormatAngle(CString& angleAsString, const double angle, const int width, const int precision) {
  CString FormatSpecification;
  FormatSpecification.Format(L"%%%i.%if\u00B0", width, precision);
  angleAsString.Format(FormatSpecification, Eo::RadianToDegree(angle));
}

void AeSys::FormatLength(CString& lengthAsString, Units units, const double length, const int minWidth, const int precision) {
  const size_t bufSize{32};
  auto lengthAsBuffer = lengthAsString.GetBufferSetLength(bufSize);

  if (units == kArchitectural || units == kArchitecturalS) {
    FormatLengthArchitectural(lengthAsBuffer, bufSize, units, length);
  } else if (units == kEngineering) {
    FormatLengthEngineering(lengthAsBuffer, bufSize, length, minWidth, precision);
  } else {
    FormatLengthSimple(lengthAsBuffer, bufSize, units, length, minWidth, precision);
  }
  lengthAsString.ReleaseBuffer();
}

void AeSys::FormatLengthArchitectural(LPWSTR lengthAsBuffer, const size_t bufSize, Units units, const double length) {
  WCHAR szBuf[16]{};

  double ScaledLength = length * AeSysView::GetActiveView()->GetWorldScale();

  wcscpy_s(lengthAsBuffer, bufSize, (length >= 0.0) ? L" " : L"-");
  ScaledLength = fabs(ScaledLength);

  int Feet = int(ScaledLength / 12.0);
  int Inches = abs(int(fmod(ScaledLength, 12.0)));

  int FractionPrecision = GetArchitecturalUnitsFractionPrecision();
  int Numerator = int(fabs(fmod(ScaledLength, 1.0)) * (double)(FractionPrecision) + 0.5);

  if (Numerator == FractionPrecision) {
    if (Inches == 11) {
      Feet++;
      Inches = 0;
    } else {
      Inches++;
    }
    Numerator = 0;
  }
  _itow_s(Feet, szBuf, 16, 10);
  wcscat_s(lengthAsBuffer, bufSize, szBuf);
  wcscat_s(lengthAsBuffer, bufSize, L"'");

  _itow_s(Inches, szBuf, 16, 10);
  wcscat_s(lengthAsBuffer, bufSize, szBuf);
  if (Numerator > 0) {
    wcscat_s(lengthAsBuffer, static_cast<size_t>(bufSize), (units == kArchitecturalS) ? L"\\S" : L"-");
    int iGrtComDivisor = GreatestCommonDivisor(Numerator, FractionPrecision);
    Numerator /= iGrtComDivisor;
    int Denominator = FractionPrecision / iGrtComDivisor;
    _itow_s(Numerator, szBuf, 16, 10);
    wcscat_s(lengthAsBuffer, bufSize, szBuf);
    wcscat_s(lengthAsBuffer, bufSize, L"/");
    _itow_s(Denominator, szBuf, 16, 10);
    wcscat_s(lengthAsBuffer, bufSize, szBuf);
    if (units == kArchitecturalS) wcscat_s(lengthAsBuffer, bufSize, L";");
  }
  wcscat_s(lengthAsBuffer, bufSize, L"\"");
}
void AeSys::FormatLengthEngineering(LPWSTR lengthAsBuffer, const size_t bufSize, const double length, const int width, const int precision) {
  WCHAR szBuf[16]{};

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
void AeSys::FormatLengthSimple(LPWSTR lengthAsBuffer, const size_t bufSize, Units units, const double length, const int width, const int precision) {
  double ScaledLength = length * AeSysView::GetActiveView()->GetWorldScale();

  CString formatSpecification;
  formatSpecification.Format(L"%%%i.%if", width, precision);
  CString formatted{};

  switch (units) {
    case kFeet:
      formatSpecification.Append(L"'");
      formatted.Format(formatSpecification, ScaledLength / 12.0);
      break;
    case kInches:
      formatSpecification.Append(L"\"");
      formatted.Format(formatSpecification, ScaledLength);
      break;
    case kMeters:
      formatSpecification.Append(L"m");
      formatted.Format(formatSpecification, ScaledLength * 0.0254);
      break;
    case kMillimeters:
      formatSpecification.Append(L"mm");
      formatted.Format(formatSpecification, ScaledLength * 25.4);
      break;
    case kCentimeters:
      formatSpecification.Append(L"cm");
      formatted.Format(formatSpecification, ScaledLength * 2.54);
      break;
    case kDecimeters:
      formatSpecification.Append(L"dm");
      formatted.Format(formatSpecification, ScaledLength * 0.254);
      break;
    case kKilometers:
      formatSpecification.Append(L"km");
      formatted.Format(formatSpecification, ScaledLength * 0.0000254);
      break;

    case kArchitectural:
    case kArchitecturalS:
    case kEngineering:
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
  wchar_t token[32]{0};
  int linePosition{0};
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
        wchar_t* fractionEnd{nullptr};
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

double AeSys::ParseLength(wchar_t* inputLine) {
  wchar_t* end{nullptr};

  // Parse the leading numeric portion of the string or possible the numerator of a fraction
  double length = wcstod(inputLine, &end);

  if (end == inputLine) { throw L"Invalid length format."; }
  // The only valid case for a leading fraction is a variation of SimpleUnitsLengthToken `{sign}{fraction}(\'|\"|{metric_units})`
  if (*end == L'/') {
    wchar_t* fractionEnd{nullptr};
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
double AeSys::ParseLength(Units units, wchar_t* inputLine) {
  try {
    int iTokId{0};
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

    if (iTyp == lex::ArchitecturalUnitsLengthToken || iTyp == lex::EngineeringUnitsLengthToken || iTyp == lex::SimpleUnitsLengthToken) {
      return (length[0]);
    } else {
      lex::ConvertValTyp(iTyp, lex::RealToken, &lDef, length);

      switch (units) {
        case kArchitectural:
        case kEngineering:
        case kFeet:
          length[0] *= 12.0;
          break;

        case kMeters:
          length[0] *= 39.37007874015748;
          break;

        case kMillimeters:
          length[0] *= 0.03937007874015748;
          break;

        case kCentimeters:
          length[0] *= 0.3937007874015748;
          break;

        case kDecimeters:
          length[0] *= 3.937007874015748;
          break;

        case kKilometers:
          length[0] *= 39370.07874015748;
          break;

        case AeSys::kArchitecturalS:
        case AeSys::kInches:
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

CString EoAppLoadStringResource(UINT resourceIdentifier) {
  CString String;
  VERIFY(String.LoadStringW(resourceIdentifier) == TRUE);
  return String;
}
CString EoAppGetPathFromCommandLine() {
  CString PathName = ::GetCommandLineW();
  int LastPathDelimiter = PathName.ReverseFind(L'\\');
  PathName = PathName.Mid(1, LastPathDelimiter - 1);

  return PathName;
}
