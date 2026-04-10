#include "Stdafx.h"

#include "EoGsRenderDeviceDirect2D.h"

using Microsoft::WRL::ComPtr;

// ── Construction / Destruction ──────────────────────────────────────────

EoGsRenderDeviceDirect2D::EoGsRenderDeviceDirect2D(
    ID2D1RenderTarget* renderTarget, ID2D1Factory* d2dFactory, IDWriteFactory* dwriteFactory)
    : m_renderTarget(renderTarget), m_d2dFactory(d2dFactory), m_dwriteFactory(dwriteFactory) {
  if (m_renderTarget != nullptr) {
    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_brush);
    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_fillBrush);
  }
}

EoGsRenderDeviceDirect2D::~EoGsRenderDeviceDirect2D() {
  // Pop any outstanding clip layers to keep the render target state valid
  while (m_clipDepth > 0) {
    m_renderTarget->PopAxisAlignedClip();
    --m_clipDepth;
  }
}

// ── Private Helpers ─────────────────────────────────────────────────────

D2D1_COLOR_F EoGsRenderDeviceDirect2D::ColorRefToD2D(COLORREF cr) noexcept {
  return D2D1::ColorF(GetRValue(cr) / 255.0f, GetGValue(cr) / 255.0f, GetBValue(cr) / 255.0f);
}

ComPtr<ID2D1StrokeStyle> EoGsRenderDeviceDirect2D::CreateStrokeStyleForPenStyle(int penStyle) const {
  if (penStyle == PS_SOLID || penStyle == PS_NULL || m_d2dFactory == nullptr) { return nullptr; }

  D2D1_DASH_STYLE dashStyle = D2D1_DASH_STYLE_SOLID;
  switch (penStyle) {
    case PS_DOT: dashStyle = D2D1_DASH_STYLE_DOT; break;
    case PS_DASH: dashStyle = D2D1_DASH_STYLE_DASH; break;
    case PS_DASHDOT: dashStyle = D2D1_DASH_STYLE_DASH_DOT; break;
    case PS_DASHDOTDOT: dashStyle = D2D1_DASH_STYLE_DASH_DOT_DOT; break;
    default: return nullptr;
  }

  D2D1_STROKE_STYLE_PROPERTIES strokeProps = D2D1::StrokeStyleProperties();
  strokeProps.dashStyle = dashStyle;

  ComPtr<ID2D1StrokeStyle> style;
  m_d2dFactory->CreateStrokeStyle(strokeProps, nullptr, 0, &style);
  return style;
}

void EoGsRenderDeviceDirect2D::EnsureBrush(const D2D1_COLOR_F& color) {
  if (m_brush) {
    m_brush->SetColor(color);
  } else if (m_renderTarget != nullptr) {
    m_renderTarget->CreateSolidColorBrush(color, &m_brush);
  }
}

void EoGsRenderDeviceDirect2D::EnsureFillBrush(const D2D1_COLOR_F& color) {
  if (m_fillBrush) {
    m_fillBrush->SetColor(color);
  } else if (m_renderTarget != nullptr) {
    m_renderTarget->CreateSolidColorBrush(color, &m_fillBrush);
  }
}

