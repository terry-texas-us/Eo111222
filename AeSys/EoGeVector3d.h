#pragma once

#include "Eo.h"

class CFile;
class EoGePoint3d;

class EoGeVector3d {
 public:
  double x{};
  double y{};
  double z{};

  constexpr EoGeVector3d() noexcept = default;
  constexpr EoGeVector3d(double x, double y, double z) noexcept : x{x}, y{y}, z{z} {}

  EoGeVector3d(const EoGePoint3d& p, const EoGePoint3d& q) noexcept;

  [[nodiscard]] bool operator==(const EoGeVector3d& vector) const noexcept;
  [[nodiscard]] bool operator!=(const EoGeVector3d& vector) const noexcept;

  constexpr void operator+=(const EoGeVector3d& v) noexcept {
    x += v.x;
    y += v.y;
    z += v.z;
  }

  constexpr void operator-=(const EoGeVector3d& v) noexcept {
    x -= v.x;
    y -= v.y;
    z -= v.z;
  }

  constexpr void operator*=(double t) noexcept {
    x *= t;
    y *= t;
    z *= t;
  }

  void operator/=(double t);

  [[nodiscard]] EoGeVector3d operator-() const noexcept { return EoGeVector3d(-x, -y, -z); }

  [[nodiscard]] constexpr EoGeVector3d operator-(const EoGeVector3d& v) const noexcept {
    return {x - v.x, y - v.y, z - v.z};
  }

  [[nodiscard]] constexpr EoGeVector3d operator+(const EoGeVector3d& v) const noexcept {
    return {x + v.x, y + v.y, z + v.z};
  }

  [[nodiscard]] constexpr EoGeVector3d operator*(double t) const noexcept { return {x * t, y * t, z * t}; }

  [[nodiscard]] EoGeVector3d operator/(double t) const;

  /** @brief Checks if the vector is a null vector. */
  [[nodiscard]] constexpr bool IsNearNull() const noexcept {
    return (SquaredLength() < Eo::geometricTolerance * Eo::geometricTolerance);
  }

  [[nodiscard]] bool IsEqualTo(const EoGeVector3d& other, double tolerance = Eo::geometricTolerance) const noexcept;

  [[nodiscard]] double Length() const;

  /** @brief Normalizes the vector to a unit vector.
   * If the vector is a null vector, it remains unchanged.
   */
  void Normalize();

  /** @brief Rotates the vector about an arbitrary axis by a given angle.
   * @param axis The axis to rotate about (should be a unit vector).
   * @param angle The angle in radians.
   */
  void RotateAboutArbitraryAxis(const EoGeVector3d& axis, double angle);

  /** @brief Returns the square of the length of the vector.
   * @return The squared length.
   */
  [[nodiscard]] constexpr double SquaredLength() const noexcept { return (x * x + y * y + z * z); }

  [[nodiscard]] CString ToString() const;

  [[nodiscard]] std::string ToStringStd() const { return std::format("{:.6f};{:.6f};{:.6f}", x, y, z); }

  void Read(CFile& file);
  void Write(CFile& file) const;

  constexpr void Get(double& xOut, double& yOut, double& zOut) const noexcept {
    xOut = x;
    yOut = y;
    zOut = z;
  }

  constexpr EoGeVector3d& Set(double xNew, double yNew, double zNew) noexcept {
    x = xNew;
    y = yNew;
    z = zNew;
    return *this;
  }

  static const EoGeVector3d positiveUnitX;
  static const EoGeVector3d positiveUnitY;
  static const EoGeVector3d positiveUnitZ;
};

/** @brief Compute a not so arbitrary axis for AutoCAD entities
* @param normal Normal vector
* @return Computed arbitrary axis
*/
[[nodiscard]] EoGeVector3d ComputeArbitraryAxis(const EoGeVector3d& normal);

/** @brief Rotates a vector about the Z axis by a given angle
* @param vector Vector to rotate
* @param angle Angle in radians
* @return Rotated vector
*/
[[nodiscard]] EoGeVector3d RotateVectorAboutZAxis(const EoGeVector3d& vector, double angle);

/** @brief Computes the cross product of the two vectors
* @param vector1 First vector
* @param vector2 Second vector
* @return Cross product vector
*/
[[nodiscard]] constexpr EoGeVector3d CrossProduct(const EoGeVector3d& vector1, const EoGeVector3d& vector2) noexcept;

/** @brief Computes the scalar product (= inner product) of the two vectors.
 * @param u First vector
 * @param v Second vector
 * @return Dot product
 * @note In the Euclidean space there is a strong relationship between the dot product and lengths and angles.
 * For a vector a, a • a is the square of its length, and, more generally, if b is another vector
 * @code
 * u • v = |u| |v| cos(θ) where |u| and |v| denote the length (magnitude) of u and v and θ is the angle between them.
 * Since |u| cos(θ) is the scalar projection of u onto v, the dot product can be understood geometrically as the product of this projection with the length of v.
 * As the cosine of 90° is zero, the dot product of two perpendicular vectors is always zero. 
   If u and v have length one (they are unit vectors), the dot product simply gives the cosine of the angle between them. 
   Thus, given two vectors, the angle between them can be found by rearranging the above formula:
     θ = arccos(u • v / |u| |v|)
 * @endcode
 */
[[nodiscard]] constexpr double DotProduct(const EoGeVector3d& u, const EoGeVector3d& v) noexcept {
  return (u.x * v.x + u.y * v.y + u.z * v.z);
}

/** @brief Checks if two vectors are nearly equal within a geometric tolerance.
 * @param vector1 First vector
 * @param vector2 Second vector
 * @return true if the vectors are nearly equal, false otherwise
 */
[[nodiscard]] constexpr bool IsNearEqual(const EoGeVector3d& vector1, const EoGeVector3d& vector2) noexcept {
  return (vector1 - vector2).IsNearNull();
}
