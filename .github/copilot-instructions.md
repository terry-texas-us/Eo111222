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

DXF reading via `EoDxfLib` is operational for core entity types (3DFACE, ARC, ATTRIB, CIRCLE, ELLIPSE, HATCH, MTEXT, INSERT, LINE, LWPOLYLINE, POLYLINE, SOLID, SPLINE, TEXT, TRACE, VIEWPORT), following the ezdxf architecture model. Parsed entities are converted to `EoDbPrimitive`-derived objects and stored in `AeSysDoc` layers. See `Peg & Tra File Formats.md` for the legacy file structure.

The AE2026 (V2) handle architecture is **implemented**: `EoDbPrimitive` carries `m_handle`/`m_ownerHandle` (`std::uint64_t`), assigned via `EoDbHandleManager`. Entity→layer/linetype handle linkage covers current import/export needs. Extension dictionaries are **deferred** — see the **Handle Architecture** section below.

## PEG File Compatibility
- **AE2011** (V1): The only backward-compatible format. All V1 read/write paths must be preserved.
- **AE2026** (V2): Experimental — writes are discarded after debug review until a milestone is reached. New fields can be added freely without versioning guards.

## General Guidelines
- Purpose: Native MFC/C++ CAD/graphics application (AeSys). Keep suggestions compatible with the existing MFC architecture and on-disk file formats.
- Architecture & patterns: MFC document/view pattern (classes: `AeSysDoc`, `AeSysView`) – suggestions should maintain MFC idioms where applicable.
- Geometric primitives implement a common base (`EoDbPrimitive`) with virtuals for drawing, selection, transform, serialization – preserve virtual contract and ABI when changing signatures.
- Use `Peg & Tra File Formats.md` to help understand the legacy file structure.
- **vcxproj explicit listing**: `AeSys.vcxproj` no longer uses a wildcard `<ClCompile Include="*.cpp" ...>`. All `.cpp` files are now listed explicitly in alphabetical order under the `<!-- Everything else in the folder -->` comment, except for the three special-case entries (`Stdafx.cpp` with Create PCH, `lex.yy.cpp` with NotUsing PCH + Level3, `MainFrm.cpp` with Use PCH). When adding a new `.cpp` file, insert it in the correct alphabetical position in that explicit list. Do NOT use or restore a wildcard include for ClCompile.

## Code Style, Linters & Formatting
- Repository contains `.clang-format` and `.clang-tidy` at root – prefer those settings for formatting and static-analysis suggestions.
- Minimize raw `new`/`delete`.
- Be conservative in migration from `CString` to `std::wstring` – prefer consistent conversions and avoid unnecessary copies. Prefer `std::wstring` as the only internal text API and avoid `std::string` except at unavoidable external byte boundaries.
- Step away from MFC `CObject`; minimize dynamic runtime features; avoid file serialization.
- Prefer camelCase for local variables.
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

### Implemented
- **ATTDEF storage**
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

## Multiple Paper-Space Layouts

### Architecture Overview
AeSys supports multiple paper-space layouts, matching the DXF LAYOUT object model. Each layout has its own `CLayers` collection, keyed by the layout's `BLOCK_RECORD` handle. The default layout uses `PaperSpaceBlockRecord` (0x1E).

### Data Model
`AeSysDoc` stores paper-space layers in a per-layout map:std::unordered_map<std::uint64_t, CLayers> m_paperSpaceLayoutLayers{}
std::uint64_t m_activeLayoutHandle{EoDxf::Handles::PaperSpaceBlockRecord}
The map key is the `BLOCK_RECORD` handle associated with the layout (`EoDxfLayout::m_blockRecordHandle`). `operator[]` auto-creates an empty `CLayers` on first access, so import/creation code does not need explicit initialization.

### DXF Layout Discrimination
DXF entities carry two space indicators:
- **Group code 67** (`EoDxfGraphic::m_space`): Binary `ModelSpace`/`PaperSpace` — insufficient for multi-layout.
- **Group code 330** (`EoDxfGraphic::m_ownerHandle`): Owner handle pointing to the `BLOCK_RECORD` for the entity's layout — this is the discriminator.

The chain is: Entity `ownerHandle` → `BLOCK_RECORD` handle → `EoDxfLayout::m_blockRecordHandle`.

### Accessor Hierarchy
| Method | Returns | Notes |
|--------|---------|-------|
| `PaperSpaceLayers()` | `CLayers&` for active layout | Uses `m_paperSpaceLayoutLayers[m_activeLayoutHandle]` |
| `ActiveSpaceLayers()` | Model or active layout's `CLayers&` | Delegates through `m_activeSpace` |
| `SpaceLayers(space)` | Model or active layout's `CLayers&` | PaperSpace routes through active layout |
| `LayoutLayers(handle)` | `CLayers&` for specific layout | Direct map access by block record handle |
| `PaperSpaceLayoutLayers()` | Full map reference | For iteration (cleanup, export, PEG persistence) |
| `AddLayerToLayout(layer, handle)` | void | Adds layer to specific layout |
| `FindLayerInLayout(name, handle)` | `EoDbLayer*` | Searches specific layout |
| `ActiveLayoutHandle()` | `std::uint64_t` | Current active layout's block record handle |
| `SetActiveLayoutHandle(handle)` | void | Switches active layout |

### Cleanup
`RemoveAllLayerTableLayers()` iterates all map entries, clearing and deleting each layout's layers, then clears the map and resets `m_activeLayoutHandle` to `PaperSpaceBlockRecord`. `AnyLayerRemove()` searches model space then all paper-space layouts.

All five implementation phases are complete (data model, import routing, PEG V2 persistence, DXF export/rendering, layout tab UI).

### PEG V2 Persistence
`kLayoutTable` (0x0205) serializes `EoDxfLayout` fields. `kMultiLayoutPaperSpaceSection` (0x0106) replaces legacy `kPaperSpaceSection` (0x0105); `ReadPaperSpaceSection` handles both for backward compatibility.

### Key Files
| File | Role |
|------|------|
| `AeSys\AeSysDoc.h` | `m_paperSpaceLayoutLayers` map, `m_activeLayoutHandle`, all accessor declarations |
| `AeSys\AeSysDocLayers.cpp` | Accessor implementations, cleanup, layout-specific `AddLayerToLayout`/`FindLayerInLayout` |
| `AeSys\AeSysDocDisplay.cpp` | `DisplayModelSpaceThroughViewports`, `CreateDefaultPaperSpaceViewport` — use `PaperSpaceLayers()` |
| `AeSys\EoDbPegFile.cpp` | `ReadLayoutTable`/`WriteLayoutTable`, paper-space section read/write |
| `EoDxfLib\EoDxfBase.h` | `PaperSpaceBlockRecord = 0x1E` — default layout handle |
| `EoDxfLib\EoDxfObjects.h` | `EoDxfLayout` — `m_blockRecordHandle` links layout to block record |
| `AeSys\EoMfLayoutTabBar.h/.cpp` | Owner-draw tab bar for layout switching — `PopulateFromDocument`, themed via `Eo::chromeColors` |
| `AeSys\AeSysView.cpp` | `OnCreate` creates tab bar; `UpdateLayoutTabs`, `OnLayoutTabSelChange` handle population and selection |

## Plot Pipeline — Unified Layout Rendering (DXF == PEG)

### Architecture Overview
Both DXF and PEG files use the same paper-space → viewport → model-space rendering pipeline for plotting. DXF files bring their own paper-space viewports from the file. PEG files get a synthetic default viewport created by `CreateDefaultPaperSpaceViewport()` on first view initialization.

