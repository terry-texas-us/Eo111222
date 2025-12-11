#include "stdafx.h"
#include "AeSys.h"

double AeSys::GetDialogItemDouble(HWND hDlg, int controlIndex)
{
	WCHAR szBuf[32];

	::GetDlgItemTextW(hDlg, controlIndex, szBuf, 32);
	return (_wtof(szBuf));
}				

void AeSys::SetDialogItemDouble(HWND dialog, int controlIndex, double value)
{
	CString Text;
	Text.Format(L"%f", value);	
	SetDlgItemTextW(dialog, controlIndex, Text);
}
