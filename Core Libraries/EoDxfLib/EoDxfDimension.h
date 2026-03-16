#pragma once

#include <cstdint>
#include <string>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"

/** @brief Class to handle dimension entity
 *
 *  A dimension entity represents a measurement annotation in a drawing.
 *  It is defined by its type (code 70), definition point (code 10, 20, 30), and text point (code 11, 21, 31).
 *  The dimension entity can also include properties such as the dimension style (code 3), attachment point (code 71),
 * text line spacing style (code 72), dimension text (code 1), text line spacing factor (code 41), rotation angle of
 * the dimension text (code 53), and extrusion direction (code 210, 220, 230), which can affect how it is rendered in
 * the drawing.
 */
class EoDxfDimension : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfDimension() {
    m_entityType = EoDxf::DIMENSION;
    m_dimensionType = 0;
    m_dimensionTextLineSpacingStyle = 1;
    m_dimensionTextLineSpacingFactor = 1.0;
    m_rotationAngleAwayFromDefault = 0.0;
    m_attachmentPoint = 5;
    m_dimensionStyleName = L"STANDARD";
    m_definitionPoint = {};
    m_middlePointOfDimensionText = {};
    clonePoint = {};
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
    clonePoint = other.clonePoint;
  }
  virtual ~EoDxfDimension() = default;

  virtual void ApplyExtrusion() {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  [[nodiscard]] EoDxfGeometryBase3d GetDefinitionPoint() const noexcept { return m_definitionPoint; }
  [[nodiscard]] EoDxfGeometryBase3d getTextPoint() const { return m_middlePointOfDimensionText; }
  [[nodiscard]] const std::wstring& GetDimensionStyleName() const { return m_dimensionStyleName; }
  [[nodiscard]] std::int16_t GetAttachmentPoint() const noexcept { return m_attachmentPoint; }
  [[nodiscard]] std::int16_t getTextLineStyle() const noexcept { return m_dimensionTextLineSpacingStyle; }
  [[nodiscard]] const std::wstring& GetExplicitDimensionText() const { return m_explicitDimensionText; }
  [[nodiscard]] double GetDimensionTextLineSpacingFactor() const noexcept { return m_dimensionTextLineSpacingFactor; }
  [[nodiscard]] double GetRotationAngleAwayFromDefault() const noexcept { return m_rotationAngleAwayFromDefault; }
  [[nodiscard]] const std::wstring& getName() const { return m_nameOfBlockContainer; }

 protected:
  EoDxfGeometryBase3d getPt2() const { return clonePoint; }
  void setPt2(const EoDxfGeometryBase3d p) { clonePoint = p; }

 public:
  std::int16_t m_dimensionType;  // Group code 70
 private:
  std::wstring m_nameOfBlockContainer;  // Group code 2
  EoDxfGeometryBase3d m_definitionPoint;  //  Group codes 10, 20 & 30 (WCS)
  EoDxfGeometryBase3d m_middlePointOfDimensionText;  // Group codes 11, 21 & 31 (OCS)
  std::wstring m_explicitDimensionText;  // Group code 1
  std::wstring m_dimensionStyleName;  // Group code 3
  std::int16_t m_attachmentPoint;  // attachment point, code 71
  std::int16_t m_dimensionTextLineSpacingStyle{1};  // Group code 72 (optional)
  double m_dimensionTextLineSpacingFactor;  // Group code 41 (optional)
  double m_rotationAngleAwayFromDefault;  // Group code 53 (optional)

  double m_horizontalDirection{};  // Group code 51 (optional)

  EoDxfGeometryBase3d clonePoint;  // Group codes 12, 22 & 32 (OCS)
};

/** @brief Class to handle aligned dimension entity
 *
 *  An aligned dimension is a type of dimension that measures the distance between two points along a specified
 * direction. It is defined by its definition point (code 10, 20, 30), text point (code 11, 21, 31), and the two
 * definition points (code 13, 23, 33 and code 14, 24, 34). The aligned dimension can also include properties such as
 * the dimension style (code 3), attachment point (code 71), text line spacing style (code 72), dimension text (code 1),
 * text line spacing factor (code 41), rotation angle of the dimension text (code 53), and extrusion direction (code
 * 210, 220, 230), which can affect how it is rendered in the drawing.
 */
class EoDxfAlignedDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfAlignedDimension() { m_entityType = EoDxf::DIMALIGNED; }
  EoDxfAlignedDimension(const EoDxfDimension& d) : EoDxfDimension(d) { m_entityType = EoDxf::DIMALIGNED; }

  EoDxfGeometryBase3d getClonepoint() const { return getPt2(); }
  void setClonePoint(EoDxfGeometryBase3d c) { setPt2(c); }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 private:
  EoDxfGeometryBase3d m_firstDefinitinPointForLinearAndAngularDimensions;  // Group codes 13, 23 & 33 (WCS)
  EoDxfGeometryBase3d m_secondDefinitinPointForLinearAndAngularDimensions;  // Group code 14, 24 & 34 (WCS)
};

/** @brief Class to handle linear dimension entity
 *
 *  A linear dimension is a type of dimension that measures the distance between two points along a specified direction.
 *  It is defined by its definition point (code 10, 20, 30), text point (code 11, 21, 31), and the two definition points
 * (code 13, 23, 33 and code 14, 24, 34). The linear dimension can also include properties such as the dimension style
 * (code 3), attachment point (code 71), text line spacing style (code 72), dimension text (code 1), text line spacing
 * factor (code 41), rotation angle of the dimension text (code 53), and extrusion direction (code 210, 220, 230),
 * which can affect how it is rendered in the drawing. Additionally, linear dimensions can have an oblique angle
 * (code 52) that specifies the angle of the dimension line relative to the horizontal plane.
 */
class EoDxfDimLinear : public EoDxfDimension {
 public:
  EoDxfDimLinear() { m_entityType = EoDxf::DIMLINEAR; }
  EoDxfDimLinear(const EoDxfDimension& dimension) : EoDxfDimension(dimension) { m_entityType = EoDxf::DIMLINEAR; }

  double getAngle() const { return m_angleOfRotatedHorizontalOrVerticalDimensions; }
  double getOblique() const { return m_angleOfExtensionLines; }  // oblique angle, code 52

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 private:
  EoDxfGeometryBase3d m_firstDefinitinPointForLinearAndAngularDimensions;  // Group codes 13, 23 & 33 (WCS)
  EoDxfGeometryBase3d m_secondDefinitinPointForLinearAndAngularDimensions;  // Group code 14, 24 & 34 (WCS)
  double m_angleOfRotatedHorizontalOrVerticalDimensions{};  // Group code 50
  double m_angleOfExtensionLines{};  // Group code 52
};

/** @brief Class to handle radial dimension entity
 *
 *  A radial dimension is a type of dimension that measures the radius of a circle or arc.
 *  It is defined by its center point (code 10, 20, 30), text point (code 11, 21, 31), and the definition point for the
 * radius (code 15, 25, 35). The radial dimension can also include properties such as the dimension style (code 3),
 * attachment point (code 71), text line spacing style (code 72), dimension text (code 1), text line spacing factor
 * (code 41), rotation angle of the dimension text (code 53), and extrusion direction (code 210, 220, 230), which can
 * affect how it is rendered in the drawing.
 */
class EoDxfRadialDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfRadialDimension() { m_entityType = EoDxf::DIMRADIAL; }
  EoDxfRadialDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) { m_entityType = EoDxf::DIMRADIAL; }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 private:
  EoDxfGeometryBase3d m_definitionPointForDiameterRadiusAndAngularDimensions;  // Group codes 15, 25 & 35 (WCS)
  double m_leaderLengthForRadiusAndDiameterDimensions{};  // Group code 40
};

/** @brief Class to handle diametric dimension entity
 *
 *  A diametric dimension is a type of dimension that measures the diameter of a circle or arc.
 *  It is defined by its center point (code 10, 20, 30), text point (code 11, 21, 31), and the definition point for the
 * diameter (code 15, 25, 35). The diametric dimension can also include properties such as the dimension style (code 3),
 * attachment point (code 71), text line spacing style (code 72), dimension text (code 1), text line spacing factor
 * (code 41), rotation angle of the dimension text (code 53), and extrusion direction (code 210, 220, 230), which can
 * affect how it is rendered in the drawing.
 */
class EoDxfDiametricDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfDiametricDimension() { m_entityType = EoDxf::DIMDIAMETRIC; }
  EoDxfDiametricDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMDIAMETRIC;
  }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 private:
  EoDxfGeometryBase3d m_definitionPointForDiameterRadiusAndAngularDimensions;  // Group codes 15, 25 & 35 (WCS)
  double m_leaderLengthForRadiusAndDiameterDimensions{};  // Leader length, code 40
};

/** @brief Class to handle 2 line angular dimension entity
 *
 *  A 2 line angular dimension is a type of dimension that measures the angle between two lines.
 *  It is defined by its two definition points for the lines (code 13, 23, 33 and code 14, 24, 34), and the dimension
 * definition point (code 16, 26, 36). The 2 line angular dimension can also include properties such as the dimension
 * style (code 3), attachment point (code 71), text line spacing style (code 72), dimension text (code 1), text line
 * spacing factor (code 41), rotation angle of the dimension text (code 53), and extrusion direction (code 210, 220,
 * 230), which can affect how it is rendered in the drawing.
 */
class EoDxf2LineAngularDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxf2LineAngularDimension() { m_entityType = EoDxf::DIMANGULAR; }
  EoDxf2LineAngularDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMANGULAR;
  }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 private:
  EoDxfGeometryBase3d m_firstDefinitinPointForLinearAndAngularDimensions;  // Group codes 13, 23 & 33 (WCS)
  EoDxfGeometryBase3d m_secondDefinitinPointForLinearAndAngularDimensions;  // Group code 14, 24 & 34 (WCS)
  EoDxfGeometryBase3d m_definitionPointForDiameterRadiusAndAngularDimensions;  // Group codes 15, 25 & 35 (WCS)
  EoDxfGeometryBase3d m_pointDefiningDimensionArcForAngularDimensions;  // Group codes 16, 26 & 36 (OCS)
};

/** @brief Class to handle 3 point angular dimension entity
 *
 *  A 3 point angular dimension is a type of dimension that measures the angle between three points.
 *  It is defined by its vertex point (code 15, 25, 35), the two definition points for the lines (code 13, 23, 33 and
 * code 14, 24, 34), and the dimension definition point (code 10, 20, 30). The 3 point angular dimension can also
 * include properties such as the dimension style (code 3), attachment point (code 71), text line spacing style (code
 * 72), dimension text (code 1), text line spacing factor (code 41), rotation angle of the dimension text (code 53), and
 * extrusion direction (code 210, 220, 230), which can affect how it is rendered in the drawing.
 */
class EoDxf3PointAngularDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxf3PointAngularDimension() { m_entityType = EoDxf::DIMANGULAR3P; }
  EoDxf3PointAngularDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMANGULAR3P;
  }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 private:
  EoDxfGeometryBase3d m_firstDefinitinPointForLinearAndAngularDimensions;  // Group codes 13, 23 & 33 (WCS)
  EoDxfGeometryBase3d m_secondDefinitinPointForLinearAndAngularDimensions;  // Group code 14, 24 & 34 (WCS)
  EoDxfGeometryBase3d m_definitionPointForDiameterRadiusAndAngularDimensions;  // Group codes 15, 25 & 35 (WCS)
  EoDxfGeometryBase3d m_pointDefiningDimensionArcForAngularDimensions;  // Group codes 16, 26 & 36 (OCS)
};

/** @brief Class to handle ordinate dimension entity
 *
 *  An ordinate dimension is a type of dimension that measures the distance from a specified origin point to a feature
 * location point along a specified direction. It is defined by its origin point (code 10, 20, 30), the feature location
 * point (code 13, 23, 33), and the leader end point (code 14, 24, 34). The ordinate dimension can also include
 * properties such as the dimension style (code 3), attachment point (code 71), text line spacing style (code 72),
 * dimension text (code 1), text line spacing factor (code 41), rotation angle of the dimension text (code 53), and
 * extrusion direction (code 210, 220, 230), which can affect how it is rendered in the drawing.
 */
class EoDxfOrdinateDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfOrdinateDimension() { m_entityType = EoDxf::DIMORDINATE; }
  EoDxfOrdinateDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMORDINATE;
  }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 private:
  EoDxfGeometryBase3d m_firstDefinitinPointForLinearAndAngularDimensions;  // Group codes 13, 23 & 33 (WCS)
  EoDxfGeometryBase3d m_secondDefinitinPointForLinearAndAngularDimensions;  // Group code 14, 24 & 34 (WCS)
};
