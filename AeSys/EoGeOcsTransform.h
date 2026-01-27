#pragma once

#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

/** @brief Transformation matrix for DXF Object Coordinate System (OCS) conversions
 * 
 * Handles transformations between Object Coordinate System (OCS) and World Coordinate System (WCS)
 * as defined by the DXF arbitrary axis algorithm. This is commonly used for entities like 
 * circles, arcs, ellipses, and other 2D entities with an extrusion direction in DXF files.
 * 
 * The transformation is built using the arbitrary axis algorithm which constructs an 
 * orthonormal basis (u, v, n) from the extrusion normal vector.
 */
class EoGeOcsTransform : public EoGeTransformMatrix {
 public:
  /** @brief Default constructor - creates identity transformation (Z-axis extrusion) */
  EoGeOcsTransform() : EoGeTransformMatrix(), m_extrusionNormal(EoGeVector3d::positiveUnitZ) {}

  /** @brief Constructs OCS transformation from extrusion normal (OCS to WCS)
   *  @param extrusionNormal The extrusion direction defining the OCS Z-axis
   *  @note Uses the DXF arbitrary axis algorithm to construct the transformation
   */
  explicit EoGeOcsTransform(const EoGeVector3d& extrusionNormal);

  /** @brief Constructs OCS transformation with origin and extrusion normal
   *  @param origin The origin point of the OCS in WCS coordinates
   *  @param extrusionNormal The extrusion direction defining the OCS Z-axis
   */
  EoGeOcsTransform(const EoGePoint3d& origin, const EoGeVector3d& extrusionNormal);

  /** @brief Copy constructor from base transformation matrix
   *  @param matrix Base transformation matrix to copy
   */
  EoGeOcsTransform(const EoGeTransformMatrix& matrix)
      : EoGeTransformMatrix(matrix), m_extrusionNormal(EoGeVector3d::positiveUnitZ) {}

 public:
  /** @brief Gets the extrusion normal (OCS Z-axis) */
  EoGeVector3d GetExtrusionNormal() const { return m_extrusionNormal; }

  /** @brief Sets a new extrusion normal and rebuilds the transformation
   *  @param extrusionNormal New extrusion direction
   */
  void SetExtrusionNormal(const EoGeVector3d& extrusionNormal);

  /** @brief Gets the OCS X-axis direction in WCS */
  EoGeVector3d GetOcsXAxis() const;

  /** @brief Gets the OCS Y-axis direction in WCS */
  EoGeVector3d GetOcsYAxis() const;

  /** @brief Gets the OCS Z-axis direction in WCS (same as extrusion normal) */
  EoGeVector3d GetOcsZAxis() const { return m_extrusionNormal; }

  /** @brief Creates the inverse transformation (WCS to OCS)
   *  @return Transformation matrix for converting WCS coordinates to OCS
   */
  EoGeOcsTransform GetInverseOcsTransform() const;

  /** @brief Checks if this is a standard WCS transformation (extrusion = 0,0,1)
   *  @param tolerance Tolerance for comparison
   *  @return true if extrusion normal equals positive Z-axis
   */
  bool IsWorldCoordinateSystem(double tolerance = Eo::geometricTolerance) const;

  /** @brief Static factory: Creates OCS to WCS transformation
   *  @param extrusionNormal The extrusion direction
   *  @return Transformation matrix for OCS to WCS conversion
   */
  static EoGeOcsTransform CreateOcsToWcs(const EoGeVector3d& extrusionNormal);

  /** @brief Static factory: Creates WCS to OCS transformation
   *  @param extrusionNormal The extrusion direction
   *  @return Transformation matrix for WCS to OCS conversion
   */
  static EoGeOcsTransform CreateWcsToOcs(const EoGeVector3d& extrusionNormal);

 private:
  /** @brief Builds the transformation matrix using the arbitrary axis algorithm
   *  @param extrusionNormal The extrusion direction (will be normalized)
   */
  void BuildOcsTransformation(const EoGeVector3d& extrusionNormal);

  EoGeVector3d m_extrusionNormal;  ///< The OCS Z-axis (extrusion direction)
};

typedef CList<EoGeOcsTransform> EoGeOcsTransformList;