The key principle is **"DXF == PEG for plot render"**: plot output always renders through the paper-space layout pipeline, regardless of file origin. This eliminates separate code paths for DXF (which has native layouts) and PEG (which historically had none).

### Paper-Space Rendering Pipeline
`DisplayAllLayers()` checks `m_activeSpace`:
- **ModelSpace**: Iterates model-space layers, renders geometry directly.
- **PaperSpace**: Iterates paper-space layers (rendering annotations, title blocks, viewport frames), then calls `DisplayModelSpaceThroughViewports()`.

`DisplayModelSpaceThroughViewports()`:
1. Scans paper-space layers for `EoDbViewport` primitives (skips viewport ID 1 — the overall paper boundary).
2. For each viewport (ID ≥ 2): computes a GDI clip rect from paper-space geometry, pushes a view transform, configures camera/window from viewport's model-space view parameters (viewCenter, viewDirection, viewHeight, etc.), renders all model-space layers within the clip, then restores.
3. Off-center projection math: `halfExtentU = viewWidth × deviceWidth / (2 × clipWidth)`, `windowCenterU = halfExtentU × (1 − 2×clipCenterX/deviceWidth)` — maps model-space view area exactly to the GDI clip region.

### Default Paper-Space Viewport (PEG Files)
`AeSysDoc::CreateDefaultPaperSpaceViewport(AeSysView* view)` is called from `OnInitialUpdate()` after `ApplyActiveViewport()`. It:
1. Scans paper-space layers for existing viewports (ID > 1 with viewHeight > tolerance) — exits early if found (idempotent for DXF files).
2. Computes model-space extents via `layer->GetExtents(view, ...)`.
3. Creates `EoDbViewport` ID 2 with paper-space sheet sized 1:1 to model extents + 5% margin.
4. Configures viewport model-space view: viewCenter at model centroid, viewHeight = model height, viewDirection = (0,0,1) top view.
5. Places the viewport group on paper-space layer "0".
6. Handle auto-assigned by `EoDbPrimitive` constructor, registered via `RegisterHandle()`.

### EoDbViewport Primitive Fields
| Category | Fields | Purpose |
|----------|--------|---------|
| Paper-space geometry | `centerPoint`, `width`, `height` | Position/size of viewport rectangle on the paper sheet |
| Identity | `viewportStatus`, `viewportId` | Status flags; ID 1 = overall paper (skipped), ID ≥ 2 = model windows |
| Model-space view | `viewCenter`, `viewDirection`, `viewTargetPoint`, `viewHeight`, `lensLength`, `twistAngle` | Camera configuration for model-space rendering inside viewport |

### Plot Command Flow
1. `OnFilePlotFull/Half/Quarter()` → sets `m_Plot = true`, `m_PlotScaleFactor`, calls `CView::OnFilePrint()`.
2. `OnBeginPrinting()` → saves `m_activeSpace` to `m_savedActiveSpaceForPlot` (unconditionally — handles edge case of non-plot print from PaperSpace), switches to `PaperSpace` when `m_Plot`, caches paper-space extents in `m_plotExtentMin`/`m_plotExtentMax`, computes page count via `NumPages()`.
3. Per-page: `OnPrepareDC()` → tiles across paper-space sheet using cached extents and `m_PlotScaleFactor`.
4. Per-page: `OnDraw()` → `DisplayAllLayers()` with PaperSpace active → renders paper-space layers → `DisplayModelSpaceThroughViewports()` renders model-space through each viewport.
5. `OnEndPrinting()` → restores saved active space, resets `m_Plot`.

### MFC Print Lifecycle Timing
`OnPreparePrinting` → `OnBeginPrinting` → per-page(`OnPrepareDC` → `OnDraw`) → `OnEndPrinting`.

The space switch happens in `OnBeginPrinting`, so `OnPreparePrinting`'s initial page count estimate uses pre-switch extents. This is acceptable — `OnBeginPrinting` overrides with the correct count. Paper-space extents are ~5% larger than model-space (due to the margin) — the difference is negligible for initial estimates.

### Key Design Decisions
- Active space is always saved/restored in `OnBeginPrinting`/`OnEndPrinting` (not just for plot) to handle non-plot print from PaperSpace correctly.
- `OnFilePrint()` (non-plot) renders whatever the current active space is — no space switching.
- `CreateDefaultPaperSpaceViewport` is idempotent — safe to call on every `OnInitialUpdate`.
- Tiling math in `OnPrepareDC` is space-agnostic — works against whatever extents are cached in `m_plotExtentMin`/`m_plotExtentMax`.

### Key Files
| File | Role |
|------|------|
| `AeSys\AeSysDocDisplay.cpp` | `CreateDefaultPaperSpaceViewport`, `DisplayModelSpaceThroughViewports`, `DisplayAllLayers` |
| `AeSys\AeSysDoc.h` | `SetActiveSpace()`, `ActiveSpace()`, `m_activeSpace`, `m_paperSpaceLayers` |
| `AeSys\AeSysViewRender.cpp` | Print lifecycle (`OnBeginPrinting`/`OnPrepareDC`/`OnDraw`/`OnEndPrinting`), `OnInitialUpdate` |
| `AeSys\AeSysView.h` | `m_savedActiveSpaceForPlot`, `m_plotExtentMin/Max`, `m_Plot`, `m_PlotScaleFactor` |
| `AeSys\AeSysViewCommands.cpp` | `NumPages()`, `OnFilePlotFull/Half/Quarter()` plot command handlers |
| `AeSys\EoDbViewport.h` | Viewport primitive — paper-space geometry + model-space view params |

### Deferred
- **Multi-viewport layouts**: Current PEG default creates a single full-sheet viewport. Future work could support multiple viewports with different scales/views.
- **Paper-space annotations**: Title blocks, borders, dimension text in paper-space layers are rendered but not yet created programmatically for PEG files.
- **Named layouts**: DXF supports multiple named layouts; currently only the default `*Paper_Space` is used.
- **Viewport clipping in D2D**: ✅ Resolved — `DisplayModelSpaceThroughViewports` uses `PushClipRect`/`PopClipRect` which maps to `PushAxisAlignedClip`/`PopAxisAlignedClip` in the D2D backend.

## Chrome / Color Scheme

AeSys uses a single **light warm chrome** — three-tier custom palette layered over
CMFCVisualManagerOffice2007. There is no dark scheme. `Eo::chromeColors` holds the
28 named `COLORREF` fields used by `EoMfVisualManager` overrides and all chrome
drawing call sites. To adjust a color, edit the `inline constexpr` values in `Eo.h`.

## Status Bar

### Layout
`EoMfStatusBar m_statusBar` in `CMainFrame` with 16 panes defined in the `indicators[]` array:

| Index | ID | Width | Purpose |
|-------|-----|-------|---------|
| 0 | `ID_SEPARATOR` | 288px fixed | Message pane (~36 characters) |
| 1 | `ID_INDICATOR_LENGTH` | 120px fixed | Dimension length display |
| 2 | `ID_INDICATOR_ANGLE` | 100px fixed | Dimension angle display |
| 3–12 | `ID_OP0`–`ID_OP9` | Dynamic (text extent) | Mode key-command help panes |
| 13 | `ID_INDICATOR_SCALE` | 120px fixed | World Scale display (1:nnn.nn) |
| 14 | `ID_INDICATOR_ZOOM` | 100px fixed | Zoom ratio display (nnn.nnn) |
| 15 | `ID_SEPARATOR` | `SBPS_STRETCH` | Trailing stretch filler — absorbs remaining space |

The `statusInfo` constant (= 0), `statusLength` (= 1), `statusAngle` (= 2), `statusScale` (= 13), `statusZoom` (= 14) are in `MainFrm.cpp`. `statusOp0` constant (= 3, in `ModeLine.cpp`) indexes the first mode pane.

