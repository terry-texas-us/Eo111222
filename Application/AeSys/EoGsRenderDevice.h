#pragma once

/// @file EoGsRenderDevice.h
/// @brief Abstract rendering device interface — API-agnostic replacement for CDC*.
///
/// Phase 1 of the Direct2D migration plan. All primitive Display() methods and
/// rendering helpers will eventually accept EoGsRenderDevice* instead of CDC*.
/// Phase 2 provides the concrete GDI backend (EoGsRenderDeviceGdi).
///
/// Design principles:
///  - Methods accept arrays/spans, not single points — batch-oriented.
///  - Color as COLORREF — no GDI object handles cross the interface boundary.
///  - No SelectObject/CreatePen semantics leak through.
///  - Clip regions via PushClipRect/PopClipRect.
///  - Text via DrawText with font descriptor, not HFONT.

#include <cstdint>

/// @brief Abstract rendering device interface.
///
/// Provides a minimal set of drawing operations sufficient to cover the entire
/// entity Display() pipeline, the polyline dash-pattern renderer, the grid
/// display, hatch fill, background-image blitting, and viewport clipping.
///
/// Concrete backends: EoGsRenderDeviceGdi (Phase 2), EoGsRenderDeviceDirect2D (Phase 6).
class EoGsRenderDevice {
 public:
  EoGsRenderDevice() = default;
  EoGsRenderDevice(const EoGsRenderDevice&) = default;
  EoGsRenderDevice(EoGsRenderDevice&&) noexcept = default;
  EoGsRenderDevice& operator=(const EoGsRenderDevice&) = default;
  EoGsRenderDevice& operator=(EoGsRenderDevice&&) noexcept = default;

  virtual ~EoGsRenderDevice() = default;

  // ── Line Drawing ──────────────────────────────────────────────────────

  /// @brief Moves the current position to the specified client-space point.
  virtual void MoveTo(int x, int y) = 0;

  /// @brief Draws a line from the current position to the specified client-space point.
  virtual void LineTo(int x, int y) = 0;

  /// @brief Draws a connected sequence of line segments (polyline).
  /// @param points Client-space points array.
  /// @param count Number of points.
  virtual void Polyline(const POINT* points, int count) = 0;

  // ── Filled Shapes ─────────────────────────────────────────────────────

  /// @brief Draws a filled polygon using the current brush and outlined by the current pen.
  /// @param points Client-space polygon vertices.
  /// @param count Number of vertices.
  virtual void Polygon(const POINT* points, int count) = 0;

  /// @brief Draws an axis-aligned ellipse bounded by the specified rectangle.
  virtual void Ellipse(int left, int top, int right, int bottom) = 0;

  /// @brief Draws an axis-aligned rectangle outlined by the current pen.
  virtual void Rectangle(int left, int top, int right, int bottom) = 0;

  // ── Pixel Operations ──────────────────────────────────────────────────

  /// @brief Sets a single pixel at the specified client-space position.
  virtual void SetPixel(int x, int y, COLORREF color) = 0;

  /// @brief Sets a single pixel at the specified CPoint position.
  virtual void SetPixel(POINT point, COLORREF color) = 0;

  // ── Pen / Brush State ─────────────────────────────────────────────────

  /// @brief Selects a logical pen defined by style, width, and color.
  ///
  /// Replaces the CDC SelectObject(CPen*) + CreatePen pattern.
  /// The device manages pen lifetime internally.
  /// @param style GDI pen style constant (PS_SOLID, PS_DOT, PS_NULL, etc.).
  /// @param width Pen width in pixels (0 = default 1px).
  /// @param color Pen color as COLORREF.
  virtual void SelectPen(int style, int width, COLORREF color) = 0;

  /// @brief Restores the pen that was active before the last SelectPen call.
  virtual void RestorePen() = 0;

  /// @brief Selects a solid brush of the specified color.
  virtual void SelectSolidBrush(COLORREF color) = 0;

  /// @brief Selects the NULL_BRUSH stock object (hollow fill).
  virtual void SelectNullBrush() = 0;

  /// @brief Restores the brush that was active before the last SelectSolidBrush/SelectNullBrush call.
  virtual void RestoreBrush() = 0;

  // ── Text Output ───────────────────────────────────────────────────────

  /// @brief Outputs a text string at the specified client-space position.
  /// @param x Client-space X coordinate.
  /// @param y Client-space Y coordinate.
  /// @param text Pointer to wide-character string.
  /// @param length Number of characters to output.
  virtual void TextOut(int x, int y, const wchar_t* text, int length) = 0;

  /// @brief Outputs text with opaque/clipping rectangle (maps to ExtTextOutW).
  /// @param x Client-space X coordinate.
  /// @param y Client-space Y coordinate.
  /// @param options ExtTextOut option flags (ETO_OPAQUE, ETO_CLIPPED, etc.).
  /// @param rect Optional clipping/opaque rectangle (may be nullptr).
  /// @param text Pointer to wide-character string.
  /// @param length Number of characters to output.
  virtual void ExtTextOut(int x, int y, UINT options, const RECT* rect, const wchar_t* text, UINT length) = 0;

