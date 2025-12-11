#include "stdafx.h"

#include "MainFrm.h"
#include "ChildFrm.h"

#if defined(DWGDIRECT)
#include "ColorMapping.h"
#include "RxDynamicModule.h"
#endif // DWGDIRECT

#include "EoApOptions.h"

#include "Directory.h"
#include "FileBitmap.h"
#include "ddeGItms.h"
#include "DlgProcEditOps.h"
#include "Hatch.h"
#include "UnitsString.h"
#include "Dde.h"
#include "Lex.h"

LRESULT CALLBACK WndProcKeyPlan(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProcPreview(HWND, UINT, WPARAM, LPARAM);

namespace hatch
{
	double dXAxRefVecScal;
	double dYAxRefVecScal;
	double dOffAng;
	int iTableOffset[64];
	float fTableValue[1536];
}
double dPWids[] = 
{
	0., .0075, .015, .02, .03, .0075, .015, .0225, .03, .0075, .015, .0225, .03, .0075, .015, .0225
};

#include "PegColors.h"
#include "AeSysApp.h"

COLORREF* pColTbl = crHotCols;

CPrimState pstate;
CModelTransform mspace;

TCHAR AeSysApp::szLeftMouseDown[60]  = _T("");
TCHAR AeSysApp::szRightMouseDown[60] = _T("");
TCHAR AeSysApp::szLeftMouseUp[60]	= _T("{13}");
TCHAR AeSysApp::szRightMouseUp[60] = _T("{27}");

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined(DWGDIRECT)
ODRX_DECLARE_STATIC_MODULE_ENTRY_POINT(WinGDIModule);
ODRX_BEGIN_STATIC_MODULE_MAP()
	ODRX_DEFINE_STATIC_APPMODULE(L"WinGDI.txv", WinGDIModule)
ODRX_END_STATIC_MODULE_MAP()
#endif // DWGDIRECT

// AeSysApp

BEGIN_MESSAGE_MAP(AeSysApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)

	ON_COMMAND(ID_FILE_RUN, OnFileRun)
	ON_COMMAND(ID_HELP_CONTENTS, OnHelpContents)
	ON_COMMAND(ID_MODE_ANNOTATE, OnModeAnnotate)
	ON_UPDATE_COMMAND_UI(ID_MODE_ANNOTATE, OnUpdateModeAnnotate)
	ON_COMMAND(ID_MODE_CUT, OnModeCut)
	ON_UPDATE_COMMAND_UI(ID_MODE_CUT, OnUpdateModeCut)
	ON_COMMAND(ID_MODE_DIMENSION, OnModeDimension)
	ON_UPDATE_COMMAND_UI(ID_MODE_DIMENSION, OnUpdateModeDimension)
	ON_COMMAND(ID_MODE_DRAW, OnModeDraw)
	ON_UPDATE_COMMAND_UI(ID_MODE_DRAW, OnUpdateModeDraw)
	ON_COMMAND(ID_MODE_DRAW2, OnModeDraw2)
	ON_UPDATE_COMMAND_UI(ID_MODE_DRAW2, OnUpdateModeDraw2)
	ON_COMMAND(ID_MODE_EDIT, OnModeEdit)
	ON_UPDATE_COMMAND_UI(ID_MODE_EDIT, OnUpdateModeEdit)
	ON_COMMAND(ID_MODE_FIXUP, OnModeFixup)
	ON_UPDATE_COMMAND_UI(ID_MODE_FIXUP, OnUpdateModeFixup)
	ON_COMMAND(ID_MODE_LETTER, OnModeLetter)
	ON_COMMAND(ID_MODE_LPD, OnModeLPD)
	ON_UPDATE_COMMAND_UI(ID_MODE_LPD, OnUpdateModeLpd)
	ON_COMMAND(ID_MODE_NODAL, OnModeNodal)
	ON_UPDATE_COMMAND_UI(ID_MODE_NODAL, OnUpdateModeNodal)
	ON_COMMAND(ID_MODE_PIPE, OnModePipe)
	ON_UPDATE_COMMAND_UI(ID_MODE_PIPE, OnUpdateModePipe)
	ON_COMMAND(ID_MODE_POWER, OnModePower)
	ON_UPDATE_COMMAND_UI(ID_MODE_POWER, OnUpdateModePower)
	ON_COMMAND(ID_MODE_REVISE, OnModeRevise)
	ON_COMMAND(ID_MODE_TRAP, OnModeTrap)
	ON_UPDATE_COMMAND_UI(ID_MODE_TRAP, OnUpdateModeTrap)
	ON_COMMAND(ID_TRAPCOMMANDS_ADDGROUPS, OnTrapCommandsAddGroups)
	ON_COMMAND(ID_TRAPCOMMANDS_HIGHLIGHT, OnTrapCommandsHighlight)
	ON_COMMAND(ID_VIEW_MODEINFORMATION, OnViewModeInformation)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MODEINFORMATION, OnUpdateViewModeinformation)
	ON_COMMAND(ID_VIEW_STATEINFORMATION, OnViewStateInformation)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STATEINFORMATION, OnUpdateViewStateinformation)
END_MESSAGE_MAP()

// AeSysApp construction

