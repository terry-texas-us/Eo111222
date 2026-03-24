# DXF ↔ AeSys Roundtrip Fidelity Gap Summary

> Full audit of the DXF → internal → DXF read/write pipeline.
> Produced by systematic review of `EoDbDxfInterface`, all `Convert*Entity` methods, all `ExportToDxf` overrides, and all `EoDxfWrite::Write*` methods.
> Updated: reflects all fixes applied through current codebase state.

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

### 1.1 ~~`EoDbPolygon` has no `ExportToDxf` override (HATCH entities silently dropped)~~ ✅ RESOLVED
- **Fix applied**: `EoDbPolygon::ExportToDxf()` implemented in `EoDbPolygon.cpp`. Reconstructs `EoDxfHatch` from polygon vertices, hatch origin, style, fill index, pattern definition lines, and double flag. Full pipeline: `ExportToDxf` → `AddHatch` → `WriteHatch`.

### 1.2 ~~TEXT rotation angle exported in radians instead of degrees~~ ✅ RESOLVED
- **Fix applied**: `EoDbText::ExportToDxf()` uses `Eo::RadianToDegree(std::atan2(xDir.y, xDir.x))` to convert to degrees before assigning to `text.m_textRotation`. `WriteText` writes the value directly — no double conversion.

**No remaining CRITICAL issues.**

---

## 2. HIGH — Geometric/Structural Fidelity Issues

### 2.1 ~~ARC/CIRCLE center exported as WCS instead of OCS for non-default extrusion~~ ✅ RESOLVED
- **Fix applied**: `EoDbConic::ExportToDxf()` transforms center WCS→OCS via `EoGeOcsTransform::CreateWcsToOcs(m_extrusion) * m_center` before writing Circle and RadialArc entities. Ellipse export correctly writes WCS directly (per DXF spec). ARC start/end angles remain in radians internally; `EoDxfWrite::WriteArc` converts to degrees via `* EoDxf::RadiansToDegrees`.

### 2.2 SPLINE degree, knot vector, and weights discarded at import — always re-exported as uniform cubic
- **Read**: `ConvertSplineEntity` copies only control points (or fit points as fallback). Degree, knots, weights, flags, and tangents are all discarded. OCS→WCS transform is applied to control points.
- **Write**: `EoDbSpline::ExportToDxf()` hard-codes `degree = 3`, generates a uniform knot vector, sets flag `0x08` (planar), writes no weights. The `EoDxfWrite::WriteSpline` method itself fully supports degree/knots/weights/fit points — the limitation is in the `EoDbSpline` data model.
- **Impact**: Non-cubic splines (degree 2, 5, etc.), NURBS with non-uniform knots or rational weights, and splines with tangent constraints all change shape on roundtrip.
- **Fix** (multi-phase):
  1. Add `m_degree` member to `EoDbSpline`; import degree from DXF.
  2. Add `m_knots` (`std::vector<double>`) and `m_weights` (`std::vector<double>`) members.
  3. Store flags (closed/periodic/rational) for export.
  4. Update `ExportToDxf` to use stored degree/knots/weights.
  5. Update `GenPts` to accept stored knots when available.
  6. Update PEG V2 serialization per the V2 spline record design in copilot-instructions.

### 2.3 ~~Heavy POLYLINE always downgraded to LWPOLYLINE on export~~ ✅ PARTIALLY RESOLVED
- **3D polylines** ✅: `ConvertPolyline3DEntity` sets `Is3D(true)`. `EoDbPolyline::ExportToDxf()` dispatches on `Is3D()` — when true, creates `EoDxfPolyline` with flag `0x08` (3D polyline), emits vertices with flag `0x20` (3dPolylineVertex), and writes via `WritePolyline` (heavy POLYLINE + VERTEX + SEQEND). Per-vertex Z coordinates are preserved.
- **2D heavy polylines** ⚠️ **STILL OPEN**: `ConvertPolyline2DEntity` does **not** set `Is3D(true)` (correct, since these are 2D). However, `ExportToDxf` routes all non-3D polylines to `WriteLWPolyline`. 2D heavy polylines with curve-fit/spline-fit vertex data lose that structure on export. Width and bulge data are preserved since LWPOLYLINE supports them.
- **Impact**: Curve-fit/spline-fit vertex semantics from heavy 2D POLYLINE are lost. Basic 2D heavy polylines (no curve/spline fit) round-trip correctly as LWPOLYLINE.
- **Fix**: Track curve-fit/spline-fit flag at import; export those as heavy POLYLINE via `WritePolyline`.

