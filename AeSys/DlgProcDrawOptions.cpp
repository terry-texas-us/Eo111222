#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

#include "EoDlgSetupConstraints.h"
#include "EoDlgSetupColor.h"
#include "EoDlgSetupNote.h"
#include "EoDlgSetupLineType.h"

BOOL CALLBACK DlgProcDrawOptions(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM  )
{
	switch (anMsg) 
	{
	case WM_INITDIALOG: 	// message: initialize dialog box
		::SendDlgItemMessageW(hDlg, IDC_ARC_3_POINT, BM_SETCHECK, 1, 0L);
		::SendDlgItemMessageW(hDlg, IDC_CURVE_SPLINE, BM_SETCHECK, 1, 0L);
		return (TRUE);

	case WM_COMMAND:		// message: received a command

		switch (LOWORD(wParam)) 
		{
		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);

		case IDC_PEN:
			::EndDialog(hDlg, TRUE);
			{
				EoDlgSetupColor Dialog;
				if (Dialog.DoModal() == IDOK)
				{
					app.AddStringToMessageList(L"TODO: Make color selection global");
					AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Pen);
				}
			}
			break;

		case IDC_LINE:
			::EndDialog(hDlg, TRUE);
			{
				EoDlgSetupLineType Dialog(AeSysDoc::GetDoc()->LineTypeTable());
				if (Dialog.DoModal() == IDOK)
				{
					pstate.SetLineType(NULL, Dialog.m_LineType->Index());		
					AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Line);
				}
			}
			break;

		case IDC_TEXT:
			{
				CCharCellDef CCD;
				EoDlgSetupNote Dialog;
				pstate.GetCharCellDef(CCD);

				Dialog.m_TextExpansionFactor = CCD.ChrExpFacGet();
				if (Dialog.DoModal() == IDOK)
				{
					CCD.ChrExpFacSet(Dialog.m_TextExpansionFactor);
				}
			}
			break;

		case IDC_FILL:
			::EndDialog(hDlg, TRUE);
			::DialogBox(app.GetInstance(), MAKEINTRESOURCE(IDD_SETUP_HATCH), app.GetSafeHwnd(), DlgProcSetupHatch);
			break;

		case IDC_CONSTRAINTS:
			::EndDialog(hDlg, TRUE);
			{
				EoDlgSetupConstraints Dialog(AeSysView::GetActiveView());
				if (Dialog.DoModal() == IDOK)
				{
					// TODO: Display the grid
				}
			}
		}
	}
	return (FALSE); 		
}
