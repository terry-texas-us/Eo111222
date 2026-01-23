#include "Stdafx.h"

#include <afx.h>
#include <afxbasetabctrl.h>
#include <afxtabctrl.h>

#include "AeSys.h"
#include "EoApOptions.h"

EoApOptions::EoApOptions() {
  m_tabsStyle = EoApOptions::Grouped;

  m_mdiTabInfo.m_tabLocation = CMFCTabCtrl::LOCATION_BOTTOM;
  m_mdiTabInfo.m_style = CMFCTabCtrl::STYLE_3D_VS2005;
  m_mdiTabInfo.m_bTabIcons = TRUE;
  m_mdiTabInfo.m_bTabCloseButton = FALSE;
  m_mdiTabInfo.m_bTabCustomTooltips = TRUE;
  m_mdiTabInfo.m_bAutoColor = FALSE;
  m_mdiTabInfo.m_bDocumentMenu = TRUE;
  m_mdiTabInfo.m_bEnableTabSwap = TRUE;
  m_mdiTabInfo.m_bFlatFrame = TRUE;
  m_mdiTabInfo.m_bActiveTabCloseButton = TRUE;
  m_mdiTabInfo.m_nTabBorderSize = 1;

  m_tabsContextMenu = TRUE;
  m_disableSetRedraw = TRUE;
}
EoApOptions::~EoApOptions() {}
void EoApOptions::Load() {
  m_tabsStyle = (TabsStyle)app.GetInt(L"TabsStyle", EoApOptions::Grouped);

  m_mdiTabInfo.m_tabLocation = (CMFCTabCtrl::Location)app.GetInt(L"TabLocation", CMFCTabCtrl::LOCATION_BOTTOM);
  m_mdiTabInfo.m_style = (CMFCTabCtrl::Style)app.GetInt(L"TabsAppearance", CMFCTabCtrl::STYLE_3D_VS2005);
  m_mdiTabInfo.m_bTabIcons = app.GetInt(L"TabIcons", TRUE);
  m_mdiTabInfo.m_bTabCloseButton = app.GetInt(L"TabCloseButton", FALSE);
  m_mdiTabInfo.m_bTabCustomTooltips = app.GetInt(L"CustomTooltips", TRUE);
  m_mdiTabInfo.m_bAutoColor = app.GetInt(L"AutoColor", FALSE);
  m_mdiTabInfo.m_bDocumentMenu = app.GetInt(L"DocumentMenu", TRUE);
  m_mdiTabInfo.m_bEnableTabSwap = app.GetInt(L"EnableTabSwap", TRUE);
  m_mdiTabInfo.m_bFlatFrame = app.GetInt(L"FlatFrame", TRUE);
  m_mdiTabInfo.m_bActiveTabCloseButton = app.GetInt(L"ActiveTabCloseButton", TRUE);
  m_mdiTabInfo.m_nTabBorderSize = app.GetInt(L"TabBorderSize", 1);

  m_tabsContextMenu = app.GetInt(L"TabsContextMenu", TRUE);
  m_disableSetRedraw = app.GetInt(L"DisableSetRedraw", TRUE);
}
void EoApOptions::Save() const {
  app.WriteInt(L"TabsStyle", m_tabsStyle);

  app.WriteInt(L"TabLocation", m_mdiTabInfo.m_tabLocation);
  app.WriteInt(L"TabsAppearance", m_mdiTabInfo.m_style);
  app.WriteInt(L"TabIcons", m_mdiTabInfo.m_bTabIcons);
  app.WriteInt(L"TabCloseButton", m_mdiTabInfo.m_bTabCloseButton);
  app.WriteInt(L"CustomTooltips", m_mdiTabInfo.m_bTabCustomTooltips);
  app.WriteInt(L"AutoColor", m_mdiTabInfo.m_bAutoColor);
  app.WriteInt(L"DocumentMenu", m_mdiTabInfo.m_bDocumentMenu);
  app.WriteInt(L"EnableTabSwap", m_mdiTabInfo.m_bEnableTabSwap);
  app.WriteInt(L"FlatFrame", m_mdiTabInfo.m_bFlatFrame);
  app.WriteInt(L"ActiveTabCloseButton", m_mdiTabInfo.m_bActiveTabCloseButton);
  app.WriteInt(L"TabBorderSize", m_mdiTabInfo.m_nTabBorderSize);

  app.WriteInt(L"TabsContextMenu", m_tabsContextMenu);
  app.WriteInt(L"DisableSetRedraw", m_disableSetRedraw);
}
