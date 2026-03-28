# Direct2D Render Pipeline — Test Regimen

> **Purpose**: Systematic manual testing guide for gaining confidence in the Phase 6 Direct2D backend after it replaced GDI as the default renderer. No formal test framework is assumed — all tests are visual / interactive.

> **Methodology**: Each section contains a numbered checklist. For each item, note ✅ pass, ❌ fail + symptom, or ⚠️ minor difference (acceptable antialiasing/metric delta). Compare D2D vs GDI side-by-side using `View > Use Direct2D` toggle where noted.

---

## 1. Backend Toggle and Lifecycle

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 1.1 | **D2D starts as default** | Launch application, open any DXF file | Drawing renders without blank frame; no crash |
| 1.2 | **Toggle D2D → GDI at runtime** | `View > Use Direct2D` (uncheck) | Scene redraws immediately via GDI back buffer; no artifacts |
| 1.3 | **Toggle GDI → D2D at runtime** | `View > Use Direct2D` (check) | Scene redraws immediately via D2D; no artifacts |
| 1.4 | **Rapid toggle** | Toggle 10× in quick succession | No crash, no resource leak, no blank frames |
| 1.5 | **New document with D2D active** | `File > New` while D2D is default | Empty view clears to background color |
| 1.6 | **Open second document** | Open two DXF files in separate MDI children | Both views render independently; toggling D2D in one does not affect the other |
| 1.7 | **Close view while D2D active** | Close an MDI child window | No crash; D2D resources released cleanly |
| 1.8 | **GDI fallback on D2D failure** | (Difficult to trigger naturally) Verify the code path exists: if `CreateD2DRenderTarget` fails in `OnSize`, `m_useD2D` is set to `false` | Code inspection / stress test with graphics driver disabled |

---

## 2. Window and Resize Behavior

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 2.1 | **Resize window** | Drag window edges to resize | Scene re-renders at new size; no clipping, no stale pixels at edges |
| 2.2 | **Maximize / restore** | Maximize the window, then restore | Clean redraw at both sizes |
| 2.3 | **Minimize / restore** | Minimize to taskbar, then restore | Scene redraws from D2D render target (inherently double-buffered) |
| 2.4 | **Resize to very small** | Drag window to ~50×50 pixels | No crash; content clipped but render target valid |
| 2.5 | **Resize to very large** | Maximize on high-resolution display | No crash; performance acceptable |
| 2.6 | **Multi-monitor move** | Drag window between monitors with different DPI | Scene re-renders at correct DPI; text and line weights scale properly |
| 2.7 | **DPI change (Settings)** | Change Windows display scale (100% → 150%) while app is running | Render target handles DPI change gracefully |

---

## 3. Entity Rendering — Primitive Types

Use the DXF test files listed below. For each entity type, **compare D2D vs GDI** side-by-side using the toggle.

### 3.1 Lines (`EoDbLine`)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.1.1 | Basic lines | Any file with LINE entities | Lines appear at correct positions, colors, and widths |
| 3.1.2 | Thin lines (1px) | Default pen width | Clean single-pixel lines (D2D may show antialiasing) |
| 3.1.3 | Thick lines | Enable `View > Pen Widths` | Line weight renders at correct pixel width |
| 3.1.4 | Line colors | File with multiple colored lines | All colors correct; ByLayer resolution works |

### 3.2 Arcs and Circles (`EoDbConic`)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.2.1 | Circles | `Ellipse_And_EllipticalArc_Test.dxf` | Circles rendered as tessellated polylines; smooth appearance |
| 3.2.2 | Arcs | Same file | Arc start/end angles correct; sweep direction correct |
| 3.2.3 | Negative-Z extrusion | `Ellipse_NegZ_CW_Test.dxf` | Arcs with `[0,0,-1]` extrusion render correctly (CW in OCS) |
| 3.2.4 | Small arcs | Zoomed-in view of small radius arcs | No degenerate tessellation; arcs visible |