### 2.4 ATTRIB/ATTDEF structural link to INSERT lost
- **Read**: `ConvertAttribEntity` creates standalone `EoDbText` primitives with no association to the parent INSERT or original ATTDEF tag. Full text properties are preserved (alignment, oblique, generation flags, extrusion), and invisible ATTRIBs (flag `0x01`) are skipped.
- **Write**: ATTRIBs export as ordinary TEXT entities, not as ATTRIB entities following their INSERT.
- **Impact**: Attribute identity (tag name, prompt, block association) is lost on roundtrip. Block inserts that expect ATTRIBs will show TEXT instead.
- **Fix**: This is a deep structural change tied to the PEG V2 handle architecture. Requires:
  1. New `EoDbAttribute` primitive type (or metadata on `EoDbText`) preserving tag, prompt, flags, and parent INSERT handle.
  2. Export path that emits ATTRIB entities after their INSERT with proper `SEQEND`.
  3. PEG V2 serialization preserving the tag→insert relationship.

### 2.5 ~~2D POLYLINE OCS coordinates stored as-is without WCS transform~~ ✅ RESOLVED
- **Fix applied**: `ConvertPolyline2DEntity` now applies `EoGeOcsTransform::CreateOcsToWcs(extrusion)` to transform each vertex from OCS to WCS, matching the ARC/CIRCLE import pattern.

### 2.6 MTEXT multiline rendering produces multiple EoDbText — lost on re-export as individual TEXT entities
- **Read**: MTEXT with `\P` paragraph breaks is split into multiple `EoDbText` primitives per the MTEXT→multiline convention. Each line shares font definition and alignment but has its own reference system origin offset by `lineSpacingFactor × (5/3) × height` downward per line. Formatting codes `\P`, `\A`, `\S` are preserved; unsupported codes (`\f`, `\C`, `\H`, `\W`, `\T`, `\Q`, `\L`/`\l`, `\O`/`\o`) are stripped.
- **Write**: Each `EoDbText` exports individually as DXF TEXT, not reconstituted as a single MTEXT.
- **Impact**: Multiline MTEXT becomes multiple single-line TEXT entities. Reference rectangle width, paragraph structure, and MTEXT-specific properties (attachment point, line spacing style/factor) are lost.
- **Fix** (future): Track MTEXT origin in `EoDbText` metadata (or use an `EoDbMText` wrapper) so export can reconstitute `\P`-joined text into a single MTEXT entity.

### 2.7 3DFACE converts to closed `EoDbPolyline` — exports as LWPOLYLINE (NEW)
- **Read**: `Convert3dFaceEntity` creates a closed `EoDbPolyline` from the 3DFACE's 3 or 4 vertices. Invisible edge flags (group 70 bitmask) are logged but **not preserved** in the polyline.
- **Write**: The resulting polyline exports via `WriteLWPolyline` (since `Is3D()` is not set on a 3DFACE-sourced polyline).
- **Impact**: Entity type changes from 3DFACE to LWPOLYLINE on roundtrip. Per-vertex Z differences are flattened to a single elevation. Invisible edge flags are lost.
- **Fix**: Either set `Is3D(true)` on 3DFACE-sourced polylines so they export as heavy POLYLINE (preserving per-vertex Z), or create a dedicated `EoDb3dFace` primitive that exports via a `Write3dFace` path.

### 2.8 HATCH multi-loop topology lost at import (NEW)
- **Read**: `ConvertHatchEntity` processes each boundary loop. The first loop creates the `EoDbPolygon`. Subsequent loops (islands) are created as **separate** `EoDbPolygon` primitives with `kHollowInterior` fill style.
- **Write**: Each `EoDbPolygon` exports as a separate single-loop HATCH entity.
- **Impact**: A single DXF HATCH with outer boundary + inner islands becomes N separate HATCH entities, each with one boundary loop. Island detection and hatch pattern clipping are lost. Visual fidelity is preserved for the outer boundary but inner islands show as independent hollow hatches rather than holes.
- **Fix** (future): Store all boundary loops in a single `EoDbPolygon` (multi-loop support) or introduce a loop collection member. Export path would reconstruct the multi-loop boundary structure.

