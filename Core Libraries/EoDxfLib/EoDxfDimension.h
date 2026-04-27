#pragma once

#include <cstdint>
#include <string>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"

/** @brief Accumulator class for the DXF DIMENSION entity family.
 *
 *  Stores all group codes shared across every dimension subtype (AcDbDimension section)
 *  plus all subtype-specific group codes (13–16, 40, 50, 52).  The DXF reader parses
 *  every code into this single base class; subtype dispatch happens at the code-0
 *  entity boundary in ProcessDimension().
 *
 *  Subtype classes (EoDxfDimLinear, EoDxfAlignedDimension, …) exist solely for
 *  m_entityType discrimination — they carry no additional data members.
 */
class EoDxfDimension : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfDimension() noexcept {
    m_entityType = EoDxf::DIMENSION;
    m_dimensionType = 0;
    m_dimensionTextLineSpacingStyle = 1;
    m_dimensionTextLineSpacingFactor = 1.0;
    m_rotationAngleAwayFromDefault = 0.0;
    m_attachmentPoint = 5;
    m_dimensionStyleName = L"STANDARD";
  }

  EoDxfDimension(const EoDxfDimension& other) : EoDxfGraphic(other) {
    m_entityType = EoDxf::DIMENSION;
    m_dimensionType = other.m_dimensionType;
    m_nameOfBlockContainer = other.m_nameOfBlockContainer;
    m_definitionPoint = other.m_definitionPoint;
    m_middlePointOfDimensionText = other.m_middlePointOfDimensionText;
    m_explicitDimensionText = other.m_explicitDimensionText;
    m_dimensionStyleName = other.m_dimensionStyleName;
    m_attachmentPoint = other.m_attachmentPoint;
    m_dimensionTextLineSpacingStyle = other.m_dimensionTextLineSpacingStyle;
    m_dimensionTextLineSpacingFactor = other.m_dimensionTextLineSpacingFactor;
    m_rotationAngleAwayFromDefault = other.m_rotationAngleAwayFromDefault;
    m_horizontalDirection = other.m_horizontalDirection;
    m_clonePoint = other.m_clonePoint;
    // Subtype-specific accumulator fields
    m_extensionLinePoint1 = other.m_extensionLinePoint1;
    m_extensionLinePoint2 = other.m_extensionLinePoint2;
    m_radiusDiameterPoint = other.m_radiusDiameterPoint;
    m_arcDefinitionPoint = other.m_arcDefinitionPoint;
    m_rotationAngle = other.m_rotationAngle;
    m_obliqueAngle = other.m_obliqueAngle;
    m_leaderLength = other.m_leaderLength;
  }
  virtual ~EoDxfDimension() = default;

  virtual void ApplyExtrusion() noexcept {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  // --- AcDbDimension section getters ---
  [[nodiscard]] EoDxfGeometryBase3d GetDefinitionPoint() const noexcept { return m_definitionPoint; }
  [[nodiscard]] EoDxfGeometryBase3d GetTextPoint() const noexcept { return m_middlePointOfDimensionText; }
  [[nodiscard]] EoDxfGeometryBase3d GetClonePoint() const noexcept { return m_clonePoint; }
  [[nodiscard]] const std::wstring& GetDimensionStyleName() const noexcept { return m_dimensionStyleName; }
  [[nodiscard]] std::int16_t GetAttachmentPoint() const noexcept { return m_attachmentPoint; }
  [[nodiscard]] std::int16_t GetTextLineSpacingStyle() const noexcept { return m_dimensionTextLineSpacingStyle; }
  [[nodiscard]] const std::wstring& GetExplicitDimensionText() const noexcept { return m_explicitDimensionText; }
  [[nodiscard]] double GetDimensionTextLineSpacingFactor() const noexcept { return m_dimensionTextLineSpacingFactor; }
  [[nodiscard]] double GetRotationAngleAwayFromDefault() const noexcept { return m_rotationAngleAwayFromDefault; }
  [[nodiscard]] const std::wstring& GetBlockName() const noexcept { return m_nameOfBlockContainer; }

  // --- Subtype-specific accumulator getters ---
  /// Extension line definition point 1 (group codes 13/23/33, WCS).
  /// Used by: Linear, Aligned, Angular (2-line and 3-point), Ordinate.
  [[nodiscard]] EoDxfGeometryBase3d GetExtensionLinePoint1() const noexcept { return m_extensionLinePoint1; }

  /// Extension line definition point 2 (group codes 14/24/34, WCS).
  /// Used by: Linear, Aligned, Angular (2-line and 3-point), Ordinate.
  [[nodiscard]] EoDxfGeometryBase3d GetExtensionLinePoint2() const noexcept { return m_extensionLinePoint2; }

  /// Radius/diameter/angular vertex definition point (group codes 15/25/35, WCS).
  /// Used by: Radial, Diametric, Angular (2-line and 3-point).
  [[nodiscard]] EoDxfGeometryBase3d GetRadiusDiameterPoint() const noexcept { return m_radiusDiameterPoint; }

  /// Arc definition point for angular dimensions (group codes 16/26/36, OCS).
  /// Used by: Angular 2-line only.
  [[nodiscard]] EoDxfGeometryBase3d GetArcDefinitionPoint() const noexcept { return m_arcDefinitionPoint; }

  /// Rotation angle for rotated/horizontal/vertical dimensions (group code 50, degrees).
  /// Used by: Linear (rotated dimension).
  [[nodiscard]] double GetRotationAngle() const noexcept { return m_rotationAngle; }

  /// Oblique angle of extension lines (group code 52, degrees).
  /// Used by: Linear.
  [[nodiscard]] double GetObliqueAngle() const noexcept { return m_obliqueAngle; }

  /// Leader length for radius and diameter dimensions (group code 40).
  /// Used by: Radial, Diametric.
  [[nodiscard]] double GetLeaderLength() const noexcept { return m_leaderLength; }

 public:
  std::int16_t m_dimensionType;  // Group code 70

 private:
  // --- AcDbDimension common fields ---
  std::wstring m_nameOfBlockContainer;  // Group code 2
  EoDxfGeometryBase3d m_definitionPoint;  // Group codes 10/20/30 (WCS)
  EoDxfGeometryBase3d m_middlePointOfDimensionText;  // Group codes 11/21/31 (OCS)
  std::wstring m_explicitDimensionText;  // Group code 1
  std::wstring m_dimensionStyleName;  // Group code 3
  std::int16_t m_attachmentPoint;  // Group code 71
  std::int16_t m_dimensionTextLineSpacingStyle{1};  // Group code 72 (optional)
  double m_dimensionTextLineSpacingFactor;  // Group code 41 (optional)
  double m_rotationAngleAwayFromDefault;  // Group code 53 (optional)
  double m_horizontalDirection{};  // Group code 51 (optional)
  EoDxfGeometryBase3d m_clonePoint;  // Group codes 12/22/32 (OCS)

  // --- Subtype-specific accumulator fields ---
  // These are populated by the base ParseCode for ALL dimension subtypes.
  // Only the fields relevant to a particular subtype will contain meaningful values;
  // the others remain zero-initialized.
  EoDxfGeometryBase3d m_extensionLinePoint1;  // Group codes 13/23/33 (WCS)
  EoDxfGeometryBase3d m_extensionLinePoint2;  // Group codes 14/24/34 (WCS)
  EoDxfGeometryBase3d m_radiusDiameterPoint;  // Group codes 15/25/35 (WCS)
  EoDxfGeometryBase3d m_arcDefinitionPoint;  // Group codes 16/26/36 (OCS)
  double m_rotationAngle{};  // Group code 50 (degrees)
  double m_obliqueAngle{};  // Group code 52 (degrees)
  double m_leaderLength{};  // Group code 40
};

