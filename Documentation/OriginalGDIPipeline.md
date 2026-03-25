# Original GDI Rendering Pipeline — AeSys Baseline Audit

> **Snapshot date**: pre-Direct2D migration, clean git state.
> **Purpose**: Permanent reference for the GDI pipeline being replaced. This document does NOT evolve — it records the state at migration start.

## Executive Summary

AeSys uses **GDI exclusively** (no GDI+). All rendering flows through MFC's `CDC*` wrapper around Win32 `HDC`. There are no retained display lists, no off-screen frame buffer, and no hardware acceleration. Every `WM_PAINT` redraws the entire scene from entity data.

---

## Pipeline Architecture (5 Layers)

```
┌───────────────────────────────────────────────────────┐
│ 1. FRAME DISPATCH                                     │
│    OnDraw(CDC*) → DisplayAllLayers(view, dc)          │
│    OnMouseMove  → rubberband XOR lines/rects          │
│    OnPrint      → same pipeline to printer DC         │
├───────────────────────────────────────────────────────┤
│ 2. LAYER ITERATION                                    │
│    EoDbLayer::Display(view, dc)                       │
│    Sets static layer color/linetype/weight/scale      │
│    Palette swap: Eo::ColorPalette vs GrayPalette      │
├───────────────────────────────────────────────────────┤
│ 3. PRIMITIVE VIRTUAL DISPATCH                         │
│    EoDbPrimitive::Display(AeSysView*, CDC*) = 0       │
│    11 concrete types, each calls:                     │
│      renderState.SetPen(view, dc, ...)                │
│      then emits geometry via polyline:: or direct GDI │
├───────────────────────────────────────────────────────┤
│ 4. GEOMETRY EMISSION (pseudo–vertex buffer)           │
│    polyline::BeginLineStrip/Loop()                    │
│    polyline::SetVertex(point3d)                       │
│    polyline::__End(view, dc, lineType)                │
│    → accumulates into file-scope EoGePoint4dArray     │
├───────────────────────────────────────────────────────┤
│ 5. RASTERIZATION (GDI calls)                          │
│    ModelViewTransformPoints → clip → ProjectToClient  │
│    dc->MoveTo / LineTo / Polyline                     │
│    dc->Polygon (filled shapes)                        │
│    dc->SetPixel (grid, points, dot dashes)            │
│    dc->TextOutW (TrueType text)                       │
│    dc->Ellipse (point circles)                        │
│    dc->StretchBlt (background image)                  │
│    dc->Rectangle (rubberband)                         │
│    ::CreatePen → 8-slot LRU pen cache                 │
│    LOGFONT + CreateFontIndirect (TrueType)            │
│    CBrush for solid polygon fills                     │
│    R2_XORPEN for interactive rubberband               │
└───────────────────────────────────────────────────────┘
```

---

## Layer 1 — Frame Dispatch

### OnDraw Entry Point (`AeSysView::OnDraw`)
- Called by MFC on `WM_PAINT` via `CView::OnPaint()`.
- Gets clip box from `deviceContext->GetClipBox()`. Returns immediately if empty.
- Calls `BackgroundImageDisplay(dc)` → `DisplayGrid(dc)` → `document->DisplayAllLayers(view, dc)`.
- Ends with `ValidateRect(nullptr)` — no partial invalidation.
- `m_ViewRendered` flag exists but is not connected to any off-screen buffer; it skips re-rendering when true, but nothing sets it reliably.

### OnMouseMove Rubberband
- `R2_XORPEN` erase-then-draw pattern for interactive feedback.
- `Lines` mode: `MoveTo`/`LineTo` with `CPen(PS_SOLID, 0, Eo::colorRubberband)`.
- `Rectangles` mode: `Rectangle()` with `NULL_BRUSH`.
- Directly acquires `GetDC()` / `ReleaseDC()` — not the paint DC.

### Printing
- Same `Display()` pipeline renders to a printer `CDC*`.
- `OnPrepareDC` sets up page metrics from `GetDeviceCaps(HORZSIZE/VERTSIZE)`.
- Viewport and view-transform are pushed/popped around the print pass.

