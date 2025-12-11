#include "stdafx.h"
#include "AeSys.h"

BOOL CALLBACK DlgProcSetupMouse_Buttons(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{
	switch (anMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemTextW(hDlg, IDC_LEFT_DOWN, app.szLeftMouseDown);						  
		SetDlgItemTextW(hDlg, IDC_LEFT_UP, app.szLeftMouseUp);
		SetDlgItemTextW(hDlg, IDC_RIGHT_DOWN, app.szRightMouseDown);
		SetDlgItemTextW(hDlg, IDC_RIGHT_UP, app.szRightMouseUp);
		return (TRUE);

	case WM_COMMAND:

		switch (LOWORD(wParam)) 
		{
		case IDOK:
			::GetDlgItemTextW(hDlg, IDC_LEFT_DOWN, app.szLeftMouseDown, sizeof(app.szLeftMouseDown) / sizeof(WCHAR));
			::GetDlgItemTextW(hDlg, IDC_LEFT_UP, app.szLeftMouseUp, sizeof(app.szLeftMouseUp) / sizeof(WCHAR));
			::GetDlgItemTextW(hDlg, IDC_RIGHT_DOWN, app.szRightMouseDown, sizeof(app.szRightMouseDown) / sizeof(WCHAR));
			::GetDlgItemTextW(hDlg, IDC_RIGHT_UP, app.szRightMouseUp, sizeof(app.szRightMouseUp) / sizeof(WCHAR));
			// fall thru to end dialog

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;
	}
	return (FALSE); 		
}

