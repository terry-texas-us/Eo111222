
#include "StdAfx.h"

#include <cassert>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include "AeSys.h"
#include "EoApOptions.h"
#include "EoCtrlColorComboBox.h"
#include "EoCtrlLineTypeComboBox.h"
#include "EoCtrlLineWeightComboBox.h"
#include "EoCtrlTextStyleComboBox.h"
#include "EoDlgTextStyleManager.h"
#include "EoMfVisualManager.h"
#include "MainFrm.h"
#include "Resource.h"
#include <algorithm>

namespace {

/// @brief Pixel (0,0) background key color of the IDR_STYLES toolbar bitmap.
/// Must match the background fill color in res/Toolbar Bitmaps/Styles_T-More-dark-24.bmp.
constexpr COLORREF kStylesToolbarBitmapKey = RGB(55, 55, 50);

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
    ID_SEPARATOR,  // 0: message pane (fixed ~36 characters)
    ID_INDICATOR_LENGTH,  // 1: dimension length display
    ID_INDICATOR_ANGLE,  // 2: dimension angle display
    ID_OP0,  // 3–12: mode key-command help panes
    ID_OP1, ID_OP2, ID_OP3, ID_OP4, ID_OP5, ID_OP6, ID_OP7, ID_OP8, ID_OP9,
    ID_INDICATOR_SCALE,  // 13: world scale display
    ID_INDICATOR_ZOOM,  // 14: zoom ratio display
    ID_SEPARATOR,  // 15: stretch filler — absorbs remaining space
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
ON_COMMAND(ID_TEXTSTYLE_BUTTON, &CMainFrame::OnTextStyleManager)
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
ON_UPDATE_COMMAND_UI(ID_LINEWEIGHT_COMBO, OnUpdateLineWeightCombo)
ON_UPDATE_COMMAND_UI(ID_TEXTSTYLE_COMBO, OnUpdateTextStyleCombo)
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

  // Default button/image sizes for initial toolbar creation. The actual button height
  // is adjusted after the first RecalcLayout, which creates the combo HWNDs via
  // OnShow → CreateCombo. At that point we measure the real combo closed height and
  // set all toolbars to match, ensuring icon-only and combo-containing toolbars
  // report identical row heights to the docking manager.
  const CSize imageSize(24, 24);
  const CSize buttonSize(32, 32);

  if (!m_standardToolBar.CreateEx(this, TBSTYLE_FLAT, Style, CRect(1, 1, 1, 1), IDR_MAINFRAME_24) ||
      !m_standardToolBar.LoadToolBar(IDR_MAINFRAME_24, 0, 0, TRUE)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create toolbar\n");
    return -1;
  }
  // SetSizes must be called AFTER LoadToolBar — LoadToolBar with bLocked=TRUE
  // recalculates m_sizeButton from resource dimensions, overriding any pre-set sizes.
  m_standardToolBar.SetSizes(buttonSize, imageSize);
  m_standardToolBar.SetWindowTextW(L"Standard");

  if (!m_renderPropertiesToolBar.CreateEx(this, TBSTYLE_FLAT, Style) ||
      !m_renderPropertiesToolBar.LoadToolBar(IDR_RENDER_PROPERTIES, 0, 0, TRUE)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create render properties toolbar\n");
    return -1;
  }
  // Match cell height to the standard toolbar so all toolbars on the same docking row
  // report identical height to the docking manager.
  m_renderPropertiesToolBar.SetSizes(buttonSize, imageSize);
  m_renderPropertiesToolBar.SetWindowTextW(L"Properties");

  if (!m_stylesToolBar.CreateEx(this, TBSTYLE_FLAT, Style) || !m_stylesToolBar.LoadToolBar(IDR_STYLES, 0, 0, TRUE)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create styles toolbar\n");
    return -1;
  }
  // Match cell height to the standard toolbar
  m_stylesToolBar.SetSizes(buttonSize, imageSize);
  m_stylesToolBar.SetWindowTextW(L"Styles");

  // Replace the bitmap background-key pixels with the current scheme's toolbarBackground,
  // then disable transparency. MFC's DrawEx composites onto a white-initialized intermediate
  // bitmap before SRCCOPY-ing to the target — disabling transparency eliminates that path
  // entirely, so background pixels paint as exactly toolbarBackground and blend seamlessly.
  if (auto* lockedImages = m_stylesToolBar.GetLockedImages()) {
    lockedImages->AdaptColors(kStylesToolbarBitmapKey, Eo::SchemeColors(Eo::activeColorScheme).toolbarBackground);
    lockedImages->SetTransparentColor(static_cast<COLORREF>(-1));
  }

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
  m_stylesToolBar.EnableDocking(CBRS_ALIGN_ANY);
  m_propertiesPane.EnableDocking(CBRS_ALIGN_ANY);
  m_outputPane.EnableDocking(CBRS_ALIGN_ANY);