  /// @brief Sets the text foreground color.
  /// @return The previous text color.
  virtual COLORREF SetTextColor(COLORREF color) = 0;

  /// @brief Sets the text alignment flags.
  /// @param align GDI text alignment flags (TA_LEFT, TA_BASELINE, etc.).
  /// @return The previous alignment flags.
  virtual UINT SetTextAlign(UINT align) = 0;

  /// @brief Sets the background mix mode for text and hatched brushes.
  /// @param mode Background mode (TRANSPARENT or OPAQUE).
  /// @return The previous background mode.
  virtual int SetBkMode(int mode) = 0;

  /// @brief Sets the background color (used with OPAQUE mode and certain ROP operations).
  /// @param color The background color.
  /// @return The previous background color.
  virtual COLORREF SetBkColor(COLORREF color) = 0;

  // ── Font Selection ────────────────────────────────────────────────────

  /// @brief Creates and selects a font from a LOGFONT descriptor.
  ///
  /// The device manages font object lifetime. The previously selected font
  /// is saved internally and can be restored with RestoreFont().
  /// @param logFont Pointer to a LOGFONT structure describing the font.
  virtual void SelectFont(const LOGFONT* logFont) = 0;

  /// @brief Restores the font that was active before the last SelectFont call.
  virtual void RestoreFont() = 0;

  // ── Drawing Mode ──────────────────────────────────────────────────────

  /// @brief Sets the foreground mix mode (raster operation for pens and filled interiors).
  /// @param drawMode GDI ROP2 constant (R2_COPYPEN, R2_XORPEN, etc.).
  /// @return The previous ROP2 mode.
  virtual int SetROP2(int drawMode) = 0;

  // ── Device Capabilities ───────────────────────────────────────────────

  /// @brief Retrieves device-specific capability information.
  /// @param index GDI device-caps index (HORZRES, VERTRES, LOGPIXELSX, etc.).
  /// @return The requested capability value.
  virtual int GetDeviceCaps(int index) const = 0;

  // ── Clipping ──────────────────────────────────────────────────────────

  /// @brief Retrieves the bounding rectangle of the current clip region.
  /// @param rect Output rectangle.
  /// @return Clip box type (SIMPLEREGION, COMPLEXREGION, NULLREGION, or ERROR).
  virtual int GetClipBox(RECT& rect) const = 0;

  /// @brief Saves the current device state and pushes an axis-aligned clip rectangle.
  ///
  /// This combines SaveDC + IntersectClipRect into a single call.
  /// Must be matched by a PopClipRect() call.
  /// @param left Left edge of the clip rectangle.
  /// @param top Top edge of the clip rectangle.
  /// @param right Right edge of the clip rectangle.
  /// @param bottom Bottom edge of the clip rectangle.
  virtual void PushClipRect(int left, int top, int right, int bottom) = 0;

  /// @brief Restores the device state saved by the last PushClipRect call.
  virtual void PopClipRect() = 0;

  // ── Bitmap Operations ─────────────────────────────────────────────────

  /// @brief Copies a rectangle from a source device to this device with stretching.
  ///
  /// Maps to StretchBlt. The source is identified by an opaque handle that
  /// the GDI backend interprets as CDC*.
  /// @param destX Destination X.
  /// @param destY Destination Y.
  /// @param destWidth Destination width.
  /// @param destHeight Destination height.
  /// @param sourceDevice Opaque pointer to the source render device (same concrete type).
  /// @param srcX Source X.
  /// @param srcY Source Y.
  /// @param srcWidth Source width.
  /// @param srcHeight Source height.
  /// @param rop Raster operation code (e.g., SRCCOPY).
  virtual void StretchBlt(int destX, int destY, int destWidth, int destHeight, EoGsRenderDevice* sourceDevice,
      int srcX, int srcY, int srcWidth, int srcHeight, DWORD rop) = 0;

  // ── Palette (GDI-specific, no-op on D2D) ──────────────────────────────

  /// @brief Selects a logical palette (GDI-specific; D2D backend may no-op).
  /// @param palette Opaque palette handle (HPALETTE cast to void*).
  /// @param forceBackground If true, maps palette to background.
  /// @return Previous palette handle (or nullptr).
  virtual void* SelectPalette(void* palette, bool forceBackground) = 0;

  /// @brief Maps palette entries to the system palette.
  virtual void RealizePalette() = 0;

  // ── Underlying Device Access (transitional) ───────────────────────────

  /// @brief Returns the underlying CDC* for code that has not yet been migrated.
  ///
  /// This is a transitional escape hatch. Code should progressively stop
  /// calling this as methods move to the abstract interface. Returns nullptr
  /// for non-GDI backends.
  virtual CDC* GetCDC() const = 0;
};
