#include "StdAfx.h"

#include <algorithm>

#include "Eo.h"
#include "EoMfVisualManager.h"

IMPLEMENT_DYNCREATE(EoMfVisualManager, CMFCVisualManagerOffice2007)

EoMfVisualManager::EoMfVisualManager() {
  // Start from Office2007 ObsidianBlack as the baseline
  CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);

  RefreshColors();
}

EoMfVisualManager::~EoMfVisualManager() {}

void EoMfVisualManager::RefreshColors() {
  const auto& colors = Eo::chromeColors;

  // Recreate cached brushes
  m_toolbarBackgroundBrush.DeleteObject();
  m_toolbarBackgroundBrush.CreateSolidBrush(colors.toolbarBackground);

  m_menuBackgroundBrush.DeleteObject();
  m_menuBackgroundBrush.CreateSolidBrush(colors.menuBackground);

  m_captionBrush.DeleteObject();
  m_captionBrush.CreateSolidBrush(colors.captionBackground);

  m_captionActiveBrush.DeleteObject();
  m_captionActiveBrush.CreateSolidBrush(colors.captionActiveBackground);

  m_tabActiveBrush.DeleteObject();
  m_tabActiveBrush.CreateSolidBrush(colors.tabActiveBackground);

  m_tabInactiveBrush.DeleteObject();
  m_tabInactiveBrush.CreateSolidBrush(colors.tabInactiveBackground);

  m_statusBarBrush.DeleteObject();
  m_statusBarBrush.CreateSolidBrush(colors.statusBarBackground);

  m_menuHighlightBrush.DeleteObject();
  m_menuHighlightBrush.CreateSolidBrush(colors.menuHighlightBackground);

  // Recreate cached pens
  m_borderPen.DeleteObject();
  m_borderPen.CreatePen(PS_SOLID, 1, colors.borderColor);

  m_separatorPen.DeleteObject();
  m_separatorPen.CreatePen(PS_SOLID, 1, colors.separatorColor);

  m_menuHighlightBorderPen.DeleteObject();
  m_menuHighlightBorderPen.CreatePen(PS_SOLID, 1, colors.menuHighlightBorder);
}

// --- Bar / toolbar background ---

void EoMfVisualManager::OnDrawBarGripper(
    CDC* /*deviceContext*/, CRect /*rectGripper*/, BOOL /*isHorz*/, CBasePane* /*bar*/) {
  // No-op: suppresses gripper dots for a clean, modern toolbar appearance.
}

void EoMfVisualManager::OnFillBarBackground(
    CDC* deviceContext, CBasePane* bar, CRect rectClient, CRect rectClip, BOOL /*ncArea*/) {
  if (bar->IsKindOf(RUNTIME_CLASS(CMFCStatusBar))) {
    deviceContext->FillRect(rectClip.IsRectEmpty() ? rectClient : rectClip, &m_statusBarBrush);
    return;
  }
  // Popup dropdown menus use the deeper menu background for overlay depth; the menu BAR
  // itself falls through to the toolbar brush for a unified chrome header.
  if (bar->IsKindOf(RUNTIME_CLASS(CMFCPopupMenuBar))) {
    deviceContext->FillRect(rectClip.IsRectEmpty() ? rectClient : rectClip, &m_menuBackgroundBrush);
    return;
  }
  // Toolbar and all other bar types
  deviceContext->FillRect(rectClip.IsRectEmpty() ? rectClient : rectClip, &m_toolbarBackgroundBrush);
}

// --- Tabs ---

void EoMfVisualManager::OnEraseTabsArea(CDC* deviceContext, CRect rect, const CMFCBaseTabCtrl* /*tabWnd*/) {
  deviceContext->FillRect(rect, &m_toolbarBackgroundBrush);
}

BOOL EoMfVisualManager::OnEraseTabsFrame(CDC* deviceContext, CRect rect, const CMFCBaseTabCtrl* /*tabWnd*/) {
  deviceContext->FillRect(rect, &m_toolbarBackgroundBrush);
  return TRUE;
}

// --- MDI client area ---

BOOL EoMfVisualManager::OnEraseMDIClientArea(CDC* deviceContext, CRect rectClient) {
  deviceContext->FillRect(rectClient, &m_toolbarBackgroundBrush);
  return TRUE;
}

int EoMfVisualManager::GetMDITabsBordersSize() {
  return 0;  // Suppress the default 3D sunken edge around the MDI tab content area
}

