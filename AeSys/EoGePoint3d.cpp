#include "Stdafx.h"

#include <algorithm>
#include <cmath>

#include "Eo.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

const EoGePoint3d EoGePoint3d::kOrigin{0.0, 0.0, 0.0};

EoGePoint3d::EoGePoint3d(const EoGePoint4d& initialPoint) {
  x = initialPoint.x / initialPoint.w;
  y = initialPoint.y / initialPoint.w;
  z = initialPoint.z / initialPoint.w;
}

bool EoGePoint3d::operator==(const EoGePoint3d& point) const { return (IsEqualTo(point, Eo::geometricTolerance)); }

bool EoGePoint3d::operator!=(const EoGePoint3d& point) const { return (!IsEqualTo(point, Eo::geometricTolerance)); }

void EoGePoint3d::operator+=(const EoGeVector3d& vector) {
  x += vector.x;
  y += vector.y;
  z += vector.z;
}

void EoGePoint3d::operator-=(const EoGeVector3d& vector) {
  x -= vector.x;
  y -= vector.y;
  z -= vector.z;
}

void EoGePoint3d::operator*=(double t) {
  x *= t;
  y *= t;
  z *= t;
}

void EoGePoint3d::operator/=(double t) {
  assert(fabs(t) > Eo::geometricTolerance && "Division by near-zero in EoGePoint3d::operator/=");

  if (fabs(t) > Eo::geometricTolerance) {
    x /= t;
    y /= t;
    z /= t;
  }
  // Silently unchanged in release if t ≈ 0
}

void EoGePoint3d::operator()(double xNew, double yNew, double zNew) {
  x = xNew;
  y = yNew;
  z = zNew;
}

[[nodiscard]] EoGeVector3d EoGePoint3d::operator-(const EoGePoint3d& p) const {
  return EoGeVector3d(x - p.x, y - p.y, z - p.z);
}

[[nodiscard]] EoGePoint3d EoGePoint3d::operator-(const EoGeVector3d& u) const {
  return EoGePoint3d(x - u.x, y - u.y, z - u.z);
}
[[nodiscard]] EoGePoint3d EoGePoint3d::operator+(const EoGeVector3d& u) const {
  return EoGePoint3d(x + u.x, y + u.y, z + u.z);
}
[[nodiscard]] EoGePoint3d EoGePoint3d::operator*(double t) const { return EoGePoint3d(x * t, y * t, z * t); }

[[nodiscard]] EoGePoint3d EoGePoint3d::operator/(double t) const {
  assert(fabs(t) > Eo::geometricTolerance && "Division by near-zero in EoGePoint3d::operator/");

  if (fabs(t) > Eo::geometricTolerance) { return EoGePoint3d(x / t, y / t, z / t); }
  return EoGePoint3d(x, y, z);
}

double EoGePoint3d::DistanceTo(const EoGePoint3d& p) const {
  double xDelta = p.x - x;
  double yDelta = p.y - y;
  double zDelta = p.z - z;
  return sqrt(xDelta * xDelta + yDelta * yDelta + zDelta * zDelta);
}

[[nodiscard]] bool EoGePoint3d::IsEqualTo(const EoGePoint3d& p, double tolerance) const {
  return fabs(x - p.x) <= tolerance && fabs(y - p.y) <= tolerance && fabs(z - p.z) <= tolerance;
}

bool EoGePoint3d::IsContained(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const {
  if (lowerLeftPoint.x > x + Eo::geometricTolerance || upperRightPoint.x < x - Eo::geometricTolerance) { return false; }
  if (lowerLeftPoint.y > y + Eo::geometricTolerance || upperRightPoint.y < y - Eo::geometricTolerance) { return false; }
  return true;
}

[[nodiscard]] EoGePoint3d EoGePoint3d::Max(const EoGePoint3d& p, const EoGePoint3d& q) {
  return EoGePoint3d(std::max(p.x, q.x), std::max(p.y, q.y), std::max(p.z, q.z));
}

EoGePoint3d EoGePoint3d::Mid(const EoGePoint3d& p, const EoGePoint3d& q) { return p + (q - p) * 0.5; }

[[nodiscard]] EoGePoint3d EoGePoint3d::Min(const EoGePoint3d& p, const EoGePoint3d& q) {
  return EoGePoint3d(std::min(p.x, q.x), std::min(p.y, q.y), std::min(p.z, q.z));
}

EoGePoint3d EoGePoint3d::ProjectToward(const EoGePoint3d& p, const double distance) {
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
  return (returnValue);
}

EoGePoint3d EoGePoint3d::RotateAboutAxis(const EoGePoint3d& referenceOrigin, const EoGeVector3d& referenceAxis,
                                         const double angle) {
  if (referenceAxis == EoGeVector3d::positiveUnitZ) {
    double sinAngle = sin(angle);
    double cosAngle = cos(angle);

    EoGeVector3d v(referenceOrigin, *this);

    return (EoGePoint3d(referenceOrigin.x + (v.x * cosAngle - v.y * sinAngle),
                        referenceOrigin.y + (v.x * sinAngle + v.y * cosAngle), z));
  } else {
    EoGeTransformMatrix tm(referenceOrigin, referenceAxis, angle);
    return (tm * (*this));
  }
}

CString EoGePoint3d::ToString() const {
  CString str;
  str.Format(L"%f;%f;%f\t", x, y, z);
  return (str);
}

void EoGePoint3d::Write(CFile& file) const {
  file.Write(&x, sizeof(double));
  file.Write(&y, sizeof(double));
  file.Write(&z, sizeof(double));
}

double EoGePoint3d::Distance(const EoGePoint3d& p, const EoGePoint3d& q) {
  double x = q.x - p.x;
  double y = q.y - p.y;
  double z = q.z - p.z;
  return sqrt(x * x + y * y + z * z);
}