---

## Layer 2 — Layer Iteration

### EoDbLayer::Display
```
Set static layer properties:
  EoDbPrimitive::SetLayerColor(ColorIndex())
  EoDbPrimitive::SetLayerLineTypeIndex(LineTypeIndex())
  EoDbPrimitive::SetLayerLineTypeName(...)
  EoDbPrimitive::SetLayerLineWeight(m_lineWeight)
  EoDbPrimitive::SetLayerLineTypeScale(m_lineTypeScale)

Swap global palette pointer:
  pColTbl = (detectable) ? Eo::ColorPalette : Eo::GrayPalette

Iterate groups:
  for each group → group->IsInView(view) → group->Display(view, dc)

Restore pColTbl
```

### Paper-Space Viewport Pipeline (`DisplayModelSpaceThroughViewports`)
- Walks paper-space layers for `EoDbViewport` primitives.
- For each viewport: `SaveDC` → `IntersectClipRect` → `PushViewTransform` → render model-space layers → `PopViewTransform` → `RestoreDC`.
- Off-center orthographic projection window computed from clip region / device dimensions.

---

## Layer 3 — Primitive Virtual Dispatch

### Virtual Signature
```cpp
virtual void Display(AeSysView* view, CDC* deviceContext) = 0;
```

### Per-Primitive Pattern
Every derived `Display()` follows the same pattern:
1. Resolve logical color and linetype (handles ByLayer/ByBlock).
2. Call `renderState.SetPen(view, dc, color, lineType, lineTypeName, lineWeight, lineTypeScale)`.
3. Emit geometry via `polyline::` functions or direct GDI calls.

### Primitive Types and Their Rendering

| Primitive | Geometry emission | Direct GDI? | Notes |
|-----------|------------------|-------------|-------|
| `EoDbLine` | `BeginLineStrip` → 2 vertices → `__End` | No | Simplest case |
| `EoDbConic` | `BeginLineStrip` → `GenerateApproximationVertices` → `__End` | No | Adaptive tessellation of arcs/ellipses |
| `EoDbEllipse` | `BeginLineStrip` → `GenPts` → `__End` | No | Legacy ellipse (pre-conic) |
| `EoDbPolyline` | `BeginLineStrip/Loop` → vertices + `TessellateArcSegment` → `__End` | No | Bulge arcs tessellated |
| `EoDbSpline` | `BeginLineStrip` → `GenPts` (B-spline) → `__End` | No | Cox–de Boor tessellation |
| `EoDbPolygon` | `Polygon_Display` (filled) or `BeginLineLoop` → `__End` (hollow) | `dc->Polygon` | Solid/hollow/hatch fill |
| `EoDbPolygon` (hatch) | `DisplayFilAreaHatch` scanline algorithm | `dc->SetPixel`, `EoGeLine::Display` | Per-pixel dash pattern dots |
| `EoDbText` (stroke) | `BeginLineStrip` per stroke → `__End` | No | Stroke font glyphs as polylines |
| `EoDbText` (TrueType) | — | `dc->TextOutW` | `LOGFONT` + `CreateFontIndirect` |
| `EoDbPoint` | — | `dc->SetPixel`, `dc->Ellipse` | Point styles: pixel, +, ×, circle, square |
| `EoDbBlockReference` | `PushModelTransform` → `block->Display(view, dc)` → `PopModelTransform` | No | Recursive through block geometry |
| `EoDbViewport` | — | `dc->MoveTo`/`LineTo` | Dotted rectangle boundary |
| `EoDbDimension` | Delegates to `EoDbLine::Display` + `EoDbText::Display` | Indirect | Dimension leaders + text |

---

## Layer 4 — polyline Namespace (Pseudo–Vertex Buffer)

### Heritage
Modeled after OpenGL 1.x immediate mode (`glBegin`/`glVertex`/`glEnd`). File-scope mutable state, single-threaded, non-reentrant.

### State
```cpp
namespace polyline {
  EoGePoint4dArray pts_;   // file-scope vertex accumulator
  bool LoopLine;           // close-back-to-first flag
}
```

