#include "Stdafx.h"

#include <commctrl.h>
#include <uxtheme.h>
#include <windowsx.h>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoCtrlLayerComboBox.h"
#include "EoDb.h"
#include "EoDbLayer.h"
#include "EoDlgSetupColor.h"
#include "EoGsRenderState.h"

namespace {

/// @brief Image list created from the IDB_LAYER_STATES bitmap resource (24×24 per icon).
/// Shared across all instances. Created on first use.
CImageList& GetLayerStateImages() {
  static CImageList imageList;
  if (imageList.GetSafeHandle() == nullptr) {
    CBitmap bitmap;
    bitmap.LoadBitmapW(IDB_LAYER_STATES);
    imageList.Create(24, 24, ILC_COLOR32 | ILC_MASK, 0, 1);
    imageList.Add(&bitmap, Eo::bitmapMaskColor);
  }
  return imageList;
}

/// @brief Icon indices in the IDB_LAYER_STATES bitmap (24x24 per icon).
/// Matches the indices used in EoDlgFileManage::DrawItem.
constexpr int kIconLocked = 0;
constexpr int kIconUnlocked = 1;
constexpr int kIconOn = 2;
constexpr int kIconOff = 3;
constexpr int kIconFrozen = 4;
constexpr int kIconThawed = 5;
constexpr int kIconWork = 8;
constexpr int kIconActive = 9;
constexpr int kIconStatic = 10;

}  // namespace

IMPLEMENT_SERIAL(EoCtrlLayerComboBox, CMFCToolBarComboBoxButton, VERSIONABLE_SCHEMA | 1)

EoCtrlLayerComboBox::EoCtrlLayerComboBox()
    : CMFCToolBarComboBoxButton(ID_LAYER_COMBO, -1, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 328) {
  PopulateItems();
}

void EoCtrlLayerComboBox::PopulateItems() {
  BuildItemList();

  auto* document = AeSysDoc::GetDoc();
  if (document == nullptr) { return; }

  // Select the work layer
  for (int i = 0; i < GetCount(); i++) {
    auto* layer = reinterpret_cast<EoDbLayer*>(GetItemData(i));
    if (layer != nullptr && layer->IsWork()) {
      SelectItem(i);
      return;
    }
  }
  if (GetCount() > 0) { SelectItem(0); }
}

void EoCtrlLayerComboBox::SetCurrentLayer(const CString& layerName) {
  for (int i = 0; i < GetCount(); i++) {
    auto* layer = reinterpret_cast<EoDbLayer*>(GetItemData(i));
    if (layer != nullptr && layer->Name().CompareNoCase(layerName) == 0) {
      SelectItem(i);
      return;
    }
  }
  // Rebuild and retry
  BuildItemList();
  for (int i = 0; i < GetCount(); i++) {
    auto* layer = reinterpret_cast<EoDbLayer*>(GetItemData(i));
    if (layer != nullptr && layer->Name().CompareNoCase(layerName) == 0) {
      SelectItem(i);
      return;
    }
  }
  if (GetCount() > 0) { SelectItem(0); }
}

void EoCtrlLayerComboBox::BuildItemList() {
  RemoveAllItems();

  auto* document = AeSysDoc::GetDoc();
  if (document == nullptr) {
    AddItem(L"0", 0);
    return;
  }

  for (int i = 0; i < document->GetLayerTableSize(); i++) {
    auto* layer = document->GetLayerTableLayerAt(i);
    if (layer != nullptr && layer->IsInternal()) { AddItem(layer->Name(), reinterpret_cast<DWORD_PTR>(layer)); }
  }

  if (GetCount() == 0) { AddItem(L"0", 0); }
}

void EoCtrlLayerComboBox::Serialize(CArchive& ar) {
  CMFCToolBarButton::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_iWidth;
    ar << static_cast<DWORD>(m_dwStyle);

    CString currentName;
    int curSel = CMFCToolBarComboBoxButton::GetCurSel();
    if (curSel >= 0) {
      auto* layer = reinterpret_cast<EoDbLayer*>(CMFCToolBarComboBoxButton::GetItemData(curSel));
      if (layer != nullptr) { currentName = layer->Name(); }
    }
    ar << currentName;
  } else {
    ar >> m_iWidth;
    DWORD dwStyle;
    ar >> dwStyle;
    m_dwStyle = dwStyle;

    CString savedName;
    ar >> savedName;

    BuildItemList();
    // Try to select saved name
    for (int i = 0; i < GetCount(); i++) {
      auto* layer = reinterpret_cast<EoDbLayer*>(CMFCToolBarComboBoxButton::GetItemData(i));
      if (layer != nullptr && layer->Name().CompareNoCase(savedName) == 0) {
        SelectItem(i);
        return;
      }
    }
    if (GetCount() > 0) { SelectItem(0); }
  }
}

