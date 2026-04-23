#include "Stdafx.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>
#include <uxtheme.h>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoCtrlLineTypeComboBox.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"
#include "EoDlgLineTypesSelection.h"
#include "EoGsRenderState.h"

IMPLEMENT_SERIAL(EoCtrlLineTypeComboBox, CMFCToolBarComboBoxButton, VERSIONABLE_SCHEMA | 1)

EoCtrlLineTypeComboBox::EoCtrlLineTypeComboBox()
    : CMFCToolBarComboBoxButton(ID_LINETYPE_COMBO, -1, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 250) {
  PopulateItems();
}

void EoCtrlLineTypeComboBox::PopulateItems() {
  BuildItemList();

  // Select item matching current render state
  const auto& currentName = Gs::renderState.LineTypeName();
  if (!currentName.empty()) {
    for (int i = 0; i < GetCount(); i++) {
      auto data = GetItemData(i);
      if (data == kLoadLineTypes) { continue; }
      LPCTSTR text = GetItem(i);
      if (text != nullptr && _wcsicmp(text, currentName.c_str()) == 0) {
        SelectItem(i);
        return;
      }
    }
  }
  // Fall back to Continuous (index 2, after ByLayer and ByBlock)
  if (GetCount() > 2) { SelectItem(2); }
}

void EoCtrlLineTypeComboBox::SetCurrentLineType(std::int16_t lineTypeIndex, const std::wstring& lineTypeName) {
  // Try to find by name first
  if (!lineTypeName.empty()) {
    for (int i = 0; i < GetCount(); i++) {
      auto data = GetItemData(i);
      if (data == kLoadLineTypes) { continue; }
      LPCTSTR text = GetItem(i);
      if (text != nullptr && _wcsicmp(text, lineTypeName.c_str()) == 0) {
        SelectItem(i);
        return;
      }
    }
  }
  // Try by special index
  if (lineTypeIndex == EoDbPrimitive::LINETYPE_BYLAYER) {
    SelectItem(0);
    return;
  }
  if (lineTypeIndex == EoDbPrimitive::LINETYPE_BYBLOCK) {
    if (GetCount() > 1) { SelectItem(1); }
    return;
  }
  // Rebuild to pick up any new linetypes, then retry
  BuildItemList();
  if (!lineTypeName.empty()) {
    for (int i = 0; i < GetCount(); i++) {
      auto data = GetItemData(i);
      if (data == kLoadLineTypes) { continue; }
      LPCTSTR text = GetItem(i);
      if (text != nullptr && _wcsicmp(text, lineTypeName.c_str()) == 0) {
        SelectItem(i);
        return;
      }
    }
  }
  // Fall back to Continuous
  if (GetCount() > 2) { SelectItem(2); }
}

void EoCtrlLineTypeComboBox::BuildItemList() {
  RemoveAllItems();

  // Fixed entries — always present in this order
  AddItem(L"ByLayer", static_cast<DWORD_PTR>(EoDbPrimitive::LINETYPE_BYLAYER));
  AddItem(L"ByBlock", static_cast<DWORD_PTR>(EoDbPrimitive::LINETYPE_BYBLOCK));

  // Document line types from the active document's table.
  // CONTINUOUS is added as a fixed third entry; remaining names are sorted alphabetically.
  auto* document = AeSysDoc::GetDoc();
  if (document != nullptr) {
    auto* lineTypeTable = document->LineTypeTable();
    if (lineTypeTable != nullptr && !lineTypeTable->IsEmpty()) {
      // Add CONTINUOUS first (if present in the table) so it follows ByLayer/ByBlock
      EoDbLineType* continuousLineType{};
      if (lineTypeTable->Lookup(L"CONTINUOUS", continuousLineType) && continuousLineType != nullptr) {
        AddItem(continuousLineType->Name(), reinterpret_cast<DWORD_PTR>(continuousLineType));
      }

      // Collect remaining names, sort alphabetically, then add
      std::vector<std::pair<CString, EoDbLineType*>> sortedEntries;
      POSITION position = lineTypeTable->GetStartPosition();
      while (position != nullptr) {
        CString name;
        EoDbLineType* lineType{};
        lineTypeTable->GetNextAssoc(position, name, lineType);
        if (lineType != nullptr && name.CompareNoCase(L"CONTINUOUS") != 0) {
          sortedEntries.emplace_back(name, lineType);
        }
      }
      std::sort(sortedEntries.begin(), sortedEntries.end(),
          [](const auto& a, const auto& b) { return a.first.CompareNoCase(b.first) < 0; });
      for (const auto& [name, lineType] : sortedEntries) {
        AddItem(lineType->Name(), reinterpret_cast<DWORD_PTR>(lineType));
      }
    }
  }

  // Sentinel entry
  AddItem(L"Load Line Types...", kLoadLineTypes);
}

