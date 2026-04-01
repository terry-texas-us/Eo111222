
#include "StdAfx.h"

#include <cassert>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include "AeSys.h"
#include "EoApOptions.h"
#include "EoCtrlColorComboBox.h"
#include "EoCtrlFindComboBox.h"
#include "EoCtrlLineTypeComboBox.h"
#include "EoMfVisualManager.h"
#include "MainFrm.h"
#include "Resource.h"

namespace {

/// @brief Applies the DWM immersive dark mode attribute to the window title bar.
/// Requires Windows 10 1809+ (build 17763). Silently ignored on older Windows.
/// On Windows 11 (build 22000+), also sets a custom caption color for warm tinting.
void ApplyDwmDarkMode(HWND hwnd, bool darkMode) {
  // DWMWA_USE_IMMERSIVE_DARK_MODE = 20 (Windows 10 20H1+; 19 on older insider builds)
  constexpr DWORD dwmwaUseImmersiveDarkMode = 20;
  BOOL useDarkMode = darkMode ? TRUE : FALSE;
  ::DwmSetWindowAttribute(hwnd, dwmwaUseImmersiveDarkMode, &useDarkMode, sizeof(useDarkMode));

  // DWMWA_CAPTION_COLOR = 35 (Windows 11 build 22000+). Sets a custom title bar background color.
  // Silently fails on Windows 10 where the attribute is not supported.
  constexpr DWORD dwmwaCaptionColor = 35;
  const auto& colors = Eo::SchemeColors(Eo::activeColorScheme);
  COLORREF captionColor = colors.toolbarBackground;
  ::DwmSetWindowAttribute(hwnd, dwmwaCaptionColor, &captionColor, sizeof(captionColor));
}

constexpr int statusInfo = 0;
constexpr int statusLength = 1;
constexpr int statusAngle = 2;
constexpr int statusScale = 13;
constexpr int statusZoom = 14;
constexpr int maxUserToolbars = 10;
constexpr unsigned int firstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
constexpr unsigned int lastUserToolBarId = firstUserToolBarId + maxUserToolbars - 1;
constexpr unsigned int indicators[] = {
    ID_SEPARATOR,           // 0: message pane (fixed ~36 characters)
    ID_INDICATOR_LENGTH,    // 1: dimension length display
    ID_INDICATOR_ANGLE,     // 2: dimension angle display
    ID_OP0,                 // 3–12: mode key-command help panes
    ID_OP1,
    ID_OP2,
    ID_OP3,
    ID_OP4,
    ID_OP5,
    ID_OP6,
    ID_OP7,
    ID_OP8,
    ID_OP9,
    ID_INDICATOR_SCALE,     // 13: world scale display
    ID_INDICATOR_ZOOM,      // 14: zoom ratio display
    ID_SEPARATOR,           // 15: stretch filler — absorbs remaining space
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
ON_REGISTERED_MESSAGE(AFX_WM_TOOLBARMENU, OnToolbarContextMenu)
ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, OnGetTabToolTip)
ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, OnToolbarReset)
#pragma warning(pop)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_UPDATE_COMMAND_UI(ID_MDI_TABBED, OnUpdateMdiTabbed)
ON_UPDATE_COMMAND_UI(ID_PENCOLOR_COMBO, OnUpdatePenColorCombo)
ON_UPDATE_COMMAND_UI(ID_LINETYPE_COMBO, OnUpdateLineTypeCombo)
#pragma warning(pop)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() {}
CMainFrame::~CMainFrame() {}
int CMainFrame::OnCreate(LPCREATESTRUCT createStruct) {
  if (CMDIFrameWndEx::OnCreate(createStruct) == -1) { return -1; }

  CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(EoMfVisualManager));

  // Configure MDI tabbed groups with hardcoded settings (no longer user-configurable)
  {
    CMDITabInfo tabInfo;
    tabInfo.m_tabLocation = CMFCTabCtrl::LOCATION_TOP;
    tabInfo.m_style = CMFCTabCtrl::STYLE_3D;
    tabInfo.m_bTabIcons = FALSE;
    tabInfo.m_bTabCloseButton = FALSE;
    tabInfo.m_bTabCustomTooltips = TRUE;
    tabInfo.m_bAutoColor = FALSE;
    tabInfo.m_bDocumentMenu = TRUE;
    tabInfo.m_bEnableTabSwap = TRUE;
    tabInfo.m_bFlatFrame = TRUE;
    tabInfo.m_bActiveTabCloseButton = TRUE;
    tabInfo.m_nTabBorderSize = 0;
    EnableMDITabbedGroups(TRUE, tabInfo);
  }

  if (!m_menuBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create menubar\n");
    return -1;
  }
  m_menuBar.SetPaneStyle(m_menuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

  // Prevent the menu bar from taking the focus on activation
  CMFCPopupMenu::SetForceMenuFocus(FALSE);
  DWORD Style(WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
  if (!m_standardToolBar.CreateEx(this, TBSTYLE_FLAT, Style) ||
      !m_standardToolBar.LoadToolBar(static_cast<UINT>(app.HighColorMode() ? IDR_MAINFRAME_256 : IDR_MAINFRAME))) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create toolbar\n");
    return -1;
  }
  m_standardToolBar.SetWindowTextW(L"Standard");
  m_standardToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, L"Customize...");

  if (!m_renderPropertiesToolBar.CreateEx(this, TBSTYLE_FLAT, Style) ||
      !m_renderPropertiesToolBar.LoadToolBar(IDR_RENDER_PROPERTIES)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create render properties toolbar\n");
    return -1;
  }
  m_renderPropertiesToolBar.SetWindowTextW(L"Properties");
  m_renderPropertiesToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, L"Customize...");

  InitUserToolbars(nullptr, firstUserToolBarId, lastUserToolBarId);

  if (!m_statusBar.Create(this)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create status bar\n");
    return -1;
  }
  // Remove the size gripper — modern apps allow resizing from any window edge/corner
  m_statusBar.ModifyStyle(SBARS_SIZEGRIP, 0);
  m_statusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(unsigned int));

  // Message pane: fixed width (~36 characters at default font size)
  m_statusBar.SetPaneInfo(statusInfo, ID_SEPARATOR, SBPS_NOBORDERS, 288);

  // Length and Angle panes: fixed width for dimension display
  m_statusBar.SetPaneInfo(statusLength, ID_INDICATOR_LENGTH, SBPS_NOBORDERS, 120);
  m_statusBar.SetPaneInfo(statusAngle, ID_INDICATOR_ANGLE, SBPS_NOBORDERS, 100);

  // World Scale and Zoom Ratio panes: fixed width, placed after mode panes
  m_statusBar.SetPaneInfo(statusScale, ID_INDICATOR_SCALE, SBPS_NOBORDERS, 120);
  m_statusBar.SetPaneInfo(statusZoom, ID_INDICATOR_ZOOM, SBPS_NOBORDERS, 100);

  // Trailing stretch filler: absorbs remaining space after all fixed panes
  m_statusBar.SetPaneStyle(15, SBPS_STRETCH | SBPS_NOBORDERS);

  if (!CreateDockablePanes()) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create dockable panes\n");
    return -1;
  }
  m_menuBar.EnableDocking(CBRS_ALIGN_ANY);
  m_standardToolBar.EnableDocking(CBRS_ALIGN_ANY);
  m_renderPropertiesToolBar.EnableDocking(CBRS_ALIGN_ANY);
  m_propertiesPane.EnableDocking(CBRS_ALIGN_ANY);
  m_outputPane.EnableDocking(CBRS_ALIGN_ANY);

  EnableDocking(CBRS_ALIGN_ANY);

  DockPane(&m_menuBar);
  DockPane(&m_standardToolBar);
  DockPane(&m_renderPropertiesToolBar);
  DockPane(&m_propertiesPane);
  DockPane(&m_outputPane);

  ApplyColorScheme();

  EnableAutoHidePanes(CBRS_ALIGN_ANY);

  EnableWindowsDialog(ID_WINDOW_MANAGER, IDS_WINDOWS_MANAGER, TRUE);

  // Enable automatic creation and management of the pop-up pane menu, which displays a list of application panes.
  EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, L"Customize...", ID_VIEW_TOOLBARS, FALSE, FALSE);
  EnableFullScreenMode(ID_VIEW_FULLSCREEN);
  EnableFullScreenMainMenu(TRUE);

  CMFCToolBar::EnableQuickCustomization();

  if (CMFCToolBar::GetUserImages() == nullptr) {
    // load user-defined toolbar images
    if (m_userImages.Load(L".\\res\\UserImages.bmp")) {
      m_userImages.SetImageSize(CSize(16, 16), FALSE);
      CMFCToolBar::SetUserImages(&m_userImages);
    }
  }
  // Shows the document name after thumbnail before the application name in a frame window title.
  ModifyStyle(0, FWS_PREFIXTITLE);

  ApplyDwmDarkMode(GetSafeHwnd(), Eo::activeColorScheme == Eo::ColorScheme::Dark);

  return 0;
}
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) {
  if (!CMDIFrameWndEx::PreCreateWindow(cs)) { return FALSE; }

  return TRUE;
}

