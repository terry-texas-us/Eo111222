#include "Stdafx.h"

#include <format>

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDlgTextStyleManager.h"
#include "EoGsRenderState.h"

namespace {
constexpr int kHeightColumn = 2;
constexpr int kWidthFactorColumn = 3;
constexpr UINT kInPlaceEditId = 2001;
}  // namespace

IMPLEMENT_DYNAMIC(EoDlgTextStyleManager, CDialog)

EoDlgTextStyleManager::EoDlgTextStyleManager(CWnd* parent)
    : CDialog(IDD, parent) {}

void EoDlgTextStyleManager::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_TEXTSTYLE_LIST, m_styleList);
}

BEGIN_MESSAGE_MAP(EoDlgTextStyleManager, CDialog)
ON_NOTIFY(NM_DBLCLK, IDC_TEXTSTYLE_LIST, &EoDlgTextStyleManager::OnDoubleClickStyleList)
END_MESSAGE_MAP()

BOOL EoDlgTextStyleManager::OnInitDialog() {
  CDialog::OnInitDialog();

  m_styleList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

  m_styleList.InsertColumn(0, L"Name", LVCFMT_LEFT, 100);
  m_styleList.InsertColumn(1, L"Font", LVCFMT_LEFT, 80);
  m_styleList.InsertColumn(2, L"Height", LVCFMT_RIGHT, 70);
  m_styleList.InsertColumn(3, L"Width Factor", LVCFMT_RIGHT, 80);
  m_styleList.InsertColumn(4, L"Oblique Angle", LVCFMT_RIGHT, 80);

  PopulateStyleList();

  return TRUE;
}

void EoDlgTextStyleManager::PopulateStyleList() {
  m_styleList.DeleteAllItems();

  auto* document = AeSysDoc::GetDoc();
  if (document == nullptr) { return; }

  const auto& textStyleTable = document->TextStyleTable();
  int row = 0;
  for (const auto& style : textStyleTable) {
    // Skip shape file entries (flag 0x01)
    if (style.m_flagValues & 0x01) { continue; }

    m_styleList.InsertItem(row, style.m_name.c_str());
    m_styleList.SetItemText(row, 1, style.m_font.c_str());

    auto heightText = std::format(L"{:.4f}", style.m_height);
    m_styleList.SetItemText(row, 2, heightText.c_str());

    auto widthText = std::format(L"{:.4f}", style.m_widthFactor);
    m_styleList.SetItemText(row, 3, widthText.c_str());

    // Display oblique angle in degrees for readability
    auto obliqueAngleDegrees = style.m_obliqueAngle * 180.0 / Eo::Pi;
    auto obliqueText = std::format(L"{:.2f}\u00B0", obliqueAngleDegrees);
    m_styleList.SetItemText(row, 4, obliqueText.c_str());

    ++row;
  }
}

void EoDlgTextStyleManager::OnDoubleClickStyleList(NMHDR* notifyMessageHeader, LRESULT* result) {
  *result = 0;

  auto* info = reinterpret_cast<NMITEMACTIVATE*>(notifyMessageHeader);
  if (info->iItem < 0) { return; }

  // Determine which sub-item (column) was clicked
  LVHITTESTINFO hitTest{};
  hitTest.pt = info->ptAction;
  m_styleList.SubItemHitTest(&hitTest);

  if (hitTest.iItem < 0) { return; }
  if (hitTest.iSubItem != kHeightColumn && hitTest.iSubItem != kWidthFactorColumn) { return; }

  BeginInPlaceEdit(hitTest.iItem, hitTest.iSubItem);
}

