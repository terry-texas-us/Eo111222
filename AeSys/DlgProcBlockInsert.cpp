#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "Preview.h"

void	BlockInsertDoOK(HWND);
LRESULT	BlockInsertGetCurSel(HWND, int, CString& strName);

EoGePoint3d* ptIns = 0;

BOOL CALLBACK DlgProcBlockInsert(HWND hDlg, UINT anMsg, WPARAM wParam, LPARAM)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	switch (anMsg)
	{
	case WM_INITDIALOG:
		{
			ptIns = new EoGePoint3d(app.GetCursorPosition());

			CString strKey;
			EoDbBlock* Block;

			POSITION pos = Document->GetFirstBlockPosition();
			while (pos != NULL)
			{
				Document->GetNextBlock(pos, strKey, Block);
				if (!Block->IsAnonymous())
					::SendDlgItemMessageW(hDlg, IDC_BLOCKS_LIST, LB_ADDSTRING, 0, (LPARAM) (LPCWSTR) strKey);
			}
			::SendDlgItemMessageW(hDlg, IDC_BLOCKS_LIST, LB_SETCURSEL, 0, 0L);

			if (Document->BlockTableIsEmpty())
			{
				WndProcPreviewClear(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW));
			}
			else
			{
				pos = Document->GetFirstBlockPosition();
				Document->GetNextBlock(pos, strKey, Block);
				SetDlgItemInt(hDlg, IDC_GROUPS, (UINT) Block->GetCount(), FALSE);
				SetDlgItemInt(hDlg, IDC_REFERENCES, Document->GetBlockReferenceCount(strKey), FALSE);
				WndProcPreviewUpdate(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW), Block);
			}
			return (TRUE);
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BLOCKS_LIST:
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				EoDbBlock* Block;
				CString strName;
				BlockInsertGetCurSel(hDlg, IDC_BLOCKS_LIST, strName);
				Document->LookupBlock(strName, Block);
				SetDlgItemInt(hDlg, IDC_GROUPS, (UINT) Block->GetCount(), FALSE);
				SetDlgItemInt(hDlg, IDC_REFERENCES, Document->GetBlockReferenceCount(strName), FALSE);
				WndProcPreviewUpdate(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW), Block);
			}
			break;

		case IDC_PURGE:
			Document->RemoveUnusedBlocks();
			delete ptIns;
			ptIns = 0;
			::EndDialog(hDlg, TRUE);
			return (TRUE);

		case IDOK:
			BlockInsertDoOK(hDlg);

		case IDCANCEL:
			delete ptIns;
			ptIns = 0;
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
	}
	return (FALSE); 		
}
void BlockInsertDoOK(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strBlkNam;
	if (BlockInsertGetCurSel(hDlg, IDC_BLOCKS_LIST, strBlkNam) != LB_ERR)
	{
		EoDbBlockReference* pSegRef = new EoDbBlockReference(strBlkNam, *ptIns);

		EoDbGroup* Group = new EoDbGroup(pSegRef);
		Document->AddWorkLayerGroup(Group); 
		Document->UpdateAllViews(NULL, EoDb::kGroup, Group);
	}
}
/// <summary> Gets the currently selected block name</summary>
LRESULT BlockInsertGetCurSel(HWND hDlg, int listIndex, CString& name)
{
	LRESULT Result = ::SendDlgItemMessageW(hDlg, listIndex, LB_GETCURSEL, 0, 0L);

	if (Result != LB_ERR)
	{
		int CurrentSelection = int(Result);

		WCHAR Name[MAX_PATH];
		memset(Name, 0, MAX_PATH);
		Result = ::SendDlgItemMessageW(hDlg, listIndex, LB_GETTEXT, (WPARAM) CurrentSelection, (LPARAM) (LPTSTR) Name);
		if (Result && Result != LB_ERR)
		{
			name.SetString(Name, int(Result));
		}
	}
	return Result;
}