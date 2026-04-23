#include "Stdafx.h"

#include <uxtheme.h>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoCtrlTextStyleComboBox.h"
#include "EoDlgTextStyleManager.h"
#include "EoGsRenderState.h"

IMPLEMENT_SERIAL(EoCtrlTextStyleComboBox, CMFCToolBarComboBoxButton, VERSIONABLE_SCHEMA | 1)

EoCtrlTextStyleComboBox::EoCtrlTextStyleComboBox()
    : CMFCToolBarComboBoxButton(ID_TEXTSTYLE_COMBO, -1, CBS_DROPDOWNLIST | CBS_HASSTRINGS, 168) {
  PopulateItems();
}

void EoCtrlTextStyleComboBox::PopulateItems() {
  BuildItemList();

  // Select "Standard" as the default
  for (int i = 0; i < GetCount(); i++) {
    LPCTSTR text = GetItem(i);
    if (text != nullptr && _wcsicmp(text, L"Standard") == 0) {
      SelectItem(i);
      return;
    }
  }
  if (GetCount() > 0) { SelectItem(0); }
}

void EoCtrlTextStyleComboBox::SetCurrentTextStyle(const std::wstring& textStyleName) {
  for (int i = 0; i < GetCount(); i++) {
    LPCTSTR text = GetItem(i);
    if (text != nullptr && _wcsicmp(text, textStyleName.c_str()) == 0) {
      SelectItem(i);
      return;
    }
  }
  // Rebuild to pick up any new styles, then retry
  BuildItemList();
  for (int i = 0; i < GetCount(); i++) {
    LPCTSTR text = GetItem(i);
    if (text != nullptr && _wcsicmp(text, textStyleName.c_str()) == 0) {
      SelectItem(i);
      return;
    }
  }
  if (GetCount() > 0) { SelectItem(0); }
}

void EoCtrlTextStyleComboBox::BuildItemList() {
  RemoveAllItems();

  auto* document = AeSysDoc::GetDoc();
  if (document == nullptr) {
    AddItem(L"Standard");
    return;
  }

  const auto& textStyleTable = document->TextStyleTable();
  if (textStyleTable.empty()) {
    AddItem(L"Standard");
    return;
  }

  for (const auto& style : textStyleTable) {
    // Skip shape file entries (flag 0x01)
    if (style.m_flagValues & 0x01) { continue; }
    AddItem(style.m_name.c_str());
  }

  if (GetCount() == 0) { AddItem(L"Standard"); }
}

void EoCtrlTextStyleComboBox::Serialize(CArchive& ar) {
  // Bypass CMFCToolBarComboBoxButton::Serialize — same pattern as color/linetype/lineweight combos.
  CMFCToolBarButton::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_iWidth;
    ar << static_cast<DWORD>(m_dwStyle);

    // Save the current text style name — items are rebuilt from the document on load.
    CString currentName;
    int curSel = CMFCToolBarComboBoxButton::GetCurSel();
    if (curSel >= 0) {
      LPCTSTR text = CMFCToolBarComboBoxButton::GetItem(curSel);
      if (text != nullptr) { currentName = text; }
    }
    ar << currentName;
  } else {
    ar >> m_iWidth;
    DWORD dwStyle;
    ar >> dwStyle;
    m_dwStyle = dwStyle;

    CString savedName;
    ar >> savedName;

    // Rebuild items from the document and select the saved name.
    BuildItemList();
    bool found = false;
    if (!savedName.IsEmpty()) {
      for (int i = 0; i < GetCount(); i++) {
        LPCTSTR text = GetItem(i);
        if (text != nullptr && savedName.CompareNoCase(text) == 0) {
          SelectItem(i);
          found = true;
          break;
        }
      }
    }
    if (!found && GetCount() > 0) { SelectItem(0); }
  }
}

CComboBox* EoCtrlTextStyleComboBox::CreateCombo(CWnd* parentWindow, const CRect& rect) {
  auto* combo = new EoCtrlTextStyleThemedCombo;
  if (!combo->Create(m_dwStyle | WS_CHILD | WS_VISIBLE | WS_VSCROLL, rect, parentWindow, m_nID)) {
    delete combo;
    return nullptr;
  }
  combo->ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
  combo->ModifyStyle(WS_BORDER, 0);
  combo->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
  ::SetWindowTheme(combo->m_hWnd, L"", L"");
  return combo;
}

BOOL EoCtrlTextStyleComboBox::NotifyCommand(int notifyCode) {
  if (notifyCode == CBN_SELCHANGE) {
    OnSelectionChanged();
    return TRUE;
  }
  return CMFCToolBarComboBoxButton::NotifyCommand(notifyCode);
}

