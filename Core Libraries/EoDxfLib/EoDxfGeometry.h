#pragma once

#include <cmath>

#include "EoDxfBase.h"

/** @brief Class representing a 2D coordinate with x and y components.
 *  Used for DXF group code pairs that are inherently 2D (e.g., snap base, grid spacing, view center in DCS).
 */
class EoDxfGeometryBase2d {
 public:
  double x{};
  double y{};

  EoDxfGeometryBase2d() noexcept = default;

  constexpr EoDxfGeometryBase2d(double x, double y) noexcept : x{x}, y{y} {}

  EoDxfGeometryBase2d(const EoDxfGeometryBase2d&) noexcept = default;
  EoDxfGeometryBase2d& operator=(const EoDxfGeometryBase2d&) noexcept = default;

  EoDxfGeometryBase2d(EoDxfGeometryBase2d&&) noexcept = default;
  EoDxfGeometryBase2d& operator=(EoDxfGeometryBase2d&&) noexcept = default;

  /** @brief Checks if this coordinate is effectively equal to another coordinate within a specified tolerance.
   *
   * This method compares the x and y components of this coordinate with those of another EoDxfGeometryBase2d
   * instance. It determines if the absolute difference between each corresponding component is less than a specified
   * tolerance value. This is useful for geometric calculations where exact equality may not be achievable due to
   * floating-point precision limitations. The default tolerance is defined by EoDxf::geometricTolerance, but it can be
   * overridden by providing a different value when calling the method.
   *
   * @param other The other EoDxfGeometryBase2d instance to compare against.
   * @param tolerance The distance within which the components of the two coordinates are considered equal.
   * @return true if the absolute difference between each corresponding component (x, y) of the two coordinates is
   * less than the specified tolerance; otherwise, false.
   */
  [[nodiscard]] bool IsEqualTo(
      const EoDxfGeometryBase2d& other, double tolerance = EoDxf::geometricTolerance) const noexcept {
    return std::abs(x - other.x) < tolerance && std::abs(y - other.y) < tolerance;
  }

  /** @brief Checks if the coordinate is effectively zero within a specified tolerance.
   *
   * This method determines if the x and y components of the coordinate are all within a certain distance
   * (tolerance) from zero. This is useful for geometric calculations where exact zero may not be achievable due to
   * floating-point precision limitations. The default tolerance is defined by EoDxf::geometricTolerance, but it can
   * be overridden by providing a different value when calling the method.
   *
   * @param tolerance The distance from zero within which the coordinate components are considered effectively zero.
   * @return true if all components (x, y) are within the specified tolerance of zero; otherwise, false.
   */
  [[nodiscard]] bool IsZero(double tolerance = EoDxf::geometricTolerance) const noexcept {
    return std::abs(x) < tolerance && std::abs(y) < tolerance;
  }
};

/** @brief Class representing a generalized 3D coordinate (using for point and vector) with x, y, and z components.
 *  Provides constructors for initialization and a method to unitize the vector.
 */
class EoDxfGeometryBase3d {
 public:
  double x{};
  double y{};
  double z{};

  EoDxfGeometryBase3d() noexcept = default;

  EoDxfGeometryBase3d(double x, double y, double z) noexcept : x{x}, y{y}, z{z} {}

  EoDxfGeometryBase3d(const EoDxfGeometryBase3d&) noexcept = default;
  EoDxfGeometryBase3d& operator=(const EoDxfGeometryBase3d&) noexcept = default;

  EoDxfGeometryBase3d(EoDxfGeometryBase3d&&) noexcept = default;
  EoDxfGeometryBase3d& operator=(EoDxfGeometryBase3d&&) noexcept = default;

  /** @brief Checks if the coordinate is effectively the default normal vector (0.0, 0.0, 1.0) within a specified
   * tolerance.
   *
   * This method determines if the x and y components of the coordinate are within a certain distance (tolerance) from
   * zero, and if the z component is within a certain distance from 1.0. This is useful for geometric calculations
   * where the default normal vector may not be exactly (0.0, 0.0, 1.0) due to floating-point precision limitations. The
   * default tolerance is defined by EoDxf::geometricTolerance, but it can be overridden by providing a different value
   * when calling the method.
   *
   * @param tolerance The distance from zero for x and y, and from one for z, within which the coordinate is considered
   * effectively the default normal vector.
   * @return true if x and y are within the specified tolerance of zero and z is within the specified tolerance of one;
   * otherwise, false.
   */
  [[nodiscard]] bool IsDefaultNormal(double tolerance = EoDxf::geometricTolerance) const noexcept {
    return std::abs(x) < tolerance && std::abs(y) < tolerance && std::abs(z - 1.0) < tolerance;
  }

