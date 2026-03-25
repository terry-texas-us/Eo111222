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
- Use `%I64X` (not `%llX`) for `std::uint64_t` in `CString::Format` — this is the MSVC-traditional specifier and is consistent throughout the codebase.

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

### Phase 1 Complete ✅ — Make Internal Handles Consistently Valid (Foundation)
Every `EoDbPrimitive` now carries a unique, non-zero `m_handle` regardless of origin:

| Sub-item | What changed | Key files |
|----------|-------------|-----------|
| **P1.1 — Auto-assign in constructors** | `EoDbPrimitive` holds a `static EoDbHandleManager*` (`sm_handleManager`), wired by `AeSysDoc` constructor and reset in `DeleteContents()`. All three base constructors (default, copy, parameterized) call `AssignHandle()` when the manager is set. | `EoDbPrimitive.h/.cpp`, `AeSysDoc.cpp`, `EoDbHandleManager.h` |
| **P1.2 — Copy = fresh handle** | Copy constructor assigns a fresh handle (new entity identity). `operator=` preserves the target's existing handle. `EoDbViewport::swap()` skips `m_handle`. | `EoDbPrimitive.h/.cpp`, `EoDbViewport.cpp` |
| **P1.3 — AccommodateHandle on import** | `SetBaseProperties()` calls `AccommodateHandle(m_handle)` after importing the DXF entity's handle, advancing the counter past any handle above `$HANDSEED`. Defensive against malformed DXF files. | `EoDbPrimitive.cpp` |
| **P1.4 — V1 PEG migration** | Automatically solved by P1.1 — V1 entities get auto-assigned handles through the base constructor on load. |  |

### Three Handle Pipelines
| Pipeline | Handle source | Mechanism |
|----------|--------------|-----------|
| **DXF import** | Entity's DXF handle | `SetBaseProperties()` overwrites the auto-assigned handle with the DXF value, then `AccommodateHandle()` advances the counter. |
| **PEG V2 load** | Persisted handle | `SetHandle()` overwrites the auto-assigned handle with the stored value. |
| **Interactive / PEG V1** | Auto-assigned | Constructor's `AssignHandle()` value persists — no overwrite. |

### Implementation Details
- `EoDbPrimitive::m_handle` / `m_ownerHandle` — `std::uint64_t`, zero-initialized (zero = no handle).
- `EoDbHandleManager` (`EoDbHandleManager.h`) — `AssignHandle()` increments and returns `m_nextHandle`; `AccommodateHandle(h)` advances the counter past any pre-existing handle; `Reset()` reinitializes to 1. `AeSysDoc` holds one instance; its seed is set from the DXF `$HANDSEED` header variable.
- `SetBaseProperties(EoDxfGraphic*, AeSysDoc*)` propagates `m_handle`, `m_ownerHandle`, layer name, linetype, and thickness from the parsed DXF entity into the primitive, then accommodates the imported handle.

### Type Decision
`std::uint64_t` is preferred over a `using EoDbHandle = std::uint64_t` alias. Handle arithmetic (increment, comparison, hex formatting) is intentional — the transparent type correctly communicates that handles are integers. A strong-typedef wrapper would require `explicit` construction and operator overloads with no practical benefit in this codebase.

### Phase 2 Complete ✅ — DXF Export Handle Unification
Entity handles survive the import→export round-trip, and the writer's handle counter is seeded from the application's handle manager:

| Sub-item | What changed | Key files |
|----------|-------------|-----------|
| **P2.1 — Handle preservation in `WriteEntity()`** | Non-zero entity handles are preserved on export; new handles allocated only for `NoHandle` entities. Counter advances past preserved handles to prevent collisions. | `EoDxfWrite.cpp` |
| **P2.2 — Type widening + seed initialization** | `m_entityCount` widened from `int` to `std::uint64_t`. `m_blockMap` value type widened to `std::uint64_t`. `FIRSTHANDLE` typed as `std::uint64_t`. Counter initialized from `max(FIRSTHANDLE, interface->GetHandleSeed())`. | `EoDxfWrite.h`, `EoDxfWrite.cpp` |
| **P2.3 — `$HANDSEED` in header** | `WriteHeader()` appends `$HANDSEED` from `HandleManager().NextHandleValue()` after populating all header variables. `GetHandleSeed()` virtual added to `EoDxfInterface`. | `EoDbDxfInterface.h`, `EoDxfInterface.h` |