### 3.3 Ellipses (`EoDbConic` — Ellipse/EllipticalArc subclass)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.3.1 | Full ellipses | `Ellipse_And_EllipticalArc_Test.dxf` | Correct major/minor axis ratio; smooth curve |
| 3.3.2 | Elliptical arcs | Same file | Partial ellipse arcs; correct start/end angles |
| 3.3.3 | Negative-Z ellipses | `Ellipse_NegZ_CW_Test.dxf` | E1–E6 entities; verify vs baseline entities |
| 3.3.4 | Radial arcs (neg-Z) | Same file | R1–R6 entities; verify sweep direction |

### 3.4 Polylines (`EoDbPolyline`)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.4.1 | LWPOLYLINE basic | `LWPolyline_Bulge_Test.dxf` | Straight-segment polylines render correctly |
| 3.4.2 | Bulge arcs | Same file | Positive/negative/mixed bulge renders as arc segments |
| 3.4.3 | Semicircle bulge | Same file | 180° arc (bulge = ±1.0) |
| 3.4.4 | Closed polyline | Same file (closed entities) | Last segment connects back to first vertex |
| 3.4.5 | Heavy 2D polyline | `Heavy_Polyline_Subtypes.dxf` | Basic/closed/elevation subtypes |
| 3.4.6 | Heavy 3D polyline | Same file | 3D vertex coordinates |
| 3.4.7 | Per-vertex width | `Single Polyline with vertex widths.dxf` | Filled trapezoids with linearly interpolated widths |
| 3.4.8 | Global vertex width | `Single Polyline (68 vertices) with global vertex width (2.5).dxf` | Uniform width fill along the entire polyline |
| 3.4.9 | Plinegen flag | `Heavy_Polyline_Subtypes.dxf` (plinegen entities) | Dash pattern generated across vertices (vs per-segment) |
| 3.4.10 | Bulge + width combo | Same file | Arc segments with width fill |

### 3.5 Splines (`EoDbSpline`)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.5.1 | Open splines | `Spline_Test.dxf` | Smooth cubic B-spline curve through tessellated points |
| 3.5.2 | Closed splines | Same file (if present) | Curve closes smoothly |
| 3.5.3 | Few control points | 3–4 control point splines | No degenerate rendering |

### 3.6 Text (`EoDbText`)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.6.1 | Stroke font text | Any file with TEXT entities, TrueType off (`View` setting) | Characters rendered as polyline strokes |
| 3.6.2 | TrueType text | Enable `View > True Type Fonts` | Text rendered via DirectWrite; correct position and size |
| 3.6.3 | Rotated text | File with non-zero rotation TEXT | Text baseline rotates correctly |
| 3.6.4 | Text alignment | Left/Center/Right justified text | Anchor point aligns correctly |
| 3.6.5 | MTEXT (multiline) | File with MTEXT entities | `\P` paragraph breaks produce line spacing |
| 3.6.6 | Formatting codes | MTEXT with `\A`, `\S`, `\P` | Alignment changes, stacked fractions, paragraph breaks render |
| 3.6.7 | ATTRIB text | `RoomNumber_Block_Insert.dxf` | Attribute values (101, 102, CONF-A) display at correct positions |
| 3.6.8 | Text height variation | Mix of small and large text in one drawing | All sizes render proportionally |
| 3.6.9 | Empty/blank text | Edge case — zero-length strings | No crash; nothing rendered |

### 3.7 Points (`EoDbPoint`)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.7.1 | Point display | File with POINT entities | Small circle or pixel at point location |
| 3.7.2 | Point color | Colored points | Correct color |

### 3.8 Polygons / Hatches (`EoDbPolygon`)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.8.1 | Solid fill | File with solid-filled regions | Filled with correct color |
| 3.8.2 | Hatch patterns | `Hatch_Boundary_Test_Mixed.dxf` | Hatch lines at correct angle and spacing |
| 3.8.3 | Hollow fill (null brush) | Polygon outline only | Outline drawn, interior transparent |