AeSysApp::AeSysApp() :
	CWinAppEx(TRUE)
{
	m_PegDocTemplate = NULL;
	m_TracingDocTemplate = NULL;
	
	EnableHtmlHelp();
	
	// Detect color depth. 256 color toolbars can be used in the
	// high or true color modes only (bits per pixel is > 8):
	CClientDC dc(AfxGetMainWnd());
	m_bHiColorIcons = dc.GetDeviceCaps(BITSPIXEL) > 8;
	
	SetShadowDir(_T("AeSys Shadow Files"));
	
	m_bEditCFImage = false;
	m_bEditCFGroups = true;
	m_bEditCFText = true;
	m_bViewStateInfo = true;		// View state info within the view
	m_bViewModeInfo = false;			// View mode info within the view
	m_TrapModeAddGroups = true;
	m_NodalModeAddGroups = true;
	m_nClipboardFormatEoGroups = 0;
	m_dEngLen = 0.;
	m_dEngAngZ = 0.;
	m_dDimLen = 0.;
	m_dDimAngZ = 0.;
	m_dScale = 1.;
	m_iUnitsPrec = 8;
	m_eUnits = EoDb::kInches;
	m_pStrokeFontDef = 0;

	InitializeConstraints();

	m_AmbientLight[0] = .2f;	// ambient light intensity (range 0. to 1.)
	m_AmbientLight[1] = .2f;
	m_AmbientLight[2] = .2f;
	m_AmbientLight[3] = 1.f;
	
	m_L0Ambient[0] = 0.f;
	m_L0Ambient[1] = 0.f;
	m_L0Ambient[2] = 0.f;
	m_L0Ambient[3] = 1.f;
	
	m_L0Diffuse[0] = 1.f;
	m_L0Diffuse[1] = 1.f;
	m_L0Diffuse[2] = 1.f;
	m_L0Diffuse[3] = 1.f;
	
	m_L0Position[0] = 0.;	// point light source position in world space
	m_L0Position[1] = 0.;
	m_L0Position[2] = 1000.;
	m_L0Position[3] = 1.;

	m_L0Specular[0] = 1.f;	// incident intensity from point light source
	m_L0Specular[1] = 1.f;
	m_L0Specular[2] = 1.f;
	m_L0Specular[3] = 1.f;

	m_MatAmbient[0] = .2f;
	m_MatAmbient[1] = .2f;
	m_MatAmbient[2] = .2f;
	m_MatAmbient[3] = 1.f;

	m_MatDiffuse[0] = .8f;
	m_MatDiffuse[1] = .8f;
	m_MatDiffuse[2] = .8f;
	m_MatDiffuse[3] = 1.f;

	m_MatSpecular[0] = 0.f;
	m_MatSpecular[1] = 0.f;
	m_MatSpecular[2] = 0.f;
	m_MatSpecular[3] = 1.f;
}

// The one and only AeSys object

AeSysApp app;

// AeSysApp initialization

BOOL AeSysApp::InitInstance()
{
	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();

	// Standard initialization
	
	// Set the registry key under which our settings are stored
	SetRegistryKey(_T("Engineers Office"));
	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	SetRegistryBase(_T("Settings"));

	m_Options.Load();
	lex::Init();

	// Initialize all Managers for usage. They are automatically constructed
	// if not yet present
	InitContextMenuManager();
	InitKeyboardManager();
	InitTooltipManager();

	CMFCToolTipInfo params;
	params.m_bVislManagerTheme = TRUE;

	GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS (CMFCToolTipCtrl), &params);

	EnableUserTools(ID_TOOLS_ENTRY, ID_USER_TOOL1, ID_USER_TOOL10, RUNTIME_CLASS(CUserTool), IDR_MENU_ARGS, IDR_MENU_DIRS);


	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

#if defined(DWGDIRECT)
	ODRX_INIT_STATIC_MODULE_MAP();
	odInitialize(this);
#endif // DWGDIRECT

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	
	m_PegDocTemplate = new CMultiDocTemplate(IDR_AESYSTYPE,
		RUNTIME_CLASS(AeSysDoc), RUNTIME_CLASS(CChildFrame), RUNTIME_CLASS(AeSysView));
	AddDocTemplate(m_PegDocTemplate);

	m_TracingDocTemplate = new CMultiDocTemplate(IDR_TRACINGTYPE,
		RUNTIME_CLASS(AeSysDoc), RUNTIME_CLASS(CChildFrame), RUNTIME_CLASS(AeSysView));
	AddDocTemplate(m_TracingDocTemplate);
	
	// create main MDI Frame window
	CMainFrame* MainFrame = new CMainFrame;

	if (!MainFrame || !MainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete MainFrame;
		return FALSE;
	}
	m_pMainWnd = MainFrame;
	m_pMainWnd->DragAcceptFiles();

	CDC* DC = m_pMainWnd->GetDC();

	m_DeviceWidthInPixels = DC->GetDeviceCaps(HORZRES);
	m_DeviceHeightInPixels = DC->GetDeviceCaps(VERTRES);
	m_DeviceWidthInMillimeters = DC->GetDeviceCaps(HORZSIZE);
	m_DeviceHeightInMillimeters = DC->GetDeviceCaps(VERTSIZE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo CommandLineInfo;
	ParseCommandLine(CommandLineInfo);

	if (CommandLineInfo.m_nShellCommand == CCommandLineInfo::FileNew)
	{
		if (!MainFrame->LoadMDIState(GetRegSectionPath()))
		{
			m_PegDocTemplate->OpenDocumentFile(NULL);
		}
	}
	else
	{	// Dispatch commands specified on the command line
		if (!ProcessShellCommand(CommandLineInfo))
		{
			return FALSE;
		}
	}
	// Get app path by stripping quotes and program name from command line
	m_strAppPath = ::GetCommandLine();
	Path_UnquoteSpaces(m_strAppPath);

	m_hMenu = ::LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));

	WNDCLASS  wc;

	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= WndProcKeyPlan;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= m_hInstance;
	wc.hIcon			= 0;
	wc.hCursor			= ::LoadCursor(0, IDC_CROSS);
	wc.hbrBackground	= (HBRUSH) ::GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName 	= 0; 
	wc.lpszClassName	= _T("View");

	if (!::RegisterClass(&wc))
		return (FALSE);

	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= WndProcPreview;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= m_hInstance;
	wc.hIcon			= 0;
	wc.hCursor			= ::LoadCursor(0, IDC_CROSS);
	wc.hbrBackground	= (HBRUSH) ::GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName 	= 0; 
	wc.lpszClassName	= _T("Preview");

	if (!::RegisterClass(&wc))
		return (FALSE);

	HatchesLoad(m_strAppPath + _T("\\Hatches\\DefaultSet.txt"));
	PenWidthsLoad(m_strAppPath + _T("\\Pens\\Widths.txt"));
	PenColorsLoad(m_strAppPath + _T("\\Pens\\Colors\\Default.txt"));
	PenStylesLoad(m_strAppPath + _T("\\Pens\\LineTypes.txt"));
	StrokeFontLoad(_T(""));
	
	InitGbls(DC);
	
	SetBackGround(crHotCols[0]);

