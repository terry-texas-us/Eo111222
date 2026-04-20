#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDlgEditBlockDefinition.h"
#include "Resource.h"
#include "WndProcPreview.h"

BEGIN_MESSAGE_MAP(EoDlgEditBlockDefinition, CDialog)
ON_LBN_SELCHANGE(IDC_BLOCKS_LIST, &EoDlgEditBlockDefinition::OnLbnSelchangeBlocksList)
END_MESSAGE_MAP()

EoDlgEditBlockDefinition::EoDlgEditBlockDefinition(AeSysDoc* document, CWnd* parent)
    : CDialog(EoDlgEditBlockDefinition::IDD, parent), m_document(document) {}

EoDlgEditBlockDefinition::~EoDlgEditBlockDefinition() {}

void EoDlgEditBlockDefinition::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_BLOCKS_LIST, m_blocksListBoxControl);
}

BOOL EoDlgEditBlockDefinition::OnInitDialog() {
  CDialog::OnInitDialog();

  CString blockName;
  EoDbBlock* block{};

  auto position = m_document->GetFirstBlockPosition();
  while (position != nullptr) {
    m_document->GetNextBlock(position, blockName, block);
    if (block->IsAnonymous() || block->IsSystemBlock(blockName)) { continue; }
    if (block->IsModelSpace(blockName.GetString()) || block->IsPaperSpace(blockName.GetString())) { continue; }
    m_blocksListBoxControl.AddString(blockName);
  }
  m_blocksListBoxControl.SetCurSel(0);

  if (m_document->BlockTableIsEmpty() || m_blocksListBoxControl.GetCount() == 0) {
    WndProcPreviewClear(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd());
  } else {
    CString firstName;
    m_blocksListBoxControl.GetText(0, firstName);
    EoDbBlock* firstBlock{};
    if (m_document->LookupBlock(firstName, firstBlock)) {
      SetDlgItemInt(IDC_GROUPS, static_cast<UINT>(firstBlock->GetCount()), FALSE);
      SetDlgItemInt(IDC_REFERENCES, static_cast<UINT>(m_document->GetBlockReferenceCount(firstName)), FALSE);
      WndProcPreviewUpdateBlock(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd(), firstBlock);
    }
  }
  CenterWindow(AfxGetMainWnd());
  return TRUE;
}

void EoDlgEditBlockDefinition::OnOK() {
  auto currentSelection = m_blocksListBoxControl.GetCurSel();
  if (currentSelection != LB_ERR) {
    m_blocksListBoxControl.GetText(currentSelection, m_selectedBlockName);
  }
  CDialog::OnOK();
}

void EoDlgEditBlockDefinition::OnLbnSelchangeBlocksList() {
  auto currentSelection = m_blocksListBoxControl.GetCurSel();
  if (currentSelection != LB_ERR) {
    CString blockName;
    m_blocksListBoxControl.GetText(currentSelection, blockName);

    EoDbBlock* block{};
    m_document->LookupBlock(blockName, block);
    SetDlgItemInt(IDC_GROUPS, static_cast<UINT>(block->GetCount()), FALSE);
    SetDlgItemInt(IDC_REFERENCES, static_cast<UINT>(m_document->GetBlockReferenceCount(blockName)), FALSE);
    WndProcPreviewUpdateBlock(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd(), block);
  }
}
