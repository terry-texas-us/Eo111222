# DXF ↔ AeSys Roundtrip Fidelity Gap Summary

> Full audit of the DXF → internal → DXF read/write pipeline.
> Produced by systematic review of `EoDbDxfInterface`, all `Convert*Entity` methods, all `ExportToDxf` overrides, and all `EoDxfWrite::Write*` methods.

---

## Severity Legend

| Severity | Meaning |
|----------|---------|
| 🔴 **CRITICAL** | Entity data silently lost on export — user loses work |
| 🟠 **HIGH** | Geometric or structural fidelity broken — round-tripped DXF visually incorrect |
| 🟡 **MEDIUM** | Property/metadata lost — DXF structurally valid but properties differ |
| 🔵 **LOW** | Informational tables or metadata not preserved — no visual impact |

---

## 1. CRITICAL — Data Loss on Export

### 1.1 `EoDbPolygon` has no `ExportToDxf` override (HATCH entities silently dropped)
- **Read**: `ConvertHatchEntity` fully parses HATCH boundary paths (polyline and edge types), OCS→WCS transforms, solid/pattern/hollow styles → creates `EoDbPolygon`.
- **Write**: `EoDbPolygon` inherits `EoDbPrimitive::ExportToDxf()` which is a **no-op** (line 128 of `EoDbPrimitive.cpp`).
- **Impact**: Every HATCH entity imported from DXF is silently dropped on re-export. Round-trip test will show all hatches missing.
- **Fix**: Implement `EoDbPolygon::ExportToDxf()` that reconstructs `EoDxfHatch` from polygon vertices, hatch origin, style, fill index. Writer infrastructure (`WriteHatch`) already supports full HATCH output including polyline boundaries, edge boundaries (LINE, ARC, ELLIPSE), pattern definition lines, and solid fills.

### 1.2 TEXT rotation angle exported in radians instead of degrees
- **Read**: DXF TEXT `group 50` is in **degrees**; parser converts deg→rad correctly.
- **Write**: `EoDbText::ExportToDxf()` computes rotation via `atan2` (**radians**) and stores in `text.m_textRotation`. `WriteText` (line 387 of `EoDxfWriteAnnotation.cpp`) writes `m_textRotation` **directly** without conversion.
- **Comparison**: `WriteArc` multiplies by `RadiansToDegrees` (line 52). `WriteInsert` multiplies by `RadiansToDegrees` (line 262). `WriteText` does **not**.
- **Comment on line 154 is incorrect**: says "EoDxfWrite converts" but WriteText does not convert.
- **Impact**: All rotated TEXT entities re-exported with wrong angle (e.g., 45° becomes ~0.785 in the DXF, interpreted as <1° by AutoCAD).
- **Fix**: Either multiply by `RadiansToDegrees` in `ExportToDxf` before assigning to `m_textRotation`, or add conversion in `WriteText`. Prefer converting in `ExportToDxf` for consistency with the "export prepares DXF-ready values" pattern used elsewhere. Fix the misleading comment on line 154.

---

## 2. HIGH — Geometric/Structural Fidelity Issues

### 2.1 ARC/CIRCLE center exported as WCS instead of OCS for non-default extrusion
- **Read**: `ConvertArcEntity`/`ConvertCircleEntity` correctly transform center from OCS→WCS via `ExtrudePointInPlace`.
- **Write**: `EoDbConic::ExportToDxf()` writes `m_center` (WCS) directly as DXF group `10/20/30` without WCS→OCS reverse transform.
- **Impact**: For entities with non-default extrusion (e.g., `[0,0,-1]`), the center point is wrong in the output DXF. Default extrusion `[0,0,1]` is transparent (WCS=OCS).
- **Fix**: Implement WCS→OCS reverse transform in `ExportToDxf` using `ComputeArbitraryAxis(extrusion).Inverse()` to transform center back to OCS before writing.