void EoCtrlLineTypeComboBox::Serialize(CArchive& ar) {
  // Bypass CMFCToolBarComboBoxButton::Serialize — same pattern as color combo.
  CMFCToolBarButton::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_iWidth;
    ar << static_cast<DWORD>(m_dwStyle);

    // Save the current line type name — items are rebuilt from the document on load.
    CString currentName;
    int curSel = CMFCToolBarComboBoxButton::GetCurSel();
    if (curSel >= 0) {
      auto data = CMFCToolBarComboBoxButton::GetItemData(curSel);
      if (data != kLoadLineTypes) {
        LPCTSTR text = CMFCToolBarComboBoxButton::GetItem(curSel);
        if (text != nullptr) { currentName = text; }
      }
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
        auto data = GetItemData(i);
        if (data == kLoadLineTypes) { continue; }
        LPCTSTR text = GetItem(i);
        if (text != nullptr && savedName.CompareNoCase(text) == 0) {
          SelectItem(i);
          found = true;
          break;
        }
      }
    }
    if (!found && GetCount() > 2) { SelectItem(2); }  // Continuous
  }
}

CComboBox* EoCtrlLineTypeComboBox::CreateCombo(CWnd* parentWindow, const CRect& rect) {
  auto* combo = new EoCtrlLineTypeOwnerDrawCombo;
  if (!combo->Create(m_dwStyle | WS_CHILD | WS_VISIBLE | WS_VSCROLL, rect, parentWindow, m_nID)) {
    delete combo;
    return nullptr;
  }
  combo->ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
  combo->ModifyStyle(WS_BORDER, 0);
  combo->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
  ::SetWindowTheme(combo->m_hWnd, L"", L"");

  // Dropdown list is 150% the width of the edit control for full name visibility
  CRect comboRect;
  combo->GetWindowRect(&comboRect);
  combo->SetDroppedWidth(comboRect.Width() * 3 / 2);
  return combo;
}

BOOL EoCtrlLineTypeComboBox::NotifyCommand(int notifyCode) {
  if (notifyCode == CBN_SELCHANGE) {
    OnSelectionChanged();
    return TRUE;
  }
  return CMFCToolBarComboBoxButton::NotifyCommand(notifyCode);
}

