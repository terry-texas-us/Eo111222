#pragma once

#include "drw_base.h"

class EoGePoint3d;
class EoGePoint4d;
class EoGeVector3d;

typedef CArray<EoGePoint3d, const EoGePoint3d&> EoGePoint3dArray;

class EoGePoint3d {
 public:
  double x;
  double y;
  double z;

 public:
  constexpr EoGePoint3d() : x(0.0), y(0.0), z(0.0) {}
  constexpr EoGePoint3d(double xInitial, double yInitial, double zInitial) : x(xInitial), y(yInitial), z(zInitial) {}
  EoGePoint3d(const EoGePoint4d& initialPoint);
  EoGePoint3d(const DRW_Coord& initialPoint) : x(initialPoint.x), y(initialPoint.y), z(initialPoint.z) {}

 public:

  /** Tests equality within geometric tolerance.
   * @param point The point to compare against.
   * @return true if points are equal within Eo::geometricTolerance.
   */
  bool operator==(const EoGePoint3d& point) const;
  bool operator!=(const EoGePoint3d& point) const;
  void operator+=(const EoGeVector3d& vector);
  void operator-=(const EoGeVector3d& vector);
  void operator*=(double t);
  void operator/=(double t);

  void operator()(double xNew, double yNew, double zNew);
  
  /** Subtracts another point from this point, resulting in a vector.
   * @param p The point to subtract from this point.
   * @return The vector resulting from the subtraction.
   */
  [[nodiscard]] EoGeVector3d operator-(const EoGePoint3d& p) const;
  
  [[nodiscard]] EoGePoint3d operator-(const EoGeVector3d& u) const;
  [[nodiscard]] EoGePoint3d operator+(const EoGeVector3d& u) const;
  [[nodiscard]] EoGePoint3d operator*(double t) const;
  
  [[nodiscard]] EoGePoint3d operator/(double t) const;

 public:
  /**Determines the distance to another point in 3D space.
   * @param p The target point to measure the distance to.
   * @return The Euclidean distance between this point and the target point.
   */ 
  double DistanceTo(const EoGePoint3d& p) const;
  
  [[nodiscard]] bool IsEqualTo(const EoGePoint3d& p, double tolerance) const;
  
  /** Determines if a point is contained by a window.
   * @param lowerLeftPoint The lower-left corner point of the window.
   * @param upperRightPoint The upper-right corner point of the window.
   * @return true if point is in window, false otherwise
   */
  bool IsContained(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const;
  
  /** Determines the relationship of this point to a rectangle defined by two corner points.
   * @param lowerLeftPoint The lower-left corner point of the rectangle.
   * @param upperRightPoint The upper-right corner point of the rectangle.
   * @return An integer code representing the relationship:
   *         0x0000 - Inside the rectangle
   *         0x0001 - Above the rectangle
   *         0x0010 - Below the rectangle
   *         0x0100 - Right of the rectangle
   *         0x1000 - Left of the rectangle
   *         Combinations of these values indicate multiple relationships (e.g., 0x0101 - above and right).
   */
  [[nodiscard]] int RelationshipToRectangle(const EoGePoint3d& lowerLeftPoint,
                                            const EoGePoint3d& upperRightPoint) const;
  /**
   * Projects this point toward or beyond point p by the specified distance.
   *
   * @param p The target point defining direction vector to project toward.
   * @param distance The magnitude of the projection.
   * @return The projected point or itself if the points coincide.
   */
  EoGePoint3d ProjectToward(const EoGePoint3d& p, const double distance) const;
  
  /** Rotates a point about another point and arbitrary axis in space.
   * @param referenceOrigin Point about which rotation will occur.
   * @param referenceAxis Unit vector defining rotation axis.
   * @param angle Rotation angle (ccw positive) in radians.
   * @return Point after rotation.
   */
  EoGePoint3d RotateAboutAxis(const EoGePoint3d& referenceOrigin, const EoGeVector3d& referenceAxis,
                              const double angle);
  CString ToString() const;
  void Read(CFile& file);
  void Write(CFile& file) const;

 public:
  static const EoGePoint3d kOrigin;
  static double Distance(const EoGePoint3d& p, const EoGePoint3d& q);

  [[nodiscard]] static EoGePoint3d Max(const EoGePoint3d& p, const EoGePoint3d& q);

  static EoGePoint3d Mid(const EoGePoint3d& p, const EoGePoint3d& q);

  [[nodiscard]] static EoGePoint3d Min(const EoGePoint3d& p, const EoGePoint3d& q);
};
