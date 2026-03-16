# Copilot Instructions

I am working on an old C++ MFC CAD project that I stopped coding on around 2000. I have the project building warning free at Wall using Visual Studio 2026 with the C++latest (19.5) compiler and the v145 toolset. The local project repo is in a folder called `D:\Visual Studio\migrations\Peg111222`. I have introduced version control with a local .git and GitHub. The public repo at GitHub URL is `https://github.com/terry-texas-us/Eo111222`, and the main project is `https://github.com/terry-texas-us/Eo111222/tree/master/AeSys`. I encourage you to reference the code there if necessary.

I need a solution for reading open source .DXF CAD files, which I will convert to the proprietary .PEG (`Peg & Tra File Formats.md`) file. For DXF initial parsing, I will use `libdxfrw`, since this is the only C++ option.

I will be making substantial changes to the .PEG file to make linear parsing of .DXF easier. Using the terminology of the .DXF specification, I want to use a handle architecture for at least the header and table sections. The only hard resource handles will be from the entities to the header and tables. I am uncertain if I need to implement extension dictionaries, but it would help with future proofing. I have no experience with this type of persistence database design.

You can assume I know the code base well and should have little trouble with modern versions of C++. Provide suggestions detailing the code modernization.

## General Guidelines
- Purpose: Native MFC/C++ CAD/graphics application (AeSys). Keep suggestions compatible with the existing MFC architecture and on-disk file formats.
- Architecture & patterns: MFC document/view pattern (classes: `AeSysDoc`, `AeSysView`) – suggestions should maintain MFC idioms where applicable.
- Geometric primitives implement a common base (`EoDbPrimitive`) with virtuals for drawing, selection, transform, serialization – preserve virtual contract and ABI when changing signatures.
- Use `Peg & Tra File Formats.md` to help understand the legacy file structure.

## Code Style, Linters & Formatting
- Repository contains `.clang-format` and `.clang-tidy` at root – prefer those settings for formatting and static-analysis suggestions.
- For Visual Studio-specific formatting preferences, reference __Tools > Options > Text Editor > C/C++ > Formatting__.
- Minimize raw `new`/`delete`.
- Be conservative in migration from `CString` to `std::wstring` – prefer consistent conversions and avoid unnecessary copies. Transition away from `std::string` where possible in the DXF/text-codec migration, favoring `std::wstring` when practical. In `EoDxfLib`, prefer `std::wstring` as the only internal text API and avoid `std::string` except at unavoidable external byte boundaries.
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

## EoDbText Render-Time Formatting Architecture
- AeSys already handles `\P`, `\A`, and `\S` formatting codes **at render time** inside `DisplayTextWithFormattingCharacters()`. The detection is done by `HasFormattingCharacters()` in `EoDbText.cpp`. This means MTEXT formatting codes that map to these (paragraph breaks, alignment changes, stacked fractions) can be **preserved in the text string** rather than stripped at import time — the renderer will handle them. Only formatting codes that AeSys does NOT support at render time (font changes \f, color \C, height \H, width \W, tracking \T, oblique \Q, underline \L/\l, overline \O/\o) need to be stripped during DXF import.

## .PEG Legacy Formatting Convention
- Legacy .peg files use `^/` … `^` for stacked fractions. `ConvertFormattingCharacters()` converts these to `\S` … `;` format at load time. DXF MTEXT already uses `\S` natively, so MTEXT stacked fractions can be passed through directly.

## EoDbText Constructor Behavior
- Both `EoDbText` constructors (`CString&` and `std::wstring&` variants) call `renderState.Color()` to set `m_color`. When importing from DXF, `SetBaseProperties()` must be called AFTER construction to override this with the entity's actual color. The current conversion code does this correctly.

## DPI Handling
- Prefer using `GetDpiForSystem` (or `GetDpiForWindow` when available) for DPI fixes in this codebase.

## Coordinate System Conventions
- **OCS (Object Coordinate System)**: DXF/DWG entities use OCS defined by an extrusion vector. When `extrusion.z < 0`, CCW in OCS appears CW when viewed from +Z in WCS.
- **WCS (World Coordinate System)**: Legacy PEG files store geometry directly in WCS without OCS conventions.
- **Normalization strategy**: When converting legacy `EoDbEllipse` to `EoDbConic`, normalize to OCS-based representation so `Display()` logic is consistent across all sources.
- Use `EoDbPrimitive::ComputeArbitraryAxis()` for DXF arbitrary axis algorithm.

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

## AeSys Text Rendering Architecture Notes
- Some DXF text formatting is handled at **render time** in AeSys, not at import time. See `DisplayTextWithFormattingCharacters()`, `HasFormattingCharacters()` in `EoDbText.cpp`.
- The `\r\n` newline convention is handled during `DisplayText()` by splitting into segments and calling `text_GetNewLinePos()` for line advancement.
- `ConvertFormattingCharacters()` is called after constructing text primitives from .peg files to normalize legacy formatting.
- The stroke font renderer (`DisplayTextSegmentUsingStrokeFont`) uses `Eo::defaultCharacterCellAspectRatio` (0.6) for character spacing, confirming the cell geometry assumption.
- TrueType font rendering path (`DisplayTextSegmentUsingTrueTypeFont`) is conditional on the font definition's precision and view settings.

## Response Formatting Preference
- Format responses as a cleaner Markdown-style preview, with better visual structure than plain text.