### 3.9 Block References (`EoDbBlockReference` + INSERT)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.9.1 | Simple block insert | `RoomNumber_Block_Insert.dxf` | Rectangle block renders at each insert location |
| 3.9.2 | Scaled insert | Same file (X-scale 1.5 instance) | Block geometry scaled non-uniformly |
| 3.9.3 | Block with attributes | Same file | ATTRIB text values positioned correctly within block |

### 3.10 Dimensions (`EoDbLabeledLine`)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.10.1 | Dimension display | File with dimension entities | Dimension lines, arrows, and text render correctly |

### 3.11 Viewports (`EoDbViewport`)

| # | Test | File | Expected |
|---|------|------|----------|
| 3.11.1 | Paper-space viewport boundary | File with paper-space layout | Dotted rectangle at viewport boundary |
| 3.11.2 | Model-space through viewport | Same file | Model-space geometry clipped to viewport region |
| 3.11.3 | Multiple viewports | Layout with 2+ viewports | Each viewport clips independently |

---

## 4. Line Types and Dash Patterns

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 4.1 | **Continuous linetype** | Entity with linetype = Continuous | Solid line |
| 4.2 | **Named dash patterns** | Entity with DASHED, CENTER, HIDDEN, etc. | Named linetype pattern renders correctly |
| 4.3 | **ByLayer linetype** | Entity inheriting linetype from layer | Correct pattern resolved from layer |
| 4.4 | **Linetype scale** | Vary global and per-entity linetype scale | Dash/gap lengths scale proportionally |
| 4.5 | **Zoom and dash patterns** | Zoom in/out on dashed entity | Pattern density adjusts with view scale (if applicable) |

---

## 5. Pen Width and Line Weight

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 5.1 | **Pen widths off** | `View > Pen Widths` unchecked | All lines render at 1px |
| 5.2 | **Pen widths on** | `View > Pen Widths` checked | Lines render at DXF-specified widths |
| 5.3 | **ByLayer line weight** | Entity with `kLnWtByLayer` | Resolves to layer's line weight |
| 5.4 | **ByLwDefault line weight** | Entity with `kLnWtByLwDefault` | Resolves to 0.25mm default |
| 5.5 | **Heavy weights** | Entities with weight ≥ 1.0mm | Thick lines with correct pixel width for DPI |
| 5.6 | **D2D vs GDI comparison** | Toggle backend with pen widths on | Width should match (D2D may show smoother edges) |

---

## 6. Color and Layer Properties

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 6.1 | **Direct entity color** | Entities with explicit color | Correct RGB rendering |
| 6.2 | **ByLayer color** | Entity color = ByLayer | Resolves to layer color |
| 6.3 | **Layer on/off** | Toggle layer visibility | Entities on hidden layers disappear; scene redraws |
| 6.4 | **Layer freeze/thaw** | If supported — freeze a layer | Frozen layer entities excluded from display |
| 6.5 | **Gray palette** | If gray mode is available | Entities render in grayscale |
| 6.6 | **Background color** | Verify `Eo::ViewBackgroundColor` clear | Background color correct in D2D (`Clear()` call) |

---

## 7. View Navigation

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 7.1 | **Zoom in (wheel up)** | Scroll wheel forward | View zooms toward cursor position; scene re-renders |
| 7.2 | **Zoom out (wheel down)** | Scroll wheel backward | View zooms away from cursor; scene re-renders |
| 7.3 | **Rapid zoom** | Scroll wheel rapidly in/out | No stuttering, no blank frames, no accumulation artifacts |
| 7.4 | **Middle-button pan** | Middle-click and drag | View pans smoothly; scene follows cursor |
| 7.5 | **Zoom extents** | Zoom to fit all geometry | All entities visible; correct framing |
| 7.6 | **Zoom to very deep level** | Zoom in on fine detail 20× | No precision artifacts; geometry remains sharp |
| 7.7 | **Zoom way out** | Zoom out until drawing is a speck | No crash; drawing still visible as small cluster |