### API
| Function | Purpose |
|----------|---------|
| `BeginLineStrip()` | Clear `pts_`, set `LoopLine = false` |
| `BeginLineLoop()` | Clear `pts_`, set `LoopLine = true` |
| `SetVertex(point3d)` | Append `EoGePoint4d(point)` to `pts_` |
| `__End(view, dc, lineType, lineTypeName)` | Flush: transform → clip → project → draw |
| `__Display(view, dc, pts, lineType)` | Custom dash pattern rendering |

### __End Dispatch Logic
```
If lineTypeName resolves to a named linetype with dashes:
  → Switch pen to PS_SOLID (index 1)
  → __Display(view, dc, pts_, lineType)   // custom dash rendering
  → Restore original pen

Else if continuous or stock type:
  → ModelViewTransformPoints(pts_)
  → AnyPointsInView? → MoveTo/LineTo loop
  → If LoopLine: LineTo back to pts_[0]

Else (legacy index fallback):
  → LookupUsingLegacyIndex → __Display
```

### __Display (Custom Dash Pattern Rendering)
For each polyline segment:
- Walks the dash element array from `EoDbLineType`.
- Computes dash/gap boundaries along the segment direction vector.
- For visible dashes: clips (`EoGePoint4d::ClipLine`), projects, draws via `dc->Polyline(clientPoints, 2)`.
- DPI-aware: dot elements use `1.0 / GetDpiForSystem()` pixel size.

---

## Layer 5 — GDI Call Sites

### Call-Site Census (27 `.cpp` files contain `CDC*`)

| Category | GDI API | Sites | Primary locations |
|----------|---------|-------|-------------------|
| Line drawing | `MoveTo`/`LineTo`/`Polyline` | 15 | `EoGePolyline.cpp`, `AeSysView.cpp`, `EoDbViewport.cpp` |
| Filled shapes | `Polygon`/`Ellipse`/`Rectangle` | 9 | `EoDbPolygon.cpp`, `EoDbPoint.cpp`, `AeSysView.cpp` |
| Pixels | `SetPixel` | 12 | `EoViConstraints.cpp` (grid), `EoDbPoint.cpp`, `EoDbPolygon.cpp` (hatch dots) |
| Text | `TextOutW` | 1 | `EoDbText.cpp` (TrueType path) |
| Bitmap | `StretchBlt`/`CreateCompatibleDC` | 10 | `AeSysView.cpp` (background image, printing) |
| Pen management | `CreatePen`/`SelectObject` | 30 | `EoGsRenderState.cpp` (LRU cache), throughout |
| Brush management | `CBrush`/`SelectStockObject` | ~8 | `EoDbPolygon.cpp`, `EoDbPoint.cpp`, `AeSysView.cpp` |
| Font management | `CFont`/`LOGFONT`/`CreateFontIndirect` | ~6 | `EoDbText.cpp` |
| Draw state | `SetROP2`/`SetBkMode`/`SetTextAlign`/`SetTextColor` | ~44 | Throughout |
| Device caps | `GetDeviceCaps` | 26 | DPI, printing metrics |
| Clipping | `SaveDC`/`RestoreDC`/`IntersectClipRect` | 3 | `AeSysDoc.cpp` (viewport pipeline) |

### Pen Resource Management (`ManagePenResources`)
- 8-slot LRU cache of `HPEN` handles.
- Keyed by `(COLORREF, lineType, penWidth)`.
- On miss: `::CreatePen(style, width, color)`, evict oldest slot.
- Line type → GDI pen style mapping: only `PS_NULL` and `PS_SOLID` are live; `PS_DOT`/`PS_DASH`/`PS_DASHDOT`/`PS_DASHDOTDOT` are dead code (superseded by `__Display` custom dash rendering).

### ROP2 Drawing Modes
- `R2_XORPEN` — rubberband lines and rectangles during mouse move.
- `R2_NOTXORPEN` — adjusted for white background.
- `R2_COPYPEN` — fallback for non-black/white backgrounds.
- Used in: `AeSysView.cpp` (rubberband), `EoGsRenderState.cpp`, various mode-specific preview functions.