void EoCtrlTextStyleComboBox::OnSelectionChanged() {
  const int selectedIndex = GetCurSel();
  if (selectedIndex < 0) { return; }

  const LPCTSTR selectedName = GetItem(selectedIndex);
  if (selectedName == nullptr) { return; }

  auto* document = AeSysDoc::GetDoc();
  if (document == nullptr) { return; }

  const auto* style = document->FindTextStyle(selectedName);
  if (style == nullptr) { return; }

  // Track the active text style name in the render state
  Gs::renderState.SetTextStyleName(selectedName);

  // Apply the text style's font to the render state font definition
  EoDbFontDefinition fontDefinition = Gs::renderState.FontDefinition();
  fontDefinition.SetFontName(style->m_font);
  // Note: SetFontDefinition requires a device context, but we don't have one here.
  // The font definition update will take effect on the next render cycle.
  auto* view = AeSysView::GetActiveView();
  if (view != nullptr) {
    auto* deviceContext = view->GetDC();
    if (deviceContext != nullptr) {
      Gs::renderState.SetFontDefinition(deviceContext, fontDefinition);
      view->ReleaseDC(deviceContext);
    }
  }

  // Apply text style height if fixed (non-zero)
  if (style->m_height > 0.0) {
    auto characterCellDefinition = Gs::renderState.CharacterCellDefinition();
    characterCellDefinition.SetHeight(style->m_height);
    Gs::renderState.SetCharacterCellDefinition(characterCellDefinition);
  }
}

void EoCtrlTextStyleComboBox::OnMove() {
  CMFCToolBarComboBoxButton::OnMove();

  // Offset the CComboBox HWND rightward to leave room for the icon area.
  if (m_pWndCombo != nullptr && m_pWndCombo->GetSafeHwnd() != nullptr) {
    CRect comboRect;
    m_pWndCombo->GetWindowRect(&comboRect);
    m_pWndCombo->GetParent()->ScreenToClient(&comboRect);
    comboRect.left += kIconAreaWidth;
    m_pWndCombo->MoveWindow(comboRect);
  }
}

BOOL EoCtrlTextStyleComboBox::OnClick(CWnd* parentWindow, BOOL delay) {
  // Check if the click is in the icon area (left of the CComboBox HWND).
  const auto messagePos = ::GetMessagePos();
  const CPoint screenPoint(GET_X_LPARAM(messagePos), GET_Y_LPARAM(messagePos));
  CPoint clientPoint = screenPoint;
  if (parentWindow != nullptr) { parentWindow->ScreenToClient(&clientPoint); }

  int iconRight = m_rectCombo.left + kIconAreaWidth;
  if (clientPoint.x >= m_rectCombo.left && clientPoint.x < iconRight) {
    OpenTextStyleManager();
    return TRUE;
  }
  return CMFCToolBarComboBoxButton::OnClick(parentWindow, delay);
}

void EoCtrlTextStyleComboBox::DrawTextStyleIcon(
    CDC* deviceContext, const CRect& iconRect, COLORREF /*textColor*/) {
  // Draw the text style icon bitmap centered in the icon area.
  CBitmap bitmap;
  if (!bitmap.LoadBitmap(IDB_TEXTSTYLE_EDIT)) { return; }

  BITMAP bmpInfo{};
  bitmap.GetBitmap(&bmpInfo);

  const int destWidth = std::min(static_cast<int>(bmpInfo.bmWidth), static_cast<int>(iconRect.Width()));
  const int destHeight = std::min(static_cast<int>(bmpInfo.bmHeight), static_cast<int>(iconRect.Height()));
  const int destX = iconRect.left + (iconRect.Width() - destWidth) / 2;
  const int destY = iconRect.top + (iconRect.Height() - destHeight) / 2;

  CDC memoryDC;
  memoryDC.CreateCompatibleDC(deviceContext);
  CBitmap* oldBitmap = memoryDC.SelectObject(&bitmap);
  deviceContext->BitBlt(destX, destY, destWidth, destHeight, &memoryDC, 0, 0, SRCCOPY);
  memoryDC.SelectObject(oldBitmap);
}

void EoCtrlTextStyleComboBox::OpenTextStyleManager() {
  auto* mainFrame = AfxGetMainWnd();
  EoDlgTextStyleManager dialog(mainFrame);
  dialog.DoModal();
  PopulateItems();
}

