#pragma once

/// @file EoGsRenderDeviceGdi.h
/// @brief GDI backend for EoGsRenderDevice — wraps CDC* behind the abstract interface.
///
/// Every abstract method delegates to the corresponding CDC call.

#include "EoGsRenderDevice.h"

/// @brief Concrete GDI rendering device wrapping a CDC*.
///
/// Ownership: the caller owns the CDC* and must ensure it outlives this device.
/// The device manages pen, brush, and font objects created through the
/// Select*/Restore* API — each stores a "previous" handle for one-level restore.
class EoGsRenderDeviceGdi : public EoGsRenderDevice {
  CDC* m_dc{};

  // One-level save/restore for pen, brush, and font
  CPen* m_previousPen{};
  CPen m_currentPen;

  CBrush* m_previousBrush{};
  CBrush m_currentBrush;

  CFont* m_previousFont{};
  CFont m_currentFont;

 public:
  /// @brief Constructs a GDI render device wrapping the given device context.
  /// @param dc Non-owning pointer to an MFC CDC. Must remain valid for the device lifetime.
  explicit EoGsRenderDeviceGdi(CDC* dc);

  ~EoGsRenderDeviceGdi() override;

  EoGsRenderDeviceGdi(const EoGsRenderDeviceGdi&) = delete;
  EoGsRenderDeviceGdi& operator=(const EoGsRenderDeviceGdi&) = delete;
  EoGsRenderDeviceGdi(EoGsRenderDeviceGdi&&) = delete;
  EoGsRenderDeviceGdi& operator=(EoGsRenderDeviceGdi&&) = delete;

  // ── Line Drawing ──────────────────────────────────────────────────────
  void MoveTo(int x, int y) override;
  void LineTo(int x, int y) override;
  void Polyline(const POINT* points, int count) override;

  // ── Filled Shapes ─────────────────────────────────────────────────────
  void Polygon(const POINT* points, int count) override;
  void Ellipse(int left, int top, int right, int bottom) override;
  void Rectangle(int left, int top, int right, int bottom) override;

  // ── Pixel Operations ──────────────────────────────────────────────────
  void SetPixel(int x, int y, COLORREF color) override;
  void SetPixel(POINT point, COLORREF color) override;

  // ── Pen / Brush State ─────────────────────────────────────────────────
  void SelectPen(int style, int width, COLORREF color) override;
  void RestorePen() override;
  void SelectSolidBrush(COLORREF color) override;
  void SelectNullBrush() override;
  void RestoreBrush() override;

  // ── Text Output ───────────────────────────────────────────────────────
  void TextOut(int x, int y, const wchar_t* text, int length) override;
  void ExtTextOut(int x, int y, UINT options, const RECT* rect, const wchar_t* text, UINT length) override;
  COLORREF SetTextColor(COLORREF color) override;
  UINT SetTextAlign(UINT align) override;
  int SetBkMode(int mode) override;
  COLORREF SetBkColor(COLORREF color) override;

  // ── Font Selection ────────────────────────────────────────────────────
  void SelectFont(const LOGFONT* logFont) override;
  void RestoreFont() override;

  // ── Drawing Mode ──────────────────────────────────────────────────────
  int SetROP2(int drawMode) override;

  // ── Device Capabilities ───────────────────────────────────────────────
  int GetDeviceCaps(int index) const override;

  // ── Clipping ──────────────────────────────────────────────────────────
  int GetClipBox(RECT& rect) const override;
  void PushClipRect(int left, int top, int right, int bottom) override;
  void PopClipRect() override;

  // ── Bitmap Operations ─────────────────────────────────────────────────
  void StretchBlt(int destX,
      int destY,
      int destWidth,
      int destHeight,
      EoGsRenderDevice* sourceDevice,
      int srcX,
      int srcY,
      int srcWidth,
      int srcHeight,
      DWORD rop) override;

  // ── Palette ───────────────────────────────────────────────────────────
  void* SelectPalette(void* palette, bool forceBackground) override;
  void RealizePalette() override;

  // ── Underlying Device Access ──────────────────────────────────────────
  CDC* GetCDC() const override;

 private:
  /// @brief Saved DC state index for PushClipRect/PopClipRect.
  int m_savedDC{};
};