### Color Table Globals
```cpp
COLORREF* pColTbl;              // Current active palette pointer
COLORREF  Eo::ColorPalette[256]; // Full-color palette
COLORREF  Eo::GrayPalette[256]; // Grayed-out palette for non-detectable layers
```
Layer display swaps `pColTbl` between these two arrays. All pen creation and `SetTextColor` calls use `pColTbl[colorIndex]`.

---

## Transform Pipeline (CPU-Only)

### World → Screen Coordinate Flow
```
EoGePoint3d (world/model coords)
  → EoGsModelTransform (block nesting stack, 4×4 matrix)
  → EoGsViewTransform (camera view matrix, 4×4)
  → EoGePoint4d (NDC, homogeneous)
  → ClipLine / IsInView (Cohen-Sutherland, software)
  → EoGsViewport::ProjectToClient (perspective divide + NDC→screen)
  → CPoint (device/client coords)
  → GDI draw calls
```

All transform math runs on the CPU. No GPU involvement.

### Key Transform Classes

| Class | Purpose |
|-------|---------|
| `EoGsModelTransform` | Push/pop stack for nested block transforms |
| `EoGsViewTransform` | Camera: position, target, up, view window, builds 4×4 |
| `EoGsViewport` | NDC → screen pixel mapping, perspective divide |
| `EoGeTransformMatrix` | 4×4 matrix: multiply, inverse, rotation factories |

---

## What Does NOT Depend on GDI

These subsystems are rendering-API-agnostic and require no changes during migration:

- **Geometry math**: `EoGeTransformMatrix`, `EoGeVector3d`, `EoGePoint3d`, `EoGeLine`
- **Entity data model**: `EoDbPrimitive` hierarchy (everything except `Display()`)
- **Tessellation**: `EoDbConic::GenerateApproximationVertices`, `EoDbSpline::GenPts`, `polyline::TessellateArcSegment`, `polyline::GeneratePointsForNPoly`
- **Selection**: `SelectUsingPoint`, `SelectUsingLine`, `SelectUsingRectangle`
- **Serialization**: PEG/TRA read/write, DXF import/export
- **Transform pipeline**: `EoGsModelTransform`, `EoGsViewTransform` (produces NDC; only the final projection-to-client step touches GDI via `CPoint`)

---

## Performance Characteristics (Pre-Migration Baseline)

| Issue | Impact | Root cause |
|-------|--------|------------|
| No frame buffer | Full scene redraw on every `WM_PAINT` | `OnDraw` renders directly to window DC |
| Per-primitive pen churn | GDI object create/select/delete per entity | `SetPen` → `ManagePenResources` on every `Display()` |
| CPU transform pipeline | Matrix multiply per vertex, per frame | No GPU projection |
| SetPixel loops | Thousands of kernel calls for grid/points | No batch pixel API in GDI |
| Hatch scanline | Individual line segments per scan row | `DisplayFilAreaHatch` software scanline fill |
| XOR rubberband | Flicker under DWM composition | `R2_XORPEN` incompatible with hardware compositing |
| No geometry caching | Tessellation recomputed every frame | Arcs, splines, etc. re-tessellated on each paint |

---

## Files Containing GDI Calls (27 `.cpp` files)

```
AeSys.cpp                  EoDbGroup.cpp           EoDbText.cpp
AeSysDoc.cpp               EoDbGroupList.cpp       EoDbViewport.cpp
AeSysView.cpp              EoDbLayer.cpp           EoDlgActiveViewKeyplan.cpp
DrawModeState.cpp          EoDbLine.cpp            EoGeLine.cpp
EoCtrlColorsButton.cpp     EoDbPoint.cpp           EoGePolyline.cpp
EoDBBlockReference.cpp     EoDbPolygon.cpp         EoGsRenderState.cpp
EoDbConic.cpp              EoDbPolyline.cpp        EoViConstraints.cpp
EoDbDimension.cpp          EoDbSpline.cpp          IdleState.cpp
EoDbEllipse.cpp                                    ModeLine.cpp
                                                   WndProcPreview.cpp
```

---