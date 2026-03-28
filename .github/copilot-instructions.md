# Copilot Instructions

You are running in full autonomous Agent mode with Planning enabled.
- Execute the ENTIRE requested plan in one continuous session.
- Do not pause, ask for confirmation, or output "continue" messages unless you encounter a critical error, security issue, or need explicit user clarification.
- Chain as many steps, tool calls, file edits, builds, and tests as needed to complete the task.
- After each major phase, briefly summarize progress but immediately proceed to the next step.
- Use Planning tools (`plan`, `adapt_plan`, `update_plan_progress`, `finish_plan`) internally when helpful.
- Prioritize completeness and correctness over stopping early.

## General Context
The local project repo is in a folder called `D:\Projects\Eo111222`.

DXF reading via `EoDxfLib` is operational for core entity types (LINE, ARC, CIRCLE, ELLIPSE, TEXT, MTEXT, ATTRIB, INSERT, LWPOLYLINE, POLYLINE, SPLINE, HATCH, VIEWPORT), following the ezdxf architecture model. Parsed entities are converted to `EoDbPrimitive`-derived objects and stored in `AeSysDoc` layers. See `Peg & Tra File Formats.md` for the legacy file structure.

The PEG V2 handle architecture is **implemented**: `EoDbPrimitive` carries `m_handle`/`m_ownerHandle` (`std::uint64_t`), assigned via `EoDbHandleManager`. Entity→layer/linetype handle linkage covers current import/export needs. Extension dictionaries are **deferred** — see the **Handle Architecture** section below.

## PEG File Compatibility
- **AE2011** (V1): The only backward-compatible format. All V1 read/write paths must be preserved.
- **AE2026** (V2): Experimental — writes are discarded after debug review until a milestone is reached. New fields can be added freely without versioning guards. Do not add backward-compatibility complexity for AE2026.

You can assume I know the code base very well and should have little trouble with modern versions of C++. Provide suggestions detailing the code modernization.

## General Guidelines
- Purpose: Native MFC/C++ CAD/graphics application (AeSys). Keep suggestions compatible with the existing MFC architecture and on-disk file formats.
- Architecture & patterns: MFC document/view pattern (classes: `AeSysDoc`, `AeSysView`) – suggestions should maintain MFC idioms where applicable.
- Geometric primitives implement a common base (`EoDbPrimitive`) with virtuals for drawing, selection, transform, serialization – preserve virtual contract and ABI when changing signatures.
- Use `Peg & Tra File Formats.md` to help understand the legacy file structure.
- **vcxproj wildcard**: `AeSys.vcxproj` uses `<ClCompile Include="*.cpp" Exclude="...">` — **never explicitly add new `.cpp` files to the project**. Only new `.h` files require an explicit `<ClInclude>` entry in both `AeSys.vcxproj` and `AeSys.vcxproj.filters`.

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
- Use `%I64X` (not `%llX`) for `std::uint64_t` in `CString::Format` — this is the MSVC-traditional specifier and is consistent throughout the codebase.

## EoDxf Text Support
- Implement only essential codec behavior first: CP1252 plus round-trip support goals for ASCII and binary DXF with ANSI_1252, UTF-8, and UTF-16.
- Current priority is EoDxfLib read/write refinement: harden the wide/narrow text boundary, then implement DXF text and hatch read/write mapped to AeSys `EoDbText` and `EoDbPolygon`, using ODAFileConverter as the DWG-to-DXF intermediate.

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

### Class Roles
| Class | Role | Serialization |
|-------|------|---------------|
| `EoDbBlock` | Block definition container; holds geometry (`EoDbGroup`) + `std::vector<EoDxfAttDef>` ATTDEF catalog | DXF BLOCK/ENDBLK + ATTDEFs; PEG block table |
| `EoDbBlockReference` | INSERT instance; carries scale/rotation/normal + `m_attributeHandles` (owned ATTRIB handles) | PEG kInsertPrimitive; DXF INSERT+SEQEND |
| `EoDbAttrib` | ATTRIB instance; `EoDbText` subclass adding `m_tagString`, `m_attributeFlags`, `m_insertHandle` | PEG V1: written as kTextPrimitive (identity lost); PEG V2: kAttribPrimitive with tag/flags/insertHandle extension |
| `EoDxfAttDef` | ATTDEF template stored raw in `EoDbBlock::m_attributeDefinitions`; not converted to a primitive | DXF round-trip only; used as template for interactive insertion prompting |

