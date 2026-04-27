#include "Stdafx.h"

#include <algorithm>
#include <cstdint>

#include "Eo.h"
#include "EoCtrlLineWeightComboBox.h"
#include "EoGsRenderState.h"

IMPLEMENT_SERIAL(EoCtrlLineWeightComboBox, CMFCToolBarComboBoxButton, VERSIONABLE_SCHEMA | 1)

EoCtrlLineWeightComboBox::EoCtrlLineWeightComboBox()
    : CMFCToolBarComboBoxButton(ID_LINEWEIGHT_COMBO, -1, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 160) {
  PopulateItems();
}

void EoCtrlLineWeightComboBox::PopulateItems() {
  BuildItemList();

  // Select item matching current render state
  const auto currentWeight = Gs::renderState.LineWeight();
  SelectItem(static_cast<DWORD_PTR>(currentWeight));
}

void EoCtrlLineWeightComboBox::SetCurrentLineWeight(EoDxfLineWeights::LineWeight lineWeight) {
  if (SelectItem(static_cast<DWORD_PTR>(lineWeight))) { return; }
  // Fallback: select ByLayer (first item)
  SelectItem(0);
}

void EoCtrlLineWeightComboBox::BuildItemList() {
  RemoveAllItems();

  // Special entries
  AddItem(L"ByLayer", static_cast<DWORD_PTR>(EoDxfLineWeights::LineWeight::kLnWtByLayer));
  AddItem(L"ByBlock", static_cast<DWORD_PTR>(EoDxfLineWeights::LineWeight::kLnWtByBlock));
  AddItem(L"Default", static_cast<DWORD_PTR>(EoDxfLineWeights::LineWeight::kLnWtByLwDefault));

  // 24 standard DXF line weights (enum values 0..23)
  static constexpr struct {
    EoDxfLineWeights::LineWeight weight;
    const wchar_t* label;
  } standardWeights[] = {
      {EoDxfLineWeights::LineWeight::kLnWt000, L"0.00 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt005, L"0.05 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt009, L"0.09 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt013, L"0.13 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt015, L"0.15 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt018, L"0.18 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt020, L"0.20 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt025, L"0.25 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt030, L"0.30 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt035, L"0.35 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt040, L"0.40 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt050, L"0.50 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt053, L"0.53 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt060, L"0.60 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt070, L"0.70 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt080, L"0.80 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt090, L"0.90 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt100, L"1.00 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt106, L"1.06 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt120, L"1.20 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt140, L"1.40 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt158, L"1.58 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt200, L"2.00 mm"},
      {EoDxfLineWeights::LineWeight::kLnWt211, L"2.11 mm"},
  };
  for (const auto& [weight, label] : standardWeights) { AddItem(label, static_cast<DWORD_PTR>(weight)); }
}

CString EoCtrlLineWeightComboBox::LineWeightToName(EoDxfLineWeights::LineWeight lineWeight) {
  switch (lineWeight) {
    case EoDxfLineWeights::LineWeight::kLnWtByLayer:
      return L"ByLayer";
    case EoDxfLineWeights::LineWeight::kLnWtByBlock:
      return L"ByBlock";
    case EoDxfLineWeights::LineWeight::kLnWtByLwDefault:
      return L"Default";
    default: {
      const auto dxfCode = EoDxfLineWeights::LineWeightToDxfIndex(lineWeight);
      CString text;
      text.Format(L"%.2f mm", dxfCode * 0.01);
      return text;
    }
  }
}

void EoCtrlLineWeightComboBox::DrawWeightPreview(CDC* deviceContext,
    const CRect& rect,
    EoDxfLineWeights::LineWeight lineWeight,
    COLORREF lineColor) {
  const int yCenter = rect.top + rect.Height() / 2;
  const int xStart = rect.left + 2;
  const int xEnd = rect.right - 2;
  if (xEnd <= xStart) { return; }

  // Determine pixel thickness for preview: special values draw as 1px
  int thickness{1};
  const auto enumIndex = static_cast<std::int8_t>(lineWeight);
  if (enumIndex >= 0 && enumIndex <= 23) {
    const auto dxfCode = EoDxfLineWeights::LineWeightToDxfIndex(lineWeight);
    // Scale: 0.00mm→1px, 2.11mm→~8px, proportional to DXF code
    UINT dpi = ::GetDpiForSystem();
    if (dpi == 0) { dpi = 96; }
    if (dxfCode > 0) { thickness = std::max(1, ::MulDiv(dxfCode, static_cast<int>(dpi), 96 * 30)); }
  }

  CPen pen(PS_SOLID, thickness, lineColor);
  CPen* oldPen = deviceContext->SelectObject(&pen);
  deviceContext->MoveTo(xStart, yCenter);
  deviceContext->LineTo(xEnd, yCenter);
  deviceContext->SelectObject(oldPen);
}

