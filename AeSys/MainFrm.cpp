
#include "StdAfx.h"
#include <Windows.h>
#include <afx.h>
#include <afxbasetabctrl.h>
#include <afxcoll.h>
#include <afxcontrolbarutil.h>
#include <afxdockingmanager.h>
#include <afxext.h>
#include <afxglobals.h>
#include <afxmdichildwndex.h>
#include <afxmdiclientareawnd.h>
#include <afxmdiframewndex.h>
#include <afxmsg_.h>
#include <afxpane.h>
#include <afxpanedivider.h>
#include <afxpopupmenu.h>
#include <afxres.h>
#include <afxstr.h>
#include <afxtempl.h>
#include <afxtoolbar.h>
#include <afxtoolbarcomboboxbutton.h>
#include <afxtoolbarscustomizedialog.h>
#include <afxusertool.h>
#include <afxusertoolsmanager.h>
#include <afxvisualmanager.h>
#include <afxvisualmanageroffice2007.h>
#include <afxvisualmanagerwindows7.h>
#include <afxwin.h>
#include <atltrace.h>
#include <atltypes.h>

#include "AeSys.h"
#include "EoApOptions.h"
#include "EoCtrlFindComboBox.h"
#include "MainFrm.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace {
constexpr int statusIcon = 0;
constexpr int statusInfo = 1;
constexpr int statusProgress = 2;
constexpr int maxUserToolbars = 10;
constexpr unsigned int firstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
constexpr unsigned int lastUserToolBarId = firstUserToolBarId + maxUserToolbars - 1;
constexpr unsigned int indicators[] = {
    ID_INDICATOR_ICON,
    ID_SEPARATOR,
    ID_INDICATOR_PROGRESS,
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
}  // namespace

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_CREATE()
#pragma warning(pop)
ON_WM_DESTROY()
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_MDIACTIVATE()
ON_WM_TIMER()
#pragma warning(pop)
ON_COMMAND(ID_CONTEXT_HELP, &CMDIFrameWndEx::OnContextHelp)
ON_COMMAND(ID_DEFAULT_HELP, &CMDIFrameWndEx::OnHelpFinder)
ON_COMMAND(ID_HELP_FINDER, &CMDIFrameWndEx::OnHelpFinder)
ON_COMMAND(ID_HELP, &CMDIFrameWndEx::OnHelp)
ON_COMMAND(ID_MDI_TABBED, OnMdiTabbed)
ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullScreen)
ON_COMMAND(ID_WINDOW_MANAGER, &CMainFrame::OnWindowManager)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_COMMAND_RANGE(ID_VIEW_APPLOOK_OFF_2007_BLUE, ID_VIEW_APPLOOK_WINDOWS_7, OnApplicationLook)
#pragma warning(pop)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_REGISTERED_MESSAGE(AFX_WM_TOOLBARMENU, OnToolbarContextMenu)
ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, OnGetTabToolTip)
ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, OnToolbarReset)
#pragma warning(pop)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_UPDATE_COMMAND_UI(ID_MDI_TABBED, OnUpdateMdiTabbed)
ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_OFF_2007_BLUE, ID_VIEW_APPLOOK_WINDOWS_7, OnUpdateApplicationLook)
#pragma warning(pop)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() : m_currentProgress(0), m_inProgress(false) {
  m_applicationLook = static_cast<UINT>(app.GetInt(L"ApplicationLook", ID_VIEW_APPLOOK_OFF_2007_BLACK));
}
CMainFrame::~CMainFrame() {}
int CMainFrame::OnCreate(LPCREATESTRUCT createStruct) {
  if (CMDIFrameWndEx::OnCreate(createStruct) == -1) return -1;

  OnApplicationLook(m_applicationLook);
  UpdateMDITabs(FALSE);

  if (!m_menuBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE)) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create menubar\n");
    return -1;
  }
  m_menuBar.SetPaneStyle(m_menuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

  // Prevent the menu bar from taking the focus on activation
  CMFCPopupMenu::SetForceMenuFocus(FALSE);
  DWORD Style(WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
  if (!m_standardToolBar.CreateEx(this, TBSTYLE_FLAT, Style) ||
      !m_standardToolBar.LoadToolBar(static_cast<UINT>(app.HighColorMode() ? IDR_MAINFRAME_256 : IDR_MAINFRAME))) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create toolbar\n");
    return -1;
  }
  m_standardToolBar.SetWindowTextW(L"Standard");
  m_standardToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, L"Customize...");

  InitUserToolbars(nullptr, firstUserToolBarId, lastUserToolBarId);

  if (!m_statusBar.Create(this)) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create status bar\n");
    return -1;
  }
  m_statusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(unsigned int));

  m_statusBar.SetPaneStyle(statusIcon, SBPS_NOBORDERS);
  m_statusBar.SetPaneStyle(statusInfo, SBPS_STRETCH | SBPS_NOBORDERS);
  m_statusBar.SetPaneWidth(statusProgress, 80);

  if (!CreateDockablePanes()) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create dockable panes\n");
    return -1;
  }
  m_menuBar.EnableDocking(CBRS_ALIGN_ANY);
  m_standardToolBar.EnableDocking(CBRS_ALIGN_ANY);
  m_propertiesPane.EnableDocking(CBRS_ALIGN_ANY);
  m_outputPane.EnableDocking(CBRS_ALIGN_ANY);

  EnableDocking(CBRS_ALIGN_ANY);

  DockPane(&m_menuBar);
  DockPane(&m_standardToolBar);
  DockPane(&m_propertiesPane);
  DockPane(&m_outputPane);

  EnableAutoHidePanes(CBRS_ALIGN_ANY);

  EnableWindowsDialog(ID_WINDOW_MANAGER, IDS_WINDOWS_MANAGER, TRUE);

  // Enable automatic creation and management of the pop-up pane menu, which displays a list of application panes.
  EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, L"Customize...", ID_VIEW_TOOLBARS, FALSE, FALSE);
  EnableFullScreenMode(ID_VIEW_FULLSCREEN);
  EnableFullScreenMainMenu(TRUE);

  CMFCToolBar::EnableQuickCustomization();

  if (CMFCToolBar::GetUserImages() == nullptr) {
    // load user-defined toolbar images
    if (m_userImages.Load(L"\\UserImages.bmp")) {
      m_userImages.SetImageSize(CSize(16, 16), FALSE);
      CMFCToolBar::SetUserImages(&m_userImages);
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
  if (!m_outputPane.Create(Caption, this, DefaultSize, TRUE, ID_VIEW_OUTPUTWND, SharedStyles | CBRS_BOTTOM)) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create Output pane\n");
    return FALSE;
  }
  Caption = EoAppLoadStringResource(IDS_PROPERTIES);
  if (!m_propertiesPane.Create(Caption, this, DefaultSize, TRUE, ID_VIEW_PROPERTIESWND, SharedStyles | CBRS_RIGHT)) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create Properties pane\n");
    return FALSE;
  }
  SetDockablePanesIcons(app.HighColorMode());
  return TRUE;
}

