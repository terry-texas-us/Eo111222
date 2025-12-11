#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "Text.h"

BOOL CALLBACK DlgProcModeLetter(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();
						
	static EoGePoint3d ptPvt;
	
	HWND hWndTextCtrl = ::GetDlgItem(hDlg, IDC_TEXT);
	
	switch (message) 
	{
	case WM_INITDIALOG:
		ptPvt = app.GetCursorPosition();
		return (TRUE);
	
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			{
				CCharCellDef ccd;
				pstate.GetCharCellDef(ccd);
				CRefSys rs(ptPvt, ccd);

				CFontDef fd;
				pstate.GetFontDef(fd);

				int iLen = ::GetWindowTextLength(hWndTextCtrl);
				if (iLen != 0)
				{
					LPWSTR pszText = new WCHAR[iLen + 1];
					::GetWindowTextW(hWndTextCtrl, pszText, iLen + 1);
					::SetWindowTextW(hWndTextCtrl, L"");

					EoDbGroup* Group = new EoDbGroup(new EoDbText(fd, rs, pszText));
					Document->AddWorkLayerGroup(Group);
					Document->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);

					delete [] pszText;
				}
				ptPvt = text_GetNewLinePos(fd, rs, 1., 0);
				::SetFocus(hWndTextCtrl);
				return (TRUE);
			}
		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;

	case WM_SIZE:
		::SetWindowPos(hWndTextCtrl, 0, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOMOVE);
		break;
	}
	return (FALSE);
}