  EnableDocking(CBRS_ALIGN_ANY);

  // Single shared row: [MenuBar][Standard][RenderProps][Styles]
  // DockPane inserts at row 0, so dock the rightmost toolbar first,
  // then build leftward with DockPaneLeftOf, ending with the menu bar.
  m_menuBar.SetExclusiveRowMode(FALSE);
  DockPane(&m_stylesToolBar);
  DockPaneLeftOf(&m_renderPropertiesToolBar, &m_stylesToolBar);
  DockPaneLeftOf(&m_standardToolBar, &m_renderPropertiesToolBar);
  DockPaneLeftOf(&m_menuBar, &m_standardToolBar);
  DockPane(&m_propertiesPane, AFX_IDW_DOCKBAR_LEFT);
  DockPane(&m_outputPane, AFX_IDW_DOCKBAR_BOTTOM);

  RecalcLayout();

  EnsureToolbarsVisible();

  ApplyColorScheme();

  // ApplyColorScheme calls LoadBitmap which can reset button sizes.
  // Re-apply combo-derived sizes after all bitmap loading is complete.
  AdjustToolbarSizesToMatchCombos();

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
  SetDockablePanesIcons();
  return TRUE;
}

void CMainFrame::SetDockablePanesIcons() {
  CSize smallIconSize(::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
  HINSTANCE resourceHandle(::AfxGetResourceHandle());

  HICON propertiesPaneIcon = static_cast<HICON>(LoadImageW(
      resourceHandle, MAKEINTRESOURCE(IDI_PROPERTIES_WND_HC), IMAGE_ICON, smallIconSize.cx, smallIconSize.cy, 0));
  m_propertiesPane.SetIcon(propertiesPaneIcon, FALSE);

  HICON outputPaneIcon = static_cast<HICON>(LoadImageW(
      resourceHandle, MAKEINTRESOURCE(IDI_OUTPUT_WND_HC), IMAGE_ICON, smallIconSize.cx, smallIconSize.cy, 0));
  m_outputPane.SetIcon(outputPaneIcon, FALSE);

  UpdateMDITabbedBarsIcons();
}

void CMainFrame::OnWindowManager() { ShowWindowsDialog(); }
void CMainFrame::OnViewCustomize() {
  CMFCToolBarsCustomizeDialog* Dialog = new CMFCToolBarsCustomizeDialog(this, TRUE);
  Dialog->EnableUserDefinedToolbars();
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
    case IDR_RENDER_PROPERTIES: {
      m_renderPropertiesToolBar.ReplaceButton(ID_PENCOLOR_COMBO, EoCtrlColorComboBox(), FALSE);
      m_renderPropertiesToolBar.ReplaceButton(ID_LINETYPE_COMBO, EoCtrlLineTypeComboBox(), FALSE);
      m_renderPropertiesToolBar.ReplaceButton(ID_LINEWEIGHT_COMBO, EoCtrlLineWeightComboBox(), FALSE);
      break;
    }
    case IDR_STYLES: {
      m_stylesToolBar.ReplaceButton(ID_TEXTSTYLE_COMBO, EoCtrlTextStyleComboBox(), FALSE);
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
  auto* UserToolsManager = app.GetUserToolsManager();
  if (UserToolsManager != nullptr && UserToolsManager->GetUserTools().IsEmpty()) {
    auto* tool1 = UserToolsManager->CreateNewTool();
    tool1->m_strLabel = L"&Notepad";
    tool1->SetCommand(L"notepad.exe");

    auto* tool2 = UserToolsManager->CreateNewTool();
    tool2->m_strLabel = L"Paint &Brush";
    tool2->SetCommand(L"mspaint.exe");

    auto* tool3 = UserToolsManager->CreateNewTool();
    tool3->m_strLabel = L"&File Explorer";
    tool3->SetCommand(L"explorer.exe");

    auto* tool4 = UserToolsManager->CreateNewTool();
    tool4->m_strLabel = L"Fanning, Fanning & Associates On-Line";
    tool4->SetCommand(L"http://www.fanningfanning.com");
  }

  // Enable customization button for user toolbars (standard and render properties are locked)
  auto customize = App::LoadStringResource(IDS_TOOLBAR_CUSTOMIZE);
  for (int i = 0; i < maxUserToolbars; i++) {
    auto* userToolbar = GetUserToolBarByIndex(i);
    if (userToolbar != nullptr) { userToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, customize); }
  }

  // Re-apply toolbar sizing after CDockingManager::LoadState restores docking layout
  // from the registry. LoadState (called from CMDIFrameWndEx::LoadFrame → LoadMDIState)
  // restores toolbar HWND positions and sizes from a previous session, overriding the
  // SetSizes values applied in OnCreate. EnsureToolbarsVisible catches toolbars hidden
  // by stale registry state. RecalcLayout forces the docking manager to re-query each
  // toolbar's preferred height (based on the still-valid button sizes from OnCreate).
  EnsureToolbarsVisible();
  AdjustToolbarSizesToMatchCombos();

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
      if (frame != nullptr) { frame->ModifyStyle(WS_SYSMENU, 0); }
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
void CMainFrame::EnsureToolbarsVisible() {
  // Safety net: after LoadMDIState() restores docking state from the registry,
  // verify that all application toolbars are visible. A stale or corrupted blob
  // (e.g. from a DPI change or structural toolbar change) can leave toolbars hidden.
  CMFCToolBar* toolbars[] = {&m_standardToolBar, &m_renderPropertiesToolBar, &m_stylesToolBar};
  for (auto* toolbar : toolbars) {
    if (!toolbar->IsVisible()) {
      toolbar->ShowPane(TRUE, FALSE, TRUE);
      CString name;
      toolbar->GetWindowText(name);
      ATLTRACE2(traceGeneral, 1, L"EnsureToolbarsVisible: forced toolbar '%s' visible\n", static_cast<LPCWSTR>(name));
    }
  }
}
void CMainFrame::AdjustToolbarSizesToMatchCombos() {
  // Measure the actual combo HWND closed height and set all toolbar button sizes
  // to match. This ensures icon-only and combo-containing toolbars report identical
  // row heights to the docking manager. Must be called after any operation that can
  // reset m_sizeButton (LoadBitmap, LoadToolBar, docking state restore).
  const CSize kImageSize(24, 24);
  CObList buttons;
  if (CMFCToolBar::GetCommandButtons(ID_PENCOLOR_COMBO, buttons) > 0) {
    auto* comboButton = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, buttons.GetHead());
    if (comboButton != nullptr) {
      auto* combo = comboButton->GetComboBox();
      if (combo != nullptr && combo->GetSafeHwnd() != nullptr) {
        CRect comboRect;
        combo->GetWindowRect(&comboRect);
        constexpr int comboVertMargin = 4;  // CMFCToolBarComboBoxButton::m_nVertMargin
        const int targetHeight = std::max(32, static_cast<int>(comboRect.Height()) + 2 * comboVertMargin);
        const CSize adjustedSize(std::max(32, targetHeight), targetHeight);
        ATLTRACE2(traceGeneral, 1, L"AdjustToolbarSizesToMatchCombos: combo=%d, button=%dx%d\n", comboRect.Height(),
            adjustedSize.cx, adjustedSize.cy);
        m_standardToolBar.SetSizesAll(adjustedSize, kImageSize);
        m_renderPropertiesToolBar.SetSizesAll(adjustedSize, kImageSize);
        m_stylesToolBar.SetSizesAll(adjustedSize, kImageSize);
        RecalcLayout();
      }
    }
  }
}
void CMainFrame::ApplyColorScheme() {
  auto* visualManager = dynamic_cast<EoMfVisualManager*>(CMFCVisualManager::GetInstance());
  if (visualManager != nullptr) { visualManager->RefreshColors(); }
  ApplyDwmDarkMode(GetSafeHwnd(), Eo::activeColorScheme == Eo::ColorScheme::Dark);

  // Refresh Windows dark mode state for common controls (scroll bars, context menus)
  HMODULE uxThemeModule = ::GetModuleHandleW(L"uxtheme.dll");
  if (uxThemeModule != nullptr) {
    // SetPreferredAppMode (ordinal 135): 2=ForceDark, 3=ForceLight
    using SetPreferredAppMode_t = DWORD(WINAPI*)(int);
    auto setPreferredAppMode =
        reinterpret_cast<SetPreferredAppMode_t>(::GetProcAddress(uxThemeModule, MAKEINTRESOURCEA(135)));
    if (setPreferredAppMode != nullptr) { setPreferredAppMode(Eo::activeColorScheme == Eo::ColorScheme::Dark ? 2 : 3); }
    // FlushMenuThemes (ordinal 136): forces menus to re-read theme data
    using FlushMenuThemes_t = void(WINAPI*)();
    auto flushMenuThemes = reinterpret_cast<FlushMenuThemes_t>(::GetProcAddress(uxThemeModule, MAKEINTRESOURCEA(136)));
    if (flushMenuThemes != nullptr) { flushMenuThemes(); }
    // RefreshImmersiveColorPolicyState (ordinal 104): refreshes dark/light policy
    using RefreshPolicy_t = void(WINAPI*)();
    auto refreshPolicy = reinterpret_cast<RefreshPolicy_t>(::GetProcAddress(uxThemeModule, MAKEINTRESOURCEA(104)));
    if (refreshPolicy != nullptr) { refreshPolicy(); }
  }

  // Set status bar text color for all panes to match the active scheme
  const auto& schemeColors = Eo::SchemeColors(Eo::activeColorScheme);
  int paneCount = m_statusBar.GetCount();
  for (int i = 0; i < paneCount; i++) { m_statusBar.SetPaneTextColor(i, schemeColors.statusBarText, FALSE); }

  m_propertiesPane.ApplyColorScheme();
  m_outputPane.ApplyColorScheme();

  // Reload and adapt styles toolbar bitmap for the active color scheme.
  m_stylesToolBar.CleanUpLockedImages();
  m_stylesToolBar.LoadBitmap(IDR_STYLES, 0U, 0U, TRUE, 0U, 0U);
  if (auto* lockedImages = m_stylesToolBar.GetLockedImages()) {
    lockedImages->AdaptColors(kStylesToolbarBitmapKey, Eo::SchemeColors(Eo::activeColorScheme).toolbarBackground);
    lockedImages->SetTransparentColor(static_cast<COLORREF>(-1));
  }

  // Swap the toolbar bitmap for the active color scheme.
  // LoadToolBar(id, ..., bLocked=TRUE) stores the initial light-scheme bitmap in
  // m_ImagesLocked. CMFCToolBar renders from m_Images when non-empty and falls back
  // to m_ImagesLocked otherwise. The strategy:
  //   1. Clear m_Images so a failed LoadBitmap leaves the slot empty (not stale).
  //   2. LoadBitmap(id) writes to m_Images (bLocked=FALSE, the default).
  // Dark bitmaps (BITMAP-only resources): Load succeeds → m_Images takes priority.
  // Light bitmaps (shared TOOLBAR+BITMAP): Load may silently fail on some MFC builds
  //   → m_Images stays empty after Clear → falls back to m_ImagesLocked (correct).
  {
    const UINT imageId = Eo::activeColorScheme == Eo::ColorScheme::Dark ? IDR_MAINFRAME_24_DARK : IDR_MAINFRAME_24;
    m_standardToolBar.GetImages()->Clear();

    if (!m_standardToolBar.LoadBitmap(imageId)) {
      ATLTRACE2(traceGeneral, 1, L"Failed to load toolbar bitmap %u\n", imageId);
    }
  }

  // LoadBitmap can reset button sizes. Re-apply combo-derived sizes.
  AdjustToolbarSizesToMatchCombos();

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

void CMainFrame::OnUpdateLineWeightCombo(CCmdUI* pCmdUI) { pCmdUI->Enable(TRUE); }

void CMainFrame::SyncLineWeightCombo(EoDxfLineWeights::LineWeight lineWeight) {
  CObList buttonsList;
  if (CMFCToolBar::GetCommandButtons(ID_LINEWEIGHT_COMBO, buttonsList) > 0) {
    for (auto pos = buttonsList.GetHeadPosition(); pos != nullptr;) {
      auto* button = DYNAMIC_DOWNCAST(EoCtrlLineWeightComboBox, buttonsList.GetNext(pos));
      if (button != nullptr) {
        button->SetCurrentLineWeight(lineWeight);
        break;
      }
    }
  }
}

void CMainFrame::OnUpdateTextStyleCombo(CCmdUI* pCmdUI) { pCmdUI->Enable(TRUE); }

void CMainFrame::SyncTextStyleCombo(const std::wstring& textStyleName) {
  CObList buttonsList;
  if (CMFCToolBar::GetCommandButtons(ID_TEXTSTYLE_COMBO, buttonsList) > 0) {
    for (auto pos = buttonsList.GetHeadPosition(); pos != nullptr;) {
      auto* button = DYNAMIC_DOWNCAST(EoCtrlTextStyleComboBox, buttonsList.GetNext(pos));
      if (button != nullptr) {
        button->SetCurrentTextStyle(textStyleName);
        break;
      }
    }
  }
}

void CMainFrame::OnTextStyleManager() {
  EoDlgTextStyleManager dialog(this);
  dialog.DoModal();

  // After the dialog closes, refresh the text style combo in case styles changed
  CObList buttonsList;
  if (CMFCToolBar::GetCommandButtons(ID_TEXTSTYLE_COMBO, buttonsList) > 0) {
    for (auto pos = buttonsList.GetHeadPosition(); pos != nullptr;) {
      auto* button = DYNAMIC_DOWNCAST(EoCtrlTextStyleComboBox, buttonsList.GetNext(pos));
      if (button != nullptr) {
        button->PopulateItems();
        break;
      }
    }
  }
}
