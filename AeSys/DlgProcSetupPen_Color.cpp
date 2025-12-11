#include "stdafx.h"
#include "AeSys.h"

void SetupPenColor_DrawEntire(LPDRAWITEMSTRUCT, int);
void OwnerDraw_Focus(LPDRAWITEMSTRUCT lpDIS, int inflate)
{
	CRect rc;
	::CopyRect(&rc, &lpDIS->rcItem);
	::InflateRect(&rc, inflate, inflate);

	HBRUSH	hbr;
	if (lpDIS->itemState & ODS_FOCUS)
		hbr = (HBRUSH) ::GetStockObject(GRAY_BRUSH);
	else
		hbr = ::CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	::FrameRect(lpDIS->hDC, &rc, hbr);
	::DeleteObject(hbr);
}
void OwnerDraw_Select(LPDRAWITEMSTRUCT lpDIS, int inflate)
{
	CRect rc;
	::CopyRect(&rc, &lpDIS->rcItem);
	::InflateRect(&rc, inflate, inflate);

	HBRUSH	hbr;
	if (lpDIS->itemState & ODS_SELECTED)
		hbr = (HBRUSH) ::GetStockObject(BLACK_BRUSH);
	else
		hbr = ::CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	::FrameRect(lpDIS->hDC, &rc, hbr);
	::DeleteObject(hbr);
}

BOOL CALLBACK DlgProcSetupPenColor(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM  lParam)
{
	TCHAR szBuf[16];

	int 	i;
	EoInt16 nPenColor;

	switch (anMsg) 
	{
	case WM_INITDIALOG:
		for (i = 0; i < sizeof(ColorPalette) / sizeof(COLORREF); i++)
		{
			_itot_s(i, szBuf, 16, 10);
			::SendDlgItemMessage(hDlg, IDC_COL_LIST, CB_ADDSTRING, 0, (LPARAM) szBuf);
		}
		nPenColor = pstate.PenColor();
		::SendDlgItemMessage(hDlg, IDC_COL_LIST, CB_SETCURSEL, nPenColor, 0L);
		return (TRUE);

	case WM_COMMAND:										
		{
			switch (LOWORD(wParam)) 
			{			
			case IDOK:
				nPenColor = (EoInt16) ::GetDlgItemInt(hDlg, IDC_COL_LIST, 0, FALSE);
				// left broken while moving device context get out of SetPenColor
				pstate.SetPenColor(NULL /* DeviceContext */, nPenColor);

			case IDCANCEL:
				::EndDialog(hDlg, TRUE);
				return (TRUE);
			}
			break;
		}
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT) lParam;
			if (lpDIS->itemID == - 1)							// Empty combo box
				OwnerDraw_Focus(lpDIS, 0);					// Draw only focus rectangle
			else
			{
				switch (lpDIS->itemAction)
				{
				case ODA_DRAWENTIRE:
					SetupPenColor_DrawEntire(lpDIS, - 2);
					break;

				case ODA_SELECT:
					OwnerDraw_Select(lpDIS, 0);				  
					break;

				case ODA_FOCUS:
					OwnerDraw_Focus(lpDIS, 0);
					break;
				}
			}
			return (TRUE);
		}
	}
	return (FALSE); 		
}
/// <summary>Draw the pen color number and a rectangle filled with the pen color.</summary>
void  SetupPenColor_DrawEntire(LPDRAWITEMSTRUCT lpDIS, int inflate)
{
	CDC DeviceContext;
	DeviceContext.Attach(lpDIS->hDC);

	TCHAR PenColorNumber[16];
	memset(PenColorNumber, 0, 16);
	int PenColorNumberLength = (int) ::SendMessage(lpDIS->hwndItem, CB_GETLBTEXT, lpDIS->itemID, (LPARAM) (LPTSTR) PenColorNumber);

	CRect rc;
	::CopyRect(&rc, &lpDIS->rcItem);

	DeviceContext.ExtTextOut(rc.right - 16, rc.top + 2, ETO_CLIPPED, &rc, PenColorNumber, PenColorNumberLength, 0);

	::InflateRect(&rc, inflate - 2, inflate - 2);
	rc.right -= 24;

	CBrush brush(ColorPalette[lpDIS->itemID]);
	DeviceContext.FillRect(&rc, &brush);

	OwnerDraw_Select(lpDIS, inflate);
	OwnerDraw_Focus(lpDIS, inflate);

	DeviceContext.Detach();
}