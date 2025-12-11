#include "stdafx.h"

extern TCHAR szLayerName[64];

BOOL CALLBACK DlgProcGetLayerName(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM  )
{
	switch (anMsg) 
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_NAME, szLayerName);
		return (TRUE);

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			::GetDlgItemText(hDlg, IDC_NAME, szLayerName, sizeof(szLayerName) / sizeof(TCHAR));
			::EndDialog(hDlg, TRUE);
			return (TRUE);

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;
	}
	return (FALSE); 											
}