### Implemented (Phases 1–6 Complete)
- **ATTDEF storage** (`ConvertAttDefEntity`): Parsed `EoDxfAttDef` stored directly in `EoDbBlock::m_attributeDefinitions` (not rendered — avoids overlapping INSERT ATTRIB text). Full DXF property set preserved for round-trip export.
- **ATTRIB import** (`ConvertAttribEntity`): Converted to `EoDbAttrib` (not plain `EoDbText`) via the same OCS→WCS/alignment pipeline as TEXT. Linked to parent INSERT via `m_insertHandle`/`m_ownerHandle`; INSERT owns the handle in `m_attributeHandles`. ATTRIBs are added to the INSERT's `EoDbGroup` (not a separate group).
- **INSERT import** (`ConvertInsertEntity`): Creates `EoDbBlockReference` + captures the group via `m_currentInsertGroup`. Subsequent `AddAttrib` calls add to that group before `AddToDocument`.
- **Interactive attribute prompting** (`EoDlgBlockInsert::OnOK`): When inserting a block with ATTDEFs, iterates `EoDbBlock::AttributeDefinitions()`, skips constant (flag 2) and invisible (flag 1), uses default without prompting for preset (flag 8), otherwise shows `EoDlgAttributePrompt` for each tag. `CreateAttribFromAttDef` builds the reference system from ATTDEF properties in block space, then calls `attrib->Transform(insertTransform)` to position in WCS. Handle linking (`SetInsertHandle`/`AddAttributeHandle`) matches the DXF import pattern.
- **DXF export**: INSERT → ATTRIB → SEQEND sequence written by `EoDbBlockReference::ExportToDxf`.
- **Explode**: `EoDbGroup::ExplodeBlockReferences` converts `EoDbAttrib` → `EoDbText` (attribute identity discarded).

### ATTRIB Flag Semantics (DXF group code 70)
| Bit | Meaning | Prompting behavior |
|-----|---------|-------------------|
| 1 | Invisible | Skip — not rendered, not prompted |
| 2 | Constant | Skip — value is fixed in block definition |
| 4 | Verify | Prompt and re-prompt until confirmed (currently treated same as normal) |
| 8 | Preset | Use default value silently without prompting |

### Structural Constraints
- ATTRIBs have no structural link in PEG V1 — written as `kTextPrimitive`, tag name and block association are lost on V1 save. V2 extension (`kAttribPrimitive`) preserves `m_tagString`, `m_attributeFlags`, `m_insertHandle`.
- Interactively-created ATTRIBs (`CreateAttribFromAttDef`) do not call `SetBaseProperties` — layer name is empty (group is on the work layer). DXF export of those ATTRIBs will write an empty layer string. This is consistent with the parent `EoDbBlockReference` behavior.
- `EoDbBlock::HasAttributes()` (flag bit 1) is preserved from DXF block flags; not yet used during rendering or editing.

### Deferred
- **Edit attributes command**: select INSERT, enumerate `m_attributeHandles` via `FindPrimitiveByHandle`, prompt for new values per tag.
- **PEG V2 load-time handle resolution**: after reading kAttribPrimitive, resolve `m_insertHandle` → `EoDbBlockReference*` pointer.
- **Layer propagation for interactive ATTRIBs**: inherit INSERT's layer name in `CreateAttribFromAttDef`.
- **ATTDEF `Verify` flag**: re-prompt loop until user confirms the entered value.

### Key Files
| File | Role |
|------|------|
| `AeSys\EoDbBlock.h/.cpp` | Block definition; `m_attributeDefinitions`; `BasePoint()`, `HasAttributes()`, `AttributeDefinitions()` |
| `AeSys\EoDbBlockReference.h/.cpp` | INSERT primitive; `BuildTransformMatrix(basePoint)`; `AddAttributeHandle`; `ExportToDxf` writes SEQEND |
| `AeSys\EoDbAttrib.h/.cpp` | ATTRIB primitive; `m_tagString`, `m_attributeFlags`, `m_insertHandle`; V1 writes as kTextPrimitive |
| `AeSys\EoDlgBlockInsert.cpp` | Interactive insertion; `CreateAttribFromAttDef` helper; prompting loop in `OnOK` |
| `AeSys\EoDlgAttributePrompt.h/.cpp` | Single-attribute prompt dialog (IDD=343); inputs: blockName/tagName/promptString/defaultValue; output: enteredValue |
| `AeSys\EoDbDxfInterface.cpp` | `ConvertAttDefEntity`, `ConvertAttribEntity`, `ConvertInsertEntity`, `AddAttrib` |

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