void EoCtrlLineTypeComboBox::OnSelectionChanged() {
  const int selectedIndex = GetCurSel();
  if (selectedIndex < 0) { return; }

  const auto data = GetItemData(selectedIndex);

  if (data == kLoadLineTypes) {
    // Open the line type selection dialog (same flow as OnSetupLineType)
    auto* document = AeSysDoc::GetDoc();
    if (document != nullptr) {
      EoDlgLineTypesSelection dialog(*(document->LineTypeTable()));

      EoDbLineType* currentLineType{};
      const auto& currentName = Gs::renderState.LineTypeName();
      if (!currentName.empty()) {
        (void)document->LineTypeTable()->Lookup(CString(currentName.c_str()), currentLineType);
      }
      if (currentLineType == nullptr) {
        document->LineTypeTable()->LookupUsingLegacyIndex(
            static_cast<std::uint16_t>(Gs::renderState.LineTypeIndex()), currentLineType);
      }
      dialog.SetSelectedLineType(currentLineType);

      if (dialog.DoModal() == IDOK) {
        EoDbLineType* selectedLineType = dialog.GetSelectedLineType();
        if (selectedLineType != nullptr) {
          // Copy file-loaded linetypes into the document table
          if (dialog.IsSelectedFromFileList()) {
            EoDbLineType* existingLineType{};
            if (!document->LineTypeTable()->Lookup(selectedLineType->Name(), existingLineType)) {
              auto* clonedLineType = new EoDbLineType(*selectedLineType);
              document->LineTypeTable()->SetAt(clonedLineType->Name(), clonedLineType);
              selectedLineType = clonedLineType;
            } else {
              selectedLineType = existingLineType;
            }
          }
          Gs::renderState.SetLineType(static_cast<CDC*>(nullptr), static_cast<std::int16_t>(selectedLineType->Index()));
          Gs::renderState.SetLineTypeName(std::wstring(selectedLineType->Name()));
          auto* activeView = AeSysView::GetActiveView();
          if (activeView != nullptr) { activeView->UpdateStateInformation(AeSysView::Line); }
        }
      }
    }
    // Rebuild and restore selection to current render state (not "Load Line Types...")
    SetCurrentLineType(Gs::renderState.LineTypeIndex(), Gs::renderState.LineTypeName());
    return;
  }

  // ByLayer / ByBlock
  if (data == static_cast<DWORD_PTR>(EoDbPrimitive::LINETYPE_BYLAYER)) {
    Gs::renderState.SetLineType(static_cast<CDC*>(nullptr), EoDbPrimitive::LINETYPE_BYLAYER);
    Gs::renderState.SetLineTypeName(L"ByLayer");
    auto* activeView = AeSysView::GetActiveView();
    if (activeView != nullptr) { activeView->UpdateStateInformation(AeSysView::Line); }
    return;
  }
  if (data == static_cast<DWORD_PTR>(EoDbPrimitive::LINETYPE_BYBLOCK)) {
    Gs::renderState.SetLineType(static_cast<CDC*>(nullptr), EoDbPrimitive::LINETYPE_BYBLOCK);
    Gs::renderState.SetLineTypeName(L"ByBlock");
    auto* activeView = AeSysView::GetActiveView();
    if (activeView != nullptr) { activeView->UpdateStateInformation(AeSysView::Line); }
    return;
  }

  // Named line type — data is EoDbLineType* (only valid while a document is open)
  if (AeSysDoc::GetDoc() == nullptr) { return; }
  auto* lineType = reinterpret_cast<EoDbLineType*>(data);
  if (lineType != nullptr) {
    Gs::renderState.SetLineType(static_cast<CDC*>(nullptr), static_cast<std::int16_t>(lineType->Index()));
    Gs::renderState.SetLineTypeName(std::wstring(lineType->Name()));
    auto* activeView = AeSysView::GetActiveView();
    if (activeView != nullptr) { activeView->UpdateStateInformation(AeSysView::Line); }
  }
}

void EoCtrlLineTypeComboBox::DrawDashPreview(
    CDC* deviceContext, const CRect& rect, const EoDbLineType* lineType, COLORREF lineColor) {
  const int yCenter = rect.top + rect.Height() / 2;
  const double xStart = rect.left + 2.0;
  const double xEnd = rect.right - 2.0;
  if (xEnd <= xStart) { return; }

  CPen pen(PS_SOLID, 1, lineColor);
  CPen* oldPen = deviceContext->SelectObject(&pen);

  if (lineType == nullptr || lineType->DashElements().empty()) {
    // Continuous — solid line across full preview width
    deviceContext->MoveTo(static_cast<int>(xStart), yCenter);
    deviceContext->LineTo(static_cast<int>(xEnd), yCenter);
  } else {
    const auto& dashElements = lineType->DashElements();
    const double availableWidth = xEnd - xStart;

    // Compute total pattern length from absolute dash/gap values.
    double patternLength = lineType->GetPatternLength();
    if (patternLength < Eo::geometricTolerance) { patternLength = 1.0; }

    // Scale the pattern so it repeats ~3 times across the preview width.
    // This produces a consistent preview regardless of the drawing-unit scale
    // of the linetype definition (AeSys native vs DXF standard).
    constexpr double targetRepetitions = 3.0;
    const double scale = availableWidth / (patternLength * targetRepetitions);

    double x = xStart;
    while (x < xEnd) {
      for (const double len : dashElements) {
        double pixelLen = std::abs(len) * scale;
        if (pixelLen < 1.0) { pixelLen = 1.0; }
        if (len > 0.0) {
          deviceContext->MoveTo(static_cast<int>(x), yCenter);
          x += pixelLen;
          deviceContext->LineTo(static_cast<int>(std::min(x, xEnd)), yCenter);
        } else if (len < 0.0) {
          x += pixelLen;
        } else {
          // Dot — zero-length element
          deviceContext->SetPixel(static_cast<int>(x), yCenter, lineColor);
          x += 1.0;
        }
        if (x >= xEnd) { break; }
      }
    }
  }
  deviceContext->SelectObject(oldPen);
}

