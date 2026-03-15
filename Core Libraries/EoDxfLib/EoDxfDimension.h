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
    angle = oblique = 0.0;
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
    def1 = other.def1;
    def2 = other.def2;
    angle = other.angle;
    oblique = other.oblique;
    arcPoint = other.arcPoint;
    circlePoint = other.circlePoint;
    length = other.length;
  }
  virtual ~EoDxfDimension() = default;

  virtual void ApplyExtrusion() {}

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  [[nodiscard]] EoDxfGeometryBase3d GetDefinitionPoint() const noexcept { return m_definitionPoint; }
  void setDefPoint(const EoDxfGeometryBase3d p) { m_definitionPoint = p; }
  EoDxfGeometryBase3d getTextPoint() const { return m_middlePointOfDimensionText; }
  [[nodiscard]] const std::wstring& GetDimensionStyleName() const { return m_dimensionStyleName; }
  [[nodiscard]] std::int16_t GetAttachmentPoint() const noexcept { return m_attachmentPoint; }
  [[nodiscard]] std::int16_t getTextLineStyle() const noexcept { return m_dimensionTextLineSpacingStyle; }
  [[nodiscard]] const std::wstring& GetExplicitDimensionText() const { return m_explicitDimensionText; }
  double GetDimensionTextLineSpacingFactor() const { return m_dimensionTextLineSpacingFactor; }
  double GetRotationAngleAwayFromDefault() const { return m_rotationAngleAwayFromDefault; }

  const std::wstring& getName() const {
    return m_nameOfBlockContainer;
  }  // Name of the block that contains the entities, code 2

 protected:
  EoDxfGeometryBase3d getPt2() const { return clonePoint; }
  void setPt2(const EoDxfGeometryBase3d p) { clonePoint = p; }
  EoDxfGeometryBase3d getPt3() const { return def1; }
  EoDxfGeometryBase3d getPt4() const { return def2; }
  EoDxfGeometryBase3d getPt5() const { return circlePoint; }
  EoDxfGeometryBase3d getPt6() const { return arcPoint; }
  void setPt6(const EoDxfGeometryBase3d p) { arcPoint = p; }
  double getAn50() const { return angle; }  // Angle of rotated, horizontal, or vertical dimensions, code 50
  double getOb52() const { return oblique; }  // oblique angle, code 52
  double getRa40() const { return length; }  // Leader length, code 40

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

  EoDxfGeometryBase3d clonePoint;  // Insertion point for clones (Baseline & Continue), code 12, 22 & 32 (OCS)
  EoDxfGeometryBase3d def1;  // Definition point 1for linear & angular, code 13, 23 & 33 (WCS)
  EoDxfGeometryBase3d def2;  // Definition point 2, code 14, 24 & 34 (WCS)

  double angle;  // Angle of rotated, horizontal, or vertical dimensions, code 50
  double oblique;  // oblique angle, code 52

  EoDxfGeometryBase3d circlePoint;  // Definition point for diameter, radius & angular dims code 15, 25 & 35 (WCS)
  EoDxfGeometryBase3d arcPoint;  // Point defining dimension arc, x coordinate, code 16, 26 & 36 (OCS)
  double length{};  // Leader length, code 40
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

  EoDxfGeometryBase3d getClonepoint() const {
    return getPt2();
  }  // Insertion for clones (Baseline & Continue), 12, 22 & 32
  void setClonePoint(EoDxfGeometryBase3d c) { setPt2(c); }

  EoDxfGeometryBase3d getDimPoint() const { return GetDefinitionPoint(); }
  void setDimPoint(const EoDxfGeometryBase3d p) { setDefPoint(p); }
  EoDxfGeometryBase3d getDef1Point() const { return getPt3(); }  // Definition point 1, code 13, 23 & 33
  EoDxfGeometryBase3d getDef2Point() const { return getPt4(); }  // Definition point 2, code 14, 24 & 34
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

  double getAngle() const { return getAn50(); }  // Angle of rotated, horizontal, or vertical dimensions, code 50
  double getOblique() const { return getOb52(); }  // oblique angle, code 52
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

  EoDxfGeometryBase3d getDiameterPoint() const { return getPt5(); }  // Definition point for radius, code 15, 25 & 35
  double getLeaderLength() const { return getRa40(); }  // Leader length, code 40
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

  EoDxfGeometryBase3d getDiameter1Point() const {
    return getPt5();
  }  // First definition point for diameter, code 15, 25 & 35
  double getLeaderLength() const { return getRa40(); }  // Leader length, code 40
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

  EoDxfGeometryBase3d getFirstLine1() const { return getPt3(); }  // Definition point line 1-1, code 13, 23 & 33
  EoDxfGeometryBase3d getFirstLine2() const { return getPt4(); }  // Definition point line 1-2, code 14, 24 & 34
  EoDxfGeometryBase3d getSecondLine1() const { return getPt5(); }  // Definition point line 2-1, code 15, 25 & 35
  EoDxfGeometryBase3d getDimPoint() const { return getPt6(); }  // Dimension definition point, code 16, 26 & 36
  void setDimPoint(const EoDxfGeometryBase3d p) { setPt6(p); }
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

  EoDxfGeometryBase3d getFirstLine() const { return getPt3(); }  // Definition point line 1, code 13, 23 & 33
  EoDxfGeometryBase3d getSecondLine() const { return getPt4(); }  // Definition point line 2, code 14, 24 & 34
  EoDxfGeometryBase3d getVertexPoint() const { return getPt5(); }  // Vertex point, code 15, 25 & 35
  EoDxfGeometryBase3d getDimPoint() const { return GetDefinitionPoint(); }
  void setDimPoint(const EoDxfGeometryBase3d p) { setDefPoint(p); }
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

  EoDxfGeometryBase3d getFirstLine() const { return getPt3(); }  // Feature location point, code 13, 23 & 33
  EoDxfGeometryBase3d getSecondLine() const { return getPt4(); }  // Leader end point, code 14, 24 & 34
};