The trailing stretch filler (index 15) must be **after** all fixed panes — `CMFCStatusBar` only lays out non-stretch panes correctly when the stretch pane doesn't precede them.

The size gripper is removed (`SBARS_SIZEGRIP` stripped via `ModifyStyle` after creation) — modern apps allow resizing from any window edge/corner.

### Length and Angle Display
`UpdateStateInformation()` in `AeSysViewRender.cpp` routes `DimLen`/`DimAng` changes to both the view overlay (existing) and status bar panes 1 and 2. The view overlay is retained for testing while the status bar display is validated.

### Mode Pane Highlighting
Active mode key panes use **accent blue background** (`captionActiveBackground`) with white text (`captionActiveText`) instead of the legacy red text approach:
- `ModeLineHighlightOp(command)` → `SetPaneBackgroundColor(index, captionActiveBackground)` + `SetPaneTextColor(index, captionActiveText)`
- `ModeLineUnhighlightOp(command)` → `SetPaneBackgroundColor(index)` (reset) + `SetPaneTextColor(index, statusBarText)`
- `ModeLineDisplay()` loops all 10 panes, applying highlight or normal state per-pane.

**Visual Manager constraint**: `EoMfVisualManager::OnDrawStatusBarPaneBorder` is a **no-op**. MFC's `DoPaint` paints per-pane `clrBackground` (from `SetPaneBackgroundColor`) *before* calling `OnDrawStatusBarPaneBorder`. Any fill in the border override would erase the per-pane background, breaking accent blue highlighting. The base status bar color is painted by `OnFillBarBackground`.

### Text Color on Scheme Switch
`CMainFrame::ApplyColorScheme()` iterates all status bar panes and sets `SetPaneTextColor(i, statusBarText)` to ensure text switches from light to dark (or vice versa) when the scheme changes.

### Length Pane Edit-in-Place
`EoMfStatusBar` (custom `CMFCStatusBar` subclass) provides interactive edit-in-place for the Length pane (index 1):
- **Double-click** on the Length pane creates an inline `CEdit` control positioned over the pane, pre-filled with the current formatted dimension length.
- **Enter** commits: parses via `app.ParseLength(app.GetUnits(), buffer)`, applies via `app.SetDimensionLength()`, then calls `UpdateStateInformation(DimLen)` to refresh both the status bar pane and the view overlay.
- **Escape** or **focus loss** cancels without changing the value.
- The `CEdit` control (ID 1001) is created once and reused; font matches the status bar font.
- This supplements (does not replace) the existing `.` accelerator → `EoDlgSetLength` dialog path.

### Angle Pane Edit-in-Place
Double-clicking the Angle pane (index 2) shows the same shared `CEdit` control for direct angle entry in degrees:
- Pre-filled with the current dimension angle formatted as `%.3f` degrees.
- **Enter** commits: parses via `_wtof()`, clamps to [-360, 360], applies via `app.SetDimensionAngle()`, then calls `UpdateStateInformation(DimAng)`.
- **Escape** or **focus loss** cancels.
- Supplements (does not replace) the existing `EoDlgSetAngle` dialog path.

### World Scale Pane Edit-in-Place
Double-clicking the World Scale pane (index 13) shows the shared `CEdit` control for direct scale entry:
- Pre-filled with the current world scale formatted as `%.2f`.
- **Enter** commits: parses via `_wtof()`, validates > 0, applies via `AeSysView::SetWorldScale()`, then calls `UpdateStateInformation(Scale)` and updates the Properties pane `kActiveViewScale` property.
- **Escape** or **focus loss** cancels.
- The `Scale` flag in `UpdateStateInformation` routes the formatted value to status bar pane 13.

### Zoom Ratio Pane Edit-in-Place
Double-clicking the Zoom Ratio pane (index 14) shows the shared `CEdit` control for direct zoom entry:
- Pre-filled with the current zoom ratio formatted as `%.3f`.
- **Enter** commits: parses and applies the new zoom ratio to the active view.
- **Escape** or **focus loss** cancels.
- The `WndRatio` flag in `UpdateStateInformation` routes the formatted value to status bar pane 14.

### View Overlay Migration Status

| Overlay Item | Flag | Target | Status |
|---|---|---|---|
| Pen Color | `Pen` | Toolbar color combo | ✅ Implemented |
| Dim Length | `DimLen` | Status bar pane 1 | ✅ Implemented |
| Dim Angle | `DimAng` | Status bar pane 2 | ✅ Implemented |
| World Scale | `Scale` | Status bar pane 13 | ✅ Implemented |
| Zoom Ratio | `WndRatio` | Status bar pane 14 | ✅ Implemented |
| Line Type | `Line` | Properties toolbar combo | Deferred |
| Text Height | `TextHeight` | Properties toolbar or pane | Deferred |
| Work Count | `WorkCount` | Status bar message pane | Deferred |
| Trap Count | `TrapCount` | Status bar message pane | Deferred |

### Key Files
| File | Role |
|------|------|
| `AeSys\EoMfStatusBar.h/.cpp` | Custom status bar — edit-in-place for Length (1), Angle (2), World Scale (13), Zoom Ratio (14); `WM_LBUTTONDBLCLK` hit-test, `PreTranslateMessage` for Enter/Escape |
| `AeSys\MainFrm.cpp` | `indicators[]`, status bar creation, `ApplyColorScheme()` text color loop, `SetPaneBackgroundColor` wrapper |
| `AeSys\MainFrm.h` | `EoMfStatusBar m_statusBar`, pane API wrappers |
| `AeSys\ModeLine.cpp` | `statusOp0` constant, `ModeLineDisplay`, `ModeLineHighlightOp`, `ModeLineUnhighlightOp` |
| `AeSys\AeSysViewRender.cpp` | `UpdateStateInformation` routes DimLen/DimAng to panes 1/2, Scale to pane 13, WndRatio to pane 14 |

### Deferred
- **DPI-aware fixed widths**: The 288px/120px/100px/120px/100px pane widths could be DPI-scaled.
- **Zoom-extent buttons**: Quick-action buttons (Zoom Extents, Zoom In, Zoom Out) adjacent to the Zoom Ratio pane.

## Toolbar Controls

### Standard Toolbar (`IDR_MAINFRAME_256`)
File (New, Open, Save, Save All) | Edit (Cut, Copy, Paste) | Print, About, Help | Find combo (`EoCtrlFindComboBox`)

### Render Properties Toolbar (`IDR_RENDER_PROPERTIES`)
A separate dockable toolbar for primitive render-state properties. Isolates combo serialization from the standard toolbar, limiting the blast radius of any registry corruption.

Contains three combos: **Color** (`EoCtrlColorComboBox`), **Line Type** (`EoCtrlLineTypeComboBox`), **Line Weight** (`EoCtrlLineWeightComboBox`). Future additions: **Line Style** (dash pattern selection).

`IDR_RENDER_PROPERTIES` (190) is a three-button toolbar resource. `OnToolbarReset` replaces each placeholder with its combo class via `ReplaceButton()`. The toolbar has its own serialization stream — the standard toolbar no longer carries combo state.

### Color Combo (`EoCtrlColorComboBox`)
Owner-draw ACI color selection combo embedded in the render properties toolbar. Dark-theme-aware with color swatches, named colors (1–7), By Layer, By Block, dynamic custom entries (ACI 8–255), and "More Colors..." fallback to `EoDlgSetupColor`. Custom `Serialize` override (`VERSIONABLE_SCHEMA | 2`) bypasses `CMFCToolBarComboBoxButton::Serialize` to avoid DWORD_PTR truncation and item duplication on toolbar state restore. See `Documentation/MFC Custom Color Selection Control.md` for full architecture.

