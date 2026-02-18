#pragma once

#include <cassert>

#include "EoGeVector3d.h"

class EoGePoint3d;
class EoGePoint4d;

typedef CArray<EoGePoint4d, EoGePoint4d&> EoGePoint4dArray;

class EoGePoint4d {
 public:
  double x{};
  double y{};
  double z{};
  double w{};

  constexpr EoGePoint4d() noexcept = default;
  constexpr EoGePoint4d(double x, double y, double z, double w) : x{x}, y{y}, z{z}, w{w} {}

  explicit EoGePoint4d(const EoGePoint3d& initialPoint) noexcept
      : x{initialPoint.x}, y{initialPoint.y}, z{initialPoint.z}, w{1.0} {}

  void operator+=(const EoGeVector3d& v);
  void operator-=(const EoGeVector3d& v);
  void operator*=(double t);
  void operator/=(double t);

  [[nodiscard]] EoGePoint4d operator+(const EoGeVector3d& vector) const;
  [[nodiscard]] EoGePoint4d operator-(const EoGeVector3d& vector) const;
  [[nodiscard]] EoGePoint4d operator*(double t) const;
  [[nodiscard]] EoGePoint4d operator/(double t) const;

  void operator()(double x, double y, double z, double w);

  [[nodiscard]] EoGeVector3d operator-(const EoGePoint4d& ptQ) const;

  /** @brief Converts a point from homogeneous coordinates to Cartesian coordinates by dividing the x, y, and z components by the w component.
   * @param homogenizedPoint The point in homogeneous coordinates to be dehomogenized.
   * @param homoginizedW The homogeneous coordinate `w` of the point, used for perspective division. It is expected that `w` is not zero.
   * @return An EoGePoint3d representing the dehomogenized point in Cartesian coordinates.
   * @note This function assumes that the input point is in homogeneous coordinates (x, y, z, w) and that `w` is not zero.
   */
  [[nodiscard]] EoGePoint3d Dehomogenize() const noexcept {
    assert(std::abs(w) > Eo::geometricTolerance && "w is too close to zero for dehomogenization");
    return {x / w, y / w, z / w};
  }

  /** Determines the distance to another point 'q', ignoring z component.
   * @param q The target point to measure the distance to.
   * @return The Euclidean distance between `this` point and the targetpoint, calculated using only the x and y components.
   */
  double DistanceToPointXY(const EoGePoint4d& q) const;

  /** @brief Determines if the point is within the view volume defined by the clipping planes.
   @return true if the point is within the view volume; false otherwise.
   */
  bool IsInView();

  static bool ClipLine(EoGePoint4d& a, EoGePoint4d& b);

  /** @brief Clips a polygon defined by an array of points against the view volume defined by the clipping planes.
   * The function modifies the input array to contain only the points of the polygon that are within the view volume,
   * effectively performing polygon clipping.
   * @param pointsArray An array of EoGePoint4d objects representing the vertices of the polygon to be clipped.
   * The array is modified in place to contain only the vertices that are within the view volume after clipping.
   */
  static void ClipPolygon(EoGePoint4dArray& pointsArray);

  /** @brief Computes the intersection points of a polygon with a clipping plane defined by a point and a normal vector.
   * This function is typically used as part of the polygon clipping process, where it calculates the intersection points between the edges of the polygon and the clipping plane.
   * The resulting intersection points are added to the output array, which can then be used to construct the clipped polygon.
   * @param pointsArrayIn An array of EoGePoint4d objects representing the vertices of the polygon before clipping.
   * @param pointOnClipPlane A point that lies on the clipping plane, used to define the plane's position in space.
   * @param clipPlaneNormal A normal vector that defines the orientation of the clipping plane. The direction of this vector determines which side of the plane is considered "inside" for clipping purposes.
   * @param pointsArrayOut An array that will receive the intersection points calculated by this function. The caller is responsible for managing this array and ensuring it has sufficient capacity to hold the results.
   */
  static void IntersectionWithPln(EoGePoint4dArray& pointsArrayIn, const EoGePoint4d& pointOnClipPlane,
      EoGeVector3d& clipPlaneNormal, EoGePoint4dArray& pointsArrayOut);

  static EoGePoint4d Max(EoGePoint4d& a, EoGePoint4d& b);

  static EoGePoint4d Min(EoGePoint4d& a, EoGePoint4d& b);
};
