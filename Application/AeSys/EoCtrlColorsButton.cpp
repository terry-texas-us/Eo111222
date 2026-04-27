#include "Stdafx.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "EoCtrlColorsButton.h"

COLORREF* EoCtrlColorsButton::m_palette;
std::uint16_t EoCtrlColorsButton::m_currentIndex;
std::uint16_t EoCtrlColorsButton::m_selectedIndex;

IMPLEMENT_DYNAMIC(EoCtrlColorsButton, CMFCButton)

BEGIN_MESSAGE_MAP(EoCtrlColorsButton, CMFCButton)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_GETDLGCODE()
ON_WM_KEYDOWN()
ON_WM_LBUTTONUP()
ON_WM_MOUSEMOVE()
#pragma warning(pop)
ON_WM_PAINT()
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_SETFOCUS()
#pragma warning(pop)
END_MESSAGE_MAP()

static inline int ScaleInt(double scale, int value) {
  return static_cast<int>(std::lround(value * scale));
}

EoCtrlColorsButton::EoCtrlColorsButton() {
  m_layout = SimpleSingleRow;
  m_cellSize.cx = 8;
  m_cellSize.cy = 8;
  m_cellSpacing.cx = 1;
  m_cellSpacing.cy = 1;
  m_margins.cx = 3;
  m_margins.cy = 3;
  m_beginIndex = 1;
  m_endIndex = 1;

  m_subItem = 0;
}
EoCtrlColorsButton::~EoCtrlColorsButton() {}

void EoCtrlColorsButton::DrawCell(CDC* deviceContext, std::uint16_t index, COLORREF color) const {
  if (index == 0 || deviceContext == nullptr || m_palette == nullptr) { return; }

  CRect cellRectangle;
  SubItemRectangleByIndex(index, cellRectangle);

  // border thickness scaled for DPI
  double scale = 1.0;
  if (GetSafeHwnd()) {
    const auto dpi = static_cast<double>(GetDpiForSystem());
    scale = dpi / 96.0;
  }
  const int frameThickness = std::max(1, ScaleInt(scale, 1));

  if (index == m_currentIndex || index == m_selectedIndex) {
    CBrush frameBrush;
    if (index == m_currentIndex) {
      frameBrush.CreateSysColorBrush(COLOR_HIGHLIGHT);
    } else {
      frameBrush.CreateSysColorBrush(COLOR_WINDOWFRAME);
    }
    deviceContext->FrameRect(&cellRectangle, &frameBrush);
    cellRectangle.DeflateRect(frameThickness, frameThickness);
    CBrush innerFrameBrush;
    innerFrameBrush.CreateSysColorBrush(COLOR_BTNHIGHLIGHT);
    deviceContext->FrameRect(&cellRectangle, &innerFrameBrush);
    cellRectangle.DeflateRect(frameThickness, frameThickness);
  }
  CBrush brush(color);
  deviceContext->FillRect(&cellRectangle, &brush);
}

CSize EoCtrlColorsButton::SizeToContent(BOOL calculateOnly) {
  CRect beginRectangle;
  CRect endRectangle;
  CRect unionRectangle;
  SubItemRectangleByIndex(m_beginIndex, beginRectangle);
  SubItemRectangleByIndex(m_endIndex, endRectangle);
  unionRectangle.UnionRect(beginRectangle, endRectangle);
  CSize size = unionRectangle.Size();

  // Apply scaling to spacing/margins when computing final size
  double scale = 1.0;
  if (GetSafeHwnd()) {
    const auto dpi = static_cast<double>(GetDpiForSystem());
    scale = dpi / 96.0;
  }
  const int scaledSpacingX = ScaleInt(scale, m_cellSpacing.cx);
  const int scaledMarginX = ScaleInt(scale, m_margins.cx);
  const int scaledSpacingY = ScaleInt(scale, m_cellSpacing.cy);
  const int scaledMarginY = ScaleInt(scale, m_margins.cy);

  size.cx += 2 * (scaledSpacingX + scaledMarginX);
  size.cy += 2 * (scaledSpacingY + scaledMarginY);
  if (!calculateOnly) {
    CRect clientRectangle;
    GetWindowRect(clientRectangle);
    const CWnd* const parent = GetParent();
    if (parent) {
      parent->ScreenToClient(clientRectangle);
      clientRectangle.right = clientRectangle.left + size.cx;
      clientRectangle.bottom = clientRectangle.top + size.cy;

      MoveWindow(clientRectangle);
    }
  }
  return size;
}