void EoCtrlLineTypeComboBox::OnDraw(CDC* deviceContext, const CRect& rect, CMFCToolBarImages* images, BOOL isHorz,
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

    CRect rectButton = m_rectButton;
    if (GetGlobalData()->m_bIsBlackHighContrast) { rectButton.DeflateRect(1, 1); }

    if (rectButton.left > rectCombo.left + 1) {
      CMFCVisualManager::GetInstance()->OnDrawComboDropButton(
          deviceContext, rectButton, isDisabled, m_pWndCombo->GetDroppedState(), isHighlighted, this);
    }

    const int curSel = CMFCToolBarComboBoxButton::GetCurSel();
    if (curSel >= 0) {
      const auto itemData = CMFCToolBarComboBoxButton::GetItemData(curSel);
      LPCTSTR itemText = CMFCToolBarComboBoxButton::GetItem(curSel);
      if (itemText == nullptr) { itemText = L""; }

      CRect rectContent = rectCombo;
      rectContent.right = m_rectButton.left;

      const bool isLoadEntry = (itemData == kLoadLineTypes);

      // Dash pattern preview area (~DPI-scaled 1" or 64px at 96 DPI)
      UINT dpi = ::GetDpiForSystem();
      if (dpi == 0) { dpi = 96; }
      const int previewWidth = ::MulDiv(64, dpi, 96);

      if (!isLoadEntry) {
        int previewRight = rectContent.left + 2 + previewWidth;
        if (previewRight > rectContent.right - 40) {
          previewRight = rectContent.right - 40;  // Leave room for text
        }
        const CRect previewRect(rectContent.left + 2, rectContent.top, previewRight, rectContent.bottom);

        // Determine the line type pointer — only dereference when a document is open
        // and the pointer is verified against the current document's line type table
        const EoDbLineType* lineType = nullptr;
        if (itemData != static_cast<DWORD_PTR>(EoDbPrimitive::LINETYPE_BYLAYER) &&
            itemData != static_cast<DWORD_PTR>(EoDbPrimitive::LINETYPE_BYBLOCK)) {
          auto* document = AeSysDoc::GetDoc();
          if (document != nullptr && !document->IsClosing()) {
            auto* candidate = reinterpret_cast<const EoDbLineType*>(itemData);
            // Validate pointer against current document's line type table
            auto* lineTypeTable = document->LineTypeTable();
            bool valid = false;
            if (lineTypeTable != nullptr) {
              POSITION pos = lineTypeTable->GetStartPosition();
              while (pos != nullptr) {
                CString name;
                EoDbLineType* tableEntry{};
                lineTypeTable->GetNextAssoc(pos, name, tableEntry);
                if (tableEntry == candidate) { valid = true; break; }
              }
            }
            if (valid) { lineType = candidate; }
          }
        }
        DrawDashPreview(deviceContext, previewRect, lineType, schemeColors.menuText);

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
      } else {
        // "Load Line Types..." — text only
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
}

// --- Owner-draw combo control ---

BEGIN_MESSAGE_MAP(EoCtrlLineTypeOwnerDrawCombo, CComboBox)
ON_WM_CTLCOLOR()
ON_WM_NCCALCSIZE()
ON_WM_PAINT()
ON_WM_NCPAINT()
ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

HBRUSH EoCtrlLineTypeOwnerDrawCombo::OnCtlColor(CDC* deviceContext, CWnd* control, UINT ctlColor) {
  if (ctlColor == CTLCOLOR_LISTBOX) {
    m_dropdownBackgroundBrush.DeleteObject();
    m_dropdownBackgroundBrush.CreateSolidBrush(Eo::chromeColors.menuBackground);
    deviceContext->SetBkColor(Eo::chromeColors.menuBackground);
    return static_cast<HBRUSH>(m_dropdownBackgroundBrush);
  }
  return CComboBox::OnCtlColor(deviceContext, control, ctlColor);
}

void EoCtrlLineTypeOwnerDrawCombo::OnNcCalcSize(BOOL /*calcValidRects*/, NCCALCSIZE_PARAMS* /*params*/) {
  // Forces entire window to client area for custom flat painting.
}

void EoCtrlLineTypeOwnerDrawCombo::OnPaint() {
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

void EoCtrlLineTypeOwnerDrawCombo::OnNcPaint() {
  CWindowDC dc(this);
  CRect windowRect;
  GetWindowRect(&windowRect);
  windowRect.OffsetRect(-windowRect.left, -windowRect.top);
  dc.FillSolidRect(&windowRect, Eo::chromeColors.paneBackground);
}

BOOL EoCtrlLineTypeOwnerDrawCombo::OnEraseBkgnd(CDC* /*deviceContext*/) { return TRUE; }

void EoCtrlLineTypeOwnerDrawCombo::MeasureItem(LPMEASUREITEMSTRUCT measureItemStruct) {
  UINT dpi = ::GetDpiForWindow(m_hWnd);
  if (dpi == 0) { dpi = 96; }
  measureItemStruct->itemHeight = ::MulDiv(18, dpi, 96);
}

void EoCtrlLineTypeOwnerDrawCombo::DrawItem(LPDRAWITEMSTRUCT drawItemStruct) {
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

  const bool isLoadEntry = (itemData == EoCtrlLineTypeComboBox::kLoadLineTypes);

  // Dash pattern preview area (~DPI-scaled 1" or 64px at 96 DPI)
  UINT dpi = ::GetDpiForWindow(m_hWnd);
  if (dpi == 0) { dpi = 96; }
  const int previewWidth = ::MulDiv(64, dpi, 96);

  if (!isLoadEntry) {
    int previewRight = itemRect.left + 2 + previewWidth;
    if (previewRight > itemRect.right - 40) { previewRight = itemRect.right - 40; }
    CRect previewRect(itemRect.left + 2, itemRect.top, previewRight, itemRect.bottom);

    // Only dereference EoDbLineType* when a document is open and the pointer
    // is verified against the current document's line type table.
    const EoDbLineType* lineType = nullptr;
    if (itemData != static_cast<DWORD_PTR>(EoDbPrimitive::LINETYPE_BYLAYER) &&
        itemData != static_cast<DWORD_PTR>(EoDbPrimitive::LINETYPE_BYBLOCK)) {
      auto* document = AeSysDoc::GetDoc();
      if (document != nullptr && !document->IsClosing()) {
        auto* candidate = reinterpret_cast<const EoDbLineType*>(itemData);
        auto* lineTypeTable = document->LineTypeTable();
        bool valid = false;
        if (lineTypeTable != nullptr) {
          POSITION pos = lineTypeTable->GetStartPosition();
          while (pos != nullptr) {
            CString name;
            EoDbLineType* tableEntry{};
            lineTypeTable->GetNextAssoc(pos, name, tableEntry);
            if (tableEntry == candidate) { valid = true; break; }
          }
        }
        if (valid) { lineType = candidate; }
      }
    }

    // ByLayer and ByBlock draw as continuous (null lineType)
    EoCtrlLineTypeComboBox::DrawDashPreview(&dc, previewRect, lineType, textColor);

    CRect textRect(previewRight + 4, itemRect.top, itemRect.right - 1, itemRect.bottom);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(textColor);
    dc.DrawText(itemText, textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  } else {
    // "Load Line Types..." — text only, no preview
    CRect textRect(itemRect.left + 3, itemRect.top, itemRect.right - 1, itemRect.bottom);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(textColor);
    dc.DrawText(itemText, textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  }

  // Highlight border for selected dropdown items
  if ((itemState & ODS_SELECTED) && !(itemState & ODS_COMBOBOXEDIT)) {
    CBrush highlightBrush(schemeColors.menuHighlightBorder);
    dc.FrameRect(itemRect, &highlightBrush);
  }

  dc.Detach();
}
