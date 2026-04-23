#include "Stdafx.h"

#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDlgFileManageBlocks.h"
#include "Resource.h"
#include "WndProcPreview.h"

// Context menu command ID (local to this translation unit)
namespace {
constexpr UINT kContextMenuRename = 1;
}

BEGIN_MESSAGE_MAP(EoDlgFileManageBlocks, CDialog)
ON_LBN_SELCHANGE(IDC_BLOCKS_LIST, &EoDlgFileManageBlocks::OnLbnSelchangeBlocksList)
ON_LBN_DBLCLK(IDC_BLOCKS_LIST, &EoDlgFileManageBlocks::BeginInPlaceRename)
ON_WM_CONTEXTMENU()
ON_EN_KILLFOCUS(IDC_BLOCKS_RENAME_EDIT, &EoDlgFileManageBlocks::OnEnKillfocusInPlaceEdit)
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

void EoDlgFileManageBlocks::BeginInPlaceRename() {
  const auto selectedIndex = m_blocksList.GetCurSel();
  if (selectedIndex == LB_ERR) { return; }

  CString currentName;
  m_blocksList.GetText(selectedIndex, currentName);

  // Position a CEdit control over the selected list item rectangle
  CRect itemRect;
  m_blocksList.GetItemRect(selectedIndex, &itemRect);
  m_blocksList.ClientToScreen(&itemRect);
  ScreenToClient(&itemRect);

  if (!m_inPlaceEdit.GetSafeHwnd()) {
    m_inPlaceEdit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, itemRect, this, IDC_BLOCKS_RENAME_EDIT);
    m_inPlaceEdit.SetFont(m_blocksList.GetFont());
  } else {
    m_inPlaceEdit.MoveWindow(&itemRect);
  }

  m_renamingIndex = selectedIndex;
  m_inPlaceEdit.SetWindowText(currentName);
  m_inPlaceEdit.SetSel(0, -1);
  m_inPlaceEdit.ShowWindow(SW_SHOW);
  m_inPlaceEdit.SetFocus();
}

void EoDlgFileManageBlocks::CommitRename() {
  if (m_renamingIndex == LB_ERR || !m_inPlaceEdit.GetSafeHwnd()) { return; }

  m_inPlaceEdit.ShowWindow(SW_HIDE);

  CString oldName;
  m_blocksList.GetText(m_renamingIndex, oldName);

  CString newName;
  m_inPlaceEdit.GetWindowText(newName);
  newName.Trim();

  const auto oldIndex = m_renamingIndex;
  m_renamingIndex = LB_ERR;

  if (newName.IsEmpty() || newName == oldName) { return; }

  auto* blockPtr = reinterpret_cast<EoDbBlock*>(m_blocksList.GetItemData(oldIndex));

  if (!m_document->RenameBlock(oldName, newName)) {
    CString message;
    message.Format(L"Cannot rename '%s' to '%s'. The name may already be in use.", oldName.GetString(), newName.GetString());
    AfxMessageBox(message, MB_ICONWARNING);
    return;
  }

  // Refresh the list entry: remove old string, insert new one, restore item data and selection
  m_blocksList.DeleteString(oldIndex);
  const auto newIndex = m_blocksList.AddString(newName);
  m_blocksList.SetItemData(newIndex, DWORD_PTR(blockPtr));
  m_blocksList.SetCurSel(newIndex);
  OnLbnSelchangeBlocksList();
}

void EoDlgFileManageBlocks::CancelRename() {
  m_renamingIndex = LB_ERR;
  if (m_inPlaceEdit.GetSafeHwnd()) { m_inPlaceEdit.ShowWindow(SW_HIDE); }
  m_blocksList.SetFocus();
}

BOOL EoDlgFileManageBlocks::PreTranslateMessage(MSG* message) {
  // F2 on the list box starts rename
  if (message->message == WM_KEYDOWN && message->hwnd == m_blocksList.GetSafeHwnd()) {
    if (message->wParam == VK_F2) {
      BeginInPlaceRename();
      return TRUE;
    }
  }
  // Enter commits; Escape cancels the in-place edit
  if (m_inPlaceEdit.GetSafeHwnd() && m_inPlaceEdit.IsWindowVisible()) {
    if (message->message == WM_KEYDOWN && message->hwnd == m_inPlaceEdit.GetSafeHwnd()) {
      if (message->wParam == VK_RETURN) {
        CommitRename();
        return TRUE;
      }
      if (message->wParam == VK_ESCAPE) {
        CancelRename();
        return TRUE;
      }
    }
  }
  return CDialog::PreTranslateMessage(message);
}

void EoDlgFileManageBlocks::OnContextMenu(CWnd* window, CPoint point) {
  // Only show context menu for clicks on the list box
  if (window->GetSafeHwnd() != m_blocksList.GetSafeHwnd()) {
    CDialog::OnContextMenu(window, point);
    return;
  }

  // Hit-test to select the item under the cursor before showing the menu
  CPoint clientPoint = point;
  m_blocksList.ScreenToClient(&clientPoint);
  BOOL outsideClient = FALSE;
  const auto hitIndex = m_blocksList.ItemFromPoint(clientPoint, outsideClient);
  if (!outsideClient && hitIndex != LB_ERR) { m_blocksList.SetCurSel(hitIndex); }

  const auto selectedIndex = m_blocksList.GetCurSel();
  if (selectedIndex == LB_ERR) { return; }

  CMenu popupMenu;
  popupMenu.CreatePopupMenu();
  popupMenu.AppendMenu(MF_STRING, kContextMenuRename, L"Rename\tF2");

  const auto commandId = static_cast<UINT>(popupMenu.TrackPopupMenu(
      TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, point.x, point.y, this));

  if (commandId == kContextMenuRename) { BeginInPlaceRename(); }
}

void EoDlgFileManageBlocks::OnEnKillfocusInPlaceEdit() {
  // Focus leaving the edit — commit whatever is there (matches Explorer behaviour)
  if (m_renamingIndex != LB_ERR) { CommitRename(); }
}

void EoDlgFileManageBlocks::OnEnReturnInPlaceEdit() {
  CommitRename();
}