BOOL CMainFrame::CreateDockablePanes() {
  CSize defaultSize(200, 200);

  const DWORD sharedStyles(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI);

  auto Caption = App::LoadStringResource(IDS_OUTPUT);
  if (!m_outputPane.Create(Caption, this, defaultSize, TRUE, ID_VIEW_OUTPUTWND, sharedStyles | CBRS_BOTTOM)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create Output pane\n");
    return FALSE;
  }
  Caption = App::LoadStringResource(IDS_PROPERTIES);
  if (!m_propertiesPane.Create(Caption, this, defaultSize, TRUE, ID_VIEW_PROPERTIESWND, sharedStyles | CBRS_RIGHT)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create Properties pane\n");
    return FALSE;
  }
  SetDockablePanesIcons(app.HighColorMode());
  return TRUE;
}

void CMainFrame::SetDockablePanesIcons(bool highColorMode) {
  CSize smallIconSize(::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
  HINSTANCE resourceHandle(::AfxGetResourceHandle());

  HICON propertiesPaneIcon = static_cast<HICON>(
      LoadImageW(resourceHandle, MAKEINTRESOURCE(highColorMode ? IDI_PROPERTIES_WND_HC : IDI_PROPERTIES_WND),
          IMAGE_ICON, smallIconSize.cx, smallIconSize.cy, 0));
  m_propertiesPane.SetIcon(propertiesPaneIcon, FALSE);

  HICON outputPaneIcon =
      static_cast<HICON>(LoadImageW(resourceHandle, MAKEINTRESOURCE(highColorMode ? IDI_OUTPUT_WND_HC : IDI_OUTPUT_WND),
          IMAGE_ICON, smallIconSize.cx, smallIconSize.cy, 0));
  m_outputPane.SetIcon(outputPaneIcon, FALSE);

  UpdateMDITabbedBarsIcons();
}

void CMainFrame::OnWindowManager() { ShowWindowsDialog(); }
void CMainFrame::OnViewCustomize() {
  CMFCToolBarsCustomizeDialog* Dialog = new CMFCToolBarsCustomizeDialog(this, TRUE);
  Dialog->EnableUserDefinedToolbars();

  // Setup combobox:
  Dialog->ReplaceButton(ID_EDIT_FIND, EoCtrlFindComboBox());

  Dialog->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp, LPARAM name) {
  LRESULT result = CMDIFrameWndEx::OnToolbarCreateNew(wp, name);
  if (result == 0) { return 0; }
  auto* userToolbar = (CMFCToolBar*)result;
  assert(userToolbar != nullptr);

  auto customize = App::LoadStringResource(IDS_TOOLBAR_CUSTOMIZE);

  userToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, customize);
  return result;
}

