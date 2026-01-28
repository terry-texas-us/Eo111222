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
  if (fabs(t) > Eo::geometricTolerance) {
    x /= t;
    y /= t;
    z /= t;
  }
}
void EoGePoint3d::operator()(double xNew, double yNew, double zNew) {
  x = xNew;
  y = yNew;
  z = zNew;
}

[[nodiscard]] EoGeVector3d EoGePoint3d::operator-(const EoGePoint3d& q) const {
  return EoGeVector3d(q.x - x, q.y - y, q.z - z);
}

[[nodiscard]] EoGePoint3d EoGePoint3d::operator-(const EoGeVector3d& vector) const {
  return EoGePoint3d(x - vector.x, y - vector.y, z - vector.z);
}
[[nodiscard]] EoGePoint3d EoGePoint3d::operator+(const EoGeVector3d& vector) const {
  return EoGePoint3d(x + vector.x, y + vector.y, z + vector.z);
}
[[nodiscard]] EoGePoint3d EoGePoint3d::operator*(double t) const { return EoGePoint3d(x * t, y * t, z * t); }

[[nodiscard]] EoGePoint3d EoGePoint3d::operator/(double t) const {
  if (fabs(t) > Eo::geometricTolerance) { return EoGePoint3d(x / t, y / t, z / t); }
  return EoGePoint3d(x, y, z);
}

double EoGePoint3d::DistanceTo(const EoGePoint3d& point) const {
  double X = point.x - x;
  double Y = point.y - y;
  double Z = point.z - z;
  return sqrt(X * X + Y * Y + Z * Z);
}
[[nodiscard]] bool EoGePoint3d::IsEqualTo(const EoGePoint3d& pt, double tolerance) const {
  return (fabs(x - pt.x) > tolerance || fabs(y - pt.y) > tolerance || fabs(z - pt.z) > tolerance ? false : true);
}
bool EoGePoint3d::IsContained(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const {
  double dReps = Eo::geometricTolerance + fabs(Eo::geometricTolerance * x);

  if (lowerLeftPoint.x > x + dReps || upperRightPoint.x < x - dReps) return false;

  dReps = Eo::geometricTolerance + fabs(Eo::geometricTolerance * y);
  if (lowerLeftPoint.y > y + dReps || upperRightPoint.y < y - dReps) return false;

  return true;
}
[[nodiscard]] EoGePoint3d EoGePoint3d::Max(const EoGePoint3d& a, const EoGePoint3d& b) {
  return EoGePoint3d(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

EoGePoint3d EoGePoint3d::Mid(const EoGePoint3d& a, const EoGePoint3d& b) { return a + (a - b) * 0.5; }

[[nodiscard]] EoGePoint3d EoGePoint3d::Min(const EoGePoint3d& a, const EoGePoint3d& b) {
  return EoGePoint3d(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

EoGePoint3d EoGePoint3d::ProjectToward(const EoGePoint3d& b, const double distance) {
  EoGeVector3d directionVector(*this, b);

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
EoGePoint3d EoGePoint3d::RotateAboutAxis(const EoGePoint3d& referenceOrigin, const EoGeVector3d& referenceAxis, const double angle) {
  if (referenceAxis == EoGeVector3d::positiveUnitZ) {
    double SinAng = sin(angle);
    double CosAng = cos(angle);

    EoGeVector3d v(referenceOrigin, *this);

    return (EoGePoint3d(referenceOrigin.x + (v.x * CosAng - v.y * SinAng), referenceOrigin.y + (v.x * SinAng + v.y * CosAng), z));
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

double EoGePoint3d::Distance(const EoGePoint3d& a, const EoGePoint3d& b) {
  double x = b.x - a.x;
  double y = b.y - a.y;
  double z = b.z - a.z;
  return sqrt(x * x + y * y + z * z);
}
