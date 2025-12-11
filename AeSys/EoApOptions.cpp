#include "stdafx.h"
#include "AeSys.h"

EoApOptions::EoApOptions() {
	m_nTabsStyle = EoApOptions::Grouped;

	m_MdiTabInfo.m_tabLocation = CMFCTabCtrl::LOCATION_BOTTOM;
	m_MdiTabInfo.m_style = CMFCTabCtrl::STYLE_3D_VS2005;
	m_MdiTabInfo.m_bTabIcons = TRUE;
	m_MdiTabInfo.m_bTabCloseButton = FALSE;
	m_MdiTabInfo.m_bTabCustomTooltips = TRUE;
	m_MdiTabInfo.m_bAutoColor = FALSE;
	m_MdiTabInfo.m_bDocumentMenu = TRUE;
	m_MdiTabInfo.m_bEnableTabSwap = TRUE;
	m_MdiTabInfo.m_bFlatFrame = TRUE;
	m_MdiTabInfo.m_bActiveTabCloseButton = TRUE;
	m_MdiTabInfo.m_nTabBorderSize = 1;

	m_bTabsContextMenu = TRUE;
	m_bDisableSetRedraw = TRUE;
}
EoApOptions::~EoApOptions() {
}
void EoApOptions::Load() {
	m_nTabsStyle = (TabsStyle) app.GetInt(L"TabsStyle", EoApOptions::Grouped);

	m_MdiTabInfo.m_tabLocation = (CMFCTabCtrl::Location) app.GetInt(L"TabLocation", CMFCTabCtrl::LOCATION_BOTTOM);
	m_MdiTabInfo.m_style = (CMFCTabCtrl::Style) app.GetInt(L"TabsAppearance", CMFCTabCtrl::STYLE_3D_VS2005);
	m_MdiTabInfo.m_bTabIcons = app.GetInt(L"TabIcons", TRUE);
	m_MdiTabInfo.m_bTabCloseButton = app.GetInt(L"TabCloseButton", FALSE);
	m_MdiTabInfo.m_bTabCustomTooltips = app.GetInt(L"CustomTooltips", TRUE);
	m_MdiTabInfo.m_bAutoColor = app.GetInt(L"AutoColor", FALSE);
	m_MdiTabInfo.m_bDocumentMenu = app.GetInt(L"DocumentMenu", TRUE);
	m_MdiTabInfo.m_bEnableTabSwap = app.GetInt(L"EnableTabSwap", TRUE);
	m_MdiTabInfo.m_bFlatFrame = app.GetInt(L"FlatFrame", TRUE);
	m_MdiTabInfo.m_bActiveTabCloseButton = app.GetInt(L"ActiveTabCloseButton", TRUE);
	m_MdiTabInfo.m_nTabBorderSize = app.GetInt(L"TabBorderSize", 1);

	m_bTabsContextMenu = app.GetInt(L"TabsContextMenu", TRUE);
	m_bDisableSetRedraw = app.GetInt(L"DisableSetRedraw", TRUE);
}
void EoApOptions::Save() {
	app.WriteInt(L"TabsStyle", m_nTabsStyle);

	app.WriteInt(L"TabLocation", m_MdiTabInfo.m_tabLocation);
	app.WriteInt(L"TabsAppearance", m_MdiTabInfo.m_style);
	app.WriteInt(L"TabIcons", m_MdiTabInfo.m_bTabIcons);
	app.WriteInt(L"TabCloseButton", m_MdiTabInfo.m_bTabCloseButton);
	app.WriteInt(L"CustomTooltips", m_MdiTabInfo.m_bTabCustomTooltips);
	app.WriteInt(L"AutoColor", m_MdiTabInfo.m_bAutoColor);
	app.WriteInt(L"DocumentMenu", m_MdiTabInfo.m_bDocumentMenu);
	app.WriteInt(L"EnableTabSwap", m_MdiTabInfo.m_bEnableTabSwap);
	app.WriteInt(L"FlatFrame", m_MdiTabInfo.m_bFlatFrame);
	app.WriteInt(L"ActiveTabCloseButton", m_MdiTabInfo.m_bActiveTabCloseButton);
	app.WriteInt(L"TabBorderSize", m_MdiTabInfo.m_nTabBorderSize);

	app.WriteInt(L"TabsContextMenu", m_bTabsContextMenu);
	app.WriteInt(L"DisableSetRedraw", m_bDisableSetRedraw);
}