void EoDlgTextStyleManager::BeginInPlaceEdit(int row, int column) {
  // Cancel any existing edit
  CancelInPlaceEdit();

  CRect subItemRect;
  m_styleList.GetSubItemRect(row, column, LVIR_LABEL, subItemRect);

  m_editingRow = row;
  m_editingColumn = column;

  const CString currentText = m_styleList.GetItemText(row, column);

  if (!m_inPlaceEdit.GetSafeHwnd()) {
    m_inPlaceEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_RIGHT, subItemRect, &m_styleList,
        kInPlaceEditId);
    m_inPlaceEdit.SetFont(m_styleList.GetFont());
  } else {
    m_inPlaceEdit.MoveWindow(&subItemRect);
    m_inPlaceEdit.ShowWindow(SW_SHOW);
  }

  m_inPlaceEdit.SetWindowTextW(currentText);
  m_inPlaceEdit.SetSel(0, -1);
  m_inPlaceEdit.SetFocus();
}

void EoDlgTextStyleManager::CommitInPlaceEdit() {
  if (m_editingRow < 0 || !m_inPlaceEdit.GetSafeHwnd() || !m_inPlaceEdit.IsWindowVisible()) { return; }

  CString editText;
  m_inPlaceEdit.GetWindowTextW(editText);
  m_inPlaceEdit.ShowWindow(SW_HIDE);

  double newValue = _wtof(editText);
  const int editingColumn = m_editingColumn;
  const int editingRow = m_editingRow;
  m_editingRow = -1;
  m_editingColumn = -1;

  if (editingColumn == kHeightColumn) {
    if (newValue < 0.0) { newValue = 0.0; }
  } else if (editingColumn == kWidthFactorColumn) {
    if (newValue <= 0.0) { newValue = 1.0; }  // width factor must be positive
  }

  // Update the list display
  const auto formattedText = std::format(L"{:.4f}", newValue);
  m_styleList.SetItemText(editingRow, editingColumn, formattedText.c_str());

  // Find the corresponding style in the document and update it.
  auto* document = AeSysDoc::GetDoc();
  if (document != nullptr) {
    const CString styleName = m_styleList.GetItemText(editingRow, 0);
    auto& textStyleTable = document->TextStyleTable();
    for (auto& style : textStyleTable) {
      if (style.m_flagValues & 0x01) { continue; }
      if (_wcsicmp(style.m_name.c_str(), styleName) == 0) {
        if (editingColumn == kHeightColumn) {
          style.m_height = newValue;
          // Sync render state height if this is the active text style
          if (_wcsicmp(Gs::renderState.TextStyleName().c_str(), styleName) == 0 && newValue > 0.0) {
            auto characterCellDefinition = Gs::renderState.CharacterCellDefinition();
            characterCellDefinition.SetHeight(newValue);
            Gs::renderState.SetCharacterCellDefinition(characterCellDefinition);
          }
        } else if (editingColumn == kWidthFactorColumn) {
          style.m_widthFactor = newValue;
        }
        document->SetModifiedFlag(TRUE);
        break;
      }
    }
  }
}

void EoDlgTextStyleManager::CancelInPlaceEdit() {
  if (m_inPlaceEdit.GetSafeHwnd() && m_inPlaceEdit.IsWindowVisible()) { m_inPlaceEdit.ShowWindow(SW_HIDE); }
  m_editingRow = -1;
  m_editingColumn = -1;
}

void EoDlgTextStyleManager::OnEndEditKillFocus() { CommitInPlaceEdit(); }

BOOL EoDlgTextStyleManager::PreTranslateMessage(MSG* msg) {
  if (m_inPlaceEdit.GetSafeHwnd() && m_inPlaceEdit.IsWindowVisible()) {
    if (msg->message == WM_KEYDOWN) {
      if (msg->wParam == VK_RETURN) {
        CommitInPlaceEdit();
        return TRUE;
      }
      if (msg->wParam == VK_ESCAPE) {
        CancelInPlaceEdit();
        return TRUE;
      }
    }
    if (msg->message == WM_KILLFOCUS || (msg->message == WM_LBUTTONDOWN && msg->hwnd != m_inPlaceEdit.m_hWnd)) {
      CommitInPlaceEdit();
    }
  }
  return CDialog::PreTranslateMessage(msg);
}
