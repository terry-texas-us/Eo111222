#include "stdafx.h"
#include "AeSysDoc.h"
#include "DlgProcEditTrap_CommandsQuery.h"

HTREEITEM tvAddItem(HWND hTree, HTREEITEM hParent, LPTSTR pszText, CObject* object)
{
	TV_INSERTSTRUCT tvIS;
	tvIS.hParent = hParent;
	tvIS.hInsertAfter = TVI_LAST;
	tvIS.item.mask = TVIF_TEXT | TVIF_PARAM;
	tvIS.item.hItem = NULL;
	tvIS.item.iImage = 0;

	tvIS.item.pszText = (LPWSTR) pszText;
	tvIS.item.lParam = (LPARAM) object;
	return TreeView_InsertItem(hTree, &tvIS);
}

BOOL CALLBACK DlgProcEditTrap_CommandsQuery(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hWndGroupTree = ::GetDlgItem(hDlg, IDC_GROUP_TREE);
	HWND hWndExtra = ::GetDlgItem(hDlg, IDC_EXTRA_LIST_CTRL);
	HWND hWndGeometry = ::GetDlgItem(hDlg, IDC_GEOMETRY_LIST);
	switch (message)
	{ 
	case WM_INITDIALOG:
		{
			EoDbGroupList* GroupsInTrap = AeSysDoc::GetDoc()->GroupsInTrap();
			HTREEITEM htiGroupList = tvAddItem(hWndGroupTree, TVI_ROOT, L"<Groups>", GroupsInTrap);

			GroupsInTrap->AddToTreeViewControl(hWndGroupTree, htiGroupList);

			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = 75;
			lvc.iSubItem = 0; lvc.pszText = L"Property";
			ListView_InsertColumn(hWndExtra, 0, &lvc); 
			lvc.cx = 150;
			lvc.iSubItem = 1; lvc.pszText = L"Value";
			ListView_InsertColumn(hWndExtra, 1, &lvc);

			lvc.cx = 75;
			lvc.iSubItem = 0; lvc.pszText = L"Property";
			ListView_InsertColumn(hWndGeometry, 0, &lvc); 
			lvc.iSubItem = 1; lvc.pszText = L"X-Axis";
			ListView_InsertColumn(hWndGeometry, 1, &lvc);
			lvc.iSubItem = 2; lvc.pszText = L"Y-Axis";
			ListView_InsertColumn(hWndGeometry, 2, &lvc);
			lvc.iSubItem = 3; lvc.pszText = L"Z-Axis";
			ListView_InsertColumn(hWndGeometry, 3, &lvc);


			TreeView_Expand(hWndGroupTree, htiGroupList, TVE_EXPAND);
			return (TRUE);
		}
	case WM_NOTIFY:
		{	
			NMHDR*  pnm = (NMHDR*) lParam;
			if (pnm->idFrom == IDC_GROUP_TREE)
			{
				if (pnm->code == TVN_SELCHANGED)
				{
					NM_TREEVIEW* nmtv = (NM_TREEVIEW*) lParam;
					WCHAR szText[256];
					szText[0] = '\0';
					TV_ITEM item;
					::ZeroMemory(&item, sizeof(item));
					item.hItem = nmtv->itemNew.hItem;
					item.mask = TVIF_TEXT | TVIF_PARAM;
					item.pszText = szText;
					item.cchTextMax = sizeof(szText) / sizeof(WCHAR);
					TreeView_GetItem(hWndGroupTree, &item);

					ListView_DeleteAllItems(hWndExtra);
					ListView_DeleteAllItems(hWndGeometry);

					if (wcscmp(item.pszText, L"<Groups>") == 0) 
					{
					}
					else if (wcscmp(item.pszText, L"<Group>") == 0) 
					{
					}
					else
					{
						EoDbPrimitive* Primitive = (EoDbPrimitive*) item.lParam;
						DlgProcEditTrap_CommandsQueryFillExtraList(hDlg, Primitive);
						DlgProcEditTrap_CommandsQueryFillGeometryList(hDlg, Primitive);
					}
					return (TRUE);
				}
			}
			break;
		}	
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDCANCEL)
			{
				::EndDialog(hDlg, TRUE);
				return (TRUE);
			}
		}
	}
	return (FALSE); 		
}
void DlgProcEditTrap_CommandsQueryFillExtraList(HWND hDlg, EoDbPrimitive* pPrim)
{
	HWND hWndExtra = ::GetDlgItem(hDlg, IDC_EXTRA_LIST_CTRL);

	LVITEM lvi; 
	::ZeroMemory(&lvi, sizeof(lvi));
	lvi.mask = LVIF_TEXT | LVIF_STATE;

	WCHAR szBuf[64];

	int iItem = 0;

	CString str;
	pPrim->FormatExtra(str);

	size_t nOff = 0;
	for (size_t nDel = str.Mid(nOff).Find(';'); nDel != - 1;)
	{
		lvi.iItem = iItem; 
		lvi.pszText = szBuf;

		wcscpy_s(szBuf, str.Mid(nOff, nDel));
		ListView_InsertItem (hWndExtra, &lvi);
		nOff += nDel + 1;
		nDel = str.Mid(nOff).Find('\t');
		size_t nLen = min(nDel, sizeof(szBuf) / sizeof(WCHAR) - 1);
		wcscpy_s(szBuf, 64, str.Mid(nOff, nLen));
		ListView_SetItemText(hWndExtra, iItem++, 1, szBuf);
		nOff += nDel + 1;
		nDel = str.Mid(nOff).Find(';');
	}							 
}	 

void DlgProcEditTrap_CommandsQueryFillGeometryList(HWND hDlg, EoDbPrimitive* pPrim)
{
	HWND hWndGeometry = ::GetDlgItem(hDlg, IDC_GEOMETRY_LIST);

	LVITEM lvi; 
	::ZeroMemory(&lvi, sizeof(lvi));
	lvi.mask = LVIF_TEXT | LVIF_STATE;

	WCHAR szBuf[64];
	int iItem = 0;

	CString strBuf;
	pPrim->FormatGeometry(strBuf);

	size_t nOff = 0;
	for (size_t nDel = strBuf.Mid(nOff).Find(';'); nDel != - 1;)
	{
		lvi.iItem = iItem; 
		lvi.pszText = szBuf;

		wcscpy_s(szBuf, 64, strBuf.Mid(nOff, nDel));
		ListView_InsertItem (hWndGeometry, &lvi);
		nOff += nDel + 1;
		nDel = strBuf.Mid(nOff).Find(';');
		wcscpy_s(szBuf, 64, strBuf.Mid(nOff, nDel));
		ListView_SetItemText(hWndGeometry, iItem, 1, szBuf);
		nOff += nDel + 1;
		nDel = strBuf.Mid(nOff).Find(';');
		wcscpy_s(szBuf, 64, strBuf.Mid(nOff, nDel));
		ListView_SetItemText(hWndGeometry, iItem, 2, szBuf);
		nOff += nDel + 1;
		nDel = strBuf.Mid(nOff).Find('\t');
		wcscpy_s(szBuf, 64, strBuf.Mid(nOff, nDel));
		ListView_SetItemText(hWndGeometry, iItem++, 3, szBuf);
		nOff += nDel + 1;
		nDel = strBuf.Mid(nOff).Find(';');
	}							 
}