#if defined(GL_FUNCTIONALITY)
	// Setup pixel format.
	// For now we mix gdi calls so no double buffering enabled

	PIXELFORMATDESCRIPTOR pfd; 
 
	::ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); 
	pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI; 
	pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24; 
    pfd.cAlphaBits = 0;
    pfd.cAccumBits = 0; 
    pfd.cDepthBits = 32;					// 32 bit z-buffer 
    pfd.cStencilBits = 0;					// no stencil buffer
	pfd.cAuxBuffers = 0;					// no auxiliary buffer
	pfd.iLayerType = PFD_MAIN_PLANE;
 
	int Pixelformat = ChoosePixelFormat(DC->GetSafeHdc(), &pfd); 
 
	SetPixelFormat(DC->GetSafeHdc(), Pixelformat, &pfd);

	// Associate a rendering context with the device context of the application window
	// Make it the active context (for now it is also the only context)
	app.hRC = wglCreateContext(DC->GetSafeHdc());
	wglMakeCurrent(DC->GetSafeHdc(), app.hRC);
	
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_POLYGON_SMOOTH);

	// clear buffers with a black pen (or whatever background color is)
	
	COLORREF cr = app.PenColorsGetHot(0);
	glClearColor(GetRValue(cr), GetGValue(cr), GetBValue(cr), 0.);

	// Global ambient light level is same for RGB values (white light only)
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_AmbientLight);
	
	glEnable(GL_LIGHTING);
	
	// Use material properties and lighting parameters to render

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_MatAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m_MatDiffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m_MatSpecular);
		
	// Light 0 is only point light initially
	float Position[] = {float(m_L0Position[0]), float(m_L0Position[1]), float(m_L0Position[2]), 1.f};
	glLightfv(GL_LIGHT0, GL_POSITION, Position);
	
	glLightfv(GL_LIGHT0, GL_AMBIENT, m_L0Ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, m_L0Diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, m_L0Specular);
	glEnable(GL_LIGHT0);
	
	// Enable smooth shading. This is the default. 
	glShadeModel(GL_SMOOTH);
	// Enable hidden surface removal
	glEnable(GL_DEPTH_TEST); 
	glEnable(GL_NORMALIZE);

#endif // GL_FUNCTIONALITY
	OnModeDraw();
	
	// Initialize for using DDEML
	dde::Setup(m_hInstance);

	// This is the private data format used to pass EoGroups from one instance to another
	m_nClipboardFormatEoGroups = RegisterClipboardFormat(_T("EoGroups"));
	
	m_pMainWnd->ShowWindow(m_nCmdShow);
	m_pMainWnd->UpdateWindow();
	
	return TRUE;
}

// AeSysApp message handlers

