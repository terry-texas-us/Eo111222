#pragma once

#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

class EoDbCharacterCellDefinition;

/** @brief Represents a 3D reference system defined by an origin point and two orthogonal direction vectors (X and Y).
 * The reference system is used for positioning and orienting text in the drawing.
 */
class EoGeReferenceSystem {
  EoGePoint3d m_origin{EoGePoint3d::kOrigin};
  EoGeVector3d m_xDirection{1.0, 0.0, 0.0};
  EoGeVector3d m_yDirection{0.0, 1.0, 0.0};

 public:
  EoGeReferenceSystem() = default;

  /** @brief Constructs a reference system for text based on the current view and character cell definition.
   *  @param origin The origin point of the reference system.
   *  @param characterCellDefinition The character cell definition containing height, rotation angle, slant angle, and expansion factor.
   */
  EoGeReferenceSystem(const EoGePoint3d& origin, const EoDbCharacterCellDefinition& characterCellDefinition);
  EoGeReferenceSystem(const EoGePoint3d& origin, EoGeVector3d xDirection, EoGeVector3d yDirection);

  EoGeReferenceSystem(const EoGeReferenceSystem&) = default;
  EoGeReferenceSystem& operator=(const EoGeReferenceSystem&) = default;

  ~EoGeReferenceSystem() = default;

  [[nodiscard]] EoGePoint3d Origin() const noexcept { return m_origin; }

  /** @brief Returns the transformation matrix that transforms points from the reference system's local coordinate system to the world coordinate system.
   */
  [[nodiscard]] EoGeTransformMatrix TransformMatrix() const;
  
  /** @brief Calculates the unit normal vector of the reference system.
   *  @return The unit normal vector of the reference system.
   */
  [[nodiscard]] EoGeVector3d UnitNormal() const;
  
  [[nodiscard]] EoGeVector3d XDirection() const noexcept { return m_xDirection; }
  [[nodiscard]] EoGeVector3d YDirection() const noexcept { return m_yDirection; }

  /** Rescales the reference system based on the specified character cell definition.
   * The X and Y directions are adjusted according to the height, expansion factor, rotation angle, and slant angle defined in the character cell definition.
   * @param characterCellDefinition The character cell definition containing the parameters for rescaling the reference system.
   */
  void Rescale(const EoDbCharacterCellDefinition& characterCellDefinition);
  
  void SetOrigin(const EoGePoint3d& origin) { m_origin = origin; }
  void SetXDirection(const EoGeVector3d& xDirection) { m_xDirection = xDirection; }
  void SetYDirection(const EoGeVector3d& yDirection) { m_yDirection = yDirection; }

  void Transform(const EoGeTransformMatrix& transformMatrix);

  void Read(CFile& file);

  void Write(CFile& file);
};
