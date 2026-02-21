#pragma once

#include <format>

#include "EoGeVector3d.h"
#include "drw_base.h"

class EoGePoint3d;
class EoGePoint4d;

typedef CArray<EoGePoint3d, const EoGePoint3d&> EoGePoint3dArray;

class EoGePoint3d {
 public:
  double x{};
  double y{};
  double z{};

  constexpr EoGePoint3d() noexcept = default;
  constexpr EoGePoint3d(double x, double y, double z) noexcept : x{x}, y{y}, z{z} {}

  constexpr explicit EoGePoint3d(const DRW_Coord& point) noexcept : x{point.x}, y{point.y}, z{point.z} {}

  explicit EoGePoint3d(const EoGePoint4d& point) noexcept;

  /** Tests equality within geometric tolerance.
   * @param point The point to compare against.
   * @return true if points are equal within Eo::geometricTolerance.
   */
  bool operator==(const EoGePoint3d& point) const noexcept;
  bool operator!=(const EoGePoint3d& point) const noexcept;

  /** Translates `this` point by the given vector.
   * @param v The vector to add to `this` point.
   */
  constexpr void operator+=(const EoGeVector3d& v) noexcept {
    x += v.x;
    y += v.y;
    z += v.z;
  }

  /** Translates `this` point by the negative of the given vector.
   * @param v The vector to subtract from `this` point.
   */
  constexpr void operator-=(const EoGeVector3d& v) noexcept {
    x -= v.x;
    y -= v.y;
    z -= v.z;
  }

  /** Scales `this` point by the given factor.
   * @param t The factor to scale `this` point by.
   */
  constexpr void operator*=(double t) noexcept {
    x *= t;
    y *= t;
    z *= t;
  }

  void operator/=(double t);

  /** Sets the coordinates of `this` point to the given values.
   * @param xNew The new x-coordinate.
   * @param yNew The new y-coordinate.
   * @param zNew The new z-coordinate.
   */
  constexpr EoGePoint3d& Set(double xNew, double yNew, double zNew) noexcept {
    x = xNew;
    y = yNew;
    z = zNew;
    return *this;
  }

  /** Subtracts another point from `this` point, resulting in a vector.
   * @param p The point to subtract from `this` point.
   * @return The vector resulting from the subtraction.
   */
  [[nodiscard]] constexpr EoGeVector3d operator-(const EoGePoint3d& p) const noexcept {
    return {x - p.x, y - p.y, z - p.z};
  }

  /** Subtracts a vector from `this` point, resulting in a new point.
   * @param u The vector to subtract from `this` point.
   * @return The new point resulting from the subtraction.
   */
  [[nodiscard]] constexpr EoGePoint3d operator-(const EoGeVector3d& u) const noexcept {
    return {x - u.x, y - u.y, z - u.z};
  }

  /** Adds a vector to `this` point, resulting in a new point.
   * @param u The vector to add to `this` point.
   * @return The new point resulting from the addition.
   */
  [[nodiscard]] constexpr EoGePoint3d operator+(const EoGeVector3d& u) const noexcept {
    return {x + u.x, y + u.y, z + u.z};
  }

  [[nodiscard]] constexpr EoGePoint3d operator*(double t) const noexcept { return {x * t, y * t, z * t}; }

  [[nodiscard]] EoGePoint3d operator/(double t) const;

  /**Determines the distance to another point in 3D space.
   * @param p The target point to measure the distance to.
   * @return The Euclidean distance between this point and the target point.
   */
  [[nodiscard]] double DistanceTo(const EoGePoint3d& p) const noexcept;

  [[nodiscard]] bool IsEqualTo(
      const EoGePoint3d& p, double tolerance = Eo::geometricTolerance) const noexcept;

  /** Determines if a point is contained by a window.
   * @param lowerLeftPoint The lower-left corner point of the window.
   * @param upperRightPoint The upper-right corner point of the window.
   * @return true if point is in window, false otherwise
   */
  [[nodiscard]] bool IsContained(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const;

  /** Determines the relationship of this point to a rectangle defined by two corner points.
   * @param lowerLeftPoint The lower-left corner point of the rectangle.
   * @param upperRightPoint The upper-right corner point of the rectangle.
   * @return An integer code representing the relationship:
   *         0 - Inside the rectangle
   *         1 - Above the rectangle
   *         2 - Below the rectangle
   *         4 - Right of the rectangle
   *         8 - Left of the rectangle
   *         Combinations of these values indicate multiple relationships (e.g., 5 - above and right).
   */
  [[nodiscard]] int RelationshipToRectangle(
      const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const;
  /**
   * Projects this point toward or beyond point p by the specified distance.
   *
   * @param p The target point defining direction vector to project toward.
   * @param distance The magnitude of the projection.
   * @return The projected point or itself if the points coincide.
   */
  [[nodiscard]] EoGePoint3d ProjectToward(const EoGePoint3d& p, double distance) const;

  /** Rotates a point about another point and arbitrary axis in space.
   * @param referenceOrigin Point about which rotation will occur.
   * @param referenceAxis Unit vector defining rotation axis.
   * @param angle Rotation angle (ccw positive) in radians.
   * @return Point after rotation.
   */
  [[nodiscard]] EoGePoint3d RotateAboutAxis(
      const EoGePoint3d& referenceOrigin, const EoGeVector3d& referenceAxis, double angle);

  [[nodiscard]] CString ToString() const;

  [[nodiscard]] std::string ToStringStd() const { return std::format("{:.6f};{:.6f};{:.6f}", x, y, z); }

  void Read(CFile& file);
  void Write(CFile& file) const;

  static const EoGePoint3d kOrigin;
  [[nodiscard]] static double Distance(const EoGePoint3d& p, const EoGePoint3d& q) noexcept;

  [[nodiscard]] static constexpr EoGePoint3d Max(const EoGePoint3d& p, const EoGePoint3d& q) noexcept {
    return {std::max(p.x, q.x), std::max(p.y, q.y), std::max(p.z, q.z)};
  }

  [[nodiscard]] static constexpr EoGePoint3d Mid(const EoGePoint3d& p, const EoGePoint3d& q) noexcept {
    return p + (q - p) * 0.5;
  }

  [[nodiscard]] static constexpr EoGePoint3d Min(const EoGePoint3d& p, const EoGePoint3d& q) noexcept {
    return {std::min(p.x, q.x), std::min(p.y, q.y), std::min(p.z, q.z)};
  }
};