/// @brief Converts an integer pixel coordinate to a D2D point aligned on pixel centers.
/// GDI integer coordinates address pixel top-left corners. D2D integer coordinates fall on
/// pixel *boundaries* (between adjacent pixels). A 1px line at an integer boundary straddles
/// two pixel rows, causing inconsistent rasterization on diagonals in aliased mode.
/// Adding 0.5 centers the line on the pixel, matching GDI's coordinate convention.
static D2D1_POINT_2F PixelCenterPoint(int x, int y) noexcept {
  return D2D1::Point2F(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
}

// ── Line Drawing ────────────────────────────────────────────────────────

void EoGsRenderDeviceDirect2D::MoveTo(int x, int y) {
  m_currentPosition = PixelCenterPoint(x, y);
}

void EoGsRenderDeviceDirect2D::LineTo(int x, int y) {
  if (m_renderTarget == nullptr || m_penStyle == PS_NULL) { return; }

  auto endPoint = PixelCenterPoint(x, y);
  EnsureBrush(m_penColor);
  m_renderTarget->DrawLine(m_currentPosition, endPoint, m_brush.Get(), m_penWidth, m_strokeStyle.Get());
  m_currentPosition = endPoint;
}

void EoGsRenderDeviceDirect2D::Polyline(const POINT* points, int count) {
  if (m_renderTarget == nullptr || count < 2 || m_penStyle == PS_NULL) { return; }

  EnsureBrush(m_penColor);

  // Two-point case: simple DrawLine (no joins needed — used by DisplayDashPattern)
  if (count == 2) {
    auto p0 = PixelCenterPoint(points[0].x, points[0].y);
    auto p1 = PixelCenterPoint(points[1].x, points[1].y);
    m_renderTarget->DrawLine(p0, p1, m_brush.Get(), m_penWidth, m_strokeStyle.Get());
    m_currentPosition = p1;
    return;
  }

  // Multi-point case: path geometry for proper line joins (eliminates 45° diagonal gaps)
  if (m_d2dFactory == nullptr) { return; }

  ComPtr<ID2D1PathGeometry> path;
  if (FAILED(m_d2dFactory->CreatePathGeometry(&path))) { return; }

  ComPtr<ID2D1GeometrySink> sink;
  if (FAILED(path->Open(&sink))) { return; }

  sink->BeginFigure(PixelCenterPoint(points[0].x, points[0].y), D2D1_FIGURE_BEGIN_HOLLOW);

  for (int i = 1; i < count; ++i) {
    sink->AddLine(PixelCenterPoint(points[i].x, points[i].y));
  }

  sink->EndFigure(D2D1_FIGURE_END_OPEN);
  if (FAILED(sink->Close())) { return; }

  m_renderTarget->DrawGeometry(path.Get(), m_brush.Get(), m_penWidth, m_strokeStyle.Get());

  m_currentPosition = PixelCenterPoint(points[count - 1].x, points[count - 1].y);
}

// ── Filled Shapes ───────────────────────────────────────────────────────

void EoGsRenderDeviceDirect2D::Polygon(const POINT* points, int count) {
  if (m_renderTarget == nullptr || m_d2dFactory == nullptr || count < 2) { return; }

  ComPtr<ID2D1PathGeometry> path;
  if (FAILED(m_d2dFactory->CreatePathGeometry(&path))) { return; }

  ComPtr<ID2D1GeometrySink> sink;
  if (FAILED(path->Open(&sink))) { return; }

  sink->BeginFigure(
      D2D1::Point2F(static_cast<float>(points[0].x), static_cast<float>(points[0].y)), D2D1_FIGURE_BEGIN_FILLED);

  for (int i = 1; i < count; ++i) {
    sink->AddLine(D2D1::Point2F(static_cast<float>(points[i].x), static_cast<float>(points[i].y)));
  }

  sink->EndFigure(D2D1_FIGURE_END_CLOSED);
  if (FAILED(sink->Close())) { return; }

  // Fill (unless null brush)
  if (!m_useNullBrush && m_fillBrush) { m_renderTarget->FillGeometry(path.Get(), m_fillBrush.Get()); }

  // Stroke outline
  if (m_penStyle != PS_NULL) {
    EnsureBrush(m_penColor);
    m_renderTarget->DrawGeometry(path.Get(), m_brush.Get(), m_penWidth, m_strokeStyle.Get());
  }
}

void EoGsRenderDeviceDirect2D::Ellipse(int left, int top, int right, int bottom) {
  if (m_renderTarget == nullptr) { return; }

  auto centerX = (static_cast<float>(left) + static_cast<float>(right)) / 2.0f;
  auto centerY = (static_cast<float>(top) + static_cast<float>(bottom)) / 2.0f;
  auto radiusX = (static_cast<float>(right) - static_cast<float>(left)) / 2.0f;
  auto radiusY = (static_cast<float>(bottom) - static_cast<float>(top)) / 2.0f;

  D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(centerX, centerY), radiusX, radiusY);

  if (!m_useNullBrush && m_fillBrush) { m_renderTarget->FillEllipse(ellipse, m_fillBrush.Get()); }

  if (m_penStyle != PS_NULL) {
    EnsureBrush(m_penColor);
    m_renderTarget->DrawEllipse(ellipse, m_brush.Get(), m_penWidth, m_strokeStyle.Get());
  }
}