LRESULT CMainFrame::OnToolbarReset(WPARAM toolbarResourceId, LPARAM lparam) {
  ATLTRACE2(traceGeneral, 3, L"CMainFrame::OnToolbarReset(%i, %i)\n", toolbarResourceId, lparam);

  switch (toolbarResourceId) {
    case IDR_MAINFRAME:
    case IDR_MAINFRAME_256: {
      m_standardToolBar.ReplaceButton(ID_EDIT_FIND, EoCtrlFindComboBox(), FALSE);
      break;
    }
    case IDR_RENDER_PROPERTIES: {
      m_renderPropertiesToolBar.ReplaceButton(ID_PENCOLOR_COMBO, EoCtrlColorComboBox(), FALSE);
      m_renderPropertiesToolBar.ReplaceButton(ID_LINETYPE_COMBO, EoCtrlLineTypeComboBox(), FALSE);
      break;
    }
    case IDR_PROPERTIES:
      break;
  }
  return 0;
}
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
  auto Customize = App::LoadStringResource(IDS_TOOLBAR_CUSTOMIZE);

  for (int i = 0; i < maxUserToolbars; i++) {
    CMFCToolBar* UserToolbar = GetUserToolBarByIndex(i);
    if (UserToolbar != nullptr) { UserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, Customize); }
  }
  return TRUE;
}
LRESULT CMainFrame::OnToolbarContextMenu(WPARAM, LPARAM point) {
  CMenu PopupToolbarMenu;
  VERIFY(PopupToolbarMenu.LoadMenu(IDR_POPUP_TOOLBAR));

  auto* SubMenu = PopupToolbarMenu.GetSubMenu(0);
  assert(SubMenu != nullptr);

  if (SubMenu) {
    CPoint Point(AFX_GET_X_LPARAM(point), AFX_GET_Y_LPARAM(point));

    auto* PopupMenu = new CMFCPopupMenu;
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

    auto* PopupSubMenu = menu.GetSubMenu(0);
    assert(PopupSubMenu != nullptr);

    if (PopupSubMenu) { pMenuPopup->GetMenuBar()->ImportFromMenu(*PopupSubMenu, TRUE); }
  }
  return TRUE;
}

