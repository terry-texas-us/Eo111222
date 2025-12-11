#include "stdafx.h"
#include "AeSys.h"

#include "Hatch.h"

BOOL CALLBACK DlgProcSetupHatch(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{
	EoInt16	nStyleId;

	switch (anMsg) 
	{
	case WM_INITDIALOG: 	
		SetDlgItemInt(hDlg, IDC_FIL_AREA_HAT_ID, pstate.PolygonIntStyleId(), FALSE);
		app.SetDialogItemDouble(hDlg, IDC_FIL_AREA_HAT_X_SCAL, hatch::dXAxRefVecScal);
		app.SetDialogItemDouble(hDlg, IDC_FIL_AREA_HAT_Y_SCAL, hatch::dYAxRefVecScal);
		app.SetDialogItemDouble(hDlg, IDC_FIL_AREA_HAT_ROT_ANG, EoToDegree(hatch::dOffAng));
		return (TRUE);

	case WM_COMMAND:

		switch (LOWORD(wParam)) 
		{
		case IDOK:
			pstate.SetPolygonIntStyle(EoDb::Hatch);
			nStyleId = EoInt16(GetDlgItemInt(hDlg, IDC_FIL_AREA_HAT_ID, 0, FALSE));
			pstate.SetPolygonIntStyleId(nStyleId);
			hatch::dXAxRefVecScal = EoMax(.01, app.GetDialogItemDouble(hDlg, IDC_FIL_AREA_HAT_X_SCAL));
			hatch::dYAxRefVecScal = EoMax(.01, app.GetDialogItemDouble(hDlg, IDC_FIL_AREA_HAT_Y_SCAL));
			hatch::dOffAng = EoArcLength(app.GetDialogItemDouble(hDlg, IDC_FIL_AREA_HAT_ROT_ANG));
			// fall thru to end dialog

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;
	}
	return (FALSE); 		
}