### Phase 3.1 Complete ✅ — Owner Handles on Export
Every AC1015+ entity now gets a DXF-compliant owner handle derived from the export context:

| Sub-item | What changed | Key files |
|----------|-------------|-----------|
| **P3.1a — Owner derivation in `WriteEntity()`** | Owner handle is always derived from export context instead of relying on `m_ownerHandle`: block entity → `m_currentHandle` (block record); paper-space entity → `0x1E` (`*Paper_Space` block record); model-space entity → `0x1F` (`*Model_Space` block record). | `EoDxfWrite.cpp` |
| **P3.1b — Table owner bug fixes** | Fixed 4 incorrect hardcoded owner handles: `WriteTextstyle` `"2"→"3"` (STYLE table), `WriteVport` `"2"→"8"` (VPORT table), `*Paper_Space` BLOCK `"1B"→"1E"` (block record), `*Paper_Space` ENDBLK `"1F"→"1E"` (block record). | `EoDxfWriteTables.cpp`, `EoDxfWrite.cpp` |

### DXF Ownership Hierarchy (Hardcoded Infrastructure Handles)
```
0x00  (root)
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
```

### Phase 3.2+3.3 Complete ✅ — Table Object Handle Preservation
Imported table entry handles now survive the DXF round-trip. Two-part fix: propagate handles from internal objects into DXF structures (`EoDbDxfInterface.h`), then preserve non-zero handles in the writer (`EoDxfWriteTables.cpp`) using the same conditional pattern as `WriteEntity()`.

| Sub-item | What changed | Key files |
|----------|-------------|-----------|
| **P3.2 — Layer handle propagation** | `WriteLayers` lambda sets `dxfLayer.m_handle = layer->Handle()`. `WriteLayer` preserves non-zero handle; layer `"0"` always gets hardcoded `0x10`. | `EoDbDxfInterface.h`, `EoDxfWriteTables.cpp` |
| **P3.3a — Linetype handle propagation** | `WriteLTypes` sets `dxfLinetype.m_handle = lineType->Handle()`. `WriteLinetype` preserves non-zero handle (skips ByLayer/ByBlock/Continuous which have hardcoded handles). | `EoDbDxfInterface.h`, `EoDxfWriteTables.cpp` |
| **P3.3b — TextStyle handle propagation** | `WriteTextstyles` sets `dxfTextStyle.m_handle = entry.m_handle`. `WriteTextstyle` preserves non-zero handle. | `EoDbDxfInterface.h`, `EoDxfWriteTables.cpp` |
| **P3.3c — DimStyle handle propagation** | `WriteDimstyles` sets `dxfDimStyle.m_handle = entry.m_handle`. `WriteDimStyle` preserves non-zero handle via group code `105` (not `5`). | `EoDbDxfInterface.h`, `EoDxfWriteTables.cpp` |
| **P3.3d — AppId + VPort handle preservation** | `WriteAppId` and `WriteVport` also preserve non-zero handles for completeness (no import propagation yet — VPorts lack internal handle members). | `EoDxfWriteTables.cpp` |

All four internal table object types (`EoDbLayer`, `EoDbLineType`, `EoDbTextStyle`, `EoDbDimStyle`) already had `m_handle`/`m_ownerHandle` members with getters/setters and import-side propagation from prior work — the gap was entirely on the export path.