void EoGsRenderDeviceDirect2D::Rectangle(int left, int top, int right, int bottom) {
  if (m_renderTarget == nullptr) { return; }

  auto rect =
      D2D1::RectF(static_cast<float>(left), static_cast<float>(top), static_cast<float>(right), static_cast<float>(bottom));

  if (!m_useNullBrush && m_fillBrush) { m_renderTarget->FillRectangle(rect, m_fillBrush.Get()); }

  if (m_penStyle != PS_NULL) {
    EnsureBrush(m_penColor);
    m_renderTarget->DrawRectangle(rect, m_brush.Get(), m_penWidth, m_strokeStyle.Get());
  }
}

// ── Pixel Operations ────────────────────────────────────────────────────

void EoGsRenderDeviceDirect2D::SetPixel(int x, int y, COLORREF color) {
  if (m_renderTarget == nullptr) { return; }

  auto d2dColor = ColorRefToD2D(color);
  EnsureBrush(d2dColor);
  auto rect = D2D1::RectF(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + 1), static_cast<float>(y + 1));
  m_renderTarget->FillRectangle(rect, m_brush.Get());
}

void EoGsRenderDeviceDirect2D::SetPixel(POINT point, COLORREF color) { SetPixel(point.x, point.y, color); }

// ── Pen / Brush State ───────────────────────────────────────────────────

void EoGsRenderDeviceDirect2D::SelectPen(int style, int width, COLORREF color) {
  // Save current state for one-level restore
  m_savedPenColor = m_penColor;
  m_savedPenWidth = m_penWidth;
  m_savedPenStyle = m_penStyle;
  m_savedStrokeStyle = m_strokeStyle;
  m_hasSavedPen = true;

  m_penColor = ColorRefToD2D(color);
  m_penWidth = ((width > 0) ? static_cast<float>(width) : 1.0f) * m_penWidthScale;
  m_penStyle = style;
  m_strokeStyle = CreateStrokeStyleForPenStyle(style);
}

void EoGsRenderDeviceDirect2D::RestorePen() {
  if (m_hasSavedPen) {
    m_penColor = m_savedPenColor;
    m_penWidth = m_savedPenWidth;
    m_penStyle = m_savedPenStyle;
    m_strokeStyle = m_savedStrokeStyle;
    m_hasSavedPen = false;
  }
}

void EoGsRenderDeviceDirect2D::SelectSolidBrush(COLORREF color) {
  m_savedFillColor = m_fillBrush ? m_fillBrush->GetColor() : D2D1::ColorF(D2D1::ColorF::White);
  m_hasSavedBrush = true;
  m_useNullBrush = false;

  EnsureFillBrush(ColorRefToD2D(color));
}

void EoGsRenderDeviceDirect2D::SelectNullBrush() {
  m_savedFillColor = m_fillBrush ? m_fillBrush->GetColor() : D2D1::ColorF(D2D1::ColorF::White);
  m_hasSavedBrush = true;
  m_useNullBrush = true;
}

void EoGsRenderDeviceDirect2D::RestoreBrush() {
  if (m_hasSavedBrush) {
    m_useNullBrush = false;
    EnsureFillBrush(m_savedFillColor);
    m_hasSavedBrush = false;
  }
}

// ── Text Output ─────────────────────────────────────────────────────────

void EoGsRenderDeviceDirect2D::TextOut(int x, int y, const wchar_t* text, int length) {
  if (m_renderTarget == nullptr || m_dwriteFactory == nullptr || m_textFormat == nullptr || length <= 0) { return; }

  ComPtr<IDWriteTextLayout> textLayout;
  m_dwriteFactory->CreateTextLayout(
      text, static_cast<UINT32>(length), m_textFormat.Get(), 10000.0f, 10000.0f, &textLayout);
  if (textLayout == nullptr) { return; }

  // Compute origin offset based on GDI text alignment flags
  DWRITE_TEXT_METRICS metrics{};
  textLayout->GetMetrics(&metrics);

  auto originX = static_cast<float>(x);
  auto originY = static_cast<float>(y);

  if (m_textAlign & TA_CENTER) {
    originX -= metrics.width / 2.0f;
  } else if (m_textAlign & TA_RIGHT) {
    originX -= metrics.width;
  }

  if (m_textAlign & TA_BASELINE) {
    // Get baseline offset from the first line
    DWRITE_LINE_METRICS lineMetrics{};
    UINT32 lineCount = 0;
    textLayout->GetLineMetrics(&lineMetrics, 1, &lineCount);
    if (lineCount > 0) { originY -= lineMetrics.baseline; }
  } else if (m_textAlign & TA_BOTTOM) {
    originY -= metrics.height;
  }
  // TA_TOP (default): originY is already correct

  // Apply rotation transform for non-zero escapement (GDI lfEscapement is CCW in tenths of degrees;
  // D2D Rotation() is CW, so negate to match GDI visual behavior)
  D2D1::Matrix3x2F savedTransform;
  bool hasRotation = (m_escapement != 0);
  if (hasRotation) {
    m_renderTarget->GetTransform(&savedTransform);
    float angleDegrees = -m_escapement / 10.0f;
    auto rotation = D2D1::Matrix3x2F::Rotation(angleDegrees, D2D1::Point2F(static_cast<float>(x), static_cast<float>(y)));
    m_renderTarget->SetTransform(rotation * savedTransform);
  }

  // Draw background rectangle for OPAQUE mode
  if (m_bkMode == OPAQUE) {
    auto bgRect = D2D1::RectF(originX, originY, originX + metrics.width, originY + metrics.height);
    EnsureFillBrush(m_bkColor);
    m_renderTarget->FillRectangle(bgRect, m_fillBrush.Get());
  }

  EnsureBrush(m_textColor);
  m_renderTarget->DrawTextLayout(D2D1::Point2F(originX, originY), textLayout.Get(), m_brush.Get());

  // Restore the original transform
  if (hasRotation) { m_renderTarget->SetTransform(savedTransform); }
}

