#pragma once

/// @file EoGsRenderDeviceDirect2D.h
/// @brief Direct2D backend for EoGsRenderDevice — hardware-accelerated 2D rendering.
///
/// Implements all abstract methods from EoGsRenderDevice using
/// ID2D1HwndRenderTarget, ID2D1SolidColorBrush, and IDWriteFactory / IDWriteTextFormat.
///
/// Ownership: the caller owns the render target and write factory; this device
/// holds non-owning pointers and manages only per-frame brush/stroke resources.

#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include "EoGsRenderDevice.h"

/// @brief Concrete Direct2D rendering device.
///
/// Wraps an ID2D1RenderTarget* (non-owning) and an IDWriteFactory* (non-owning).
/// The caller must bracket each frame with BeginDraw/EndDraw on the render target
/// before creating this device and dispatching drawing calls through it.
class EoGsRenderDeviceDirect2D : public EoGsRenderDevice {
  ID2D1RenderTarget* m_renderTarget{};
  ID2D1Factory* m_d2dFactory{};
  IDWriteFactory* m_dwriteFactory{};

  // Cached device-dependent resources
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_fillBrush;

  // Pen state tracking
  D2D1_COLOR_F m_penColor{};
  float m_penWidth{1.0f};
  float m_penWidthScale{1.0f};  ///< Multiplier for pen width (>1 for printer DPI scaling)
  int m_penStyle{PS_SOLID};
  Microsoft::WRL::ComPtr<ID2D1StrokeStyle> m_strokeStyle;  // null = solid

  // Saved pen state (one-level restore)
  D2D1_COLOR_F m_savedPenColor{};
  float m_savedPenWidth{1.0f};
  int m_savedPenStyle{PS_SOLID};
  Microsoft::WRL::ComPtr<ID2D1StrokeStyle> m_savedStrokeStyle;
  bool m_hasSavedPen{};

  // Fill state
  bool m_useNullBrush{};
  D2D1_COLOR_F m_savedFillColor{};
  bool m_hasSavedBrush{};

  // Current position for MoveTo/LineTo emulation
  D2D1_POINT_2F m_currentPosition{};

  // Text state
  D2D1_COLOR_F m_textColor{};
  UINT m_textAlign{TA_LEFT | TA_TOP};
  int m_bkMode{TRANSPARENT};
  D2D1_COLOR_F m_bkColor{};
  Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
  int m_escapement{};  ///< LOGFONT lfEscapement in tenths of degrees (CCW in GDI screen space)
  LOGFONT m_savedLogFont{};
  bool m_hasSavedFont{};

  // ROP2 state (limited D2D support)
  int m_rop2{R2_COPYPEN};

  // Clip stack depth for PushClipRect/PopClipRect
  int m_clipDepth{};

 public:
  /// @brief Constructs a Direct2D render device wrapping the given render target.
  /// @param renderTarget Non-owning pointer to a D2D render target (must be between BeginDraw/EndDraw).
  /// @param d2dFactory Non-owning pointer to the D2D factory (for creating stroke styles / path geometries).
  /// @param dwriteFactory Non-owning pointer to the DirectWrite factory (for creating text formats).
  EoGsRenderDeviceDirect2D(ID2D1RenderTarget* renderTarget, ID2D1Factory* d2dFactory, IDWriteFactory* dwriteFactory);

  ~EoGsRenderDeviceDirect2D() override;

  /// @brief Returns the underlying D2D render target (non-owning).
  [[nodiscard]] ID2D1RenderTarget* RenderTarget() const noexcept { return m_renderTarget; }

  /// @brief Returns the underlying D2D factory (non-owning).
  [[nodiscard]] ID2D1Factory* D2DFactory() const noexcept { return m_d2dFactory; }

  /// @brief Sets the pen-width scale factor for printer DPI compensation.
  /// @param scale  Typically printerDPI / 96.0f (1.0 for screen rendering).
  void SetPenWidthScale(float scale) noexcept { m_penWidthScale = scale; }

  EoGsRenderDeviceDirect2D(const EoGsRenderDeviceDirect2D&) = delete;
  EoGsRenderDeviceDirect2D& operator=(const EoGsRenderDeviceDirect2D&) = delete;
  EoGsRenderDeviceDirect2D(EoGsRenderDeviceDirect2D&&) = delete;
  EoGsRenderDeviceDirect2D& operator=(EoGsRenderDeviceDirect2D&&) = delete;

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
  /// @brief Converts a COLORREF to a D2D1_COLOR_F.
  static D2D1_COLOR_F ColorRefToD2D(COLORREF cr) noexcept;

  /// @brief Creates a D2D1 stroke style for the given GDI pen style.
  /// @return ComPtr to the stroke style, or nullptr for PS_SOLID.
  Microsoft::WRL::ComPtr<ID2D1StrokeStyle> CreateStrokeStyleForPenStyle(int penStyle) const;

  /// @brief Ensures the main brush exists and sets its color.
  void EnsureBrush(const D2D1_COLOR_F& color);

  /// @brief Ensures the fill brush exists and sets its color.
  void EnsureFillBrush(const D2D1_COLOR_F& color);
};
