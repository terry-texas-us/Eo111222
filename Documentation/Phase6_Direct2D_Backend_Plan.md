# Phase 6 — Direct2D Backend Implementation Plan

## Goal
Implement `EoGsRenderDeviceDirect2D` as a concrete backend for the `EoGsRenderDevice` interface (32 pure virtuals + `GetCDC()` escape hatch), enabling hardware-accelerated 2D rendering with antialiasing. The GDI backend (`EoGsRenderDeviceGdi`) remains the default and printer fallback — Phase 6 delivers a **switchable** D2D path that can be toggled for visual validation before becoming the default.

## Prerequisites (All Complete ✅)
- Phase 1: `EoGsRenderDevice.h` — abstract interface (32 pure virtuals)
- Phase 2: `EoGsRenderDeviceGdi` — concrete GDI backend
- Phase 3: `Display(AeSysView*, EoGsRenderDevice*)` — virtual signature across all 11 primitive types
- Phase 4: Off-screen GDI back buffer in `AeSysView` (`m_backBufferDC`, `m_sceneInvalid`)
- Phase 5: `EoGsVertexBuffer` + `polyline::` thin wrappers + `DisplayText*` migrated to `EoGsRenderDevice*`

## Architecture Overview

### Render Target Strategy: `ID2D1HwndRenderTarget`
- Simplest D2D render target — renders directly to the `AeSysView` HWND.
- `BeginDraw()` / `EndDraw()` bracket each frame.
- Device loss: `EndDraw()` returns `D2DERR_RECREATE_TARGET` → discard all device-dependent resources and recreate.
- Resize: call `Resize()` in `OnSize` — avoids full target recreation.
- **Replaces the Phase 4 GDI back buffer** — `ID2D1HwndRenderTarget` is inherently double-buffered.

### Alternative: `ID2D1DCRenderTarget` (Considered, Deferred)
- Wraps a CDC* — useful for printer output but adds `BindDC()` complexity and loses HWND benefits.
- Keep as a future option for printing. Phase 6 targets screen rendering only.

### Factory Singleton
- `ID2D1Factory` (single-threaded) — one per process, created at app startup (`AeSys::InitInstance`).
- `IDWriteFactory` — one per process, created alongside D2D factory.
- Both stored as `Microsoft::WRL::ComPtr<>` in `AeSys` (the application class).

### Resource Ownership
| Resource | Owner | Lifetime |
|----------|-------|----------|
| `ID2D1Factory` | `AeSys` | App lifetime |
| `IDWriteFactory` | `AeSys` | App lifetime |
| `ID2D1HwndRenderTarget` | `AeSysView` | View lifetime (recreated on device loss) |
| `ID2D1SolidColorBrush` | `EoGsRenderDeviceDirect2D` | Per-frame or cached |
| `ID2D1StrokeStyle` | `EoGsRenderDeviceDirect2D` | Cached by dash pattern |
| `IDWriteTextFormat` | `EoGsRenderDeviceDirect2D` | Cached by LOGFONT signature |

## `GetCDC()` Escape Hatch Dependency Analysis

17 call sites currently use `GetCDC()` to access `CDC*` from within `Display()` methods:

| File | Purpose | D2D Strategy |
|------|---------|-------------|
| `EoDbLine.cpp` | `renderState.SetPen()` + `renderState.SetColor()` | Phase 6.3: Migrate `renderState` to `EoGsRenderDevice*` |
| `EoDbConic.cpp` | `renderState.SetPen()` | Same |
| `EoDbEllipse.cpp` | `renderState.SetPen()` | Same |
| `EoDbPoint.cpp` | `renderState.SetPen()` + `CPen` for circle | Phase 6.4: Point circle via D2D `Ellipse()` |
| `EoDbPolygon.cpp` (×2) | `renderState.SetPen()` + hatch fill | Same + hatch lines via D2D |
| `EoDbPolyline.cpp` | `renderState.SetPen()` + `DisplayWidthFill` | Same + width fill via D2D `Polygon()` |
| `EoDbSpline.cpp` | `renderState.SetPen()` | Same |
| `EoDbText.cpp` (×3) | `renderState.SetPen()` + TrueType font | Phase 6.5: DirectWrite for TrueType |
| `EoDbDimension.cpp` | `renderState.SetPen()` + `renderState.SetColor()` | Same |
| `EoDbViewport.cpp` | `CPen` for dotted boundary | Phase 6.4: D2D `SelectPen(PS_DOT, ...)` |
| `AeSysDoc.cpp` (×2) | `renderState.Save()/Restore()` + `renderState.SetROP2()` | Phase 6.6: ROP2 emulation |
| `EoGsVertexBuffer.cpp` | `renderState.SetLineType()` for dash patterns | Phase 6.3: Route through `EoGsRenderDevice*` |

### Key Insight
All 17 `GetCDC()` calls funnel through `renderState.*` methods. The critical migration is making `EoGsRenderState` accept `EoGsRenderDevice*` instead of `CDC*`. Once that's done, `GetCDC()` calls drop to zero in Display methods.

## Sub-Phase Breakdown

### Phase 6.1 — D2D/DWrite Factory Infrastructure
**Scope**: App-level COM setup, factory singletons, library linkage.

**Changes**:
- `Stdafx.h`: Add `#include <d2d1.h>`, `#include <dwrite.h>`, `#include <wrl/client.h>`
- `AeSys.vcxproj`: Add `d2d1.lib`, `dwrite.lib` to `AdditionalDependencies` (all 4 configurations)
- `AeSys.h`: Add `Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory` and `ComPtr<IDWriteFactory> m_dwriteFactory` members. Getters: `ID2D1Factory* D2DFactory()`, `IDWriteFactory* DWriteFactory()`
- `AeSys.cpp`: Create factories in `InitInstance()` via `D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, ...)` and `DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, ...)`. Release in `ExitInstance()` (ComPtr handles automatically).
- Validation: Build succeeds; factories are non-null on startup.

**Risk**: Low. COM is already initialized by MFC (`AfxOleInit`).

### Phase 6.2 — `EoGsRenderDeviceDirect2D` Skeleton
**Scope**: New class implementing all 32 interface methods with initial D2D render target.

**New files**: `EoGsRenderDeviceDirect2D.h`, `EoGsRenderDeviceDirect2D.cpp`

**Class design**:
```
class EoGsRenderDeviceDirect2D : public EoGsRenderDevice {
  ID2D1HwndRenderTarget* m_renderTarget;  // Non-owning — AeSysView owns lifetime
  IDWriteFactory* m_dwriteFactory;        // Non-owning — AeSys owns lifetime

  // Cached device-dependent resources
  ComPtr<ID2D1SolidColorBrush> m_brush;
  ComPtr<ID2D1SolidColorBrush> m_fillBrush;

  // Pen state tracking
  D2D1_COLOR_F m_penColor{};
  float m_penWidth{1.0f};
  int m_penStyle{PS_SOLID};  // For dash pattern mapping
  ComPtr<ID2D1StrokeStyle> m_strokeStyle;  // null = solid

  // Current position for MoveTo/LineTo emulation
  D2D1_POINT_2F m_currentPosition{};

  // Text state
  D2D1_COLOR_F m_textColor{};
  UINT m_textAlign{TA_LEFT | TA_TOP};
  ComPtr<IDWriteTextFormat> m_textFormat;
  ...
};
```

