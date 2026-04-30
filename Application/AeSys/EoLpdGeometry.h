#pragma once

#include <string>

#include "EoGeLine.h"
#include "Section.h"

class EoDbGroup;
class EoGePoint3d;

/// Duct horizontal justification — used by transition and elbow geometry.
enum EJust { Left = -1, Center, Right };

/// Elbow geometry type for Low Pressure Duct mode.
enum EElbow { Mittered, Radial };

/// Configuration parameters for Low Pressure Duct mode. Persists across mode
/// switches; owned by AeSysView as a single member and read/written via
/// EoDlgLowPressureDuctOptions.
struct LpdConfig {
  double ductSeamSize{0.03125};
  double ductTapSize{0.09375};
  double insideRadiusFactor{1.5};
  double transitionSlope{4.0};
  bool generateTurningVanes{true};
  bool beginWithTransition{false};
  EElbow elbowType{Mittered};
  EJust justification{Center};
};

/// @brief Free-function geometry generators for Low Pressure Duct (rectangular)
/// components.  All functions are stateless algorithms that populate an
/// EoDbGroup; no view, document, or dialog dependencies.  Caller-supplied
/// parameters replace the view-member reads that the original AeSysView
/// implementations used.
namespace Lpd {

/// @brief Appends an end-cap (marker point + cap line) to @p group.
/// The midpoint of [beginPoint, endPoint] receives the special marker point
/// (color 15, style 8) carrying the section dimensions as data.
void GenerateEndCap(const EoGePoint3d& beginPoint,
    const EoGePoint3d& endPoint,
    Section section,
    EoDbGroup* group);

/// @brief Appends straight-section geometry (two parallel lines + two end-caps)
/// to @p group.
void GenerateRectangularSection(const EoGeLine& referenceLine,
    double eccentricity,
    Section section,
    EoDbGroup* group);

/// @brief Appends a mitered rectangular elbow fitting to @p group.
/// Both reference lines are modified in-place (seam set-back applied).
/// @param ductSeamSize         Seam allowance distance.
/// @param centerLineEccentricity  Eccentricity for parallel offset.
/// @param generateTurningVanes When true, a dashed splitter line is added.
void GenerateRectangularElbow(EoGeLine& previousReferenceLine,
    Section previousSection,
    EoGeLine& currentReferenceLine,
    Section currentSection,
    double ductSeamSize,
    double centerLineEccentricity,
    bool generateTurningVanes,
    EoDbGroup* group);

/// @brief Appends a rectangular transition fitting to @p group.
void GenerateTransition(const EoGeLine& referenceLine,
    double eccentricity,
    EJust justification,
    double slope,
    Section previousSection,
    Section currentSection,
    EoDbGroup* group);

/// @brief Appends rise/drop geometry (section + cross indicator lines) to @p group.
/// @param indicatorLineTypeName  Line type for the diagonal rise/drop indicators.
void GenerateRiseDrop(const std::wstring& indicatorLineTypeName,
    Section section,
    EoGeLine& referenceLine,
    double ductSeamSize,
    double eccentricity,
    EoDbGroup* group);

/// @brief Returns the required transition length for a section change.
/// Returns 0 when the sections are identical.
[[nodiscard]] double LengthOfTransition(EJust justification,
    double slope,
    Section previousSection,
    Section currentSection) noexcept;

}  // namespace Lpd
