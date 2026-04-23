#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDlgBlocks.h"
#include "Resource.h"
#include "WndProcPreview.h"

namespace {
constexpr UINT kContextMenuRename   = 1;
constexpr UINT kContextMenuEditBlock = 2;
}  // namespace

BEGIN_MESSAGE_MAP(EoDlgBlocks, CDialog)
ON_LBN_SELCHANGE(IDC_BLOCKS_LIST, &EoDlgBlocks::OnLbnSelchangeBlocksList)
ON_LBN_DBLCLK(IDC_BLOCKS_LIST, &EoDlgBlocks::BeginInPlaceRename)
ON_BN_CLICKED(IDC_EDIT_BLOCK, &EoDlgBlocks::OnBnClickedEditBlock)
ON_BN_CLICKED(IDC_PURGE_UNUSED, &EoDlgBlocks::OnBnClickedPurge)
ON_UPDATE_COMMAND_UI(IDC_EDIT_BLOCK, &EoDlgBlocks::OnUpdateEditBlock)
ON_EN_KILLFOCUS(IDC_BASE_POINT_X, &EoDlgBlocks::OnEnKillfocusBasePoint)
ON_EN_KILLFOCUS(IDC_BASE_POINT_Y, &EoDlgBlocks::OnEnKillfocusBasePoint)
ON_EN_KILLFOCUS(IDC_BASE_POINT_Z, &EoDlgBlocks::OnEnKillfocusBasePoint)
ON_WM_CONTEXTMENU()
ON_EN_KILLFOCUS(IDC_BLOCKS_RENAME_EDIT, &EoDlgBlocks::OnEnKillfocusInPlaceEdit)
END_MESSAGE_MAP()

EoDlgBlocks::EoDlgBlocks(AeSysDoc* document, CWnd* parent)
    : CDialog(EoDlgBlocks::IDD, parent), m_document(document) {}

EoDlgBlocks::~EoDlgBlocks() {}

void EoDlgBlocks::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_BLOCKS_LIST, m_blocksList);
}

BOOL EoDlgBlocks::OnInitDialog() {
  CDialog::OnInitDialog();

  ASSERT(m_document != nullptr);

  // Append document path to caption
  CString captionText;
  GetWindowTextW(captionText);
  captionText += L" - ";
  captionText += m_document->GetPathName();
  SetWindowTextW(captionText);

  const auto* previewWindow = GetDlgItem(IDC_BLOCK_PREVIEW);
  m_previewWindowHandle = previewWindow ? previewWindow->GetSafeHwnd() : nullptr;

  m_blocksList.SetHorizontalExtent(512);

  CString blockName;
  EoDbBlock* block{};

  auto position = m_document->GetFirstBlockPosition();
  while (position != nullptr) {
    m_document->GetNextBlock(position, blockName, block);
    if (block == nullptr) { continue; }
    if (block->IsAnonymous() || block->IsSystemBlock(blockName)) { continue; }
    if (block->IsModelSpace(blockName.GetString()) || block->IsPaperSpace(blockName.GetString())) { continue; }
    const auto itemIndex = m_blocksList.AddString(blockName);
    m_blocksList.SetItemData(itemIndex, DWORD_PTR(block));
  }

  if (m_previewWindowHandle) { WndProcPreviewClear(m_previewWindowHandle); }

  // Disable "Edit Block..." when already in an editor session
  if (GetDlgItem(IDC_EDIT_BLOCK)) {
    GetDlgItem(IDC_EDIT_BLOCK)->EnableWindow(!m_document->IsInEditor());
  }

  if (m_blocksList.GetCount() > 0) {
    m_blocksList.SetCurSel(0);
    UpdateSelectionInfo(0);
  }
  CenterWindow(AfxGetMainWnd());
  return TRUE;
}