### 2.2 SPLINE degree, knot vector, and weights discarded at import — always re-exported as uniform cubic
- **Read**: `ConvertSplineEntity` copies only control points (or fit points as fallback). Degree, knots, weights, flags, tangents are all discarded.
- **Write**: `EoDbSpline::ExportToDxf()` regenerates a uniform cubic (degree=3) knot vector and writes no weights.
- **Impact**: Non-cubic splines (degree 2, 5, etc.), NURBS with non-uniform knots or rational weights, and splines with tangent constraints all change shape on roundtrip.
- **Fix** (multi-phase):
  1. Add `m_degree` member to `EoDbSpline`; import degree from DXF.
  2. Add `m_knots` (`std::vector<double>`) and `m_weights` (`std::vector<double>`) members.
  3. Store flags (closed/periodic/rational) for export.
  4. Update `ExportToDxf` to use stored degree/knots/weights.
  5. Update `GenPts` to accept stored knots when available.
  6. Update PEG V2 serialization per the V2 spline record design in copilot-instructions.

### 2.3 Heavy POLYLINE always downgraded to LWPOLYLINE on export
- **Read**: Heavy 2D POLYLINE (`ConvertPolyline2DEntity`) and 3D POLYLINE (`ConvertPolyline3DEntity`) are both fully parsed into `EoDbPolyline`.
- **Write**: `EoDbPolyline::ExportToDxf()` always creates `EoDxfLwPolyline` and calls `WriteLWPolyline`.
- **Impact**:
  - **3D polylines**: Per-vertex Z coordinates are lost (LWPOLYLINE stores only a single elevation for all vertices).
  - **2D heavy polylines**: Curve-fit/spline-fit vertex data, polyface mesh structures, and per-vertex type flags are lost.
  - Width data is preserved (LWPOLYLINE supports per-vertex widths).
- **Fix**: Add a flag or dimension indicator to `EoDbPolyline` at import time. If 3D or has heavy-specific properties, export via `WritePolyline` (heavy POLYLINE + VERTEX + SEQEND) which already exists and is fully functional.

### 2.4 ATTRIB/ATTDEF structural link to INSERT lost
- **Read**: `ConvertAttribEntity` creates standalone `EoDbText` primitives with no association to the parent INSERT or original ATTDEF tag.
- **Write**: ATTRIBs export as ordinary TEXT entities, not as ATTRIB entities following their INSERT.
- **Impact**: Attribute identity (tag name, prompt, block association) is lost on roundtrip. Block inserts that expect ATTRIBs will show TEXT instead.
- **Fix**: This is a deep structural change tied to the PEG V2 handle architecture. Requires:
  1. New `EoDbAttribute` primitive type (or metadata on `EoDbText`) preserving tag, prompt, flags, and parent INSERT handle.
  2. Export path that emits ATTRIB entities after their INSERT with proper `SEQEND`.
  3. PEG V2 serialization preserving the tag→insert relationship.

### 2.5 2D POLYLINE OCS coordinates stored as-is without WCS transform
- **Read**: `ConvertPolyline2DEntity` stores OCS x,y coordinates directly (with Z from elevation). No OCS→WCS transform is applied.
- **Impact**: For 2D polylines with non-default extrusion, vertex coordinates are in OCS but treated as WCS by rendering and other operations.
- **Fix**: Apply `ExtrudePointInPlace` to each vertex during conversion, matching the ARC/CIRCLE pattern.

### 2.6 MTEXT multiline rendering produces multiple EoDbText — lost on re-export as individual TEXT entities
- **Read**: MTEXT with `\P` paragraph breaks is split into multiple `EoDbText` primitives per the instructions' MTEXT→multiline convention.
- **Write**: Each `EoDbText` exports individually as DXF TEXT, not reconstituted as a single MTEXT.
- **Impact**: Multiline MTEXT becomes multiple single-line TEXT entities. Formatting, reference rectangle width, and paragraph structure are lost.
- **Fix** (future): Track MTEXT origin in `EoDbText` metadata (or use an `EoDbMText` wrapper) so export can reconstitute `\P`-joined text into a single MTEXT entity.

---

## 3. MEDIUM — Property Loss

### 3.1 Text styles not round-tripped
- **Read**: `ConvertTextStyleTable` logs style name but discards all properties (height, width factor, oblique angle, font file, bigfont, generation flags).
- **Write**: `WriteTextstyles()` is empty `{}`.
- **Impact**: All text style references in TEXT/MTEXT entities reference non-existent styles in the output DXF. AutoCAD falls back to "Standard".
- **Fix**: Implement `EoDbTextStyle` table in `AeSysDoc`; populate from DXF import; export via `WriteTextstyles`.