## EoDbPrimitive Virtual Contract

### Non-Pure Virtuals with Base Implementations
Two methods provide common formatting that all derived classes share — derived overrides call the base first, then append type-specific content:

| Method | Base output | Derived contract |
|--------|-------------|------------------|
| `FormatExtra(CString& extra)` | `Handle;%I64X\tOwner;%I64X\tLayer;%s\tColor;%s\tLineType;%s` | Call base → `AppendFormat` type-specific `Name;Value\t` pairs → `extra += L'\t'` terminator |
| `AddReportToMessageList(const EoGePoint3d&)` | One message line: `Handle: %I64X  Owner: %I64X  Layer: %s  Color: %s  LineType: %s` | Call base → add type-label and geometry lines via `app.AddStringToMessageList` |

### FormatExtra Terminator Rule
`FillExtraList` in `EoDlgEditTrapCommandsQuery.cpp` parses `FormatExtra` output by alternating `Find(';')` / `Find('\t')`. The last pair **must** end with `\t` — if missing, `CString::Mid(offset, -1)` returns an empty string for the final value. Every `FormatExtra` override must end with `extra += L'\t'`. `EoDbConic` uses a single terminator placed **after** its subtype switch block, not inside each case.

### Pure Virtuals — Candidates for Future Relaxation
The following `= 0` virtuals have safe universal defaults and are strong candidates to become non-pure virtual:

| Method | Safe default | Rationale |
|--------|-------------|----------|
| `Identical(EoDbPrimitive*)` | `return false` | 8 of 11 derived classes already implement exactly this |
| `SelectUsingLine`, `SelectUsingPoint`, `SelectUsingRectangle`, `IsPointOnControlPoint` | `return false` | Display-only primitives (annotation, viewport overlays) need no selection |
| `FormatGeometry(CString&)` | no-op | Future non-geometric primitives have no geometry fields to report |
| `GetAllPoints(EoGePoint3dArray&)` | `points.SetSize(0)` | Semantically correct "no control points" default |

The following must remain `= 0`: `Display`, `Copy`, `Assign`, `Transform`/`Translate`/`TranslateUsingMask`, `Write` (both overloads), `Is`, `AddToTreeViewControl`, `GetExtents`, `IsInView`, `GoToNextControlPoint`/`GetControlPoint`/`SelectAtControlPoint`.

## Handle Architecture

### Overview
Every `EoDbPrimitive` carries a unique, non-zero `m_handle` (`std::uint64_t`) assigned via `EoDbHandleManager`. `AeSysDoc` holds one instance; seed is set from `$HANDSEED`.

### Three Handle Pipelines
| Pipeline | Handle source | Mechanism |
|----------|--------------|-----------|
| **DXF import** | Entity's DXF handle | `SetBaseProperties()` overwrites auto-assigned handle, then `AccommodateHandle()` advances counter. |
| **PEG V2 load** | Persisted handle | `SetHandle()` overwrites auto-assigned handle. |
| **Interactive / PEG V1** | Auto-assigned | Constructor's `AssignHandle()` persists. |

### Key Design Rules
- Copy constructor assigns a **fresh handle** (new identity). `operator=` preserves target's existing handle.
- `std::uint64_t` preferred over alias — handle arithmetic is intentional.
- Export: non-zero handles preserved; new handles allocated only for handle-zero entities. Owner handle derived from export context (block record / `*Model_Space` 0x1F / `*Paper_Space` 0x1E).
- Table object handles (Layer, Linetype, TextStyle, DimStyle, AppId, VPort, BlockRecord, BLOCK entity) all survive DXF round-trip.

