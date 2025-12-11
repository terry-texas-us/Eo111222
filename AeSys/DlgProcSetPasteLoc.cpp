#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"

BOOL CALLBACK DlgProcSetPasteLoc(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM  )
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	EoGePoint3d pt;

	switch (anMsg) 
	{
	case WM_INITDIALOG:
		return (TRUE);

	case WM_COMMAND:

		switch (LOWORD(wParam)) 
		{
		case IDOK:
			pt = app.GetCursorPosition();
			Document->SetTrapPivotPoint(pt);
			::EndDialog(hDlg, TRUE);
			return (TRUE);

		case IDCANCEL:
			::EndDialog(hDlg, FALSE);
			return (TRUE);
		}
		break;
	}
	return (FALSE); 		
}
