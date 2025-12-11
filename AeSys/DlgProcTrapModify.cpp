#include "stdafx.h"
#include "AeSysDoc.h"

#include "DlgProcTrapModify.h"
#include "Hatch.h"

/// <summary>Modifies attributes of all group primatives in current trap to current settings.</summary>
/// <remarks>Trap color index is not modified.</remarks>
BOOL CALLBACK DlgProcTrapModify(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	switch (anMsg)
	{
	case WM_INITDIALOG: 								
		return (TRUE);

	case WM_COMMAND:								
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			DlgProcTrapModifyDoOK(hDlg);				
			Document->UpdateAllViews(NULL, 0L, NULL);

		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
	}
	return (FALSE); 		
}

void DlgProcTrapModifyDoOK(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();
	if (::SendDlgItemMessageW(hDlg, IDC_MOD_PEN, BM_GETCHECK, 0, 0L))
		Document->ModifyTrappedGroupsPenColor(pstate.PenColor());
	if (::SendDlgItemMessageW(hDlg, IDC_MOD_LINE, BM_GETCHECK, 0, 0L))
		Document->ModifyTrappedGroupsLineType(pstate.LineType());
	if (::SendDlgItemMessageW(hDlg, IDC_MOD_FILL, BM_GETCHECK, 0, 0L))
		DlgProcTrapModifyPolygons();

	CCharCellDef ccd;
	pstate.GetCharCellDef(ccd);

	CFontDef fd;
	pstate.GetFontDef(fd);

	if (::SendDlgItemMessageW(hDlg, IDC_MOD_NOTE, BM_GETCHECK, 0, 0L))
		Document->ModifyTrappedGroupsNoteAttributes(fd, ccd, TM_TEXT_ALL);
	else if (::SendDlgItemMessageW(hDlg, IDC_FONT, BM_GETCHECK, 0, 0L))
		Document->ModifyTrappedGroupsNoteAttributes(fd, ccd, TM_TEXT_FONT);
	else if (::SendDlgItemMessageW(hDlg, IDC_HEIGHT, BM_GETCHECK, 0, 0L))
		Document->ModifyTrappedGroupsNoteAttributes(fd, ccd, TM_TEXT_HEIGHT);

}

void DlgProcTrapModifyPolygons()
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

			if (pPrim->Is(EoDbPrimitive::kPolygonPrimitive))
			{
				EoDbPolygon* pPolygon = static_cast<EoDbPolygon*>(pPrim);
				pPolygon->SetIntStyle(pstate.PolygonIntStyle());
				pPolygon->SetIntStyleId(pstate.PolygonIntStyleId());
				pPolygon->SetHatRefVecs(hatch::dOffAng, hatch::dXAxRefVecScal, hatch::dYAxRefVecScal);
			}
		}
	}
}
