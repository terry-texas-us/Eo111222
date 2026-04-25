#include "Stdafx.h"

#include <cmath>
#include <cstdint>
#include <string>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbDxfInterface.h"
#include "EoDbFontDefinition.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDxfEntities.h"
#include "EoGeOcsTransform.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

/** @brief Converts a DXF Linear/Rotated DIMENSION entity to exploded AeSys primitives.
 *
 *  DXF DIMENSION entities are complex: they carry definition points and a reference to a dimension
 *  style, and AutoCAD uses these to generate block geometry (extension lines, dimension line,
 *  arrowheads, text). AeSys does not have a native dimension engine, so this converter produces
 *  **exploded geometry** — the same primitives AutoCAD would generate, computed from definition
 *  points and dimstyle variables.
 *
 *  ## DXF Group Code Summary (Linear/Rotated)
 *  | Code | Field | Notes |
 *  |------|-------|-------|
 *  | 10/20/30 | Definition point | On the dimension line (WCS) |
 *  | 11/21/31 | Text midpoint | Middle of dimension text (OCS) |
 *  | 13/23/33 | Extension line 1 origin | First measured feature point (WCS) |
 *  | 14/24/34 | Extension line 2 origin | Second measured feature point (WCS) |
 *  | 50 | Rotation angle | Degrees — 0=horizontal, 90=vertical |
 *  | 52 | Oblique angle | Extension line oblique angle (degrees, optional) |
 *  | 3 | Dimension style name | Lookup in document dimstyle table |
 *
 *  ## Geometry Construction
 *  1. The measurement direction is defined by the rotation angle.
 *  2. The perpendicular direction defines the extension line run.
 *  3. The dimension line Y-level (in the rotated frame) is derived from the definition point.
 *  4. Extension lines run from the feature origins (offset by dimexo) to the dimension line
 *     (extended by dimexe).
 *  5. Tick marks at dimension line endpoints: parallel 45° oblique diagonals crossing the
 *     dimension line. Rendered when dimtsz > 0 (explicit tick size) or as a fallback when
 *     dimasz > 0 and dimtsz == 0 (arrowhead block names are not rendered).
 *  6. Dimension text is placed at the DXF text midpoint (group 11/21/31) with Middle vertical
 *     alignment — the midpoint already incorporates dimtad and dimgap offsets.
 *  7. Text orientation: when the measurement direction points left (angle 90°–270°), text axes
 *     are flipped 180° so dimension text always reads left-to-right.
 *  8. Measurement formatting: dimlunit controls linear unit format (Decimal, Architectural,
 *     Engineering, Scientific, Fractional). dimlfac scales the raw measurement. dimrnd rounds
 *     to the specified increment. dimzin controls zero suppression for architectural/fractional
 *     formats. dimdsep overrides the decimal separator.
 *
 *  ## dimfrac Clarification
 *  `dimfrac` (group 276) controls **fraction stacking display** in Architectural (dimlunit=4)
 *  and Fractional (dimlunit=5) formats: 0=horizontal bar, 1=diagonal bar, 2=inline (not stacked).
 *  It does NOT switch between fractional and decimal inch display. All three values produce the
 *  same fractional number text; only the visual rendering of the fraction differs. AeSys stroke
 *  font renders fractions inline regardless, so dimfrac is informational only.
 *
 *  ## dimlunit/dimunit Fallback
 *  `dimlunit` (group 277, AC1015+) is the authoritative linear unit format variable. If zero or
 *  absent, the legacy `dimunit` (group 270, AC1012+) is consulted. If both are zero, defaults
 *  to Decimal (2).
 *
 *  ## Limitations
 *  - Arrowhead blocks (dimblk/dimblk1/dimblk2) are not rendered — oblique ticks are used as fallback.
 *  - Oblique extension lines (code 52) are not yet implemented.
 *  - Text is always placed at the DXF text midpoint; dimgap is not clipped from the dimension line.
 *  - Tolerance text (dimtol/dimlim) is not generated.
 *  - Alternate units (dimalt) are not generated.
 *  - Architectural fractions use inline display regardless of dimfrac (stacking not supported).
 *
 *  @param dimension The parsed DXF DIMENSION entity (Linear subtype).
 *  @param document The AeSys document receiving the created primitives.
 */