void CMainFrame::UpdateMDITabs(BOOL resetMDIChild) {
  CList<UINT, UINT> lstCommands;
  if (AreMDITabs(nullptr)) {
    lstCommands.AddTail(ID_WINDOW_ARRANGE);
    lstCommands.AddTail(ID_WINDOW_CASCADE);
    lstCommands.AddTail(ID_WINDOW_TILE_HORZ);
    lstCommands.AddTail(ID_WINDOW_TILE_VERT);
  }
  CMFCToolBar::SetNonPermittedCommands(lstCommands);
  if (resetMDIChild) {
    HWND hwndT = ::GetWindow(m_hWndMDIClient, GW_CHILD);
    while (hwndT != nullptr) {
      CMDIChildWndEx* frame = DYNAMIC_DOWNCAST(CMDIChildWndEx, CWnd::FromHandle(hwndT));
      if (frame != nullptr) {
        frame->ModifyStyle(WS_SYSMENU, 0);
      }
      hwndT = ::GetWindow(hwndT, GW_HWNDNEXT);
    }
    m_menuBar.SetMaximizeMode(FALSE);
  }
  if (m_propertiesPane.IsAutoHideMode()) {
    m_propertiesPane.BringWindowToTop();
    auto* divider = m_propertiesPane.GetDefaultPaneDivider();
    if (divider != nullptr) { divider->BringWindowToTop(); }
  }
  CMDIFrameWndEx::m_bDisableSetRedraw = TRUE;

  RecalcLayout();
  RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

BOOL CMainFrame::OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL bDrop) {
  if (bDrop) { return FALSE; }
  CMenu menu;
  VERIFY(menu.LoadMenu(IDR_POPUP_MDITABS));

  auto* PopupSubMenu = menu.GetSubMenu(0);
  assert(PopupSubMenu != nullptr);

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
  auto* toolTipInfo = (CMFCTabToolTipInfo*)lp;
  assert(toolTipInfo != nullptr);

  if (toolTipInfo) {
    assert(toolTipInfo->m_pTabWnd != nullptr);
    if (!toolTipInfo->m_pTabWnd->IsMDITab()) { return 0; }
    toolTipInfo->m_strText.Format(L"Tab #%d Custom Tooltip", toolTipInfo->m_nTabIndex + 1);
  }
  return 0;
}

