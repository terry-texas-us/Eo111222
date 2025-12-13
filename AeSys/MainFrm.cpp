#include "stdafx.h"
#include "MainFrm.h"
#include "AeSys.h"

#include "EoCtrlFindComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
ON_WM_CREATE()
ON_WM_DESTROY()
ON_WM_TIMER()
// Global help commands
ON_COMMAND(ID_HELP_FINDER, &CMDIFrameWndEx::OnHelpFinder)
ON_COMMAND(ID_HELP, &CMDIFrameWndEx::OnHelp)
ON_COMMAND(ID_CONTEXT_HELP, &CMDIFrameWndEx::OnContextHelp)
ON_COMMAND(ID_DEFAULT_HELP, &CMDIFrameWndEx::OnHelpFinder)
ON_COMMAND(ID_WINDOW_MANAGER, &CMainFrame::OnWindowManager)
ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullScreen)
ON_COMMAND(ID_MDI_TABBED, OnMdiTabbed)
ON_UPDATE_COMMAND_UI(ID_MDI_TABBED, OnUpdateMdiTabbed)
ON_REGISTERED_MESSAGE(AFX_WM_TOOLBARMENU, OnToolbarContextMenu)
ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, OnGetTabToolTip)
ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, OnToolbarReset)
ON_COMMAND_RANGE(ID_VIEW_APPLOOK_OFF_2007_BLUE, ID_VIEW_APPLOOK_WINDOWS_7, OnApplicationLook)
ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_OFF_2007_BLUE, ID_VIEW_APPLOOK_WINDOWS_7, OnUpdateApplicationLook)
ON_WM_MDIACTIVATE()
END_MESSAGE_MAP()

const int iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

static UINT indicators[] = {
    ID_INDICATOR_ICON,      // status icon
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_PROGRESS,  // progress bar
    ID_OP0,
    ID_OP1,
    ID_OP2,
    ID_OP3,
    ID_OP4,
    ID_OP5,
    ID_OP6,
    ID_OP7,
    ID_OP8,
    ID_OP9,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame() : m_CurrentProgress(0), m_InProgress(false) {
  m_ApplicationLook = static_cast<UINT>(app.GetInt(L"ApplicationLook", ID_VIEW_APPLOOK_OFF_2007_BLACK));
}
CMainFrame::~CMainFrame() {}
int CMainFrame::OnCreate(LPCREATESTRUCT createStruct) {
  if (CMDIFrameWndEx::OnCreate(createStruct) == -1) return -1;

  OnApplicationLook(m_ApplicationLook);
  UpdateMDITabs(FALSE);

  if (!m_MenuBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE)) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create menubar\n");
    return -1;
  }
  m_MenuBar.SetPaneStyle(m_MenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

  // Prevent the menu bar from taking the focus on activation
  CMFCPopupMenu::SetForceMenuFocus(FALSE);
  DWORD Style(WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
  if (!m_StandardToolBar.CreateEx(this, TBSTYLE_FLAT, Style) ||
      !m_StandardToolBar.LoadToolBar(static_cast<UINT>(app.HighColorMode() ? IDR_MAINFRAME_256 : IDR_MAINFRAME))) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create toolbar\n");
    return -1;
  }
  m_StandardToolBar.SetWindowTextW(L"Standard");
  m_StandardToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, L"Customize...");

  InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);

  if (!m_StatusBar.Create(this)) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create status bar\n");
    return -1;
  }
  m_StatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));

  m_StatusBar.SetPaneStyle(nStatusIcon, SBPS_NOBORDERS);
  m_StatusBar.SetPaneStyle(nStatusInfo, SBPS_STRETCH | SBPS_NOBORDERS);
  m_StatusBar.SetPaneWidth(nStatusProgress, 80);

  if (!CreateDockablePanes()) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create dockable panes\n");
    return -1;
  }
  m_MenuBar.EnableDocking(CBRS_ALIGN_ANY);
  m_StandardToolBar.EnableDocking(CBRS_ALIGN_ANY);
  m_PropertiesPane.EnableDocking(CBRS_ALIGN_ANY);
  m_OutputPane.EnableDocking(CBRS_ALIGN_ANY);

  EnableDocking(CBRS_ALIGN_ANY);

  DockPane(&m_MenuBar);
  DockPane(&m_StandardToolBar);
  DockPane(&m_PropertiesPane);
  DockPane(&m_OutputPane);

  EnableAutoHidePanes(CBRS_ALIGN_ANY);

  EnableWindowsDialog(ID_WINDOW_MANAGER, IDS_WINDOWS_MANAGER, TRUE);

  // Enable automatic creation and management of the pop-up pane menu, which displays a list of application panes.
  EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, L"Customize...", ID_VIEW_TOOLBARS, FALSE, FALSE);
  EnableFullScreenMode(ID_VIEW_FULLSCREEN);
  EnableFullScreenMainMenu(TRUE);

  CMFCToolBar::EnableQuickCustomization();

  if (CMFCToolBar::GetUserImages() == NULL) {
    // load user-defined toolbar images
    if (m_UserImages.Load(L"\\UserImages.bmp")) {
      m_UserImages.SetImageSize(CSize(16, 16), FALSE);
      CMFCToolBar::SetUserImages(&m_UserImages);
    }
  }
  // Shows the document name after thumbnail before the application name in a frame window title.
  ModifyStyle(0, FWS_PREFIXTITLE);

  return 0;
}
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) {
  if (!CMDIFrameWndEx::PreCreateWindow(cs)) return FALSE;

  return TRUE;
}
BOOL CMainFrame::CreateDockablePanes() {
  CSize DefaultSize(200, 200);

  const DWORD SharedStyles(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI);

  CString Caption = EoAppLoadStringResource(IDS_OUTPUT);
  if (!m_OutputPane.Create(Caption, this, DefaultSize, TRUE, ID_VIEW_OUTPUTWND, SharedStyles | CBRS_BOTTOM)) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create Output pane\n");
    return FALSE;
  }
  Caption = EoAppLoadStringResource(IDS_PROPERTIES);
  if (!m_PropertiesPane.Create(Caption, this, DefaultSize, TRUE, ID_VIEW_PROPERTIESWND, SharedStyles | CBRS_RIGHT)) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create Properties pane\n");
    return FALSE;
  }
  SetDockablePanesIcons(app.HighColorMode());
  return TRUE;
}