**Method implementation strategy**:
| Category | D2D Implementation |
|----------|--------------------|
| `MoveTo` | Store `m_currentPosition` |
| `LineTo` | `DrawLine(m_currentPosition, newPoint, brush, width, strokeStyle)` → update position |
| `Polyline` | Loop `DrawLine` for each segment (or build `ID2D1PathGeometry`) |
| `Polygon` | `ID2D1PathGeometry` → `FillGeometry` + `DrawGeometry` |
| `Ellipse` | `DrawEllipse` / `FillEllipse` |
| `Rectangle` | `DrawRectangle` / `FillRectangle` |
| `SetPixel` | `FillRectangle(1×1)` |
| `SelectPen` | Update `m_penColor`, `m_penWidth`, create `ID2D1StrokeStyle` for non-solid |
| `RestorePen` | Restore previous pen state (stack or single-level) |
| `SelectSolidBrush` | Create/recolor `m_fillBrush` |
| `SelectNullBrush` | Set fill flag to skip fill in `Polygon`/`Rectangle` |
| `TextOut` | `IDWriteTextLayout` → `DrawTextLayout` |
| `ExtTextOut` | Same with clip rect |
| `SetTextColor` | Update `m_textColor` |
| `SetTextAlign` | Store alignment flags, apply during `DrawTextLayout` |
| `SetBkMode` | Store mode (TRANSPARENT = no background rect) |
| `SetBkColor` | Store color for OPAQUE mode background |
| `SelectFont` | Create `IDWriteTextFormat` from `LOGFONT` |
| `SetROP2` | **R2_COPYPEN**: no-op (default). **R2_XORPEN**: See Phase 6.6 notes |
| `GetDeviceCaps` | Return values from `GetDpiForWindow()` and render target pixel size |
| `GetClipBox` | Return render target pixel size as RECT |
| `PushClipRect` | `PushAxisAlignedClip(D2D1::RectF(...), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE)` |
| `PopClipRect` | `PopAxisAlignedClip()` |
| `StretchBlt` | **No-op or assert** — D2D frame buffer replaces GDI BitBlt |
| `SelectPalette` | No-op, return nullptr — D2D is true-color |
| `RealizePalette` | No-op |
| `GetCDC` | **Return nullptr** — D2D backend has no CDC |

**Validation**: Build succeeds with all methods stubbed. No visual test yet.

### Phase 6.3 — Migrate `EoGsRenderState` from `CDC*` to `EoGsRenderDevice*`
**Scope**: The critical enabler — `renderState.SetPen()`, `SetColor()`, `SetLineType()`, `SetFontDefinition()`, `Save()/Restore()`, `SetROP2()`, `ManagePenResources()`.

**Current signatures** (all take `CDC*`):
```cpp
void SetPen(AeSysView*, CDC*, int16, int16);
void SetPen(AeSysView*, CDC*, int16, int16, const wstring&);
void SetPen(AeSysView*, CDC*, int16, int16, const wstring&, LineWeight, double);
void SetColor(CDC*, int16);
void SetLineType(CDC*, int16);
void SetFontDefinition(CDC*, const EoDbFontDefinition&);
void ManagePenResources(CDC*, int16, int, int16);
int SetROP2(CDC*, int);  // New wrapper needed
void Restore(CDC*, int);
```

**Migration approach**: Overload with `EoGsRenderDevice*` variants that call through to the render device interface:
1. `ManagePenResources(EoGsRenderDevice*, ...)` → calls `renderDevice->SelectPen(style, width, color)` instead of `::CreatePen` + `SelectObject`.
2. `SetPen(AeSysView*, EoGsRenderDevice*, ...)` → same logic but calls the new `ManagePenResources` overload.
3. `SetColor(EoGsRenderDevice*, ...)`, `SetLineType(EoGsRenderDevice*, ...)` → thin wrappers.
4. `Save()`/`Restore(EoGsRenderDevice*, ...)` → save/restore internal state; let render device handle its own state.

**Then update all 17 `GetCDC()` call sites** in Display methods to pass `renderDevice` directly to `renderState.*`.

**After this phase**: Zero `GetCDC()` calls in entity Display methods. The escape hatch becomes a dead letter for the rendering pipeline.