---

## 3. MEDIUM — Property Loss

### 3.1 ~~Text styles not round-tripped~~ ✅ RESOLVED
- **Fix applied**: `ConvertTextStyleTable` stores all properties in `EoDbTextStyle` entries via `AeSysDoc::AddTextStyleEntry()`. `WriteTextstyles()` iterates the table and writes each style via `EoDxfWrite::WriteTextstyle()`. Oblique angle is converted deg→rad on read and rad→deg on write.

### 3.2 ~~TEXT oblique angle not exported~~ ✅ RESOLVED
- **Fix applied**: `EoDbText::ExportToDxf()` recovers oblique angle from Y-direction shear via `DotProduct(xUnit, yUnit)` → `asin` → degrees.

### 3.3 ~~TEXT generation flags not exported~~ ✅ RESOLVED
- **Fix applied**: `EoDbText::ExportToDxf()` sets `text.m_textGenerationFlags = m_textGenerationFlags`.

### 3.4 ~~TEXT extrusion direction not exported~~ ✅ RESOLVED
- **Fix applied**: `EoDbText::ExportToDxf()` sets `text.m_extrusionDirection = {m_extrusion.x, m_extrusion.y, m_extrusion.z}`.

### 3.5 ~~LWPOLYLINE elevation not preserved on export~~ ✅ RESOLVED
- **Fix applied**: `EoDbPolyline::ExportToDxf()` sets `lwPolyline.m_elevation = m_pts[0].z`.

### 3.6 ~~LWPOLYLINE plinegen flag mapping~~ ✅ RESOLVED
- **Fix applied**: `EoDbPolyline::ExportToDxf()` maps `HasPlinegen()` → DXF flag `0x80` via explicit `if` check.

### 3.7 ~~Entity thickness not preserved for most entity types~~ ✅ RESOLVED
- **Fix applied**: `m_thickness` stored in `EoDbPrimitive` (base class). `SetBaseProperties` reads thickness from parsed DXF entity. `PopulateDxfBaseProperties` propagates `m_thickness` to the DXF entity struct. All derived types inherit automatically — `WriteArc`, `WriteCircle`, `WriteLine`, `WriteText`, `WritePoint` all call `WriteThickness` on the populated value.

### 3.8 ~~HATCH pattern definition lines not written~~ ✅ RESOLVED
- **Fix applied**: `WriteHatch` writes full pattern definition line data (angle, base point, offset, dash lengths) from `EoDxfHatch::m_patternDefinitionLines`. `EoDbPolygon::ExportToDxf` passes through stored pattern definition lines.

### 3.9 POINT display mode ($PDMODE/$PDSIZE) exported but limited
- **Read**: `$PDMODE` and `$PDSIZE` are imported as part of the full header variable passthrough.
- **Write**: Header variables are exported via the generic `std::visit` visitor. `EoDbPoint::ExportToDxf` sets location via `PopulateDxfBaseProperties` + point coordinates.
- **Impact**: Point style display is preserved via header variables but PDMODE/PDSIZE per-entity overrides are not supported (the DXF spec does not define per-entity PDMODE — this is a minor display limitation, not a roundtrip gap).

### 3.10 TEXT alignment lossy mapping — AeSys 3×3 vs DXF 6×4 (NEW)
- **Read**: `ConvertTextEntity` and `ConvertAttribEntity` map DXF's 6 horizontal × 4 vertical alignment codes to AeSys's 3×3 grid: Aligned→Left, Middle→Center, Fit→Right (horizontal); Baseline→Bottom, Bottom→Bottom (vertical). Aligned/Fit stretching behavior is not implemented — only direction is preserved.
- **Write**: `EoDbText::ExportToDxf()` maps back from the 3×3 grid to DXF codes. The Aligned/Fit/Middle distinction is lost.
- **Impact**: Text using DXF Aligned, Fit, or Middle horizontal alignment will have different alignment semantics after roundtrip. Visual position may shift depending on text string length.
- **Note**: This is partially by design — AeSys does not implement Aligned/Fit text stretching.

