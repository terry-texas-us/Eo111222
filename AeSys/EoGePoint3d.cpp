#include "Stdafx.h"

#include <cmath>

#include "Eo.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

const EoGePoint3d EoGePoint3d::kOrigin{0.0, 0.0, 0.0};

EoGePoint3d::EoGePoint3d(const EoGePoint4d& point) noexcept
    : x{point.x / point.w}, y{point.y / point.w}, z{point.z / point.w} {}

bool EoGePoint3d::operator==(const EoGePoint3d& point) const noexcept {
  return (IsEqualTo(point, Eo::geometricTolerance));
}

bool EoGePoint3d::operator!=(const EoGePoint3d& point) const noexcept {
  return (!IsEqualTo(point, Eo::geometricTolerance));
}

void EoGePoint3d::operator/=(double t) {
  assert(std::abs(t) > Eo::geometricTolerance && "Division by near-zero in EoGePoint3d::operator/=");

  if (std::abs(t) > Eo::geometricTolerance) {
    x /= t;
    y /= t;
    z /= t;
  }
  // Silently unchanged in release if t ≈ 0
}

EoGePoint3d EoGePoint3d::operator/(double t) const {
  assert(std::abs(t) > Eo::geometricTolerance && "Division by near-zero in EoGePoint3d::operator/");

  if (std::abs(t) > Eo::geometricTolerance) { return EoGePoint3d(x / t, y / t, z / t); }
  return EoGePoint3d(x, y, z);
}

double EoGePoint3d::DistanceTo(const EoGePoint3d& p) const noexcept {
  double xDelta = p.x - x;
  double yDelta = p.y - y;
  double zDelta = p.z - z;
  return std::sqrt(xDelta * xDelta + yDelta * yDelta + zDelta * zDelta);
}

bool EoGePoint3d::IsEqualTo(const EoGePoint3d& p, double tolerance) const noexcept {
  return std::abs(x - p.x) <= tolerance && std::abs(y - p.y) <= tolerance && std::abs(z - p.z) <= tolerance;
}

bool EoGePoint3d::IsContained(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const {
  if (lowerLeftPoint.x > x + Eo::geometricTolerance || upperRightPoint.x < x - Eo::geometricTolerance) { return false; }
  if (lowerLeftPoint.y > y + Eo::geometricTolerance || upperRightPoint.y < y - Eo::geometricTolerance) { return false; }
  return true;
}

EoGePoint3d EoGePoint3d::ProjectToward(const EoGePoint3d& p, double distance) const {
  EoGeVector3d directionVector(*this, p);

  double length = directionVector.Length();
  if (length > Eo::geometricTolerance) {
    directionVector *= distance / length;
    return (*this + directionVector);
  }
  return (*this);
}

void EoGePoint3d::Read(CFile& file) {
  file.Read(&x, sizeof(double));
  file.Read(&y, sizeof(double));
  file.Read(&z, sizeof(double));
}

int EoGePoint3d::RelationshipToRectangle(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const {
  int returnValue{};

  if (y > upperRightPoint.y + Eo::geometricTolerance) {
    returnValue = 1;
  } else if (y < lowerLeftPoint.y - Eo::geometricTolerance) {
    returnValue = 2;
  }

  if (x > upperRightPoint.x + Eo::geometricTolerance) {
    returnValue |= 4;
  } else if (x < lowerLeftPoint.x - Eo::geometricTolerance) {
    returnValue |= 8;
  }
  return returnValue;
}

EoGePoint3d EoGePoint3d::RotateAboutAxis(
    const EoGePoint3d& referenceOrigin, const EoGeVector3d& referenceAxis, double angle) {
  if (referenceAxis == EoGeVector3d::positiveUnitZ) {
    double sinAngle = std::sin(angle);
    double cosAngle = std::cos(angle);

    EoGeVector3d v(referenceOrigin, *this);

    return (EoGePoint3d(referenceOrigin.x + (v.x * cosAngle - v.y * sinAngle),
        referenceOrigin.y + (v.x * sinAngle + v.y * cosAngle), z));
  } else {
    EoGeTransformMatrix transformMatrix(referenceOrigin, referenceAxis, angle);
    return (transformMatrix * (*this));
  }
}

[[nodiscard]] CString EoGePoint3d::ToString() const {
  CString str;
  str.Format(L"%f;%f;%f\t", x, y, z);
  return str;
}

void EoGePoint3d::Write(CFile& file) const {
  file.Write(&x, sizeof(double));
  file.Write(&y, sizeof(double));
  file.Write(&z, sizeof(double));
}

double EoGePoint3d::Distance(const EoGePoint3d& p, const EoGePoint3d& q) noexcept {
  double x = q.x - p.x;
  double y = q.y - p.y;
  double z = q.z - p.z;
  return std::sqrt(x * x + y * y + z * z);
}
