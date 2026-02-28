Follow these rules for every code change:

- Use the project’s .clang-format and .clang-tidy at repo root.
- Prefer camelCase for local variables; convert PascalCase locals to camelCase; use m_ prefix for class members.
- Mark simple geometric operations and getters noexcept when possible.
- Mark getters [[nodiscard]] when appropriate.
- Minimize raw new/delete — prefer smart pointers or stack objects.
- Be conservative migrating CString to std::wstring; prefer consistent conversions without unnecessary copies.
- Step away from MFC CObject where possible; minimize dynamic runtime features; avoid MFC file serialization.
- Use GetDpiForSystem() or GetDpiForWindow() for any DPI-related code.
- All angles are in radians. Use NormalizeTo2Pi(), Eo::TwoPi, Eo::HalfPi, Eo::Pi.
- Use Eo::geometricTolerance for geometric comparisons (< instead of == 0.0). 
- Use Eo::numericEpsilon for floating-point arithmetic.
- Reference geometry classes: EoGePoint3d, EoGeVector3d, EoGeTransformMatrix, EoGeLine, EoGeReferenceSystem.
- Core primitive headers: AeSys\EoDbPrimitive.h, EoDbLine.h, EoDbConic.h, EoDbPoint.h, EoDbPolyline.h, EoDbPolygon.h, EoDbSpline.h, EoDbText.h, EoDbBlockReference.h.
- For DXF/OCS: normalize legacy EoDbEllipse to EoDbConic using OCS and EoDbPrimitive::ComputeArbitraryAxis(). 
  Respect OCS extrusion vector and WCS legacy PEG storage.