CComboBox* EoCtrlLayerComboBox::CreateCombo(CWnd* parentWindow, const CRect& rect) {
  auto* combo = new EoCtrlLayerOwnerDrawCombo;
  // Inflate the rect height so the dropdown can show multiple items.
  // For CBS_DROPDOWNLIST, the total Create() rect height determines the dropdown extent.
  CRect adjustedRect = rect;
  adjustedRect.bottom = adjustedRect.top + 300;
  if (!combo->Create(m_dwStyle | WS_CHILD | WS_VISIBLE | WS_VSCROLL, adjustedRect, parentWindow, m_nID)) {
    delete combo;
    return nullptr;
  }
  combo->ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
  combo->ModifyStyle(WS_BORDER, 0);
  combo->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
  ::SetWindowTheme(combo->m_hWnd, L"", L"");
  return combo;
}

BOOL EoCtrlLayerComboBox::NotifyCommand(int notifyCode) {
  if (notifyCode == CBN_SELCHANGE) {
    OnSelectionChanged();
    return TRUE;
  }
  if (notifyCode == CBN_DROPDOWN) { PopulateItems(); }
  return CMFCToolBarComboBoxButton::NotifyCommand(notifyCode);
}

void EoCtrlLayerComboBox::OnSelectionChanged() {
  int selectedIndex = GetCurSel();
  if (selectedIndex < 0) { return; }

  auto* layer = reinterpret_cast<EoDbLayer*>(GetItemData(selectedIndex));
  if (layer == nullptr) { return; }

  auto* document = AeSysDoc::GetDoc();
  if (document == nullptr) { return; }

  // Switch to this layer as the work layer
  auto* previousWorkLayer = document->SetWorkLayer(layer);

  if (previousWorkLayer != nullptr && previousWorkLayer != layer) {
    previousWorkLayer->SetStateActive();
    document->UpdateAllViews(nullptr, EoDb::kLayerSafe, previousWorkLayer);
  }
  document->UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);
}

