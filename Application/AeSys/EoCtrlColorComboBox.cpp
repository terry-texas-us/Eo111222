#include "Stdafx.h"

#include <cstdint>
#include <uxtheme.h>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoCtrlColorComboBox.h"
#include "EoDbPrimitive.h"
#include "EoDlgSetupColor.h"
#include "EoGsRenderState.h"

IMPLEMENT_SERIAL(EoCtrlColorComboBox, CMFCToolBarComboBoxButton, VERSIONABLE_SCHEMA | 2)

EoCtrlColorComboBox::EoCtrlColorComboBox()
    : CMFCToolBarComboBoxButton(
          ID_PENCOLOR_COMBO, -1, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS) {
  PopulateItems();
}

void EoCtrlColorComboBox::PopulateItems() {
  BuildItemList(renderState.Color());
  SelectItem(static_cast<DWORD_PTR>(renderState.Color()));
}

void EoCtrlColorComboBox::SetCurrentColor(std::int16_t aciIndex) {
  if (SelectItem(static_cast<DWORD_PTR>(aciIndex))) { return; }

  // ACI not in current list (8–255 custom color) — rebuild with custom entry.
  BuildItemList(aciIndex);
  SelectItem(static_cast<DWORD_PTR>(aciIndex));
}

void EoCtrlColorComboBox::BuildItemList(std::int16_t activeAciIndex) {
  RemoveAllItems();
  AddItem(L"By Layer", static_cast<DWORD_PTR>(EoDbPrimitive::COLOR_BYLAYER));
  AddItem(L"By Block", static_cast<DWORD_PTR>(EoDbPrimitive::COLOR_BYBLOCK));
  AddItem(L"red", 1);
  AddItem(L"yellow", 2);
  AddItem(L"green", 3);
  AddItem(L"cyan", 4);
  AddItem(L"blue", 5);
  AddItem(L"magenta", 6);
  AddItem(L"white", 7);
  if (activeAciIndex >= 8 && activeAciIndex <= 255) {
    CString text;
    text.Format(L"%d", activeAciIndex);
    AddItem(text, static_cast<DWORD_PTR>(activeAciIndex));
  }
  AddItem(L"More Colors...", kMoreColors);
}

COLORREF EoCtrlColorComboBox::AciToColorRef(std::int16_t aciIndex) {
  if (aciIndex >= 0 && aciIndex <= 255) { return Eo::ColorPalette[aciIndex]; }
  return RGB(0, 0, 0);
}

CString EoCtrlColorComboBox::AciToName(std::int16_t aciIndex) {
  switch (aciIndex) {
    case EoDbPrimitive::COLOR_BYLAYER: return L"By Layer";
    case EoDbPrimitive::COLOR_BYBLOCK: return L"By Block";
    case 1: return L"red";
    case 2: return L"yellow";
    case 3: return L"green";
    case 4: return L"cyan";
    case 5: return L"blue";
    case 6: return L"magenta";
    case 7: return L"white";
    default: {
      CString str;
      str.Format(L"%d", aciIndex);
      return str;
    }
  }
}

void EoCtrlColorComboBox::Serialize(CArchive& ar) {
  // Call grandparent directly — skip CMFCToolBarComboBoxButton::Serialize which
  // would serialize combo items (causing duplicates with constructor-added items
  // and DWORD_PTR truncation of kMoreColors sentinel on x64).
  CMFCToolBarButton::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_iWidth;
    ar << static_cast<DWORD>(m_dwStyle);

    // Save only the selected ACI color — items are rebuilt on load.
    int curSel = CMFCToolBarComboBoxButton::GetCurSel();
    auto selectedColor = static_cast<std::int32_t>(renderState.Color());
    if (curSel >= 0) {
      auto itemData = CMFCToolBarComboBoxButton::GetItemData(curSel);
      if (itemData != kMoreColors) {
        selectedColor = static_cast<std::int32_t>(itemData);
      }
    }
    ar << selectedColor;
  } else {
    UINT schema = ar.GetObjectSchema();
    std::int16_t activeColor = renderState.Color();

    if (schema <= 1) {
      // Old format written by CMFCToolBarComboBoxButton::Serialize.
      // Consume all fields to advance archive position past the old data.
      ar >> m_iWidth;
      DWORD dwStyle;
      ar >> dwStyle;
      m_dwStyle = dwStyle;

      int nCount = 0;
      ar >> nCount;
      for (int i = 0; i < nCount; i++) {
        CString text;
        ar >> text;
        DWORD data;
        ar >> data;
      }
      int selIndex = 0;
      ar >> selIndex;
    } else {
      // Schema 2+: compact format with just the selected ACI color.
      ar >> m_iWidth;
      DWORD dwStyle;
      ar >> dwStyle;
      m_dwStyle = dwStyle;

      std::int32_t savedColor = 0;
      ar >> savedColor;
      activeColor = static_cast<std::int16_t>(savedColor);
    }

    // Rebuild items fresh — clears duplicates from constructor's PopulateItems().
    BuildItemList(activeColor);
    SelectItem(static_cast<DWORD_PTR>(activeColor));
  }
}