**Risk**: Medium. `ManagePenResources` has an 8-slot LRU pen cache that's GDI-specific. The D2D path needs a simpler model (just set color/width/style directly — no object caching needed). The overloaded methods dispatch by backend type.

### Phase 6.4 — Render Target Integration in `AeSysView`
**Scope**: Create `ID2D1HwndRenderTarget` in `AeSysView`, wire up `BeginDraw`/`EndDraw` frame lifecycle.

**Changes to `AeSysView`**:
```cpp
// New members
ComPtr<ID2D1HwndRenderTarget> m_d2dRenderTarget;
bool m_useD2D{false};  // Toggle for A/B testing
```

- `OnSize`: Call `m_d2dRenderTarget->Resize(D2D1::SizeU(cx, cy))` when D2D is active.
- `OnDraw`: When `m_useD2D`:
  1. `m_d2dRenderTarget->BeginDraw()`
  2. `m_d2dRenderTarget->Clear(D2D1::ColorF(Eo::ViewBackgroundColor))`
  3. Create `EoGsRenderDeviceDirect2D renderDevice(m_d2dRenderTarget.Get(), dwriteFactory)`
  4. `document->DisplayAllLayers(this, &renderDevice)`
  5. `hr = m_d2dRenderTarget->EndDraw()`
  6. If `hr == D2DERR_RECREATE_TARGET` → `DiscardDeviceResources()` + `InvalidateScene()`
- `OnDraw`: When `!m_useD2D`: existing GDI back buffer path (unchanged).

**Toggle**: Menu item `View > Use Direct2D` or `#define USE_DIRECT2D` compile-time flag.

**GDI back buffer coexistence**: When D2D is active, the GDI back buffer is not used. When GDI is active, D2D render target is not created. Clean separation.

### Phase 6.5 — Text Rendering via DirectWrite
**Scope**: `TextOut`, `ExtTextOut`, `SelectFont` implementations using `IDWriteTextLayout`.

**LOGFONT → IDWriteTextFormat mapping**:
```
fontFamily = logFont->lfFaceName
fontSize = abs(logFont->lfHeight) * 72.0f / dpi  // Convert device units to points
fontWeight = logFont->lfWeight → DWRITE_FONT_WEIGHT
fontStyle = logFont->lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL
```

**Text alignment**: D2D `DrawTextLayout` uses (x, y) as the top-left corner of the layout box. GDI's `TA_BASELINE` / `TA_BOTTOM` / `TA_CENTER` require metric-based offset computation via `IDWriteTextLayout::GetMetrics()`.

**Stroke font text**: Already renders through `polyline::End(view, renderDevice, ...)` — works unchanged through D2D line drawing. Only TrueType font rendering needs DirectWrite.

### Phase 6.6 — ROP2 / XOR Emulation
**Scope**: Handle `R2_XORPEN` used by rubberband and `OnUpdate` erase/trap rendering.

**D2D has no XOR drawing mode.** Strategies:
1. **Rubberband**: Draw alpha-blended overlay geometry after the scene. Scene is retained in the D2D render target; rubberband is transient. This is the recommended approach from the copilot-instructions.
2. **OnUpdate erase/trap**: The XOR erase pattern (draw same geometry twice to erase) can be replaced by full scene re-render (fast with D2D) or by maintaining a damage list.
3. **Short-term**: `SetROP2(R2_XORPEN)` on the D2D backend can fall back to GDI via `GetDC()` + XOR draw + `ReleaseDC()`. This is ugly but preserves behavior during transition.

**Recommended**: Defer XOR emulation. Keep rubberband and OnUpdate incremental hints on GDI (they bypass the render device — they use screen DC directly in `OnMouseMove` and `OnUpdate`). Phase 6 focuses on the full-scene `OnDraw` path only.

### Phase 6.7 — Viewport Clipping
**Scope**: `DisplayModelSpaceThroughViewports` uses `PushClipRect`/`PopClipRect` for paper-space viewport rendering.