### 3.11 Layer properties partially lost on export (NEW)
- **Read**: `ConvertLayerTable` reads all layer properties: name, color, frozen (→ `SetStateOff`), locked (logged only), linetype, lineweight (logged only). Layers are created in the correct space (model/paper) using `AddLayerToSpace`.
- **Write**: `WriteLayers` exports: name, color, off state, linetype name, plotting flag. **Not exported**: frozen flag, locked flag, lineweight, layer handle.
- **Impact**: Frozen layers become off (state is preserved but semantics differ — frozen layers exclude from regeneration). Locked layers lose their locked state. Lineweight is lost. Handle graph for layers is not maintained.
- **Fix**: Map off/frozen states separately in `WriteLayers`. Add locked flag and lineweight output.

### 3.12 3DFACE invisible edge flags not preserved (NEW)
- **Read**: `Convert3dFaceEntity` logs invisible edge flags (group 70) but does not store them in the resulting `EoDbPolyline`.
- **Write**: Exported as LWPOLYLINE — no invisible edge concept exists.
- **Impact**: 3DFACE entities with selectively invisible edges lose that information.
- **Note**: Related to §2.7 — a dedicated 3DFACE export path would restore this.

---

## 4. LOW — Metadata/Table Gaps

### 4.1 ~~Header variables: only 4 of ~280 imported~~ ✅ RESOLVED
- **Fix applied**: `ConvertHeaderSection` imports ALL header variables as-is via `SetHeaderSectionVariable()` using a variant type (`std::wstring`, `int16_t`, `int32_t`, `bool`, `int64_t`, `std::uint64_t`, `double`, `EoGePoint3d`, `EoGeVector3d`). `$HANDSEED` is applied to the document handle manager. `WriteHeader` exports all stored variables via `std::visit` dispatch on the variant type.

### 4.2 ~~Dimension styles not round-tripped~~ ✅ RESOLVED
- **Fix applied**: `ConvertDimStyle` stores ~80 properties in `EoDbDimStyle` entries via `AeSysDoc::AddDimStyleEntry()`. `WriteDimstyles()` iterates the table and writes each style via `EoDxfWrite::WriteDimstyle()`.

### 4.3 ~~CLASSES section not round-tripped~~ ✅ RESOLVED
- **Fix applied**: `ConvertClassesSection` stores all class properties (record type, class name, C++ class name, app name, proxy capabilities flags, instance count, was-a-zombie flag, is-an-entity flag) in `EoDbClassEntry` via `AeSysDoc::AddClassEntry()`. `WriteClasses()` iterates the table and writes each class.

### 4.4 ~~OBJECTS section not round-tripped~~ ✅ RESOLVED
- **Fix applied**: `WriteObjects()` now checks `m_interface->HasUnsupportedObjects()`. When imported OBJECTS section data exists (from DXF round-trip), it writes those objects directly via `WriteUnsupportedObjects()` and returns — the imported data already contains the root dictionary, ACAD_GROUP, and all other non-graphical objects. When no imported objects exist (new drawing), the original hardcoded minimal dictionary path (root dict C, ACAD_GROUP D, image definitions) is used.
- **Key change**: The `HasUnsupportedObjects()` virtual (default `false`) on `EoDxfInterface` with override in `EoDbDxfInterface` provides the conditional gate. This eliminates the duplicate-dictionary bug where both hardcoded and imported dictionaries were written.

### 4.5 ~~AppId table not round-tripped~~ ✅ RESOLVED
- **Fix applied**: `ConvertAppIdTable` stores application ID name, flags, handle, and owner handle in `EoDbAppIdEntry` via `AeSysDoc::AddAppIdEntry()`. `WriteAppId()` iterates the table and writes each entry.

### 4.6 Block record table incomplete
- **Read**: Block records are implicitly created via BLOCK entities.
- **Write**: `WriteBlockRecord` writes `*Model_Space`, `*Paper_Space`, and user block names. Missing: `$ACADVER`-dependent properties, block record handles for proper handle graph.
- **Impact**: Block record handles are not preserved — this can break handle-based cross-references in DXF files read by strict parsers.

### 4.7 Entity types read but not exported (read-only entities)
These entity types are parsed from DXF but have no internal representation or export path:

| Entity | Read Status | Export Status | Notes |
|--------|-------------|---------------|-------|
| SOLID | Skipped (logged) | N/A | No internal `EoDbSolid` type |
| TRACE | Skipped (logged) | N/A | No internal type |
| RAY | Skipped (logged) | N/A | No internal type; `AddRay` is no-op |
| XLINE | Skipped (logged) | N/A | No internal type; `AddXline` is no-op |
| IMAGE | Skipped (logged) | N/A | No internal type; `AddImage` is no-op |
| LEADER | Parsed → not stored | Writer exists (`WriteLeader`) | No internal `EoDbLeader` type; `AddLeader` is no-op |
| MLEADER | Parsed → not stored | Writer exists (`WriteMLeader`) | No internal `EoDbMLeader` type; `AddMLeader` is no-op |
| DIMENSION (7 types) | Parsed → not stored | Writer exists (`WriteDimension`) | No internal `EoDbDimension` type; `AddDimAlign` etc. decrement counter only |
| Polyface mesh | Skipped (logged) | N/A | `AddPolyline` flag `0x40` → skip + log |
| Polygon mesh | Skipped (logged) | N/A | `AddPolyline` flag `0x10` → skip + log |
| ATTDEF | Parsed + counted | No export | Template only — rendering would overlap ATTRIBs |

### 4.8 ~~Unsupported object passthrough not wired~~ ✅ RESOLVED
- **Fix applied**: `WriteUnsupportedObjects()` iterates stored `EoDxfUnsupportedObject` instances and calls `m_dxfWriter->WriteUnsupportedObject()` for each.
- **Note**: Now reachable — `WriteObjects()` calls `WriteUnsupportedObjects()` when `HasUnsupportedObjects()` returns true (see §4.4).

---

## 5. Entity-by-Entity Roundtrip Summary

| Entity | Read | Internal | Export | Fidelity | Key Gaps |
|--------|------|----------|--------|----------|----------|
| LINE | ✅ Full | `EoDbLine` | ✅ Full | 🟢 **Good** | — |
| ARC | ✅ OCS→WCS | `EoDbConic` | ✅ WCS→OCS | 🟢 **Good** | Angles: internal rad → WriteArc converts to deg ✅ |
| CIRCLE | ✅ OCS→WCS | `EoDbConic` | ✅ WCS→OCS | 🟢 **Good** | — |
| ELLIPSE | ✅ WCS | `EoDbConic` | ✅ WCS direct | 🟢 **Good** | `CorrectAxis()` validates axis/ratio |
| TEXT | ✅ Full | `EoDbText` | ✅ Full | 🟢 **Good** | Rotation ✅ oblique ✅ gen flags ✅ extrusion ✅ alignment (lossy 3×3) |
| MTEXT | ✅ Split to multiline | `EoDbText[]` | ⚠️ Individual TEXT | 🟠 High gap | Reconstitution needed §2.6 |
| ATTRIB | ✅ → `EoDbText` | `EoDbText` | ⚠️ As TEXT | 🟠 High gap | Structural link lost §2.4 |
| ATTDEF | ✅ Parsed | Not stored | ❌ None | 🔵 By design | Template; intentionally not rendered |
| INSERT | ✅ Full | `EoDbBlockReference` | ✅ Full | 🟢 **Good** | Rotation rad→deg ✅, scale, array, extrusion all preserved |
| LWPOLYLINE | ✅ Full | `EoDbPolyline` | ✅ Full | 🟢 **Good** | Elevation ✅ plinegen ✅ width ✅ bulge ✅ |
| POLYLINE 2D | ✅ OCS→WCS | `EoDbPolyline` | ⚠️ → LWPOLYLINE | 🟡 Medium | Curve-fit/spline-fit data lost §2.3; basic 2D OK |
| POLYLINE 3D | ✅ Full | `EoDbPolyline` | ✅ Heavy POLYLINE | 🟢 **Good** | `Is3D()` dispatch → `WritePolyline` ✅ |
| SPLINE | ⚠️ Lossy | `EoDbSpline` | ⚠️ Uniform cubic | 🟠 High gap | Degree/knots/weights discarded §2.2 |
| HATCH | ✅ Full | `EoDbPolygon` | ✅ Full (single loop) | 🟡 Medium | Multi-loop topology lost §2.8; single-loop OK |
| VIEWPORT | ✅ Full | `EoDbViewport` | ✅ Full | 🟢 **Good** | All 17 view params + forces PaperSpace |
| POINT | ✅ Full | `EoDbPoint` | ✅ Full | 🟢 **Good** | — |
| 3DFACE | ✅ → closed polyline | `EoDbPolyline` | ⚠️ → LWPOLYLINE | 🟠 High gap | Entity type changes §2.7; invisible edges lost §3.12 |
| BLOCK | ✅ Full | `EoDbBlock` | ✅ Full | 🟢 **Good** | Geometry + base point + flags |
| ACAD_PROXY | ✅ Full passthrough | Stored raw | ✅ Full passthrough | 🟢 **Good** | Binary AcGi graphics + entity data + handles preserved |

