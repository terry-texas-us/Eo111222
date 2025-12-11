#include "stdafx.h"
#include "AeSys.h"

#include "EoMfWorkspaceDockablePane.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int BorderSize = 1;

static TCHAR* TabsStyles[] = 
{
	_T("None"),
	_T("Standard"),
	_T("Grouped"),
	NULL
};
static TCHAR* TabLocations[] = 
{
	_T("On Bottom"),
	_T("On Top"),
	NULL
};

BEGIN_MESSAGE_MAP(EoMfWorkspaceDockablePane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
END_MESSAGE_MAP()

EoMfWorkspaceDockablePane::EoMfWorkspaceDockablePane()
{
}
EoMfWorkspaceDockablePane::~EoMfWorkspaceDockablePane()
{
}
int EoMfWorkspaceDockablePane::OnCreate(LPCREATESTRUCT createStruct) 
{
	if (CDockablePane::OnCreate(createStruct) == - 1)
	{
		return - 1;
	}	
	CRect EmptyRect;
	EmptyRect.SetRectEmpty();

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, EmptyRect, this, 1))
	{
		ATLTRACE(atlTraceGeneral, 0, _T("Failed to create Property Grid Control\n"));
		return - 1;
	}
	m_wndPropList.EnableHeaderCtrl(TRUE, _T("Property"), _T("Value"));
	m_wndPropList.EnableDescriptionArea(TRUE);
	m_wndPropList.SetVSDotNetLook(TRUE);

	CMFCPropertyGridProperty* TabsStyle = new CMFCPropertyGridProperty(_T("Tabs Style"), _T(""), _T("Set the Tabs Style to None, Standard, or Grouped"), kTabsStyle, NULL, NULL, NULL);
	TabsStyle->AddOption(::TabsStyles[0], TRUE);
	TabsStyle->AddOption(::TabsStyles[1], TRUE);
	TabsStyle->AddOption(::TabsStyles[2], TRUE);
	TabsStyle->SetValue(::TabsStyles[app.m_Options.m_nTabsStyle]);
	TabsStyle->AllowEdit(FALSE);
	m_wndPropList.AddProperty(TabsStyle);
	
	CMFCPropertyGridProperty* TabLocation = new CMFCPropertyGridProperty(_T("Tab Location"), _T(""), _T("Set the Tab Location to Top or Bottom"), kTabLocation, NULL, NULL, NULL);
	TabLocation->AddOption(::TabLocations[0], TRUE);
	TabLocation->AddOption(::TabLocations[1], TRUE);
	TabLocation->SetValue(::TabLocations[app.m_Options.m_MdiTabInfo.m_tabLocation]);
	TabLocation->AllowEdit(FALSE);
	m_wndPropList.AddProperty(TabLocation);
	
	COleVariant varTabsAutoColor((short)(app.m_Options.m_MdiTabInfo.m_bAutoColor == TRUE), VT_BOOL);
	m_wndPropList.AddProperty(new CMFCPropertyGridProperty(_T("Tabs auto-color"), varTabsAutoColor, _T("Set Workspace Tabs to use automatic color"), kTabsAutoColor));
	
	COleVariant varTabIcons((short)(app.m_Options.m_MdiTabInfo.m_bTabIcons == TRUE), VT_BOOL);
	m_wndPropList.AddProperty(new CMFCPropertyGridProperty(_T("Tab icons"), varTabIcons, _T("Show document icons on Workspace Tabs"), kTabIcons));
	
	CMFCPropertyGridProperty* pBorderProp = new CMFCPropertyGridProperty(_T("Border Size"), (COleVariant) ((short) app.m_Options.m_MdiTabInfo.m_nTabBorderSize), _T("Set Workspace border size from 0 to 10 pixels"), kTabsBorderSize);
	pBorderProp->EnableSpinControl(TRUE, 0, 10);
	pBorderProp->AllowEdit(FALSE);
	m_wndPropList.AddProperty(pBorderProp);

	SetPropState();
	return 0;
}
void EoMfWorkspaceDockablePane::OnSize(UINT type, int cx, int cy) 
{
	CDockablePane::OnSize(type, cx, cy);

	m_wndPropList.SetWindowPos(NULL, BorderSize, BorderSize, cx - 2 * BorderSize, cy - 2 * BorderSize, SWP_NOACTIVATE | SWP_NOZORDER);
}
void EoMfWorkspaceDockablePane::OnPaint() 
{
	CPaintDC DC(this);
	CRect BoundingRectangle;
	
	m_wndPropList.GetWindowRect(BoundingRectangle);
	ScreenToClient(BoundingRectangle);
	BoundingRectangle.InflateRect(BorderSize, BorderSize);
	DC.Draw3dRect(BoundingRectangle, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}
LRESULT EoMfWorkspaceDockablePane::OnPropertyChanged(WPARAM, LPARAM lparam)
{
	CMFCPropertyGridProperty* Property = (CMFCPropertyGridProperty*) lparam;

	BOOL ResetMDIChild = FALSE;

	switch (int(Property->GetData()))
	{
		case kTabsStyle:
		{
			CString TabStyle = (LPCTSTR) (_bstr_t) Property->GetValue();
			ResetMDIChild = TRUE;

			for (int i = 0; ::TabsStyles[i] != NULL; i++)
			{
				if (TabStyle == ::TabsStyles[i])
				{
					switch (i)
					{
						case 0:
							app.m_Options.m_nTabsStyle = EoApOptions::None;
							break;

						case 1:
							app.m_Options.m_nTabsStyle = EoApOptions::Standard;
							break;

						case 2:
							app.m_Options.m_nTabsStyle = EoApOptions::Grouped;
							break;
					}
					break;
				}
			}
			SetPropState();
			break;
		}
		case kTabLocation:
		{
			CString TabLocation = (LPCTSTR) (_bstr_t) Property->GetValue();
			app.m_Options.m_MdiTabInfo.m_tabLocation = (TabLocation == TabLocations[0] ? CMFCTabCtrl::LOCATION_BOTTOM : CMFCTabCtrl::LOCATION_TOP);
			break;
		}
		case kTabsAutoColor:
			app.m_Options.m_MdiTabInfo.m_bAutoColor = Property->GetValue().boolVal == VARIANT_TRUE;
			break;

		case kTabIcons:
			app.m_Options.m_MdiTabInfo.m_bTabIcons = Property->GetValue().boolVal == VARIANT_TRUE;
			break;

		case kTabsBorderSize:
		{
			int nBorder = Property->GetValue().iVal;
			app.m_Options.m_MdiTabInfo.m_nTabBorderSize = min(10, max (0, nBorder));
			break;
		}
	}
	app.UpdateMDITabs(ResetMDIChild);
	return 0;
}
void EoMfWorkspaceDockablePane::SetPropState()
{
	for (int i = 0; i < m_wndPropList.GetPropertyCount(); i++)
	{
		CMFCPropertyGridProperty* Property = m_wndPropList.GetProperty(i);
		ASSERT_VALID(Property);

		if (int(Property->GetData()) != kTabsStyle)
		{
			Property->Enable(app.m_Options.m_nTabsStyle != EoApOptions::None);
		}
	}
	if (m_wndPropList.GetSafeHwnd() != NULL)
	{
		m_wndPropList.RedrawWindow();
	}
}

