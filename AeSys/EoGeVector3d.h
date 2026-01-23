#pragma once

#include <afxstr.h>

#include "Eo.h"

class CFile;
class EoGePoint3d;

class EoGeVector3d {
 public:
  double x;
  double y;
  double z;

  constexpr EoGeVector3d() noexcept : x(0.0), y(0.0), z(0.0) {}

  EoGeVector3d(double xInitial, double yInitial, double zInitial);

  EoGeVector3d(const EoGePoint3d& pointP, const EoGePoint3d& pointQ);

  bool operator==(const EoGeVector3d& vector) const noexcept;
  bool operator!=(const EoGeVector3d& vector) const noexcept;
  void operator+=(const EoGeVector3d& vector) noexcept;
  void operator-=(const EoGeVector3d& vector) noexcept;
  void operator*=(double t) noexcept;
  void operator/=(double t);

  void operator()(double xNew, double yNew, double zNew) noexcept;
  EoGeVector3d operator-() const noexcept;
  EoGeVector3d operator-(const EoGeVector3d& vector) const noexcept;
  EoGeVector3d operator+(const EoGeVector3d& vector) const noexcept;

  EoGeVector3d operator*(double t) const noexcept;

  /** @brief Checks if the vector is a null vector. */
  constexpr bool IsNearNull() const noexcept { return (SquaredLength() < Eo::lengthEpsilon * Eo::lengthEpsilon); }

  [[nodiscard]] double Length() const;

  /** @brief Normalizes the vector to a unit vector.
   * If the vector is a null vector, it remains unchanged.
   */
  void Normalize();

  /** @brief Rotates the vector about an arbitrary axis by a given angle.
   * @param axis The axis to rotate about (should be a unit vector).
   * @param angle The angle in radians.
   */
  void RotAboutArbAx(const EoGeVector3d& axis, double angle);

  /** @brief Returns the square of the length of the vector.
   * @return The squared length.
   */
  constexpr double SquaredLength() const noexcept { return (x * x + y * y + z * z); }

  CString ToString() const;
  void Read(CFile& file);
  void Write(CFile& file) const;

  void Get(double& xOut, double& yOut, double& zOut) const noexcept {
    xOut = x;
    yOut = y;
    zOut = z;
  }

  void Set(double xNew, double yNew, double zNew) noexcept {
    x = xNew;
    y = yNew;
    z = zNew;
  }

  static const EoGeVector3d positiveUnitX;
  static const EoGeVector3d positiveUnitY;
  static const EoGeVector3d positiveUnitZ;
};

/** @brief Compute a not so arbitrary axis for AutoCAD entities
* @param normal Normal vector
* @return Computed arbitrary axis
*/
EoGeVector3d ComputeArbitraryAxis(const EoGeVector3d& normal);

/** @brief Rotates a vector about the Z axis by a given angle
* @param vector Vector to rotate
* @param angle Angle in radians
* @return Rotated vector
*/
EoGeVector3d RotateVectorAboutZAxis(const EoGeVector3d& vector, double angle);

/** @brief Computes the cross product of the two vectors
* @param vector1 First vector
* @param vector2 Second vector
* @return Cross product vector
*/
EoGeVector3d EoGeCrossProduct(const EoGeVector3d& vector1, const EoGeVector3d& vector2);

/** @brief Computes the scalar product (= inner product) of the two vectors.
 * @param vector1 First vector
 * @param vector2 Second vector
 * @return Dot product
 * @note In the Euclidean space there is a strong relationship between the dot product and lengths and angles.
 * For a vector a, a • a is the square of its length, and, more generally, if b is another vector
 * @code
 * a • b = |a| |b| cos(θ) where |a| and |b| denote the length (magnitude) of a and b and θ is the angle between them.
 * Since |a| cos(θ) is the scalar projection of a onto b, the dot product can be understood geometrically as the product of this projection with the length of b.
 * As the cosine of 90° is zero, the dot product of two perpendicular vectors is always zero. 
   If a and b have length one (they are unit vectors), the dot product simply gives the cosine of the angle between them. 
   Thus, given two vectors, the angle between them can be found by rearranging the above formula:
     θ = arccos(a • b / |a| |b|)
 * @endcode
 */
double EoGeDotProduct(const EoGeVector3d& vector1, const EoGeVector3d& vector2);

constexpr bool EoGeNearEqual(const EoGeVector3d& vector1, const EoGeVector3d& vector2) noexcept {
  return (vector1 - vector2).IsNearNull();
}