int AeSysApp::ExitInstance()
{
	m_Options.Save();
	
	dde::Uninitialize();

	return CWinAppEx::ExitInstance();
}
void AeSysApp::PreLoadState()
{
	GetContextMenuManager()->AddMenu(_T("My menu"), IDR_CONTEXT_MENU);

	// TODO: add another context menus here
}
void AeSysApp::UpdateMDITabs(BOOL resetMDIChild)
{
	((CMainFrame*) AfxGetMainWnd())->UpdateMDITabs (resetMDIChild);
}
void AeSysApp::OnTrapCommandsHighlight()
{
	m_bTrapHighlight = !m_bTrapHighlight;

	app.CheckMenuItem(ID_TRAPCOMMANDS_HIGHLIGHT, MF_BYCOMMAND | (m_bTrapHighlight ? MF_CHECKED : MF_UNCHECKED));
	
	//LPARAM lHint = m_bTrapHighlight ? EoDb::kGroupsSafeTrap : EoDb::kGroupsSafe;
	//UpdateAllViews(NULL, lHint, &m_TrappedGroupList);
}
void AeSysApp::OnEditCfImage()
{
	m_bEditCFImage = !m_bEditCFImage;
	app.CheckMenuItem(ID_EDIT_CF_IMAGE, MF_BYCOMMAND | (m_bEditCFImage ? MF_CHECKED : MF_UNCHECKED));
}
void AeSysApp::OnEditCfGroups()
{
	m_bEditCFGroups = !m_bEditCFGroups;
	app.CheckMenuItem(ID_EDIT_CF_GROUPS, MF_BYCOMMAND | (m_bEditCFGroups ? MF_CHECKED : MF_UNCHECKED));
}
void AeSysApp::OnEditCfText()
{
	m_bEditCFText = !m_bEditCFText;
	app.CheckMenuItem(ID_EDIT_CF_TEXT, MF_BYCOMMAND | (m_bEditCFText ? MF_CHECKED : MF_UNCHECKED));
}
void AeSysApp::OnModeAnnotate()
{
	m_ModeResourceIdentifier = IDR_ANNOTATE_MODE;
	m_iPrimModeId = ID_MODE_ANNOTATE;
	LoadModeResources(ID_MODE_ANNOTATE);
}
void AeSysApp::OnModeCut()
{
	m_ModeResourceIdentifier = IDR_CUT_MODE;
	m_iPrimModeId = ID_MODE_CUT;
	LoadModeResources(ID_MODE_CUT);
}
void AeSysApp::OnModeDimension()
{
	m_ModeResourceIdentifier = IDR_DIMENSION_MODE;
	m_iPrimModeId = ID_MODE_DIMENSION;
	LoadModeResources(ID_MODE_DIMENSION);
}
void AeSysApp::OnModeDraw()
{
	m_ModeResourceIdentifier = IDR_DRAW_MODE;
	m_iPrimModeId = ID_MODE_DRAW;
	LoadModeResources(ID_MODE_DRAW);
}
void AeSysApp::OnModeDraw2()
{
	m_ModeResourceIdentifier = IDR_DRAW2_MODE;
	m_iPrimModeId = ID_MODE_DRAW2;
	LoadModeResources(ID_MODE_DRAW2);
}
void AeSysApp::OnModeEdit()
{
	m_ModeResourceIdentifier = IDR_EDIT_MODE;
	LoadModeResources(ID_MODE_EDIT);
}
void AeSysApp::OnModeFixup()
{
	m_ModeResourceIdentifier = IDR_FIXUP_MODE;
	LoadModeResources(ID_MODE_FIXUP);
}
void AeSysApp::OnModeLetter()
{
	::DialogBox(app.GetInstance(), MAKEINTRESOURCE(IDD_ADD_NOTE), GetSafeHwnd(), DlgProcModeLetter);
}
void AeSysApp::OnModeLPD()
{
	m_ModeResourceIdentifier = IDR_LPD_MODE;
	LoadModeResources(ID_MODE_LPD);
}
void AeSysApp::OnModeNodal()
{
	m_ModeResourceIdentifier = IDR_NODAL_MODE;
	LoadModeResources(ID_MODE_NODAL);
}
void AeSysApp::OnModePipe()
{
	m_ModeResourceIdentifier = IDR_PIPE_MODE;
	LoadModeResources(ID_MODE_PIPE);
}
void AeSysApp::OnModePower()
{
	m_ModeResourceIdentifier = IDR_POWER_MODE;
	LoadModeResources(ID_MODE_POWER);
}
void AeSysApp::OnModeRevise()
{
	::DialogBox(app.GetInstance(), MAKEINTRESOURCE(IDD_ADD_NOTE), GetSafeHwnd(), DlgProcModeRevise);
}
void AeSysApp::OnModeTrap(void)
{
	if (m_TrapModeAddGroups)
	{
		m_ModeResourceIdentifier = IDR_TRAP_MODE;
		LoadModeResources(ID_MODE_TRAP);
	}
	else
	{
		m_ModeResourceIdentifier = IDR_TRAPR_MODE;
		LoadModeResources(ID_MODE_TRAPR);
	}
}
void AeSysApp::OnTrapCommandsAddGroups()
{
	m_TrapModeAddGroups = !m_TrapModeAddGroups;
	app.CheckMenuItem(ID_TRAPCOMMANDS_ADDGROUPS, (m_TrapModeAddGroups ? MF_CHECKED : MF_UNCHECKED));
	m_Mode = m_TrapModeAddGroups ? ID_MODE_TRAP : ID_MODE_TRAPR;
	
	OnModeTrap();
}
void AeSysApp::OnFileRun()
{
	TCHAR szFilter[256];
	::LoadString(app.GetInstance(), IDS_OPENFILE_FILTER_APPS, szFilter, sizeof(szFilter) / sizeof(TCHAR));
	
	CFileDialog dlg(TRUE, _T("exe"), _T("*.exe"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilter);
	dlg.m_ofn.lpstrTitle = _T("Run Application");

	if (dlg.DoModal() == IDOK)
	{
		CString strFile = dlg.GetFileName();
		//Note: use of winexec should be replaced with createprocess
		//WinExec(strFile, SW_SHOW);
	}
}
void AeSysApp::OnHelpContents()
{
	::WinHelp(app.GetSafeHwnd(), _T("peg.hlp"), HELP_CONTENTS, 0L);
}
void AeSysApp::PenWidthsLoad(const CString& strFileName)
{	
	CStdioFile fl;
	
	if (fl.Open(strFileName, CFile::modeRead | CFile::typeText)) 
	{
		TCHAR PenWidths[64];
		
		while (fl.ReadString(PenWidths, sizeof(PenWidths) / sizeof(TCHAR) - 1) != 0)
		{
			LPTSTR NextToken = NULL;
			
			int iId = _tstoi(_tcstok_s(PenWidths, _T("="), &NextToken));
			double dVal = _tstof(_tcstok_s(NULL, _T(",\n"), &NextToken));
			
			if (iId >= 0 && iId < sizeof(dPWids) / sizeof(dPWids[0]))
				dPWids[iId] = dVal;
		}
	}
}
EoDb::FileTypes AeSysApp::GetFileTypeFromPath(const CString& pathName)
{
	EoDb::FileTypes Type(EoDb::kUnknown);
	CString Extension = pathName.Right(3);

	if (!Extension.IsEmpty())
	{
		if (Extension.CompareNoCase(_T("peg")) == 0)
		{
			Type = EoDb::kPeg;
		}
		else if (Extension.CompareNoCase(_T("tra")) == 0)
		{
			Type = EoDb::kTracing;
		}
		else if (Extension.CompareNoCase(_T("jb1")) == 0)
		{
			Type = EoDb::kJob;
		}
		else if (Extension.CompareNoCase(_T("dwg")) == 0)
		{
			Type = EoDb::kDwg;
		}
		else if (Extension.CompareNoCase(_T("dxf")) == 0)
		{
			Type = EoDb::kDxf;
		}
		else if (Extension.CompareNoCase(_T("dxb")) == 0)
		{
			Type = EoDb::kDxb;
		}
	}
	return Type;
}
void AeSysApp::SetShadowDir(const CString& strDir) 
{
	TCHAR szPath[MAX_PATH];

	if (SUCCEEDED(SHGetSpecialFolderPath(m_pMainWnd->GetSafeHwnd(), szPath, CSIDL_PERSONAL, TRUE))) 
	{
		m_strShadowDir = szPath;
	}
	else
	{
		m_strShadowDir.Empty();
	}
	m_strShadowDir += _T("\\") + strDir + _T("\\");
		
	_tmkdir(m_strShadowDir);
}
EoGePoint3d AeSysApp::GetCursorPosition()
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	return (ActiveView == NULL) ? EoGePoint3d::kOrigin : ActiveView->GetCursorPosition();
}
/// <summary> Positions cursor at targeted position.</summary>
void AeSysApp::SetCursorPosition(EoGePoint3d pt)
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	ActiveView->SetCursorPosition(pt);
}
// Loads the hatch table.
void AeSysApp::HatchesLoad(const CString& strFileName)
{	
	CFileException e;
	CStdioFile fl;
	
	if (!fl.Open(strFileName, CFile::modeRead | CFile::typeText, &e))
		return;

	TCHAR	szLn[128];
	double	dTotStrsLen;
	int 	iNmbEnts, iNmbStrsId;
	
	TCHAR szValDel[] = _T(",\0");
	int iHatId = 0;
	int iNmbHatLns = 0;
	int iTblId = 0;
	
	while (fl.ReadString(szLn, sizeof(szLn) / sizeof(TCHAR) - 1) != 0) 
	{
		if (*szLn == '!')								// New Hatch index
		{
			if (iHatId != 0)
				hatch::fTableValue[hatch::iTableOffset[iHatId]] = float(iNmbHatLns);
			hatch::iTableOffset[++iHatId] = iTblId++;
			iNmbHatLns = 0;
		}
		else 
		{
			iNmbStrsId = iTblId;
			iTblId += 2;
			iNmbEnts = 0;
			dTotStrsLen = 0.;
			LPTSTR NextToken = NULL;
			LPTSTR pTok = _tcstok_s(szLn, szValDel, &NextToken);
			while (pTok != 0) 
			{
				hatch::fTableValue[iTblId] = float(_tstof(pTok));
				iNmbEnts++;
				if (iNmbEnts >= 6)
					dTotStrsLen = dTotStrsLen + hatch::fTableValue[iTblId];
				iTblId++;
				pTok = _tcstok_s(0, szValDel, &NextToken);
			}
			hatch::fTableValue[iNmbStrsId++] = float(iNmbEnts - 5);
			hatch::fTableValue[iNmbStrsId] = float(dTotStrsLen);
			iNmbHatLns++;
		}
	}
}
EoGePoint3d AeSysApp::HomePointGet(int i)
{
	if (i >= 0 && i < 9)
		return (m_ptHomePoint[i]);
	
	return (EoGePoint3d::kOrigin);
}
void AeSysApp::HomePointSave(int i, const EoGePoint3d& pt)
{
	if (i >= 0 && i < 9)
		m_ptHomePoint[i] = pt;
}
//Initializes all peg global sections to their default (startup) values.
void AeSysApp::InitGbls(CDC* deviceContext)
{
	pstate.SetPolygonIntStyle(EoDb::Hollow);
	
	pstate.SetPolygonIntStyleId(1);

	hatch::dXAxRefVecScal = .1;
	hatch::dYAxRefVecScal = .1;
	hatch::dOffAng = 0.;

	dlgproceditops::SetMirrorScale(- 1, 1., 1.);
	
	dlgproceditops::SetRotOrd(0, 1, 2);
	dlgproceditops::SetRotAng(0., 0., 45.);
	dlgproceditops::SetScale(2., 2., 2.);

	CCharCellDef ccd;
	pstate.SetCharCellDef(ccd);

	CFontDef fd;
	pstate.SetFontDef(deviceContext, fd);
	
	app.SetScale(96.);
	app.SetUnits(EoDb::kEngineering);
	app.SetUnitsPrec(8);
	app.SetDimLen(.125);
	app.SetDimAngZ(45.);

	InitializeConstraints();

	m_bTrapHighlight = true;
	m_nTrapHighlightPenColor = 15;

	//Document->InitializeGroupAndPrimitiveEdit();
	pstate.SetPen(deviceContext, 1, 1);
	pstate.SetPointStyle(1);
}
//Note: Move PenStylesLoad to AeSysDoc
/// <summary>Loads the PenStyle table.</summary>
void AeSysApp::PenStylesLoad(const CString& strFileName)
{	
	AeSysDoc* Document = AeSysDoc::GetDoc();
	if (Document == NULL)
		return;

	CStdioFile fl;
	
	// TODO check for existance of table  - crashes on 99 otherwise ands no pen dlg box.
		
	if (fl.Open(strFileName, CFile::modeRead | CFile::typeText)) 
	{
		CString strDescription;
		CString strName;
		TCHAR pBuf[128];
		
		EoUInt16 wLensMax = 8;
		double* pLen = new double[wLensMax];
		
		LPTSTR NextToken;
		while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(TCHAR) - 1) != 0)
		{
			NextToken = 0;
			int iId = _tstoi(_tcstok_s(pBuf, _T("="), &NextToken));
			strName = _tcstok_s(0, _T(","), &NextToken);
			strDescription = _tcstok_s(0, _T("\n"), &NextToken);
			fl.ReadString(pBuf, sizeof(pBuf) / sizeof(TCHAR) - 1);
			
			NextToken = 0;
			EoUInt16 wLens = EoUInt16(_tstoi(_tcstok_s(pBuf, _T(",\n"), &NextToken)));
			
			if (wLens > wLensMax)
			{
				delete [] pLen;
				pLen = new double[wLens];
				wLensMax = wLens;
			}
			for (EoUInt16 w = 0; w < wLens; w++)
				pLen[w] = _tstof(_tcstok_s(0, _T(",\n"), &NextToken));
			
			Document->PenStylesInsert(iId, new CPenStyle(strName, strDescription, wLens, pLen));
		}
		delete [] pLen;
	}
}
void AeSysApp::PenColorsChoose()
{
	CHOOSECOLOR 	cc;

	::ZeroMemory(&cc, sizeof(CHOOSECOLOR));
	cc.lStructSize = sizeof(CHOOSECOLOR);
	
	cc.rgbResult = crHotCols[pstate.PenColor()];
	cc.lpCustColors = crHotCols;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_SOLIDCOLOR;
	::ChooseColor(&cc);

	cc.rgbResult = crWarmCols[pstate.PenColor()];
	cc.lpCustColors = crWarmCols;
	::ChooseColor(&cc);
	SetBackGround(crHotCols[0]);
	
	AeSysDoc::GetDoc()->UpdateAllViews(NULL, 0L, NULL);
}
// Loads the color table.
void AeSysApp::PenColorsLoad(const CString& strFileName)
{	
	CStdioFile fl;

	if (fl.Open(strFileName, CFile::modeRead | CFile::typeText)) 
	{
		TCHAR pBuf[128];
		LPTSTR	pId, pRed, pGreen, pBlue;
		
		while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(TCHAR) - 1) != 0 && _tcsnicmp(pBuf, _T("<Colors>"), 8) != 0);
		
		while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(TCHAR) - 1) != 0 && *pBuf != '<')
		{
			LPTSTR NextToken = NULL;
			pId = _tcstok_s(pBuf, _T("="), &NextToken);
			pRed = _tcstok_s(0, _T(","), &NextToken);
			pGreen = _tcstok_s(0, _T(","), &NextToken);
			pBlue = _tcstok_s(0, _T(","), &NextToken);
			crHotCols[_tstoi(pId)] = RGB(_tstoi(pRed), _tstoi(pGreen), _tstoi(pBlue));
			pRed = _tcstok_s(0, _T(","), &NextToken);
			pGreen = _tcstok_s(0, _T(","), &NextToken);
			pBlue = _tcstok_s(0, _T("\n"), &NextToken);
			crWarmCols[_tstoi(pId)] = RGB(_tstoi(pRed), _tstoi(pGreen), _tstoi(pBlue));
		}
	}
}
void AeSysApp::SetBackGround(COLORREF cr)
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	
	if (ActiveView != NULL)
	{
		::SetClassLongPtr(ActiveView->GetSafeHwnd(), GCLP_HBRBACKGROUND, (LONG_PTR) ::CreateSolidBrush(cr));

		CDC* DC = ActiveView->GetDC();
		DC->SetBkColor(cr);
	}
}
void AeSysApp::LoadModeResources(int mode)
{
	BuildModifiedAcceleratorTable();
	
	m_Mode = mode;
	msgInformation(0);

	StatusLineDisplay();
	
	AeSysView* ActiveView = AeSysView::GetActiveView();
	if (ActiveView != 0)
	{
		ActiveView->SetModeCursor(m_Mode);
		ActiveView->ModeLineDisplay();
		ActiveView->RubberBandingDisable();
	}
}
void AeSysApp::StatusLineDisplay(EStatusLineItem sli)
{
	if (m_bViewStateInfo)	
	{	
		AeSysDoc* Document = AeSysDoc::GetDoc();
		AeSysView* ActiveView = AeSysView::GetActiveView();
		
		if (ActiveView == NULL)
		{
			return;
		}

		CDC* DeviceContext = ActiveView->GetDC();
		
		CRect rc;
		TCHAR szBuf[128];
		
		CFont* Font = (CFont*) DeviceContext->SelectStockObject(ANSI_VAR_FONT);
		UINT nTextAlign = DeviceContext->SetTextAlign(TA_LEFT | TA_TOP);
		COLORREF crText = DeviceContext->SetTextColor(AppGetTextCol()); 
		COLORREF crBk = DeviceContext->SetBkColor(~AppGetTextCol());

		TEXTMETRIC tm;		 
		DeviceContext->GetTextMetrics(&tm);
	
		CRect rcClient;
		ActiveView->GetClientRect(&rcClient);
		
		if (sli == All || sli == WorkCnt)
		{	// print num in work 
			rc.SetRect(0, rcClient.top, 8 * tm.tmAveCharWidth, rcClient.top + tm.tmHeight);
			_stprintf_s(szBuf, 128, _T("%i  "), Document->GetHotLayersCount() + Document->GetWarmLayersCount());
			DeviceContext->ExtTextOut(rc.left, rc.top, ETO_CLIPPED  | ETO_OPAQUE, &rc, szBuf, (UINT) _tcslen(szBuf), 0);
		}
		if (sli == All || sli == TrapCnt)
		{	// print num in trap 
			rc.SetRect(10 * tm.tmAveCharWidth, rcClient.top, 19 * tm.tmAveCharWidth, rcClient.top + tm.tmHeight);
			_stprintf_s(szBuf, 128, _T(" %i  "), Document->TrapGroupCount());
			DeviceContext->ExtTextOut(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE,  &rc, szBuf, (UINT) _tcslen(szBuf), 0);
		}
		if (sli == All || sli == Pen)
		{	// print pen  info
			rc.SetRect(21 * tm.tmAveCharWidth, rcClient.top, 27 * tm.tmAveCharWidth, rcClient.top + tm.tmHeight);
			_stprintf_s(szBuf, 128, _T("P %i "), pstate.PenColor());
			DeviceContext->ExtTextOut(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT) _tcslen(szBuf), 0);
		}
		if (sli == All || sli == Line)
		{	// print line info
			rc.SetRect(29 * tm.tmAveCharWidth, rcClient.top, 35 * tm.tmAveCharWidth, rcClient.top + tm.tmHeight);
			_stprintf_s(szBuf, 128, _T("L %i"), pstate.PenStyle());
			DeviceContext->ExtTextOut(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT) _tcslen(szBuf), 0);
		}
		if (sli == All || sli == TextHeight)
		{	// print text height
			CCharCellDef ccd;
			pstate.GetCharCellDef(ccd);
			rc.SetRect(37 * tm.tmAveCharWidth, rcClient.top, 47 * tm.tmAveCharWidth, rcClient.top + tm.tmHeight);
			_stprintf_s(szBuf, 128, _T("T %6.2f"), ccd.ChrHgtGet());
			DeviceContext->ExtTextOut(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT) _tcslen(szBuf), 0);
		}
		if (sli == All || sli == Scale)
		{	//print scale
			rc.SetRect(49 * tm.tmAveCharWidth, rcClient.top, 59 * tm.tmAveCharWidth, rcClient.top + tm.tmHeight);
			_stprintf_s(szBuf, 128, _T("1: %6.2f"), GetScale());
			DeviceContext->ExtTextOut(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT) _tcslen(szBuf), 0);
		}
		if (sli == All || sli == WndRatio)
		{	//print zoom
			rc.SetRect(61 * tm.tmAveCharWidth, rcClient.top, 71 * tm.tmAveCharWidth, rcClient.top + tm.tmHeight); 
			double dRatio = ActiveView->GetWidthInInches() / ActiveView->ModelViewGetUExt();
			_stprintf_s(szBuf, 128, _T("@ %6.3f"), dRatio);
			DeviceContext->ExtTextOut(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT) _tcslen(szBuf), 0);
		}
		if (sli == All || sli == DimLen)
		{	// print DimLen
			rc.SetRect(73 * tm.tmAveCharWidth, rcClient.top, 90 * tm.tmAveCharWidth, rcClient.top + tm.tmHeight);			  
			UnitsString_FormatLength(szBuf, 128, GetUnits(), GetDimLen(), 16, 4);
			DeviceContext->ExtTextOut(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT) _tcslen(szBuf), 0);
		}
		if (sli == All || sli == DimAng)
		{	// print DimAngle
			rc.SetRect(92 * tm.tmAveCharWidth, rcClient.top, 107 * tm.tmAveCharWidth, rcClient.top + tm.tmHeight); 
			_stprintf_s(szBuf, 128, _T(">> %8.4f"), GetDimAngZ());
			DeviceContext->ExtTextOut(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT) _tcslen(szBuf), 0);
		}
		// restore Device Context to its original state
		DeviceContext->SetBkColor(crBk);
		DeviceContext->SetTextColor(crText);
		DeviceContext->SetTextAlign(nTextAlign);
		DeviceContext->SelectObject(Font);
	}
}
// size (16384) of object for 96 character
// font set with a maximum of 4096 stokes
void AeSysApp::StrokeFontLoad(const CString& strPathName)
{	
	if (strPathName.GetLength() < 1)
	{
        int    fontlen;
        HRSRC  hrsrc;
		LPVOID fontptr;

        // Open binary resources
        hrsrc   = FindResource(NULL, MAKEINTRESOURCE(IDR_PEGSTROKEFONT), _T("STROKEFONT"));
        fontlen = SizeofResource(NULL, hrsrc);
        m_pStrokeFontDef = new char[fontlen];

        fontptr = LockResource(LoadResource(NULL, hrsrc));
		memcpy(m_pStrokeFontDef, fontptr, fontlen);

    } 
    else 
    {
		HANDLE hFile = CreateFile(
			strPathName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); 
		if (hFile != ((HANDLE) - 1)) 
		{ 
			if (SetFilePointer (hFile, 0, 0, FILE_BEGIN) != (DWORD) - 1)
			{
				if (m_pStrokeFontDef == 0)
					m_pStrokeFontDef = new char[16384];
			
				DWORD nBytesRead;
				ReadFile(hFile, m_pStrokeFontDef, 16384U, &nBytesRead, 0); 
			} 
			CloseHandle(hFile);
		}
	}
}
void AeSysApp::StrokeFontRelease()
{
	// Releases a large character array loaded for stroke fonts.
	
	if (m_pStrokeFontDef != 0)
		delete [] m_pStrokeFontDef;
}
COLORREF AppGetTextCol()
{
	return (~(crHotCols[0] | 0xff000000));
}
void AeSysApp::OnUpdateModeDraw(CCmdUI *pCmdUI)
{
	ATLTRACE(atlTraceGeneral, 0, _T("AeSysApp::OnUpdateModeDraw\n"));
	pCmdUI->SetCheck(m_Mode == ID_MODE_DRAW);
}

