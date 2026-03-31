#include "Stdafx.h"

#include <Uxtheme.h>
#pragma comment(lib, "UxTheme.lib")

#include <algorithm>
#include <cassert>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoMfPropertiesDockablePane.h"
#include "Resource.h"

BEGIN_MESSAGE_MAP(EoMfPropertiesDockablePane, CDockablePane)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_CREATE()
ON_WM_SETFOCUS()
ON_WM_SETTINGCHANGE()
ON_WM_SIZE()
#pragma warning(pop)
ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
ON_COMMAND(ID_PROPERTIES1, OnProperties1)
ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
#pragma warning(pop)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
ON_UPDATE_COMMAND_UI(ID_PROPERTIES1, OnUpdateProperties1)
ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
#pragma warning(pop)
END_MESSAGE_MAP()

EoMfPropertiesDockablePane::EoMfPropertiesDockablePane() {}
EoMfPropertiesDockablePane::~EoMfPropertiesDockablePane() {}
/// @brief Callback for EnumChildWindows — applies SetWindowTheme to each child HWND.
static BOOL CALLBACK ApplyThemeToChildWindows(HWND childHwnd, LPARAM lParam) {
  auto* themeName = reinterpret_cast<const wchar_t*>(lParam);
  ::SetWindowTheme(childHwnd, themeName, nullptr);
  return TRUE;
}