CComboBox* EoCtrlColorComboBox::CreateCombo(CWnd* parentWindow, const CRect& rect) {
  auto* combo = new EoCtrlColorOwnerDrawCombo;
  if (!combo->Create(
          m_dwStyle | WS_CHILD | WS_VISIBLE | WS_VSCROLL, rect, parentWindow, m_nID)) {
    delete combo;
    return nullptr;
  }
  // Remove all border styles and disable visual-style theme for fully custom flat painting.
  combo->ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
  combo->ModifyStyle(WS_BORDER, 0);
  combo->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
  ::SetWindowTheme(combo->m_hWnd, L"", L"");
  return combo;
}

BOOL EoCtrlColorComboBox::NotifyCommand(int notifyCode) {
  if (notifyCode == CBN_SELCHANGE) {
    OnSelectionChanged();
    return TRUE;
  }
  return CMFCToolBarComboBoxButton::NotifyCommand(notifyCode);
}

void EoCtrlColorComboBox::OnSelectionChanged() {
  int selectedIndex = GetCurSel();
  if (selectedIndex < 0) { return; }

  DWORD_PTR data = GetItemData(selectedIndex);

  if (data == kMoreColors) {
    // Open the full color selection dialog
    EoDlgSetupColor dialog;
    dialog.m_ColorIndex = static_cast<std::uint16_t>(renderState.Color());
    if (dialog.DoModal() == IDOK) {
      auto newColor = static_cast<std::int16_t>(dialog.m_ColorIndex);
      renderState.SetColor(static_cast<CDC*>(nullptr), newColor);
      auto* activeView = AeSysView::GetActiveView();
      if (activeView != nullptr) { activeView->UpdateStateInformation(AeSysView::Pen); }
    }
    // Restore combo selection to actual current color (not "More Colors...")
    SetCurrentColor(renderState.Color());
    return;
  }

  auto newColor = static_cast<std::int16_t>(data);
  renderState.SetColor(static_cast<CDC*>(nullptr), newColor);
  auto* activeView = AeSysView::GetActiveView();
  if (activeView != nullptr) { activeView->UpdateStateInformation(AeSysView::Pen); }
}

