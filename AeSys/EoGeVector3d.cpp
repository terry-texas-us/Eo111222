#include "Stdafx.h"

#include <cmath>
#include <stdexcept>

#include "Eo.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

const EoGeVector3d EoGeVector3d::positiveUnitX(1.0, 0.0, 0.0);
const EoGeVector3d EoGeVector3d::positiveUnitY(0.0, 1.0, 0.0);
const EoGeVector3d EoGeVector3d::positiveUnitZ(0.0, 0.0, 1.0);

EoGeVector3d::EoGeVector3d(const EoGePoint3d& p, const EoGePoint3d& q) : x(q.x - p.x), y(q.y - p.y), z(q.z - p.z) {}

bool EoGeVector3d::operator==(const EoGeVector3d& v) const noexcept { return IsNearEqual(*this, v); }

bool EoGeVector3d::operator!=(const EoGeVector3d& v) const noexcept { return !IsNearEqual(*this, v); }

void EoGeVector3d::operator+=(const EoGeVector3d& v) noexcept {
  x += v.x;
  y += v.y;
  z += v.z;
}

void EoGeVector3d::operator-=(const EoGeVector3d& v) noexcept {
  x -= v.x;
  y -= v.y;
  z -= v.z;
}

void EoGeVector3d::operator*=(double t) noexcept {
  x *= t;
  y *= t;
  z *= t;
}

/** @brief Divides the vector by a scalar value.
 * @param t The scalar value to divide the vector by.
 */
void EoGeVector3d::operator/=(double t) {
  assert(fabs(t) > Eo::geometricTolerance && "Division by near-zero in EoGeVector3d::operator/=");

  if (fabs(t) > Eo::geometricTolerance) {
    x /= t;
    y /= t;
    z /= t;
  }
  // Silently unchanged in release if t ≈ 0
}

void EoGeVector3d::operator()(double xNew, double yNew, double zNew) noexcept {
  x = xNew;
  y = yNew;
  z = zNew;
}

EoGeVector3d EoGeVector3d::operator-(const EoGeVector3d& v) const noexcept {
  return EoGeVector3d(x - v.x, y - v.y, z - v.z);
}

EoGeVector3d EoGeVector3d::operator+(const EoGeVector3d& v) const noexcept {
  return EoGeVector3d(x + v.x, y + v.y, z + v.z);
}

EoGeVector3d EoGeVector3d::operator/(double t) const {
  assert(fabs(t) > Eo::geometricTolerance && "Division by near-zero in EoGeVector3d::operator/");

  if (fabs(t) > Eo::geometricTolerance) { return EoGeVector3d(x / t, y / t, z / t); }
  return *this;  // Return unchanged on near-zero
}

/** @brief Determines if this vector is equal to another vector within a specified tolerance.
 * @param other The other vector to compare with.
 * @param tolerance The tolerance within which the vectors are considered equal.
 * @return true if the vectors are equal within the specified tolerance; false otherwise.
 */
bool EoGeVector3d::IsEqualTo(const EoGeVector3d& other, double tolerance) const noexcept {
  return fabs(x - other.x) <= tolerance && fabs(y - other.y) <= tolerance && fabs(z - other.z) <= tolerance;
}

double EoGeVector3d::Length() const { return sqrt(SquaredLength()); }

void EoGeVector3d::Normalize() {
  if (IsNearNull()) { throw std::domain_error("Cannot normalize zero-length vector"); }
  *this /= Length();
}

void EoGeVector3d::Read(CFile& file) {
  file.Read(&x, sizeof(double));
  file.Read(&y, sizeof(double));
  file.Read(&z, sizeof(double));
}

void EoGeVector3d::RotAboutArbAx(const EoGeVector3d& referenceAxis, double angle) {
  if (referenceAxis == positiveUnitZ) {
    *this = RotateVectorAboutZAxis(*this, angle);
  } else {
    EoGeTransformMatrix transformMatrix(EoGePoint3d::kOrigin, referenceAxis, angle);

    *this = transformMatrix * (*this);
  }
}

CString EoGeVector3d::ToString() const {
  CString str;
  str.Format(L"%f;%f;%f\t", x, y, z);
  return str;
}

void EoGeVector3d::Write(CFile& file) const {
  file.Write(&x, sizeof(double));
  file.Write(&y, sizeof(double));
  file.Write(&z, sizeof(double));
}

/** @brief Computes an arbitrary axis vector that is perpendicular to the given normal vector.
 * @param normal The normal vector to which the arbitrary axis will be perpendicular.
 * @return An arbitrary axis vector perpendicular to the normal vector.
 * @note The arbitrary axis algorithm is used by AutoCAD internally to implement the arbitrary but consistent generation 
 * of object coordinate systems for all entities that use object coordinates.
 */
EoGeVector3d ComputeArbitraryAxis(const EoGeVector3d& normal) {
  // AutoCAD's arbitrary axis algorithm uses 1/64 as the threshold (See: AutoCAD DXF Reference - Arbitrary Axis Algorithm)
  constexpr double epsilon{1.0 / 64.0};  // AutoCAD compatibility - do not change

  EoGeVector3d arbitraryAxis;
  if ((fabs(normal.x) < epsilon) && (fabs(normal.y) < epsilon)) {
    arbitraryAxis = CrossProduct(EoGeVector3d::positiveUnitY, normal);
  } else {
    arbitraryAxis = CrossProduct(EoGeVector3d::positiveUnitZ, normal);
  }
  return arbitraryAxis;
}

EoGeVector3d RotateVectorAboutZAxis(const EoGeVector3d& vector, double angle) {
  double sinAngle{};
  double cosAngle{};

  if (fabs(angle) < Eo::geometricTolerance || fabs(angle - Eo::TwoPi) <= Eo::geometricTolerance) {
    cosAngle = 1.0;
  } else if (fabs(angle - Eo::HalfPi) < Eo::geometricTolerance ||
             fabs(angle + Eo::Pi + Eo::HalfPi) < Eo::geometricTolerance) {
    sinAngle = 1.0;
  } else if (fabs(angle - Eo::Pi) < Eo::geometricTolerance) {
    cosAngle = -1.0;
  } else if (fabs(angle - Eo::Pi - Eo::HalfPi) < Eo::geometricTolerance ||
             fabs(angle + Eo::HalfPi) < Eo::geometricTolerance) {
    sinAngle = -1.0;
  } else {
    sinAngle = sin(angle);
    cosAngle = cos(angle);
  }
  return EoGeVector3d(vector.x * cosAngle - vector.y * sinAngle, vector.x * sinAngle + vector.y * cosAngle, vector.z);
}

EoGeVector3d CrossProduct(const EoGeVector3d& u, const EoGeVector3d& v) noexcept {
  EoGeVector3d crossProduct(u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x);
  return crossProduct;
}

double DotProduct(const EoGeVector3d& u, const EoGeVector3d& v) noexcept {
  return (u.x * v.x + u.y * v.y + u.z * v.z);
}
