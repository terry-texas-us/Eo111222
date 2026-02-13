#pragma once

#include "EoGeVector3d.h"

class EoGePoint3d;
class EoGePoint4d;

typedef CArray<EoGePoint4d, EoGePoint4d&> EoGePoint4dArray;

class EoGePoint4d {
 public:
  double x;
  double y;
  double z;
  double w;

 public:  // Constructors and destructor
  EoGePoint4d();
  EoGePoint4d(const double initialX, const double initialY, const double initialZ, const double initialW);
  EoGePoint4d(const EoGePoint3d& initialPoint);

 public:  // Operators
  void operator+=(const EoGeVector3d& v);
  void operator-=(const EoGeVector3d& v);
  void operator*=(const double t);
  void operator/=(const double t);

  [[nodiscard]] EoGePoint4d operator+(const EoGeVector3d& vector) const;
  [[nodiscard]] EoGePoint4d operator-(const EoGeVector3d& vector) const;
  [[nodiscard]] EoGePoint4d operator*(const double t) const;
  [[nodiscard]] EoGePoint4d operator/(const double t) const;

  void operator()(const double x, const double y, const double z, const double w);

  [[nodiscard]] EoGeVector3d operator-(const EoGePoint4d& ptQ) const;

 public:  // Methods
  /// <summary>Determines the xy distance between two points.</summary>
  double DistanceToPointXY(const EoGePoint4d& ptQ) const;
  /// <summary>Performs a containment test on a point.</summary>
  bool IsInView();

 public:
  static bool ClipLine(EoGePoint4d& ptA, EoGePoint4d& ptB);
  /// <summary>Sutherland-hodgman-like polygon clip by view volume.</summary>
  static void ClipPolygon(EoGePoint4dArray& pointsArray);
  /// <summary>Sutherland-hodgman-like polygon clip by clip plane.</summary>
  /// <remarks>Visibility determined using dot product.</remarks>
  static void IntersectionWithPln(EoGePoint4dArray& pointsArrayIn, const EoGePoint4d& pointOnClipPlane,
                                  EoGeVector3d& clipPlaneNormal, EoGePoint4dArray& pointsArrayOut);
  static EoGePoint4d Max(EoGePoint4d& ptA, EoGePoint4d& ptB);
  static EoGePoint4d Min(EoGePoint4d& ptA, EoGePoint4d& ptB);
};