void EoCtrlLayerComboBox::OnMove() {
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

BOOL EoCtrlLayerComboBox::OnClick(CWnd* parentWindow, BOOL delay) {
  DWORD messagePos = ::GetMessagePos();
  CPoint screenPoint(GET_X_LPARAM(messagePos), GET_Y_LPARAM(messagePos));
  CPoint clientPoint = screenPoint;
  if (parentWindow != nullptr) { parentWindow->ScreenToClient(&clientPoint); }

  int iconRight = m_rectCombo.left + kIconAreaWidth;
  if (clientPoint.x >= m_rectCombo.left && clientPoint.x < iconRight) {
    OpenLayerManager();
    return TRUE;
  }
  return CMFCToolBarComboBoxButton::OnClick(parentWindow, delay);
}

void EoCtrlLayerComboBox::DrawLayerIcon(CDC* deviceContext, const CRect& iconRectangle) {
  CBitmap bitmap;
  if (!bitmap.LoadBitmap(IDB_LAYER_MANAGE)) { return; }

  BITMAP bitmapInfo{};
  bitmap.GetBitmap(&bitmapInfo);

  const int destWidth = std::min(static_cast<int>(bitmapInfo.bmWidth), static_cast<int>(iconRectangle.Width()));
  const int destHeight = std::min(static_cast<int>(bitmapInfo.bmHeight), static_cast<int>(iconRectangle.Height()));
  const int destX = iconRectangle.left + (iconRectangle.Width() - destWidth) / 2;
  const int destY = iconRectangle.top + (iconRectangle.Height() - destHeight) / 2;

  CDC memoryDC;
  memoryDC.CreateCompatibleDC(deviceContext);
  CBitmap* oldBitmap = memoryDC.SelectObject(&bitmap);
  deviceContext->BitBlt(destX, destY, destWidth, destHeight, &memoryDC, 0, 0, SRCCOPY);
  memoryDC.SelectObject(oldBitmap);
}

void EoCtrlLayerComboBox::OpenLayerManager() {
  auto* document = AeSysDoc::GetDoc();
  if (document != nullptr) { document->OnFileManage(); }
}

void EoCtrlLayerComboBox::OnDraw(CDC* deviceContext, const CRect& rect, CMFCToolBarImages* images, BOOL isHorz,
    BOOL isCustomizeMode, BOOL isHighlighted, BOOL drawBorder, BOOL grayDisabledButtons) {
  if (m_pWndCombo == nullptr || m_pWndCombo->GetSafeHwnd() == nullptr || !isHorz) {
    CMFCToolBarButton::OnDraw(
        deviceContext, rect, images, isHorz, isCustomizeMode, isHighlighted, drawBorder, grayDisabledButtons);
    return;
  }

  BOOL isDisabled = (isCustomizeMode && !IsEditable()) || (!isCustomizeMode && (m_nStyle & TBBS_DISABLED));

  if (m_bFlat) {
    const auto& schemeColors = Eo::chromeColors;

    CRect rectCombo = m_rectCombo;

    // Border — delegates to our OnDrawComboBorder override in EoMfVisualManager.
    CMFCVisualManager::GetInstance()->OnDrawComboBorder(
        deviceContext, rectCombo, isDisabled, m_pWndCombo->GetDroppedState(), isHighlighted, this);

    // Fill interior with scheme-aware paneBackground.
    rectCombo.DeflateRect(2, 2);
    deviceContext->FillSolidRect(rectCombo, schemeColors.paneBackground);

    // Draw icon area with separator line
    CRect iconRect(rectCombo.left, rectCombo.top, rectCombo.left + kIconAreaWidth - 2, rectCombo.bottom);
    DrawLayerIcon(deviceContext, iconRect);

    // Subtle vertical separator between icon and combo content
    int separatorX = rectCombo.left + kIconAreaWidth - 2;
    CPen separatorPen(PS_SOLID, 1, schemeColors.borderColor);
    CPen* oldPen = deviceContext->SelectObject(&separatorPen);
    deviceContext->MoveTo(separatorX, rectCombo.top + 2);
    deviceContext->LineTo(separatorX, rectCombo.bottom - 2);
    deviceContext->SelectObject(oldPen);

    // Drop-down button.
    CRect rectButton = m_rectButton;
    if (CMFCVisualManager::GetInstance() != nullptr) {
      CMFCVisualManager::GetInstance()->OnDrawComboDropButton(
          deviceContext, rectButton, isDisabled, m_pWndCombo->GetDroppedState(), isHighlighted, this);
    }

    // Draw selected item content directly on the toolbar DC
    int curSel = CMFCToolBarComboBoxButton::GetCurSel();
    if (curSel >= 0) {
      auto* layer = reinterpret_cast<EoDbLayer*>(CMFCToolBarComboBoxButton::GetItemData(curSel));

      CRect rectContent = rectCombo;
      rectContent.left += kIconAreaWidth;
      rectContent.right = m_rectButton.left;

      if (layer != nullptr) {
        auto& stateImages = GetLayerStateImages();
        constexpr int iconSize = EoCtrlLayerOwnerDrawCombo::kIconSize;
        constexpr int iconStep = EoCtrlLayerOwnerDrawCombo::kIconStep;
        int iconY = rectContent.top + (rectContent.Height() - iconSize) / 2;
        int x = rectContent.left + EoCtrlLayerOwnerDrawCombo::kOnOffX;

        // On/Off icon
        stateImages.Draw(deviceContext, layer->IsOff() ? kIconOff : kIconOn, CPoint(x, iconY), ILD_TRANSPARENT);
        x += iconStep;

        // Freeze icon
        stateImages.Draw(
            deviceContext, layer->IsFrozen() ? kIconFrozen : kIconThawed, CPoint(x, iconY), ILD_TRANSPARENT);
        x += iconStep;

        // Lock icon
        stateImages.Draw(
            deviceContext, layer->IsLocked() ? kIconLocked : kIconUnlocked, CPoint(x, iconY), ILD_TRANSPARENT);
        x += iconStep;

        // Color swatch
        constexpr int swatchSize = EoCtrlLayerOwnerDrawCombo::kColorWidth;
        int swatchY = rectContent.top + (rectContent.Height() - swatchSize) / 2;
        CRect swatchRect(x, swatchY, x + swatchSize, swatchY + swatchSize);
        deviceContext->FillSolidRect(swatchRect, layer->ColorValue());
        CBrush borderBrush(schemeColors.borderColor);
        deviceContext->FrameRect(swatchRect, &borderBrush);
        x += swatchSize + 6;

        // Layer name
        CRect textRect(x, rectContent.top, rectContent.right - 1, rectContent.bottom);
        CFont* oldFont = nullptr;
        if (m_pWndCombo != nullptr) {
          CFont* comboFont = m_pWndCombo->GetFont();
          if (comboFont != nullptr) { oldFont = deviceContext->SelectObject(comboFont); }
        }
        deviceContext->SetBkMode(TRANSPARENT);
        deviceContext->SetTextColor(schemeColors.menuText);
        deviceContext->DrawText(layer->Name(), -1, textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        if (oldFont != nullptr) { deviceContext->SelectObject(oldFont); }
      }
    }
  }
}

// --- Owner-draw combo control ---

BEGIN_MESSAGE_MAP(EoCtrlLayerOwnerDrawCombo, CComboBox)
ON_WM_CTLCOLOR()
ON_WM_NCCALCSIZE()
ON_WM_PAINT()
ON_WM_NCPAINT()
ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

/// @brief Subclass proc for the dropdown listbox — intercepts clicks on icon areas.
static LRESULT CALLBACK ListboxSubclassProc(
    HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR /*subclassId*/, DWORD_PTR refData) {
  if (message == WM_LBUTTONDOWN) {
    auto* combo = reinterpret_cast<EoCtrlLayerOwnerDrawCombo*>(refData);
    CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    // Hit-test which item
    LRESULT hitResult = ::SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(point.x, point.y));
    int itemIndex = LOWORD(hitResult);
    if (HIWORD(hitResult) == 0 && itemIndex >= 0 && itemIndex < combo->GetCount()) {
      auto* layer = reinterpret_cast<EoDbLayer*>(combo->GetItemData(itemIndex));
      if (layer != nullptr) {
        int clickX = point.x;
        bool handled = false;
        auto* document = AeSysDoc::GetDoc();

        using C = EoCtrlLayerOwnerDrawCombo;
        if (clickX >= C::kOnOffX && clickX < C::kOnOffX + C::kIconStep) {
          if (!layer->IsWork()) {
            if (layer->IsOff()) {
              layer->ClearStateFlag(static_cast<std::uint16_t>(EoDbLayer::State::isOff));
              if (document != nullptr) { document->UpdateAllViews(nullptr, EoDb::kLayer, layer); }
            } else {
              if (document != nullptr) { document->UpdateAllViews(nullptr, EoDb::kLayerErase, layer); }
              layer->SetStateOff();
            }
            handled = true;
          }
        } else if (clickX >= C::kFreezeX && clickX < C::kFreezeX + C::kIconStep) {
          if (!layer->IsWork()) {
            layer->SetFrozen(!layer->IsFrozen());
            handled = true;
          }
        } else if (clickX >= C::kLockX && clickX < C::kLockX + C::kIconStep) {
          if (!layer->IsWork()) {
            layer->SetLocked(!layer->IsLocked());
            handled = true;
          }
        } else if (clickX >= C::kColorX && clickX < C::kColorX + C::kColorWidth) {
          EoDlgSetupColor dialog;
          dialog.m_ColorIndex = static_cast<std::uint16_t>(layer->ColorIndex());
          if (dialog.DoModal() == IDOK) {
            layer->SetColorIndex(static_cast<std::int16_t>(dialog.m_ColorIndex));
            handled = true;
          }
        }

        if (handled) {
          ::InvalidateRect(hwnd, nullptr, TRUE);
          combo->Invalidate();
          if (document != nullptr) { document->UpdateAllViews(nullptr, EoDb::kLayerSafe, nullptr); }
          return 0;  // Eat the click — don't change selection or close dropdown
        }
      }
    }
  }
  return ::DefSubclassProc(hwnd, message, wParam, lParam);
}

HBRUSH EoCtrlLayerOwnerDrawCombo::OnCtlColor(CDC* deviceContext, CWnd* control, UINT ctlColor) {
  if (ctlColor == CTLCOLOR_LISTBOX) {
    // Subclass the dropdown listbox on first encounter for icon click handling
    if (!m_listboxSubclassed && control->GetSafeHwnd() != nullptr) {
      ::SetWindowSubclass(control->GetSafeHwnd(), ListboxSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
      m_listboxSubclassed = true;
    }
    m_dropdownBackgroundBrush.DeleteObject();
    m_dropdownBackgroundBrush.CreateSolidBrush(Eo::chromeColors.menuBackground);
    deviceContext->SetBkColor(Eo::chromeColors.menuBackground);
    return static_cast<HBRUSH>(m_dropdownBackgroundBrush);
  }
  return CComboBox::OnCtlColor(deviceContext, control, ctlColor);
}

void EoCtrlLayerOwnerDrawCombo::OnNcCalcSize(BOOL /*calcValidRects*/, NCCALCSIZE_PARAMS* /*params*/) {}

void EoCtrlLayerOwnerDrawCombo::OnPaint() {
  CPaintDC dc(this);
  CRect clientRect;
  GetClientRect(&clientRect);

  dc.FillSolidRect(&clientRect, Eo::chromeColors.paneBackground);

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

void EoCtrlLayerOwnerDrawCombo::OnNcPaint() {
  CWindowDC dc(this);
  CRect windowRect;
  GetWindowRect(&windowRect);
  windowRect.OffsetRect(-windowRect.left, -windowRect.top);
  dc.FillSolidRect(&windowRect, Eo::chromeColors.paneBackground);
}

BOOL EoCtrlLayerOwnerDrawCombo::OnEraseBkgnd(CDC* /*deviceContext*/) { return TRUE; }

void EoCtrlLayerOwnerDrawCombo::DrawDropdownArrow(CDC* deviceContext, const CRect& rect, COLORREF arrowColor) {
  UINT dpi = ::GetDpiForWindow(m_hWnd);
  if (dpi == 0) { dpi = 96; }
  int arrowWidth = ::MulDiv(7, dpi, 96) | 1;
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

void EoCtrlLayerOwnerDrawCombo::MeasureItem(LPMEASUREITEMSTRUCT measureItemStruct) {
  measureItemStruct->itemHeight = 24;
}

void EoCtrlLayerOwnerDrawCombo::DrawItem(LPDRAWITEMSTRUCT drawItemStruct) {
  if (drawItemStruct->itemID == static_cast<UINT>(-1)) { return; }

  CDC dc;
  dc.Attach(drawItemStruct->hDC);

  CRect itemRect(drawItemStruct->rcItem);
  UINT itemState = drawItemStruct->itemState;

  auto* layer = reinterpret_cast<EoDbLayer*>(drawItemStruct->itemData);

  const auto& schemeColors = Eo::chromeColors;

  // Background
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

  if (layer == nullptr) {
    dc.Detach();
    return;
  }

  auto& stateImages = GetLayerStateImages();
  int iconY = itemRect.top + (itemRect.Height() - kIconSize) / 2;
  int x = itemRect.left + kOnOffX;

  // On/Off icon
  stateImages.Draw(&dc, layer->IsOff() ? kIconOff : kIconOn, CPoint(x, iconY), ILD_TRANSPARENT);
  x += kIconStep;

  // Freeze icon
  stateImages.Draw(&dc, layer->IsFrozen() ? kIconFrozen : kIconThawed, CPoint(x, iconY), ILD_TRANSPARENT);
  x += kIconStep;

  // Lock icon
  stateImages.Draw(&dc, layer->IsLocked() ? kIconLocked : kIconUnlocked, CPoint(x, iconY), ILD_TRANSPARENT);
  x += kIconStep;

  // Color swatch
  int swatchY = itemRect.top + (itemRect.Height() - kColorWidth) / 2;
  CRect swatchRect(x, swatchY, x + kColorWidth, swatchY + kColorWidth);
  dc.FillSolidRect(swatchRect, layer->ColorValue());
  CBrush borderBrush(schemeColors.borderColor);
  dc.FrameRect(swatchRect, &borderBrush);
  x += kColorWidth + 6;

  // Layer name
  CRect textRect(x, itemRect.top, itemRect.right - 1, itemRect.bottom);
  dc.SetBkMode(TRANSPARENT);
  dc.SetTextColor(textColor);
  dc.DrawText(layer->Name(), -1, textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

  // Highlight border for selected dropdown items
  if ((itemState & ODS_SELECTED) && !(itemState & ODS_COMBOBOXEDIT)) {
    CBrush highlightBrush(schemeColors.menuHighlightBorder);
    dc.FrameRect(itemRect, &highlightBrush);
  }

  dc.Detach();
}