### 3.2 TEXT oblique angle not exported
- **Read**: `ConvertTextEntity` bakes oblique angle into the reference system Y-direction via shear rotation.
- **Write**: `EoDbText::ExportToDxf()` does not recover or set `text.m_obliqueAngle` (defaults to 0.0). `WriteText` writes group 51 as 0.0.
- **Impact**: Oblique (italic-like) text renders correctly in AeSys (baked into reference system) but exports as non-oblique.
- **Fix**: Recover oblique angle from the reference system's Y-direction shear component during export.

### 3.3 TEXT generation flags not exported
- **Read**: `ConvertTextEntity` reads `m_textGenerationFlags` (group 71) for backward/upside-down text.
- **Write**: `EoDbText::ExportToDxf()` does not set `text.m_textGenerationFlags`.
- **Impact**: Mirrored text loses its mirroring on roundtrip.
- **Fix**: Store generation flags in `EoDbText` and export them.

### 3.4 TEXT extrusion direction not exported
- **Read**: Extrusion is used for OCS→WCS transform during conversion.
- **Write**: `EoDbText::ExportToDxf()` does not set extrusion direction on the `EoDxfText`. Default `[0,0,1]` is assumed.
- **Impact**: Text with non-default extrusion loses its plane orientation.
- **Fix**: Store original extrusion in `EoDbText`; set on DXF entity during export.

### 3.5 LWPOLYLINE elevation not preserved on export
- **Read**: `ConvertLWPolylineEntity` uses elevation (group 38) to set vertex Z coordinates.
- **Write**: `EoDbPolyline::ExportToDxf()` does not recover elevation from vertex Z; writes `m_elevation = 0.0`.
- **Impact**: LWPOLYLINE entities at non-zero elevation lose their Z position on roundtrip.
- **Fix**: Set `lwPolyline.m_elevation` from the Z coordinate of the first vertex (LWPOLYLINE vertices share a common Z).

### 3.6 LWPOLYLINE plinegen flag mapping
- **Read**: DXF flag `0x80` → `sm_Plinegen = 0x0008` (correctly mapped).
- **Write**: Need to verify reverse mapping `0x0008` → DXF flag `0x80` in export. Currently `polylineFlag` is passed through; if internal flags are used directly, plinegen bit will be wrong.
- **Fix**: Ensure flag bit translation in `ExportToDxf`.

### 3.7 Entity thickness not preserved for most entity types
- **Read**: Thickness (group 39) is parsed by `EoDxfGraphic::ParseBaseCode`.
- **Write**: `WriteText`, `WriteCircle`, `WriteArc`, `WriteLine`, `WritePoint` all call `WriteThickness`. But `EoDbLine::ExportToDxf`, `EoDbConic::ExportToDxf`, `EoDbText::ExportToDxf` do not set thickness on the DXF entity.
- **Impact**: Entity thickness lost on roundtrip.
- **Fix**: Store thickness in `EoDbPrimitive` (base class); propagate in `PopulateDxfBaseProperties`.

### 3.8 HATCH pattern definition lines not written
- **Write**: `WriteHatch` (line 170–171) writes `m_numberOfPatternDefinitionLines` but has a comment: "Pattern line data would go here, but it's not implemented in AeSys yet."
- **Impact**: Non-solid hatches lose their pattern definition. AutoCAD will not be able to display the correct pattern.
- **Fix**: Write pattern definition line data (angle, origin, offset, dash lengths) per boundary path.

### 3.9 POINT display mode ($PDMODE/$PDSIZE) exported but limited
- **Read**: `$PDMODE` and `$PDSIZE` are among the 4 header variables imported.
- **Write**: Header variables are exported via the generic visitor. But `EoDbPoint::ExportToDxf` does not set point-specific properties beyond location.
- **Impact**: Point style display is preserved via header variables but PDMODE/PDSIZE per-entity overrides are not supported.

---

## 4. LOW — Metadata/Table Gaps

