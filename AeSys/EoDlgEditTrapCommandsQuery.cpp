#include "Stdafx.h"
#include <Windows.h>
#include <afx.h>
#include <afxdd_.h>
#include <afxmsg_.h>
#include <afxstr.h>
#include <afxwin.h>
#include <algorithm>
#include <wchar.h>

#include "AeSysDoc.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoDlgEditTrapCommandsQuery.h"
#include "Resource.h"

HTREEITEM tvAddItem(HWND tree, HTREEITEM parent, LPWSTR text, LPCVOID object) {
  TV_INSERTSTRUCT tvIS{};
  tvIS.hParent = parent;
  tvIS.hInsertAfter = TVI_LAST;
  tvIS.item.mask = TVIF_TEXT | TVIF_PARAM;
  tvIS.item.pszText = const_cast<LPWSTR>(text);
  tvIS.item.lParam = reinterpret_cast<LPARAM>(object);

  return TreeView_InsertItem(tree, &tvIS);
}

IMPLEMENT_DYNAMIC(EoDlgEditTrapCommandsQuery, CDialog)

BEGIN_MESSAGE_MAP(EoDlgEditTrapCommandsQuery, CDialog)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_NOTIFY(TVN_SELCHANGED, IDC_GROUP_TREE, &EoDlgEditTrapCommandsQuery::OnTvnSelchangedGroupTree)
#pragma warning(pop)
END_MESSAGE_MAP()

EoDlgEditTrapCommandsQuery::EoDlgEditTrapCommandsQuery(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgEditTrapCommandsQuery::IDD, pParent) {}
EoDlgEditTrapCommandsQuery::~EoDlgEditTrapCommandsQuery() {}
void EoDlgEditTrapCommandsQuery::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_GROUP_TREE, m_GroupTreeViewControl);
  DDX_Control(dataExchange, IDC_GEOMETRY_LIST, m_GeometryListViewControl);
  DDX_Control(dataExchange, IDC_EXTRA_LIST_CTRL, m_ExtraListViewControl);
}

BOOL EoDlgEditTrapCommandsQuery::OnInitDialog() {
  CDialog::OnInitDialog();

  auto groupTreeWindow = ::GetDlgItem(this->GetSafeHwnd(), IDC_GROUP_TREE);
  auto* groupsInTrap = AeSysDoc::GetDoc()->GroupsInTrap();
  CString label{L"<Groups>"};
  auto groupListTreeItem = tvAddItem(groupTreeWindow, TVI_ROOT, label.GetBuffer(), groupsInTrap);
  groupsInTrap->AddToTreeViewControl(groupTreeWindow, groupListTreeItem);

  const auto oneInchWidth = static_cast<int>(GetDpiForSystem());

  m_ExtraListViewControl.InsertColumn(0, L"Property", LVCFMT_LEFT, oneInchWidth);
  m_ExtraListViewControl.InsertColumn(1, L"Value", LVCFMT_LEFT, oneInchWidth);

  m_GeometryListViewControl.InsertColumn(0, L"Property", LVCFMT_LEFT, oneInchWidth);
  m_GeometryListViewControl.InsertColumn(1, L"X-Axis", LVCFMT_LEFT, oneInchWidth);
  m_GeometryListViewControl.InsertColumn(2, L"Y-Axis", LVCFMT_LEFT, oneInchWidth);
  m_GeometryListViewControl.InsertColumn(3, L"Z-Axis", LVCFMT_LEFT, oneInchWidth);

  TreeView_Expand(groupTreeWindow, groupListTreeItem, TVE_EXPAND);
  return TRUE;
}

void EoDlgEditTrapCommandsQuery::OnTvnSelchangedGroupTree(NMHDR* pNMHDR, LRESULT* pResult) {
  LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

  wchar_t text[256]{};

  TV_ITEM item{};
  item.hItem = pNMTreeView->itemNew.hItem;
  item.mask = TVIF_TEXT | TVIF_PARAM;
  item.pszText = text;
  item.cchTextMax = sizeof(text) / sizeof(wchar_t);

  m_GroupTreeViewControl.GetItem(&item);
  m_ExtraListViewControl.DeleteAllItems();
  m_GeometryListViewControl.DeleteAllItems();

  if (wcscmp(item.pszText, L"<Groups>") == 0) {
  } else if (wcscmp(item.pszText, L"<Group>") == 0) {
  } else {
    auto* primitive = (EoDbPrimitive*)item.lParam;
    FillExtraList(primitive);
    FillGeometryList(primitive);
  }
  *pResult = 0;
}

void EoDlgEditTrapCommandsQuery::FillExtraList(EoDbPrimitive* primitive) {
  wchar_t szBuf[64]{};

  int iItem{0};

  CString extra;
  primitive->FormatExtra(extra);

  int offset{0};
  for (int delimiter = extra.Mid(offset).Find(';'); delimiter != -1;) {
    wcscpy_s(szBuf, extra.Mid(offset, delimiter));

    szBuf[sizeof(szBuf) / sizeof(wchar_t) - 1] = L'\0';  // Ensure zero-termination
    m_ExtraListViewControl.InsertItem(iItem, szBuf);

    offset += delimiter + 1;
    delimiter = extra.Mid(offset).Find('\t');
    int nLen = std::min(delimiter, static_cast<int>(sizeof(szBuf) / sizeof(wchar_t)) - 1);
    wcscpy_s(szBuf, 64, extra.Mid(offset, nLen));

    m_ExtraListViewControl.SetItemText(iItem++, 1, szBuf);

    offset += delimiter + 1;
    delimiter = extra.Mid(offset).Find(';');
  }
}

void EoDlgEditTrapCommandsQuery::FillGeometryList(EoDbPrimitive* primitive) {
  wchar_t szBuf[64]{};
  int iItem{0};

  CString geometry;
  primitive->FormatGeometry(geometry);

  int offset{0};
  for (int delimiter = geometry.Mid(offset).Find(';'); delimiter != -1;) {
    wcscpy_s(szBuf, 64, geometry.Mid(offset, delimiter));
    m_GeometryListViewControl.InsertItem(iItem, szBuf);
    offset += delimiter + 1;
    delimiter = geometry.Mid(offset).Find(';');
    wcscpy_s(szBuf, 64, geometry.Mid(offset, delimiter));
    m_GeometryListViewControl.SetItemText(iItem, 1, szBuf);
    offset += delimiter + 1;
    delimiter = geometry.Mid(offset).Find(';');
    wcscpy_s(szBuf, 64, geometry.Mid(offset, delimiter));
    m_GeometryListViewControl.SetItemText(iItem, 2, szBuf);
    offset += delimiter + 1;
    delimiter = geometry.Mid(offset).Find('\t');
    wcscpy_s(szBuf, 64, geometry.Mid(offset, delimiter));
    m_GeometryListViewControl.SetItemText(iItem++, 3, szBuf);
    offset += delimiter + 1;
    delimiter = geometry.Mid(offset).Find(';');
  }
}