void EoGsRenderDeviceDirect2D::ExtTextOut(
    int x, int y, UINT options, const RECT* rect, const wchar_t* text, UINT length) {
  if (m_renderTarget == nullptr) { return; }

  // Handle opaque background rectangle
  if ((options & ETO_OPAQUE) && rect != nullptr) {
    EnsureFillBrush(m_bkColor);
    auto bgRect = D2D1::RectF(
        static_cast<float>(rect->left), static_cast<float>(rect->top),
        static_cast<float>(rect->right), static_cast<float>(rect->bottom));
    m_renderTarget->FillRectangle(bgRect, m_fillBrush.Get());
  }

  if (text != nullptr && length > 0) { TextOut(x, y, text, static_cast<int>(length)); }
}

COLORREF EoGsRenderDeviceDirect2D::SetTextColor(COLORREF color) {
  auto previousR = static_cast<BYTE>(m_textColor.r * 255.0f);
  auto previousG = static_cast<BYTE>(m_textColor.g * 255.0f);
  auto previousB = static_cast<BYTE>(m_textColor.b * 255.0f);
  COLORREF previous = RGB(previousR, previousG, previousB);

  m_textColor = ColorRefToD2D(color);
  return previous;
}

UINT EoGsRenderDeviceDirect2D::SetTextAlign(UINT align) {
  UINT previous = m_textAlign;
  m_textAlign = align;
  return previous;
}

int EoGsRenderDeviceDirect2D::SetBkMode(int mode) {
  int previous = m_bkMode;
  m_bkMode = mode;
  return previous;
}

COLORREF EoGsRenderDeviceDirect2D::SetBkColor(COLORREF color) {
  auto previousR = static_cast<BYTE>(m_bkColor.r * 255.0f);
  auto previousG = static_cast<BYTE>(m_bkColor.g * 255.0f);
  auto previousB = static_cast<BYTE>(m_bkColor.b * 255.0f);
  COLORREF previous = RGB(previousR, previousG, previousB);

  m_bkColor = ColorRefToD2D(color);
  return previous;
}

// ── Font Selection ──────────────────────────────────────────────────────

void EoGsRenderDeviceDirect2D::SelectFont(const LOGFONT* logFont) {
  if (m_dwriteFactory == nullptr || logFont == nullptr) { return; }

  // Save current for one-level restore
  if (m_textFormat) {
    m_savedLogFont = *logFont;
    m_hasSavedFont = true;
  }

  // Capture escapement for rotation in TextOut — tenths of degrees, CCW in GDI screen space
  m_escapement = logFont->lfEscapement;

  // Convert LOGFONT height to points — LOGFONT lfHeight is in device units (negative = character height)
  float fontSize = static_cast<float>(abs(logFont->lfHeight));
  if (fontSize < 1.0f) { fontSize = 12.0f; }

  // Convert device units to DIPs (Direct2D works in DIPs which are 1/96 inch)
  // The LOGFONT lfHeight in device units maps directly to DIPs for 96 DPI
  // For other DPIs, the render target's DPI scaling handles it

  auto fontWeight = static_cast<DWRITE_FONT_WEIGHT>(logFont->lfWeight > 0 ? logFont->lfWeight : DWRITE_FONT_WEIGHT_NORMAL);
  auto fontStyle = logFont->lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

  m_textFormat.Reset();
  m_dwriteFactory->CreateTextFormat(logFont->lfFaceName, nullptr, fontWeight, fontStyle, DWRITE_FONT_STRETCH_NORMAL,
      fontSize, L"", &m_textFormat);
}