### Line Type Combo (`EoCtrlLineTypeComboBox`)
Owner-draw line type selection combo. Follows the same `CMFCToolBarComboBoxButton` subclass + `CComboBox` owner-draw subclass pattern as Color Combo. Dark-theme-aware. Custom `Serialize` override (`VERSIONABLE_SCHEMA`) bypasses base serialization.

### Line Weight Combo (`EoCtrlLineWeightComboBox`)
Owner-draw DXF line weight selection combo embedded in the render properties toolbar. Follows the same `CMFCToolBarComboBoxButton` subclass + `CComboBox` owner-draw subclass pattern as Color and Line Type combos.

**Items**: 3 special entries (ByLayer, ByBlock, Default) + 24 standard DXF line weights (0.00–2.11 mm) with `"X.XX mm"` labels. Items carry `DWORD_PTR` data set to `EoDxfLineWeights::LineWeight` enum values.

**Owner-draw preview**: Each item renders a thickness-proportional line preview alongside the text label. `DrawWeightPreview()` computes pixel thickness as `max(1, MulDiv(dxfCode, dpi, 96 * 30))` — produces 1–8px across the 0.00–2.11 mm range. Special values (ByLayer/ByBlock/Default) draw as 1px.

**Serialization**: `VERSIONABLE_SCHEMA | 1`. Custom `Serialize` override saves/restores a single `int32_t` weight enum value; items are rebuilt from `BuildItemList()` on load (not serialized individually).

**Render state integration**: `OnSelectionChanged()` → `renderState.SetLineWeight(newWeight)`. `EoGsRenderState::m_lineWeight` stores the **unresolved** enum value (ByLayer/ByBlock/ByLwDefault/concrete) **before** the ByLayer→concrete resolution chain in `SetPen`, so the UI can read back the user's setting.

**Sync chain**: `UpdateStateInformation(Pen)` → `CMainFrame::SyncLineWeightCombo(weight)` → `GetCommandButtons(ID_LINEWEIGHT_COMBO)` → `DYNAMIC_DOWNCAST(EoCtrlLineWeightComboBox>` → `SetCurrentLineWeight(weight)` → `SelectItem(DWORD_PTR(enum))`.

**ModifyState**: `EoDbPrimitive::ModifyState()` copies `renderState.LineWeight()` into `m_lineWeight` alongside color and lineTypeName, completing the property trio.

**WithProperties**: `EoDbPrimitive::WithProperties(color, lineTypeName, lineWeight)` — the fluent creation API used by all interactive draw modes. All three render properties are explicit parameters — no hidden reads from `renderState`. Call sites fall into three categories:
- **Interactive creation** (majority): `->WithProperties(renderState.Color(), renderState.LineTypeName(), renderState.LineWeight())`
- **Primitive copy** (explode, cut, split): `->WithProperties(source->Color(), source->LineTypeName(), source->LineWeight())`
- **Legacy file import** (PEG V1): `->WithProperties(color, lineTypeName, EoDxfLineWeights::LineWeight::kLnWtByLwDefault)` (format has no lineWeight)

### Key Files
| File | Role |
|------|------|
| `EoCtrlColorComboBox.h/.cpp` | Color combo control implementation |
| `EoCtrlLineTypeComboBox.h/.cpp` | Line type combo control implementation |
| `EoCtrlLineWeightComboBox.h/.cpp` | Line weight combo control implementation |
| `EoGsRenderState.h/.cpp` | Render state management for line weight |
| `MainFrm.cpp` | Toolbar setup and synchronization logic |

### Toolbar Button Sizing — MFC Render Path

Understanding the two separate height pipelines is essential for any future sizing work.

#### Icon Button Pipeline (`CMFCToolBar`)
1. `LoadToolBar(id, ..., bLocked=TRUE)` — sets `m_bLocked=TRUE`, stores the resource-derived button size in `m_sizeButtonLocked` via `SetLockedSizes()`.
2. `GetButtonSize()` — returns **`m_sizeButtonLocked`** when `m_bLocked=TRUE` (not `m_sizeButton`). Calling `SetSizes()` only updates `m_sizeButton` — **silently ignored for locked toolbars**.
3. `AdjustLocations()` — divides the band into equal button rects using `m_sizeButtonLocked.cy` as the row height.
4. `EoMfVisualManager::OnFillButtonInterior()` — fills the button rect (inset 1px chrome). At a 41 px band the visual background appears ~36 px due to the 2–3 px top/bottom chrome inset drawn by the default visual manager.

**Rule**: For locked toolbars always call `EoMfStatelessToolBar::SetSizesAll(buttonSize, imageSize)`, which calls both `SetSizes()` (updates `m_sizeButton`) and `SetLockedSizes(buttonSize, imageSize, TRUE)` (updates `m_sizeButtonLocked`, `bDontAdjust=TRUE` defers `AdjustLayout` to the caller).

`LoadBitmap(bLocked=TRUE)` resets `m_sizeButtonLocked` from the bitmap dimensions — `AdjustToolbarSizesToMatchCombos()` must be called **after** every `LoadBitmap` path to re-apply the desired sizes.

#### Combo Button Pipeline (`CMFCToolBarComboBoxButton`)
1. `OnCalculateSize()` — returns `max(GetButtonSize().cy, m_nComboHeight + 2 * m_nVertMargin)`. `m_nVertMargin = 4` is **hardcoded** in `CMFCToolBarComboBoxButton`.
2. `OnMove()` — positions the combo HWND via `m_rect.DeflateRect(0, m_nVertMargin)`. With a 41 px band and `m_nVertMargin=4`, the HWND height = 41 − 8 = **33 px**, which renders as ~32 px visible.

#### Why a ~4 px Gap Remains
At 100 % DPI with the equalized 41 px band:
- Icon button visual background ≈ **36 px** (41 px minus ~2–3 px top/bottom chrome).
- Combo HWND = 41 − 2 × 4 = **33 px** ≈ 32 px visible.

Closing this gap would require subclassing `CMFCToolBarComboBoxButton` to reduce `m_nVertMargin` (or override `OnCalculateSize`/`OnMove`). That is a **regression risk** — not pursued. The current 1 px visual offset is accepted.

#### `AdjustToolbarSizesToMatchCombos()` — Guard Logic
Measures the `ID_PENCOLOR_COMBO` HWND closed height at runtime, computes `targetHeight = max(32, comboHeight + 2 * 4)`, then calls `SetSizesAll(CSize(targetHeight, targetHeight), CSize(24, 24))` on all three toolbars (`m_standardToolBar`, `m_renderPropertiesToolBar`, `m_stylesToolBar`) followed by `RecalcLayout()`. Call sites:
- End of `OnCreate` — after `ApplyColorScheme()` which calls `LoadBitmap`.
- End of `ApplyColorScheme()` — after all `LoadBitmap` calls (scheme switch resets `m_sizeButtonLocked`).
- Inside `LoadFrame()` — after `EnsureToolbarsVisible()`, guards against stale docking-state restore.

### Known Issues — Registry Serialization

#### Button-State Blobs (Suppressed)
`CMFCToolBar::SaveState`/`LoadState` write opaque binary blobs to `HKCU\SOFTWARE\Engineers Office\AeSys`. Each blob encodes button images, combo item lists, and custom button state. Any mismatch between saved and runtime state (schema change, DPI change, metric override, `ReplaceButton` order change) causes cascading corruption: lost icons, text-only labels, duplicated combo items.

`EoMfStatelessToolBar` suppresses this by overriding both methods to return `TRUE` without performing any registry I/O. Buttons are rebuilt from resources and `OnToolbarReset` on every launch.