---

## 6. Prioritized Action Items

### ~~Phase 1 — Fix Critical Data Loss~~ ✅ ALL RESOLVED

| # | Item | Status |
|---|------|--------|
| ~~1~~ | ~~Fix TEXT rotation: multiply by `RadiansToDegrees` in `ExportToDxf`~~ | ✅ Resolved |
| ~~2~~ | ~~Implement `EoDbPolygon::ExportToDxf` for HATCH export~~ | ✅ Resolved |

### Phase 2 — Fix High-Priority Geometric/Structural Issues

| # | Item | Effort | Files | Status |
|---|------|--------|-------|--------|
| ~~3~~ | ~~WCS→OCS reverse transform for ARC/CIRCLE export~~ | ~~M~~ | | ✅ Resolved |
| ~~4~~ | ~~2D POLYLINE OCS→WCS transform at import~~ | ~~M~~ | | ✅ Resolved |
| ~~5~~ | ~~Preserve LWPOLYLINE elevation on export~~ | ~~S~~ | | ✅ Resolved |
| ~~6~~ | ~~Heavy POLYLINE export (3D via `WritePolyline`)~~ | ~~M~~ | | ✅ Resolved |
| 7 | Store spline degree at import; use in export | M | `EoDbSpline.h`, `EoDbSpline.cpp`, `EoDbDxfInterface.cpp` | Open |
| 8 | 3DFACE → preserve entity type or set `Is3D()` for Z preservation | M | `EoDbDxfInterface.cpp`, `EoDbPolyline.cpp` | Open (NEW) |
| 9 | HATCH multi-loop: store all boundary loops in single `EoDbPolygon` | L | `EoDbPolygon.h`, `EoDbPolygon.cpp`, `EoDbDxfInterface.cpp` | Open (NEW) |
| 10 | 2D heavy POLYLINE: track curve-fit/spline-fit for heavy export | M | `EoDbPolyline.h`, `EoDbPolyline.cpp`, `EoDbDxfInterface.cpp` | Open |

### Phase 3 — Property Preservation

| # | Item | Effort | Files | Status |
|---|------|--------|-------|--------|
| ~~11~~ | ~~TEXT oblique angle recovery and export~~ | ~~M~~ | | ✅ Resolved |
| ~~12~~ | ~~TEXT generation flags storage and export~~ | ~~S~~ | | ✅ Resolved |
| ~~13~~ | ~~TEXT extrusion direction passthrough~~ | ~~S~~ | | ✅ Resolved |
| ~~14~~ | ~~Entity thickness passthrough via base class~~ | ~~M~~ | | ✅ Resolved |
| ~~15~~ | ~~HATCH pattern definition line export~~ | ~~M~~ | | ✅ Resolved |
| ~~16~~ | ~~LWPOLYLINE plinegen flag bit translation on export~~ | ~~S~~ | | ✅ Resolved |
| 17 | Layer export: frozen/locked/lineweight/handle | M | `EoDbDxfInterface.h` (`WriteLayers`) | Open (NEW) |
| 18 | 3DFACE invisible edge flags | S | `EoDbDxfInterface.cpp`, `EoDbPolyline.h` | Open (NEW) |

### ~~Phase 4 — Table/Section Round-Trip~~ ✅ ALL RESOLVED

| # | Item | Status |
|---|------|--------|
| ~~19~~ | ~~Text style table passthrough~~ | ✅ Resolved |
| ~~20~~ | ~~Header variable passthrough map~~ | ✅ Resolved |
| ~~21~~ | ~~Dim style table passthrough~~ | ✅ Resolved |
| ~~22~~ | ~~AppId table passthrough~~ | ✅ Resolved |
| ~~23~~ | ~~Unsupported objects wiring~~ | ✅ Resolved |

