# Copilot Instructions

I am working on an old C++ MFC CAD project that I stopped coding on around 2000. I have the project building warning free at Wall using Visual Studio 2026 with the C++latest (19.5) compiler and the v145 toolset.
The local project repo is in a folder called `D:\Visual Studio\migrations\Peg111222`. I have introduced version control with a local .git and GitHub. The public repo at GitHub URL is `https://github.com/terry-texas-us/Eo111222`, and the main project is `https://github.com/terry-texas-us/Eo111222/tree/master/AeSys`. I encourage you to reference the code there if necessary.

I need a solution for reading open source .DXF CAD files, which I will convert to the proprietary .PEG (`Peg & Tra File Formats.md`) file. 
For DXF initial parsing, I will use `libdxfrw`, since this is the only C++ option.

I will be making substantial changes to the .PEG file to make linear parsing of .DXF easier.
Using the terminology of the .DXF specification, I want to use a handle architecture for at least the header and table sections. 
The only hard resource handles will be from the entities to the header and tables. I am uncertain if I need to implement extension dictionaries, but it would help with future proofing. 
I have no experience with this type of persistence database design.

You can assume I know the code base well and should have little trouble with modern versions of C++. Provide suggestions detailing the code modernization.

## General Guidelines
- Purpose: Native MFC/C++ CAD/graphics application (AeSys). Keep suggestions compatible with the existing MFC architecture and on-disk file formats.
- Architecture & patterns: MFC document/view pattern (classes: `AeSysDoc`, `AeSysView`) – suggestions should maintain MFC idioms where applicable.
- Geometric primitives implement a common base (`EoDbPrimitive`) with virtuals for drawing, selection, transform, serialization – preserve virtual contract and ABI when changing signatures.
- Use `Peg & Tra File Formats.md` to help understand the legacy file structure.

## Code Style, Linters & Formatting
- Repository contains `.clang-format` and `.clang-tidy` at root – prefer those settings for formatting and static-analysis suggestions.
- For Visual Studio-specific formatting preferences, reference __Tools > Options > Text Editor > C/C++ > Formatting__.
- Prefer RAII and smart pointers (`std::unique_ptr`, `std::shared_ptr` when required). Minimize raw `new`/`delete`.
- Be conservative in migration from `CString` to `std::wstring` – prefer consistent conversions and avoid unnecessary copies.
- Step away from MFC `CObject`; minimize dynamic runtime features; avoid file serialization; use `std::containers` and modern C++ idioms for collections.
- Migrate away from MFC `CTypedPtrMap` to `std::map` and drop `CObject` inheritance for `EoDbLineType`.
- Prefer camelCase for local variables; convert PascalCase local variables to camelCase when requested.
- Prefer marking simple geometric operations and getters `noexcept` when possible.

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

## Workspace Preferences and Constraints
- Use Visual Studio 2026 with C++latest (19.5) and v145 toolset.
- Prefer RAII and smart pointers.
- Preserve MFC idioms and ABI stability of `EoDbPrimitive` virtuals.
- Use `.clang-format` and `.clang-tidy` from the repo root.
- Use `libdxfrw` for DXF parsing.
- External library `Tools\reflex_static_lib.lib` is present.