#### Docking Layout (Still Serialized)
CDockingManager docking position (which bar, which side, float vs docked) **is still serialized** separately by MFC — `EoMfStatelessToolBar` does not suppress this. If a stale docking blob hides a toolbar after upgrade:
- `EnsureToolbarsVisible()` in `LoadFrame()` iterates all toolbars and calls `ShowControlBar(..., TRUE)` as a safety net.
- `AdjustToolbarSizesToMatchCombos()` follows immediately to re-apply sizes after any layout restore.

#### Per-Control Custom `Serialize` Overrides
`EoCtrlColorComboBox`, `EoCtrlLineTypeComboBox`, `EoCtrlLineWeightComboBox`, and `EoCtrlTextStyleComboBox` each carry a `VERSIONABLE_SCHEMA` `Serialize` override that bypasses `CMFCToolBarComboBoxButton::Serialize` entirely. This prevents `DWORD_PTR` truncation (32-bit read of 64-bit item data) and item duplication on toolbar state restore. Items are always rebuilt from `BuildItemList()` at runtime — never read from the registry blob.

This is the least-bad mitigation available without abandoning `CMFCToolBarComboBoxButton` as the base class. The user considers MFC's black-box toolbar state persistence a fundamental limitation.

## Paper-Space Sheet Background

### Architecture Overview
`DisplayPaperSpaceSheet()` renders a visual paper sheet and viewport boundary outlines behind paper-space entity content. It is called from `DisplayAllLayers()` before paper-space layer rendering. The paper-space view uses a gray "table" background (`Eo::PaperSpaceBackgroundColor()`) with a white sheet rectangle on top, matching the industry-standard AutoCAD convention.

### Color Scheme
| Element | Color | Notes |
|---------|-------|-------|
| Table background (dark view) | `RGB(96, 95, 90)` | Warm dark gray — `Eo::PaperSpaceBackgroundColor()` |
| Table background (light view) | `RGB(192, 191, 187)` | Warm light gray — `Eo::PaperSpaceBackgroundColor()` |
| Sheet fill | `RGB(255, 255, 255)` | Always white — `Eo::PaperSpaceSheetColor()` |
| Sheet border | `RGB(100, 99, 94)` | Dark warm gray — visible against both table and sheet |
| Drop shadow | `RGB(64, 63, 58)` | Offset +4px right/down behind the sheet |
| Viewport borders (inactive) | `RGB(140, 140, 134)` | Medium warm gray on the white sheet |
| Viewport border (active) | `Eo::chromeColors.captionActiveBackground` | 2px accent blue (VS blue) |

### Sheet Bounds Discovery
Sheet bounds are determined in priority order:
1. **Layout extents** (`EoDxfLayout::m_limminX/Y`, `m_limmaxX/Y`) from the active paper-space layout — used when the DXF file provides layout limits.
2. **Viewport bounding box** — union of all viewport rectangles (center ± halfWidth/halfHeight) with a 2% margin — fallback for PEG files and layouts without explicit limits.

If no valid bounds can be determined (degenerate area < `Eo::geometricTolerance`), the function returns early without drawing.

### Rendering
- **Drop shadow**: Dark gray filled polygon offset +4px right and down from the sheet corners (device coordinates).
- **Sheet rectangle**: White fill with dark warm gray border — sits on the gray table.
- **Viewport boundaries**: Gray outlines for each viewport with ID ≥ 2 and valid dimensions. Active viewport uses accent blue border at 2px width.

### ACI 7 in Paper Space
The ACI 7 → black swap remains correct: entities render on the **white sheet**, so ACI 7 must be black for visibility. The gray table background does not affect entity rendering.

### ViewportInfo
A local `ViewportInfo` struct gathers viewport data during the layer scan:
```cpp
struct ViewportInfo { EoGePoint3d center; double halfWidth; double halfHeight; EoDbViewport* primitive; };
```
The `primitive` pointer enables the rendering loop to identify the active viewport for accent highlighting.

## Paper-Space Viewport Interaction

All paper-space viewport interaction features are fully implemented.

### Viewport Activation
Double-click inside a viewport in paper space activates it (accent blue border, 2px); double-click elsewhere deactivates. `AeSysView::m_activeViewportPrimitive` (`EoDbViewport*`) tracks the active viewport (session-only). `AeSysDoc::HitTestViewport(worldPoint)` performs point-in-rectangle containment. `OnLayoutTabChange()` calls `DeactivateViewport()` before switching spaces.

### Coordinate Transform Routing
`ConfigureViewportTransform(const EoDbViewport*)` pushes the paper-space view transform and configures an off-center projection matching `DisplayModelSpaceThroughViewports` math exactly. `GetCursorPosition()`, `SetCursorPosition()`, and `RubberBandingStartAtEnable()` each call configure→transform→restore, enabling model-space cursor coordinates, rubber-banding, and odometer readout when a viewport is active.

### Paper-Space Dimming
`DimPaperSpaceOverlay()` fills a semi-transparent overlay (~40% opacity) over everything except the active viewport's rectangle. D2D path: `ID2D1PathGeometry` with `D2D1_COMBINE_MODE_EXCLUDE`. GDI path: `ExcludeClipRect` + `GdiAlphaBlend`. No-op when no viewport is active. Called last in `DisplayAllLayers()` (after `DisplayModelSpaceThroughViewports`).

### Entity Routing
`SetActiveViewportPrimitive()` saves the current work layer and switches `m_workLayer` to model-space layer "0". `DeactivateViewport()` restores it. `AddWorkLayerGroup()` is the sole routing control point — all draw commands route to model space automatically with no command-level changes.

### Viewport Pan / Zoom
Middle-button drag in `OnMouseMove` converts pixel delta to model-space delta using projected clip dimensions and updates `viewport->SetViewCenter()`. `OnWindowZoomIn`/`OnWindowZoomOut` scale `viewHeight` by 0.9/1.111, anchoring the cursor point via `GetCursorPosition`/`SetCursorPosition`. `m_displayLocked` (`EoDbViewport`) blocks pan and zoom but not entity creation.

### Layout Tab Bar Controls
`EoMfLayoutTabBar` hosts three right-aligned controls: **Space Label** (IDC_LAYOUT_SPACE_LABEL 1490) — shows MODEL/PAPER, toggles active space on click; **Scale Combo** (IDC_VIEWPORT_SCALE_COMBO 1491) — preset ratios 1:1…1:100 plus 2:1/5:1/10:1, computed as `viewport->Height()/viewport->ViewHeight()`, custom shown when no preset matches; **Lock Button** (IDC_VIEWPORT_LOCK_BUTTON 1492) — toggles `m_displayLocked`. `UpdateViewportState()` is called from `SetActiveViewportPrimitive`, `DeactivateViewport`, and `OnLayoutTabChange`. `RepositionControls()` is called from `OnSize`.

### Key Files
| File | Role |
|------|------|
| `AeSys\AeSysView.h` | `m_activeViewportPrimitive`, `m_savedWorkLayerForViewport`, accessors |
| `AeSys\AeSysViewInput.cpp` | `OnLButtonDblClk`, `SetActiveViewportPrimitive`, `DeactivateViewport`, `ConfigureViewportTransform`, coordinate method branches |
| `AeSys\AeSysViewCamera.cpp` | `OnWindowZoomIn`/`OnWindowZoomOut` viewport branches |
| `AeSys\AeSysDocDisplay.cpp` | `DisplayPaperSpaceSheet`, `DisplayModelSpaceThroughViewports`, `DimPaperSpaceOverlay`, `HitTestViewport` |
| `AeSys\EoMfLayoutTabBar.h/.cpp` | Tab bar state controls — creation, `UpdateViewportState`, `RepositionControls`, scale presets |
| `AeSys\EoDbViewport.h` | `m_displayLocked`, `SetViewCenter`, `SetViewHeight` |

