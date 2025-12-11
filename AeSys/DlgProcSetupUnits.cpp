#include "stdafx.h"
#include "AeSys.h"

void UnitsDlgProcInit(HWND);
void UnitsDlgProcDoOK(HWND);

static WCHAR* MetricUnits[] =
{
	L"Meters",
	L"Millimeters",
	L"Centimeters",
	L"Decimeters",
	L"Kilometers",
	NULL
};

BOOL CALLBACK DlgProcUnits(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{
	switch (anMsg) 
	{
	case WM_INITDIALOG:
		UnitsDlgProcInit(hDlg);
		return (TRUE);

	case WM_COMMAND:

		switch (LOWORD(wParam)) 
		{
		case IDOK:
			UnitsDlgProcDoOK(hDlg);

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
		break;
	}
	return (FALSE); 		
}
void UnitsDlgProcInit(HWND hDlg)
{
	int	CheckButtonId = EoMin(IDC_ARCHITECTURAL + app.GetUnits(), IDC_METRIC);

	::CheckRadioButton(hDlg, IDC_ARCHITECTURAL, IDC_METRIC, CheckButtonId);

	SetDlgItemInt(hDlg, IDC_PRECISION, app.GetArchitecturalUnitsFractionPrecision(), FALSE);

	for (int i = 0; ::MetricUnits[i] != NULL; i++)
	{
		::SendDlgItemMessageW(hDlg, IDC_METRIC_UNITS, LB_ADDSTRING, 0, (LPARAM) MetricUnits[i]);
	}	
	if (CheckButtonId == IDC_METRIC)
	{
		::SendDlgItemMessageW(hDlg, IDC_METRIC_UNITS, LB_SETCURSEL, (WPARAM) (app.GetUnits() - AeSys::kMeters), 0L);
	}
}
void UnitsDlgProcDoOK(HWND hDlg)
{
	if (IsDlgButtonChecked(hDlg, IDC_ARCHITECTURAL))
	{
		app.SetUnits(AeSys::kArchitectural);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_ENGINEERING))
	{
		app.SetUnits(AeSys::kEngineering);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_FEET))
	{
		app.SetUnits(AeSys::kFeet);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_INCHES))
	{
		app.SetUnits(AeSys::kInches);
	}
	else
	{	// get units from metric list
		switch (::SendDlgItemMessageW(hDlg, IDC_METRIC_UNITS, LB_GETCURSEL, 0, 0L))
		{
		case 0:
			app.SetUnits(AeSys::kMeters);
			break;
		case 1:
			app.SetUnits(AeSys::kMillimeters);
			break;
		case 2:
			app.SetUnits(AeSys::kCentimeters);
			break;
		case 3:
			app.SetUnits(AeSys::kDecimeters);
			break;
		default:
			app.SetUnits(AeSys::kKilometers);
		}
	}						
	BOOL Translated;
	int Precision = GetDlgItemInt(hDlg, IDC_PRECISION, &Translated, FALSE);
	if (Translated)
	{
		app.SetArchitecturalUnitsFractionPrecision(Precision);
	}
}