void CMainFrame::SetDockablePanesIcons(bool highColorMode) {
  CSize smallIconSize(::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
  HINSTANCE resourceHandle(::AfxGetResourceHandle());

  HICON propertiesPaneIcon =
      static_cast<HICON>(LoadImageW(resourceHandle, MAKEINTRESOURCE(highColorMode ? IDI_PROPERTIES_WND_HC : IDI_PROPERTIES_WND),
                          IMAGE_ICON, smallIconSize.cx, smallIconSize.cy, 0));
  m_propertiesPane.SetIcon(propertiesPaneIcon, FALSE);

  HICON outputPaneIcon =
      static_cast<HICON>(LoadImageW(resourceHandle, MAKEINTRESOURCE(highColorMode ? IDI_OUTPUT_WND_HC : IDI_OUTPUT_WND),
                          IMAGE_ICON, smallIconSize.cx, smallIconSize.cy, 0));
  m_outputPane.SetIcon(outputPaneIcon, FALSE);

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
      m_standardToolBar.ReplaceButton(ID_EDIT_FIND, EoCtrlFindComboBox(), FALSE);
      break;
    }
    case IDR_PROPERTIES:
      break;
  }
  return 0;
}
void CMainFrame::OnApplicationLook(UINT look) {
  m_applicationLook = look;

  switch (m_applicationLook) {
    case ID_VIEW_APPLOOK_WINDOWS_7:
      CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
      break;

    default:
      switch (m_applicationLook) {
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
  RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_UPDATENOW);

  app.WriteInt(L"ApplicationLook", static_cast<int>(m_applicationLook));
}
void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI) { pCmdUI->SetRadio(m_applicationLook == pCmdUI->m_nID); }
BOOL CMainFrame::LoadFrame(UINT resourceId, DWORD defaultStyle, CWnd* parentWindow, CCreateContext* createContext) {
  if (!CMDIFrameWndEx::LoadFrame(resourceId, defaultStyle, parentWindow, createContext)) { return FALSE; }

  // Add some tools for example....
  CUserToolsManager* UserToolsManager = app.GetUserToolsManager();
  if (UserToolsManager != nullptr && UserToolsManager->GetUserTools().IsEmpty()) {
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

  for (int i = 0; i < maxUserToolbars; i++) {
    CMFCToolBar* UserToolbar = GetUserToolBarByIndex(i);
    if (UserToolbar != nullptr) { UserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, Customize); }
  }
  return TRUE;
}
LRESULT CMainFrame::OnToolbarContextMenu(WPARAM, LPARAM point) {
  CMenu PopupToolbarMenu;
  VERIFY(PopupToolbarMenu.LoadMenu(IDR_POPUP_TOOLBAR));

  CMenu* SubMenu = PopupToolbarMenu.GetSubMenu(0);
  ASSERT(SubMenu != nullptr);

  if (SubMenu) {
    CPoint Point(AFX_GET_X_LPARAM(point), AFX_GET_Y_LPARAM(point));

    CMFCPopupMenu* PopupMenu = new CMFCPopupMenu;
    PopupMenu->Create(this, Point.x, Point.y, SubMenu->Detach());
  }
  return 0;
}
BOOL CMainFrame::OnShowPopupMenu(CMFCPopupMenu* pMenuPopup) {
  CMDIFrameWndEx::OnShowPopupMenu(pMenuPopup);

  if (pMenuPopup != nullptr && pMenuPopup->GetMenuBar()->CommandToIndex(ID_VIEW_TOOLBARS) >= 0) {
    if (CMFCToolBar::IsCustomizeMode()) {
      // Don't show toolbars list in the cuztomization mode!
      return FALSE;
    }
    pMenuPopup->RemoveAllItems();

    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_POPUP_TOOLBAR));

    CMenu* PopupSubMenu = menu.GetSubMenu(0);
    ASSERT(PopupSubMenu != nullptr);

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
  if (AreMDITabs(nullptr)) {
    lstCommands.AddTail(ID_WINDOW_ARRANGE);
    lstCommands.AddTail(ID_WINDOW_CASCADE);
    lstCommands.AddTail(ID_WINDOW_TILE_HORZ);
    lstCommands.AddTail(ID_WINDOW_TILE_VERT);
  }
  CMFCToolBar::SetNonPermittedCommands(lstCommands);
  if (resetMDIChild) {
    BOOL bMaximize = app.m_Options.m_nTabsStyle != EoApOptions::None;

    HWND hwndT = ::GetWindow(m_hWndMDIClient, GW_CHILD);
    while (hwndT != nullptr) {
      CMDIChildWndEx* pFrame = DYNAMIC_DOWNCAST(CMDIChildWndEx, CWnd::FromHandle(hwndT));
      if (pFrame != nullptr) {
        ASSERT_VALID(pFrame);
        if (bMaximize) {
          pFrame->ModifyStyle(WS_SYSMENU, 0);
        } else {
          pFrame->ModifyStyle(0, WS_SYSMENU);
          pFrame->ShowWindow(SW_RESTORE);

          // Force a resize to happen on all the "restored" MDI child windows
          CRect rectFrame;
          pFrame->GetWindowRect(rectFrame);
          pFrame->SetWindowPos(nullptr, -1, -1, rectFrame.Width() + 1, rectFrame.Height(),
                               SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
          pFrame->SetWindowPos(nullptr, -1, -1, rectFrame.Width(), rectFrame.Height(),
                               SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
        }
      }
      hwndT = ::GetWindow(hwndT, GW_HWNDNEXT);
    }
    if (bMaximize) { m_menuBar.SetMaximizeMode(FALSE); }
  }
  if (m_propertiesPane.IsAutoHideMode()) {
    m_propertiesPane.BringWindowToTop();
    CPaneDivider* Divider = m_propertiesPane.GetDefaultPaneDivider();
    if (Divider != nullptr) { Divider->BringWindowToTop(); }
  }
  CMDIFrameWndEx::m_bDisableSetRedraw = app.m_Options.m_bDisableSetRedraw;

  RecalcLayout();
  RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

// CMainFrame message handlers

BOOL CMainFrame::OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL bDrop) {
  if (bDrop || !app.m_Options.m_bTabsContextMenu) { return FALSE; }
  CMenu menu;
  VERIFY(menu.LoadMenu(IDR_POPUP_MDITABS));

  CMenu* PopupSubMenu = menu.GetSubMenu(0);
  ASSERT(PopupSubMenu != nullptr);

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
  ASSERT(pInfo != nullptr);

  if (pInfo) {
    ASSERT_VALID(pInfo->m_pTabWnd);
    if (!pInfo->m_pTabWnd->IsMDITab()) { return 0; }
    pInfo->m_strText.Format(L"Tab #%d Custom Tooltip", pInfo->m_nTabIndex + 1);
  }
  return 0;
}
void CMainFrame::OnMdiTabbed() {
  CMDIChildWndEx* pMDIChild = DYNAMIC_DOWNCAST(CMDIChildWndEx, MDIGetActive());
  if (pMDIChild == nullptr) {
    ASSERT(FALSE);
    return;
  }
  TabbedDocumentToControlBar(pMDIChild);
}
void CMainFrame::OnUpdateMdiTabbed(CCmdUI* pCmdUI) { pCmdUI->SetCheck(); }
void CMainFrame::OnDestroy() {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"CMainFrame::OnDestroy() - Entering\n");
  PostQuitMessage(0);  // Force WM_QUIT message to terminate message loop
}
CString CMainFrame::GetPaneText(int index) { return m_statusBar.GetPaneText(index); }
void CMainFrame::SetPaneInfo(int index, UINT newId, UINT style, int width) {
  m_statusBar.SetPaneInfo(index, newId, style, width);
}
BOOL CMainFrame::SetPaneText(int index, LPCWSTR newText) { return m_statusBar.SetPaneText(index, newText); }
void CMainFrame::SetPaneStyle(int index, UINT style) { m_statusBar.SetPaneStyle(index, style); }
void CMainFrame::SetPaneTextColor(int index, COLORREF textColor) { m_statusBar.SetPaneTextColor(index, textColor); }
static UINT_PTR TimerId = 2;

