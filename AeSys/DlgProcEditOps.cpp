#include "stdafx.h"
#include "AeSys.h"
#include "DlgProcEditOps.h"

namespace dlgproceditops
{
	double dXRotAng;
	double dYRotAng;
	double dZRotAng;

	double dXScale;
	double dYScale;
	double dZScale;

	double dXMirrorScale;
	double dYMirrorScale;
	double dZMirrorScale;

	int iRotOrd[3];
}
using namespace dlgproceditops;

EoGeVector3d dlgproceditops::GetMirrorScale()
{
	return (EoGeVector3d(dXMirrorScale, dYMirrorScale, dZMirrorScale));
}
EoGeVector3d dlgproceditops::GetRotAng()
{
	return (EoGeVector3d(dXRotAng, dYRotAng, dZRotAng));
}
CTMat dlgproceditops::GetInvertedRotTrnMat()
{
	CTMat tm(iRotOrd, GetRotAng());
	tm.Inverse();
	return (tm);
}
void dlgproceditops::GetRotOrd(int* order)
{
	order[0] = iRotOrd[0];
	order[1] = iRotOrd[1];
	order[2] = iRotOrd[2];
}
CTMat dlgproceditops::GetRotTrnMat()
{
	return (CTMat(iRotOrd, GetRotAng()));
}
EoGeVector3d dlgproceditops::GetScaleFactors()
{
	return (EoGeVector3d(dXScale, dYScale, dZScale));
}
EoGeVector3d dlgproceditops::GetInvertedScale()
{
	double dX = fabs(dXScale) > DBL_EPSILON ? 1. / dXScale : 1.;
	double dY = fabs(dYScale) > DBL_EPSILON ? 1. / dYScale : 1.;
	double dZ = fabs(dZScale) > DBL_EPSILON ? 1. / dZScale : 1.;
	
	return (EoGeVector3d(dX, dY, dZ));
}				
void dlgproceditops::SetMirrorScale(double dX, double dY, double dZ) 
{
	dXMirrorScale = dX;
	dYMirrorScale = dY;
	dZMirrorScale = dZ;
}
void dlgproceditops::SetRotAng(double dX, double dY, double dZ) 
{
	dXRotAng = dX;
	dYRotAng = dY;
	dZRotAng = dZ;
}
void dlgproceditops::SetRotOrd(int i0, int i1, int i2)
{
	iRotOrd[0] = i0;
	iRotOrd[1] = i1;
	iRotOrd[2] = i2;
}
void dlgproceditops::SetScale(double dX, double dY, double dZ) 
{
	dXScale = dX;
	dYScale = dY;
	dZScale = dZ;
}

BOOL CALLBACK DlgProcEditOps(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{
	if (anMsg == WM_INITDIALOG)
	{
		app.SetDialogItemDouble(hDlg, IDC_EDIT_OP_SIZ_X, dXScale);
		app.SetDialogItemDouble(hDlg, IDC_EDIT_OP_SIZ_Y, dYScale);
		app.SetDialogItemDouble(hDlg, IDC_EDIT_OP_SIZ_Z, dZScale);

		app.SetDialogItemDouble(hDlg, IDC_EDIT_OP_ROT_X, dXRotAng);
		app.SetDialogItemDouble(hDlg, IDC_EDIT_OP_ROT_Y, dYRotAng);
		app.SetDialogItemDouble(hDlg, IDC_EDIT_OP_ROT_Z, dZRotAng);

		if (dXMirrorScale < 0.)
			::CheckDlgButton(hDlg, IDC_EDIT_OP_MIR_X, 1);
		else if (dYMirrorScale < 0.)
			::CheckDlgButton(hDlg, IDC_EDIT_OP_MIR_Y, 1);
		else
			::CheckDlgButton(hDlg, IDC_EDIT_OP_MIR_Z, 1);
		return (TRUE);
	}
	else if (anMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam)) 
		{
		case IDC_EDIT_OP_SIZING:
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_SIZ_X), TRUE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_SIZ_Y), TRUE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_SIZ_Z), TRUE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_ROT_X), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_ROT_Y), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_ROT_Z), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_MIR_X), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_MIR_Y), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_MIR_Z), FALSE);
			return (TRUE);

		case IDC_EDIT_OP_ROTATION:
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_SIZ_X), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_SIZ_Y), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_SIZ_Z), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_ROT_X), TRUE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_ROT_Y), TRUE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_ROT_Z), TRUE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_MIR_X), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_MIR_Y), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_MIR_Z), FALSE);
			return (TRUE);

		case IDC_EDIT_OP_MIRRORING:
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_SIZ_X), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_SIZ_Y), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_SIZ_Z), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_ROT_X), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_ROT_Y), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_ROT_Z), FALSE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_MIR_X), TRUE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_MIR_Y), TRUE);
			::EnableWindow(::GetDlgItem(hDlg, IDC_EDIT_OP_MIR_Z), TRUE);
			return (TRUE);

		case IDC_EDIT_OP_MIR_X:
			::CheckDlgButton(hDlg, IDC_EDIT_OP_MIR_Y, 0);
			::CheckDlgButton(hDlg, IDC_EDIT_OP_MIR_Z, 0);
			return (TRUE);

		case IDC_EDIT_OP_MIR_Y:
			::CheckDlgButton(hDlg, IDC_EDIT_OP_MIR_X, 0);
			::CheckDlgButton(hDlg, IDC_EDIT_OP_MIR_Z, 0);
			return (TRUE);

		case IDC_EDIT_OP_MIR_Z:
			::CheckDlgButton(hDlg, IDC_EDIT_OP_MIR_X, 0);
			::CheckDlgButton(hDlg, IDC_EDIT_OP_MIR_Y, 0);
			return (TRUE);

		case IDOK:
			dXScale = app.GetDialogItemDouble(hDlg, IDC_EDIT_OP_SIZ_X);
			dYScale = app.GetDialogItemDouble(hDlg, IDC_EDIT_OP_SIZ_Y);
			dZScale = app.GetDialogItemDouble(hDlg, IDC_EDIT_OP_SIZ_Z);

			dXRotAng = app.GetDialogItemDouble(hDlg, IDC_EDIT_OP_ROT_X);
			dYRotAng = app.GetDialogItemDouble(hDlg, IDC_EDIT_OP_ROT_Y);
			dZRotAng = app.GetDialogItemDouble(hDlg, IDC_EDIT_OP_ROT_Z);

			if (IsDlgButtonChecked(hDlg, IDC_EDIT_OP_MIR_X))
				SetMirrorScale(- 1, 1., 1.);
			else if (IsDlgButtonChecked(hDlg, IDC_EDIT_OP_MIR_Y))
				SetMirrorScale(1., - 1., 1.);
			else
				SetMirrorScale(1., 1., - 1.);

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
	}
	return (FALSE); 		
}