  /** @brief Checks if this coordinate is effectively equal to another coordinate within a specified tolerance.
   *
   * This method compares the x, y, and z components of this coordinate with those of another EoDxfGeometryBase3d
   * instance. It determines if the absolute difference between each corresponding component is less than a specified
   * tolerance value. This is useful for geometric calculations where exact equality may not be achievable due to
   * floating-point precision limitations. The default tolerance is defined by EoDxf::geometricTolerance, but it can be
   * overridden by providing a different value when calling the method.
   *
   * @param other The other EoDxfGeometryBase3d instance to compare against.
   * @param tolerance The distance within which the components of the two coordinates are considered equal.
   * @return true if the absolute difference between each corresponding component (x, y, z) of the two coordinates is
   * less than the specified tolerance; otherwise, false.
   */
  [[nodiscard]] bool IsEqualTo(
      const EoDxfGeometryBase3d& other, double tolerance = EoDxf::geometricTolerance) const noexcept {
    return std::abs(x - other.x) < tolerance && std::abs(y - other.y) < tolerance && std::abs(z - other.z) < tolerance;
  }

  /** @brief Checks if the coordinate is effectively zero within a specified tolerance.
   *
   * This method determines if the x, y, and z components of the coordinate are all within a certain distance
   * (tolerance) from zero. This is useful for geometric calculations where exact zero may not be achievable due to
   * floating-point precision limitations. The default tolerance is defined by EoDxf::geometricTolerance, but it can be
   * overridden by providing a different value when calling the method.
   *
   * @param tolerance The distance from zero within which the coordinate components are considered effectively zero.
   * @return true if all components (x, y, z) are within the specified tolerance of zero; otherwise, false.
   */
  [[nodiscard]] bool IsZero(double tolerance = EoDxf::geometricTolerance) const noexcept {
    return std::abs(x) < tolerance && std::abs(y) < tolerance && std::abs(z) < tolerance;
  }

  /** @brief Convert `this` coordinate to a unit-length vector (in-place).
   */
  void Unitize() noexcept {
    const double dist = sqrt(x * x + y * y + z * z);

    if (dist > EoDxf::geometricTolerance) {
      x = x / dist;
      y = y / dist;
      z = z / dist;
    }
  }

  /** @brief Return a unitized copy of `this` coordinate.
   *  This method creates a copy of the current EoDxfGeometryBase3d instance, applies the Unitize() method to the copy,
   *  and returns the unitized copy. The original coordinate remains unchanged.
   *  @return A new EoDxfGeometryBase3d instance that is a unitized version of the original coordinate.
   */
  [[nodiscard]] EoDxfGeometryBase3d Unitized() const noexcept {
    EoDxfGeometryBase3d result{*this};
    result.Unitize();
    return result;
  }
};

/** @brief Class representing a vertex of a lightweight polyline, with coordinates and optional width and bulge
 * properties. The vertex is defined by its x and y coordinates (group codes 10 and 20), optional start and end widths
 * (group codes 40 and 41), and an optional bulge value (group code 42) that defines the curvature between this vertex
 * and the next one in the polyline.
 *
 * The z field carries the WCS Z coordinate after ApplyExtrusion() (for non-default extrusion) or the shared
 * LWPOLYLINE elevation (group code 38) for default extrusion. It is not a DXF group code — it is computed
 * during import to avoid losing the elevation/extrusion Z component.
 */
struct EoDxfPolylineVertex2d {
  double x{};  // x coordinate, code 10
  double y{};  // y coordinate, code 20
  double z{};  // WCS z (computed from elevation/extrusion, not a DXF group code)
  double stawidth{};  // Start width, code 40
  double endwidth{};  // End width, code 41
  double bulge{};  // bulge, code 42

  /** @brief Default-constructed vertex (all zero). */
  EoDxfPolylineVertex2d() noexcept = default;

  /** @brief Position + bulge constructor (widths stay zero).
   * Matches the most common usage in DXF parsers.
   */
  constexpr EoDxfPolylineVertex2d(double x, double y, double bulge = {}) noexcept
      : x{x}, y{y}, z{}, stawidth{}, endwidth{}, bulge{bulge} {}
};
