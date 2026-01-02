#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbGroup.h"
#include "EoDlgBlockInsert.h"
#include "Preview.h"
#include "Resource.h"

// EoDlgBlockInsert dialog

IMPLEMENT_DYNAMIC(EoDlgBlockInsert, CDialog)

BEGIN_MESSAGE_MAP(EoDlgBlockInsert, CDialog)
ON_LBN_SELCHANGE(IDC_BLOCKS_LIST, &EoDlgBlockInsert::OnLbnSelchangeBlocksList)
ON_BN_CLICKED(IDC_PURGE, &EoDlgBlockInsert::OnBnClickedPurge)
END_MESSAGE_MAP()

EoGePoint3d EoDlgBlockInsert::InsertionPoint;

EoDlgBlockInsert::EoDlgBlockInsert(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgBlockInsert::IDD, pParent) {}
EoDlgBlockInsert::EoDlgBlockInsert(AeSysDoc* document, CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgBlockInsert::IDD, pParent), m_Document(document) {}
EoDlgBlockInsert::~EoDlgBlockInsert() {}
void EoDlgBlockInsert::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_BLOCKS_LIST, m_BlocksListBoxControl);
}
BOOL EoDlgBlockInsert::OnInitDialog() {
  CDialog::OnInitDialog();

  InsertionPoint = app.GetCursorPosition();

  CString BlockName;
  EoDbBlock* Block;

  auto BlockPosition = m_Document->GetFirstBlockPosition();
  while (BlockPosition != nullptr) {
    m_Document->GetNextBlock(BlockPosition, BlockName, Block);
    if (!Block->IsAnonymous()) { m_BlocksListBoxControl.AddString(BlockName); }
  }
  m_BlocksListBoxControl.SetCurSel(0);

  if (m_Document->BlockTableIsEmpty()) {
    WndProcPreviewClear(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd());
  } else {
    BlockPosition = m_Document->GetFirstBlockPosition();
    m_Document->GetNextBlock(BlockPosition, BlockName, Block);
    SetDlgItemInt(IDC_GROUPS, static_cast<UINT>(Block->GetCount()), FALSE);
    SetDlgItemInt(IDC_REFERENCES, static_cast<UINT>(m_Document->GetBlockReferenceCount(BlockName)), FALSE);
    WndProcPreviewUpdateBlock(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd(), Block);
  }
  return TRUE;
}
void EoDlgBlockInsert::OnOK() {
  int CurrentSelection = m_BlocksListBoxControl.GetCurSel();

  if (CurrentSelection != LB_ERR) {
    CString BlockName;
    m_BlocksListBoxControl.GetText(CurrentSelection, BlockName);

    EoDbBlockReference* BlockReference = new EoDbBlockReference(BlockName, InsertionPoint);

    EoDbGroup* Group = new EoDbGroup(BlockReference);
    m_Document->AddWorkLayerGroup(Group);
    m_Document->UpdateAllViews(nullptr, EoDb::kGroup, Group);
  }
  CDialog::OnOK();
}
void EoDlgBlockInsert::OnLbnSelchangeBlocksList() {
  int CurrentSelection = m_BlocksListBoxControl.GetCurSel();

  if (CurrentSelection != LB_ERR) {
    CString BlockName;
    m_BlocksListBoxControl.GetText(CurrentSelection, BlockName);

    EoDbBlock* Block;
    m_Document->LookupBlock(BlockName, Block);
    SetDlgItemInt(IDC_GROUPS, static_cast<UINT>(Block->GetCount()), FALSE);
    SetDlgItemInt(IDC_REFERENCES, static_cast<UINT>(m_Document->GetBlockReferenceCount(BlockName)), FALSE);
    WndProcPreviewUpdateBlock(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd(), Block);
  }
}
void EoDlgBlockInsert::OnBnClickedPurge() {
  m_Document->RemoveUnusedBlocks();

  CDialog::OnOK();
}