void EoDbDxfInterface::ConvertDimLinearEntity(const EoDxfDimLinear& dimension, AeSysDoc* document) const {
  ATLTRACE2(traceGeneral, 2, L"DimLinear entity conversion\n");

  // --- Resolve dimension style ---
  const auto* dimStyle = document->FindDimStyle(dimension.GetDimensionStyleName());
  if (dimStyle == nullptr) {
    ATLTRACE2(traceGeneral, 1, L"DimLinear: dimension style '%s' not found, using defaults\n",
        dimension.GetDimensionStyleName().c_str());
  }

  // DimStyle variables with defaults matching the DXF "Standard" style.
  // When dimscale is 0 or negative, treat as 1.0 (DXF convention: 0 = "use layout scale").
  const double rawDimscale = dimStyle != nullptr ? dimStyle->dimscale : 1.0;
  const double dimscale = rawDimscale > Eo::geometricTolerance ? rawDimscale : 1.0;
  const double dimasz = (dimStyle != nullptr ? dimStyle->dimasz : 0.18) * dimscale;
  const double dimexo = (dimStyle != nullptr ? dimStyle->dimexo : 0.0625) * dimscale;
  const double dimexe = (dimStyle != nullptr ? dimStyle->dimexe : 0.18) * dimscale;
  const double dimtxt = (dimStyle != nullptr ? dimStyle->dimtxt : 0.18) * dimscale;
  const double dimtsz = (dimStyle != nullptr ? dimStyle->dimtsz : 0.0) * dimscale;
  const double dimdle = (dimStyle != nullptr ? dimStyle->dimdle : 0.0) * dimscale;
  const std::int16_t dimse1 = dimStyle != nullptr ? dimStyle->dimse1 : 0;
  const std::int16_t dimse2 = dimStyle != nullptr ? dimStyle->dimse2 : 0;
  [[maybe_unused]] const std::int16_t dimtad = dimStyle != nullptr ? dimStyle->dimtad : 0;
  const std::int16_t dimclrd = dimStyle != nullptr ? dimStyle->dimclrd : 0;  // 0 = ByBlock
  const std::int16_t dimclre = dimStyle != nullptr ? dimStyle->dimclre : 0;
  const std::int16_t dimclrt = dimStyle != nullptr ? dimStyle->dimclrt : 0;
  const double dimlfac = dimStyle != nullptr ? dimStyle->dimlfac : 1.0;
  const double dimrnd = dimStyle != nullptr ? dimStyle->dimrnd : 0.0;
  const std::int16_t dimzin = dimStyle != nullptr ? dimStyle->dimzin : 0;
  const std::int16_t dimdsep = dimStyle != nullptr ? dimStyle->dimdsep : 0;  // 0 = default '.'
  const std::wstring dimblk = dimStyle != nullptr ? dimStyle->dimblk : std::wstring{};

  // Resolve dimlunit: prefer dimlunit (group 277, AC1015+), fall back to dimunit (group 270, pre-R2000).
  // Both use the same value encoding: 1=Scientific, 2=Decimal, 3=Engineering, 4=Architectural, 5=Fractional.
  // dimunit (group 270) is obsolete in R2000+ but may be the only value set in older DXF files or converters.
  std::int16_t resolvedDimlunit = dimStyle != nullptr ? dimStyle->dimlunit : 2;
  if (resolvedDimlunit <= 0 && dimStyle != nullptr) {
    resolvedDimlunit = dimStyle->dimunit;
  }
  if (resolvedDimlunit <= 0 || resolvedDimlunit > 6) {
    resolvedDimlunit = 2;  // Default to Decimal
  }

  // --- Extract definition points ---
  const auto defPt = dimension.GetDefinitionPoint();      // On the dimension line (WCS)
  const auto extPt1 = dimension.GetExtensionLinePoint1();  // Feature point 1 (WCS)
  const auto extPt2 = dimension.GetExtensionLinePoint2();  // Feature point 2 (WCS)
  const auto textPtOcs = dimension.GetTextPoint();          // Text midpoint (OCS per DXF spec)

  // Transform text midpoint from OCS → WCS using the entity's extrusion direction.
  // DXF DIMENSION group codes 11/21/31 are in OCS; other definition points (10,13,14) are WCS.
  const EoGeVector3d dimExtrusionDirection{
      dimension.m_extrusionDirection.x, dimension.m_extrusionDirection.y, dimension.m_extrusionDirection.z};
  const bool dimNeedsOcsTransform = Eo::IsGeometricallyNonZero(dimExtrusionDirection.x) ||
      Eo::IsGeometricallyNonZero(dimExtrusionDirection.y) ||
      Eo::IsGeometricallyNonZero(dimExtrusionDirection.z - 1.0);

  EoDxfGeometryBase3d textPt = textPtOcs;
  if (dimNeedsOcsTransform) {
    EoGeOcsTransform dimOcsTransform{dimExtrusionDirection};
    auto transformedTextPt = EoGePoint3d{textPtOcs.x, textPtOcs.y, textPtOcs.z};
    transformedTextPt = dimOcsTransform * transformedTextPt;
    textPt.x = transformedTextPt.x;
    textPt.y = transformedTextPt.y;
    textPt.z = transformedTextPt.z;
  }

  // --- Build measurement direction from rotation angle ---
  const double rotationRadians = Eo::DegreeToRadian(dimension.GetRotationAngle());
  const double cosRot = std::cos(rotationRadians);
  const double sinRot = std::sin(rotationRadians);

  // Measurement direction (along the dimension line) and perpendicular (extension line run)
  const EoGeVector3d measureDir{cosRot, sinRot, 0.0};
  const EoGeVector3d extDir{-sinRot, cosRot, 0.0};

  // --- Project feature points onto the dimension line ---
  // The dimension line passes through defPt perpendicular to extDir.
  // dimLineLevel = how far along extDir the dimension line sits (from origin).
  const EoGeVector3d defVec{defPt.x, defPt.y, defPt.z};
  const double dimLineLevel = DotProduct(EoGeVector3d{defVec}, extDir);

  // Project extension line origins along extDir
  const double ext1Level = DotProduct(EoGeVector3d{extPt1.x, extPt1.y, extPt1.z}, extDir);
  const double ext2Level = DotProduct(EoGeVector3d{extPt2.x, extPt2.y, extPt2.z}, extDir);

  // Positions along measureDir for each extension line origin
  const double ext1Along = DotProduct(EoGeVector3d{extPt1.x, extPt1.y, extPt1.z}, measureDir);
  const double ext2Along = DotProduct(EoGeVector3d{extPt2.x, extPt2.y, extPt2.z}, measureDir);

  // Dimension line endpoint positions (projected feature points at dim line level)
  const EoGePoint3d dimLinePt1{
      extPt1.x + extDir.x * (dimLineLevel - ext1Level),
      extPt1.y + extDir.y * (dimLineLevel - ext1Level),
      extPt1.z};
  const EoGePoint3d dimLinePt2{
      extPt2.x + extDir.x * (dimLineLevel - ext2Level),
      extPt2.y + extDir.y * (dimLineLevel - ext2Level),
      extPt2.z};

  // --- Extension line geometry ---
  // Extension lines run from feature origin (offset by dimexo) toward the dimension line
  // (extended by dimexe past it). The direction is from the feature point toward the dim line.
  auto buildExtensionLine = [&](const EoDxfGeometryBase3d& featurePoint, double featureLevel,
                                 const EoGePoint3d& dimLinePoint) -> std::pair<EoGePoint3d, EoGePoint3d> {
    const double totalRun = dimLineLevel - featureLevel;
    const double sign = totalRun >= 0.0 ? 1.0 : -1.0;

    const EoGePoint3d startPoint{
        featurePoint.x + extDir.x * dimexo * sign,
        featurePoint.y + extDir.y * dimexo * sign,
        featurePoint.z};
    const EoGePoint3d endPoint{
        dimLinePoint.x + extDir.x * dimexe * sign,
        dimLinePoint.y + extDir.y * dimexe * sign,
        dimLinePoint.z};
    return {startPoint, endPoint};
  };

  // --- Create extension line 1 ---
  if (dimse1 == 0) {
    auto [start1, end1] = buildExtensionLine(extPt1, ext1Level, dimLinePt1);
    auto* extLine1 = EoDbLine::CreateLine(start1, end1);
    extLine1->SetBaseProperties(&dimension, document);
    if (dimclre != 0) { extLine1->SetColor(dimclre); }
    AddToDocument(extLine1, document, dimension.m_space, dimension.m_ownerHandle);
  }

  // --- Create extension line 2 ---
  if (dimse2 == 0) {
    auto [start2, end2] = buildExtensionLine(extPt2, ext2Level, dimLinePt2);
    auto* extLine2 = EoDbLine::CreateLine(start2, end2);
    extLine2->SetBaseProperties(&dimension, document);
    if (dimclre != 0) { extLine2->SetColor(dimclre); }
    AddToDocument(extLine2, document, dimension.m_space, dimension.m_ownerHandle);
  }

  // --- Create dimension line ---
  // When dimdle > 0, the dimension line extends past the extension lines
  const EoGePoint3d dimLineStart{
      dimLinePt1.x - measureDir.x * dimdle,
      dimLinePt1.y - measureDir.y * dimdle,
      dimLinePt1.z};
  const EoGePoint3d dimLineEnd{
      dimLinePt2.x + measureDir.x * dimdle,
      dimLinePt2.y + measureDir.y * dimdle,
      dimLinePt2.z};

  auto* dimLine = EoDbLine::CreateLine(dimLineStart, dimLineEnd);
  dimLine->SetBaseProperties(&dimension, document);
  if (dimclrd != 0) { dimLine->SetColor(dimclrd); }
  AddToDocument(dimLine, document, dimension.m_space, dimension.m_ownerHandle);

  // --- Create tick marks
  // Tick marks are short diagonal lines at 45° across the dimension line endpoints.
  // When dimtsz > 0, use dimtsz as the tick size (explicit architectural tick).
  // When dimtsz == 0 and dimasz > 0, check dimblk for oblique/tick block names and
  // render tick marks using dimasz. Known tick block names: _OBLIQUE, OBLIQUE, _DOT,
  // _DOTSMALL, _DOTBLANK, _SMALL, _OPEN, _CLOSED, etc.
  // As a general fallback when dimtsz == 0 and dimasz > 0: render oblique ticks using
  // dimasz (better than nothing for non-associative exploded dimensions).
  const bool hasExplicitTick = dimtsz > Eo::geometricTolerance;
  const bool hasArrowFallback = !hasExplicitTick && dimasz > Eo::geometricTolerance;
  const double tickSize = hasExplicitTick ? dimtsz : dimasz;

  if (hasExplicitTick || hasArrowFallback) {
    // AutoCAD oblique ticks are parallel 45° diagonal lines crossing the dimension line.
    // Both endpoints use the same offset direction (measureDir + extDir) so the ticks
    // are parallel rather than forming a cross.
    const EoGeVector3d tickOffset = (measureDir + extDir) * (tickSize * 0.5);

    auto* tick1 = EoDbLine::CreateLine(dimLinePt1 - tickOffset, dimLinePt1 + tickOffset);
    tick1->SetBaseProperties(&dimension, document);
    if (dimclrd != 0) { tick1->SetColor(dimclrd); }
    AddToDocument(tick1, document, dimension.m_space, dimension.m_ownerHandle);

    auto* tick2 = EoDbLine::CreateLine(dimLinePt2 - tickOffset, dimLinePt2 + tickOffset);
    tick2->SetBaseProperties(&dimension, document);
    if (dimclrd != 0) { tick2->SetColor(dimclrd); }
    AddToDocument(tick2, document, dimension.m_space, dimension.m_ownerHandle);
  }

  // --- Create dimension text ---
  // Use explicit dimension text if provided, otherwise compute the measurement value.
  std::wstring dimensionText;
  const auto& explicitText = dimension.GetExplicitDimensionText();
  if (!explicitText.empty()) {
    dimensionText = explicitText;
  } else {
    // Compute the measurement: distance between feature points projected onto the measurement direction
    const double rawMeasurement = std::abs(ext2Along - ext1Along);
    // Apply linear measurement scale factor (dimlfac)
    double measurement = rawMeasurement * (Eo::IsGeometricallyNonZero(dimlfac) ? dimlfac : 1.0);

    // Apply dimrnd rounding: rounds the measurement to the nearest dimrnd increment.
    // For example, dimrnd=0.25 rounds to nearest quarter unit; dimrnd=0.5 to nearest half.
    // dimrnd=0.0 (default) means no rounding — precision is controlled only by dimdec.
    if (dimrnd > Eo::geometricTolerance) {
      measurement = std::round(measurement / dimrnd) * dimrnd;
    }

    const int decimalPlaces = dimStyle != nullptr ? dimStyle->dimdec : 4;

    ATLTRACE2(traceGeneral, 2,
        L"  DimLinear text: dimlunit=%d (raw=%d, dimunit=%d), dimdec=%d, dimrnd=%.6f, measurement=%.10f\n",
        resolvedDimlunit, dimStyle ? dimStyle->dimlunit : -1, dimStyle ? dimStyle->dimunit : -1, decimalPlaces, dimrnd,
        measurement);

    // Format based on resolvedDimlunit (linear unit format)
    switch (resolvedDimlunit) {
      case 4: {  // Architectural: feet'-inches fraction"
        // dimfrac (group 276) controls fraction stacking display (0=horizontal, 1=diagonal, 2=inline)
        // but does NOT change the number format — all three produce the same fractional text.
        // AeSys stroke font renders fractions inline regardless, so dimfrac is informational only.
        constexpr wchar_t kInchMark = 0x22;  // '"' — avoids escaping issues
        const double totalInches = measurement;
        int feet = static_cast<int>(totalInches / 12.0);
        double remainingInches = totalInches - feet * 12.0;

        // Clean floating-point noise: snap near-zero and near-12 remainders.
        // This eliminates artifacts like 360.0 - 30*12 = 1.137e-12 instead of 0.
        if (std::abs(remainingInches) < 1e-9) {
          remainingInches = 0.0;
        } else if (std::abs(remainingInches - 12.0) < 1e-9) {
          remainingInches = 0.0;
          ++feet;
        }

        const int wholeInches = static_cast<int>(remainingInches);
        const double fractionalPart = remainingInches - wholeInches;

        // Find nearest fraction (1/2, 1/4, 1/8, 1/16, 1/32, 1/64)
        // Use precision from dimdec: 0→1, 1→2, 2→4, 3→8, 4→16, 5→32, 6→64
        const int denominator = 1 << (decimalPlaces > 0 ? decimalPlaces : 4);
        const int numerator = static_cast<int>(std::round(fractionalPart * denominator));

        std::wstring result;
        // dimzin bit 2: suppress 0 feet; bit 3: suppress 0 inches
        const bool suppressLeadingZeroFeet = (dimzin & 0x04) != 0;
        const bool suppressTrailingZeroInches = (dimzin & 0x08) != 0;

        if (feet != 0 || !suppressLeadingZeroFeet) {
          result += std::to_wstring(feet);
          result += L"'-";
        }

        if (numerator == 0 || numerator == denominator) {
          // No fraction — just whole inches
          const int adjustedInches = wholeInches + (numerator == denominator ? 1 : 0);
          if (adjustedInches != 0 || !suppressTrailingZeroInches || result.empty()) {
            result += std::to_wstring(adjustedInches);
            result += kInchMark;
          } else if (!result.empty() && result.back() == L'-') {
            // Remove trailing dash when suppressing zero inches
            result.pop_back();
            result += kInchMark;
          }
        } else {
          // Reduce fraction to lowest terms
          int num = numerator;
          int den = denominator;
          while (num > 0 && den > 0 && num % 2 == 0 && den % 2 == 0) {
            num /= 2;
            den /= 2;
          }
          if (wholeInches != 0) {
            result += std::to_wstring(wholeInches) + L' ';
          }
          result += std::to_wstring(num);
          result += L'/';
          result += std::to_wstring(den);
          result += kInchMark;
        }
        dimensionText = result;
        break;
      }
      case 3: {  // Engineering: feet'-decimal inches"
        constexpr wchar_t kInchMark = 0x22;  // '"' — avoids escaping issues
        const double totalInches = measurement;
        int feet = static_cast<int>(totalInches / 12.0);
        double remainingInches = totalInches - feet * 12.0;

        // Clean floating-point noise: snap near-zero and near-12 remainders
        if (std::abs(remainingInches) < 1e-9) {
          remainingInches = 0.0;
        } else if (std::abs(remainingInches - 12.0) < 1e-9) {
          remainingInches = 0.0;
          ++feet;
        }

        wchar_t buffer[64]{};
        std::swprintf(buffer, std::size(buffer), L"%d'-%.*f", feet, decimalPlaces, remainingInches);
        dimensionText = buffer;
        dimensionText += kInchMark;
        break;
      }
      case 1: {  // Scientific: mantissa + exponent
        wchar_t buffer[64]{};
        std::swprintf(buffer, std::size(buffer), L"%.*E", decimalPlaces, measurement);
        dimensionText = buffer;
        break;
      }
      case 5: {  // Fractional
        const int whole = static_cast<int>(measurement);
        const double fractionalPart = measurement - whole;
        const int denominator = 1 << (decimalPlaces > 0 ? decimalPlaces : 4);
        int numerator = static_cast<int>(std::round(fractionalPart * denominator));
        if (numerator == 0) {
          dimensionText = std::to_wstring(whole);
        } else {
          int num = numerator;
          int den = denominator;
          while (num > 0 && den > 0 && num % 2 == 0 && den % 2 == 0) {
            num /= 2;
            den /= 2;
          }
          if (whole != 0) {
            dimensionText = std::to_wstring(whole) + L' ';
          }
          dimensionText += std::to_wstring(num) + L'/' + std::to_wstring(den);
        }
        break;
      }
      default: {  // case 2: Decimal (default), case 6: Windows Desktop
        wchar_t buffer[64]{};
        std::swprintf(buffer, std::size(buffer), L"%.*f", decimalPlaces, measurement);
        dimensionText = buffer;

        // Apply dimdsep (decimal separator override)
        if (dimdsep != 0 && dimdsep != L'.') {
          for (auto& ch : dimensionText) {
            if (ch == L'.') {
              ch = static_cast<wchar_t>(dimdsep);
              break;
            }
          }
        }
        break;
      }
    }
  }

  if (!dimensionText.empty() && dimtxt > Eo::geometricTolerance) {
    // Build text reference system at the text midpoint, oriented along the measurement direction
    const EoGePoint3d textOrigin{textPt.x, textPt.y, textPt.z};

    // Ensure text reads left-to-right (or bottom-to-top for vertical dimensions).
    // When the measurement direction angle is in the 90°–270° range (pointing generally left),
    // flip both text axes by 180° so the text is not rendered upside down.
    auto textMeasureDir = measureDir;
    auto textExtDir = extDir;
    const double measureAngle = std::atan2(measureDir.y, measureDir.x);
    if (measureAngle > Eo::HalfPi + Eo::geometricTolerance ||
        measureAngle < -(Eo::HalfPi + Eo::geometricTolerance)) {
      textMeasureDir = EoGeVector3d{-measureDir.x, -measureDir.y, -measureDir.z};
      textExtDir = EoGeVector3d{-extDir.x, -extDir.y, -extDir.z};
    }

    auto xAxisDirection = textMeasureDir;
    auto yAxisDirection = textExtDir;

    // Scale directions for the reference system
    // xDirection encodes: widthScale × height × defaultCharacterCellAspectRatio
    // yDirection encodes: text height
    const double widthScale = 1.0;
    yAxisDirection *= dimtxt;
    xAxisDirection *= widthScale * dimtxt * Eo::defaultCharacterCellAspectRatio;

    EoGeReferenceSystem referenceSystem(textOrigin, xAxisDirection, yAxisDirection);

    // Resolve text style from dimstyle (dimtxsty is a handle string — look up the style name)
    EoDbFontDefinition fontDefinition{};
    if (dimStyle != nullptr && !dimStyle->dimtxsty.empty()) {
      // dimtxsty is stored as a handle reference; for now use the document's default text style
      // since resolving handle→name requires handle map lookup which may not be populated for styles
    }

    // Center the text horizontally; always use Middle vertical alignment.
    // The DXF text midpoint (group 11/21/31) is positioned by AutoCAD to be the CENTER
    // of the dimension text, already accounting for dimtad and dimgap offsets. Using
    // Middle alignment places the text correctly at that midpoint regardless of dimtad.
    fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Center);
    fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);

    auto* textPrimitive = new EoDbText(fontDefinition, referenceSystem, dimensionText);
    textPrimitive->SetBaseProperties(&dimension, document);
    if (dimclrt != 0) { textPrimitive->SetColor(dimclrt); }
    AddToDocument(textPrimitive, document, dimension.m_space, dimension.m_ownerHandle);
  }
}