---

## 8. Rubberband (Interactive Feedback)

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 8.1 | **Line rubberband (D2D)** | Enter a mode that shows line rubberband (e.g., Draw mode); move mouse | Gray rubberband line follows cursor from anchor point |
| 8.2 | **Rectangle rubberband (D2D)** | Enter Trap mode; move mouse | Gray rubberband rectangle follows cursor |
| 8.3 | **Rubberband appearance** | Observe rubberband line/rect | Solid gray (`Eo::colorRubberband = #666666`); no flickering |
| 8.4 | **Rubberband disable** | Complete an operation or press Escape | Rubberband disappears cleanly; no ghost image |
| 8.5 | **Rubberband during rapid mouse movement** | Move mouse quickly while rubberband active | Rubberband tracks smoothly without lag or tearing |
| 8.6 | **GDI rubberband comparison** | Toggle to GDI; repeat 8.1–8.5 | XOR rubberband works; visually different but functional |
| 8.7 | **No rubberband residue** | After disabling rubberband, pan/zoom | No leftover rubberband ghost lines in the scene |

---

## 9. Selection and Interaction

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 9.1 | **Point selection** | Click on an entity in Edit/Trap mode | Entity highlights or is selected correctly |
| 9.2 | **Rectangle selection** | Drag a selection rectangle | Entities within rectangle are selected |
| 9.3 | **Selection highlight** | Selected entity visual feedback | Highlight or color change visible |
| 9.4 | **Control point selection** | Click on entity control point | Control point engaged; cursor changes |
| 9.5 | **Drag / transform** | Select and drag an entity | Entity moves in real-time; scene updates |
| 9.6 | **Undo after edit** | Modify entity, then Undo | Entity reverts; scene redraws correctly |

---

## 10. Editing Operations with Visual Feedback

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 10.1 | **Add entity (line)** | Draw mode → click two points | New line appears correctly |
| 10.2 | **Add entity (text)** | Annotate mode → enter text | New text renders at specified position |
| 10.3 | **Delete entity** | Select and delete | Entity removed; scene redraws without it |
| 10.4 | **Copy / paste** | Copy entity, paste elsewhere | Pasted entity renders at new position |
| 10.5 | **Move** | Move an entity | Entity position updates; original position cleared |
| 10.6 | **Break polyline** | Break a polyline primitive | Two segments result; both render correctly |
| 10.7 | **Explode block** | Explode a block reference | Individual primitives render independently |
| 10.8 | **OnUpdate hints** | All of the above | Incremental hint updates trigger `InvalidateScene()` under D2D |

---

## 11. File I/O and Round-Trip

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 11.1 | **Open DXF** | Open each DXF test file | All entities render; no crash |
| 11.2 | **Open PEG (V1)** | Open a legacy .peg file | Entities render with auto-assigned handles |
| 11.3 | **Open PEG (V2)** | Open a V2 .peg file (with paper-space) | Both spaces render; paper-space viewports functional |
| 11.4 | **Save PEG** | Save an opened DXF as .peg, reopen | Geometry preserved; rendering matches |
| 11.5 | **Export DXF** | Export to DXF, reopen | Round-trip fidelity; rendering matches original |
| 11.6 | **Empty document** | New file, save, reopen | No crash; empty view |
| 11.7 | **Large file** | Open `AEC_Bldg_Plan_Sample-(2010)-1.dxf` or `Floor Plan Sample.dxf` | Renders completely; acceptable performance |

---

## 12. Printing

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 12.1 | **Print with D2D active** | `File > Print` while D2D is the active renderer | Printing uses GDI path (CDC*-based); output is correct |
| 12.2 | **Print preview** | `File > Print Preview` | Preview renders correctly |
| 12.3 | **Print after toggle** | Toggle D2D off → print → toggle D2D on | No state corruption between render paths |