### 4.1 Header variables: only 4 of ~280 imported
- **Read**: `ConvertHeaderSection` imports only `$ACADVER`, `$CLAYER`, `$PDMODE`, `$PDSIZE`.
- **Write**: Only variables stored in `EoDbHeaderSection` are exported.
- **Missing important variables**: `$HANDSEED`, `$INSBASE`, `$EXTMIN`, `$EXTMAX`, `$LIMMIN`, `$LIMMAX`, `$LTSCALE`, `$DIMSCALE`, `$TEXTSIZE`, `$TEXTSTYLE`, `$CELTYPE`, `$CECOLOR`, `$MEASUREMENT`, `$LUNITS`, `$LUPREC`, `$AUNITS`, `$AUPREC`, etc.
- **Fix**: Add a passthrough map for all header variables, storing them as-is (variant type already supports this). Apply `$HANDSEED` for handle management. Selectively interpret variables that affect rendering (e.g., `$LTSCALE`).

### 4.2 Dimension styles not round-tripped
- **Read**: `ConvertDimStyle` logs style name, discards all properties.
- **Write**: `WriteDimstyles()` is empty `{}`.
- **Impact**: DIMENSION entities reference non-existent dim styles.
- **Fix**: Implement passthrough storage for dim style tables.

### 4.3 CLASSES section not round-tripped
- **Read**: `ConvertClassesSection` logs class name only.
- **Write**: `WriteClasses` writes empty CLASSES section.
- **Impact**: Custom entity classes (proxy entities, ACAD_TABLE, etc.) lose their class definitions. Does not affect standard entity types.
- **Fix**: Implement passthrough storage for class definitions.

### 4.4 OBJECTS section not round-tripped
- **Read**: Unsupported objects are stored as raw group code data.
- **Write**: `WriteObjects()` is empty `{}`. `WriteUnsupportedObjects()` is empty `{}`.
- **Impact**: DICTIONARY, LAYOUT, PLOTSETTINGS, MLINESTYLE, MATERIAL, VISUALSTYLE, etc. all lost. LAYOUT objects in particular break AutoCAD's paper-space/model-space tab structure.
- **Fix**: Implement passthrough for DICTIONARY and LAYOUT objects at minimum.

### 4.5 AppId table not round-tripped
- **Read**: `ConvertAppIdTable` logs name only.
- **Write**: No AppId write path.
- **Impact**: Application-specific extended data referencing registered application IDs will lose their AppId entries.

### 4.6 Block record table incomplete
- **Read**: Block records are implicitly created via BLOCK entities.
- **Write**: `WriteBlockRecord` exists but only writes `*Model_Space` and `*Paper_Space` plus user blocks. Missing: `$ACADVER`-dependent properties, block record handles for proper handle graph.

### 4.7 Entity types read but not exported (read-only entities)
These entity types are parsed from DXF but have no internal representation or export path:

| Entity | Read Status | Export Status | Notes |
|--------|-------------|---------------|-------|
| SOLID | Skipped (logged) | N/A | No internal `EoDbSolid` type |
| TRACE | Skipped (logged) | N/A | No internal type |
| RAY | Skipped (logged) | N/A | No internal type |
| XLINE | Skipped (logged) | N/A | No internal type |
| IMAGE | Skipped (logged) | N/A | No internal type |
| LEADER | Parsed → not stored | Writer exists (`WriteLeader`) | No internal `EoDbLeader` type |
| MLEADER | Parsed → not stored | Writer exists (`WriteMLeader`) | No internal `EoDbMLeader` type |
| DIMENSION (7 types) | Parsed → not stored | Writer exists (`WriteDimension`) | No internal `EoDbDimension` type |
| Polyface mesh | Skipped (logged) | N/A | Not mappable to `EoDbPolyline` |
| Polygon mesh | Skipped (logged) | N/A | Not mappable to `EoDbPolyline` |
| ATTDEF | Parsed + counted | No export | Template only — rendering would overlap ATTRIBs |