**D2D mapping**:
- `PushClipRect` → `PushAxisAlignedClip(D2D1::RectF(...), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE)`
- `PopClipRect` → `PopAxisAlignedClip()`
- These are direct 1:1 mappings — straightforward.

### Phase 6.8 — Polygon Fill and Hatch
**Scope**: `EoDbPolygon::Display()` uses `Polygon()` for solid/hatch fill.

**D2D mapping**:
- **Solid fill**: `ID2D1PathGeometry` from vertex array → `FillGeometry(path, fillBrush)` + `DrawGeometry(path, strokeBrush, width)`.
- **Hatch fill**: AeSys computes hatch lines procedurally and draws them as individual line segments. These already go through `polyline::End` → `EoGsRenderDevice::LineTo` — no special hatch handling needed in D2D.

## File Inventory

### New Files
| File | Purpose |
|------|---------|
| `EoGsRenderDeviceDirect2D.h` | D2D backend class declaration |
| `EoGsRenderDeviceDirect2D.cpp` | D2D backend implementation |

### Modified Files
| File | Change |
|------|--------|
| `Stdafx.h` | Add `<d2d1.h>`, `<dwrite.h>`, `<wrl/client.h>` |
| `AeSys.vcxproj` | Add `d2d1.lib`, `dwrite.lib`; add new `.h`/`.cpp` to project |
| `AeSys.vcxproj.filters` | Add new files to filter tree |
| `AeSys.h` | D2D/DWrite factory `ComPtr<>` members and getters |
| `AeSys.cpp` | Factory creation in `InitInstance()` |
| `AeSysView.h` | `ComPtr<ID2D1HwndRenderTarget>`, `m_useD2D` flag |
| `AeSysView.cpp` | `OnDraw` D2D path, `OnSize` resize, device loss recovery |
| `EoGsRenderState.h` | Overloaded methods accepting `EoGsRenderDevice*` |
| `EoGsRenderState.cpp` | `ManagePenResources`/`SetPen`/`SetColor`/`SetLineType` D2D-aware overloads |
| `EoDbLine.cpp` | Replace `GetCDC()` → pass `renderDevice` to `renderState` |
| `EoDbConic.cpp` | Same |
| `EoDbEllipse.cpp` | Same |
| `EoDbPoint.cpp` | Same |
| `EoDbPolygon.cpp` (×2) | Same |
| `EoDbPolyline.cpp` | Same |
| `EoDbSpline.cpp` | Same |
| `EoDbText.cpp` (×3) | Same |
| `EoDbDimension.cpp` | Same |
| `EoDbViewport.cpp` | Same |
| `AeSysDoc.cpp` (×2) | Same |
| `EoGsVertexBuffer.cpp` | Same |

## Execution Order

### Session A (Foundation + Lines + Polylines)
1. **6.1** — D2D/DWrite factory infrastructure
2. **6.2** — `EoGsRenderDeviceDirect2D` skeleton (all 32 methods)
3. **6.3** — Migrate `EoGsRenderState` CDC* → `EoGsRenderDevice*` (eliminate `GetCDC()` from Display)
4. **6.4** — Render target in `AeSysView` + `BeginDraw`/`EndDraw` + toggle
5. Build + test with LINE/ARC/CIRCLE/POLYLINE rendering

### Session B (Text + Polish)
6. **6.5** — DirectWrite text rendering
7. **6.7** — Viewport clipping
8. **6.8** — Polygon fill
9. Visual validation with DXF test files
10. Device loss recovery testing

### Session C (XOR / Incremental — Can Defer)
11. **6.6** — ROP2/XOR strategy for rubberband and OnUpdate
12. Toggle D2D as default

## D2D API Quick Reference

