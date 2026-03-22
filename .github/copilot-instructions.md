# Copilot Instructions

The local project repo is in a folder called `D:\Projects\Eo111222`.

I need a solution for reading open source .DXF CAD files, which I will convert to the proprietary .PEG (`Peg & Tra File Formats.md`) file. For DXF initial parsing, I will follow the example of ezdxf.

I will be making substantial changes to the .PEG file to make linear parsing of .DXF easier. Using the terminology of the .DXF specification, I want to use a handle architecture for at least the header and table sections. The only hard resource handles will be from the entities to the header and tables. I am uncertain if I need to implement extension dictionaries, but it would help with future proofing. I have no experience with this type of persistence database design.

You can assume I know the code base very well and should have little trouble with modern versions of C++. Provide suggestions detailing the code modernization.

## General Guidelines
- Purpose: Native MFC/C++ CAD/graphics application (AeSys). Keep suggestions compatible with the existing MFC architecture and on-disk file formats.
- Architecture & patterns: MFC document/view pattern (classes: `AeSysDoc`, `AeSysView`) – suggestions should maintain MFC idioms where applicable.
- Geometric primitives implement a common base (`EoDbPrimitive`) with virtuals for drawing, selection, transform, serialization – preserve virtual contract and ABI when changing signatures.
- Use `Peg & Tra File Formats.md` to help understand the legacy file structure.

## Code Style, Linters & Formatting
- Repository contains `.clang-format` and `.clang-tidy` at root – prefer those settings for formatting and static-analysis suggestions.
- For Visual Studio-specific formatting preferences, reference __Tools > Options > Text Editor > C/C++ > Formatting__.
- Minimize raw `new`/`delete`.
- Be conservative in migration from `CString` to `std::wstring` – prefer consistent conversions and avoid unnecessary copies. Prefer `std::wstring` as the only internal text API and avoid `std::string` except at unavoidable external byte boundaries.
- Step away from MFC `CObject`; minimize dynamic runtime features; avoid file serialization.
- Prefer camelCase for local variables; convert PascalCase local variables to camelCase when requested.
- Prefer marking simple geometric operations and getters `noexcept` when possible.
- Prefer `[[nodiscard]]` for getters when possible.
- Use verbose naming for variables and functions to enhance self-documentation. Aim for clarity over terseness, and include comments where necessary to explain complex logic. Favor explicit formatting/parsing options when behavior is intended to be self-documenting.

## EoDxf Text Support
- Implement only essential codec behavior first: CP1252 plus round-trip support goals for ASCII and binary DXF with ANSI_1252, UTF-8, and UTF-16.
- Current priority is EoDxfLib read/write refinement: harden the wide/narrow text boundary, then implement DXF text and hatch read/write mapped to AeSys `EoDbText` and `EoDbPolygon`, using ODA_Converter as the DWG-to-DXF intermediate.

## EoDbText ↔ DXF TEXT/MTEXT Mapping Conventions
### Reference System Encoding
- `EoDbText` stores all geometric text properties in its `EoGeReferenceSystem`:
  - `yDirection.Length()` recovers **text height**
  - `xDirection.Length()` recovers **widthScale × height × 0.6** (where 0.6 = `Eo::defaultCharacterCellAspectRatio`)
  - Direction of `xDirection` encodes the **baseline direction** (rotation)
  - Oblique angle is baked into the Y-direction as a shear rotation around the extrusion normal

To recover DXF properties from the reference system:
- `height = yDirection.Length()`
- `widthScale = xDirection.Length() / (height * Eo::defaultCharacterCellAspectRatio)`

### Character Cell Aspect Ratio and Line Spacing Reciprocal
- `Eo::defaultCharacterCellAspectRatio = 0.6` (width/height). Its reciprocal `1/0.6 = 5/3` is exactly the DXF default MTEXT baseline-to-baseline line spacing ratio. This is not coincidental — both derive from the same stroke font cell geometry.

### DXF Alignment → AeSys Alignment (Lossy Mapping)
- DXF TEXT has 6 horizontal × 4 vertical alignment values. AeSys supports only 3×3:
  - **H**: Left, Center, Right (DXF Aligned→Left, Middle→Center, Fit→Right)
  - **V**: Top, Middle, Bottom (DXF Baseline→Bottom, Bottom→Bottom)
  - Aligned/Fit stretching behavior is NOT implemented — only direction is preserved.

### MTEXT → EoDbText Multiline Convention
- DXF MTEXT becomes **multiple EoDbText primitives in a single EoDbGroup** (model space) or sequential primitives in a block. Lines are split on `\P` paragraph breaks. Each line shares the same font definition and alignment but has its own reference system origin offset by `lineSpacingFactor × (5/3) × height` downward per line.

### DXF TEXT group code 50 is in DEGREES; MTEXT group code 50 is in RADIANS
- This is a known DXF spec inconsistency. `EoDxfMText::UpdateAngle()` resolves the x-axis direction vector if present.