void EoCtrlLineWeightComboBox::Serialize(CArchive& ar) {
  // Bypass CMFCToolBarComboBoxButton::Serialize — same pattern as color/linetype combos.
  CMFCToolBarButton::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_iWidth;
    ar << static_cast<DWORD>(m_dwStyle);

    // Save the current line weight enum value — items are rebuilt on load.
    auto currentWeight = static_cast<std::int32_t>(Gs::renderState.LineWeight());
    const int curSel = CMFCToolBarComboBoxButton::GetCurSel();
    if (curSel >= 0) {
      const auto itemData = CMFCToolBarComboBoxButton::GetItemData(curSel);
      currentWeight = static_cast<std::int32_t>(itemData);
    }
    ar << currentWeight;
  } else {
    ar >> m_iWidth;
    DWORD dwStyle;
    ar >> dwStyle;
    m_dwStyle = dwStyle;

    std::int32_t savedWeight = 0;
    ar >> savedWeight;

    // Rebuild items and select the saved weight.
    BuildItemList();
    if (!SelectItem(static_cast<DWORD_PTR>(savedWeight))) {
      // Fallback: ByLayer
      SelectItem(static_cast<DWORD_PTR>(EoDxfLineWeights::LineWeight::kLnWtByLayer));
    }
  }
}

CComboBox* EoCtrlLineWeightComboBox::CreateCombo(CWnd* parentWindow, const CRect& rect) {
  auto* combo = new EoCtrlLineWeightOwnerDrawCombo;
  if (!combo->Create(m_dwStyle | WS_CHILD | WS_VISIBLE | WS_VSCROLL, rect, parentWindow, m_nID)) {
    delete combo;
    return nullptr;
  }
  combo->ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
  combo->ModifyStyle(WS_BORDER, 0);
  combo->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
  return combo;
}

BOOL EoCtrlLineWeightComboBox::NotifyCommand(int notifyCode) {
  if (notifyCode == CBN_SELCHANGE) {
    OnSelectionChanged();
    return TRUE;
  }
  return CMFCToolBarComboBoxButton::NotifyCommand(notifyCode);
}

void EoCtrlLineWeightComboBox::OnSelectionChanged() {
  const int selectedIndex = GetCurSel();
  if (selectedIndex < 0) { return; }

  const auto data = GetItemData(selectedIndex);
  const auto newWeight = static_cast<EoDxfLineWeights::LineWeight>(data);
  Gs::renderState.SetLineWeight(newWeight);
}