### Phase 3.4 Complete ✅ — Block Record Handle Preservation
Imported block record and BLOCK entity handles now survive the DXF round-trip. `WriteBlockRecord` accepts an optional handle parameter and uses the same preservation pattern as `WriteEntity()` and the table write functions. `WriteBlock` preserves the BLOCK entity handle when available; ENDBLK retains the record+2 convention.

| Sub-item | What changed | Key files |
|----------|-------------|-----------|
| **P3.4a — Block record handle propagation** | `WriteBlockRecords` passes `block->OwnerHandle()` (= imported BLOCK_RECORD handle). `WriteBlockRecord` preserves non-zero handle; otherwise allocates sequentially. Stored in `m_blockMap` for entity ownership derivation. | `EoDbDxfInterface.h`, `EoDxfWrite.h`, `EoDxfWrite.cpp` |
| **P3.4b — BLOCK entity handle propagation** | `WriteBlocks` sets `dxfBlock.m_handle = block->Handle()`. `WriteBlock` preserves non-zero BLOCK entity handle; otherwise falls back to record+1 convention. ENDBLK stays at record+2 (handle not stored separately in `EoDbBlock`). | `EoDbDxfInterface.h`, `EoDxfWrite.cpp` |

The complete BLOCK_RECORD → BLOCK → entities → ENDBLK handle chain is now preserved on round-trip: block record handle feeds into `m_blockMap` → `m_currentHandle` → entity owner handles (via `WriteEntity` P3.1), BLOCK entity handle preserved directly, ENDBLK derived as record+2.

### Phase 4 Complete ✅ — Handle → Object Reverse Lookup Map
`AeSysDoc` maintains a non-owning `std::unordered_map<std::uint64_t, HandleObject> m_handleMap` for O(1) handle-to-object lookup across both spaces, the block table, layer tables, and linetype table. `HandleObject` is `std::variant<EoDbPrimitive*, EoDbLayer*, EoDbLineType*, EoDbBlock*>`, covering all heap-allocated, pointer-stable handle-bearing types.

| Sub-item | What changed | Key files |
|----------|-------------|-----------|
| **P4.1 — Map infrastructure** | `m_handleMap` member + 5 methods: `RegisterHandle`, `UnregisterHandle`, `FindPrimitiveByHandle`, `RegisterGroupHandles`, `UnregisterGroupHandles`. `DeleteContents()` bulk-clears with `m_handleMap.clear()`. | `AeSysDoc.h`, `AeSysDoc.cpp` |
| **P4.1 — Registration at import/load** | `RegisterHandle(primitive)` called in `AddToDocument` (DXF import), `ReadEntitiesSection`/`ReadPaperSpaceSection`/`ReadBlocksSection` (PEG load). | `EoDbDxfInterface.cpp`, `EoDbPegFile.cpp` |
| **P4.1 — Registration at interactive add** | `RegisterGroupHandles(group)` called in `AddWorkLayerGroup` and `AddWorkLayerGroups` — covers paste, undelete, expand, compress, text add. | `AeSysDoc.cpp` |
| **P4.1 — Unregistration at hard-delete** | `UnregisterGroupHandles` called in `DeleteAllTrappedGroups`, `InitializeWorkLayer`, `RemoveLayerTableLayerAt`. | `EoDbTrappedGroups.cpp`, `AeSysDoc.cpp` |
| **P4.2 — Mutation consistency** | All primitive-destroying mutation paths now maintain the map: `OnPrimBreak` (unregister old + register new), `BreakPolylines`/`ExplodeBlockReferences` (via `AeSysDoc::GetDoc()`), `RemoveEmptyNotesAndDelete`/`RemoveDuplicatePrimitives` (unregister before delete), `RemoveUnusedBlocks`/`InsertBlock` (unregister before block destroy), `OnClearActiveLayers`/`OnClearAllLayers`/`OnClearAllTracings` (unregister before `DeleteGroupsAndRemoveAll`). | `AeSysDoc.cpp`, `EoDbGroup.cpp`, `EoDbPegBlockTable.cpp`, `AeSysDoc.h` |
| **P4.3 — Full object graph** | Map widened from `EoDbPrimitive*` to `HandleObject` variant. 4 `RegisterHandle` overloads (Primitive, Layer, LineType, Block). `FindObjectByHandle` returns `const HandleObject*`; typed finders `FindPrimitiveByHandle`, `FindLayerByHandle`, `FindLineTypeByHandle`, `FindBlockByHandle` extract via `std::get_if`. Registration at all creation points: `AddLayerTableLayer`/`AddLayerToSpace` (layers), `ConvertLinetypesTable`/PEG load (linetypes), `InsertBlock` (blocks). Unregistration at all destruction points: `RemoveAllLayerTableLayers`/`RemoveLayerTableLayerAt`/`RemoveEmptyLayers` (layers), `RemoveAllBlocks`/`RemoveUnusedBlocks`/`InsertBlock` replacement (blocks). | `AeSysDoc.h`, `AeSysDoc.cpp`, `EoDbDxfInterface.cpp`, `EoDbPegFile.cpp`, `EoDbPegBlockTable.cpp` |

