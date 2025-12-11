#include "stdafx.h"
#include "AeSys.h"

void AeSys::InitializeConstraints()
{
	m_AxisConstraintInfluenceAngle = 5.;
	m_AxisConstraintOffsetAngle = 0.;

	m_DisplayGridWithLines = false;
	m_DisplayGridWithPoints = false;
	m_GridSnapIsOn = false;
	m_XGridLineSpacing = 12.;
	m_YGridLineSpacing = 12.;
	m_ZGridLineSpacing = 12.;
	m_XGridPointSpacing = 3.;
	m_YGridPointSpacing = 3.;
	m_ZGridPointSpacing = 0.;
	m_XGridSnapSpacing = 1.;
	m_YGridSnapSpacing = 1.;
	m_ZGridSnapSpacing = 1.;
	m_MaximumDotsPerLine = 64;
}
/// <summary>
///Set Axis constraint tolerance angle and offset axis constraint offset angle.
///Constrains a line to nearest axis pivoting on first endpoint.
/// </summary>
/// <remarks>Offset angle only support about z-axis</remarks>
/// <returns>Point after snap</returns>
EoGePoint3d AeSys::SnapPointToAxis(EoGePoint3d& beginPoint, EoGePoint3d& endPoint)
{
	CLine Line(beginPoint, endPoint);

	return (Line.ConstrainToAxis(m_AxisConstraintInfluenceAngle, m_AxisConstraintOffsetAngle));

}
/// <summary>Modifies the user grid and axis constraints.</summary>
BOOL CALLBACK DlgProcSetupConstraintsAxis(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{	
	switch (anMsg) 
	{
	case WM_INITDIALOG:
		app.SetDialogItemDouble(hDlg, IDC_USR_AX_INF_ANG, app.AxisConstraintInfluenceAngle());
		app.SetDialogItemDouble(hDlg, IDC_USR_AX_Z_OFF_ANG, app.AxisConstraintOffsetAngle());

		return (TRUE);

	case WM_COMMAND:

		switch (LOWORD(wParam)) 
		{
		case IDOK:
			app.AxisConstraintInfluenceAngle(app.GetDialogItemDouble(hDlg, IDC_USR_AX_INF_ANG));
			app.AxisConstraintOffsetAngle(app.GetDialogItemDouble(hDlg, IDC_USR_AX_Z_OFF_ANG));

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;
	}
	return (FALSE); 		
}