void AeSysApp::OnUpdateModeAnnotate(CCmdUI *pCmdUI)
{
	ATLTRACE(atlTraceGeneral, 0, _T("AeSysApp::OnUpdateModeAnnotate\n"));
	pCmdUI->SetCheck(m_Mode == ID_MODE_ANNOTATE);
}

void AeSysApp::OnUpdateModeTrap(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_Mode == ID_MODE_TRAP);
}

void AeSysApp::OnUpdateModeEdit(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_Mode == ID_MODE_EDIT);
}

void AeSysApp::OnUpdateModeDraw2(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_Mode == ID_MODE_DRAW2);
}

void AeSysApp::OnUpdateModeDimension(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_Mode == ID_MODE_DIMENSION);
}

void AeSysApp::OnUpdateModeCut(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_Mode == ID_MODE_CUT);
}

void AeSysApp::OnUpdateModeNodal(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_Mode == ID_MODE_NODAL);
}

void AeSysApp::OnUpdateModeFixup(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_Mode == ID_MODE_FIXUP);
}

void AeSysApp::OnUpdateModeLpd(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_Mode == ID_MODE_LPD);
}

void AeSysApp::OnUpdateModePower(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_Mode == ID_MODE_POWER);
}

void AeSysApp::OnUpdateModePipe(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_Mode == ID_MODE_PIPE);
}

