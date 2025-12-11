#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

BOOL CALLBACK DlgProcFixupOptions(HWND hDlg, UINT message, WPARAM wParam, LPARAM  )
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	
	switch (message) 
	{
	case WM_INITDIALOG:
		app.SetDialogItemDouble(hDlg, IDC_FIX_SIZ, ActiveView->m_FixupModeCornerSize);
		app.SetDialogItemDouble(hDlg, IDC_FIX_AX_TOL, ActiveView->m_FixupModeAxisTolerance);
		return (TRUE);

	case WM_COMMAND:

		switch (LOWORD(wParam)) 
		{
		case IDOK:
			ActiveView->m_FixupModeCornerSize = EoMax(0., app.GetDialogItemDouble(hDlg, IDC_FIX_SIZ));
			
			ActiveView->m_FixupModeAxisTolerance = EoMax(0., app.GetDialogItemDouble(hDlg, IDC_FIX_AX_TOL));
			ActiveView->SetAxisConstraintInfluenceAngle(ActiveView->m_FixupModeAxisTolerance);

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;
	}
	return (FALSE); 		
}