### Deferred
- Escape key deactivation of active viewport.
- DPI-aware scale combo width (currently 80px fixed).
- Scale combo direct text entry.

## Isolating Editor Pattern (Block Editor → Layer/Tracing Editor)

### Architecture Overview
The Block Editor is the first implementation of an **isolating editor** pattern — a mode that temporarily replaces the document's visible content with an isolated subset for focused editing. The same pattern applies to future Layer (Tracing) editing and any `EoDbGroupList`-derived collection.

### How the Block Editor Works
1. **Enter** (`AeSysDoc::EnterBlockEditMode`): Saves current state, creates a temporary `*BlockEdit` layer, deep-copies the target block's `EoDbGroupList` onto it, switches `m_activeSpace` so only the edit layer is visible.
2. **Edit**: All standard draw/modify commands work — they operate on the edit layer via the normal `AddWorkLayerGroup`/trap pipeline. The user sees only the block's content in isolation.
3. **Save** (`OnToolsSaveBlockEdit`): Deep-copies the edit layer content back into the source `EoDbBlock`'s group list, replacing the original.
4. **Exit** (`OnToolsCancelBlockEdit`): Deletes the edit layer and its content, restores saved state, returns to normal view.

### Extension Pattern for Layer/Tracing Editor
Any `EoDbGroupList`-derived container (layer, tracing, named group collection) can follow the same pattern:

| Step | Block Editor | Layer Editor (future) |
|------|-------------|----------------------|
| Source | `EoDbBlock::m_groupList` | `EoDbLayer::GetGroupList()` |
| Enter | Deep-copy block → edit layer | Deep-copy layer → edit layer (or edit in-place with isolation) |
| Isolation | Hide all layers, show only `*BlockEdit` | Hide all layers except target, or deep-copy to temp |
| Save | Copy edit content → block's group list | Copy back or commit in-place |
| Exit | Delete edit layer, restore state | Restore visibility, clean up |

### Key Design Constraints
- **Edit layer setup**: New `EoDbLayer` instances default to `m_lineType = nullptr`, which resolves to linetype index 0 (`Null` → `PS_NULL` invisible pen). Always call `SetLineType(m_continuousLineType)` and `SetColorIndex(7)` on new edit layers — follow the `SetCommonTableEntries()` pattern for layer "0".
- **State save/restore**: The enter method must save and restore: active space, work layer, view transform, layout tab state. Use `AeSysView` members for view-local state.
- **Tab bar integration**: `EoMfLayoutTabBar::UpdateBlockEditState(true, name)` switches to editor mode UI (shows Save/SaveAs/Close buttons, hides normal tabs/controls). `UpdateBlockEditState(false)` restores.
- **Handle management**: Deep-copied primitives get fresh handles from `EoDbHandleManager`. The edit layer's content is ephemeral — handles are not persisted.

### Key Files
| File | Role |
|------|------|
| `AeSys\AeSysDoc.h/.cpp` | `EnterBlockEditMode`, `OnToolsSaveBlockEdit`, `OnToolsCancelBlockEdit`, `m_blockEditLayer`, `m_blockEditBlock` |
| `AeSys\EoDbBlock.h/.cpp` | Block definition container with `m_groupList` (`EoDbGroupList`) |
| `AeSys\EoDbGroupList.h/.cpp` | Group list base — `DeepCopy()`, iteration, entity management |
| `AeSys\EoMfLayoutTabBar.cpp` | `UpdateBlockEditState` — editor UI state; owner-draw buttons |

## Owner-Draw Controls in Chrome Areas

### Problem
MFC standard controls (`CButton` with `BS_FLAT`, `BS_BITMAP`, etc.) use Windows system themes (rounded corners, 3D bevels, `COLOR_BTNFACE` background) that clash with AeSys's flat, sharp-cornered chrome style painted by `EoMfVisualManager`. This is visible when controls are placed on toolbars, tab bars, or other custom-painted surfaces.

### Solution Pattern
Use `BS_OWNERDRAW` buttons with parent-handled `WM_DRAWITEM` to replicate the toolbar hover style exactly:

1. **Create**: `CButton::Create(L"", WS_CHILD | BS_OWNERDRAW, ...)` — no `BS_FLAT`, no `BS_BITMAP`.
2. **Background**: Fill with the chrome surface color (e.g., `toolbarBackground` for tab areas) so the button blends seamlessly.
3. **Hover/Press**: `menuHighlightBackground` fill + 1px `menuHighlightBorder` sharp `Rectangle()` — matches `EoMfVisualManager::OnFillButtonInterior` + `OnDrawButtonBorder`.
4. **Content**: `BitBlt` the bitmap centered, or `DrawText` for text buttons.
5. **Hover tracking**: `PreTranslateMessage` intercepts `WM_MOUSEMOVE`/`WM_MOUSELEAVE` on button HWNDs, tracks `m_hoveredButton`, calls `InvalidateRect` to trigger redraws. The `TRACKMOUSEEVENT` with `TME_LEAVE` ensures the leave message is generated.
6. **Background brush**: `OnCtlColor` returns a brush matching the surface color for non-owner-draw child controls (combo boxes, etc.) on the same surface.
7. **Tooltips**: `CToolTipCtrl` with `TTS_ALWAYSTIP`, `AddTool` per button, `RelayEvent` in `PreTranslateMessage`.

### When to Use Owner-Draw
- Any `CButton` placed on a custom-painted surface (tab bar, toolbar area, docking pane caption).
- When the default Windows button theme (rounded corners, 3D bevel, system `COLOR_BTNFACE`) conflicts with the flat chrome style.
- When hover/press feedback needs to match `EoMfVisualManager`'s toolbar button style.

### Reference Implementation
`EoMfLayoutTabBar::OnDrawItem` and `PreTranslateMessage` in `EoMfLayoutTabBar.cpp` — block edit Save/SaveAs/Close buttons.

## Mode State Pattern

### Architecture Overview
`AeSysView` owns a `std::stack<std::unique_ptr<AeSysState>> m_stateStack`. Push/pop is always available — the stack starts empty and all dispatch points guard with `if (state != nullptr)`, so zero stack size is a valid idle condition.

`AeSysState` (`AeSysState.h`) is the abstract base with safe no-op defaults for all lifecycle and input virtuals. Derived mode states own per-command sub-state (previous op, point accumulator, geometry scratchpad). All 12 primary mode states are fully migrated.

The `ON_COMMAND_RANGE(ID_DRAW_MODE_OPTIONS, ID_DRAW_MODE_SHIFT_RETURN)` entry is **permanently removed** from the message map. Individual `ON_COMMAND` handlers remain.

### File Naming Convention
Mode state pairs follow the `EoMs<Mode>` prefix:

| File pair | Mode |
|-----------|------|
| `EoMsDraw.h/.cpp` | Draw |
| `EoMsDraw2.h/.cpp` | Draw2 (parallel wall) |
| `EoMsAnnotate.h/.cpp` | Annotate |
| `EoMsCut.h/.cpp` | Cut |
| `EoMsDimension.h/.cpp` | Dimension |
| `EoMsEdit.h/.cpp` | Edit |
| `EoMsFixup.h/.cpp` | Fixup |
| `EoMsLpd.h/.cpp` | Low-Pressure Duct |
| `EoMsNodal.h/.cpp` | Nodal |
| `EoMsPipe.h/.cpp` | Pipe |
| `EoMsPower.h/.cpp` | Power |
| `EoMsTrap.h/.cpp` | Trap |