### Phase 5 — Structural/Architectural (V2 dependent)

| # | Item | Effort | Files | Status |
|---|------|--------|-------|--------|
| 24 | Full spline knot/weight storage (V2 PEG) | L | `EoDbSpline.h/.cpp`, PEG serialization | Open |
| 25 | ATTRIB→INSERT structural link (V2 handle architecture) | XL | New primitive type, PEG V2 | Open |
| 26 | MTEXT reconstitution from multiline `EoDbText` | L | `EoDbText`, `EoDbDxfInterface` | Open |
| 27 | OBJECTS/DICTIONARY/LAYOUT passthrough | XL | `EoDbDxfInterface.h`, `AeSysDoc` | Open |
| 28 | Wire `WriteObjects()` to emit section + call `WriteUnsupportedObjects()` | S | `EoDbDxfInterface.h` | Open (NEW) |

### Effort Key
- **S** = Small (< 50 LOC, single file)
- **M** = Medium (50–200 LOC, 2–4 files)
- **L** = Large (200–500 LOC, multiple files)
- **XL** = Extra Large (500+ LOC, architectural change)

---

## 7. What Already Works Well

### Entity Roundtrip
- **LINE**: Full roundtrip — start/end points, thickness, all base properties.
- **ARC**: Full roundtrip — OCS→WCS on read, WCS→OCS on write, angles correctly converted (internal radians → DXF degrees via `WriteArc`).
- **CIRCLE**: Full roundtrip — OCS→WCS on read, WCS→OCS on write, radius preserved.
- **ELLIPSE**: Full roundtrip — WCS coordinates pass through correctly, `CorrectAxis()` validates axis/ratio, extrusion preserved for minor axis derivation.
- **TEXT**: Full roundtrip — rotation (rad→deg ✅), oblique angle (shear recovery ✅), generation flags ✅, extrusion direction ✅, width scale recovery ✅, alignment point selection (first vs second based on justification ✅).
- **INSERT**: Full roundtrip — scale, rotation (correctly converted rad→deg), array properties (rows, columns, spacing), extrusion all preserved.
- **LWPOLYLINE**: Full roundtrip — vertices, elevation, closed flag, plinegen (internal `0x0008` ↔ DXF `0x80`), constant width, per-vertex bulge, per-vertex start/end widths.
- **POLYLINE 3D**: Full roundtrip — `Is3D()` dispatch → `WritePolyline` (heavy POLYLINE + VERTEX + SEQEND), per-vertex Z preserved.
- **HATCH** (single-loop): Full roundtrip — solid/pattern/hollow styles, hatch angle/scale, pattern definition lines (angle, base point, offset, dash lengths), polyline boundary loop.
- **VIEWPORT**: Full roundtrip — all 17 view parameters preserved in both PEG and DXF. Forces PaperSpace on export.
- **POINT**: Full roundtrip.
- **BLOCK definitions**: Full geometry + base point + flags roundtrip.
- **ACAD_PROXY_ENTITY**: Full binary passthrough — AcGi graphics data, entity data, handles all preserved. Full AcGi metafile parser supports types 1–33 (line, polyline, circle, arc, text, shell, xline, ray, mesh, etc.).

### Table/Section Roundtrip
- **Layer table**: Full roundtrip on read — name, color, frozen (→ off), linetype. Write: name, color, off, linetype, plotting flag. (Frozen/locked/lineweight gap noted in §3.11.)
- **Linetype table**: Full roundtrip — name, pattern length, dash elements.
- **Text style table**: Full roundtrip — font name, height, width factor, oblique angle (deg↔rad conversion), generation flags, last height used.
- **VPORT table**: Full roundtrip — ~25 properties including center, height, direction, target, lens length, front/back clip, render mode, UCS data.
- **Dimension style table**: Full roundtrip — ~80 properties.
- **CLASSES section**: Full roundtrip — record type, class name, C++ class name, app name, proxy capabilities, instance count, flags.
- **AppId table**: Full roundtrip — name, flags, handle, owner handle.
- **Header variables**: Full passthrough roundtrip — all variables stored via variant type, exported via `std::visit` dispatch. `$HANDSEED` applied to handle manager.