### DXF Ownership Hierarchy (Hardcoded Infrastructure Handles)0x00  (root)
├── 0x01  BLOCK_RECORD table → block records (owner=1)
│   ├── 0x1E  *Paper_Space block record → paper-space entities (owner=1E)
│   ├── 0x1F  *Model_Space block record → model-space entities (owner=1F)
│   └── dynamic  named block records → block entities (owner=block record handle)
├── 0x02  LAYER table → layer entries (owner=2)
├── 0x03  STYLE table → text style entries (owner=3)
├── 0x05  LTYPE table → linetype entries (owner=5)
├── 0x08  VPORT table → vport entries (owner=8)
├── 0x09  APPID table → app entries (owner=9)
├── 0x0A  DIMSTYLE table → dimstyle entries (owner=A)
└── 0x0C  root dictionary
### Handle → Object Reverse Lookup Map
`AeSysDoc` maintains `std::unordered_map<std::uint64_t, HandleObject> m_handleMap` for O(1) lookup. `HandleObject` is `std::variant<EoDbPrimitive*, EoDbLayer*, EoDbLineType*, EoDbBlock*>` — covers all heap-allocated, pointer-stable handle-bearing types. Value-type entries (`EoDbTextStyle`, `EoDbDimStyle`, `EoDbAppIdEntry`) deferred until migrated to pointer-stable containers.

### Handle Map Invariants
- Registration is idempotent; soft-delete preserves registration; hard-delete requires `UnregisterHandle()`.
- `DeleteContents()` calls `m_handleMap.clear()` before destroying objects — individual unregistration not needed in teardown.
- `EoDbGroup.cpp` uses `AeSysDoc::GetDoc()` (static) to reach the handle map.

### Structural Links (Session-Only — PEG V2 Serialization Deferred)
- **ATTRIB→INSERT**: `EoDbBlockReference::m_attributeHandles` populated during DXF import. `m_currentInsertPrimitive` in `EoDbDxfInterface` tracks the active INSERT. Invisible/zero-height/empty ATTRIBs skipped (not linked). Resolvable via `FindPrimitiveByHandle()`.
- **MTEXT identity**: `EoDbText::m_mtextProperties` (`std::optional<EoDbMTextProperties>`) preserves attachment point, drawing direction, line spacing, rectangle width. `ExportToDxf` dispatches to `ExportAsMText()` when present. MTEXT group code 50 is in **radians** (TEXT is degrees).
- **OBJECTS section**: `HasUnsupportedObjects()` virtual gates `WriteObjects()` to emit imported objects on round-trip or hardcoded minimal dicts for new drawings. Eliminates duplicate-dictionary bug.

### Deferred
- Extension dictionaries (XDICT, ACAD_REACTORS): not needed for current linkage.
- PEG V2 handle serialization per primitive type.

## User-Specific Notes
- User may rename `EoDbLabeledLine` to `EoDbLabeledLine` in the future — it's a labeled line primitive, not a true DXF dimension.

## Pen Render Pipeline

### Architecture Overview
The pen render pipeline converts entity display properties (color, line type, line weight) into GDI pen resources for on-screen rendering. The pipeline has three layers:

1. **Property resolution** (`EoGsRenderState::SetPen`) — resolves ByLayer/ByBlock/ByLwDefault indirection
2. **GDI pen management** (`ManagePenResources`) — 8-slot LRU pen cache with `::CreatePen()`
3. **Linetype rendering** (`polyline::__End` / `polyline::__Display`) — AeSys's own dash pattern engine

### Line Weight Resolution Chain
`SetPen` (7-arg overload) resolves entity line weight to a concrete pixel width:

| Entity weight | Resolution | Source |
|--------------|------------|--------|
| Explicit (enum 0–23) | DXF code → mm → pixels | `LineWeightToDxfIndex` → `dxfCode * (0.01/25.4) * DPI` |
| `kLnWtByLayer` | → layer's `m_lineWeight` → resolve again | `EoDbPrimitive::LayerLineWeight()` |
| `kLnWtByLwDefault` | → `kLnWt025` (0.25 mm system default) | Hardcoded in `SetPen` |
| `kLnWtByBlock` | → `kLnWt025` (same as ByLwDefault) | Passes through ByLwDefault resolution |

- **Zoom-independent**: All line weights render at fixed screen pixel widths regardless of view scale. This matches AutoCAD's "Display Lineweight" model-space behavior.
- **DPI-aware**: Uses `GetDeviceCaps(LOGPIXELSX)` for the mm-to-pixel conversion.
- **Toggle**: `View > Pen Widths` (`AeSysView::PenWidthsOn()`) enables/disables width rendering. When off, all pens render at 1px (GDI default width 0).