### HandleObject Variant Type
- `using HandleObject = std::variant<EoDbPrimitive*, EoDbLayer*, EoDbLineType*, EoDbBlock*>` — defined before `AeSysDoc` class in `AeSysDoc.h`.
- Covers all **heap-allocated, pointer-stable** handle-bearing types. Value-type table entries (`EoDbTextStyle`, `EoDbDimStyle`, `EoDbAppIdEntry`) are deferred until migrated to pointer-stable containers — their `std::vector` storage invalidates pointers on reallocation.
- `EoDbVPortTableEntry` has no handle members and is excluded.

### Handle Map Invariants
- **Registration is idempotent**: re-registering the same handle overwrites with the same variant value.
- **Soft-delete preserves registration**: moving primitives to the deleted-groups list keeps pointers valid — no map update needed.
- **Hard-delete requires unregistration**: any path calling `delete` on a handle-bearing object must call `UnregisterHandle(object->Handle())` first.
- **`EoDbGroup.cpp` document access**: Group-level methods use `AeSysDoc::GetDoc()` (static) to reach the handle map.
- **Bulk clear covers teardown ordering**: `DeleteContents()` calls `m_handleMap.clear()` before destroying linetypes, blocks, and layers — so their destructors don't need individual unregistration.
- **Linetype unregistration**: No independent linetype removal path exists outside `DeleteContents` — `RemoveUnused()` is a no-op. The bulk clear covers all linetype handles.

### Next Phases (Deferred)
- **Phase 5 — Structural Links**: ~~ATTRIB→INSERT~~, ~~MTEXT identity preservation~~, ~~OBJECTS section round-trip~~.
- **Extension dictionaries** (XDICT, ACAD_REACTORS): not needed for current entity→layer/linetype linkage.
- **PEG V2 handle serialization**: handles will be written alongside entity records when V2 binary format is defined per primitive type.

### Phase 5.1 Complete ✅ — ATTRIB→INSERT Structural Linking
Each `EoDbBlockReference` now owns a list of ATTRIB primitive handles, populated during DXF import when `ProcessInsertAttribs` delivers ATTRIBs sequentially after the parent INSERT.