void EoCtrlColorsButton::SubItemRectangleByIndex(std::uint16_t index, CRect& rectangle) const {
  double scale{1.0};
  if (GetSafeHwnd()) {
    const auto dpi = static_cast<double>(GetDpiForSystem());
    scale = dpi / 96.0;
  }

  const int xMargins = ScaleInt(scale, m_margins.cx);
  const int yMargins = ScaleInt(scale, m_margins.cy);
  const int xCellSpacing = ScaleInt(scale, m_cellSpacing.cx);
  const int yCellSpacing = ScaleInt(scale, m_cellSpacing.cy);
  const int xCellSize = ScaleInt(scale, m_cellSize.cx);
  const int yCellSize = ScaleInt(scale, m_cellSize.cy);
  rectangle.top = yMargins + yCellSpacing;
  rectangle.left = xMargins + xCellSpacing;

  switch (m_layout) {
    case SimpleSingleRow:
      rectangle.left += (index - m_beginIndex) * (xCellSize + xCellSpacing);
      break;
    case GridDown5RowsOddOnly:
      rectangle.top += (((index - m_beginIndex) % 10) / 2) * (yCellSize + yCellSpacing);
      rectangle.left += ((index - m_beginIndex) / 10) * (xCellSize + xCellSpacing);
      break;
    case GridUp5RowsEvenOnly:
      rectangle.top += (4 - ((index - m_beginIndex) % 10) / 2) * (yCellSize + yCellSpacing);
      rectangle.left += ((index - m_beginIndex) / 10) * (xCellSize + xCellSpacing);
  }
  rectangle.bottom = rectangle.top + yCellSize;
  rectangle.right = rectangle.left + xCellSize;
}

std::uint16_t EoCtrlColorsButton::SubItemByPoint(const CPoint& point) const {
  CRect rectangle;
  rectangle.SetRectEmpty();

  switch (m_layout) {
    case SimpleSingleRow:
      for (auto index = m_beginIndex; index <= m_endIndex; index++) {
        SubItemRectangleByIndex(index, rectangle);
        if (rectangle.PtInRect(point) == TRUE) { return index; }
      }
      break;
    case GridDown5RowsOddOnly:
      for (auto index = m_beginIndex; index <= m_endIndex; index++) {
        if ((index % 2) != 0) {
          SubItemRectangleByIndex(index, rectangle);
          if (rectangle.PtInRect(point) == TRUE) { return index; }
        }
      }
      break;
    case GridUp5RowsEvenOnly:
      for (auto index = m_beginIndex; index <= m_endIndex; index++) {
        if ((index % 2) == 0) {
          SubItemRectangleByIndex(index, rectangle);
          if (rectangle.PtInRect(point) == TRUE) { return index; }
        }
      }
  }
  return 0;
}

void EoCtrlColorsButton::OnDraw(CDC* deviceContext, const CRect& /*rectangle */, UINT /* state */) {
  m_selectedIndex = 0;

  if (m_palette == nullptr || deviceContext == nullptr) { return; }

  for (auto index = m_beginIndex; index <= m_endIndex; index++) {
    if (m_layout == SimpleSingleRow) {
      DrawCell(deviceContext, index, m_palette[index]);
    } else if (m_layout == GridDown5RowsOddOnly && ((index % 2) != 0)) {
      DrawCell(deviceContext, index, m_palette[index]);
    } else if (m_layout == GridUp5RowsEvenOnly && ((index % 2) == 0)) {
      DrawCell(deviceContext, index, m_palette[index]);
    }
  }
}

UINT EoCtrlColorsButton::OnGetDlgCode() {
  return DLGC_WANTARROWS;
}

