#include "stdafx.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

void trapFilterByPenColor(EoInt16);
void trapFilterByLineStyle(int);
void trapFilterByPrimType(const EoDbPrimitive::PrimitiveTypes type);

BOOL CALLBACK DlgProcTrapFilter(HWND dialogHandle, UINT message, WPARAM wParam, LPARAM)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();
	EoDbLineTypeTable* LineTypeTable = Document->LineTypeTable();
	
	if (message == WM_INITDIALOG)
	{
		CString strElementName[] = {L"Arc", L"Insert", L"Line", L"Point", L"Text", L"Panel", L"Polyline"};

		SetDlgItemInt(dialogHandle, IDC_TRAP_FILTER_PEN_ID, 1, FALSE);

		CString Name;
		EoDbLineType* LineType;
		POSITION Position = LineTypeTable->GetStartPosition();
		while (Position)
		{
			LineTypeTable->GetNextAssoc(Position, Name, LineType);
			::SendDlgItemMessageW(dialogHandle, IDC_TRAP_FILTER_LINE_LIST, CB_ADDSTRING, 0, (LPARAM) (LPCWSTR) Name);
		}
		::SendDlgItemMessageW(dialogHandle, IDC_TRAP_FILTER_LINE_LIST, CB_SETCURSEL, 0, 0);

		for (EoUInt16 w = 0; w < sizeof(strElementName) / sizeof(strElementName[0]); w++)
			::SendDlgItemMessageW(dialogHandle, IDC_TRAP_FILTER_ELEMENT_LIST, LB_ADDSTRING, 0, (LPARAM) (LPCWSTR) strElementName[w]);

		::SendDlgItemMessageW(dialogHandle, IDC_TRAP_FILTER_ELEMENT_LIST, LB_SETCURSEL, 0, 0L);
		return (TRUE);
	}
	else if (message == WM_COMMAND)
	{
		EoInt16 nPenColor = SHRT_MAX;
		int iElementId = - 1;

		switch (LOWORD(wParam))
		{
		case IDOK:
			if (IsDlgButtonChecked(dialogHandle, IDC_TRAP_FILTER_PEN))
			{
				nPenColor = EoInt16(GetDlgItemInt(dialogHandle, IDC_TRAP_FILTER_PEN_ID, 0, FALSE));
				trapFilterByPenColor(nPenColor);
			}
			if (IsDlgButtonChecked(dialogHandle, IDC_TRAP_FILTER_LINE))
			{
				EoUInt16 LineTypeIndex = SHRT_MAX;
				WCHAR szBuf[32];

				if (::GetDlgItemTextW(dialogHandle, IDC_TRAP_FILTER_LINE_LIST, (LPTSTR) szBuf, sizeof(szBuf) / sizeof(WCHAR)))
				{
					EoDbLineType* LineType;
					if (LineTypeTable->Lookup(szBuf, LineType))
					{
						LineTypeIndex = LineType->Index();
					}
				}
				if (LineTypeIndex != SHRT_MAX)
				{
					trapFilterByLineStyle(LineTypeIndex);
				}
			}
			if (IsDlgButtonChecked(dialogHandle, IDC_TRAP_FILTER_ELEMENT))
			{
				iElementId = int(::SendDlgItemMessageW(dialogHandle, IDC_TRAP_FILTER_ELEMENT_LIST, LB_GETCURSEL, 0, 0));

				switch (iElementId)	// **PrimitiveTypes**
				{
				case 0:
					trapFilterByPrimType(EoDbPrimitive::kEllipsePrimitive);
					break;
				case 1:
					trapFilterByPrimType(EoDbPrimitive::kGroupReferencePrimitive);
					break;
				case 2:
					trapFilterByPrimType(EoDbPrimitive::kLinePrimitive);
					break;
				case 3:
					trapFilterByPrimType(EoDbPrimitive::kPointPrimitive);
					break;
				case 4:
					trapFilterByPrimType(EoDbPrimitive::kTextPrimitive);
					break;
				case 5:
					trapFilterByPrimType(EoDbPrimitive::kPolygonPrimitive);
					break;
				case 6:
					trapFilterByPrimType(EoDbPrimitive::kPolylinePrimitive);

				}
			}

		case IDCANCEL:
			::EndDialog(dialogHandle, TRUE);
			return (TRUE);
		}
	}
	return (FALSE); 		
}

void trapFilterByLineStyle(int iLnId)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	POSITION pos = Document->GetFirstTrappedGroupPosition();
	while (pos != 0)
	{
		EoDbGroup* Group = Document->GetNextTrappedGroup(pos);

		POSITION posPrim = Group->GetHeadPosition();
		while (posPrim != 0)
		{
			EoDbPrimitive* pPrim = Group->GetNext(posPrim);
			if (pPrim->LineType() == iLnId)
			{	// Line match remove it
				Document->RemoveTrappedGroup(Group);
				Document->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
				break;							 
			}
		}
	}
	AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
void trapFilterByPenColor(EoInt16 nPenColor)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	POSITION pos = Document->GetFirstTrappedGroupPosition();
	while (pos != 0)
	{
		EoDbGroup* Group = Document->GetNextTrappedGroup(pos);

		POSITION posPrim = Group->GetHeadPosition();
		while (posPrim != 0)
		{
			EoDbPrimitive* pPrim = Group->GetNext(posPrim);
			if (pPrim->PenColor() == nPenColor)
			{	// Color match remove it
				Document->RemoveTrappedGroup(Group);
				Document->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
				break;							 
			}
		}
	}
	AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
void trapFilterByPrimType(const EoDbPrimitive::PrimitiveTypes type)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	POSITION pos = Document->GetFirstTrappedGroupPosition();
	while (pos != 0)
	{
		bool bFilter = FALSE;

		EoDbGroup* Group = Document->GetNextTrappedGroup(pos);

		POSITION posPrim = Group->GetHeadPosition();
		while (posPrim != 0)
		{
			EoDbPrimitive* pPrim = Group->GetNext(posPrim);

			switch(type)
			{
			case EoDbPrimitive::kLinePrimitive:
				bFilter = pPrim->Is(EoDbPrimitive::kLinePrimitive);
				break;
			case EoDbPrimitive::kEllipsePrimitive:
				bFilter = pPrim->Is(EoDbPrimitive::kEllipsePrimitive);
				break;
			case EoDbPrimitive::kGroupReferencePrimitive:
				bFilter = pPrim->Is(EoDbPrimitive::kGroupReferencePrimitive);
				break;
			case EoDbPrimitive::kTextPrimitive:
				bFilter = pPrim->Is(EoDbPrimitive::kTextPrimitive);
				break;
			case EoDbPrimitive::kPolygonPrimitive:
				bFilter = pPrim->Is(EoDbPrimitive::kPolygonPrimitive);
				break;
			case EoDbPrimitive::kPolylinePrimitive:
				bFilter = pPrim->Is(EoDbPrimitive::kPolylinePrimitive);
				break;
			case EoDbPrimitive::kPointPrimitive:
				bFilter = pPrim->Is(EoDbPrimitive::kPointPrimitive);
				break;
			}
			if (bFilter)
			{
				Document->RemoveTrappedGroup(Group);
				Document->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
				break;							 
			}
		}
	}
	AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
