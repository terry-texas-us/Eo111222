#include "Stdafx.h"

#include <Windows.h>

#include <afx.h>
#include <afxbutton.h>
#include <afxmsg_.h>
#include <afxwin.h>
#include <atltypes.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "EoCtrlColorsButton.h"

COLORREF* EoCtrlColorsButton::m_palette;
EoUInt16 EoCtrlColorsButton::m_currentIndex;
EoUInt16 EoCtrlColorsButton::m_selectedIndex;

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

static inline int ScaleInt(double scale, int value) { return static_cast<int>(std::lround(value * scale)); }

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

void EoCtrlColorsButton::DrawCell(CDC* deviceContext, EoUInt16 index, COLORREF color) const {
  if (index == 0 || deviceContext == nullptr || m_palette == nullptr) { return; }

  CRect CellRectangle;
  SubItemRectangleByIndex(index, CellRectangle);

  // border thickness scaled for DPI
  double scale = 1.0;
  if (GetSafeHwnd()) {
    auto dpi = static_cast<double>(GetDpiForSystem());
    scale = dpi / 96.0;
  }
  int frameThickness = std::max(1, ScaleInt(scale, 1));

  if (index == m_currentIndex || index == m_selectedIndex) {
    CBrush FrameBrush;
    if (index == m_currentIndex) {
      FrameBrush.CreateSysColorBrush(COLOR_HIGHLIGHT);
    } else {
      FrameBrush.CreateSysColorBrush(COLOR_WINDOWFRAME);
    }
    deviceContext->FrameRect(&CellRectangle, &FrameBrush);
    CellRectangle.DeflateRect(frameThickness, frameThickness);
    CBrush InnerFrameBrush;
    InnerFrameBrush.CreateSysColorBrush(COLOR_BTNHIGHLIGHT);
    deviceContext->FrameRect(&CellRectangle, &InnerFrameBrush);
    CellRectangle.DeflateRect(frameThickness, frameThickness);
  }
  CBrush Brush(color);
  deviceContext->FillRect(&CellRectangle, &Brush);
}

CSize EoCtrlColorsButton::SizeToContent(BOOL calculateOnly) {
  CRect BeginRectangle;
  CRect EndRectangle;
  CRect UnionRectangle;
  SubItemRectangleByIndex(m_beginIndex, BeginRectangle);
  SubItemRectangleByIndex(m_endIndex, EndRectangle);
  UnionRectangle.UnionRect(BeginRectangle, EndRectangle);
  CSize Size = UnionRectangle.Size();

  // Apply scaling to spacing/margins when computing final size
  double scale = 1.0;
  if (GetSafeHwnd()) {
    auto dpi = static_cast<double>(GetDpiForSystem());
    scale = dpi / 96.0;
  }
  int scaledSpacingX = ScaleInt(scale, m_cellSpacing.cx);
  int scaledMarginX = ScaleInt(scale, m_margins.cx);
  int scaledSpacingY = ScaleInt(scale, m_cellSpacing.cy);
  int scaledMarginY = ScaleInt(scale, m_margins.cy);

  Size.cx += 2 * (scaledSpacingX + scaledMarginX);
  Size.cy += 2 * (scaledSpacingY + scaledMarginY);
  if (!calculateOnly) {
    CRect ClientRectangle;
    GetWindowRect(ClientRectangle);
    CWnd* parent = GetParent();
    if (parent) {
      parent->ScreenToClient(ClientRectangle);

      ClientRectangle.right = ClientRectangle.left + Size.cx;
      ClientRectangle.bottom = ClientRectangle.top + Size.cy;

      MoveWindow(ClientRectangle);
    }
  }
  return Size;
}