/// Aligned dimension subtype (AcDbAlignedDimension).
/// Data is fully accumulated in the base class; this subtype exists only for m_entityType discrimination.
class EoDxfAlignedDimension : public EoDxfDimension {
 public:
  EoDxfAlignedDimension() noexcept { m_entityType = EoDxf::DIMALIGNED; }
  EoDxfAlignedDimension(const EoDxfDimension& d) noexcept : EoDxfDimension(d) { m_entityType = EoDxf::DIMALIGNED; }
};

/// Linear/Rotated dimension subtype (AcDbAlignedDimension + AcDbRotatedDimension).
/// Data is fully accumulated in the base class; this subtype exists only for m_entityType discrimination.
class EoDxfDimLinear : public EoDxfDimension {
 public:
  EoDxfDimLinear() { m_entityType = EoDxf::DIMLINEAR; }
  EoDxfDimLinear(const EoDxfDimension& dimension) : EoDxfDimension(dimension) { m_entityType = EoDxf::DIMLINEAR; }
};

/// Radial dimension subtype (AcDbRadialDimension).
/// Data is fully accumulated in the base class; this subtype exists only for m_entityType discrimination.
class EoDxfRadialDimension : public EoDxfDimension {
 public:
  EoDxfRadialDimension() noexcept { m_entityType = EoDxf::DIMRADIAL; }
  EoDxfRadialDimension(const EoDxfDimension& dimension) noexcept : EoDxfDimension(dimension) { m_entityType = EoDxf::DIMRADIAL; }
};

/// Diametric dimension subtype (AcDbDiametricDimension).
/// Data is fully accumulated in the base class; this subtype exists only for m_entityType discrimination.
class EoDxfDiametricDimension : public EoDxfDimension {
 public:
  EoDxfDiametricDimension() { m_entityType = EoDxf::DIMDIAMETRIC; }
  EoDxfDiametricDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMDIAMETRIC;
  }
};

/// 2-line angular dimension subtype (AcDb2LineAngularDimension).
/// Data is fully accumulated in the base class; this subtype exists only for m_entityType discrimination.
class EoDxf2LineAngularDimension : public EoDxfDimension {
 public:
  EoDxf2LineAngularDimension() noexcept { m_entityType = EoDxf::DIMANGULAR; }
  EoDxf2LineAngularDimension(const EoDxfDimension& dimension) noexcept : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMANGULAR;
  }
};

/// 3-point angular dimension subtype (AcDb3PointAngularDimension).
/// Data is fully accumulated in the base class; this subtype exists only for m_entityType discrimination.
class EoDxf3PointAngularDimension : public EoDxfDimension {
 public:
  EoDxf3PointAngularDimension() { m_entityType = EoDxf::DIMANGULAR3P; }
  EoDxf3PointAngularDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMANGULAR3P;
  }
};

/// Ordinate dimension subtype (AcDbOrdinateDimension).
/// Data is fully accumulated in the base class; this subtype exists only for m_entityType discrimination.
class EoDxfOrdinateDimension : public EoDxfDimension {
 public:
  EoDxfOrdinateDimension() noexcept { m_entityType = EoDxf::DIMORDINATE; }
  EoDxfOrdinateDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMORDINATE;
  }
};