---

## 13. Device Loss and Recovery

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 13.1 | **Rapid resize stress** | Resize window rapidly for 10 seconds | No crash; scene always redraws |
| 13.2 | **Alt-Tab under load** | Open large file, alt-tab rapidly | No crash; scene recovers on focus |
| 13.3 | **Display settings change** | Change resolution or refresh rate while app is running | D2D render target recreated if needed; scene redraws |
| 13.4 | **GPU driver timeout** | Heavy rendering that triggers TDR (rare) | `D2DERR_RECREATE_TARGET` handled; GDI fallback activates |
| 13.5 | **Sleep / wake** | Put PC to sleep and wake | Scene redraws on wake; no blank view |
| 13.6 | **Remote Desktop** | Connect via RDP with D2D active | May fall back to GDI (D2D HWND render targets may not support RDP); verify no crash |

---

## 14. D2D-Specific Visual Quality

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 14.1 | **Antialiasing on lines** | Observe diagonal lines in D2D mode | Smooth antialiased edges (vs GDI aliased) |
| 14.2 | **Antialiasing on arcs** | Observe arc/circle tessellation segments | Smoother joints between segments |
| 14.3 | **Text antialiasing** | Observe TrueType text | ClearType or grayscale antialiased glyphs |
| 14.4 | **Pixel-level accuracy** | Zoom in very close to entity endpoints | Endpoints land on correct coordinates |
| 14.5 | **Subpixel rendering** | Fine geometry at various zoom levels | No shimmer or jitter when panning |
| 14.6 | **Color accuracy** | Compare exact entity color between D2D and GDI | Colors match (COLORREF → D2D_COLOR_F conversion verified) |

---

## 15. Edge Cases and Stress Tests

| # | Test | Steps | Expected |
|---|------|-------|----------|
| 15.1 | **Zero-length lines** | Entity with coincident start/end | No crash; may render as dot or nothing |
| 15.2 | **Degenerate polygon** | Polygon with < 3 vertices | No crash |
| 15.3 | **Very long polyline** | Polyline with hundreds of vertices | Renders completely; acceptable performance |
| 15.4 | **Overlapping entities** | Many entities at same location | All render (draw order correct) |
| 15.5 | **Entities at coordinate extremes** | Large coordinate values (±1e6) | No floating-point overflow in transform pipeline |
| 15.6 | **Single pixel viewport** | Viewport very small | No crash; clipping works |
| 15.7 | **All layers hidden** | Turn off all layers | Empty view; no crash |
| 15.8 | **Empty layer table** | New document with no entities | Clean background; no null-pointer dereference |
| 15.9 | **Unicode text** | Text entities with extended Latin (CP1252 range) | Characters render correctly in both stroke and TrueType fonts |

---

## 16. Performance Observations

These are qualitative — note whether D2D is faster, slower, or equivalent to GDI.

| # | Scenario | Metric |
|---|----------|--------|
| 16.1 | Full scene redraw (large file) | Time from invalidation to completed render |
| 16.2 | Zoom in/out responsiveness | Perceived lag between wheel scroll and scene update |
| 16.3 | Pan responsiveness | Smoothness during middle-button drag |
| 16.4 | Rubberband update rate | Smoothness of rubberband tracking during mouse movement |
| 16.5 | Text-heavy drawing | Frame time with many TEXT/MTEXT entities |
| 16.6 | Width-filled polylines | Frame time with many width-rendered polylines |

---

## DXF Test File Index