void CMainFrame::OnMdiTabbed() {
  CMDIChildWndEx* pMDIChild = DYNAMIC_DOWNCAST(CMDIChildWndEx, MDIGetActive());
  if (pMDIChild == nullptr) {
    assert(FALSE);
    return;
  }
  TabbedDocumentToControlBar(pMDIChild);
}
void CMainFrame::OnUpdateMdiTabbed(CCmdUI* pCmdUI) { pCmdUI->SetCheck(); }
void CMainFrame::OnDestroy() {
  ATLTRACE2(traceGeneral, 3, L"CMainFrame::OnDestroy() - Entering\n");
  PostQuitMessage(0);  // Force WM_QUIT message to terminate message loop
}
CString CMainFrame::GetPaneText(int index) { return m_statusBar.GetPaneText(index); }
void CMainFrame::SetPaneInfo(int index, UINT newId, UINT style, int width) {
  m_statusBar.SetPaneInfo(index, newId, style, width);
}
BOOL CMainFrame::SetPaneText(int index, LPCWSTR newText) { return m_statusBar.SetPaneText(index, newText); }
void CMainFrame::SetPaneStyle(int index, UINT style) { m_statusBar.SetPaneStyle(index, style); }
void CMainFrame::SetPaneTextColor(int index, COLORREF textColor) { m_statusBar.SetPaneTextColor(index, textColor); }
void CMainFrame::SetPaneBackgroundColor(int index, COLORREF backgroundColor) {
  m_statusBar.SetPaneBackgroundColor(index, backgroundColor);
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
void CMainFrame::ApplyColorScheme() {
  auto* visualManager = dynamic_cast<EoMfVisualManager*>(CMFCVisualManager::GetInstance());
  if (visualManager != nullptr) {
    visualManager->RefreshColors();
  }
  ApplyDwmDarkMode(GetSafeHwnd(), Eo::activeColorScheme == Eo::ColorScheme::Dark);

  // Refresh Windows dark mode state for common controls (scroll bars, context menus)
  HMODULE uxThemeModule = ::GetModuleHandleW(L"uxtheme.dll");
  if (uxThemeModule != nullptr) {
    // SetPreferredAppMode (ordinal 135): 2=ForceDark, 3=ForceLight
    using SetPreferredAppMode_t = DWORD(WINAPI*)(int);
    auto setPreferredAppMode =
        reinterpret_cast<SetPreferredAppMode_t>(::GetProcAddress(uxThemeModule, MAKEINTRESOURCEA(135)));
    if (setPreferredAppMode != nullptr) {
      setPreferredAppMode(Eo::activeColorScheme == Eo::ColorScheme::Dark ? 2 : 3);
    }
    // FlushMenuThemes (ordinal 136): forces menus to re-read theme data
    using FlushMenuThemes_t = void(WINAPI*)();
    auto flushMenuThemes =
        reinterpret_cast<FlushMenuThemes_t>(::GetProcAddress(uxThemeModule, MAKEINTRESOURCEA(136)));
    if (flushMenuThemes != nullptr) {
      flushMenuThemes();
    }
    // RefreshImmersiveColorPolicyState (ordinal 104): refreshes dark/light policy
    using RefreshPolicy_t = void(WINAPI*)();
    auto refreshPolicy =
        reinterpret_cast<RefreshPolicy_t>(::GetProcAddress(uxThemeModule, MAKEINTRESOURCEA(104)));
    if (refreshPolicy != nullptr) {
      refreshPolicy();
    }
  }

  // Set status bar text color for all panes to match the active scheme
  const auto& schemeColors = Eo::SchemeColors(Eo::activeColorScheme);
  int paneCount = m_statusBar.GetCount();
  for (int i = 0; i < paneCount; i++) {
    m_statusBar.SetPaneTextColor(i, schemeColors.statusBarText, FALSE);
  }

  m_propertiesPane.ApplyColorScheme();
  m_outputPane.ApplyColorScheme();
  RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void CMainFrame::OnUpdatePenColorCombo(CCmdUI* pCmdUI) { pCmdUI->Enable(TRUE); }

void CMainFrame::SyncColorCombo(std::int16_t aciIndex) {
  CObList buttonsList;
  if (CMFCToolBar::GetCommandButtons(ID_PENCOLOR_COMBO, buttonsList) > 0) {
    for (auto pos = buttonsList.GetHeadPosition(); pos != nullptr;) {
      auto* button = DYNAMIC_DOWNCAST(EoCtrlColorComboBox, buttonsList.GetNext(pos));
      if (button != nullptr) {
        button->SetCurrentColor(aciIndex);
        break;
      }
    }
  }
}

void CMainFrame::OnUpdateLineTypeCombo(CCmdUI* pCmdUI) { pCmdUI->Enable(TRUE); }

void CMainFrame::SyncLineTypeCombo(std::int16_t lineTypeIndex, const std::wstring& lineTypeName) {
  CObList buttonsList;
  if (CMFCToolBar::GetCommandButtons(ID_LINETYPE_COMBO, buttonsList) > 0) {
    for (auto pos = buttonsList.GetHeadPosition(); pos != nullptr;) {
      auto* button = DYNAMIC_DOWNCAST(EoCtrlLineTypeComboBox, buttonsList.GetNext(pos));
      if (button != nullptr) {
        button->SetCurrentLineType(lineTypeIndex, lineTypeName);
        break;
      }
    }
  }
}