void CMainFrame::SetDockablePanesIcons(bool highColorMode) {
  CSize SmallIconSize(::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
  HINSTANCE ResourceHandle(::AfxGetResourceHandle());

  HICON PropertiesPaneIcon =
      (HICON)::LoadImage(ResourceHandle, MAKEINTRESOURCE(highColorMode ? IDI_PROPERTIES_WND_HC : IDI_PROPERTIES_WND),
                         IMAGE_ICON, SmallIconSize.cx, SmallIconSize.cy, 0);
  m_PropertiesPane.SetIcon(PropertiesPaneIcon, FALSE);

  HICON OutputPaneIcon =
      (HICON)::LoadImage(ResourceHandle, MAKEINTRESOURCE(highColorMode ? IDI_OUTPUT_WND_HC : IDI_OUTPUT_WND),
                         IMAGE_ICON, SmallIconSize.cx, SmallIconSize.cy, 0);
  m_OutputPane.SetIcon(OutputPaneIcon, FALSE);

  UpdateMDITabbedBarsIcons();
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const { CMDIFrameWndEx::AssertValid(); }

void CMainFrame::Dump(CDumpContext& dc) const { CMDIFrameWndEx::Dump(dc); }

#endif  //_DEBUG

void CMainFrame::OnWindowManager() { ShowWindowsDialog(); }
void CMainFrame::OnViewCustomize() {
  CMFCToolBarsCustomizeDialog* Dialog = new CMFCToolBarsCustomizeDialog(this, TRUE);
  Dialog->EnableUserDefinedToolbars();

  // Setup combobox:
  Dialog->ReplaceButton(ID_EDIT_FIND, EoCtrlFindComboBox());

  Dialog->Create();
}
LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp, LPARAM name) {
  LRESULT Result = CMDIFrameWndEx::OnToolbarCreateNew(wp, name);
  if (Result == 0) { return 0; }
  CMFCToolBar* UserToolbar = (CMFCToolBar*)Result;
  ASSERT_VALID(UserToolbar);

  CString Customize = EoAppLoadStringResource(IDS_TOOLBAR_CUSTOMIZE);

  UserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, Customize);
  return Result;
}
LRESULT CMainFrame::OnToolbarReset(WPARAM toolbarResourceId, LPARAM lparam) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"CMainFrame::OnToolbarReset(%i, %i)\n", toolbarResourceId, lparam);

  switch (toolbarResourceId) {
    case IDR_MAINFRAME:
    case IDR_MAINFRAME_256: {
      m_StandardToolBar.ReplaceButton(ID_EDIT_FIND, EoCtrlFindComboBox(), FALSE);
      break;
    }
    case IDR_PROPERTIES:
      break;
  }
  return 0;
}
void CMainFrame::OnApplicationLook(UINT look) {
  m_ApplicationLook = look;

  switch (m_ApplicationLook) {
    case ID_VIEW_APPLOOK_WINDOWS_7:
      CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
      break;

    default:
      switch (m_ApplicationLook) {
        case ID_VIEW_APPLOOK_OFF_2007_BLUE:
          CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
          break;

        case ID_VIEW_APPLOOK_OFF_2007_BLACK:
          CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
          break;

        case ID_VIEW_APPLOOK_OFF_2007_AQUA:
          CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
          break;

        case ID_VIEW_APPLOOK_OFF_2007_SILVER:
          CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
          break;
      }
      CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
  }
  CDockingManager* DockingManager = GetDockingManager();
  ASSERT_VALID(DockingManager);
  DockingManager->AdjustPaneFrames();
  DockingManager->SetDockingMode(DT_SMART);

  RecalcLayout();
  RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_UPDATENOW);

  app.WriteInt(L"ApplicationLook", static_cast<int>(m_ApplicationLook));
}
void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI) { pCmdUI->SetRadio(m_ApplicationLook == pCmdUI->m_nID); }
BOOL CMainFrame::LoadFrame(UINT resourceId, DWORD defaultStyle, CWnd* parentWindow, CCreateContext* createContext) {
  if (!CMDIFrameWndEx::LoadFrame(resourceId, defaultStyle, parentWindow, createContext)) { return FALSE; }

  // Add some tools for example....
  CUserToolsManager* UserToolsManager = app.GetUserToolsManager();
  if (UserToolsManager != NULL && UserToolsManager->GetUserTools().IsEmpty()) {
    CUserTool* Tool1 = UserToolsManager->CreateNewTool();
    Tool1->m_strLabel = L"&Notepad";
    Tool1->SetCommand(L"notepad.exe");

    CUserTool* Tool2 = UserToolsManager->CreateNewTool();
    Tool2->m_strLabel = L"Paint &Brush";
    Tool2->SetCommand(L"mspaint.exe");

    CUserTool* Tool3 = UserToolsManager->CreateNewTool();
    Tool3->m_strLabel = L"&Windows Explorer";
    Tool3->SetCommand(L"explorer.exe");

    CUserTool* Tool4 = UserToolsManager->CreateNewTool();
    Tool4->m_strLabel = L"Fanning, Fanning & Associates On-&Line";
    Tool4->SetCommand(L"http://www.fanningfanning.com");
  }

  // Enable customization button for all user toolbars
  CString Customize = EoAppLoadStringResource(IDS_TOOLBAR_CUSTOMIZE);

  for (int i = 0; i < iMaxUserToolbars; i++) {
    CMFCToolBar* UserToolbar = GetUserToolBarByIndex(i);
    if (UserToolbar != NULL) { UserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, Customize); }
  }
  return TRUE;
}
LRESULT CMainFrame::OnToolbarContextMenu(WPARAM, LPARAM point) {
  CMenu PopupToolbarMenu;
  VERIFY(PopupToolbarMenu.LoadMenu(IDR_POPUP_TOOLBAR));

  CMenu* SubMenu = PopupToolbarMenu.GetSubMenu(0);
  ASSERT(SubMenu != NULL);

  if (SubMenu) {
    CPoint Point(AFX_GET_X_LPARAM(point), AFX_GET_Y_LPARAM(point));

    CMFCPopupMenu* PopupMenu = new CMFCPopupMenu;
    PopupMenu->Create(this, Point.x, Point.y, SubMenu->Detach());
  }
  return 0;
}
BOOL CMainFrame::OnShowPopupMenu(CMFCPopupMenu* pMenuPopup) {
  CMDIFrameWndEx::OnShowPopupMenu(pMenuPopup);

  if (pMenuPopup != NULL && pMenuPopup->GetMenuBar()->CommandToIndex(ID_VIEW_TOOLBARS) >= 0) {
    if (CMFCToolBar::IsCustomizeMode()) {
      // Don't show toolbars list in the cuztomization mode!
      return FALSE;
    }
    pMenuPopup->RemoveAllItems();

    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_POPUP_TOOLBAR));

    CMenu* PopupSubMenu = menu.GetSubMenu(0);
    ASSERT(PopupSubMenu != NULL);

    if (PopupSubMenu) { pMenuPopup->GetMenuBar()->ImportFromMenu(*PopupSubMenu, TRUE); }
  }
  return TRUE;
}
void CMainFrame::UpdateMDITabs(BOOL resetMDIChild) {
  switch (app.m_Options.m_nTabsStyle) {
    case EoApOptions::None: {
      int MDITabsType;

      if (AreMDITabs(&MDITabsType)) {
        if (MDITabsType == 1) {
          EnableMDITabs(FALSE);
        } else if (MDITabsType == 2) {
          CMDITabInfo TabInfo;  // ignored when tabbed groups are disabled

          EnableMDITabbedGroups(FALSE, TabInfo);
        }
      } else {
        HWND ActiveWnd = (HWND)m_wndClientArea.SendMessage(WM_MDIGETACTIVE);
        m_wndClientArea.PostMessage(WM_MDICASCADE);
        ::BringWindowToTop(ActiveWnd);
      }
      break;
    }
    case EoApOptions::Standard: {
      HWND ActiveWnd = (HWND)m_wndClientArea.SendMessage(WM_MDIGETACTIVE);
      m_wndClientArea.PostMessageW(WM_MDIMAXIMIZE, WPARAM(ActiveWnd), 0L);
      ::BringWindowToTop(ActiveWnd);

      EnableMDITabs(TRUE, app.m_Options.m_MdiTabInfo.m_bTabIcons, app.m_Options.m_MdiTabInfo.m_tabLocation,
                    app.m_Options.m_MdiTabInfo.m_bTabCloseButton, app.m_Options.m_MdiTabInfo.m_style,
                    app.m_Options.m_MdiTabInfo.m_bTabCustomTooltips,
                    app.m_Options.m_MdiTabInfo.m_bActiveTabCloseButton);

      GetMDITabs().EnableAutoColor(app.m_Options.m_MdiTabInfo.m_bAutoColor);
      GetMDITabs().EnableTabDocumentsMenu(app.m_Options.m_MdiTabInfo.m_bDocumentMenu);
      GetMDITabs().EnableTabSwap(app.m_Options.m_MdiTabInfo.m_bEnableTabSwap);
      GetMDITabs().SetTabBorderSize(app.m_Options.m_MdiTabInfo.m_nTabBorderSize);
      GetMDITabs().SetFlatFrame(app.m_Options.m_MdiTabInfo.m_bFlatFrame);
      break;
    }
    case EoApOptions::Grouped: {
      HWND ActiveWnd = (HWND)m_wndClientArea.SendMessage(WM_MDIGETACTIVE);
      m_wndClientArea.PostMessage(WM_MDIMAXIMIZE, WPARAM(ActiveWnd), 0L);
      ::BringWindowToTop(ActiveWnd);

      EnableMDITabbedGroups(TRUE, app.m_Options.m_MdiTabInfo);
      break;
    }
  }
  CList<UINT, UINT> lstCommands;
  if (AreMDITabs(NULL)) {
    lstCommands.AddTail(ID_WINDOW_ARRANGE);
    lstCommands.AddTail(ID_WINDOW_CASCADE);
    lstCommands.AddTail(ID_WINDOW_TILE_HORZ);
    lstCommands.AddTail(ID_WINDOW_TILE_VERT);
  }
  CMFCToolBar::SetNonPermittedCommands(lstCommands);
  if (resetMDIChild) {
    BOOL bMaximize = app.m_Options.m_nTabsStyle != EoApOptions::None;

    HWND hwndT = ::GetWindow(m_hWndMDIClient, GW_CHILD);
    while (hwndT != NULL) {
      CMDIChildWndEx* pFrame = DYNAMIC_DOWNCAST(CMDIChildWndEx, CWnd::FromHandle(hwndT));
      if (pFrame != NULL) {
        ASSERT_VALID(pFrame);
        if (bMaximize) {
          pFrame->ModifyStyle(WS_SYSMENU, 0);
        } else {
          pFrame->ModifyStyle(0, WS_SYSMENU);
          pFrame->ShowWindow(SW_RESTORE);

          // Force a resize to happen on all the "restored" MDI child windows
          CRect rectFrame;
          pFrame->GetWindowRect(rectFrame);
          pFrame->SetWindowPos(NULL, -1, -1, rectFrame.Width() + 1, rectFrame.Height(),
                               SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
          pFrame->SetWindowPos(NULL, -1, -1, rectFrame.Width(), rectFrame.Height(),
                               SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
        }
      }
      hwndT = ::GetWindow(hwndT, GW_HWNDNEXT);
    }
    if (bMaximize) { m_MenuBar.SetMaximizeMode(FALSE); }
  }
  if (m_PropertiesPane.IsAutoHideMode()) {
    m_PropertiesPane.BringWindowToTop();
    CPaneDivider* Divider = m_PropertiesPane.GetDefaultPaneDivider();
    if (Divider != NULL) { Divider->BringWindowToTop(); }
  }
  CMDIFrameWndEx::m_bDisableSetRedraw = app.m_Options.m_bDisableSetRedraw;

  RecalcLayout();
  RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

// CMainFrame message handlers

BOOL CMainFrame::OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL bDrop) {
  if (bDrop || !app.m_Options.m_bTabsContextMenu) { return FALSE; }
  CMenu menu;
  VERIFY(menu.LoadMenu(IDR_POPUP_MDITABS));

  CMenu* PopupSubMenu = menu.GetSubMenu(0);
  ASSERT(PopupSubMenu != NULL);

  if (PopupSubMenu) {
    if ((dwAllowedItems & AFX_MDI_CAN_BE_DOCKED) == 0) { PopupSubMenu->DeleteMenu(ID_MDI_TABBED, MF_BYCOMMAND); }
    CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;
    if (pPopupMenu) {
      pPopupMenu->SetAutoDestroy(FALSE);
      pPopupMenu->Create(this, point.x, point.y, PopupSubMenu->GetSafeHmenu());
    }
  }
  return TRUE;
}
LRESULT CMainFrame::OnGetTabToolTip(WPARAM /*wp*/, LPARAM lp) {
  CMFCTabToolTipInfo* pInfo = (CMFCTabToolTipInfo*)lp;
  ASSERT(pInfo != NULL);

  if (pInfo) {
    ASSERT_VALID(pInfo->m_pTabWnd);
    if (!pInfo->m_pTabWnd->IsMDITab()) { return 0; }
    pInfo->m_strText.Format(L"Tab #%d Custom Tooltip", pInfo->m_nTabIndex + 1);
  }
  return 0;
}
void CMainFrame::OnMdiTabbed() {
  CMDIChildWndEx* pMDIChild = DYNAMIC_DOWNCAST(CMDIChildWndEx, MDIGetActive());
  if (pMDIChild == NULL) {
    ASSERT(FALSE);
    return;
  }
  TabbedDocumentToControlBar(pMDIChild);
}
void CMainFrame::OnUpdateMdiTabbed(CCmdUI* pCmdUI) { pCmdUI->SetCheck(); }
void CMainFrame::OnDestroy() {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"CMainFrame::OnDestroy() - Entering\n");

#if defined(USING_ODA)
  odUninitialize();
#endif  // USING_ODA

  PostQuitMessage(0);  // Force WM_QUIT message to terminate message loop
}
CString CMainFrame::GetPaneText(int index) { return m_StatusBar.GetPaneText(index); }
void CMainFrame::SetPaneInfo(int index, UINT newId, UINT style, int width) {
  m_StatusBar.SetPaneInfo(index, newId, style, width);
}
BOOL CMainFrame::SetPaneText(int index, LPCWSTR newText) { return m_StatusBar.SetPaneText(index, newText); }
void CMainFrame::SetPaneStyle(int index, UINT style) { m_StatusBar.SetPaneStyle(index, style); }
void CMainFrame::SetPaneTextColor(int index, COLORREF textColor) { m_StatusBar.SetPaneTextColor(index, textColor); }
static UINT_PTR TimerId = 2;

void CMainFrame::OnStartProgress(void) {
  if (m_InProgress) {
    KillTimer(TimerId);
    m_StatusBar.EnablePaneProgressBar(nStatusProgress, -1);

    m_InProgress = false;

    return;
  }
  m_StatusBar.EnablePaneProgressBar(nStatusProgress, 100);

  m_CurrentProgress = 0;
  m_InProgress = true;

  TimerId = SetTimer(2, 1, NULL);
}
void CMainFrame::OnTimer(UINT_PTR nIDEvent) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"CMainFrame::OnTimer(%i)\n", nIDEvent);

  if (nIDEvent == TimerId) {
    m_CurrentProgress += 10;

    if (m_CurrentProgress > 100) { m_CurrentProgress = 0; }
    m_StatusBar.SetPaneProgress(nStatusProgress, m_CurrentProgress);
  }
}
void CMainFrame::OnViewFullScreen(void) { ShowFullScreen(); }
CMFCToolBarComboBoxButton* CMainFrame::GetFindCombo(void) {
  CMFCToolBarComboBoxButton* FoundCombo = NULL;

  CObList ButtonsList;
  if (CMFCToolBar::GetCommandButtons(ID_EDIT_FIND_COMBO, ButtonsList) > 0) {
    for (POSITION Position = ButtonsList.GetHeadPosition(); FoundCombo == NULL && Position != NULL;) {
      CMFCToolBarComboBoxButton* Combo = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, ButtonsList.GetNext(Position));

      if (Combo != NULL && Combo->GetEditCtrl()->GetSafeHwnd() == ::GetFocus()) { FoundCombo = Combo; }
    }
  }
  return FoundCombo;
}
