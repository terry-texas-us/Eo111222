#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

void DlgProcSetupNoteInit(HWND hDlg);

BOOL CALLBACK DlgProcSetupNote(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
	switch (message) 
	{
	case WM_INITDIALOG:

		DlgProcSetupNoteInit(hDlg);
		return (TRUE);

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			{
				AeSysView* ActiveView = AeSysView::GetActiveView();
				CDC* DeviceContext = (ActiveView == NULL) ? NULL : ActiveView->GetDC();

				CCharCellDef ccd;
				pstate.GetCharCellDef(ccd);

				WCHAR szBuf[32];

				::GetDlgItemTextW(hDlg, IDC_TEXT_HEIGHT, szBuf, sizeof(szBuf) / sizeof(WCHAR));
				ccd.ChrHgtSet(_wtof(szBuf));
				::GetDlgItemTextW(hDlg, IDC_TEXT_ROTATION, szBuf, sizeof(szBuf) / sizeof(WCHAR));
				ccd.TextRotAngSet(EoToRadian(_wtof(szBuf)));
				::GetDlgItemTextW(hDlg, IDC_TEXT_EXP_FAC, szBuf, sizeof(szBuf) / sizeof(WCHAR));
				ccd.ChrExpFacSet(_wtof(szBuf));
				::GetDlgItemTextW(hDlg, IDC_TEXT_INCLIN, szBuf, sizeof(szBuf) / sizeof(WCHAR));
				ccd.ChrSlantAngSet(EoToRadian(_wtof(szBuf)));

				CFontDef fd;
				pstate.GetFontDef(fd);

				::GetDlgItemTextW(hDlg, IDC_TEXT_SPACING, szBuf, sizeof(szBuf) / sizeof(WCHAR));
				fd.CharacterSpacing(_wtof(szBuf));

				EoUInt16 HorizontalAlignment = IDC_TEXT_ALIGN_HOR_LEFT;
				while (!IsDlgButtonChecked(hDlg, HorizontalAlignment))
					HorizontalAlignment++;
				HorizontalAlignment = EoUInt16(1 - IDC_TEXT_ALIGN_HOR_LEFT + HorizontalAlignment);

				EoUInt16 VerticalAlignment = IDC_TEXT_ALIGN_VER_BOT;
				while (!IsDlgButtonChecked(hDlg, VerticalAlignment))
					VerticalAlignment++;
				VerticalAlignment = EoUInt16(4 + IDC_TEXT_ALIGN_VER_BOT - VerticalAlignment);

				fd.HorizontalAlignment(HorizontalAlignment);
				fd.VerticalAlignment(VerticalAlignment);

				EoUInt16 Path = IDC_PATH_RIGHT;
				while (!IsDlgButtonChecked(hDlg, Path))
					Path++;
				Path = EoUInt16(Path - IDC_PATH_RIGHT);

				fd.Path(Path);

				LRESULT lrSel = ::SendDlgItemMessageW(hDlg, IDC_FONT_NAME, CB_GETCURSEL, 0, 0L);
				if (lrSel != CB_ERR)
				{
					::SendDlgItemMessageW(hDlg, IDC_FONT_NAME, CB_GETLBTEXT, (WPARAM) lrSel, (LPARAM) ((LPTSTR) szBuf));
					fd.FontName(szBuf);	

					EoUInt16 Precision = EoUInt16(wcscmp(szBuf, L"Simplex.psf") != 0 ? EoDb::kEoTrueType : EoDb::kStrokeType);
					fd.Precision(Precision);
				}
				pstate.SetFontDef(DeviceContext, fd);
				pstate.SetCharCellDef(ccd);
			}
		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;
	}
	return (FALSE); 											
}

int CALLBACK EnumFontFamProc(const LPLOGFONT lplf, const LPTEXTMETRIC, int, LPARAM lParam)
{
	HWND hDlg = HWND(lParam);

	const LPENUMLOGFONT elf = (const LPENUMLOGFONT) lplf;
	::SendDlgItemMessageW(hDlg, IDC_FONT_NAME, CB_ADDSTRING, 0, (LPARAM) (LPCSTR) elf->elfFullName);

	return 1;
}

void DlgProcSetupNoteInit(HWND hDlg)
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	CDC* DeviceContext = (ActiveView == NULL) ? NULL : ActiveView->GetDC();

	LOGFONT lf;
	::ZeroMemory(&lf, sizeof(lf));

	lf.lfFaceName[0] = '\0';
	lf.lfCharSet = ANSI_CHARSET;

	::EnumFontFamiliesEx(DeviceContext->GetSafeHdc(), &lf, (FONTENUMPROC) EnumFontFamProc, LPARAM (hDlg), 0);

	::SendDlgItemMessageW(hDlg, IDC_FONT_NAME, CB_ADDSTRING, 0, (LPARAM) L"Simplex.psf");

	CCharCellDef ccd;
	pstate.GetCharCellDef(ccd);

	app.SetDialogItemDouble(hDlg, IDC_TEXT_HEIGHT, ccd.ChrHgtGet());
	app.SetDialogItemDouble(hDlg, IDC_TEXT_ROTATION, EoToDegree(ccd.TextRotAngGet()));
	app.SetDialogItemDouble(hDlg, IDC_TEXT_EXP_FAC, ccd.ChrExpFacGet());
	app.SetDialogItemDouble(hDlg, IDC_TEXT_INCLIN, EoToDegree(ccd.ChrSlantAngGet()));


	CFontDef fd;
	pstate.GetFontDef(fd);

	WCHAR szBuf[32];
	wcscpy_s(szBuf, 32, fd.FontName());
	::SendDlgItemMessageW(hDlg, IDC_FONT_NAME, CB_SELECTSTRING, (WPARAM) - 1, (LPARAM) ((LPTSTR) szBuf));

	app.SetDialogItemDouble(hDlg, IDC_TEXT_SPACING, fd.CharacterSpacing());

	::CheckRadioButton(hDlg, IDC_PATH_RIGHT, IDC_PATH_DOWN, IDC_PATH_RIGHT + fd.Path());
	::CheckRadioButton(hDlg, IDC_TEXT_ALIGN_HOR_LEFT, IDC_TEXT_ALIGN_HOR_RIGHT, IDC_TEXT_ALIGN_HOR_LEFT + fd.HorizontalAlignment() - 1);
	::CheckRadioButton(hDlg, IDC_TEXT_ALIGN_VER_BOT, IDC_TEXT_ALIGN_VER_TOP, IDC_TEXT_ALIGN_VER_BOT - fd.VerticalAlignment() + 4);
}