void CMainFrame::OnStartProgress() {
  if (m_inProgress) {
    KillTimer(TimerId);
    m_statusBar.EnablePaneProgressBar(statusProgress, -1);

    m_inProgress = false;

    return;
  }
  m_statusBar.EnablePaneProgressBar(statusProgress, 100);

  m_currentProgress = 0;
  m_inProgress = true;

  TimerId = SetTimer(2, 1, nullptr);
}
void CMainFrame::OnTimer(UINT_PTR nIDEvent) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"CMainFrame::OnTimer(%i)\n", nIDEvent);

  if (nIDEvent == TimerId) {
    m_currentProgress += 10;

    if (m_currentProgress > 100) { m_currentProgress = 0; }
    m_statusBar.SetPaneProgress(statusProgress, m_currentProgress);
  }
}
void CMainFrame::OnViewFullScreen() { ShowFullScreen(); }
CMFCToolBarComboBoxButton* CMainFrame::GetFindCombo() {
  CMFCToolBarComboBoxButton* FoundCombo = nullptr;

  CObList ButtonsList;
  if (CMFCToolBar::GetCommandButtons(ID_EDIT_FIND_COMBO, ButtonsList) > 0) {
    for (auto Position = ButtonsList.GetHeadPosition(); FoundCombo == nullptr && Position != nullptr;) {
      CMFCToolBarComboBoxButton* Combo = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, ButtonsList.GetNext(Position));

      if (Combo != nullptr && Combo->GetEditCtrl()->GetSafeHwnd() == ::GetFocus()) { FoundCombo = Combo; }
    }
  }
  return FoundCombo;
}