Sub-mode states do not use the `EoMs` prefix: `PickAndDragState.h/.cpp`, `PrimitiveMendState.h/.cpp`.

### Dispatch Points (always-active, null-safe)
| Location | Dispatch |
|----------|---------|
| `AeSysViewInput.cpp::PreTranslateMessage` | `state->HandleKeypad(...)` |
| `AeSysViewInput.cpp::OnLButtonDown` | `state->OnLButtonDown(...)` — early return if state present |
| `AeSysViewInput.cpp::OnRButtonDown` | always calls `CView::OnRButtonDown` — no state dispatch; paired delivery required by Windows |
| `AeSysViewInput.cpp::OnRButtonUp` | `state->OnRButtonUp(...)` — early return if state present |
| `AeSysViewInput.cpp::OnMouseMove` | `state->OnMouseMove(...)` — falls through to existing mode switch |
| `AeSysViewRender.cpp::OnDraw` (back buffer) | `state->OnDraw(...)` — additive overlay after `DisplayAllLayers` |
| `AeSysViewRender.cpp::OnUpdate` | `state->OnUpdate(...)` — `isHandledByState` gate |
| `AeSysViewDrawMode.cpp::OnDrawCommand` | `state->HandleCommand(...)` when non-null |

### Mouse Routing Rules
- **LMB (`OnLButtonDown` / `OnLButtonUp`)**: When stack is non-empty the state's handler is called and the method returns immediately — the state owns all left-button input. `CView::OnLButtonUp` is suppressed when a state is active, matching the RMB pattern. The base `AeSysState::OnLButtonDown` calls `(void)HandleCommand(context, GetActiveOp())`, so most states need only override `GetActiveOp()` and `HandleCommand()`.
- **RMB (`OnRButtonDown` / `OnRButtonUp`)**: `OnRButtonDown` always calls `CView::OnRButtonDown` unconditionally — Windows needs the paired down/up delivery to maintain mouse-capture and `WM_CONTEXTMENU` generation state. No concrete state overrides `OnRButtonDown`; all RMB action happens in `OnRButtonUp`. `OnRButtonUp` **does** early-return when a state is present, suppressing `CView::OnRButtonUp` (which would generate `WM_CONTEXTMENU`). When a state is active, `OnRButtonUp` first calls `state->BuildContextMenu(this, menu)`; if it returns `true`, a `TrackPopupMenuEx(... | TPM_RETURNCMD | TPM_NONOTIFY)` is shown synchronously and the chosen command is dispatched via `SendMessage(WM_COMMAND, chosenCommand)`. The world-space cursor position at RMB-up is captured **before** the menu is shown and restored via `SetCursorPosition` **before** dispatch — commit handlers (Pipe/LPD/Draw2 return, Edit move/copy place, Trap stitch) read the intended click point, not the menu-item screen position. If `BuildContextMenu` returns `false`, the legacy `state->OnRButtonUp(...)` path runs (cancel/commit semantics per mode).
- **Context menu construction**: `AeSysState::BuildContextMenu` is a virtual returning `false` by default. Concrete states override to return a context-sensitive menu when a gesture is active and `false` when idle. Mid-gesture menus expose a single commit (e.g. `ID_*_MODE_RETURN/STITCH/SLICE/FIELD/CLIP`) plus `Cancel`. `EditModeState` and `TrapModeState` also expose an idle menu when the trap is non-empty (transform actions / trap actions). `DrawModeState`'s menu is point-count-aware: `MF_GRAYED` flags are computed from `m_points.GetSize()` per op (polygon ≥3 to close, ≥2 to finish as polyline; b-spline ≥2 to close; multi-click ops ≥2 to undo last). Menus are built programmatically with `CreatePopupMenu` + `AppendMenu`; the `IDR_CONTEXT_*` resource templates in `AeSys.rc` are reserved for future `LoadMenu` adoption but are not currently consumed.
- **Draw mode RMB (`OnDrawModeFinish`)**: `DrawModeState::OnRButtonUp` calls `context->OnDrawModeFinish()`. `OnDrawModeFinish` guards `previousDrawCommand == 0` with an explicit early return **before** entering the switch — the idle/no-gesture case does nothing. The `default` branch of the switch (non-closeable ops: line, arc, quad, circle, ellipse) is only reachable when a real op is active but lacks enough points to commit; it delegates to `OnDrawModeEscape()` as a cancel. Do not remove the idle guard — without it, RMB on an idle draw state would incorrectly trigger escape-path cleanup.
- **OnMouseMove**: State handler fires first but does not consume the event; the existing `app.CurrentMode()` switch follows. States that fully own preview (e.g., `PickAndDragState`, `PrimitiveMendState`) have their switch `case` **removed** to avoid double-handling.
- **OnDraw**: `state->OnDraw(...)` is called **after** `DisplayAllLayers` + `DisplayUniquePoints` — additive overlay only; states must never call `DisplayAllLayers` themselves.

### GetActiveOp / HandleCommand Pattern
Every concrete state supplies:
- `[[nodiscard]] UINT GetActiveOp() const noexcept override` — returns the state's current active op field (`m_previousOp`, `m_previousDrawCommand`, or `m_previousCommand`).
- `[[nodiscard]] bool HandleCommand(AeSysView* context, UINT command) override` — contains a `static constexpr UINT opTo*Command[]` table indexed by `command - ID_OP0`; sends the mapped `WM_COMMAND` and returns `true`, or returns `false` for unmapped ops.

The base `AeSysState::OnLButtonDown` calls `(void)HandleCommand(context, GetActiveOp())`. No concrete state needs to override `OnLButtonDown` for this purpose — only `GetActiveOp()` and `HandleCommand()` need implementing.

### PreviewGroup Contract
- Preview primitives are built inside `EoDbHandleSuppressionScope` — no handles assigned.
- Preview is rebuilt from scratch on every `OnMouseMove` — no incremental update.
- `OnExit` of every state must call `context->PreviewGroup().DeletePrimitivesAndRemoveAll()` and `context->RubberBandingDisable()`.
- Call `InvalidateScene()` for back-buffer changes; `Invalidate()` for overlay-only updates.

### Destructor Safety Rule
`~AeSysView` drains the stack via raw `pop()` only — **no `OnExit` calls in the destructor**. Window APIs are illegal on a dead HWND. States release owned heap memory via their `unique_ptr` destructors.

### PopAllModeStates
`AeSysView::PopAllModeStates()` — empties the stack, calling `OnExit` on each state in order. Called at the top of every `OnMode*()` switching command to ensure clean teardown.

### State Class Inventory
| Class | File | Mode | Owns |
|-------|------|------|------|
| `DrawModeState` | `EoMsDraw` | Draw | `m_previousDrawCommand`, points |
| `PickAndDragState` | `PickAndDragState` | Primitive/Group Edit sub-mode | group, primitive, begin/end points, transform seg |
| `PrimitiveMendState` | `PrimitiveMendState` | Mend sub-mode | primitive ptr, owning copy for Escape, vertex index |
| `Draw2ModeState` | `EoMsDraw2` | Draw2 | previous op/point, continuation flags, reference lines, assembly/section group ptrs |
| `LpdModeState` | `EoMsLpd` | Low-Pressure Duct | previous op/point/section, reference lines, original/end-cap group ptrs |
| `DimensionModeState` | `EoMsDimension` | Dimension | previous command, previous cursor position |
| `AnnotateModeState` | `EoMsAnnotate` | Annotate | previous op, points |
| `CutModeState` | `EoMsCut` | Cut | previous op, points |
| `EditModeState` | `EoMsEdit` | Edit | previous op, points |
| `FixupModeState` | `EoMsFixup` | Fixup | previous command, reference/previous/current group+primitive+line |
| `NodalModeState` | `EoMsNodal` | Nodal | previous command, previous cursor position |
| `PipeModeState` | `EoMsPipe` | Pipe | previous op, points |
| `PowerModeState` | `EoMsPower` | Power | previous op, points |
| `TrapModeState` | `EoMsTrap` | Trap | previous op, points |

