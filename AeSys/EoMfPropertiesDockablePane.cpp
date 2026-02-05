#include "Stdafx.h"

#include <algorithm>
#include <cassert>

#include "AeSys.h"
#include "AeSysView.h"
#include "EoApOptions.h"
#include "EoMfPropertiesDockablePane.h"
#include "Resource.h"

static const wchar_t* TabsStyles[] = {L"None", L"Standard", L"Grouped", nullptr};
static const wchar_t* TabLocations[] = {L"On Bottom", L"On Top", nullptr};

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
int EoMfPropertiesDockablePane::OnCreate(LPCREATESTRUCT createStruct) {
  if (CDockablePane::OnCreate(createStruct) == -1) { return -1; }
  CRect EmptyRect;
  EmptyRect.SetRectEmpty();

  if (!m_wndObjectCombo.Create(
          WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | CBS_SORT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
          EmptyRect, this, 1)) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create Properties Combo\n");
    return -1;
  }
  m_wndObjectCombo.AddString(L"Application");
  m_wndObjectCombo.AddString(L"Persistant");
  m_wndObjectCombo.SetFont(CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT)), TRUE);
  m_wndObjectCombo.SetCurSel(0);

  if (!m_PropertyGrid.Create(WS_VISIBLE | WS_CHILD, EmptyRect, this, 2)) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create Properties Grid \n");
    return -1;
  }
  InitializePropertyGrid();
  SetWorkspaceTabsSubItemsState();

  m_PropertiesToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
  m_PropertiesToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE, 0, 0, 0);
  m_PropertiesToolBar.CleanUpLockedImages();
  m_PropertiesToolBar.LoadBitmap(static_cast<UINT>(app.HighColorMode() ? IDB_PROPERTIES_HC : IDR_PROPERTIES), 0U, 0U,
                                 TRUE, 0U, 0U);

  m_PropertiesToolBar.SetPaneStyle(m_PropertiesToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
  m_PropertiesToolBar.SetPaneStyle(m_PropertiesToolBar.GetPaneStyle() &
                                   ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM |
                                     CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
  m_PropertiesToolBar.SetOwner(this);

  // All commands will be routed via this control , not via the parent frame:
  m_PropertiesToolBar.SetRouteCommandsViaFrame(FALSE);

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

  BOOL ResetMDIChild = FALSE;

  switch (int(Property->GetData())) {
    case kTabsStyle: {
      CString TabStyle = (LPCWSTR)(_bstr_t)Property->GetValue();
      ResetMDIChild = TRUE;

      for (int i = 0; ::TabsStyles[i] != nullptr; i++) {
        if (TabStyle == ::TabsStyles[i]) {
          switch (i) {
            case 0:
              app.m_Options.m_tabsStyle = EoApOptions::None;
              break;

            case 1:
              app.m_Options.m_tabsStyle = EoApOptions::Standard;
              break;

            case 2:
              app.m_Options.m_tabsStyle = EoApOptions::Grouped;
              break;
          }
          break;
        }
      }
      SetWorkspaceTabsSubItemsState();
      break;
    }
    case kTabLocation: {
      CString TabLocation = (LPCWSTR)(_bstr_t)Property->GetValue();
      app.m_Options.m_mdiTabInfo.m_tabLocation =
          (TabLocation == TabLocations[0] ? CMFCTabCtrl::LOCATION_BOTTOM : CMFCTabCtrl::LOCATION_TOP);
      break;
    }
    case kTabsAutoColor:
      app.m_Options.m_mdiTabInfo.m_bAutoColor = Property->GetValue().boolVal == VARIANT_TRUE;
      break;

    case kTabIcons:
      app.m_Options.m_mdiTabInfo.m_bTabIcons = Property->GetValue().boolVal == VARIANT_TRUE;
      break;

    case kTabBorderSize: {
      int nBorder = Property->GetValue().iVal;
      app.m_Options.m_mdiTabInfo.m_nTabBorderSize = std::min(8, std::max(0, nBorder));
      break;
    }
    case kActiveViewScale: {
      auto* activeView = AeSysView::GetActiveView();
      activeView->SetWorldScale(Property->GetValue().dblVal);
      activeView->UpdateStateInformation(AeSysView::Scale);
      return LRESULT(0);
    }
  }
  app.UpdateMDITabs(ResetMDIChild);

  return LRESULT(0);
}

void EoMfPropertiesDockablePane::OnExpandAllProperties() { m_PropertyGrid.ExpandAll(); }
void EoMfPropertiesDockablePane::OnProperties1() {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"EoMfPropertiesDockablePane::OnProperties1\n");
}
void EoMfPropertiesDockablePane::OnSortProperties() {
  m_PropertyGrid.SetAlphabeticMode(!m_PropertyGrid.IsAlphabeticMode());
}
void EoMfPropertiesDockablePane::OnUpdateExpandAllProperties(CCmdUI* pCmdUI) {
  (void)pCmdUI;
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 4, L"EoMfPropertiesDockablePane::OnUpdatExpandAllProperties\n");
}
void EoMfPropertiesDockablePane::OnUpdateProperties1(CCmdUI* pCmdUI) {
  (void)pCmdUI;
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 4, L"EoMfPropertiesDockablePane::OnUpdatProperties1\n");
}
void EoMfPropertiesDockablePane::OnUpdateSortProperties(CCmdUI* pCmdUI) {
  pCmdUI->SetCheck(m_PropertyGrid.IsAlphabeticMode());
}
void EoMfPropertiesDockablePane::AdjustLayout() {
  if (GetSafeHwnd() == nullptr) { return; }
  CRect rectClient, rectCombo;
  GetClientRect(rectClient);

  m_wndObjectCombo.GetWindowRect(&rectCombo);

  int cyCmb = rectCombo.Size().cy;
  int cyTlb = m_PropertiesToolBar.CalcFixedLayout(FALSE, TRUE).cy;

  m_wndObjectCombo.SetWindowPos(nullptr, rectClient.left, rectClient.top, rectClient.Width(), 200,
                                SWP_NOACTIVATE | SWP_NOZORDER);
  m_PropertiesToolBar.SetWindowPos(nullptr, rectClient.left, rectClient.top + cyCmb, rectClient.Width(), cyTlb,
                                   SWP_NOACTIVATE | SWP_NOZORDER);
  m_PropertyGrid.SetWindowPos(nullptr, rectClient.left, rectClient.top + cyCmb + cyTlb, rectClient.Width(),
                              rectClient.Height() - (cyCmb + cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
}

void EoMfPropertiesDockablePane::InitializePropertyGrid() {
  SetPropertyGridFont();

  m_PropertyGrid.EnableHeaderCtrl(FALSE, L"Property", L"Value");
  m_PropertyGrid.EnableDescriptionArea(TRUE);

  m_PropertyGrid.SetVSDotNetLook(TRUE);
  m_PropertyGrid.SetGroupNameFullWidth(TRUE, TRUE);

  m_PropertyGrid.MarkModifiedProperties(TRUE, TRUE);

  auto* workspaceTabsGroup = new CMFCPropertyGridProperty(L"Workspace Tabs");

  auto* tabsStyle = new CMFCPropertyGridProperty(
      L"Tabs Style", L"", L"Set the Tabs Style to None, Standard, or Grouped", kTabsStyle, nullptr, nullptr, nullptr);
  tabsStyle->AddOption(::TabsStyles[0], TRUE);
  tabsStyle->AddOption(::TabsStyles[1], TRUE);
  tabsStyle->AddOption(::TabsStyles[2], TRUE);
  tabsStyle->SetValue(::TabsStyles[app.m_Options.m_tabsStyle]);
  tabsStyle->AllowEdit(FALSE);
  workspaceTabsGroup->AddSubItem(tabsStyle);

  auto* tabLocation = new CMFCPropertyGridProperty(L"Tab Location", L"", L"Set the Tab Location to Top or Bottom",
                                                   kTabLocation, nullptr, nullptr, nullptr);
  tabLocation->AddOption(::TabLocations[0], TRUE);
  tabLocation->AddOption(::TabLocations[1], TRUE);
  tabLocation->SetValue(::TabLocations[app.m_Options.m_mdiTabInfo.m_tabLocation]);
  tabLocation->AllowEdit(FALSE);
  workspaceTabsGroup->AddSubItem(tabLocation);

  COleVariant tabsAutoColor((short)(app.m_Options.m_mdiTabInfo.m_bAutoColor == TRUE), VT_BOOL);
  workspaceTabsGroup->AddSubItem(new CMFCPropertyGridProperty(
      L"Tabs auto-color", tabsAutoColor, L"Set Workspace Tabs to use automatic color", kTabsAutoColor));

  COleVariant tabIcons((short)(app.m_Options.m_mdiTabInfo.m_bTabIcons == TRUE), VT_BOOL);
  workspaceTabsGroup->AddSubItem(
      new CMFCPropertyGridProperty(L"Tab icons", tabIcons, L"Show document icons on Workspace Tabs", kTabIcons));

  COleVariant tabBorderSize((short)(app.m_Options.m_mdiTabInfo.m_nTabBorderSize), VT_I2);
  CMFCPropertyGridProperty* borderSize = new CMFCPropertyGridProperty(
      L"Border Size", tabBorderSize, L"Set Workspace border size from 0 to 8 pixels", kTabBorderSize);
  borderSize->EnableSpinControl(TRUE, 0, 8);
  borderSize->AllowEdit(FALSE);
  workspaceTabsGroup->AddSubItem(borderSize);

  m_PropertyGrid.AddProperty(workspaceTabsGroup);

  auto* activeView = AeSysView::GetActiveView();
  double scale = (activeView == nullptr) ? 1.0 : activeView->GetWorldScale();

  auto* activeViewGroup = new CMFCPropertyGridProperty(L"Active View");
  auto* worldScaleProperty = new CMFCPropertyGridProperty(
      L"World Scale", (_variant_t)scale, L"Specifies the world scale used in the Active View", kActiveViewScale);
  activeViewGroup->AddSubItem(worldScaleProperty);
  activeViewGroup->AddSubItem(new CMFCPropertyGridProperty(L"Use True Type fonts", (_variant_t) true,
                                                           L"Specifies that the Active View uses True Type fonts"));
  m_PropertyGrid.AddProperty(activeViewGroup);
  worldScaleProperty->Enable(activeView != nullptr);

  auto* appearanceGroup = new CMFCPropertyGridProperty(L"Appearance");

  appearanceGroup->AddSubItem(
      new CMFCPropertyGridProperty(L"3D Look", (_variant_t) false,
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

  auto* lengthPrecision = new CMFCPropertyGridProperty(L"Length Precision", (_variant_t)8l,
                                                       L"Specifies the precision used to display lengths");
  lengthPrecision->EnableSpinControl(TRUE, 0, 256);
  appearanceGroup->AddSubItem(lengthPrecision);

  appearanceGroup->AddSubItem(new CMFCPropertyGridProperty(
      L"Caption", (_variant_t)L"About", L"Specifies the text that will be displayed in the window's title bar"));

  m_PropertyGrid.AddProperty(appearanceGroup);

  auto* pointGrid = new CMFCPropertyGridProperty(L"Point Grid", 0, TRUE);

  auto* property =
      new CMFCPropertyGridProperty(L"X", (_variant_t)3.0, L"Specifies the point grid x spacing");
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

  noteGroup->AddSubItem(new CMFCPropertyGridFontProperty(L"Font", lf, CF_EFFECTS | CF_SCREENFONTS,
                                                         L"Specifies the default font for the window"));
  noteGroup->AddSubItem(new CMFCPropertyGridProperty(L"Use System Font", (_variant_t) true,
                                                     L"Specifies that the window uses MS Shell Dlg font"));

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

void EoMfPropertiesDockablePane::SetWorkspaceTabsSubItemsState() {
  for (int i = 0; i < m_PropertyGrid.GetPropertyCount(); i++) {
    auto* property = m_PropertyGrid.GetProperty(i);
    assert(property != nullptr);
    if (wcscmp(property->GetName(), L"Workspace Tabs") == 0) {
      for (int subItemIndex = 1; subItemIndex < property->GetSubItemsCount(); subItemIndex++) {
        auto* subProperty = property->GetSubItem(subItemIndex);
        assert(subProperty != nullptr);
        subProperty->Enable(app.m_Options.m_tabsStyle != EoApOptions::None);
      }
    }
  }
  if (m_PropertyGrid.GetSafeHwnd() != nullptr) { m_PropertyGrid.RedrawWindow(); }
}