void EoCtrlColorComboBox::OnDraw(CDC* deviceContext, const CRect& rect, CMFCToolBarImages* images,
    BOOL isHorz, BOOL isCustomizeMode, BOOL isHighlighted, BOOL drawBorder,
    BOOL grayDisabledButtons) {
  if (m_pWndCombo == nullptr || m_pWndCombo->GetSafeHwnd() == nullptr || !isHorz) {
    CMFCToolBarButton::OnDraw(
        deviceContext, rect, images, isHorz, isCustomizeMode, isHighlighted, drawBorder, grayDisabledButtons);
    return;
  }

  BOOL isDisabled =
      (isCustomizeMode && !IsEditable()) || (!isCustomizeMode && (m_nStyle & TBBS_DISABLED));

  if (m_bFlat) {
    if (m_bIsHotEdit) { isHighlighted = TRUE; }

    CRect rectCombo = m_rectCombo;

    // Border — delegates to our OnDrawComboBorder override in EoMfVisualManager.
    CMFCVisualManager::GetInstance()->OnDrawComboBorder(
        deviceContext, rectCombo, isDisabled, m_pWndCombo->GetDroppedState(), isHighlighted, this);

    // Fill interior with scheme-aware paneBackground instead of clrWindow (white).
    rectCombo.DeflateRect(2, 2);
    const auto& schemeColors = Eo::SchemeColors(Eo::activeColorScheme);
    deviceContext->FillSolidRect(rectCombo, schemeColors.paneBackground);

    // Drop-down button.
    CRect rectButton = m_rectButton;
    if (GetGlobalData()->m_bIsBlackHighContrast) { rectButton.DeflateRect(1, 1); }

    if (rectButton.left > rectCombo.left + 1) {
      CMFCVisualManager::GetInstance()->OnDrawComboDropButton(
          deviceContext, rectButton, isDisabled, m_pWndCombo->GetDroppedState(), isHighlighted, this);
    }

    // Draw selected item content directly on the toolbar DC.
    // Uses the button's internal item list — not the combo control — because the control
    // may not yet be populated when the toolbar first paints.
    int curSel = CMFCToolBarComboBoxButton::GetCurSel();
    if (curSel >= 0) {
      DWORD_PTR itemData = CMFCToolBarComboBoxButton::GetItemData(curSel);
      LPCTSTR itemText = CMFCToolBarComboBoxButton::GetItem(curSel);
      if (itemText == nullptr) { itemText = L""; }

      CRect rectContent = rectCombo;
      rectContent.right = m_rectButton.left;

      bool isMoreColors = (itemData == kMoreColors);

      constexpr int swatchMargin = 2;
      int swatchSize = rectContent.Height() - 2 * swatchMargin;

      if (!isMoreColors && swatchSize > 0) {
        CRect swatchRect(
            rectContent.left + swatchMargin + 1, rectContent.top + swatchMargin,
            rectContent.left + swatchMargin + 1 + swatchSize, rectContent.top + swatchMargin + swatchSize);

        auto aciIndex = static_cast<std::int16_t>(itemData);
        COLORREF swatchColor =
            (aciIndex == EoDbPrimitive::COLOR_BYLAYER || aciIndex == EoDbPrimitive::COLOR_BYBLOCK)
            ? RGB(255, 255, 255)
            : AciToColorRef(aciIndex);
        deviceContext->FillSolidRect(swatchRect, swatchColor);
        CBrush borderBrush(schemeColors.borderColor);
        deviceContext->FrameRect(swatchRect, &borderBrush);
      }

      int textLeft = isMoreColors
          ? rectContent.left + swatchMargin + 1
          : rectContent.left + swatchMargin + 1 + swatchSize + 4;
      CRect textRect(textLeft, rectContent.top, rectContent.right - 1, rectContent.bottom);

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

// --- Owner-draw combo control ---

BEGIN_MESSAGE_MAP(EoCtrlColorOwnerDrawCombo, CComboBox)
ON_WM_CTLCOLOR()
ON_WM_NCCALCSIZE()
ON_WM_PAINT()
ON_WM_NCPAINT()
ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

HBRUSH EoCtrlColorOwnerDrawCombo::OnCtlColor(CDC* deviceContext, CWnd* control, UINT ctlColor) {
  if (ctlColor == CTLCOLOR_LISTBOX) {
    const auto& schemeColors = Eo::SchemeColors(Eo::activeColorScheme);
    m_dropdownBackgroundBrush.DeleteObject();
    m_dropdownBackgroundBrush.CreateSolidBrush(schemeColors.menuBackground);
    deviceContext->SetBkColor(schemeColors.menuBackground);
    return static_cast<HBRUSH>(m_dropdownBackgroundBrush);
  }
  return CComboBox::OnCtlColor(deviceContext, control, ctlColor);
}

void EoCtrlColorOwnerDrawCombo::OnNcCalcSize(
    BOOL /*calcValidRects*/, NCCALCSIZE_PARAMS* /*params*/) {
  // Do not call base — forces the entire window to be client area.
  // This prevents the system from reserving a non-client region for the dropdown button,
  // allowing OnPaint to paint the button area with the theme-correct background.
}

void EoCtrlColorOwnerDrawCombo::OnPaint() {
  CPaintDC dc(this);
  CRect clientRect;
  GetClientRect(&clientRect);

  const auto& schemeColors = Eo::SchemeColors(Eo::activeColorScheme);

  // Fill entire client area with paneBackground (dark: 32,32,29; light: 253,252,249).
  dc.FillSolidRect(&clientRect, schemeColors.paneBackground);

  // Content area — the full client rect.  The dropdown arrow is drawn by the visual
  // manager's OnDrawComboDropButton in the toolbar button area adjacent to this window.
  int curSel = GetCurSel();
  if (curSel != CB_ERR) {
    DRAWITEMSTRUCT dis{};
    dis.CtlType = ODT_COMBOBOX;
    dis.CtlID = GetDlgCtrlID();
    dis.itemID = static_cast<UINT>(curSel);
    dis.itemAction = ODA_DRAWENTIRE;
    dis.itemState = ODS_COMBOBOXEDIT;
    dis.hwndItem = m_hWnd;
    dis.hDC = dc.m_hDC;
    dis.rcItem = clientRect;
    dis.itemData = GetItemData(curSel);
    DrawItem(&dis);
  }
}

void EoCtrlColorOwnerDrawCombo::OnNcPaint() {
  // Paint non-client border with paneBackground — eliminates the system chrome frame.
  CWindowDC dc(this);
  CRect windowRect;
  GetWindowRect(&windowRect);
  windowRect.OffsetRect(-windowRect.left, -windowRect.top);
  const auto& schemeColors = Eo::SchemeColors(Eo::activeColorScheme);
  dc.FillSolidRect(&windowRect, schemeColors.paneBackground);
}

BOOL EoCtrlColorOwnerDrawCombo::OnEraseBkgnd(CDC* /*deviceContext*/) { return TRUE; }

void EoCtrlColorOwnerDrawCombo::DrawDropdownArrow(
    CDC* deviceContext, const CRect& rect, COLORREF arrowColor) {
  UINT dpi = ::GetDpiForWindow(m_hWnd);
  if (dpi == 0) { dpi = 96; }
  int arrowWidth = ::MulDiv(7, dpi, 96) | 1;  // Ensure odd for centered tip
  int arrowHeight = ::MulDiv(4, dpi, 96);
  if (arrowHeight < 3) { arrowHeight = 3; }

  int centerX = (rect.left + rect.right) / 2;
  int centerY = (rect.top + rect.bottom) / 2;
  int left = centerX - arrowWidth / 2;
  int top = centerY - arrowHeight / 2;

  POINT points[3] = {{left, top}, {left + arrowWidth, top}, {centerX, top + arrowHeight}};
  CBrush arrowBrush(arrowColor);
  CPen arrowPen(PS_SOLID, 1, arrowColor);
  auto* oldBrush = deviceContext->SelectObject(&arrowBrush);
  auto* oldPen = deviceContext->SelectObject(&arrowPen);
  deviceContext->Polygon(points, 3);
  deviceContext->SelectObject(oldBrush);
  deviceContext->SelectObject(oldPen);
}

void EoCtrlColorOwnerDrawCombo::MeasureItem(LPMEASUREITEMSTRUCT measureItemStruct) {
  UINT dpi = ::GetDpiForWindow(m_hWnd);
  if (dpi == 0) { dpi = 96; }
  measureItemStruct->itemHeight = ::MulDiv(18, dpi, 96);
}

void EoCtrlColorOwnerDrawCombo::DrawItem(LPDRAWITEMSTRUCT drawItemStruct) {
  if (drawItemStruct->itemID == static_cast<UINT>(-1)) { return; }

  CDC dc;
  dc.Attach(drawItemStruct->hDC);

  CRect itemRect(drawItemStruct->rcItem);
  auto itemData = drawItemStruct->itemData;
  UINT itemState = drawItemStruct->itemState;

  const auto& schemeColors = Eo::SchemeColors(Eo::activeColorScheme);

  // Background and text colors — theme-aware
  COLORREF backgroundColor;
  COLORREF textColor;
  if (itemState & ODS_COMBOBOXEDIT) {
    // Closed combo display area: paneBackground matches VS toolbar combo style
    backgroundColor = schemeColors.paneBackground;
    textColor = schemeColors.menuText;
  } else if (itemState & ODS_SELECTED) {
    backgroundColor = schemeColors.menuHighlightBackground;
    textColor = schemeColors.menuText;
  } else {
    backgroundColor = schemeColors.menuBackground;
    textColor = schemeColors.menuText;
  }
  dc.FillSolidRect(itemRect, backgroundColor);

  // Item text
  CString itemText;
  GetLBText(static_cast<int>(drawItemStruct->itemID), itemText);

  constexpr int swatchMargin = 2;
  int swatchSize = itemRect.Height() - 2 * swatchMargin;
  CRect swatchRect(itemRect.left + swatchMargin + 1, itemRect.top + swatchMargin,
      itemRect.left + swatchMargin + 1 + swatchSize, itemRect.top + swatchMargin + swatchSize);

  bool isMoreColors = (itemData == EoCtrlColorComboBox::kMoreColors);

  if (!isMoreColors) {
    auto aciIndex = static_cast<std::int16_t>(itemData);

    if (aciIndex == EoDbPrimitive::COLOR_BYLAYER || aciIndex == EoDbPrimitive::COLOR_BYBLOCK) {
      // Hollow swatch with border for By Layer / By Block
      dc.FillSolidRect(swatchRect, RGB(255, 255, 255));
      CBrush borderBrush(schemeColors.borderColor);
      dc.FrameRect(swatchRect, &borderBrush);
    } else {
      // Filled color swatch
      COLORREF swatchColor = EoCtrlColorComboBox::AciToColorRef(aciIndex);
      dc.FillSolidRect(swatchRect, swatchColor);
      CBrush borderBrush(schemeColors.borderColor);
      dc.FrameRect(swatchRect, &borderBrush);
    }
  }

  // Text after swatch (or at left margin for "More Colors...")
  int textLeft = isMoreColors ? itemRect.left + swatchMargin + 1 : swatchRect.right + 4;
  CRect textRect(textLeft, itemRect.top, itemRect.right - 1, itemRect.bottom);
  dc.SetBkMode(TRANSPARENT);
  dc.SetTextColor(textColor);
  dc.DrawText(itemText, textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

  // Highlight border for selected dropdown items (not display area)
  if ((itemState & ODS_SELECTED) && !(itemState & ODS_COMBOBOXEDIT)) {
    CBrush highlightBrush(schemeColors.menuHighlightBorder);
    dc.FrameRect(itemRect, &highlightBrush);
  }

  dc.Detach();
}
