#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

void AeSys::GetGridLineSpacing(double& x, double& y, double& z) 
{
	x = m_XGridLineSpacing; 
	y = m_YGridLineSpacing; 
	z = m_ZGridLineSpacing;
}
void AeSys::SetGridLineSpacing(double x, double y, double z) 
{
	m_XGridLineSpacing = x; 
	m_YGridLineSpacing = y; 
	m_ZGridLineSpacing = z;
}
void AeSys::GetGridPointSpacing(double& x, double& y, double& z) 
{
	x = m_XGridPointSpacing; 
	y = m_YGridPointSpacing; 
	z = m_ZGridPointSpacing;
}
void AeSys::SetGridPointSpacing(double x, double y, double z) 
{
	m_XGridPointSpacing = x; 
	m_YGridPointSpacing = y; 
	m_ZGridPointSpacing = z;
}
void AeSys::GetGridSnapSpacing(double& x, double& y, double& z) 
{
	x = m_XGridSnapSpacing; 
	y = m_YGridSnapSpacing; 
	z = m_ZGridSnapSpacing;
}
void AeSys::SetGridSnapSpacing(double x, double y, double z) 
{
	m_XGridSnapSpacing = x; 
	m_YGridSnapSpacing = y; 
	m_ZGridSnapSpacing = z;
}
/// <summary>
///Generates a point display centered about the user origin in one or more of the three orthogonal planes 
///for the current user grid.
/// </summary>
void AeSys::DisplayGrid(AeSysView* view, CDC* deviceContext)
{	
	double dHalfPts = m_MaximumDotsPerLine * .5;

	if (app.DisplayGridWithPoints())
	{	
		EoGePoint3d	pt;

		if (fabs(m_YGridPointSpacing) > DBL_EPSILON && fabs(m_ZGridPointSpacing) > DBL_EPSILON) 
		{
			COLORREF Color = app.PenColorsGetHot(1);

			pt.x = m_GridOrigin.x;
			pt.z = m_GridOrigin.z - dHalfPts * m_ZGridPointSpacing;
			for (int i = 0; i < m_MaximumDotsPerLine; i++)
			{
				pt.y = m_GridOrigin.y - dHalfPts * m_YGridPointSpacing;
				for (int i2 = 0; i2 < m_MaximumDotsPerLine; i2++)
				{
					view->DisplayPixel(deviceContext, Color, pt);
					pt.y += m_YGridPointSpacing;
				}
				pt.z += m_ZGridPointSpacing;
			}
		}
		if (fabs(m_XGridPointSpacing) > DBL_EPSILON && fabs(m_ZGridPointSpacing) > DBL_EPSILON) 
		{
			COLORREF Color = app.PenColorsGetHot(2);

			pt.x = m_GridOrigin.x - dHalfPts * m_XGridPointSpacing;
			pt.y = m_GridOrigin.y;
			for (int i = 0; i < m_MaximumDotsPerLine; i++)
			{
				pt.z = m_GridOrigin.z - dHalfPts * m_ZGridPointSpacing;
				for (int i2 = 0; i2 < m_MaximumDotsPerLine; i2++)
				{
					view->DisplayPixel(deviceContext, Color, pt);
					pt.z += m_ZGridPointSpacing;
				}
				pt.x += m_XGridPointSpacing;
			}
		}
		if (fabs(m_XGridPointSpacing) > DBL_EPSILON && fabs(m_YGridPointSpacing) > DBL_EPSILON) 
		{
			COLORREF Color = app.PenColorsGetHot(3);

			pt.y = m_GridOrigin.y - dHalfPts * m_YGridPointSpacing;
			pt.z = m_GridOrigin.z;
			for (int i = 0; i < m_MaximumDotsPerLine; i++)
			{
				pt.x = m_GridOrigin.x - dHalfPts * m_XGridPointSpacing;
				for (int i2 = 0; i2 < m_MaximumDotsPerLine; i2++)
				{
					view->DisplayPixel(deviceContext, Color, pt);
					pt.x += m_XGridPointSpacing;
				}
				pt.y += m_YGridPointSpacing;
			}
		}
	}
	if (app.DisplayGridWithLines())
	{
		if (fabs(m_XGridLineSpacing) > DBL_EPSILON && fabs(m_YGridLineSpacing) > DBL_EPSILON) 
		{
			CLine ln;

			int i;
			EoInt16 nPenColor = pstate.PenColor();
			EoInt16 LineType = pstate.LineType();
			pstate.SetPen(view, deviceContext, 250, 1);

			ln.begin.x = m_GridOrigin.x - dHalfPts * m_XGridLineSpacing;
			ln.end.x = m_GridOrigin.x + dHalfPts * m_XGridLineSpacing;
			ln.begin.y = m_GridOrigin.y - dHalfPts * m_YGridLineSpacing;
			ln.begin.z = m_GridOrigin.z;
			ln.end.z = m_GridOrigin.z;
			for (i = 0; i < m_MaximumDotsPerLine; i++)
			{
				ln.end.y = ln.begin.y;
				ln.Display(view, deviceContext);
				ln.begin.y += m_YGridLineSpacing;
			}
			ln.begin.y = m_GridOrigin.y - dHalfPts * m_YGridLineSpacing;
			ln.end.y = m_GridOrigin.y + dHalfPts * m_YGridLineSpacing;
			for (i = 0; i < m_MaximumDotsPerLine; i++)
			{
				ln.end.x = ln.begin.x;
				ln.Display(view, deviceContext);
				ln.begin.x += m_XGridLineSpacing;
			}
			pstate.SetPen(view, deviceContext, nPenColor, LineType);
		}
	}
}
/// <summary>Determines the nearest point on system constraining grid.</summary>
EoGePoint3d AeSys::SnapPointToGrid(EoGePoint3d& arPt)
{
	EoGePoint3d	pt = arPt;

	if (app.IsGridSnapEnabled())
	{
		if (fabs(m_XGridSnapSpacing) > DBL_EPSILON)
		{
			pt.x -= fmod((arPt.x - m_GridOrigin.x), m_XGridSnapSpacing);
			if (fabs(pt.x - arPt.x) > m_XGridSnapSpacing * .5)
				pt.x += fsign(m_XGridSnapSpacing, arPt.x - m_GridOrigin.x);
		}
		if (fabs(m_YGridSnapSpacing) > DBL_EPSILON)
		{
			pt.y -= fmod((arPt.y - m_GridOrigin.y), m_YGridSnapSpacing);
			if (fabs(pt.y - arPt.y) > m_YGridSnapSpacing * .5)
				pt.y += fsign(m_YGridSnapSpacing, arPt.y - m_GridOrigin.y);
		}
		if (fabs(m_ZGridSnapSpacing) > DBL_EPSILON)
		{
			pt.z -= fmod((arPt.z - m_GridOrigin.z), m_ZGridSnapSpacing);
			if (fabs(pt.z - arPt.z) > m_ZGridSnapSpacing * .5)
				pt.z += fsign(m_ZGridSnapSpacing, arPt.z - m_GridOrigin.z);
		}
	}
	return (pt);
}
// Modifies the user grid and axis constraints.
BOOL CALLBACK DlgProcSetupConstraints(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{
	switch (anMsg) 
	{
	case WM_INITDIALOG: {
		double x, y, z;

		app.GetGridSnapSpacing(x, y, z);
		app.SetDialogItemUnitsText(hDlg, IDC_USR_GRID_X_INT, x);
		app.SetDialogItemUnitsText(hDlg, IDC_USR_GRID_Y_INT, y);
		app.SetDialogItemUnitsText(hDlg, IDC_USR_GRID_Z_INT, z);

		app.GetGridPointSpacing(x, y, z);
		app.SetDialogItemUnitsText(hDlg, IDC_GRID_DOT_INT_X, x);
		app.SetDialogItemUnitsText(hDlg, IDC_GRID_DOT_INT_Y, y);
		app.SetDialogItemUnitsText(hDlg, IDC_GRID_DOT_INT_Z, z);

		app.GetGridLineSpacing(x, y, z);
		app.SetDialogItemUnitsText(hDlg, IDC_GRID_LN_INT_X, x);
		app.SetDialogItemUnitsText(hDlg, IDC_GRID_LN_INT_Y, y);
		app.SetDialogItemUnitsText(hDlg, IDC_GRID_LN_INT_Z, z);

		::CheckDlgButton(hDlg, IDC_GRID_SNAP_ON, app.IsGridSnapEnabled());
		::CheckDlgButton(hDlg, IDC_GRID_PTS_DIS_ON, app.DisplayGridWithPoints());
		::CheckDlgButton(hDlg, IDC_GRID_LNS_DIS_ON, app.DisplayGridWithLines());
		return (TRUE);
						}
	case WM_COMMAND:

		switch (LOWORD(wParam)) 
		{
		case IDOK: {
			double x, y, z;

			x = app.GetDialogItemUnitsText(hDlg, IDC_USR_GRID_X_INT);
			y = app.GetDialogItemUnitsText(hDlg, IDC_USR_GRID_Y_INT);
			z = app.GetDialogItemUnitsText(hDlg, IDC_USR_GRID_Z_INT);
			app.SetGridSnapSpacing(x, y, z);

			x = app.GetDialogItemUnitsText(hDlg, IDC_GRID_DOT_INT_X);
			y = app.GetDialogItemUnitsText(hDlg, IDC_GRID_DOT_INT_Y);
			z = app.GetDialogItemUnitsText(hDlg, IDC_GRID_DOT_INT_Z);
			app.SetGridPointSpacing(x, y, z);

			x = app.GetDialogItemUnitsText(hDlg, IDC_GRID_LN_INT_X);
			y = app.GetDialogItemUnitsText(hDlg, IDC_GRID_LN_INT_Y);
			z = app.GetDialogItemUnitsText(hDlg, IDC_GRID_LN_INT_Z);
			app.SetGridLineSpacing(x, y, z);

			app.GridSnapIsOn(IsDlgButtonChecked(hDlg, IDC_GRID_SNAP_ON) == BST_CHECKED);
			app.DisplayGridWithPoints(IsDlgButtonChecked(hDlg, IDC_GRID_PTS_DIS_ON) == BST_CHECKED);
			app.DisplayGridWithLines(IsDlgButtonChecked(hDlg, IDC_GRID_LNS_DIS_ON) == BST_CHECKED);

			// fall thru to end dialog
				   }				
		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;
	}
	return (FALSE); 		
}