| Sub-item | What changed | Key files |
|----------|-------------|-----------|
| **P5.1a — Attribute handle storage** | `EoDbBlockReference` gains `std::vector<std::uint64_t> m_attributeHandles` with `AddAttributeHandle()`, `AttributeHandles()`, `ClearAttributeHandles()` API. Copy constructor and `operator=` propagate attribute handles. | `EoDbBlockReference.h`, `EoDbBlockReference.cpp` |
| **P5.1b — INSERT tracking state** | `EoDbDxfInterface` gains `EoDbBlockReference* m_currentInsertPrimitive{}` — set by `AddInsert`, consumed by `AddAttrib`. Mirrors the `m_currentOpenBlockDefinition` pattern for block definitions. | `EoDbDxfInterface.h` |
| **P5.1c — Return-value plumbing** | `ConvertInsertEntity` returns `EoDbBlockReference*` (was `void`). `ConvertAttribEntity` returns `EoDbText*` (was `void`), with early returns yielding `nullptr` for skipped ATTRIBs. | `EoDbDxfInterface.h`, `EoDbDxfInterface.cpp` |
| **P5.1d — Handle linking in AddAttrib** | `AddAttrib` body moved to `.cpp` (avoids circular include). After `ConvertAttribEntity` returns, appends `textPrimitive->Handle()` to `m_currentInsertPrimitive->AddAttributeHandle()` when both are non-null. | `EoDbDxfInterface.cpp` |
| **P5.1e — Reporting** | `FormatExtra` appends `Attributes;N` count field. `AddReportToMessageList` lists individual ATTRIB handles when present. | `EoDbBlockReference.cpp` |

#### Design Notes
- **Lifetime**: `m_currentInsertPrimitive` is set in `AddInsert` and consumed by subsequent `AddAttrib` calls. It is naturally superseded by the next `AddInsert` call. The DXF parser flow (`ProcessInsert` → `ProcessInsertAttribs` → SEQEND) guarantees ATTRIBs arrive immediately after their parent INSERT.
- **Skipped ATTRIBs**: Invisible attributes (flag bit 0), zero-height, and empty-value ATTRIBs are skipped by `ConvertAttribEntity` (returns `nullptr`). Their handles are NOT linked to the parent INSERT — they have no corresponding `EoDbText` primitive.
- **Handle resolution**: ATTRIB handles stored in `m_attributeHandles` can be resolved to `EoDbText*` via `AeSysDoc::FindPrimitiveByHandle()` (Phase 4 infrastructure).
- **PEG V2 serialization**: Attribute handle list is not yet persisted — deferred to PEG V2 per-primitive binary format definition. Currently survives only within a single session (DXF import → memory → DXF export is possible via handle lookup; PEG save/load loses the link).

### Phase 5.2 Complete ✅ — MTEXT Identity Preservation
Each `EoDbText` that originated from a DXF MTEXT entity now carries an `EoDbMTextProperties` struct preserving MTEXT-specific DXF properties. `ExportToDxf` dispatches to MTEXT export (instead of TEXT) when these properties are present, enabling correct DXF round-trip.

| Sub-item | What changed | Key files |
|----------|-------------|----------|
| **P5.2a — Property struct** | `EoDbMTextProperties` struct with 5 fields: `attachmentPoint`, `drawingDirection`, `lineSpacingStyle`, `lineSpacingFactor`, `referenceRectangleWidth`. `std::optional<EoDbMTextProperties> m_mtextProperties` member on `EoDbText`. API: `SetMTextProperties()`, `IsFromMText()`, `MTextProperties()`. | `EoDbText.h` |
| **P5.2b — Copy/assign propagation** | Copy constructor and `operator=` propagate `m_mtextProperties`. | `EoDbText.cpp` |
| **P5.2c — Import-side storage** | `ConvertMTextEntity` populates `EoDbMTextProperties` from the parsed `EoDxfMText` and calls `SetMTextProperties()` on the created `EoDbText`. | `EoDbDxfInterface.cpp` |
| **P5.2d — Export dispatch** | `ExportToDxf` checks `m_mtextProperties.has_value()` and delegates to `ExportAsMText()` which populates `EoDxfMText` from the reference system (height, rotation in radians, insertion point) plus stored MTEXT properties and calls `writer->AddMText()`. TEXT-origin primitives use the existing `AddText()` path. | `EoDbText.cpp` |
| **P5.2e — Reporting** | `FormatExtra` includes `Source;MTEXT` or `Source;TEXT` field. `AddReportToMessageList` shows `<Text (MTEXT)>` header and a detail line with attachment point, drawing direction, line spacing, and rectangle width. | `EoDbText.cpp` |