void EoGsRenderDeviceDirect2D::RestoreFont() {
  // Text format is stateless in DirectWrite — we don't need to "restore" a GDI object.
  // The saved font is only meaningful if we need to recreate the previous format.
  m_hasSavedFont = false;
}

// ── Drawing Mode ────────────────────────────────────────────────────────

int EoGsRenderDeviceDirect2D::SetROP2(int drawMode) {
  int previous = m_rop2;
  m_rop2 = drawMode;
  // D2D has no ROP2 equivalent. R2_COPYPEN is the only mode that works naturally.
  // R2_XORPEN callers should be handled at a higher level (rubberband overlay, scene invalidation).
  return previous;
}

// ── Device Capabilities ─────────────────────────────────────────────────

int EoGsRenderDeviceDirect2D::GetDeviceCaps(int index) const {
  if (m_renderTarget == nullptr) { return 0; }

  auto size = m_renderTarget->GetSize();  // DIPs — matches D2D coordinate space
  float dpiX = 0.0f;
  float dpiY = 0.0f;
#pragma warning(suppress : 4996)  // GetDpi deprecated in favor of ID2D1DeviceContext methods
  m_renderTarget->GetDpi(&dpiX, &dpiY);

  switch (index) {
    case HORZRES: return static_cast<int>(size.width);
    case VERTRES: return static_cast<int>(size.height);
    case LOGPIXELSX: return static_cast<int>(dpiX);
    case LOGPIXELSY: return static_cast<int>(dpiY);
    case BITSPIXEL: return 32;
    case HORZSIZE: return static_cast<int>(size.width * 25.4f / dpiX);
    case VERTSIZE: return static_cast<int>(size.height * 25.4f / dpiY);
    default: return 0;
  }
}

// ── Clipping ────────────────────────────────────────────────────────────

int EoGsRenderDeviceDirect2D::GetClipBox(RECT& rect) const {
  if (m_renderTarget == nullptr) { return NULLREGION; }

  auto size = m_renderTarget->GetSize();  // DIPs — matches D2D coordinate space
  rect.left = 0;
  rect.top = 0;
  rect.right = static_cast<LONG>(size.width);
  rect.bottom = static_cast<LONG>(size.height);
  return SIMPLEREGION;
}

void EoGsRenderDeviceDirect2D::PushClipRect(int left, int top, int right, int bottom) {
  if (m_renderTarget == nullptr) { return; }

  auto clipRect = D2D1::RectF(
      static_cast<float>(left), static_cast<float>(top), static_cast<float>(right), static_cast<float>(bottom));

  m_renderTarget->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
  ++m_clipDepth;
}

void EoGsRenderDeviceDirect2D::PopClipRect() {
  if (m_renderTarget != nullptr && m_clipDepth > 0) {
    m_renderTarget->PopAxisAlignedClip();
    --m_clipDepth;
  }
}

// ── Bitmap Operations ───────────────────────────────────────────────────

void EoGsRenderDeviceDirect2D::StretchBlt(int /*destX*/, int /*destY*/, int /*destWidth*/, int /*destHeight*/,
    EoGsRenderDevice* /*sourceDevice*/, int /*srcX*/, int /*srcY*/, int /*srcWidth*/, int /*srcHeight*/,
    DWORD /*rop*/) {
  // No-op for Direct2D — the D2D render target is inherently double-buffered.
  // Cross-device blitting would require ID2D1Bitmap interop.
}

// ── Palette ─────────────────────────────────────────────────────────────

void* EoGsRenderDeviceDirect2D::SelectPalette(void* /*palette*/, bool /*forceBackground*/) {
  return nullptr;  // D2D is true-color — no palette management needed
}

void EoGsRenderDeviceDirect2D::RealizePalette() {
  // No-op for Direct2D
}

// ── Underlying Device Access ────────────────────────────────────────────

CDC* EoGsRenderDeviceDirect2D::GetCDC() const {
  // D2D backend has no CDC — callers must not use this in the D2D path.
  return nullptr;
}
