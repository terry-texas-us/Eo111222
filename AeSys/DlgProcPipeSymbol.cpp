#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

void DlgProcPipeSymbolInit(HWND);

BOOL CALLBACK DlgProcPipeSymbol(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	switch (anMsg) 
	{
	case WM_INITDIALOG:
		DlgProcPipeSymbolInit(hDlg);
		return (TRUE);

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			ActiveView->PipeCurrentSymbol(::SendDlgItemMessageW(hDlg, IDC_LIST, LB_GETCURSEL, 0, 0L));

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;

		break;	
	}
	return (FALSE); 		
}
void DlgProcPipeSymbolInit(HWND hDlg)
{
	AeSysView* ActiveView = AeSysView::GetActiveView();

	WCHAR szNames[512];
	LPTSTR pName;

	::LoadStringW(app.GetInstance(), IDS_SUBPROC_PIPE_SYMBOL_NAMES, szNames, 256);

	LPTSTR NextToken = NULL;
	pName = wcstok_s(szNames, L"\t", &NextToken);
	while (pName != 0)
	{		
		::SendDlgItemMessageW(hDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM) pName);
		pName = wcstok_s(0, L"\t", &NextToken);
	}
	::SendDlgItemMessageW(hDlg, IDC_LIST, LB_SETCURSEL, ActiveView->PipeCurrentSymbol(), 0L);
}