#### Design Notes
- **Single-primitive model**: MTEXT is stored as a single `EoDbText` with `\P` paragraph breaks preserved in the text string. The renderer `DisplayTextWithFormattingCharacters()` handles `\P` splitting at display time. This is more efficient than splitting into multiple primitives and naturally round-trips through the single-MTEXT export path.
- **DXF TEXT vs MTEXT export**: The presence or absence of `m_mtextProperties` is the sole discriminator. Text primitives created interactively or loaded from PEG files have `m_mtextProperties == std::nullopt` and export as TEXT. Text primitives created by `ConvertMTextEntity` have the struct populated and export as MTEXT.
- **Rotation angle units**: MTEXT group code 50 is in **radians** (unlike TEXT group code 50 which is in degrees). `ExportAsMText` passes `std::atan2` result directly without degree conversion.
- **PEG V2 serialization**: `m_mtextProperties` is not yet persisted — deferred to PEG V2 per-primitive binary format definition. Currently survives only within a single session (DXF import → memory → DXF export preserves MTEXT identity; PEG save/load loses it and re-exports as TEXT).

### Phase 5.3 Complete ✅ — OBJECTS Section Round-Trip
The OBJECTS section now survives DXF round-trip. All non-graphical objects (DICTIONARY, LAYOUT, PLOTSETTINGS, MLINESTYLE, MATERIAL, VISUALSTYLE, etc.) captured during import are written back on export, eliminating the duplicate-dictionary bug.

| Sub-item | What changed | Key files |
|----------|-------------|----------|
| **P5.3a — HasUnsupportedObjects() virtual** | `EoDxfInterface` gains `HasUnsupportedObjects()` virtual (default `false`). Provides the conditional gate for the writer to distinguish round-trip exports from new-drawing exports. | `EoDxfInterface.h` |
| **P5.3b — Interface override** | `EoDbDxfInterface` overrides `HasUnsupportedObjects()` to query `!m_document->UnsupportedObjects().empty()`. | `EoDbDxfInterface.h` |
| **P5.3c — Conditional WriteObjects()** | `EoDxfWrite::WriteObjects()` checks `HasUnsupportedObjects()`. When true: writes imported objects via `WriteUnsupportedObjects()` and returns (skips hardcoded dicts). When false: writes minimal hardcoded root dict C + ACAD_GROUP D + image definitions (existing behavior for new drawings). | `EoDxfWrite.cpp` |

#### Design Notes
- **Duplicate-dictionary bug**: Previously, `WriteObjects()` always wrote hardcoded root dict (handle C) and ACAD_GROUP (handle D), then `WriteUnsupportedObjects()` wrote all imported objects which included the original C and D — producing invalid DXF with duplicate dictionaries. The conditional branch eliminates this.
- **Image definitions**: When the imported-objects path is taken, `m_imageDef` is cleaned up but not written — AeSys does not accumulate image definitions during import (`LinkImage` is a no-op). Image data is preserved in the unsupported objects collection if it was in the source DXF.
- **Handle preservation**: OBJECTS section handles are within the `$HANDSEED` range from DXF import, so no handle collision risk. The unsupported objects store raw group code data including their original handles.
- **New-drawing fallback**: When `HasUnsupportedObjects()` returns false (new drawing, PEG import, or no OBJECTS section in source), the original hardcoded minimal-dictionary path runs unchanged.

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

**Why the non-SOLID GDI styles are dead code**: Entity linetype rendering is handled by `polyline::__Display()`, which reads dash/gap patterns from `EoDbLineType` objects and draws individual line segments with a PS_SOLID pen. The `__End` function temporarily switches to `PS_SOLID` (via `renderState.SetLineType(deviceContext, 1)`) before calling `__Display`, then restores the original lineTypeIndex. The GDI PS_DOT/PS_DASH patterns in `ManagePenResources` are never visible during entity display.

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
| `EoGePolyline.cpp` | `polyline::__End` (linetype dispatch), `polyline::__Display` (custom dash rendering) |
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

