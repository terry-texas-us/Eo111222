#include "stdafx.h"

#include "OwnerDraw.h"
#include "Polyline.h"

void SetupLineType_DrawEntire(LPDRAWITEMSTRUCT lpDIS, int inflate);
void SetupLineType_Init(HWND);

BOOL CALLBACK DlgProcSetupLineType(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL bTranslated;
	TCHAR szBuf[32];
	EoInt16 LineType;

	switch (message) 
	{
	case WM_INITDIALOG:
		SetupLineType_Init(hDlg);
		return (TRUE);

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			bTranslated = TRUE;
			LineType = EoInt16(GetDlgItemInt(hDlg, IDC_LINETYPES, &bTranslated, TRUE));
			if (bTranslated == 0)
			{
				bTranslated = ::GetDlgItemText(hDlg, IDC_LINETYPES, (LPTSTR) szBuf, sizeof(szBuf) / sizeof(TCHAR));
				if (bTranslated != 0)
				{
					EoDbLineTypeTable* LineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();
					LineType = LineTypeTable->LookupName(szBuf);
				}
				bTranslated = LineType != EoDbLineTypeTable::LINETYPE_LookupErr;
			}
			if (bTranslated)
			{
				// left broken while moving device context get out of SetLineType
				pstate.SetLineType(NULL, LineType);

				::EndDialog(hDlg, TRUE);
				return (TRUE);
			}
			break;

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT) lParam;
			if (lpDIS->itemID == - 1)	// Empty combo box .. Draw only focus rectangle
				OwnerDraw_Focus(lpDIS, 0);
			else
			{
				switch (lpDIS->itemAction)
				{
				case ODA_DRAWENTIRE:
					SetupLineType_DrawEntire(lpDIS, 0);
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
/// <summary>Draw the pen style number and a sample line showing its appearance.</summary>
void  SetupLineType_DrawEntire(LPDRAWITEMSTRUCT lpDIS, int)
{
	LRESULT Result = ::SendMessage(lpDIS->hwndItem, CB_GETCURSEL, 0, 0);
	if (Result != CB_ERR)
	{
		TCHAR LineTypeIndex[32];
		memset(LineTypeIndex, 0, 32);
		Result = ::SendMessage(lpDIS->hwndItem, CB_GETLBTEXT, lpDIS->itemID, (LPARAM) (LPTSTR) LineTypeIndex);

		if (Result && Result != CB_ERR)
		{
			EoDbLineTypeTable* LineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();
			EoInt16 nStyle = LineTypeTable->LookupName(LineTypeIndex);
			if (nStyle != EoDbLineTypeTable::LINETYPE_LookupErr)
			{
				CDC DeviceContext;
				DeviceContext.Attach(lpDIS->hDC);

				CRect rc;
				::CopyRect(&rc, &lpDIS->rcItem);

				int LineTypeIndexLength = int(Result);
				DeviceContext.ExtTextOut(rc.right - 72, rc.top + 2, ETO_CLIPPED, &rc, LineTypeIndex, LineTypeIndexLength, 0);

				EoInt16 nPenColor = pstate.PenColor();
				EoInt16 LineType = pstate.LineType();
				pstate.SetPen(NULL, &DeviceContext, 0, nStyle);

				rc.right -= 80;

				AeSysView* ActiveView = AeSysView::GetActiveView();

				ActiveView->ViewportPushActive();
				ActiveView->SetViewportSize(rc.right + rc.left, rc.bottom + rc.top);

				double UExtent = double(rc.right + rc.left) / double(DeviceContext.GetDeviceCaps(LOGPIXELSX)); 
				double VExtent = double(rc.bottom + rc.top) / double(DeviceContext.GetDeviceCaps(LOGPIXELSY)); 

				ActiveView->ModelViewPushActive();
				ActiveView->ModelViewInitialize();
				ActiveView->ModelViewSetWindow(0., 0., UExtent, VExtent);
				ActiveView->ModelViewSetTarget(EoGePoint3d::kOrigin);
				ActiveView->ModelViewSetCameraPosition(EoGeVector3d::kZAxis);

				CLine ln(EoGePoint3d(0., VExtent / 2., 0.), EoGePoint3d(UExtent, VExtent / 2., 0.));
				ln.Display(ActiveView, &DeviceContext);

				ActiveView->ModelViewPopActive();
				ActiveView->ViewportPopActive();

				OwnerDraw_Select(lpDIS, 0);
				OwnerDraw_Focus(lpDIS, 0);

				pstate.SetPen(NULL, &DeviceContext, nPenColor, LineType);
				DeviceContext.Detach();
			}
		}
	}
}
void SetupLineType_Init(HWND hDlg)
{
	HWND ComboBoxHandle = ::GetDlgItem(hDlg, IDC_LINETYPES);
	EoDbLineTypeTable* LineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();

	LineTypeTable->FillComboBox(ComboBoxHandle);
	EoInt16 LineType = pstate.LineType();
	::SendMessage(ComboBoxHandle, CB_SETCURSEL, LineType, 0L);
}