// --- Docking pane caption ---

COLORREF EoMfVisualManager::OnDrawPaneCaption(
    CDC* deviceContext, CDockablePane* /*bar*/, BOOL active, CRect rectCaption, CRect /*rectButtons*/) {
  const auto& colors = Eo::chromeColors;

  // Active captions use warm elevated color (not accent blue) to avoid jarring flash on focus change
  CBrush& brush = active ? m_tabActiveBrush : m_captionBrush;
  deviceContext->FillRect(rectCaption, &brush);

  return active ? colors.captionActiveText : colors.captionText;
}

// --- Tabs ---

void EoMfVisualManager::OnDrawTab(CDC* deviceContext, CRect rectTab, int tabIndex, BOOL isActive,
    const CMFCBaseTabCtrl* tabWnd) {
  const auto& colors = Eo::chromeColors;

  // Clip to tab bounds to prevent bleeding into adjacent tabs
  int savedDC = deviceContext->SaveDC();
  deviceContext->IntersectClipRect(rectTab);

  // Fill tab background
  CBrush& brush = isActive ? m_tabActiveBrush : m_tabInactiveBrush;
  deviceContext->FillRect(rectTab, &brush);

  // Draw tab border
  CPen* previousPen = deviceContext->SelectObject(&m_borderPen);
  CBrush* previousBrush = static_cast<CBrush*>(deviceContext->SelectStockObject(NULL_BRUSH));
  deviceContext->Rectangle(rectTab);
  deviceContext->SelectObject(previousBrush);

  // Active tab gets a 2px accent bottom border for clear distinction
  if (isActive) {
    CPen accentPen(PS_SOLID, 2, colors.captionActiveBackground);
    deviceContext->SelectObject(&accentPen);
    deviceContext->MoveTo(rectTab.left + 1, rectTab.bottom - 1);
    deviceContext->LineTo(rectTab.right - 1, rectTab.bottom - 1);
  }
  deviceContext->SelectObject(previousPen);

  // Draw the active tab close button and tab label
  if (tabWnd != nullptr) {
    CRect textRect = rectTab;
    textRect.DeflateRect(4, 0);

    // If the active tab has an inline close button, draw it and shrink the text rect
    if (isActive && tabWnd->IsActiveTabCloseButton()) {
      CRect rectClose = tabWnd->GetTabCloseButton();
      if (!rectClose.IsRectEmpty()) {
        textRect.right = rectClose.left;
        OnDrawTabCloseButton(deviceContext, rectClose, tabWnd, tabWnd->IsTabCloseButtonHighlighted(),
            tabWnd->IsTabCloseButtonPressed(), FALSE);
      }
    }

    CString tabText;
    tabWnd->GetTabLabel(tabIndex, tabText);

    if (!tabText.IsEmpty()) {
      auto previousTextColor =
          deviceContext->SetTextColor(isActive ? colors.tabActiveText : colors.tabInactiveText);
      auto previousBkMode = deviceContext->SetBkMode(TRANSPARENT);

      deviceContext->DrawText(tabText, textRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

      deviceContext->SetBkMode(previousBkMode);
      deviceContext->SetTextColor(previousTextColor);
    }
  }
  deviceContext->RestoreDC(savedDC);
}

void EoMfVisualManager::OnFillTab(CDC* deviceContext, CRect rectFill, CBrush* /*brFill*/, int /*tabIndex*/,
    BOOL isActive, const CMFCBaseTabCtrl* /*tabWnd*/) {
  CBrush& brush = isActive ? m_tabActiveBrush : m_tabInactiveBrush;
  deviceContext->FillRect(rectFill, &brush);
}

void EoMfVisualManager::OnDrawTabCloseButton(CDC* deviceContext, CRect rect,
    const CMFCBaseTabCtrl* /*tabWnd*/, BOOL isHighlighted, BOOL isPressed, BOOL /*isDisabled*/) {
  const auto& colors = Eo::chromeColors;

  // Fill button background on hover/press
  if (isPressed) {
    CBrush pressBrush(colors.menuHighlightBackground);
    deviceContext->FillRect(rect, &pressBrush);
  } else if (isHighlighted) {
    CBrush hoverBrush(colors.menuHighlightBackground);
    deviceContext->FillRect(rect, &hoverBrush);
  }

  // DPI-aware pen width for the X glyph
  UINT dpi = ::GetDpiForWindow(deviceContext->GetWindow()->GetSafeHwnd());
  int penWidth = (std::max)(1, static_cast<int>(dpi) / 96);

  // Draw the X glyph — use paneText for normal visibility, tabActiveText on hover
  COLORREF xColor = (isHighlighted || isPressed) ? colors.tabActiveText : colors.paneText;
  CPen xPen(PS_SOLID, penWidth, xColor);
  CPen* previousPen = deviceContext->SelectObject(&xPen);

  // Center the X within the button rect, sized proportionally
  int margin = (std::max)(2, rect.Width() / 4);
  int left = rect.left + margin;
  int top = rect.top + margin;
  int right = rect.right - margin;
  int bottom = rect.bottom - margin;

  deviceContext->MoveTo(left, top);
  deviceContext->LineTo(right, bottom);
  deviceContext->MoveTo(right, top);
  deviceContext->LineTo(left, bottom);

  deviceContext->SelectObject(previousPen);
}

COLORREF EoMfVisualManager::GetTabTextColor(
    const CMFCBaseTabCtrl* /*tabWnd*/, int /*tabIndex*/, BOOL isActive) {
  const auto& colors = Eo::chromeColors;
  return isActive ? colors.tabActiveText : colors.tabInactiveText;
}

// --- Toolbar button text ---

COLORREF EoMfVisualManager::GetToolbarButtonTextColor(
    CMFCToolBarButton* button, CMFCVisualManager::AFX_BUTTON_STATE /*state*/) {
  const auto& colors = Eo::chromeColors;

  if (button != nullptr && button->IsKindOf(RUNTIME_CLASS(CMFCToolBarMenuButton))) {
    // Menu bar top-level items use menu text color
    return colors.menuText;
  }
  return colors.paneText;
}

// --- Toolbar / caption buttons ---

void EoMfVisualManager::OnFillButtonInterior(CDC* deviceContext, CMFCToolBarButton* /*button*/, CRect rect,
    CMFCVisualManager::AFX_BUTTON_STATE state) {
  const auto& colors = Eo::chromeColors;

  if (state == ButtonsIsPressed) {
    CBrush pressBrush(colors.menuHighlightBackground);
    deviceContext->FillRect(rect, &pressBrush);
  } else if (state == ButtonsIsHighlighted) {
    CBrush hoverBrush(colors.menuHighlightBackground);
    deviceContext->FillRect(rect, &hoverBrush);
  }
  // Normal state: no fill — let the bar background show through
}

void EoMfVisualManager::OnDrawButtonBorder(CDC* deviceContext, CMFCToolBarButton* /*button*/, CRect rect,
    CMFCVisualManager::AFX_BUTTON_STATE state) {
  const auto& colors = Eo::chromeColors;

  if (state == ButtonsIsHighlighted || state == ButtonsIsPressed) {
    CPen borderPen(PS_SOLID, 1, colors.menuHighlightBorder);
    CPen* previousPen = deviceContext->SelectObject(&borderPen);
    CBrush* previousBrush = static_cast<CBrush*>(deviceContext->SelectStockObject(NULL_BRUSH));
    deviceContext->Rectangle(rect);
    deviceContext->SelectObject(previousBrush);
    deviceContext->SelectObject(previousPen);
  }
  // Normal state: no border — MFC's default disabled image rendering handles dimming.
}

void EoMfVisualManager::OnDrawCaptionButton(CDC* deviceContext, CMFCCaptionButton* button, BOOL /*active*/,
    BOOL /*isHorz*/, BOOL /*isMaximized*/, BOOL isDisabled, int /*imageId*/) {
  const auto& colors = Eo::chromeColors;

  CRect rect = button->GetRect();

  // Fill background on hover/press
  if (button->m_bPushed && !isDisabled) {
    CBrush pressBrush(colors.menuHighlightBackground);
    deviceContext->FillRect(rect, &pressBrush);
  } else if (button->m_bFocused && !isDisabled) {
    CBrush hoverBrush(colors.menuHighlightBackground);
    deviceContext->FillRect(rect, &hoverBrush);
  }

  // Choose glyph color
  COLORREF glyphColor = isDisabled ? colors.borderColor
      : (button->m_bFocused || button->m_bPushed) ? colors.tabActiveText
                                                   : colors.captionText;

  // DPI-aware pen width for glyph strokes
  UINT dpi = ::GetDpiForWindow(deviceContext->GetWindow()->GetSafeHwnd());
  int penWidth = (std::max)(1, static_cast<int>(dpi) / 96);

  // Draw the glyph — pin (AFX_HTLEFTBUTTON) or close (AFX_HTCLOSE)
  CPen glyphPen(PS_SOLID, penWidth, glyphColor);
  CPen* previousPen = deviceContext->SelectObject(&glyphPen);

  int margin = (std::max)(4, rect.Width() / 4);
  int left = rect.left + margin;
  int top = rect.top + margin;
  int right = rect.right - margin;
  int bottom = rect.bottom - margin;
  int centerX = rect.CenterPoint().x;
  int centerY = rect.CenterPoint().y;

  UINT hitTest = button->GetHit();
  if (hitTest == AFX_HTCLOSE) {
    // X glyph
    deviceContext->MoveTo(left, top);
    deviceContext->LineTo(right, bottom);
    deviceContext->MoveTo(right, top);
    deviceContext->LineTo(left, bottom);
  } else {
    // Pin glyph — pushpin viewed from the side (auto-hide toggle)
    // The pin head (flat bar across the top)
    deviceContext->MoveTo(left, top);
    deviceContext->LineTo(right, top);
    // Two vertical legs from the head down to the midline
    deviceContext->MoveTo(left + penWidth, top);
    deviceContext->LineTo(left + penWidth, centerY);
    deviceContext->MoveTo(right - penWidth, top);
    deviceContext->LineTo(right - penWidth, centerY);
    // Horizontal collar across the midline
    deviceContext->MoveTo(left, centerY);
    deviceContext->LineTo(right, centerY);
    // Single spike from center of collar down to bottom
    deviceContext->MoveTo(centerX, centerY);
    deviceContext->LineTo(centerX, bottom);
  }
  deviceContext->SelectObject(previousPen);
}

// --- Toolbar combo boxes ---

void EoMfVisualManager::OnDrawComboBorder(CDC* deviceContext, CRect rect, BOOL /*isDisabled*/,
    BOOL /*isDropped*/, BOOL /*isHighlighted*/, CMFCToolBarComboBoxButton* /*button*/) {
  const auto& colors = Eo::chromeColors;

  // Fill entire combo button area with paneBackground for a flat, dark appearance.
  CBrush fillBrush(colors.paneBackground);
  deviceContext->FillRect(rect, &fillBrush);

  // Draw subtle 1px border matching the scheme's structural border color.
  CPen* previousPen = deviceContext->SelectObject(&m_borderPen);
  CBrush* previousBrush = static_cast<CBrush*>(deviceContext->SelectStockObject(NULL_BRUSH));
  deviceContext->Rectangle(rect);
  deviceContext->SelectObject(previousBrush);
  deviceContext->SelectObject(previousPen);
}

void EoMfVisualManager::OnDrawComboDropButton(CDC* deviceContext, CRect rect, BOOL /*isDisabled*/,
    BOOL /*isDropped*/, BOOL /*isHighlighted*/, CMFCToolBarComboBoxButton* /*button*/) {
  const auto& colors = Eo::chromeColors;

  // Fill with paneBackground — flat, no 3D effect.
  CBrush fillBrush(colors.paneBackground);
  deviceContext->FillRect(rect, &fillBrush);

  // DPI-aware downward-pointing arrow glyph.
  UINT dpi = ::GetDpiForSystem();
  int arrowWidth = ::MulDiv(7, dpi, 96) | 1;  // Ensure odd for centered tip
  int arrowHeight = ::MulDiv(4, dpi, 96);
  if (arrowHeight < 3) { arrowHeight = 3; }

  int centerX = (rect.left + rect.right) / 2;
  int centerY = (rect.top + rect.bottom) / 2;
  int left = centerX - arrowWidth / 2;
  int top = centerY - arrowHeight / 2;

  POINT points[3] = {{left, top}, {left + arrowWidth, top}, {centerX, top + arrowHeight}};
  CBrush arrowBrush(colors.menuText);
  CPen arrowPen(PS_SOLID, 1, colors.menuText);
  auto* oldBrush = deviceContext->SelectObject(&arrowBrush);
  auto* oldPen = deviceContext->SelectObject(&arrowPen);
  deviceContext->Polygon(points, 3);
  deviceContext->SelectObject(oldBrush);
  deviceContext->SelectObject(oldPen);
}

// --- Status bar ---

void EoMfVisualManager::OnDrawStatusBarPaneBorder(
    CDC* /*deviceContext*/, CMFCStatusBar* /*bar*/, CRect /*rectPane*/, UINT /*id*/, UINT /*style*/) {
  // No-op: flat status bar with no visible pane borders.
  // The base status bar background is painted by OnFillBarBackground.
  // Per-pane backgrounds (from SetPaneBackgroundColor) are painted by MFC's DoPaint
  // BEFORE this override is called — filling here would erase them.
}

// --- Separators ---

void EoMfVisualManager::OnDrawSeparator(CDC* deviceContext, CBasePane* /*bar*/, CRect rect, BOOL isHorizontal) {
  CPen* previousPen = deviceContext->SelectObject(&m_separatorPen);

  if (isHorizontal) {
    int centerY = rect.CenterPoint().y;
    deviceContext->MoveTo(rect.left, centerY);
    deviceContext->LineTo(rect.right, centerY);
  } else {
    int centerX = rect.CenterPoint().x;
    deviceContext->MoveTo(centerX, rect.top);
    deviceContext->LineTo(centerX, rect.bottom);
  }
  deviceContext->SelectObject(previousPen);
}

// --- Menu ---

void EoMfVisualManager::OnDrawMenuBorder(CDC* deviceContext, CMFCPopupMenu* /*menu*/, CRect rect) {
  CPen* previousPen = deviceContext->SelectObject(&m_borderPen);
  CBrush* previousBrush = static_cast<CBrush*>(deviceContext->SelectStockObject(NULL_BRUSH));
  deviceContext->Rectangle(rect);
  deviceContext->SelectObject(previousBrush);
  deviceContext->SelectObject(previousPen);
}

void EoMfVisualManager::OnFillMenuImageRect(
    CDC* deviceContext, CMFCToolBarButton* /*button*/, CRect rect, CMFCVisualManager::AFX_BUTTON_STATE /*state*/) {
  deviceContext->FillRect(rect, &m_menuBackgroundBrush);
}

void EoMfVisualManager::OnHighlightMenuItem(
    CDC* deviceContext, CMFCToolBarMenuButton* /*button*/, CRect rect, COLORREF& highlightTextColor) {
  const auto& colors = Eo::chromeColors;

  deviceContext->FillRect(rect, &m_menuHighlightBrush);

  // Draw highlight border
  CPen* previousPen = deviceContext->SelectObject(&m_menuHighlightBorderPen);
  CBrush* previousBrush = static_cast<CBrush*>(deviceContext->SelectStockObject(NULL_BRUSH));
  deviceContext->Rectangle(rect);
  deviceContext->SelectObject(previousBrush);
  deviceContext->SelectObject(previousPen);

  highlightTextColor = colors.menuText;
}

COLORREF EoMfVisualManager::OnDrawMenuLabel(CDC* deviceContext, CRect rect) {
  deviceContext->FillRect(rect, &m_menuBackgroundBrush);
  return Eo::chromeColors.menuText;
}

COLORREF EoMfVisualManager::GetMenuItemTextColor(
    [[maybe_unused]] CMFCToolBarMenuButton* button, [[maybe_unused]] BOOL isHighlighted, BOOL isDisabled) {
  if (isDisabled) {
    return Eo::chromeColors.captionText;  // tertiary text for disabled items
  }
  return Eo::chromeColors.menuText;
}

// --- Pane borders and dividers ---

void EoMfVisualManager::OnDrawPaneBorder(CDC* deviceContext, CBasePane* /*bar*/, CRect& rect) {
  CPen* previousPen = deviceContext->SelectObject(&m_borderPen);
  CBrush* previousBrush = static_cast<CBrush*>(deviceContext->SelectStockObject(NULL_BRUSH));
  deviceContext->Rectangle(rect);
  deviceContext->SelectObject(previousBrush);
  deviceContext->SelectObject(previousPen);
}

void EoMfVisualManager::OnDrawPaneDivider(
    CDC* deviceContext, CPaneDivider* /*divider*/, CRect rect, BOOL /*autoHideMode*/) {
  CBrush borderBrush(Eo::chromeColors.borderColor);
  deviceContext->FillRect(rect, &borderBrush);
}