void EoDlgBlocks::UpdateSelectionInfo(int listIndex) {
  if (listIndex == LB_ERR) {
    SetDlgItemText(IDC_GROUPS, L"0");
    SetDlgItemText(IDC_REFERENCES, L"0");
    SetDlgItemText(IDC_BASE_POINT_X, L"0");
    SetDlgItemText(IDC_BASE_POINT_Y, L"0");
    SetDlgItemText(IDC_BASE_POINT_Z, L"0");
    if (m_previewWindowHandle) { WndProcPreviewClear(m_previewWindowHandle); }
    return;
  }

  CString blockName;
  m_blocksList.GetText(listIndex, blockName);
  auto* block = reinterpret_cast<EoDbBlock*>(m_blocksList.GetItemData(listIndex));
  if (!block) { return; }

  SetDlgItemInt(IDC_GROUPS, static_cast<UINT>(block->GetCount()), FALSE);
  SetDlgItemInt(IDC_REFERENCES, static_cast<UINT>(m_document->GetBlockReferenceCount(blockName)), FALSE);

  const auto basePoint = block->BasePoint();
  CString xText;
  xText.Format(L"%.4f", basePoint.x);
  SetDlgItemText(IDC_BASE_POINT_X, xText);
  CString yText;
  yText.Format(L"%.4f", basePoint.y);
  SetDlgItemText(IDC_BASE_POINT_Y, yText);
  CString zText;
  zText.Format(L"%.4f", basePoint.z);
  SetDlgItemText(IDC_BASE_POINT_Z, zText);

  if (m_previewWindowHandle) { WndProcPreviewUpdateBlock(m_previewWindowHandle, block); }
}

void EoDlgBlocks::OnLbnSelchangeBlocksList() {
  UpdateSelectionInfo(m_blocksList.GetCurSel());
}

void EoDlgBlocks::OnBnClickedEditBlock() {
  if (m_document->IsInEditor()) {
    app.AddStringToMessageList(L"Already in an editor session.");
    return;
  }
  const auto selectedIndex = m_blocksList.GetCurSel();
  if (selectedIndex == LB_ERR) { return; }

  CString blockName;
  m_blocksList.GetText(selectedIndex, blockName);

  EndDialog(IDOK);

  if (!m_document->EnterBlockEditMode(blockName)) {
    app.AddStringToMessageList(L"Failed to enter block edit mode.");
  }
}

void EoDlgBlocks::OnUpdateEditBlock(CCmdUI* cmdUI) {
  const auto selectedIndex = m_blocksList.GetCurSel();
  cmdUI->Enable(!m_document->IsInEditor() && selectedIndex != LB_ERR);
}

void EoDlgBlocks::OnEnKillfocusBasePoint() {
  const auto selectedIndex = m_blocksList.GetCurSel();
  if (selectedIndex == LB_ERR) { return; }

  auto* block = reinterpret_cast<EoDbBlock*>(m_blocksList.GetItemData(selectedIndex));
  if (!block) { return; }

  CString xText;
  CString yText;
  CString zText;
  GetDlgItemText(IDC_BASE_POINT_X, xText);
  GetDlgItemText(IDC_BASE_POINT_Y, yText);
  GetDlgItemText(IDC_BASE_POINT_Z, zText);

  const auto newX = _wtof(xText);
  const auto newY = _wtof(yText);
  const auto newZ = _wtof(zText);
  const auto currentBase = block->BasePoint();

  if (newX == currentBase.x && newY == currentBase.y && newZ == currentBase.z) { return; }

  block->SetBasePoint(EoGePoint3d(newX, newY, newZ));
  m_document->SetModifiedFlag();
}

// --- "Purge Unused" button ---

void EoDlgBlocks::OnBnClickedPurge() {
  const auto unusedCount = m_document->GetUnusedBlockCount();
  if (unusedCount == 0) {
    AfxMessageBox(L"All block definitions are referenced. Nothing to purge.", MB_OK | MB_ICONINFORMATION);
    return;
  }
  CString message;
  message.Format(L"Remove %d unreferenced block definition%s?", unusedCount, unusedCount == 1 ? L"" : L"s");
  if (AfxMessageBox(message, MB_YESNO | MB_ICONQUESTION) != IDYES) { return; }

  m_document->RemoveUnusedBlocks();
  m_document->SetModifiedFlag();

  // Repopulate the list — deleted blocks are gone, selections cleared
  const auto currentSel = m_blocksList.GetCurSel();
  CString selectedName;
  if (currentSel != LB_ERR) { m_blocksList.GetText(currentSel, selectedName); }

  m_blocksList.ResetContent();
  CString name;
  EoDbBlock* block{};
  auto position = m_document->GetFirstBlockPosition();
  while (position != nullptr) {
    m_document->GetNextBlock(position, name, block);
    if (block == nullptr) { continue; }
    if (block->IsAnonymous() || block->IsSystemBlock(name)) { continue; }
    if (block->IsModelSpace(name.GetString()) || block->IsPaperSpace(name.GetString())) { continue; }
    const auto index = m_blocksList.AddString(name);
    m_blocksList.SetItemDataPtr(index, block);
  }

  // Restore selection if the block still exists, otherwise clear info
  const auto newIndex = selectedName.IsEmpty() ? LB_ERR : m_blocksList.FindStringExact(-1, selectedName);
  if (newIndex != LB_ERR) {
    m_blocksList.SetCurSel(newIndex);
    UpdateSelectionInfo(newIndex);
  } else {
    UpdateSelectionInfo(LB_ERR);
  }
}