### 4.8 Unsupported object passthrough
- **Read**: `EoDxfUnsupportedObject` stores raw group code data for unknown OBJECTS section entries.
- **Write**: `WriteUnsupportedObjects()` is empty `{}` in the interface override, though `EoDxfWrite::WriteUnsupportedObject()` exists.
- **Fix**: Wire the interface's `WriteUnsupportedObjects` to iterate stored unsupported objects and call `m_dxfWriter->WriteUnsupportedObject()`.

---

## 5. Entity-by-Entity Roundtrip Summary

| Entity | Read | Internal | Export | Fidelity | Key Gaps |
|--------|------|----------|--------|----------|----------|
| LINE | ✅ Full | `EoDbLine` | ✅ Full | 🟢 Good | Thickness not propagated |
| ARC | ✅ OCS→WCS | `EoDbConic` | ⚠️ WCS center | 🟠 High gap | WCS→OCS reverse needed §2.1 |
| CIRCLE | ✅ OCS→WCS | `EoDbConic` | ⚠️ WCS center | 🟠 High gap | WCS→OCS reverse needed §2.1 |
| ELLIPSE | ✅ WCS | `EoDbConic` | ✅ WCS direct | 🟢 Good | Extrusion passthrough OK |
| TEXT | ✅ Full | `EoDbText` | ⚠️ Angle bug | 🔴 Critical | Rotation in radians §1.2; oblique §3.2; flags §3.3 |
| MTEXT | ✅ Split to multiline | `EoDbText[]` | ⚠️ Individual TEXT | 🟠 High gap | Reconstitution needed §2.6 |
| ATTRIB | ✅ → `EoDbText` | `EoDbText` | ⚠️ As TEXT | 🟠 High gap | Structural link lost §2.4 |
| ATTDEF | ✅ Parsed | Not stored | ❌ None | 🔵 By design | Template; intentionally not rendered |
| INSERT | ✅ Full | `EoDbBlockReference` | ✅ Full | 🟢 Good | Rotation angle converted correctly |
| LWPOLYLINE | ✅ Full | `EoDbPolyline` | ⚠️ No elevation | 🟡 Medium | Elevation §3.5; plinegen §3.6 |
| POLYLINE 2D | ✅ Full | `EoDbPolyline` | ⚠️ → LWPOLYLINE | 🟠 High gap | Downgrade §2.3; OCS §2.5 |
| POLYLINE 3D | ✅ Full | `EoDbPolyline` | ⚠️ → LWPOLYLINE | 🟠 High gap | Z lost §2.3 |
| SPLINE | ⚠️ Lossy | `EoDbSpline` | ⚠️ Uniform cubic | 🟠 High gap | Degree/knots/weights §2.2 |
| HATCH | ✅ Full | `EoDbPolygon` | ❌ **Dropped** | 🔴 **Critical** | No ExportToDxf §1.1 |
| VIEWPORT | ✅ Full | `EoDbViewport` | ✅ Full | 🟢 Good | All view params preserved |
| POINT | ✅ Full | `EoDbPoint` | ✅ Full | 🟢 Good | |
| 3DFACE | ✅ Full | `EoDbPrimitive` (direct) | ❓ Needs verification | 🟡 Unknown | |
| BLOCK | ✅ Full | `EoDbBlock` | ✅ Full | 🟢 Good | Geometry + base point + flags |
| ACAD_PROXY | ✅ Full passthrough | Stored raw | ✅ Full passthrough | 🟢 Good | Binary graphics + entity data preserved |

---

## 6. Prioritized Action Items

### Phase 1 — Fix Critical Data Loss (immediate)

| # | Item | Effort | Files |
|---|------|--------|-------|
| 1 | Fix TEXT rotation: multiply by `RadiansToDegrees` in `ExportToDxf` | S | `EoDbText.cpp` |
| 2 | Implement `EoDbPolygon::ExportToDxf` for HATCH export | L | `EoDbPolygon.h`, `EoDbPolygon.cpp` |

### Phase 2 — Fix High-Priority Geometric Issues