### .PEG Text Serialization (fixed format)
- `kTextPrimitive` → color → lineTypeIndex → fontDefinition → referenceSystem → text string (tab-delimited CP_ACP). No oblique angle, width scale, or text generation flags are persisted separately — they must be baked into the reference system or font definition at import time.

### DXF TEXT Origin Point Selection
- Left + Baseline: first alignment point (group 10/20/30)
- Aligned/Fit: first alignment point (baseline start); both points define direction
- All other non-default alignments: **second alignment point** (group 11/21/31)

### DXF ATTRIB Duplicate Group Codes (AcDbText vs AcDbAttribute)
- DXF ATTRIB entities contain two subclass sections: `AcDbText` (text properties) and `AcDbAttribute` (attribute-specific properties).
- AutoCAD and ODA Converter **duplicate** group codes 71 (text generation flags), 72 (horizontal justification), and 11/21/31 (second alignment point) in both sections, potentially with **different values**.
- The `AcDbText` section's values are authoritative. `ParseBaseCode()` tracks the subclass marker via `m_pastAttributeSubclassMarker` and ignores codes 71, 72, 11/21/31 after the `AcDbAttribute` or `AcDbAttributeDefinition` marker.
- This follows the same pattern as `EoDxfAcadProxyEntity::m_pastProxySubclassMarker` for disambiguating code 330.

## BLOCK / ATTDEF / INSERT / ATTRIB — Current State and Direction

### Current Implementation (DXF Import Only)
- **ATTDEF** (`ConvertAttDefEntity`): Parsed and counted but **not rendered**. ATTDEFs are block-definition templates; rendering them would produce overlapping text with ATTRIB values at the same transformed position.
- **ATTRIB** (`ConvertAttribEntity`): Fully converted to `EoDbText` primitives using the same pipeline as `ConvertTextEntity` (OCS→WCS transform, alignment point selection, justification mapping, oblique angle). Added to model-space groups on the entity's layer via `AddToDocument`.
- **INSERT** (`ConvertInsertEntity`): Creates `EoDbBlockReference` with scale/rotation/normal. The DXF reader calls `AddInsert` first, then `ProcessInsertAttribs` delivers each ATTRIB sequentially — they become separate `EoDbGroup` entries on the layer.
- **Block geometry** (LINE, ARC, etc. inside BLOCK definitions): Converted normally and stored in `EoDbBlock` via `m_currentOpenBlockDefinition->AddTail()`.

### Structural Limitation
- ATTRIBs become standalone `EoDbText` with no structural link to their parent INSERT or ATTDEF tag. In .PEG V1, they serialize as ordinary text primitives — the attribute identity (tag name, block association) is lost on save.
- `EoDbBlock::HasAttributes()` (flag bit 1) is preserved from the DXF block flags but is not currently used during rendering or editing.

### Future Direction (.PEG V2)
- The BLOCK/ATTDEF/INSERT/ATTRIB relationship is a strong forcing function for the .PEG V2 handle architecture: block references owning attribute handles, ATTDEFs persisting in block definitions, tag-based attribute editing across inserts.
- Interactive attribute prompting (iterate ATTDEFs on block insertion, prompt for values, create positioned ATTRIBs) is deferred until the handle architecture can preserve the tag association on save.

### Test File
- `DXF Test Files/RoomNumber_Block_Insert.dxf` — 4×3 rectangle block with centered NUMBER ATTDEF, three INSERT instances with ATTRIB values (101, 102, CONF-A), one with X-scale 1.5.

## EoDbText Rendering and Formatting Architecture
- AeSys handles `\P`, `\A`, and `\S` formatting codes **at render time** inside `DisplayTextWithFormattingCharacters()` (detection via `HasFormattingCharacters()` in `EoDbText.cpp`). MTEXT formatting codes that map to these can be **preserved in the text string** rather than stripped at import time. Only unsupported codes (font `\f`, color `\C`, height `\H`, width `\W`, tracking `\T`, oblique `\Q`, underline `\L`/`\l`, overline `\O`/`\o`) need to be stripped during DXF import.
- The `\r\n` newline convention is handled during `DisplayText()` by splitting into segments via `text_GetNewLinePos()` for line advancement.
- Legacy .peg files use `^/` … `^` for stacked fractions. `ConvertFormattingCharacters()` converts these to `\S` … `;` format at load time. DXF MTEXT already uses `\S` natively, so MTEXT stacked fractions pass through directly.
- The stroke font renderer (`DisplayTextSegmentUsingStrokeFont`) uses `Eo::defaultCharacterCellAspectRatio` (0.6) for character spacing.
- TrueType font rendering path (`DisplayTextSegmentUsingTrueTypeFont`) is conditional on the font definition's precision and view settings.

