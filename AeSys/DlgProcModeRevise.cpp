#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Text.h"

/// <remarks>
///Text related attributes for all notes generated will be same as those of the text last picked.
///Upon exit attributes restored to their entry values.
/// </remarks>
BOOL CALLBACK DlgProcModeRevise(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static CFontDef fd;
	static CRefSys rs;
	static EoDbText* pText = 0;
	
	AeSysView* ActiveView = AeSysView::GetActiveView();

	HWND hWndTextCtrl = ::GetDlgItem(hDlg, IDC_TEXT);

	switch (message)
	{
	case WM_INITDIALOG:
		pText = ActiveView->SelectTextUsingPoint(app.GetCursorPosition());
		if (pText != 0)
		{
			pText->GetFontDef(fd);
			pText->GetRefSys(rs);
			::SetWindowTextW(hWndTextCtrl, pText->Text());
		}
		else
			::EndDialog(hDlg, TRUE);

		return (TRUE);

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{	
		case IDOK:
			{
				int iLen = ::GetWindowTextLength(hWndTextCtrl);

				LPWSTR pszNew = new WCHAR[iLen + 1];
				::GetWindowTextW(hWndTextCtrl, pszNew, iLen + 1);

				AeSysDoc* Document = AeSysDoc::GetDoc();

				if (pText != 0)
				{
					Document->UpdateAllViews(NULL, EoDb::kPrimitiveEraseSafe, pText);
					pText->SetText(pszNew);
					Document->UpdateAllViews(NULL, EoDb::kPrimitiveSafe, pText);
				}
				else
				{
					EoDbGroup* Group = new EoDbGroup(new EoDbText(fd, rs, pszNew)); 
					Document->AddWorkLayerGroup(Group);
					Document->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
				}					
				delete [] pszNew;

				rs.SetOrigin(text_GetNewLinePos(fd, rs, 1., 0));

				pText = ActiveView->SelectTextUsingPoint(rs.Origin());
				if (pText != 0)
				{
					pText->GetFontDef(fd);
					pText->GetRefSys(rs);
					::SetWindowTextW(hWndTextCtrl, pText->Text());
				}
				else
					SetWindowTextW(hWndTextCtrl, L"");

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