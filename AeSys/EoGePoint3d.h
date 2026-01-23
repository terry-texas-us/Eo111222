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

 public:  // Constructors and destructor
  EoGePoint3d() : x(0.0), y(0.0), z(0.0) {}
  EoGePoint3d(double xInitial, double yInitial, double zInitial) : x(xInitial), y(yInitial), z(zInitial) {}
  EoGePoint3d(const EoGePoint4d& initialPoint);
  EoGePoint3d(const DRW_Coord& initialPoint) : x(initialPoint.x), y(initialPoint.y), z(initialPoint.z) {}

 public:  // Operators
  bool operator==(const EoGePoint3d& point) const;
  bool operator!=(const EoGePoint3d& point) const;
  void operator+=(const EoGeVector3d& vector);
  void operator-=(const EoGeVector3d& vector);
  void operator*=(double t);
  void operator/=(double t);

  void operator()(double xNew, double yNew, double zNew);
  EoGeVector3d operator-(const EoGePoint3d& ptQ) const;

  EoGePoint3d operator-(const EoGeVector3d& vector) const;
  EoGePoint3d operator+(const EoGeVector3d& vector) const;
  EoGePoint3d operator*(double t) const;
  EoGePoint3d operator/(double t) const;

 public:  // Methods
  /// <summary>Determines the distance to a point without the z component.</summary>
  double DistanceTo(const EoGePoint3d& point) const;
  bool IsEqualTo(const EoGePoint3d& point, double tolerance) const;
  /// <summary>Determines if a point is contained by a window.</summary>
  /// <returns>true if point is in window, false otherwise</returns>
  bool IsContained(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const;
  /// <summary>Determines relationship of a point to a window.</summary>
  // Returns:
  //		0 - point is contained in window
  //		bit 1 set - point above window
  //		bit 2 set - point below window
  //		bit 4 set - point to the right of window
  //		bit 8 set - point to the left of window
  int RelationshipToRectangle(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const;
  /// <summary>Projects a point toward or beyond another point.</summary>
  /// <param name="ptQ">point defining direction vector</param>
  /// <param name="distance">magnitude of projection</param>
  /// <returns> projected point or itself if points coincide</returns>
  EoGePoint3d ProjectToward(const EoGePoint3d& ptQ, const double distance);
  /// <summary>Rotates a point about another point and arbitrary axis in space.</summary>
  /// <param name="referenceOrigin">point about which rotation will occur</param>
  /// <param name="referenceAxis">unit vector defining rotation axis</param>
  /// <param name="angle">rotation angle (ccw positive) in radians</param>
  /// <returns>Point after rotation</returns>
  EoGePoint3d RotateAboutAxis(const EoGePoint3d& referenceOrigin, const EoGeVector3d& referenceAxis, const double angle);
  CString ToString() const;
  void Read(CFile& file);
  void Write(CFile& file) const;

 public:
  static const EoGePoint3d kOrigin;

  static EoGePoint3d Max(EoGePoint3d& ptA, EoGePoint3d& ptB);
  static EoGePoint3d Mid(EoGePoint3d& ptA, EoGePoint3d& ptB);
  static EoGePoint3d Min(EoGePoint3d& ptA, EoGePoint3d& ptB);
};