void AeSysApp::OnViewModeInformation()
{
	m_bViewModeInfo = !m_bViewModeInfo;
	AeSysDoc::GetDoc()->UpdateAllViews(NULL, 0L, NULL);
}
void AeSysApp::OnUpdateViewModeinformation(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bViewModeInfo);
}
void AeSysApp::OnViewStateInformation()
{
	m_bViewStateInfo = !m_bViewStateInfo;
	AeSysDoc::GetDoc()->UpdateAllViews(NULL, 0L, NULL);
}
void AeSysApp::OnUpdateViewStateinformation(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bViewStateInfo);
}

#if defined(DWGDIRECT)
const ODCOLORREF* AeSysApp::curPalette() const
{
  const ODCOLORREF *refcolor = odcmAcadPalette(m_background);
  return refcolor;
}

#ifdef _MT
#ifndef _DLL

// For MT configurations only
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
extern "C" {

  ALLOCDLL_EXPORT void* odrxAlloc(size_t s)
  {
    return ::malloc(s);
  }

  ALLOCDLL_EXPORT void* odrxRealloc(void* p, size_t new_size, size_t /*old_size*/)
  {
    return ::realloc(p, new_size);
  }

  ALLOCDLL_EXPORT void odrxFree(void* p) 
  {
    ::free(p);
  }

} // extern "C"
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#endif
#endif
#endif // DWGDIRECT