void EoCtrlColorsButton::SubItemRectangleByIndex(EoUInt16 index, CRect& rectangle) const {
  double scale{1.0};
  if (GetSafeHwnd()) {
    auto dpi = static_cast<double>(GetDpiForSystem());
    scale = dpi / 96.0;
  }

  int xMargins = ScaleInt(scale, m_margins.cx);
  int yMargins = ScaleInt(scale, m_margins.cy);
  int xCellSpacing = ScaleInt(scale, m_cellSpacing.cx);
  int yCellSpacing = ScaleInt(scale, m_cellSpacing.cy);
  int xCellSize = ScaleInt(scale, m_cellSize.cx);
  int yCellSize = ScaleInt(scale, m_cellSize.cy);

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

EoUInt16 EoCtrlColorsButton::SubItemByPoint(const CPoint& point) const {
  CRect Rectangle;
  Rectangle.SetRectEmpty();

  switch (m_layout) {
    case SimpleSingleRow:
      for (EoUInt16 Index = m_beginIndex; Index <= m_endIndex; Index++) {
        SubItemRectangleByIndex(Index, Rectangle);
        if (Rectangle.PtInRect(point) == TRUE) { return Index; }
      }
      break;
    case GridDown5RowsOddOnly:
      for (EoUInt16 Index = m_beginIndex; Index <= m_endIndex; Index++) {
        if ((Index % 2) != 0) {
          SubItemRectangleByIndex(Index, Rectangle);
          if (Rectangle.PtInRect(point) == TRUE) { return Index; }
        }
      }
      break;
    case GridUp5RowsEvenOnly:
      for (EoUInt16 Index = m_beginIndex; Index <= m_endIndex; Index++) {
        if ((Index % 2) == 0) {
          SubItemRectangleByIndex(Index, Rectangle);
          if (Rectangle.PtInRect(point) == TRUE) { return Index; }
        }
      }
  }
  return 0;
}

void EoCtrlColorsButton::OnDraw(CDC* deviceContext, const CRect& /*rectangle */, UINT /* state */) {
  m_selectedIndex = 0;

  if (m_palette == nullptr || deviceContext == nullptr) { return; }

  for (EoUInt16 Index = m_beginIndex; Index <= m_endIndex; Index++) {
    if (m_layout == SimpleSingleRow) {
      DrawCell(deviceContext, Index, m_palette[Index]);
    } else if (m_layout == GridDown5RowsOddOnly && ((Index % 2) != 0)) {
      DrawCell(deviceContext, Index, m_palette[Index]);
    } else if (m_layout == GridUp5RowsEvenOnly && ((Index % 2) == 0)) {
      DrawCell(deviceContext, Index, m_palette[Index]);
    }
  }
}

UINT EoCtrlColorsButton::OnGetDlgCode() { return DLGC_WANTARROWS; }

void EoCtrlColorsButton::OnKeyDown(UINT keyCode, UINT repeatCount, UINT flags) {
  if (keyCode >= VK_LEFT && keyCode <= VK_DOWN) {
    CDC* DeviceContext = GetDC();
    if (DeviceContext) {
      m_selectedIndex = 0;
      if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
        DrawCell(DeviceContext, m_subItem, m_palette[m_subItem]);
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

      CRect CurrentSubItemRectangle;
      SubItemRectangleByIndex(m_subItem, CurrentSubItemRectangle);

      m_selectedIndex = m_subItem;
      if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
        DrawCell(DeviceContext, m_subItem, m_palette[m_subItem]);
      }
      ReleaseDC(DeviceContext);

      NMHDR NotifyStructure{};
      NotifyStructure.hwndFrom = GetSafeHwnd();
      NotifyStructure.idFrom = static_cast<uint64_t>(GetDlgCtrlID());
      ::SendMessageW(GetParent()->GetSafeHwnd(), WM_NOTIFY, (WPARAM)NotifyStructure.idFrom, (LPARAM)&NotifyStructure);
    }
  }
  CMFCButton::OnKeyDown(keyCode, repeatCount, flags);
}

void EoCtrlColorsButton::OnLButtonUp(UINT flags, CPoint point) {
  EoUInt16 CurrentSubItem = SubItemByPoint(point);
  if (CurrentSubItem != 0) { m_subItem = CurrentSubItem; }
  CMFCButton::OnLButtonUp(flags, point);
}

void EoCtrlColorsButton::OnMouseMove(UINT flags, CPoint point) {
  CDC* DeviceContext = GetDC();
  if (DeviceContext) {
    m_selectedIndex = 0;
    if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
      DrawCell(DeviceContext, m_subItem, m_palette[m_subItem]);
    }

    m_subItem = SubItemByPoint(point);

    if (m_subItem != 0) {
      m_selectedIndex = m_subItem;

      if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
        DrawCell(DeviceContext, m_subItem, m_palette[m_subItem]);
      }

      NMHDR NotifyStructure{};
      NotifyStructure.hwndFrom = GetSafeHwnd();
      NotifyStructure.idFrom = static_cast<std::uint64_t>(GetDlgCtrlID());

      ::SendMessageW(GetParent()->GetSafeHwnd(), WM_NOTIFY, (WPARAM)NotifyStructure.idFrom, (LPARAM)&NotifyStructure);
    }
    ReleaseDC(DeviceContext);
  }

  CMFCButton::OnMouseMove(flags, point);
}

void EoCtrlColorsButton::OnPaint() { CMFCButton::OnPaint(); }

void EoCtrlColorsButton::OnSetFocus(CWnd* oldWindow) {
  CMFCButton::OnSetFocus(oldWindow);

  CDC* DeviceContext = GetDC();
  if (DeviceContext) {
    if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
      DrawCell(DeviceContext, m_subItem, m_palette[m_subItem]);
    }

    m_subItem = m_beginIndex;
    CRect CurrentSubItemRectangle;
    SubItemRectangleByIndex(m_subItem, CurrentSubItemRectangle);

    m_selectedIndex = m_subItem;
    if (m_palette && m_subItem >= m_beginIndex && m_subItem <= m_endIndex) {
      DrawCell(DeviceContext, m_subItem, m_palette[m_subItem]);
    }
    ReleaseDC(DeviceContext);

    NMHDR NotifyStructure{};
    NotifyStructure.hwndFrom = GetSafeHwnd();
    NotifyStructure.idFrom = static_cast<std::uint64_t>(GetDlgCtrlID());

    ::SendMessageW(GetParent()->GetSafeHwnd(), WM_NOTIFY, (WPARAM)NotifyStructure.idFrom, (LPARAM)&NotifyStructure);
  }
}