### Core Patterns
```cpp
// Factory creation (once, in InitInstance)
D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
    reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));

// Render target creation (once per view, or on device loss)
D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps =
    D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(cx, cy));
factory->CreateHwndRenderTarget(rtProps, hwndProps, &m_renderTarget);

// Brush creation (device-dependent — recreate on device loss)
m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(r, g, b), &m_brush);

// Frame rendering
m_renderTarget->BeginDraw();
m_renderTarget->Clear(D2D1::ColorF(bgColor));
// ... all draw calls ...
HRESULT hr = m_renderTarget->EndDraw();
if (hr == D2DERR_RECREATE_TARGET) { DiscardDeviceResources(); }

// Line drawing
m_renderTarget->DrawLine(
    D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2),
    m_brush, strokeWidth, m_strokeStyle.Get());

// Polygon (path geometry)
ComPtr<ID2D1PathGeometry> path;
factory->CreatePathGeometry(&path);
ComPtr<ID2D1GeometrySink> sink;
path->Open(&sink);
sink->BeginFigure(D2D1::Point2F(pts[0].x, pts[0].y), D2D1_FIGURE_BEGIN_FILLED);
for (int i = 1; i < count; ++i)
    sink->AddLine(D2D1::Point2F(pts[i].x, pts[i].y));
sink->EndFigure(D2D1_FIGURE_END_CLOSED);
sink->Close();
m_renderTarget->FillGeometry(path.Get(), m_fillBrush.Get());
m_renderTarget->DrawGeometry(path.Get(), m_brush, strokeWidth);

// Resize (no recreation needed)
m_renderTarget->Resize(D2D1::SizeU(cx, cy));
```

### COLORREF ↔ D2D Color
```cpp
inline D2D1_COLOR_F ColorRefToD2D(COLORREF cr) {
    return D2D1::ColorF(
        GetRValue(cr) / 255.0f,
        GetGValue(cr) / 255.0f,
        GetBValue(cr) / 255.0f);
}
```

### Stroke Styles (Pen Style Mapping)
```cpp
// PS_SOLID → nullptr (no stroke style = solid)
// PS_DOT → D2D1_DASH_STYLE_DOT
// PS_DASH → D2D1_DASH_STYLE_DASH
// PS_DASHDOT → D2D1_DASH_STYLE_DASH_DOT
// PS_DASHDOTDOT → D2D1_DASH_STYLE_DASH_DOT_DOT
// PS_NULL → skip draw call entirely

D2D1_STROKE_STYLE_PROPERTIES strokeProps = D2D1::StrokeStyleProperties();
strokeProps.dashStyle = D2D1_DASH_STYLE_DOT;
factory->CreateStrokeStyle(strokeProps, nullptr, 0, &m_strokeStyle);
```

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Device loss during heavy rendering | Visual glitch → blank frame | `EndDraw` check + resource recreation + `InvalidateScene()` |
| DPI mismatch between GDI coordinates and D2D DIPs | Scaled/offset rendering | Use `GetDpiForWindow()` and `D2D1_RENDER_TARGET_PROPERTIES::dpiX/Y` |
| `GetCDC()` returns nullptr on D2D backend | Crash in unmigrated code | Phase 6.3 eliminates all Display-path calls. Assert on `GetCDC()` for D2D. |
| Rubberband XOR drawing | No D2D equivalent | Keep rubberband on GDI screen DC (existing behavior) until Phase 7+ |
| Text metrics mismatch (GDI vs DirectWrite) | Slightly different text positioning | Accept minor differences; DirectWrite metrics are more accurate |
| `ManagePenResources` 8-slot LRU cache | Not needed for D2D | D2D path bypasses cache — `SelectPen` directly sets brush color/width |

## Success Criteria
1. All DXF test files render identically (within antialiasing differences) in D2D mode
2. Zero `GetCDC()` calls in entity `Display()` methods
3. `View > Use Direct2D` toggle switches backends without restart
4. Device loss recovery works (resize rapidly, alt-tab during render)
5. Sub-second full-scene refresh maintained (D2D should be equal or faster than the Phase 5 GDI baseline)
6. Build clean with `/W4 /WX` across all 4 configurations