void EoMfPropertiesDockablePane::ApplyColorScheme() {
  const auto& colors = Eo::SchemeColors(Eo::activeColorScheme);
  m_PropertyGrid.SetCustomColors(colors.paneBackground, colors.paneText, colors.paneGroupBackground,
      colors.paneGroupText, colors.paneDescriptionBackground, colors.paneDescriptionText, colors.paneLine);

  // Reload toolbar images from the original bitmap, then adapt for dark theme if needed
  m_PropertiesToolBar.CleanUpLockedImages();
  m_PropertiesToolBar.LoadBitmap(
      static_cast<UINT>(app.HighColorMode() ? IDB_PROPERTIES_HC : IDR_PROPERTIES), 0U, 0U, TRUE, 0U, 0U);
  if (Eo::activeColorScheme == Eo::ColorScheme::Dark) {
    if (auto* lockedImages = m_PropertiesToolBar.GetLockedImages()) {
      lockedImages->AdaptColors(RGB(0, 0, 0), RGB(200, 200, 200));
    }
  }

  // Apply dark/light scroll bar theme to the property grid and all its children (internal scrollbar)
  const wchar_t* themeName = (Eo::activeColorScheme == Eo::ColorScheme::Dark) ? L"DarkMode_Explorer" : L"Explorer";
  ::SetWindowTheme(m_PropertyGrid.GetSafeHwnd(), themeName, nullptr);
  ::EnumChildWindows(m_PropertyGrid.GetSafeHwnd(), ApplyThemeToChildWindows, reinterpret_cast<LPARAM>(themeName));
  ::SetWindowTheme(GetSafeHwnd(), themeName, nullptr);

  m_PropertyGrid.Invalidate();
}
int EoMfPropertiesDockablePane::OnCreate(LPCREATESTRUCT createStruct) {
  if (CDockablePane::OnCreate(createStruct) == -1) { return -1; }
  CRect EmptyRect;
  EmptyRect.SetRectEmpty();

  if (!m_PropertyGrid.Create(WS_VISIBLE | WS_CHILD, EmptyRect, this, 2)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create Properties Grid \n");
    return -1;
  }
  InitializePropertyGrid();

  m_PropertiesToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
  m_PropertiesToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE, 0, 0, 0);
  m_PropertiesToolBar.CleanUpLockedImages();
  m_PropertiesToolBar.LoadBitmap(
      static_cast<UINT>(app.HighColorMode() ? IDB_PROPERTIES_HC : IDR_PROPERTIES), 0U, 0U, TRUE, 0U, 0U);

  // In dark theme, adapt toolbar button images from dark glyphs to light for visibility
  if (Eo::activeColorScheme == Eo::ColorScheme::Dark) {
    if (auto* lockedImages = m_PropertiesToolBar.GetLockedImages()) {
      lockedImages->AdaptColors(RGB(0, 0, 0), RGB(200, 200, 200));
    }
  }

  m_PropertiesToolBar.SetPaneStyle(m_PropertiesToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
  m_PropertiesToolBar.SetPaneStyle(
      m_PropertiesToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM |
                                               CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
  m_PropertiesToolBar.SetOwner(this);

  // All commands will be routed via this control , not via the parent frame:
  m_PropertiesToolBar.SetRouteCommandsViaFrame(FALSE);

  // Apply dark scroll bar theme at creation time (including property grid internal scrollbar)
  if (Eo::activeColorScheme == Eo::ColorScheme::Dark) {
    ::SetWindowTheme(m_PropertyGrid.GetSafeHwnd(), L"DarkMode_Explorer", nullptr);
    ::EnumChildWindows(
        m_PropertyGrid.GetSafeHwnd(), ApplyThemeToChildWindows, reinterpret_cast<LPARAM>(L"DarkMode_Explorer"));
    ::SetWindowTheme(GetSafeHwnd(), L"DarkMode_Explorer", nullptr);
  }

  AdjustLayout();
  return 0;
}
void EoMfPropertiesDockablePane::OnSetFocus(CWnd* oldWindow) {
  CDockablePane::OnSetFocus(oldWindow);
  m_PropertyGrid.SetFocus();
}
void EoMfPropertiesDockablePane::OnSettingChange(UINT flags, LPCWSTR section) {
  CDockablePane::OnSettingChange(flags, section);
  SetPropertyGridFont();
}
void EoMfPropertiesDockablePane::OnSize(UINT type, int cx, int cy) {
  CDockablePane::OnSize(type, cx, cy);
  AdjustLayout();
}
LRESULT EoMfPropertiesDockablePane::OnPropertyChanged(WPARAM, LPARAM lparam) {
  CMFCPropertyGridProperty* Property = (CMFCPropertyGridProperty*)lparam;

  switch (int(Property->GetData())) {
    case kActiveViewScale: {
      auto* activeView = AeSysView::GetActiveView();
      activeView->SetWorldScale(Property->GetValue().dblVal);
      activeView->UpdateStateInformation(AeSysView::Scale);
      return LRESULT(0);
    }
  }
  return LRESULT(0);
}

void EoMfPropertiesDockablePane::OnExpandAllProperties() { m_PropertyGrid.ExpandAll(); }
void EoMfPropertiesDockablePane::OnProperties1() {
  ATLTRACE2(traceGeneral, 3, L"EoMfPropertiesDockablePane::OnProperties1\n");
}
void EoMfPropertiesDockablePane::OnSortProperties() {
  m_PropertyGrid.SetAlphabeticMode(!m_PropertyGrid.IsAlphabeticMode());
}
void EoMfPropertiesDockablePane::OnUpdateExpandAllProperties(CCmdUI* pCmdUI) {
  (void)pCmdUI;
  ATLTRACE2(traceGeneral, 4, L"EoMfPropertiesDockablePane::OnUpdatExpandAllProperties\n");
}
void EoMfPropertiesDockablePane::OnUpdateProperties1(CCmdUI* pCmdUI) {
  (void)pCmdUI;
  ATLTRACE2(traceGeneral, 4, L"EoMfPropertiesDockablePane::OnUpdatProperties1\n");
}
void EoMfPropertiesDockablePane::OnUpdateSortProperties(CCmdUI* pCmdUI) {
  pCmdUI->SetCheck(m_PropertyGrid.IsAlphabeticMode());
}
void EoMfPropertiesDockablePane::AdjustLayout() {
  if (GetSafeHwnd() == nullptr) { return; }
  CRect rectClient;
  GetClientRect(rectClient);

  int cyTlb = m_PropertiesToolBar.CalcFixedLayout(FALSE, TRUE).cy;

  m_PropertiesToolBar.SetWindowPos(
      nullptr, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
  m_PropertyGrid.SetWindowPos(nullptr, rectClient.left, rectClient.top + cyTlb, rectClient.Width(),
      rectClient.Height() - cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}

void EoMfPropertiesDockablePane::InitializePropertyGrid() {
  SetPropertyGridFont();

  m_PropertyGrid.EnableHeaderCtrl(FALSE, L"Property", L"Value");
  m_PropertyGrid.EnableDescriptionArea(TRUE);

  m_PropertyGrid.SetVSDotNetLook(TRUE);
  m_PropertyGrid.SetGroupNameFullWidth(TRUE, TRUE);

  m_PropertyGrid.MarkModifiedProperties(TRUE, TRUE);

  auto* activeView = AeSysView::GetActiveView();
  double scale = (activeView == nullptr) ? 1.0 : activeView->GetWorldScale();

  auto* activeViewGroup = new CMFCPropertyGridProperty(L"Active View");
  auto* worldScaleProperty = new CMFCPropertyGridProperty(
      L"World Scale", (_variant_t)scale, L"Specifies the world scale used in the Active View", kActiveViewScale);
  activeViewGroup->AddSubItem(worldScaleProperty);
  activeViewGroup->AddSubItem(new CMFCPropertyGridProperty(
      L"Use True Type fonts", (_variant_t) true, L"Specifies that the Active View uses True Type fonts"));
  m_PropertyGrid.AddProperty(activeViewGroup);
  worldScaleProperty->Enable(activeView != nullptr);

  auto* appearanceGroup = new CMFCPropertyGridProperty(L"Appearance");

  appearanceGroup->AddSubItem(new CMFCPropertyGridProperty(L"3D Look", (_variant_t) false,
      L"Specifies the window's font will be non-bold and controls will have a 3D border"));

  auto* lengthUnits =
      new CMFCPropertyGridProperty(L"Length Units", L"Engineering", L"Specifies the units used to display lengths");
  lengthUnits->AddOption(L"Architectural", TRUE);
  lengthUnits->AddOption(L"Engineering", TRUE);
  lengthUnits->AddOption(L"Feet", TRUE);
  lengthUnits->AddOption(L"Inches", TRUE);
  lengthUnits->AddOption(L"Meters", TRUE);
  lengthUnits->AddOption(L"Millimeters", TRUE);
  lengthUnits->AddOption(L"Centimeters", TRUE);
  lengthUnits->AddOption(L"Decimeters", TRUE);
  lengthUnits->AddOption(L"Kilometers", TRUE);
  lengthUnits->AllowEdit(FALSE);
  appearanceGroup->AddSubItem(lengthUnits);

  auto* lengthPrecision = new CMFCPropertyGridProperty(
      L"Length Precision", (_variant_t)8l, L"Specifies the precision used to display lengths");
  lengthPrecision->EnableSpinControl(TRUE, 0, 256);
  appearanceGroup->AddSubItem(lengthPrecision);

  appearanceGroup->AddSubItem(new CMFCPropertyGridProperty(
      L"Caption", (_variant_t)L"About", L"Specifies the text that will be displayed in the window's title bar"));

  m_PropertyGrid.AddProperty(appearanceGroup);

  auto* pointGrid = new CMFCPropertyGridProperty(L"Point Grid", 0, TRUE);

  auto* property = new CMFCPropertyGridProperty(L"X", (_variant_t)3.0, L"Specifies the point grid x spacing");
  pointGrid->AddSubItem(property);

  property = new CMFCPropertyGridProperty(L"Y", (_variant_t)3.0, L"Specifies the point grid y spacing");
  pointGrid->AddSubItem(property);

  property = new CMFCPropertyGridProperty(L"Z", (_variant_t)0.0, L"Specifies the point grid z spacing");
  pointGrid->AddSubItem(property);

  m_PropertyGrid.AddProperty(pointGrid);

  auto* noteGroup = new CMFCPropertyGridProperty(L"Note");

  LOGFONT lf;
  CFont* font = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
  font->GetLogFont(&lf);

  lstrcpy(lf.lfFaceName, L"Arial");

  noteGroup->AddSubItem(new CMFCPropertyGridFontProperty(
      L"Font", lf, CF_EFFECTS | CF_SCREENFONTS, L"Specifies the default font for the window"));
  noteGroup->AddSubItem(new CMFCPropertyGridProperty(
      L"Use System Font", (_variant_t) true, L"Specifies that the window uses MS Shell Dlg font"));

  auto* horizontalAlignment = new CMFCPropertyGridProperty(
      L"Horizontal Alignment", L"Left", L"Specifies the horizontal alignment used for new notes");
  horizontalAlignment->AddOption(L"Left", TRUE);
  horizontalAlignment->AddOption(L"Center", TRUE);
  horizontalAlignment->AddOption(L"Right", TRUE);
  horizontalAlignment->AllowEdit(FALSE);
  noteGroup->AddSubItem(horizontalAlignment);

  auto* verticalAlignment = new CMFCPropertyGridProperty(
      L"Vertical Alignment", L"Bottom", L"Specifies the vertical alignment used for new notes");
  verticalAlignment->AddOption(L"Bottom", TRUE);
  verticalAlignment->AddOption(L"Middle", TRUE);
  verticalAlignment->AddOption(L"Top", TRUE);
  verticalAlignment->AllowEdit(FALSE);
  noteGroup->AddSubItem(verticalAlignment);

  auto* path = new CMFCPropertyGridProperty(L"Path", L"Right", L"Specifies the text path used for new notes");
  path->AddOption(L"Right", TRUE);
  path->AddOption(L"Left", TRUE);
  path->AddOption(L"Up", TRUE);
  path->AddOption(L"Down", TRUE);
  path->AllowEdit(FALSE);
  noteGroup->AddSubItem(path);

  m_PropertyGrid.AddProperty(noteGroup);

  auto* miscGroup = new CMFCPropertyGridProperty(L"Misc");
  property = new CMFCPropertyGridProperty(L"(Name)", L"Application");
  property->Enable(FALSE);
  miscGroup->AddSubItem(property);

  auto* colorProperty = new CMFCPropertyGridColorProperty(
      L"Window Color", RGB(210, 192, 254), nullptr, L"Specifies the default window color");
  colorProperty->EnableOtherButton(L"Other...");
  colorProperty->EnableAutomaticButton(L"Default", ::GetSysColor(COLOR_3DFACE));
  miscGroup->AddSubItem(colorProperty);

  static wchar_t BASED_CODE szFilter[] = L"Icon Files(*.ico)|*.ico|All Files(*.*)|*.*||";
  miscGroup->AddSubItem(
      new CMFCPropertyGridFileProperty(L"Icon", TRUE, L"", L"ico", 0, szFilter, L"Specifies the window icon"));

  miscGroup->AddSubItem(new CMFCPropertyGridFileProperty(L"Shadow Folder Path", app.ShadowFolderPath(), 0, nullptr));

  m_PropertyGrid.AddProperty(miscGroup);
}

void EoMfPropertiesDockablePane::SetPropertyGridFont() {
  ::DeleteObject(m_PropertyGridFont.Detach());

  LOGFONT LogFont;
  afxGlobalData.fontRegular.GetLogFont(&LogFont);

  NONCLIENTMETRICS Info{};
  Info.cbSize = sizeof(Info);

  afxGlobalData.GetNonClientMetrics(Info);

  LogFont.lfHeight = Info.lfMenuFont.lfHeight;
  LogFont.lfWeight = Info.lfMenuFont.lfWeight;
  LogFont.lfItalic = Info.lfMenuFont.lfItalic;

  m_PropertyGridFont.CreateFontIndirect(&LogFont);

  m_PropertyGrid.SetFont(&m_PropertyGridFont);
}