## EoDbText Constructor Behavior
- Both `EoDbText` constructors (`CString&` and `std::wstring&` variants) call `renderState.Color()` to set `m_color`. When importing from DXF, `SetBaseProperties()` must be called AFTER construction to override this with the entity's actual color. The current conversion code does this correctly.

## DPI Handling
- Prefer using `GetDpiForSystem` (or `GetDpiForWindow` when available) for DPI fixes in this codebase.

## Coordinate System Conventions
- **OCS (Object Coordinate System)**: DXF/DWG entities use OCS defined by an extrusion vector. When `extrusion.z < 0`, CCW in OCS appears CW when viewed from +Z in WCS.
- **WCS (World Coordinate System)**: Legacy PEG files store geometry directly in WCS without OCS conventions.
- Use `EoDbPrimitive::ComputeArbitraryAxis()` for DXF arbitrary axis algorithm.

### DXF Entity Coordinate Conventions (OCS vs WCS)
- **ARC, CIRCLE**: Center (10/20/30) is in **OCS**. Angles (50/51) are in OCS. `ApplyExtrusion()` transforms center OCS→WCS via `CalculateArbitraryAxis` + `ExtrudePointInPlace`.
- **ELLIPSE**: Center (10/20/30) and major axis endpoint (11/21/31) are in **WCS** per the DXF specification. `ApplyExtrusion()` is a no-op. The extrusion direction (210/220/230) defines the ellipse plane normal only — used by `MinorAxis() = Cross(extrusion, majorAxis) × ratio`.
- This WCS/OCS difference between ELLIPSE and ARC/CIRCLE is a known DXF specification inconsistency.