// --- In-place rename ---

void EoDlgBlocks::BeginInPlaceRename() {
  const auto selectedIndex = m_blocksList.GetCurSel();
  if (selectedIndex == LB_ERR) { return; }

  CString currentName;
  m_blocksList.GetText(selectedIndex, currentName);

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

void EoDlgBlocks::CommitRename() {
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

  m_blocksList.DeleteString(oldIndex);
  const auto newIndex = m_blocksList.AddString(newName);
  m_blocksList.SetItemData(newIndex, DWORD_PTR(blockPtr));
  m_blocksList.SetCurSel(newIndex);
  UpdateSelectionInfo(newIndex);
}

void EoDlgBlocks::CancelRename() {
  m_renamingIndex = LB_ERR;
  if (m_inPlaceEdit.GetSafeHwnd()) { m_inPlaceEdit.ShowWindow(SW_HIDE); }
  m_blocksList.SetFocus();
}

BOOL EoDlgBlocks::PreTranslateMessage(MSG* message) {
  if (message->message == WM_KEYDOWN && message->hwnd == m_blocksList.GetSafeHwnd()) {
    if (message->wParam == VK_F2) {
      BeginInPlaceRename();
      return TRUE;
    }
    if (message->wParam == VK_RETURN && !m_document->IsInEditor()) {
      OnBnClickedEditBlock();
      return TRUE;
    }
  }
  if (m_inPlaceEdit.GetSafeHwnd() && m_inPlaceEdit.IsWindowVisible()) {
    if (message->message == WM_KEYDOWN && message->hwnd == m_inPlaceEdit.GetSafeHwnd()) {
      if (message->wParam == VK_RETURN) { CommitRename(); return TRUE; }
      if (message->wParam == VK_ESCAPE) { CancelRename(); return TRUE; }
    }
  }
  return CDialog::PreTranslateMessage(message);
}

void EoDlgBlocks::OnContextMenu(CWnd* window, CPoint point) {
  if (window->GetSafeHwnd() != m_blocksList.GetSafeHwnd()) {
    CDialog::OnContextMenu(window, point);
    return;
  }

  CPoint clientPoint = point;
  m_blocksList.ScreenToClient(&clientPoint);
  BOOL outsideClient = FALSE;
  const auto hitIndex = m_blocksList.ItemFromPoint(clientPoint, outsideClient);
  if (!outsideClient && hitIndex != LB_ERR) { m_blocksList.SetCurSel(hitIndex); }

  const auto selectedIndex = m_blocksList.GetCurSel();
  if (selectedIndex == LB_ERR) { return; }

  const bool canEdit = !m_document->IsInEditor();

  CMenu popupMenu;
  popupMenu.CreatePopupMenu();
  popupMenu.AppendMenu(MF_STRING | (canEdit ? MF_ENABLED : MF_GRAYED), kContextMenuEditBlock, L"Edit Block...\tEnter");
  popupMenu.AppendMenu(MF_SEPARATOR);
  popupMenu.AppendMenu(MF_STRING, kContextMenuRename, L"Rename\tF2");

  const auto commandId = static_cast<UINT>(popupMenu.TrackPopupMenu(
      TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, point.x, point.y, this));

  if (commandId == kContextMenuRename) { BeginInPlaceRename(); }
  else if (commandId == kContextMenuEditBlock) { OnBnClickedEditBlock(); }
}

void EoDlgBlocks::OnEnKillfocusInPlaceEdit() {
  if (m_renamingIndex != LB_ERR) { CommitRename(); }
}