| File | Primary Test Coverage |
|------|----------------------|
| `Ellipse_And_EllipticalArc_Test.dxf` | Circles, arcs, full ellipses, elliptical arcs |
| `Ellipse_NegZ_CW_Test.dxf` | Negative-Z extrusion edge cases (6 elliptical + 6 radial) |
| `LWPolyline_Bulge_Test.dxf` | 21 LWPOLYLINE cases: bulge arcs, open/closed, semicircle |
| `Heavy_Polyline_Subtypes.dxf` | 13 heavy POLYLINE cases: 3D, 2D, bulge, width, plinegen |
| `Single Polyline with vertex widths.dxf` | Per-vertex width fill |
| `Single Polyline (68 vertices) with global vertex width (2.5).dxf` | Global width fill, many vertices |
| `Spline_Test.dxf` | Spline rendering |
| `Hatch_Boundary_Test_Mixed.dxf` | Hatch patterns and filled polygons |
| `RoomNumber_Block_Insert.dxf` | Block INSERT with ATTRIB values, non-uniform scale |
| `Floor Plan Sample.dxf` | Complex real-world drawing (stress test) |
| `AEC_Bldg_Plan_Sample-(2010)-1.dxf` | Large real-world architectural drawing |
| `3dFace_Test.dxf` | 3DFACE entities |
| `AcadProxyEntity_Test.dxf` | Proxy entities (may be skipped) |
| `Minimal_DXF_AC1021.dxf` | Minimal valid DXF (empty-ish) |
| `AC1004–AC1027 Empty-1layout.dxf` | Version-specific empty files (import robustness) |

---

## Test Execution Checklist

### First Pass — Core Rendering (Priority 1)
- [ ] §3 Entity Rendering: All 11 primitive types with test files
- [ ] §4 Line Types: Continuous + named patterns
- [ ] §5 Pen Widths: On/off + DXF weight resolution
- [ ] §6 Colors: Direct + ByLayer
- [ ] §7 View Navigation: Zoom + pan
- [ ] §14 D2D Visual Quality: Antialiasing, color accuracy

### Second Pass — Interaction (Priority 2)
- [ ] §8 Rubberband: Line + rectangle modes
- [ ] §9 Selection: Point + rectangle
- [ ] §10 Editing: Add/delete/move/copy
- [ ] §1 Toggle: D2D ↔ GDI switching

### Third Pass — Robustness (Priority 3)
- [ ] §11 File I/O: All formats + round-trip
- [ ] §12 Printing: Print and print preview
- [ ] §13 Device Loss: Resize, sleep, RDP
- [ ] §15 Edge Cases: Degenerate geometry, stress
- [ ] §2 Window: Resize, multi-monitor, DPI

### Fourth Pass — Performance (Priority 4)
- [ ] §16 Performance: Qualitative comparison D2D vs GDI

---

## Appendix A — D2D Features to Explore Next

The following Direct2D / DirectWrite capabilities could enhance AeSys rendering beyond what GDI provided. Listed roughly in order of practical impact.

### A.1 Geometry Realization (`ID2D1DeviceContext1::CreateFilledGeometryRealization`)
- **What**: Pre-tessellates `ID2D1PathGeometry` into a device-dependent mesh. Subsequent `DrawGeometryRealization` is a single GPU draw call.
- **When useful**: Complex polylines, spline tessellations, and hatch patterns that don't change between frames. Cache the realization per entity; invalidate on transform change.
- **Impact**: Major speedup for dense drawings — eliminates per-frame CPU tessellation and path construction.

### A.2 Bitmap Caching / Command Lists (`ID2D1CommandList`)
- **What**: Record a sequence of D2D draw calls into a reusable command list, then replay it without re-executing the command stream.
- **When useful**: Layer-level caching — when a layer's entities haven't changed, replay the cached command list instead of re-issuing individual draw calls. Invalidate only on edit.
- **Impact**: Significant for drawings with many layers where only one layer is being edited.

### A.3 Antialiasing Mode Control
- **What**: `ID2D1RenderTarget::SetAntialiasMode()` — toggle between `D2D1_ANTIALIAS_MODE_PER_PRIMITIVE` (default, high quality) and `D2D1_ANTIALIAS_MODE_ALIASED` (faster, pixel-perfect).
- **When useful**: Grid dots (`SetPixel`) and fine engineering details where subpixel blur is undesirable. Could expose as a user preference (`View > Antialiased Rendering`).
- **Impact**: Low cost, high user-satisfaction — lets users choose crisp vs smooth.

