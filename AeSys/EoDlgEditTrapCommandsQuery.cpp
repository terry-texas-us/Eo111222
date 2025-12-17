#include "stdafx.h"

#include "AeSysDoc.h"
#include "EoDbPrimitive.h"
#include "EoDlgEditTrapCommandsQuery.h"

HTREEITEM tvAddItem(HWND tree, HTREEITEM parent, LPWSTR text, LPCVOID object) {
	TV_INSERTSTRUCT tvIS;
	tvIS.hParent = parent;
	tvIS.hInsertAfter = TVI_LAST;
	tvIS.item.mask = TVIF_TEXT | TVIF_PARAM;
	tvIS.item.hItem = nullptr;
	tvIS.item.iImage = 0;

	tvIS.item.pszText = (LPWSTR) text;
	tvIS.item.lParam = (LPARAM) object;
	return TreeView_InsertItem(tree, &tvIS);
}

// EoDlgEditTrapCommandsQuery dialog

IMPLEMENT_DYNAMIC(EoDlgEditTrapCommandsQuery, CDialog)

BEGIN_MESSAGE_MAP(EoDlgEditTrapCommandsQuery, CDialog)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_NOTIFY(TVN_SELCHANGED, IDC_GROUP_TREE, &EoDlgEditTrapCommandsQuery::OnTvnSelchangedGroupTree)
#pragma warning(pop)
END_MESSAGE_MAP()

EoDlgEditTrapCommandsQuery::EoDlgEditTrapCommandsQuery(CWnd* pParent /*=nullptr*/) :
	CDialog(EoDlgEditTrapCommandsQuery::IDD, pParent) {
}
EoDlgEditTrapCommandsQuery::~EoDlgEditTrapCommandsQuery() {
}
void EoDlgEditTrapCommandsQuery::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Control(dataExchange, IDC_GROUP_TREE, m_GroupTreeViewControl);
	DDX_Control(dataExchange, IDC_GEOMETRY_LIST, m_GeometryListViewControl);
	DDX_Control(dataExchange, IDC_EXTRA_LIST_CTRL, m_ExtraListViewControl);
}
BOOL EoDlgEditTrapCommandsQuery::OnInitDialog() {
	CDialog::OnInitDialog();

	HWND hWndGroupTree = ::GetDlgItem(this->GetSafeHwnd(), IDC_GROUP_TREE);
	EoDbGroupList* GroupsInTrap = AeSysDoc::GetDoc()->GroupsInTrap();
	HTREEITEM htiGroupList = tvAddItem(hWndGroupTree, TVI_ROOT, const_cast<LPWSTR>(L"<Groups>"), GroupsInTrap);
	GroupsInTrap->AddToTreeViewControl(hWndGroupTree, htiGroupList);

	m_ExtraListViewControl.InsertColumn(0, L"Property", LVCFMT_LEFT, 75);
	m_ExtraListViewControl.InsertColumn(1, L"Value", LVCFMT_LEFT, 150);

	m_GeometryListViewControl.InsertColumn(0, L"Property", LVCFMT_LEFT, 75);
	m_GeometryListViewControl.InsertColumn(1, L"X-Axis", LVCFMT_LEFT, 75);
	m_GeometryListViewControl.InsertColumn(2, L"Y-Axis", LVCFMT_LEFT, 75);
	m_GeometryListViewControl.InsertColumn(3, L"Z-Axis", LVCFMT_LEFT, 75);

	TreeView_Expand(hWndGroupTree, htiGroupList, TVE_EXPAND);
	return TRUE;
}
void EoDlgEditTrapCommandsQuery::OnTvnSelchangedGroupTree(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	WCHAR szText[256];
	szText[0] = '\0';

	TV_ITEM item;
	::ZeroMemory(&item, sizeof(item));
	item.hItem = pNMTreeView->itemNew.hItem;
	item.mask = TVIF_TEXT | TVIF_PARAM;
	item.pszText = szText;
	item.cchTextMax = sizeof(szText) / sizeof(WCHAR);

	m_GroupTreeViewControl.GetItem(&item);
	m_ExtraListViewControl.DeleteAllItems();
	m_GeometryListViewControl.DeleteAllItems();

	if (wcscmp(item.pszText, L"<Groups>") == 0) {
	}
	else if (wcscmp(item.pszText, L"<Group>") == 0) {
	}
	else {
		EoDbPrimitive* Primitive = (EoDbPrimitive*) item.lParam;
		FillExtraList(Primitive);
		FillGeometryList(Primitive);
	}
	*pResult = 0;
}
void EoDlgEditTrapCommandsQuery::FillExtraList(EoDbPrimitive* primitive) {
	WCHAR szBuf[64];

	int iItem = 0;

	CString str;
	primitive->FormatExtra(str);

	int nOff = 0;
	for (int nDel = str.Mid(nOff).Find(';'); nDel != - 1;) {
		wcscpy_s(szBuf, str.Mid(nOff, nDel));

		m_ExtraListViewControl.InsertItem(iItem, szBuf);

		nOff += nDel + 1;
		nDel = str.Mid(nOff).Find('\t');
		int nLen = std::min(nDel, static_cast<int>(sizeof(szBuf) / sizeof(WCHAR)) - 1);
		wcscpy_s(szBuf, 64, str.Mid(nOff, nLen));

		m_ExtraListViewControl.SetItemText(iItem++, 1, szBuf);

		nOff += nDel + 1;
		nDel = str.Mid(nOff).Find(';');
	}
}
void EoDlgEditTrapCommandsQuery::FillGeometryList(EoDbPrimitive* primitive) {
	WCHAR szBuf[64];
	int iItem = 0;

	CString strBuf;
	primitive->FormatGeometry(strBuf);

	int nOff = 0;
	for (int nDel = strBuf.Mid(nOff).Find(';'); nDel != - 1;) {
		wcscpy_s(szBuf, 64, strBuf.Mid(nOff, nDel));
		m_GeometryListViewControl.InsertItem(iItem, szBuf);
		nOff += nDel + 1;
		nDel = strBuf.Mid(nOff).Find(';');
		wcscpy_s(szBuf, 64, strBuf.Mid(nOff, nDel));
		m_GeometryListViewControl.SetItemText(iItem, 1, szBuf);
		nOff += nDel + 1;
		nDel = strBuf.Mid(nOff).Find(';');
		wcscpy_s(szBuf, 64, strBuf.Mid(nOff, nDel));
		m_GeometryListViewControl.SetItemText(iItem, 2, szBuf);
		nOff += nDel + 1;
		nDel = strBuf.Mid(nOff).Find('\t');
		wcscpy_s(szBuf, 64, strBuf.Mid(nOff, nDel));
		m_GeometryListViewControl.SetItemText(iItem++, 3, szBuf);
		nOff += nDel + 1;
		nDel = strBuf.Mid(nOff).Find(';');
	}
}
