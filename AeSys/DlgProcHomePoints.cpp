#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

BOOL CALLBACK DlgProcHomePointGo(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();
	AeSysView* ActiveView = AeSysView::GetActiveView();

	EoUInt16 wNotifyCode = HIWORD(wParam);
	static EoGePoint3d pt;
	
	if (anMsg == WM_INITDIALOG)
	{		 
		WCHAR szNames[256];
		::LoadStringW(app.GetInstance(), IDS_HOME_POINT_GO_NAMES, szNames, 256);
	
		LPTSTR NextToken = NULL;
		LPTSTR pName = wcstok_s(szNames, L"\t", &NextToken);
		while (pName != 0)
		{		
			::SendDlgItemMessageW(hDlg, IDC_LIST, CB_ADDSTRING, 0, (LPARAM) pName);
			pName = wcstok_s(0, L"\t", &NextToken);
		}
		::SendDlgItemMessageW(hDlg, IDC_LIST, CB_SETCURSEL, 9, 0L);
			
		pt = ActiveView->GridOrign();
							
		app.SetDialogItemUnitsText(hDlg, IDC_X, pt.x);
		app.SetDialogItemUnitsText(hDlg, IDC_Y, pt.y);
		app.SetDialogItemUnitsText(hDlg, IDC_Z, pt.z);
		return (TRUE);
	}
	else if (anMsg == WM_COMMAND)
	{
		WCHAR	szBuf[32];
		
		switch (LOWORD(wParam)) 
		{
		case IDC_LIST:
			if (wNotifyCode == CBN_EDITUPDATE) 
			{
				::GetDlgItemTextW(hDlg, IDC_LIST, (LPTSTR) szBuf, 32);
				int iId = int(::SendDlgItemMessageW(hDlg, IDC_LIST, CB_FINDSTRING, (WPARAM) - 1, (LPARAM) szBuf));
				if (iId != CB_ERR)
				{ 
					switch (iId)
					{
					case 9:
						pt = ActiveView->GridOrign();
						break;
					case 10:
						pt = Document->GetTrapPivotPoint();
						break;
					case 11:
						pt = ActiveView->ModelViewGetTarget();
						break;
					case 12:
						pt = EoGePoint3d::kOrigin;
						break;
					default:
						pt = app.HomePointGet(iId);
					}
					ActiveView->SetCursorPosition(pt);
					::EndDialog(hDlg, TRUE);
					return (TRUE);
				}
			}
			else if (wNotifyCode == CBN_SELCHANGE)
			{
				int iId = int(::SendDlgItemMessageW(hDlg, IDC_LIST, CB_GETCURSEL, 0, 0L));
				if (iId != CB_ERR)
				{
					switch (iId)
					{
					case 9:
						pt = ActiveView->GridOrign();
						break;
					case 10:
						pt = Document->GetTrapPivotPoint();
						break;
					case 11:
						pt = ActiveView->ModelViewGetTarget();
						break;
					case 12:
						pt = EoGePoint3d::kOrigin;
						break;
					default:
						pt = app.HomePointGet(iId);
					}	
					app.SetDialogItemUnitsText(hDlg, IDC_X, pt.x);
					app.SetDialogItemUnitsText(hDlg, IDC_Y, pt.y);
					app.SetDialogItemUnitsText(hDlg, IDC_Z, pt.z);
				}		 
			}
			break;

		case IDOK:
			pt.x = app.GetDialogItemUnitsText(hDlg, IDC_X);
			pt.y = app.GetDialogItemUnitsText(hDlg, IDC_Y);
			pt.z = app.GetDialogItemUnitsText(hDlg, IDC_Z);

			ActiveView->SetCursorPosition(pt);
			::EndDialog(hDlg, TRUE);
			return (TRUE);

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
	}
	return (FALSE); 		
}

BOOL CALLBACK DlgProcHomePointSet(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{
	EoUInt16 wNotifyCode = HIWORD(wParam);
	static EoGePoint3d pt;
	
	int 	iId;
	
	if (anMsg == WM_INITDIALOG)
	{		 
		WCHAR szNames[256];

		::LoadStringW(app.GetInstance(), IDS_HOME_POINT_SET_NAMES, szNames, 256);

		LPTSTR NextToken = NULL;
		LPTSTR pName = wcstok_s(szNames, L"\t", &NextToken);
		while (pName != 0)
		{		
			::SendDlgItemMessageW(hDlg, IDC_LIST, CB_ADDSTRING, 0, (LPARAM) pName);
			pName = wcstok_s(0, L"\t", &NextToken);
		}
		::SendDlgItemMessageW(hDlg, IDC_LIST, CB_SETCURSEL, 9, 0L);
		
		pt = app.GetCursorPosition();
							
		app.SetDialogItemUnitsText(hDlg, IDC_X, pt.x);
		app.SetDialogItemUnitsText(hDlg, IDC_Y, pt.y);
		app.SetDialogItemUnitsText(hDlg, IDC_Z, pt.z);
		return (TRUE);
	}
	else if (anMsg == WM_COMMAND)
	{
		WCHAR	szBuf[32];

		AeSysDoc* Document = AeSysDoc::GetDoc();
		AeSysView* ActiveView = AeSysView::GetActiveView();

		switch (LOWORD(wParam)) 
		{
		case IDC_LIST:
			if (wNotifyCode == CBN_EDITUPDATE) 
			{
				::GetDlgItemTextW(hDlg, IDC_LIST, (LPTSTR) szBuf, 32);
				iId = int(::SendDlgItemMessageW(hDlg, IDC_LIST, CB_FINDSTRING, (WPARAM) - 1, (LPARAM) szBuf));
				if (iId != CB_ERR)
				{ 
					switch (iId)
					{
					case 9:
						ActiveView->GridOrign(pt);
						break;
					case 10:
						Document->SetTrapPivotPoint(pt);
						break;
					case 11:
						ActiveView->ModelViewSetTarget(pt);
						break;
					default:
						app.HomePointSave(iId, pt);
					}
					::EndDialog(hDlg, TRUE);
					return (TRUE);
				}
			}
			break;

		case IDOK:
			pt.x = app.GetDialogItemUnitsText(hDlg, IDC_X);
			pt.y = app.GetDialogItemUnitsText(hDlg, IDC_Y);
			pt.z = app.GetDialogItemUnitsText(hDlg, IDC_Z);

			iId = int(::SendDlgItemMessageW(hDlg, IDC_LIST, CB_GETCURSEL, 0, 0L));
			if (iId == CB_ERR)
				break;
			switch (iId)
			{
			case 9:
				ActiveView->GridOrign(pt);
				break;
			case 10:
				Document->SetTrapPivotPoint(pt);
				break;
			case 11:
				ActiveView->ModelViewSetTarget(pt);
				break;
			default:
				app.HomePointSave(iId, pt);
			}
			::EndDialog(hDlg, TRUE);
			return (TRUE);

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
	}
	return (FALSE);
}