| # | Item | Effort | Files |
|---|------|--------|-------|
| 3 | WCS→OCS reverse transform for ARC/CIRCLE export | M | `EoDbConic.h`, `EoGeTransformMatrix` |
| 4 | 2D POLYLINE OCS→WCS transform at import | M | `EoDbDxfInterface.cpp` |
| 5 | Preserve LWPOLYLINE elevation on export | S | `EoDbPolyline.cpp` |
| 6 | Heavy POLYLINE export (3D via `WritePolyline`) | M | `EoDbPolyline.h`, `EoDbPolyline.cpp` |
| 7 | Store spline degree at import; use in export | M | `EoDbSpline.h`, `EoDbSpline.cpp`, `EoDbDxfInterface.cpp` |

### Phase 3 — Property Preservation

| # | Item | Effort | Files |
|---|------|--------|-------|
| 8 | TEXT oblique angle recovery and export | M | `EoDbText.cpp` |
| 9 | TEXT generation flags storage and export | S | `EoDbText.h`, `EoDbText.cpp` |
| 10 | TEXT extrusion direction passthrough | S | `EoDbText.h`, `EoDbText.cpp` |
| 11 | Entity thickness passthrough via base class | M | `EoDbPrimitive.h`, all derived types |
| 12 | HATCH pattern definition line export | M | `EoDxfDrawing.cpp` |
| 13 | LWPOLYLINE plinegen flag bit translation on export | S | `EoDbPolyline.cpp` |

### Phase 4 — Table/Section Round-Trip

| # | Item | Effort | Files |
|---|------|--------|-------|
| 14 | Text style table passthrough | M | New `EoDbTextStyle`, interface, `AeSysDoc` |
| 15 | Header variable passthrough map | M | `EoDbDxfInterface.cpp`, `EoDbHeaderSection` |
| 16 | Dim style table passthrough | M | Interface, `AeSysDoc` |
| 17 | Unsupported objects wiring | S | `EoDbDxfInterface.h` |

### Phase 5 — Structural/Architectural (V2 dependent)

| # | Item | Effort | Files |
|---|------|--------|-------|
| 18 | Full spline knot/weight storage (V2 PEG) | L | `EoDbSpline.h/.cpp`, PEG serialization |
| 19 | ATTRIB→INSERT structural link (V2 handle architecture) | XL | New primitive type, PEG V2 |
| 20 | MTEXT reconstitution from multiline EoDbText | L | `EoDbText`, `EoDbDxfInterface` |
| 21 | CLASSES section passthrough | M | Interface, new storage |
| 22 | OBJECTS/DICTIONARY/LAYOUT passthrough | XL | Interface, `AeSysDoc` |

### Effort Key
- **S** = Small (< 50 LOC, single file)
- **M** = Medium (50–200 LOC, 2–4 files)
- **L** = Large (200–500 LOC, multiple files)
- **XL** = Extra Large (500+ LOC, architectural change)

---

## 7. What Already Works Well

- **LINE**: Near-perfect roundtrip (only missing thickness passthrough).
- **ELLIPSE**: Full roundtrip — WCS coordinates pass through correctly, `CorrectAxis()` validates axis/ratio.
- **INSERT**: Full roundtrip — scale, rotation (correctly converted rad→deg), array properties, extrusion all preserved.
- **VIEWPORT**: Full roundtrip — all 17 view parameters preserved in both PEG and DXF.
- **POINT**: Full roundtrip.
- **BLOCK definitions**: Full geometry + base point + flags roundtrip.
- **Layer table**: Full roundtrip — name, color, frozen, locked, linetype, off state.
- **Linetype table**: Full roundtrip — name, pattern length, dash elements.
- **VPORT table**: Full roundtrip (newly implemented).
- **ACAD_PROXY_ENTITY**: Full binary passthrough — graphics data, entity data, handles all preserved.
- **Paper-space**: Dual-space architecture with `m_currentExportSpace` routing entities to correct space on export.
- **Handle architecture**: Entity handles and owner handles propagated through the full pipeline.
- **Base entity properties**: Layer, color (ACI + true color + color name), linetype, line weight all preserved via `WriteEntity`.
- **Application data + extended data**: `WriteAppData` and `WriteExtData` preserve `102` groups and `1001+` xdata blocks.
- **Reactor handles + extension dictionary handles**: Written by `WriteEntity` when present.
- **Binary DXF**: Full support for both ASCII and binary DXF read/write.
- **Code page handling**: CP1252/UTF-8 codec with UTF-16 detection and fallback.
