#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

BOOL CALLBACK DlgProcPipeOptions(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	if (message == WM_INITDIALOG)
	{
		app.SetDialogItemDouble(hDlg, IDC_TIC_SIZE, ActiveView->PipeTicSize());
		app.SetDialogItemDouble(hDlg, IDC_RISEDROP_RADIUS, ActiveView->PipeRiseDropRadius());
		return (TRUE);
	}
	if (message != WM_COMMAND)
		return (FALSE);

	switch (LOWORD(wParam))
	{
	case IDOK:
		ActiveView->PipeTicSize(app.GetDialogItemDouble(hDlg, IDC_TIC_SIZE));
		ActiveView->PipeRiseDropRadius(app.GetDialogItemDouble(hDlg, IDC_RISEDROP_RADIUS));
	case IDCANCEL:
		::EndDialog(hDlg, TRUE);
		return (TRUE);
	}
	return (FALSE);
}