### A.4 Direct2D Effects (`ID2D1Effect`)
- **What**: Built-in GPU-accelerated image effects: Gaussian blur, shadow, color matrix, brightness/contrast, etc.
- **When useful**: Selection highlight glow, layer dimming (replacing gray palette), background image processing.
- **Impact**: Visual polish; not core functionality.

### A.5 D2D Bitmap Render Target (`ID2D1BitmapRenderTarget`)
- **What**: Off-screen D2D bitmap that can be drawn to the HWND render target via `DrawBitmap`.
- **When useful**: Background image display (replace GDI `StretchBlt` with D2D `DrawBitmap`), off-screen layer compositing.
- **Impact**: Eliminates the remaining `StretchBlt` dependency; enables alpha blending of background images.

### A.6 DirectWrite Advanced Typography
- **What**: `IDWriteTextLayout` supports ligatures, OpenType features, per-character formatting (bold/italic/color ranges), inline objects.
- **When useful**: MTEXT rich formatting — `\f` font switches, `\C` color changes, `\b` bold, `\i` italic within a single MTEXT entity.
- **Impact**: More faithful DXF MTEXT rendering. Currently these codes are stripped on import.

### A.7 Dashed Stroke Styles with Custom Patterns
- **What**: `ID2D1StrokeStyle` with `D2D1_DASH_STYLE_CUSTOM` and a float array of dash/gap lengths.
- **When useful**: Replace the software dash-pattern engine (`DisplayDashPattern` in `EoGsVertexBuffer`) with GPU-native dashing. Map `EoDbLineType` dash/gap arrays directly to D2D custom stroke styles.
- **Impact**: Eliminates the CPU-side dash pattern tessellation loop — lines with complex linetypes become single `DrawLine`/`DrawGeometry` calls.

### A.8 Layer Opacity (`ID2D1Layer`)
- **What**: D2D layers with configurable opacity. `PushLayer(opacity=0.3)` → draw → `PopLayer()`.
- **When useful**: Dimming non-active layers during editing (e.g., show active layer at full opacity, others at 30%). More elegant than the GDI gray palette approach.
- **Impact**: UI improvement for layer-focused editing workflows.

### A.9 ID2D1DCRenderTarget for Printing
- **What**: A D2D render target that wraps a `CDC*` / `HDC`. The same drawing code that targets the HWND render target can target a printer DC.
- **When useful**: Unified render path for screen and printer — eliminates the need to maintain the GDI backend solely for printing.
- **Impact**: Long-term: allows deprecation of `EoGsRenderDeviceGdi` entirely (Phase 10 goal).

### A.10 D3D11 Interop
- **What**: `ID2D1Factory1::CreateDevice()` + `ID3D11Device` sharing. D2D surfaces become D3D11 textures.
- **When useful**: Compute shader tessellation for splines and arcs. Vertex-buffer-based rendering of large point clouds or dense polylines.
- **Impact**: Deferred until drawing complexity demands GPU compute. The current CPU tessellation is adequate for typical CAD drawings.

---

## Appendix B — Known Accepted Differences (D2D vs GDI)

| Difference | Cause | Acceptable? |
|------------|-------|-------------|
| Antialiased line edges | D2D per-primitive antialiasing | ✅ Improvement |
| Slightly different text metrics | DirectWrite vs GDI font rendering | ✅ DirectWrite is more accurate |
| No XOR rubberband appearance | D2D overlay vs GDI XOR | ✅ Design decision — gray overlay is cleaner |
| Possible sub-pixel line position shifts | D2D float coordinates vs GDI integer | ✅ Negligible at normal zoom |
| Grid dots may appear softer | D2D antialiased `SetPixel` (1×1 rect) | ⚠️ Consider aliased mode for grid |
