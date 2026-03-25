#include "Stdafx.h"

#include "EoGsRenderDeviceGdi.h"

EoGsRenderDeviceGdi::EoGsRenderDeviceGdi(CDC* dc) : m_dc(dc) {}

EoGsRenderDeviceGdi::~EoGsRenderDeviceGdi() {
  // Restore any GDI objects that are still selected into the DC
  if (m_previousPen) {
    m_dc->SelectObject(m_previousPen);
    m_previousPen = nullptr;
  }
  if (m_previousBrush) {
    m_dc->SelectObject(m_previousBrush);
    m_previousBrush = nullptr;
  }
  if (m_previousFont) {
    m_dc->SelectObject(m_previousFont);
    m_previousFont = nullptr;
  }
}

// ── Line Drawing ────────────────────────────────────────────────────────

void EoGsRenderDeviceGdi::MoveTo(int x, int y) { m_dc->MoveTo(x, y); }

void EoGsRenderDeviceGdi::LineTo(int x, int y) { m_dc->LineTo(x, y); }

void EoGsRenderDeviceGdi::Polyline(const POINT* points, int count) { m_dc->Polyline(points, count); }

// ── Filled Shapes ───────────────────────────────────────────────────────

void EoGsRenderDeviceGdi::Polygon(const POINT* points, int count) { m_dc->Polygon(points, count); }

void EoGsRenderDeviceGdi::Ellipse(int left, int top, int right, int bottom) { m_dc->Ellipse(left, top, right, bottom); }

void EoGsRenderDeviceGdi::Rectangle(int left, int top, int right, int bottom) {
  m_dc->Rectangle(left, top, right, bottom);
}

// ── Pixel Operations ────────────────────────────────────────────────────

void EoGsRenderDeviceGdi::SetPixel(int x, int y, COLORREF color) { m_dc->SetPixel(x, y, color); }

void EoGsRenderDeviceGdi::SetPixel(POINT point, COLORREF color) { m_dc->SetPixel(point, color); }

// ── Pen / Brush State ───────────────────────────────────────────────────

void EoGsRenderDeviceGdi::SelectPen(int style, int width, COLORREF color) {
  // Delete any previously created pen
  if (static_cast<HPEN>(m_currentPen) != nullptr) {
    if (m_previousPen) { m_dc->SelectObject(m_previousPen); }
    m_currentPen.DeleteObject();
  }
  m_currentPen.CreatePen(style, width, color);
  m_previousPen = m_dc->SelectObject(&m_currentPen);
}

void EoGsRenderDeviceGdi::RestorePen() {
  if (m_previousPen) {
    m_dc->SelectObject(m_previousPen);
    m_previousPen = nullptr;
  }
  m_currentPen.DeleteObject();
}

void EoGsRenderDeviceGdi::SelectSolidBrush(COLORREF color) {
  if (static_cast<HBRUSH>(m_currentBrush) != nullptr) {
    if (m_previousBrush) { m_dc->SelectObject(m_previousBrush); }
    m_currentBrush.DeleteObject();
  }
  m_currentBrush.CreateSolidBrush(color);
  m_previousBrush = m_dc->SelectObject(&m_currentBrush);
}

void EoGsRenderDeviceGdi::SelectNullBrush() {
  m_previousBrush = static_cast<CBrush*>(m_dc->SelectStockObject(NULL_BRUSH));
}

void EoGsRenderDeviceGdi::RestoreBrush() {
  if (m_previousBrush) {
    m_dc->SelectObject(m_previousBrush);
    m_previousBrush = nullptr;
  }
  m_currentBrush.DeleteObject();
}

// ── Text Output ─────────────────────────────────────────────────────────

void EoGsRenderDeviceGdi::TextOut(int x, int y, const wchar_t* text, int length) {
  m_dc->TextOutW(x, y, text, length);
}

void EoGsRenderDeviceGdi::ExtTextOut(int x, int y, UINT options, const RECT* rect, const wchar_t* text, UINT length) {
  m_dc->ExtTextOutW(x, y, options, rect, text, length, nullptr);
}

COLORREF EoGsRenderDeviceGdi::SetTextColor(COLORREF color) { return m_dc->SetTextColor(color); }

UINT EoGsRenderDeviceGdi::SetTextAlign(UINT align) { return m_dc->SetTextAlign(align); }

int EoGsRenderDeviceGdi::SetBkMode(int mode) { return m_dc->SetBkMode(mode); }

COLORREF EoGsRenderDeviceGdi::SetBkColor(COLORREF color) { return m_dc->SetBkColor(color); }

// ── Font Selection ──────────────────────────────────────────────────────

void EoGsRenderDeviceGdi::SelectFont(const LOGFONT* logFont) {
  if (static_cast<HFONT>(m_currentFont) != nullptr) {
    if (m_previousFont) { m_dc->SelectObject(m_previousFont); }
    m_currentFont.DeleteObject();
  }
  m_currentFont.CreateFontIndirect(logFont);
  m_previousFont = m_dc->SelectObject(&m_currentFont);
}

void EoGsRenderDeviceGdi::RestoreFont() {
  if (m_previousFont) {
    m_dc->SelectObject(m_previousFont);
    m_previousFont = nullptr;
  }
  m_currentFont.DeleteObject();
}

// ── Drawing Mode ────────────────────────────────────────────────────────

int EoGsRenderDeviceGdi::SetROP2(int drawMode) { return m_dc->SetROP2(drawMode); }

// ── Device Capabilities ─────────────────────────────────────────────────

int EoGsRenderDeviceGdi::GetDeviceCaps(int index) const { return m_dc->GetDeviceCaps(index); }

// ── Clipping ────────────────────────────────────────────────────────────

int EoGsRenderDeviceGdi::GetClipBox(RECT& rect) const { return m_dc->GetClipBox(&rect); }

void EoGsRenderDeviceGdi::PushClipRect(int left, int top, int right, int bottom) {
  m_savedDC = m_dc->SaveDC();
  m_dc->IntersectClipRect(left, top, right, bottom);
}

void EoGsRenderDeviceGdi::PopClipRect() {
  if (m_savedDC != 0) {
    m_dc->RestoreDC(m_savedDC);
    m_savedDC = 0;
  }
}

// ── Bitmap Operations ───────────────────────────────────────────────────

void EoGsRenderDeviceGdi::StretchBlt(int destX, int destY, int destWidth, int destHeight,
    EoGsRenderDevice* sourceDevice, int srcX, int srcY, int srcWidth, int srcHeight, DWORD rop) {
  auto* gdiSource = dynamic_cast<EoGsRenderDeviceGdi*>(sourceDevice);
  if (gdiSource && gdiSource->m_dc) {
    m_dc->StretchBlt(destX, destY, destWidth, destHeight, gdiSource->m_dc, srcX, srcY, srcWidth, srcHeight, rop);
  }
}

// ── Palette ─────────────────────────────────────────────────────────────

void* EoGsRenderDeviceGdi::SelectPalette(void* palette, bool forceBackground) {
  auto* hPalette = static_cast<CPalette*>(palette);
  auto* previous = m_dc->SelectPalette(hPalette, forceBackground ? TRUE : FALSE);
  return previous;
}

void EoGsRenderDeviceGdi::RealizePalette() { m_dc->RealizePalette(); }

// ── Underlying Device Access ────────────────────────────────────────────

CDC* EoGsRenderDeviceGdi::GetCDC() const { return m_dc; }