### Layer Display Properties
`EoDbLayer` carries display properties set before rendering its groups:

| Property | Member | Static resolver | Set in `Display()` |
|----------|--------|----------------|-------------------|
| Color | `m_color` | `EoDbPrimitive::sm_layerColor` | `SetLayerColor()` |
| LineType index | via `m_lineType->Index()` | `sm_layerLineTypeIndex` | `SetLayerLineTypeIndex()` |
| LineType name | via `m_lineType->Name()` | `sm_layerLineTypeName` | `SetLayerLineTypeName()` |
| Line weight | `m_lineWeight` | `sm_layerLineWeight` | `SetLayerLineWeight()` |
| LineType scale | `m_lineTypeScale` | `sm_layerLineTypeScale` | `SetLayerLineTypeScale()` |

Layer line weight defaults to `kLnWtByLwDefault`. DXF import propagates `EoDxfLayer::m_lineweightEnumValue` (group code 370). DXF export writes it back via `EoDxfLineWeights::LineWeightToDxfIndex()`. PEG V2 serializes it as int16 DXF code after layer handle/ownerHandle.

### GDI Pen Style Mapping (`ManagePenResources`)
`ManagePenResources` maps the internal `lineTypeIndex` to a GDI pen style:

| Index | GDI Style | Used in practice? |
|-------|-----------|------------------|
| 0 | `PS_NULL` | ✅ Invisible entities |
| 2 | `PS_DOT` | ❌ Dead code for entities — AeSys renders its own patterns |
| 3 | `PS_DASH` | ❌ Dead code |
| 6 | `PS_DASHDOT` | ❌ Dead code |
| 7 | `PS_DASHDOTDOT` | ❌ Dead code |
| default | `PS_SOLID` | ✅ All visible entity rendering |

**Why the non-SOLID GDI styles are dead code**: Entity linetype rendering is handled by `EoGsVertexBuffer::DisplayDashPattern()`, which reads dash/gap patterns from `EoDbLineType` objects and draws individual line segments with a PS_SOLID pen. The `End` function temporarily switches to `PS_SOLID` (via `renderState.SetLineType(deviceContext, 1)`) before calling `DisplayDashPattern`, then restores the original lineTypeIndex. The GDI PS_DOT/PS_DASH patterns in `ManagePenResources` are never visible during entity display.

### Non-Entity Pen Usage (Bypasses `ManagePenResources`)
These UI elements create their own `CPen` objects directly:

| Element | Pen style | Location |
|---------|-----------|----------|
| Rubberband lines | `PS_SOLID` + `Eo::colorRubberband` | `AeSysView::OnMouseMove` |
| Rubberband rectangles | `PS_SOLID` + `Eo::colorRubberband` | `AeSysView::OnMouseMove` |
| Viewport boundary | `PS_DOT` + entity color | `EoDbViewport::Display` |
| Point circles | `PS_SOLID` + entity color | `EoDbPoint::Display` |

### Legacy Color-Based Weight Table
`penWidths[16]` in `AeSys.cpp` (accessed via `app.LineWeight(colorIndex)`) is a legacy relic from the 4-pen plotter era. It maps 16 color indices to pen widths in inches (4 distinct values: 0.0, 0.0075, 0.015, 0.02, 0.03). This table is **no longer used** by the entity display pipeline — all line weight rendering now uses the DXF line weight enum resolution chain. The table remains for potential future plotter/print output use.