// Modifies the base accelerator table by defining the mode specific keys.
void AeSysApp::BuildModifiedAcceleratorTable(void)
{
	CMainFrame* MainFrame = (CMainFrame*) AfxGetMainWnd();

	HACCEL AcceleratorTableHandle = MainFrame->m_hAccelTable;
	::DestroyAcceleratorTable(AcceleratorTableHandle);

	HACCEL ModeAcceleratorTableHandle = ::LoadAccelerators(m_hInstance, MAKEINTRESOURCE(m_ModeResourceIdentifier));
	int ModeAcceleratorTableEntries = ::CopyAcceleratorTable(ModeAcceleratorTableHandle, NULL, 0);
	
	AcceleratorTableHandle = ::LoadAccelerators(m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
	int AcceleratorTableEntries = ::CopyAcceleratorTable(AcceleratorTableHandle, NULL, 0);
	
	LPACCEL ModifiedAcceleratorTable = new ACCEL[AcceleratorTableEntries + ModeAcceleratorTableEntries];
	
	::CopyAcceleratorTable(ModeAcceleratorTableHandle, ModifiedAcceleratorTable, ModeAcceleratorTableEntries);
	::CopyAcceleratorTable(AcceleratorTableHandle, &ModifiedAcceleratorTable[ModeAcceleratorTableEntries], AcceleratorTableEntries);
	
	MainFrame->m_hAccelTable = ::CreateAcceleratorTable(ModifiedAcceleratorTable, AcceleratorTableEntries + ModeAcceleratorTableEntries);
	
	delete ModifiedAcceleratorTable;
}
void AeSysApp::OnFileOpen(void)
{
	CString Filter;
	VERIFY(Filter.LoadString(IDS_OPENFILE_FILTER));

	DWORD Flags(OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);

	CFileDialog FileDialog(TRUE, NULL, NULL, Flags, Filter);
	
	CString FileName;
	FileDialog.m_ofn.lpstrFile = FileName.GetBuffer(_MAX_PATH);

	CString Title;
	VERIFY(Title.LoadString(AFX_IDS_OPENFILE));
	FileDialog.m_ofn.lpstrTitle = Title;

	int Result = FileDialog.DoModal();
	FileName.ReleaseBuffer();

	if (Result == IDOK)
	{
		OpenDocumentFile(FileName);
	}
}