void EoCtrlColorsButton::OnKeyDown(UINT keyCode, UINT repeatCount, UINT flags) {
  if (keyCode >= VK_LEFT && keyCode <= VK_DOWN) {
    auto* deviceContext = GetDC();
    if (deviceContext) {
      m_selectedIndex = 0;
      if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
        DrawCell(deviceContext, m_subItem, m_palette[m_subItem]);
      }

      if (m_layout == SimpleSingleRow) {
        switch (keyCode) {
          case VK_RIGHT:
            m_subItem++;
            break;
          case VK_LEFT:
            m_subItem--;
            break;
        }
      } else if (m_layout == GridDown5RowsOddOnly) {
        switch (keyCode) {
          case VK_DOWN:
            m_subItem += 2;
            break;
          case VK_RIGHT:
            m_subItem += 10;
            break;
          case VK_LEFT:
            m_subItem -= 10;
            break;
          case VK_UP:
            m_subItem -= 2;
            break;
        }
      } else if (m_layout == GridUp5RowsEvenOnly) {
        switch (keyCode) {
          case VK_DOWN:
            m_subItem -= 2;
            break;
          case VK_RIGHT:
            m_subItem += 10;
            break;
          case VK_LEFT:
            m_subItem -= 10;
            break;
          case VK_UP:
            m_subItem += 2;
            break;
        }
      }
      m_subItem = std::max(m_beginIndex, std::min(m_endIndex, m_subItem));

      CRect currentSubItemRectangle;
      SubItemRectangleByIndex(m_subItem, currentSubItemRectangle);

      m_selectedIndex = m_subItem;
      if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
        DrawCell(deviceContext, m_subItem, m_palette[m_subItem]);
      }
      ReleaseDC(deviceContext);

      NMHDR notifyStructure{};
      notifyStructure.hwndFrom = GetSafeHwnd();
      notifyStructure.idFrom = static_cast<uint64_t>(GetDlgCtrlID());
      ::SendMessageW(GetParent()->GetSafeHwnd(),
          WM_NOTIFY,
          static_cast<WPARAM>(notifyStructure.idFrom),
          reinterpret_cast<LPARAM>(&notifyStructure));
    }
  }
  CMFCButton::OnKeyDown(keyCode, repeatCount, flags);
}

void EoCtrlColorsButton::OnLButtonUp(UINT flags, CPoint point) {
  const auto currentSubItem = SubItemByPoint(point);
  if (currentSubItem != 0) { m_subItem = currentSubItem; }
  CMFCButton::OnLButtonUp(flags, point);
}

void EoCtrlColorsButton::OnMouseMove(UINT flags, CPoint point) {
  auto* deviceContext = GetDC();
  if (deviceContext) {
    m_selectedIndex = 0;
    if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
      DrawCell(deviceContext, m_subItem, m_palette[m_subItem]);
    }

    m_subItem = SubItemByPoint(point);

    if (m_subItem != 0) {
      m_selectedIndex = m_subItem;

      if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
        DrawCell(deviceContext, m_subItem, m_palette[m_subItem]);
      }

      NMHDR notifyStructure{};
      notifyStructure.hwndFrom = GetSafeHwnd();
      notifyStructure.idFrom = static_cast<std::uint64_t>(GetDlgCtrlID());

      ::SendMessageW(GetParent()->GetSafeHwnd(),
          WM_NOTIFY,
          static_cast<WPARAM>(notifyStructure.idFrom),
          reinterpret_cast<LPARAM>(&notifyStructure));
    }
    ReleaseDC(deviceContext);
  }
  CMFCButton::OnMouseMove(flags, point);
}

void EoCtrlColorsButton::OnPaint() {
  CMFCButton::OnPaint();
}

void EoCtrlColorsButton::OnSetFocus(CWnd* oldWindow) {
  CMFCButton::OnSetFocus(oldWindow);

  auto* deviceContext = GetDC();
  if (!deviceContext) { return; }

  if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
    DrawCell(deviceContext, m_subItem, m_palette[m_subItem]);
  }
  m_subItem = m_beginIndex;
  CRect currentSubItemRectangle;
  SubItemRectangleByIndex(m_subItem, currentSubItemRectangle);

  m_selectedIndex = m_subItem;
  if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
    DrawCell(deviceContext, m_subItem, m_palette[m_subItem]);
  }
  ReleaseDC(deviceContext);

  NMHDR notifyStructure{};
  notifyStructure.hwndFrom = GetSafeHwnd();
  notifyStructure.idFrom = static_cast<std::uint64_t>(GetDlgCtrlID());

  ::SendMessageW(GetParent()->GetSafeHwnd(), WM_NOTIFY, (WPARAM)notifyStructure.idFrom, (LPARAM)&notifyStructure);
}