void EoCtrlTextStyleComboBox::OnDraw(CDC* deviceContext, const CRect& rect, CMFCToolBarImages* images, BOOL isHorz,
    BOOL isCustomizeMode, BOOL isHighlighted, BOOL drawBorder, BOOL grayDisabledButtons) {
  if (m_pWndCombo == nullptr || m_pWndCombo->GetSafeHwnd() == nullptr || !isHorz) {
    CMFCToolBarButton::OnDraw(
        deviceContext, rect, images, isHorz, isCustomizeMode, isHighlighted, drawBorder, grayDisabledButtons);
    return;
  }

  const BOOL isDisabled = (isCustomizeMode && !IsEditable()) || (!isCustomizeMode && (m_nStyle & TBBS_DISABLED));

    if (m_bFlat) {
    if (m_bIsHotEdit) { isHighlighted = TRUE; }

    CRect rectCombo = m_rectCombo;
    CMFCVisualManager::GetInstance()->OnDrawComboBorder(
        deviceContext, rectCombo, isDisabled, m_pWndCombo->GetDroppedState(), isHighlighted, this);

    rectCombo.DeflateRect(2, 2);
    const auto& schemeColors = Eo::chromeColors;
    deviceContext->FillSolidRect(rectCombo, schemeColors.paneBackground);

    // Draw icon area with separator line
    const CRect iconRect(rectCombo.left, rectCombo.top, rectCombo.left + kIconAreaWidth - 2, rectCombo.bottom);
    DrawTextStyleIcon(deviceContext, iconRect, schemeColors.menuText);

    // Subtle vertical separator between icon and combo content
    const int separatorX = rectCombo.left + kIconAreaWidth - 2;
    CPen separatorPen(PS_SOLID, 1, schemeColors.borderColor);
    CPen* oldPen = deviceContext->SelectObject(&separatorPen);
    deviceContext->MoveTo(separatorX, rectCombo.top + 2);
    deviceContext->LineTo(separatorX, rectCombo.bottom - 2);
    deviceContext->SelectObject(oldPen);

    CRect rectButton = m_rectButton;
    if (GetGlobalData()->m_bIsBlackHighContrast) { rectButton.DeflateRect(1, 1); }

    if (rectButton.left > rectCombo.left + 1) {
      CMFCVisualManager::GetInstance()->OnDrawComboDropButton(
          deviceContext, rectButton, isDisabled, m_pWndCombo->GetDroppedState(), isHighlighted, this);
    }

    int curSel = CMFCToolBarComboBoxButton::GetCurSel();
    if (curSel >= 0) {
      LPCTSTR itemText = CMFCToolBarComboBoxButton::GetItem(curSel);
      if (itemText == nullptr) { itemText = L""; }

      CRect rectContent = rectCombo;
      rectContent.left += kIconAreaWidth;
      rectContent.right = m_rectButton.left;

      CRect textRect(rectContent.left + 3, rectContent.top, rectContent.right - 1, rectContent.bottom);

      CFont* oldFont = nullptr;
      if (m_pWndCombo != nullptr) {
        CFont* comboFont = m_pWndCombo->GetFont();
        if (comboFont != nullptr) { oldFont = deviceContext->SelectObject(comboFont); }
      }
      deviceContext->SetBkMode(TRANSPARENT);
      deviceContext->SetTextColor(schemeColors.menuText);
      deviceContext->DrawText(itemText, -1, textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      if (oldFont != nullptr) { deviceContext->SelectObject(oldFont); }
    }
  }
}

// --- Themed combo control ---

BEGIN_MESSAGE_MAP(EoCtrlTextStyleThemedCombo, CComboBox)
ON_WM_CTLCOLOR()
ON_WM_NCCALCSIZE()
ON_WM_PAINT()
ON_WM_NCPAINT()
ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

HBRUSH EoCtrlTextStyleThemedCombo::OnCtlColor(CDC* deviceContext, CWnd* /*control*/, UINT ctlColor) {
  if (ctlColor == CTLCOLOR_LISTBOX || ctlColor == CTLCOLOR_STATIC) {
    const COLORREF bgColor = (ctlColor == CTLCOLOR_LISTBOX) ? Eo::chromeColors.menuBackground : Eo::chromeColors.paneBackground;
    m_dropdownBackgroundBrush.DeleteObject();
    m_dropdownBackgroundBrush.CreateSolidBrush(bgColor);
    deviceContext->SetBkColor(bgColor);
    deviceContext->SetTextColor(Eo::chromeColors.menuText);
    return static_cast<HBRUSH>(m_dropdownBackgroundBrush);
  }
  return CComboBox::OnCtlColor(deviceContext, nullptr, ctlColor);
}

void EoCtrlTextStyleThemedCombo::OnNcCalcSize(BOOL /*calcValidRects*/, NCCALCSIZE_PARAMS* /*params*/) {
  // Forces entire window to client area for custom flat painting.
}

void EoCtrlTextStyleThemedCombo::OnPaint() {
  CPaintDC dc(this);
  CRect clientRect;
  GetClientRect(&clientRect);

  const auto& schemeColors = Eo::chromeColors;
  dc.FillSolidRect(&clientRect, schemeColors.paneBackground);

  int curSel = GetCurSel();
  if (curSel != CB_ERR) {
    CString text;
    GetLBText(curSel, text);

    CFont* oldFont = dc.SelectObject(GetFont());
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(schemeColors.menuText);
    CRect textRect = clientRect;
    textRect.left += 3;
    textRect.right -= 20;  // Leave space for dropdown arrow
    dc.DrawText(text, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    if (oldFont != nullptr) { dc.SelectObject(oldFont); }
  }
}

void EoCtrlTextStyleThemedCombo::OnNcPaint() {
  CWindowDC dc(this);
  CRect windowRect;
  GetWindowRect(&windowRect);
  windowRect.OffsetRect(-windowRect.left, -windowRect.top);
  dc.FillSolidRect(&windowRect, Eo::chromeColors.paneBackground);
}

BOOL EoCtrlTextStyleThemedCombo::OnEraseBkgnd(CDC* /*deviceContext*/) { return TRUE; }