## Angle Conventions
- All angles are in **radians**.
- Use `NormalizeTo2Pi()` to normalize angles to `[0, 2π)`.
- Sweep angles: positive = CCW, negative = CW (in the entity's coordinate system).
- Constants: `Eo::TwoPi`, `Eo::HalfPi`, `Eo::Pi`.

## Tolerance Constants
- `Eo::geometricTolerance` — for geometric comparisons (point coincidence, zero-length vectors).
- `Eo::numericEpsilon` — for floating-point arithmetic comparisons.
- Prefer `< Eo::geometricTolerance` over `== 0.0` for length/distance checks.

## Geometry Classes Quick Reference
| Class | Purpose |
|-------|---------|
| `EoGePoint3d` | 3D point with x, y, z |
| `EoGeVector3d` | 3D vector, supports `CrossProduct`, `Normalize`, `RotAboutArbAx` |
| `EoGeTransformMatrix` | 4x4 transformation; use `Inverse()` for OCS↔WCS |
| `EoGeLine` | Line segment defined by two points |
| `EoGeReferenceSystem` | Coordinate system with origin and axes |

## Key Files & Entry Points (Open/Important)
- Core primitive types and headers:
  - `AeSys\EoDbPrimitive.h`
  - `AeSys\EoDbLine.h`, `AeSys\EoDbConic.h`
  - `AeSys\EoDbPoint.h`, `AeSys\EoDbPolyline.h`, `AeSys\EoDbPolygon.h`, `AeSys\EoDbSpline.h`
  - `AeSys\EoDbText.h`, `AeSys\EoDbBlockReference.h`

## Documentation
- Utilize Doxygen for automated documentation generation. Ensure that comments are clear and descriptive, and consider the balance between verbosity and clarity to maintain readability.

## Simplex PSF Stroke Font (v2 Format)

### Format Overview
- `Simplex.psf` is a 16384-byte binary stroke font derived from the Hershey simplex font set.
- **v1** (legacy): 96-entry offset table for ASCII 32–126, stroke data at `int32[96]`, all characters use a fixed advance width of 1.0 normalized units.
- **v2** (current): magic `−2` at `int32[0]`, 225-entry offset table at `int32[1]`, 224-entry advance width table at `int32[226]`, 224-entry left bearing table at `int32[450]`, stroke data at `int32[674]`. Supports character codes 32–255 (CP1252).
- Full format specification: `Documentation/Simplex PSF Format.md`.

### Stroke Encoding
- Each stroke is a packed 32-bit integer: bits 0–11 Y displacement (sign-magnitude), bits 12–23 X displacement (sign-magnitude), bits 24–31 opcode (5 = MOVE, else DRAW).
- Hershey origin mapping: `psf_x = hershey_x × 3.75 + 50`. All characters begin with a MOVE to raw X ≈ 50.

### Normalized Coordinate System
- X displacements are scaled by `0.01 / Eo::defaultCharacterCellAspectRatio` (= 0.01/0.6 ≈ 0.01667).
- Y displacements are scaled by `0.01`.
- A full Hershey cell (raw 0–100) spans **1.667** in normalized X and **1.0** in normalized Y.
- The `EoGeReferenceSystem` transform matrix maps normalized coordinates to world coordinates.

### Per-Character Advance Widths (v2)
- Each character's advance width is stored in raw stroke X-units in the advance width table.
- Normalized cell width: `rawAdvanceWidth × 0.01 / 0.6`.
- Character advance in renderer: `cellWidth + interCharacterGap`, where `interCharacterGap = (0.32 + spacing) / 0.6`.
- To adjust a character's proportional width, edit only the **advance width table** (`int32[226..449]`).

### Left Bearing Table (v2)
- Each character's left bearing shifts strokes leftward so they are left-aligned within their proportional cell.
- Applied before the stroke loop: `ptStroke.x −= rawLeftBearing × 0.01 / 0.6`.
- Formula used during conversion: `leftBearing = max(0, minCumulativeX − 6)`, where 6 raw units provides a consistent left margin.

### Text Alignment (Center/Middle Justification)
- `GetBottomLeftCorner()` and `text_GetBoundingBox()` compute text extent using `ComputeStrokeFontTextExtent()`, which sums per-character v2 advance widths (or falls back to fixed 1.0 for v1).
- `ComputeStrokeFontTextExtent()` skips formatting characters (`\P`, `\A`, `\S` sequences), mirroring `LengthSansFormattingCharacters()` logic.
- Both functions accept `const CString& text` (not `int` character count) so they can look up per-character advance widths.
- `CharacterCellWidth()` is a file-local helper that returns the normalized cell width for a single character code.

### Tooling (Round-Trip `.psf` ↔ `.psf.txt`)
| Tool | Purpose |
|------|---------|
| `Tools/ConvertPsfToText.ps1` | Decompiles binary `.psf` → human-readable `.psf.txt` (tab-separated) |
| `Tools/ConvertTextToPsf.ps1` | Compiles `.psf.txt` → v2 binary `.psf` |
| `Tools/ConvertStrokeFontV1ToV2.ps1` | One-time v1 → v2 binary migration with advance width and left bearing computation |

- Round-trip is **byte-for-byte identical**: `.psf` → `.psf.txt` → `.psf`.
- The `.psf.txt` format uses `CHAR` lines with fields: `code`, `label`, `advanceWidth`, `leftBearing`, followed by indented `M` (move) and `D` (draw) stroke lines with raw X, Y values.

### Key Constants (`Eo.h`)
| Constant | Value | Purpose |
|----------|-------|---------|
| `strokeFontFileSizeInBytes` | 16384 | Fixed file size for both v1 and v2 |
| `strokeFontV2MagicNumber` | −2 | Identifies v2 format at `int32[0]` |
| `strokeFontV2AdvanceWidthTableStart` | 226 | Advance width table begins at `int32[226]` |
| `strokeFontV2LeftBearingTableStart` | 450 | Left bearing table begins at `int32[450]` |
| `strokeFontV2StrokeDataStart` | 674 | Stroke data begins at `int32[674]` |
| `defaultCharacterCellAspectRatio` | 0.6 | Width-to-height ratio of the character cell |

## EoDbConic ↔ DXF Conic Pipeline (ARC, CIRCLE, ELLIPSE)

### Internal Representation
- `EoDbConic` stores: `m_center` (WCS), `m_majorAxis` (WCS vector), `m_extrusion` (unit normal), `m_ratio` (minor/major, 0 < r ≤ 1), `m_startAngle`, `m_endAngle` (radians).
- `MinorAxis()` = `CrossProduct(m_extrusion, m_majorAxis) × m_ratio` — derived at render time, not stored.
- `Subclass()` classifies: `Circle` (ratio ≈ 1, full), `RadialArc` (ratio ≈ 1, partial), `Ellipse` (ratio < 1, full), `EllipticalArc` (ratio < 1, partial).

### DXF Read Pipeline

| Entity | Coord System | ApplyExtrusion | Converter | Factory | Notes |
|--------|-------------|----------------|-----------|---------|-------|
| CIRCLE | OCS | `ExtrudePointInPlace(center)` | `ConvertCircleEntity` | `CreateCircle` | majorAxis = `ComputeArbitraryAxis(ext) × radius` |
| ARC | OCS | `ExtrudePointInPlace(center)` | `ConvertArcEntity` | `CreateRadialArc` | majorAxis = `ComputeArbitraryAxis(ext) × radius`; angles: deg→rad in parser, OCS pass-through |
| ELLIPSE | **WCS** | **No-op** | `ConvertEllipseEntity` | `CreateConic` | majorAxis from DXF code 11/21/31; angles in radians |

- ARC/CIRCLE: `ApplyExtrusion` transforms center from OCS→WCS. `ComputeArbitraryAxis` reconstructs the OCS X-axis direction as the major axis, encoding the directional flip for negative-Z extrusion.
- ELLIPSE: Coordinates are already WCS per DXF spec. `ApplyExtrusion()` is intentionally a no-op. Extrusion is used only by `MinorAxis()`.

### Render Pipeline
- `Display()` → `GenerateApproximationVertices(m_center, m_majorAxis)`
- Computes `minorAxis = MinorAxis()`, builds `EoGeTransformMatrix(center, majorAxis, minorAxis)`, inverts it.
- Sweeps unit circle from `m_startAngle` through `(m_endAngle − m_startAngle)` with adaptive tessellation.
- Transform maps `(cos θ, sin θ, 0)` → WCS points via `transformMatrix * point`.

### DXF Write Pipeline
- `ExportToDxf()` dispatches on `Subclass()`:
  - **Circle** → `EoDxfCircle`: center, extrusion, radius.
  - **RadialArc** → `EoDxfArc`: center, extrusion, radius, start/end angles.
  - **Ellipse/EllipticalArc** → `EoDxfEllipse`: center, majorAxis, extrusion, ratio, start/end params.
- `WriteArc` converts angles rad→deg (DXF ARC group 50/51 are in degrees).
- `WriteEllipse` calls `CorrectAxis()` to validate ratio/axis before output.
- **Known gap**: ARC/CIRCLE export writes `m_center` (WCS) directly as DXF code 10/20/30, which should be OCS for non-default extrusion. For extrusion `[0,0,1]` (default) WCS = OCS so this is transparent. A WCS→OCS reverse transform is needed for correct round-trip with non-default extrusion.

### Test Files
- `DXF Test Files/Ellipse_NegZ_CW_Test.dxf` — 24-entity test: 6 ellipticals (E1–E6) + 6 radials (R1–R6) with both `[0,0,1]` and `[0,0,-1]` extrusion, plus default-extrusion baselines.
- `DXF Test Files/GenerateEllipseTest.ps1` — PowerShell generator using AC1021 skeleton injection.

## EoDbPolyline ↔ DXF LWPOLYLINE / POLYLINE Mapping

### Internal Representation
- `EoDbPolyline` is the generalized polyline primitive handling both DXF LWPOLYLINE and heavy 2D/3D POLYLINE entities.
- Core members: `m_flags` (int16), `m_pts` (EoGePoint3dArray), `m_bulges` (vector\<double\>), `m_startWidths` / `m_endWidths` (vector\<double\>), `m_constantWidth` (double).
- Parallel vectors (`m_bulges`, `m_startWidths`, `m_endWidths`) remain **empty** for simple polylines — zero overhead until DXF import populates them.

### Flag Bit Constants
| Constant | Value | Meaning |
|----------|-------|---------|
| `sm_Closed` | `0x0001` | Last vertex connects back to first |
| `sm_HasBulge` | `0x0002` | `m_bulges` contains per-vertex bulge values |
| `sm_HasWidth` | `0x0004` | `m_startWidths` / `m_endWidths` are populated |
| `sm_Plinegen` | `0x0008` | Generate linetype pattern across vertices |

### PEG Serialization (type code `kPolylinePrimitive` = 0x2002)Type code <0x2002>   uint16_t
Pen color            int16_t
Line type            int16_t
Flags                uint16_t   (sm_Closed | sm_HasBulge | sm_HasWidth | sm_Plinegen)
Constant width       double     (DXF group code 43; 0.0 when not set)
Number of vertices   uint16_t
{vertices}           point3d[]
if (flags & sm_HasBulge):
  {bulge values}     double[]   (one per vertex)
if (flags & sm_HasWidth):
  {start widths}     double[]   (one per vertex)
  {end widths}       double[]   (one per vertex)
### DXF Read Pipeline

| DXF Entity | Parser | Processor | Callback | Converter | Notes |
|-----------|--------|-----------|----------|-----------|-------|
| LWPOLYLINE | `EoDxfLwPolyline::ParseCode` | `ProcessLWPolyline` | `AddLWPolyline` | `ConvertLWPolylineEntity` | Lightweight; vertices in OCS with `z` from elevation |
| POLYLINE (3D) | `EoDxfPolyline::ParseCode` + `EoDxfVertex::ParseCode` | `ProcessPolyline` + `ProcessVertex` | `AddPolyline` (flag `0x08`) | `ConvertPolyline3DEntity` | Full 3D vertex coordinates |
| POLYLINE (2D) | Same as 3D | Same as 3D | `AddPolyline` (no 0x08/0x10/0x40) | `ConvertPolyline2DEntity` | OCS x,y + polyline elevation z; bulge/width per vertex |
| POLYLINE (mesh) | Same as 3D | Same as 3D | `AddPolyline` (flag `0x40` or `0x10`) | **Skipped** | Polyface mesh / polygon mesh not mappable |

### AddPolyline Subtype Discrimination (`EoDbDxfInterface.h`)flag & 0x40  → polyface mesh → skip + log
flag & 0x10  → polygon mesh  → skip + log
flag & 0x08  → 3D polyline   → ConvertPolyline3DEntity
else         → 2D polyline   → ConvertPolyline2DEntity
### DXF → EoDbPolyline Property Mapping (LWPOLYLINE)
| DXF Property | Group Code | EoDbPolyline Member/Method | Notes |
|---|---|---|---|
| Vertices (x,y) | 10/20 | `SetVertexFromLwVertex()` | Z from elevation (group 38) via `EoDxfPolylineVertex2d.z` |
| Closed | flag 0x01 | `SetClosed(true)` | |
| Plinegen | flag 0x80 | `SetPlinegen(true)` | DXF uses 0x80; PEG uses sm_Plinegen=0x0008 |
| Constant width | 43 | `SetConstantWidth()` | Expanded to per-vertex widths if no per-vertex data |
| Per-vertex bulge | per vertex | `SetBulges(vector&&)` | Only populated when any vertex has non-zero bulge |
| Per-vertex widths | 40/41 per vertex | `SetWidths(vector&&, vector&&)` | Only populated when any vertex has non-zero width |

### DXF → EoDbPolyline Property Mapping (Heavy 2D POLYLINE)
| DXF Property | Source | EoDbPolyline Member/Method | Notes |
|---|---|---|---|
| Vertex x,y | `EoDxfVertex::m_locationPoint` | `SetVertex()` | Z = polyline elevation (`m_polylineElevation.z`) |
| Closed | POLYLINE flag 0x01 | `SetClosed(true)` | |
| Plinegen | POLYLINE flag 0x80 | `SetPlinegen(true)` | |
| Per-vertex bulge | `EoDxfVertex::m_bulge` | `SetBulges(vector&&)` | |
| Per-vertex widths | `EoDxfVertex::m_startingWidth/m_endingWidth` | `SetWidths(vector&&, vector&&)` | Falls back to `m_defaultStartWidth/m_defaultEndWidth` |
| Curve-fit / spline-fit | flags 0x02/0x04 | All vertices kept | Structure lost but rendered geometry preserved |

### Bulge Arc Tessellation (`polyline::TessellateArcSegment`)
- Bulge = `tan(θ/4)` where θ is the included angle of the arc. Positive = CCW, negative = CW.
- Algorithm: compute center from chord midpoint + perpendicular offset, then direct 2D rotation of the center→start direction vector.
- Adaptive tessellation: `ceil(|sweepAngle| / 2π × segmentsPerFullCircle)` with minimum 2 segments.
- Last tessellated point is snapped to the exact endpoint to avoid floating-point drift.
- Constants: `Eo::arcTessellationSegmentsPerFullCircle = 72`, `Eo::arcTessellationMinimumSegments = 2`.

### Display Pipeline
- `Display()` → `polyline::BeginLineLoop/Strip` → emit vertices with `polyline::SetVertex` → `polyline::__End`.
- Bulge-aware: each edge tessellated via `TessellateArcSegment`. Closing segment for closed polylines respects its bulge.
- Selection/extents use `BuildTessellatedPoints()` (private helper) which produces a flattened point array with arcs expanded.
- **Width rendering not yet implemented** — width data is preserved for PEG round-trip but polylines render as zero-width lines.

### Known Limitations and Deferred Work
| Item | Notes |
|------|-------|
| Width rendering in `Display()` | Data stored but not rendered; needs filled polygon/trapezoid approach |
| `ExportToDxf()` | Not implemented; dispatch to `WriteLWPolyline` or `WritePolyline` based on dimensionality |
| 2D POLYLINE OCS→WCS | `ConvertPolyline2DEntity` stores OCS coordinates directly; non-default extrusion needs transform |
| Non-uniform scale + bulge | Bulge is dimensionless; non-uniform BLOCK INSERT scales distort arcs (matches AutoCAD behavior) |
| Break bulge arcs | Decomposing individual bulge arcs to `EoDbConic` for editing is deferred |

### Test Files
| File | Contents |
|------|----------|
| `DXF Test Files/LWPolyline_Bulge_Test.dxf` | 21 LWPOLYLINE cases: open/closed, positive/negative/mixed bulge, semicircle, closed diamond |
| `DXF Test Files/Heavy_Polyline_Subtypes.dxf` | 13 heavy POLYLINE cases: 3D open/closed, 2D basic/closed/elevation, bulge (±/mixed/closed), per-vertex width, default width, bulge+width, plinegen (open/closed) |
| `DXF Test Files/GenerateHeavyPolylineTest.ps1` | PowerShell generator for `Heavy_Polyline_Subtypes.dxf` using StringBuilder + helper functions |

## EoDbSpline ↔ DXF SPLINE Mapping and V2 .PEG Generalization

### Current State (V1 .PEG)
- `EoDbSpline` stores **only** control points (`EoGePoint3dArray m_pts`), color, and line type.
- `Display()` calls `GenPts(orderOfTheSpline, m_pts)` where `orderOfTheSpline` is a file-scope `constexpr std::int16_t{4}` (cubic, degree 3).
- `GenPts` implements Cox–de Boor B-spline tessellation with a **uniform knot vector** regenerated at render time. The knot vector, degree, weights, and flags from DXF are discarded at import.
- V1 .PEG serialization: `kSplinePrimitive → color → lineTypeIndex → uint16(pointCount) → points[]`
- All splines in a drawing render identically as uniform cubic B-splines regardless of their DXF source degree.

### DXF → EoDbSpline Import (Lossy Mapping)
| DXF Property | Group Codes | Preserved | Notes |
|---|---|---|---|
| Control points | 10/20/30 | ✅ | OCS → WCS transform applied |
| Fit points (fallback) | 11/21/31 | ✅ as control points | Used only when no control points |
| Degree | 71 | ❌ | All render as cubic (order 4) |
| Knot vector | 40 | ❌ | Regenerated as uniform |
| Weights (NURBS) | 41 | ❌ | Treated as non-rational |
| Flags (closed/periodic) | 70 | ❌ | Closure encoded in control point wrap |
| Start/end tangents | 12-13/22-23/32-33 | ❌ | Ignored |

### EoDxfSpline Parser Notes
- `EoDxfSpline::ParseCode()` accumulates control/fit points via heap-allocated `EoDxfGeometryBase3d*`. **Future**: migrate to `std::vector<EoDxfGeometryBase3d>` (value semantics — the type is a trivial POD of 3 doubles).
- `m_numberOfKnots`, `m_numberOfControlPoints`, `m_numberOfFitPoints` are declared counts (group codes 72/73/74) parsed independently from the actual vectors. The write path iterates declared counts — a count/vector size mismatch from a malformed DXF can cause `std::out_of_range`. Add validation after parse completes.
- Spline flag bits: `0x01` = Closed, `0x02` = Periodic, `0x04` = Rational, `0x08` = Planar, `0x10` = Linear.
- `IsTangentValid()` checks whether start/end tangent vectors are non-zero (tangent group codes are optional in DXF, not gated by any flag bit).

### V2 .PEG Spline Record Design
To preserve DXF fidelity through save/reload, the V2 spline record adds degree, knot vector, and optional weights:
```
kSplinePrimitive → color → lineTypeIndex → flags(uint16) → degree(int16)
  → numKnots(uint16) → numControlPoints(uint16)
  → knots[numKnots] (double[])
  → weights[numControlPoints] (double[], only if Rational flag set)
  → controlPoints[numControlPoints] (EoGePoint3d[])
```

- `Display()` calls `GenPts(degree + 1, m_pts)` using stored degree instead of file-scope constant.
- If the stored knot vector is non-empty, `GenPts` should use it instead of regenerating a uniform one.
- Rational flag (bit 0x04): when set, weight values are stored and applied during tessellation (NURBS). When clear, weight values are omitted and all weights are implicitly 1.0.
- Closed flag (bit 0x01): the control point array already encodes closure through wrapping (AutoCAD convention). The flag is informational for editing/export but does not affect `GenPts` directly.
- Backward compatibility: V1 spline records (no degree/knots) default to `degree = 3`, uniform knots, non-rational — identical to current behavior.

### GenPts Tessellation Algorithm
- Implements Cox–de Boor B-spline recursion with `order = degree + 1`.
- Uses 2D weight array with `stride = order + 1` (rows = knot span count, columns = recursion levels).
- Tessellation density: `8 × numberOfControlPoints` segments.
- Dynamic `std::vector<double>` allocation (was fixed 65×66 prior to overflow fix).
- When `order > numberOfControlPoints`, falls back to a single line segment from first to last control point.

### Known Deferred Work
- `SelectUsingRectangle` tests the raw **control polygon**, not the tessellated curve (inconsistent with `SelectUsingPoint` which tessellates first). For V2, both should operate on tessellated points.
- `GetControlPoint()`, `GoToNextControlPoint()`, `IsInView()` lack empty-array guards — add early returns for `m_pts.GetSize() == 0`.
- `operator=` should add a self-assignment guard for `CArray::Copy` safety.
- Per-spline degree storage in `EoDbSpline` as an `m_degree` member is the minimum V2 change. Knot/weight storage follows when NURBS round-trip is needed.
- The `.PEG V2` handle architecture (entity → table/header handles) provides a natural hook for spline style tables if multiple drawings need shared tessellation parameters.

## Response Formatting Preference
- Format responses as a cleaner Markdown-style preview, with better visual structure than plain text.

## Versioning
- Use `$AESVER` as the PEG file format version variable. "AES" is shortened from "AeSys". Do not add a separate `$PEGVERSION` — `$AESVER` already serves this purpose.
- Two production versions: Legacy `"AE2011"` (2005-era) and current `"AE2026"`.
- `$AESVER` is informational/documentary; the actual V2 discriminator on read is the post-EOF `kPaperSpaceSection` sentinel.

## PEG V2 Dual-Space Architecture

### In-Memory Model
- `AeSysDoc` holds two independent layer tables: `CLayers m_modelSpaceLayers` and `CLayers m_paperSpaceLayers`.
- `EoDxf::Space m_activeSpace` controls which space the UI operates on (default: `ModelSpace`).
- `ActiveSpaceLayers()` routes by `m_activeSpace`; `SpaceLayers(space)` provides explicit access.
- `AddLayerToSpace(layer, space)` and `FindLayerInSpace(name, space)` target a specific space.
- View → Model Space menu toggle (`ID_VIEW_MODELSPACE = 4811`) switches `m_activeSpace`.

### On-Disk Format (Post-EOF Extension)
The PEG V2 format appends paper-space data **after** the "EOF" marker. V1 readers (which stop after the four main sections) never encounter it.

```
[V1-compatible section — all readers see this]
  kHeaderSection (0x0101) → V2 variable triples ($AESVER, …) → kEndOfSection
  kTablesSection (0x0102)
    kViewPortTable → 0 → kEndOfTable
    kLinetypeTable → count → entries → kEndOfTable
    kLayerTable → count → MODEL-SPACE layers → kEndOfTable
  kEndOfSection
  kBlocksSection (0x0103) → count → blocks → kEndOfSection
  kGroupsSection (0x0104) → count → MODEL-SPACE entities → kEndOfSection
  "EOF"
[V2 extension — only V2 readers continue past "EOF"]
  kPaperSpaceSection (0x0105)
    kLayerTable → count → PAPER-SPACE layers → kEndOfTable
    kGroupsSection → count → PAPER-SPACE entities → kEndOfSection
  kEndOfSection
```

- **Binary equality**: A V2 file with no paper-space layers is byte-for-byte identical to V1 — nothing extra in the header, nothing after "EOF".
- **Write path**: `WriteLayerTable` and `WriteEntitiesSection` explicitly use `SpaceLayers(ModelSpace)`. `WritePaperSpaceExtension` writes the post-EOF section only when paper-space layers exist.
- **Read path**: `ReadPaperSpaceExtension` consumes the "EOF" string, checks for remaining data, and reads paper-space layers/entities if `kPaperSpaceSection` follows.
- **Sentinel constants**: `kPaperSpaceSection = 0x0105` in `EoDb::Sentinels`.

### Key Files
| File | Role |
|------|------|
| `EoDbPegFile.cpp` | `Load`/`Unload` + `ReadPaperSpaceExtension`/`WritePaperSpaceExtension` |
| `EoDbPegFile.h` | Method declarations |
| `EoDb.h` | `kPaperSpaceSection` sentinel |
| `AeSysDoc.h` | Dual-space members, accessors |
| `AeSysDoc.cpp` | `ActiveSpaceLayers`, `SpaceLayers`, `AddLayerToSpace`, `FindLayerInSpace` |

## Paper-Space Viewport Display Pipeline

### EoDbViewport Primitive
- Type code `kViewportPrimitive = 0x8000`. Full `EoDbPrimitive` virtual contract implementation.
- Stores paper-space geometry (`m_centerPoint`, `m_width`, `m_height`) and model-space view parameters (`m_viewCenter`, `m_viewDirection`, `m_viewHeight`, `m_viewTargetPoint`, `m_twistAngle`, etc.).
- PEG serialization: 17 members (color, lineType, center, size, viewport identity, 12 view parameters).
- DXF export via `ExportToDxf(EoDxfViewport&)`.
- `Display()` renders the viewport boundary rectangle with dotted pen.

### Viewport Rendering (`DisplayModelSpaceThroughViewports`)
- Walks paper-space layers for `EoDbViewport` primitives.
- For each viewport: computes GDI clip rect from viewport corners → device coords, `SaveDC`/`IntersectClipRect`.
- `PushViewTransform` + configures camera (target from `viewCenter`, direction from `viewDirection`).
- **Off-center projection window**: The orthographic projection maps view window to NDC [-1,1] which maps to FULL device viewport. Since we clip to a sub-region, the window is enlarged by `deviceDimension / clipDimension` and offset so the camera target projects to the clip region center:
  - `halfExtentU = viewWidth × deviceWidth / (2.0 × clipWidth)`
  - `halfExtentV = viewHeight × deviceHeight / (2.0 × clipHeight)`
  - `windowCenterU = halfExtentU × (1.0 - 2.0 × clipCenterX / deviceWidth)`
  - `windowCenterV = halfExtentV × (2.0 × clipCenterY / deviceHeight - 1.0)`
- Renders model-space layers, `PopViewTransform`, `RestoreDC`.

### Key Files
| File | Role |
|------|------|
| `EoDbViewport.h` / `.cpp` | Viewport primitive (full virtual contract, PEG serialization, DXF export) |
| `AeSysDoc.cpp` | `DisplayModelSpaceThroughViewports` (clip + projection pipeline) |
| `EoDbDxfInterface.h` / `.cpp` | `ConvertViewportEntity` (DXF import) |
| `EoGsViewTransform.cpp` | `BuildTransformMatrix`, `SetCenteredWindow`, `SetWindow` |