void EoCtrlLineWeightComboBox::OnDraw(CDC* deviceContext,
    const CRect& rect,
    CMFCToolBarImages* images,
    BOOL isHorz,
    BOOL isCustomizeMode,
    BOOL isHighlighted,
    BOOL drawBorder,
    BOOL grayDisabledButtons) {
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

    CRect rectButton = m_rectButton;
    if (GetGlobalData()->m_bIsBlackHighContrast) { rectButton.DeflateRect(1, 1); }

    if (rectButton.left > rectCombo.left + 1) {
      CMFCVisualManager::GetInstance()->OnDrawComboDropButton(
          deviceContext, rectButton, isDisabled, m_pWndCombo->GetDroppedState(), isHighlighted, this);
    }

    const int curSel = CMFCToolBarComboBoxButton::GetCurSel();
    if (curSel >= 0) {
      const DWORD_PTR itemData = CMFCToolBarComboBoxButton::GetItemData(curSel);
      LPCTSTR itemText = CMFCToolBarComboBoxButton::GetItem(curSel);
      if (itemText == nullptr) { itemText = L""; }

      CRect rectContent = rectCombo;
      rectContent.right = m_rectButton.left;

      // Weight preview area
      UINT dpi = ::GetDpiForSystem();
      if (dpi == 0) { dpi = 96; }
      const int previewWidth = ::MulDiv(32, dpi, 96);

      const auto lineWeight = static_cast<EoDxfLineWeights::LineWeight>(itemData);

      int previewRight = rectContent.left + 2 + previewWidth;
      previewRight = std::min<LONG>(previewRight, rectContent.right - 40);
      const CRect previewRect(rectContent.left + 2, rectContent.top, previewRight, rectContent.bottom);
      DrawWeightPreview(deviceContext, previewRect, lineWeight, schemeColors.menuText);

      CRect textRect(previewRight + 4, rectContent.top, rectContent.right - 1, rectContent.bottom);

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

BEGIN_MESSAGE_MAP(EoCtrlLineWeightOwnerDrawCombo, CComboBox)
ON_WM_CTLCOLOR()
ON_WM_NCCALCSIZE()
ON_WM_PAINT()
ON_WM_NCPAINT()
ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

HBRUSH EoCtrlLineWeightOwnerDrawCombo::OnCtlColor(CDC* deviceContext, CWnd* control, UINT ctlColor) {
  if (ctlColor == CTLCOLOR_LISTBOX) {
    m_dropdownBackgroundBrush.DeleteObject();
    m_dropdownBackgroundBrush.CreateSolidBrush(Eo::chromeColors.menuBackground);
    deviceContext->SetBkColor(Eo::chromeColors.menuBackground);
    return static_cast<HBRUSH>(m_dropdownBackgroundBrush);
  }
  return CComboBox::OnCtlColor(deviceContext, control, ctlColor);
}

void EoCtrlLineWeightOwnerDrawCombo::OnNcCalcSize(BOOL /*calcValidRects*/, NCCALCSIZE_PARAMS* /*params*/) {
  // Forces entire window to client area for custom flat painting.
}

void EoCtrlLineWeightOwnerDrawCombo::OnPaint() {
  CPaintDC dc(this);
  CRect clientRect;
  GetClientRect(&clientRect);

  dc.FillSolidRect(&clientRect, Eo::chromeColors.paneBackground);

  const int curSel = GetCurSel();
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

void EoCtrlLineWeightOwnerDrawCombo::OnNcPaint() {
  CWindowDC dc(this);
  CRect windowRect;
  GetWindowRect(&windowRect);
  windowRect.OffsetRect(-windowRect.left, -windowRect.top);
  dc.FillSolidRect(&windowRect, Eo::chromeColors.paneBackground);
}

BOOL EoCtrlLineWeightOwnerDrawCombo::OnEraseBkgnd(CDC* /*deviceContext*/) {
  return TRUE;
}

void EoCtrlLineWeightOwnerDrawCombo::MeasureItem(LPMEASUREITEMSTRUCT measureItemStruct) {
  UINT dpi = ::GetDpiForWindow(m_hWnd);
  if (dpi == 0) { dpi = 96; }
  measureItemStruct->itemHeight = ::MulDiv(18, dpi, 96);
}

void EoCtrlLineWeightOwnerDrawCombo::DrawItem(LPDRAWITEMSTRUCT drawItemStruct) {
  if (drawItemStruct->itemID == static_cast<UINT>(-1)) { return; }

  CDC dc;
  dc.Attach(drawItemStruct->hDC);

  CRect itemRect(drawItemStruct->rcItem);
  const auto itemData = drawItemStruct->itemData;
  const auto itemState = drawItemStruct->itemState;

  const auto& schemeColors = Eo::chromeColors;

  COLORREF backgroundColor;
  COLORREF textColor;
  if (itemState & ODS_COMBOBOXEDIT) {
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

  CString itemText;
  GetLBText(static_cast<int>(drawItemStruct->itemID), itemText);

  // Weight preview area
  UINT dpi = ::GetDpiForWindow(m_hWnd);
  if (dpi == 0) { dpi = 96; }
  const auto previewWidth = ::MulDiv(48, dpi, 96);

  const auto lineWeight = static_cast<EoDxfLineWeights::LineWeight>(itemData);

  int previewRight = itemRect.left + 2 + previewWidth;
  previewRight = std::min<LONG>(previewRight, itemRect.right - 40);
  const CRect previewRect(itemRect.left + 2, itemRect.top, previewRight, itemRect.bottom);
  EoCtrlLineWeightComboBox::DrawWeightPreview(&dc, previewRect, lineWeight, textColor);

  CRect textRect(previewRight + 4, itemRect.top, itemRect.right - 1, itemRect.bottom);
  dc.SetBkMode(TRANSPARENT);
  dc.SetTextColor(textColor);
  dc.DrawText(itemText, textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

  // Highlight border for selected dropdown items
  if ((itemState & ODS_SELECTED) && !(itemState & ODS_COMBOBOXEDIT)) {
    CBrush highlightBrush(schemeColors.menuHighlightBorder);
    dc.FrameRect(itemRect, &highlightBrush);
  }

  dc.Detach();
}