### On-Disk Format
The PEG V2 paper-space section is a first-class section positioned between the model-space entities and the "EOF" marker. V1 readers (which stop after `ReadEntitiesSection`) never read past the groups section's `kEndOfSection` sentinel, so the paper-space section is invisible to them.

```
[AE2011 (V1)]
  kHeaderSection (0x0101) → kEndOfSection
  kTablesSection (0x0102)
    kViewPortTable → 0 → kEndOfTable
    kLinetypeTable → count → entries → kEndOfTable
    kLayerTable → count → MODEL-SPACE layers → kEndOfTable
  kEndOfSection
  kBlocksSection (0x0103) → count → blocks → kEndOfSection
  kGroupsSection (0x0104) → count → MODEL-SPACE entities → kEndOfSection
  "EOF"

[AE2026 (V2) — with paper-space]
  kHeaderSection (0x0101) → variable triples ($AESVER, …) → kEndOfSection
  kTablesSection (0x0102)
    kViewPortTable → 0 → kEndOfTable
    kLinetypeTable → count → entries → kEndOfTable
    kLayerTable → count → MODEL-SPACE layers → kEndOfTable
  kEndOfSection
  kBlocksSection (0x0103) → count → blocks → kEndOfSection
  kGroupsSection (0x0104) → count → MODEL-SPACE entities → kEndOfSection
  kPaperSpaceSection (0x0105)
    kLayerTable → count → PAPER-SPACE layers → kEndOfTable
    kGroupsSection → count → PAPER-SPACE entities → kEndOfSection
  kEndOfSection
  "EOF"

[AE2026 (V2) — without paper-space]
  kHeaderSection (0x0101) → variable triples ($AESVER, …) → kEndOfSection
  kTablesSection (0x0102) → ... → kEndOfSection
  kBlocksSection (0x0103) → ... → kEndOfSection
  kGroupsSection (0x0104) → ... → kEndOfSection
  "EOF"
```

- **"EOF" marker**: Purely conventional — no reader depends on it. Kept as the very last thing in both versions for hex-dump readability.
- **Sentinel peek**: After `ReadEntitiesSection`, `ReadPaperSpaceSection` peeks at the next `uint16_t`. `kPaperSpaceSection` (0x0105) cannot collide with the "EOF" string (first two CP_ACP bytes 'E','O' → 0x4F45 little-endian).
- **Write path**: `WriteLayerTable` and `WriteEntitiesSection` explicitly use `SpaceLayers(ModelSpace)`. `WritePaperSpaceSection` writes the paper-space section only when `fileVersion == AE2026` and paper-space layers exist.
- **Read path**: `ReadPaperSpaceSection` peeks for `kPaperSpaceSection` before the "EOF" marker, reads paper-space layers/entities if present, then consumes the trailing "EOF" string.
- **Version gate**: `WritePaperSpaceSection` returns immediately for `AE2011` — paper-space data is silently dropped on downgrade save.
- **Sentinel constants**: `kPaperSpaceSection = 0x0105` in `EoDb::Sentinels`.

### DXF Layout Model vs PEG Paper-Space
DXF organizes layouts through `BLOCK_RECORD` entries + `*Model_Space`/`*Paper_Space` block definitions + `LAYOUT` objects in the OBJECTS section. Each entity's owner handle determines its space. PEG uses a simpler model: separate layer tables for model-space and paper-space, with the paper-space section containing its own layer table and entity groups.

### Key Files
| File | Role |
|------|------|
| `EoDbPegFile.cpp` | `Load`/`Unload` + `ReadPaperSpaceSection`/`WritePaperSpaceSection` |
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