### Infrastructure
- **Base entity properties**: Layer, color (ACI + true color + color name), linetype, line weight, thickness all preserved via `PopulateDxfBaseProperties` → `WriteEntity`.
- **Application data + extended data**: `WriteAppData` and `WriteExtData` preserve `102` groups and `1001+` xdata blocks.
- **Reactor handles + extension dictionary handles**: Written by `WriteEntity` when present.
- **Handle architecture**: Entity handles and owner handles propagated through the full pipeline via `SetBaseProperties` (read) and `PopulateDxfBaseProperties` (write).
- **Dual-space architecture**: `m_currentExportSpace` routes entities to correct space. `WriteEntities()` iterates model-space then paper-space layers. `AddToDocument` routes to correct space layer table on read.
- **Binary DXF**: Full support for both ASCII and binary DXF read/write.
- **Code page handling**: CP1252/UTF-8 codec with UTF-16 detection and fallback.
- **Unsupported objects**: `WriteUnsupportedObjects()` wired to iterate stored objects and emit raw group code data (pending `WriteObjects()` section framing §4.4).

---

## 8. Angle Pipeline Verification

This section documents the verified angle conversion pipelines to prevent regression.

### ARC (group codes 50/51)
```
DXF(degrees) → ParseCode(×DegreesToRadians) → EoDxfArc.m_startAngle(radians)
  → ConvertArcEntity → EoDbConic.m_startAngle(radians)
  → ExportToDxf → EoDxfArc.m_startAngle(radians)
  → WriteArc(×RadiansToDegrees) → DXF(degrees)  ✅ CORRECT
```

### TEXT (group code 50)
```
DXF(degrees) → ParseCode(stored as degrees) → EoDxfText.m_textRotation(degrees)
  → ConvertTextEntity(atan2 of direction vector → radians) → baked into EoGeReferenceSystem
  → ExportToDxf(RadianToDegree(atan2(xDir.y, xDir.x))) → EoDxfText.m_textRotation(degrees)
  → WriteText(written directly) → DXF(degrees)  ✅ CORRECT
```

### ELLIPSE (group codes 41/42)
```
DXF(radians) → ParseCode(stored directly) → EoDxfEllipse.m_startParameter(radians)
  → ConvertEllipseEntity → EoDbConic.m_startAngle(radians)
  → ExportToDxf → EoDxfEllipse.m_startParameter(radians)
  → WriteEllipse(written directly) → DXF(radians)  ✅ CORRECT
```

### INSERT (group code 50)
```
DXF(degrees) → ParseCode(stored as degrees) → EoDxfInsert.m_rotationAngle(degrees)
  → ConvertInsertEntity(×DegreesToRadians) → EoDbBlockReference.m_rotationAngle(radians)
  → ExportToDxf(RadianToDegree) → EoDxfInsert.m_rotationAngle(degrees)
  → WriteInsert(written directly) → DXF(degrees)  ✅ CORRECT
```

---

## 9. Summary Statistics

| Category | Total | Resolved | Open |
|----------|-------|----------|------|
| 🔴 Critical | 2 | 2 | **0** |
| 🟠 High | 8 | 3 | **5** |
| 🟡 Medium | 12 | 9 | **3** |
| 🔵 Low | 8 | 5 | **3** |
| **Total** | **30** | **19** | **11** |

### Open Items by Priority

| # | Severity | Item | Section |
|---|----------|------|---------|
| 1 | 🟠 HIGH | SPLINE degree/knots/weights lossy | §2.2 |
| 2 | 🟠 HIGH | ATTRIB structural link to INSERT | §2.4 |
| 3 | 🟠 HIGH | MTEXT multiline → individual TEXT | §2.6 |
| 4 | 🟠 HIGH | 3DFACE → LWPOLYLINE entity type change | §2.7 |
| 5 | 🟠 HIGH | HATCH multi-loop topology lost | §2.8 |
| 6 | 🟡 MEDIUM | POINT display mode per-entity | §3.9 |
| 7 | 🟡 MEDIUM | Layer export: frozen/locked/lineweight | §3.11 |
| 8 | 🟡 MEDIUM | 3DFACE invisible edge flags | §3.12 |
| 9 | 🔵 LOW | OBJECTS section not written | §4.4 |
| 10 | 🔵 LOW | Block record table incomplete | §4.6 |
| 11 | 🔵 LOW | Read-only entity types | §4.7 |