### OnOp0–OnOp8 Command Routing
`AeSysViewCommands.cpp` `OnOp0`–`OnOp8` handlers each open with:
```cpp
auto* state = GetCurrentState();
if (state != nullptr && state->HandleCommand(this, ID_OPx)) { return; }
```
`AeSysState::HandleCommand` returns `bool` (default `false`). Any state that overrides `HandleCommand` and returns `true` consumes the op-key command before it reaches the legacy `switch (app.CurrentMode())` fallthrough.

`AeSysViewDrawMode.cpp::OnDrawCommand` dispatches to `state->HandleCommand` in the same pattern.

### Raw-Pointer Lifetime Rule in State Objects
Several state classes (`Draw2ModeState`, `LpdModeState`) store raw non-owning `EoDbGroup*` / `EoDbPrimitive*` pointers to document geometry. These pointers are valid only while the state is active. `OnExit` and `ResetSequence()` must null them. They must **never be accessed** after `PopState()` is called. Document-owned groups must not be deleted through state pointers — deletion goes through the document layer API.

### Key Files
| File | Role |
|------|------|
| `AeSys\AeSysState.h` | Abstract base — lifecycle virtuals, input virtuals, `GetActiveOp()` / `HandleCommand()` defaults |
| `AeSys\EoMs*.h/.cpp` | All 12 primary mode state pairs |
| `AeSys\PickAndDragState.h/.cpp` | Primitive/Group edit sub-mode |
| `AeSys\PrimitiveMendState.h/.cpp` | Mend sub-mode; `MendStateReturn`/`MendStateEscape` on `AeSysView` |
| `AeSys\EoModeConfig.h` | `FixupConfig`, `EditConfig` plain-data config structs owned by `AeSysView` (persisted across mode switches, read/written by options dialogs) |
| `AeSys\EoLpdGeometry.h/.cpp` | Stateless `Lpd::` namespace — `GenerateEndCap`, `GenerateRectangularSection`, `GenerateRectangularElbow`, `GenerateTransition`, `GenerateRiseDrop`, `LengthOfTransition`; `LpdConfig` lives alongside |
| `AeSys\EoPipeGeometry.h/.cpp` | Stateless `Pipe::` namespace — `GenerateTickMark` (returns `bool`), `GenerateLineWithFittings`; `PipeConfig` lives alongside |
| `AeSys\EoPowerGeometry.h/.cpp` | Stateless `Power::` namespace — `FillHomeRunArrow`, `FillConductorSymbol` (returns `false` on unrecognized conductor type); `PowerConfig` lives alongside |
| `AeSys\AeSysDocNodal.cpp` | `RenderUniquePoints(view, renderDevice)` — called from `OnDraw` |
| `AeSys\AeSysView.h/.cpp` | Stack ownership, `PushState`, `PopState`, `PopAllModeStates`, `GetCurrentState`; destructor: raw `pop()` only |
| `AeSys\AeSysViewCommands.cpp` | `OnModeDraw` (push), all other `OnMode*` (call `PopAllModeStates` first) |
| `AeSys\AeSysViewInput.cpp` | `PreTranslateMessage`, `OnLButtonDown`, `OnRButtonDown`, `OnRButtonUp`, `OnMouseMove` dispatch |
| `AeSys\AeSysViewRender.cpp` | `OnDraw` back-buffer dispatch; `state->OnDraw` additive overlay |

## EoDocCommand Undo/Redo Stack

### Architecture
`AeSysDoc` owns `EoDocCommandStack m_commandStack` (64-entry cap). Commands are pushed **after** the operation has already been performed — matching the "already-done" convention used throughout. The stack fully replaces the legacy PEG `m_DeletedGroupList` one-shot restore model.

`EoDocCommand` base: `Label/Execute/Undo` pure virtuals + `Discard(AeSysDoc*)` virtual (default no-op). `Discard` is called on eviction (cap overflow, redo clear on new push, `DeleteContents`). Every `Discard` that owns an `EoDbGroup` must call `group->DeletePrimitivesAndRemoveAll()` before the `unique_ptr` destructs the group — `~EoDbGroup()` is empty by design and does not free primitives.

### Keyboard Bindings
| Key | Action |
|-----|--------|
| Ctrl+Z | Undo one step |
| Ctrl+Y | Redo one step |
| U | Undo one step (AutoCAD muscle memory) |
| Delete | `EoDocCmdDeleteGroup` — delete engaged group |
| Backspace | `EoDocCmdDeleteLastGroup` — delete last drawn group |

The Insert key (legacy "Undelete") and its accelerator have been removed from the RC. Group restore is handled exclusively by the undo stack.

### Command Inventory
| Class | Trigger | Owns group on redo branch |
|-------|---------|--------------------------|
| `EoDocCmdDeleteGroup` | Delete key | Yes — `m_ownedGroup` |
| `EoDocCmdDeleteLastGroup` | Backspace | Yes — `m_ownedGroup` |
| `EoDocCmdTrapCut` | Edit > Trap Cut | Yes — per-entry `owned` |
| `EoDocCmdDeletePrimitive` | Shift+Delete (solo/multi) | Yes — `m_wrapperGroup` |
| `EoDocCmdAddGroup` | Every `AddWorkLayerGroup` call | Yes — `m_ownedGroup` (on undo branch) |
| `EoDocCmdTransformGroups` | `TransformTrappedGroups` / `TranslateTrappedGroups` | No — stores forward matrix + non-owning group pointers |

`EoDocCmdTransformGroups` stores the forward `EoGeTransformMatrix`; undo computes the inverse via `EoGeMatrix::Inverse()`. Labels per call site: `"Move"`, `"Rotate CCW"`, `"Rotate CW"`, `"Flip"`, `"Reduce"`, `"Enlarge"`, `"Translate"`.

### PushCommand Safety Chokepoint
`AeSysDoc::PushCommand(std::unique_ptr<EoDocCommand>)` silently `Discard()`s the incoming command when `m_isClosing` or `IsInEditor()` is true — prevents recording during teardown and inside the block/tracing editor.

### Null-Layer Discipline
Call sites that obtain a layer via `AnyLayerRemove(group)` MUST handle null return as an orphan-group case: `ATLTRACE2` warning → `UnregisterGroupHandles(group)` → `group->DeletePrimitivesAndRemoveAll()` → `delete group`. Enforced at: `AeSysView::DeleteLastGroup`, `AeSysDoc::OnToolsPrimitiveDelete`, `AeSysDoc::OnEditTrapCut`.

### Key Files
| File | Role |
|------|------|
| `AeSys\EoDocCommand.h` | Full command hierarchy; `EoDocCommandStack` with `Push/Undo/Redo/Clear` |
| `AeSys\EoDocCommand.cpp` | All `Execute/Undo/Discard` implementations |
| `AeSys\AeSysDoc.h` | `m_commandStack`; `PushCommand` chokepoint |
| `AeSys\AeSysDocCommands.cpp` | `OnEditUndo`, `OnEditRedo`, `OnToolsGroupDelete`, `OnToolsPrimitiveDelete`, `OnEditTrapCut` |
| `AeSys\AeSysDocLayers.cpp` | `AddWorkLayerGroup` — pushes `EoDocCmdAddGroup` after add |
| `AeSys\EoDbTrappedGroups.cpp` | `TransformTrappedGroups` / `TranslateTrappedGroups` — push `EoDocCmdTransformGroups` |


