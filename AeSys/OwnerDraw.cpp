#include "stdafx.h"

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
