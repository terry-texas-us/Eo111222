#include "Stdafx.h"

#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDlgFileManageBlocks.h"
#include "Resource.h"
#include "WndProcPreview.h"

BEGIN_MESSAGE_MAP(EoDlgFileManageBlocks, CDialog)
ON_LBN_SELCHANGE(IDC_BLOCKS_LIST, &EoDlgFileManageBlocks::OnLbnSelchangeBlocksList)
END_MESSAGE_MAP()

EoDlgFileManageBlocks::EoDlgFileManageBlocks(CWnd* parent /*=nullptr*/) : CDialog(EoDlgFileManageBlocks::IDD, parent) {}
EoDlgFileManageBlocks::EoDlgFileManageBlocks(AeSysDoc* document, CWnd* parent /*=nullptr*/)
    : CDialog(EoDlgFileManageBlocks::IDD, parent), m_document(document) {}
EoDlgFileManageBlocks::~EoDlgFileManageBlocks() {}

void EoDlgFileManageBlocks::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_BLOCKS_LIST, m_blocksList);
  DDX_Control(dataExchange, IDC_REFERENCES, m_references);
}

BOOL EoDlgFileManageBlocks::OnInitDialog() {
  CDialog::OnInitDialog();

  if (!m_document) {
    AfxMessageBox(L"Document pointer is required.", MB_ICONERROR);
    EndDialog(IDCANCEL);
    return FALSE;
  }
  ASSERT(m_document != nullptr);

  CString captionText;
  GetWindowTextW(captionText);
  captionText += L" - ";
  captionText += m_document->GetPathName();
  SetWindowTextW(captionText);

  auto* previewWindow = GetDlgItem(IDC_BLOCK_PREVIEW);
  m_previewWindowHandle = previewWindow ? previewWindow->GetSafeHwnd() : nullptr;
  if (!m_previewWindowHandle) {
    AfxMessageBox(L"Preview control (IDC_BLOCK_PREVIEW) is missing from the dialog resource.", MB_ICONERROR);
    EndDialog(IDCANCEL);
    return FALSE;
  }
  m_blocksList.SetHorizontalExtent(512);

  CString blockName;
  EoDbBlock* block{};

  auto position = m_document->GetFirstBlockPosition();
  while (position != nullptr) {
    m_document->GetNextBlock(position, blockName, block);
    if (block == nullptr) { continue; }
    if (block->IsAnonymous() || block->IsSystemBlock(blockName)) { continue; }
    if (block->IsModelSpace(blockName.GetString()) || block->IsPaperSpace(blockName.GetString())) { continue; }
    auto itemIndex = m_blocksList.AddString(blockName);
    m_blocksList.SetItemData(itemIndex, DWORD_PTR(block));
  }
  WndProcPreviewClear(m_previewWindowHandle);

  if (m_blocksList.GetCount() > 0) {
    m_blocksList.SetCurSel(0);
    OnLbnSelchangeBlocksList();
  }
  return TRUE;
}

void EoDlgFileManageBlocks::OnLbnSelchangeBlocksList() {
  auto currentSelection = m_blocksList.GetCurSel();
  if (currentSelection == LB_ERR) { return; }
  
  if (m_blocksList.GetTextLen(currentSelection) == LB_ERR) { return; }
  
  CString blockName;
  m_blocksList.GetText(currentSelection, blockName);
  auto* block = reinterpret_cast<EoDbBlock*>(m_blocksList.GetItemData(currentSelection));

  const auto referenceCount = m_document->GetBlockReferenceCount(blockName);
  SetDlgItemInt(IDC_REFERENCES, static_cast<UINT>(referenceCount), FALSE);
  WndProcPreviewUpdateBlock(m_previewWindowHandle, block);
}