### Key Files
| File | Role |
|------|------|
| `EoGsRenderState.cpp` | `SetPen` (resolution + width calc), `ManagePenResources` (GDI pen cache) |
| `EoGsRenderState.h` | Render state members (`m_color`, `m_LineTypeIndex`, `m_lineTypeName`, `m_lineTypeScale`) |
| `EoDbLayer.h/.cpp` | Layer display properties (`m_lineWeight`, `m_lineTypeScale`), static setter calls in `Display()` |
| `EoDbPrimitive.h/.cpp` | Static layer resolution members (`sm_layerLineWeight`, `sm_layerLineTypeScale`) |
| `EoGePolyline.h/.cpp` | `polyline::` thin wrappers delegating to global `EoGsVertexBuffer` instance |
| `EoDxfLineWeights.h` | `LineWeight` enum, `LineWeightToDxfIndex`/`DxfIndexToLineWeight` converters |

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
- ARC/CIRCLE export performs WCS→OCS reverse transform for the center point when extrusion is non-default.

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
- `Display()` → `polyline::BeginLineLoop/Strip` → emit vertices with `polyline::SetVertex` → `polyline::End`.
- Bulge-aware: each edge tessellated via `TessellateArcSegment`. Closing segment for closed polylines respects its bulge.
- Selection/extents use `BuildTessellatedPoints()` (private helper) which produces a flattened point array with arcs expanded.
- **Width rendering**: `DisplayWidthFill()` renders per-segment filled trapezoids (GDI `Polygon`) with linearly interpolated widths. Arc segments are tessellated first, then each sub-segment gets a radially-offset quad. Called from `Display()` when width data is present.

### Known Limitations and Deferred Work
| Item | Notes |
|------|-------|
| Non-uniform scale + bulge | Bulge is dimensionless; non-uniform BLOCK INSERT scales distort arcs (matches AutoCAD behavior) |
| Break bulge arcs | Decomposing individual bulge arcs to `EoDbConic` for editing is deferred |

### Test Files
| File | Contents |
|------|----------|
| `DXF Test Files/LWPolyline_Bulge_Test.dxf` | 21 LWPOLYLINE cases: open/closed, positive/negative/mixed bulge, semicircle, closed diamond |
| `DXF Test Files/Heavy_Polyline_Subtypes.dxf` | 13 heavy POLYLINE cases: 3D open/closed, 2D basic/closed/elevation, bulge (±/mixed/closed), per-vertex width, default width, bulge+width, plinegen (open/closed) |
| `DXF Test Files/GenerateHeavyPolylineTest.ps1` | PowerShell generator for `Heavy_Polyline_Subtypes.dxf` using StringBuilder + helper functions |

## EoDbSpline ↔ DXF SPLINE Mapping

### Internal Representation
- `EoDbSpline` stores: `m_pts` (control points), `m_degree` (int16, default 3), `m_flags` (int16, DXF flag bits), `m_knots` (vector\<double\>), `m_weights` (vector\<double\>).
- `Display()` calls `GenPts(m_degree + 1, m_pts)` — degree-aware rendering.
- `GenPts` implements Cox–de Boor B-spline tessellation. Currently always regenerates a uniform knot vector at render time (stored knots used only for export fidelity).

### DXF → EoDbSpline Import
| DXF Property | Group Codes | Preserved | Notes |
|---|---|---|---|
| Control points | 10/20/30 | ✅ | Stored directly (no OCS transform — splines are WCS) |
| Fit points (fallback) | 11/21/31 | ✅ as control points | Used only when no control points |
| Degree | 71 | ✅ | Stored in `m_degree` |
| Knot vector | 40 | ✅ | Stored in `m_knots` (used on export; display uses uniform) |
| Weights (NURBS) | 41 | ✅ | Stored in `m_weights` when rational flag set |
| Flags | 70 | ✅ | Stored in `m_flags` (closed/periodic/rational/planar/linear) |
| Start/end tangents | 12-13/22-23/32-33 | ❌ | Ignored |

### DXF Export
- `ExportToDxf()` populates `EoDxfSpline` from stored members: degree, flags, knots, weights, control points.
- Stored knot vector and weights round-trip through DXF correctly.

### PEG Serialization
- **V1** (AE2011): `kSplinePrimitive → color → lineTypeIndex → uint16(pointCount) → points[]` — degree defaults to 3 on reload.
- **V2** (AE2026): Adds `flags(uint16) → degree(int16) → numKnots(uint16) → numControlPoints(uint16) → knots[] → weights[] (if rational) → controlPoints[]`.

### GenPts Tessellation Algorithm
- Cox–de Boor B-spline recursion with `order = degree + 1`.
- Tessellation density: `8 × numberOfControlPoints` segments.
- When `order > numberOfControlPoints`, falls back to a single line segment.

### Known Deferred Work
- `SelectUsingRectangle` tests the raw control polygon, not the tessellated curve.
- `GetControlPoint()`, `GoToNextControlPoint()`, `IsInView()` lack empty-array guards.
- Display uses uniform knots regardless of stored knots — NURBS rendering fidelity is deferred.
