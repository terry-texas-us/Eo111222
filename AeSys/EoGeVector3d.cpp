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

EoGeVector3d::EoGeVector3d(double initialX, double initialY, double initialZ) : x(initialX), y(initialY), z(initialZ) {}

EoGeVector3d::EoGeVector3d(const EoGePoint3d& ptA, const EoGePoint3d& ptB)
    : x(ptB.x - ptA.x), y(ptB.y - ptA.y), z(ptB.z - ptA.z) {}

[[nodiscard]] bool EoGeVector3d::operator==(const EoGeVector3d& vector) const noexcept {
  return (IsNearEqual(*this, vector));
}

[[nodiscard]] bool EoGeVector3d::operator!=(const EoGeVector3d& vector) const noexcept {
  return (!IsNearEqual(*this, vector));
}

void EoGeVector3d::operator+=(const EoGeVector3d& offset) noexcept {
  x += offset.x;
  y += offset.y;
  z += offset.z;
}

void EoGeVector3d::operator-=(const EoGeVector3d& offset) noexcept {
  x -= offset.x;
  y -= offset.y;
  z -= offset.z;
}

void EoGeVector3d::operator*=(double t) noexcept {
  x *= t;
  y *= t;
  z *= t;
}

void EoGeVector3d::operator/=(double t) {
  x /= t;
  y /= t;
  z /= t;
}

void EoGeVector3d::operator()(double xNew, double yNew, double zNew) noexcept {
  x = xNew;
  y = yNew;
  z = zNew;
}

[[nodiscard]] EoGeVector3d EoGeVector3d::operator-(const EoGeVector3d& vector) const noexcept {
  return EoGeVector3d(x - vector.x, y - vector.y, z - vector.z);
}

[[nodiscard]] EoGeVector3d EoGeVector3d::operator+(const EoGeVector3d& vector) const noexcept {
  return EoGeVector3d(x + vector.x, y + vector.y, z + vector.z);
}

/** @brief Determines if this vector is equal to another vector within a specified tolerance.
 * @param other The other vector to compare with.
 * @param tolerance The tolerance within which the vectors are considered equal.
 * @return true if the vectors are equal within the specified tolerance; false otherwise.
 */
[[nodiscard]] bool EoGeVector3d::IsEqualTo(const EoGeVector3d& other, double tolerance) const noexcept {
  return (fabs(x - other.x) > tolerance || fabs(y - other.y) > tolerance || fabs(z - other.z) > tolerance ? false : true);
}

[[nodiscard]] double EoGeVector3d::Length() const { return (sqrt(SquaredLength())); }

void EoGeVector3d::Normalize() {
  if (IsNearNull()) { throw std::domain_error("Cannot normalize zero-length vector"); }
  *this /= Length();
}

void EoGeVector3d::Read(CFile& file) {
  file.Read(&x, sizeof(double));
  file.Read(&y, sizeof(double));
  file.Read(&z, sizeof(double));
}

void EoGeVector3d::RotAboutArbAx(const EoGeVector3d& axis, double angle) {
  if (axis == positiveUnitZ) {
    *this = RotateVectorAboutZAxis(*this, angle);
  } else {
    EoGeTransformMatrix tm(EoGePoint3d::kOrigin, axis, angle);

    *this = tm * (*this);
  }
}

CString EoGeVector3d::ToString() const {
  CString str;
  str.Format(L"%f;%f;%f\t", x, y, z);
  return (str);
}

void EoGeVector3d::Write(CFile& file) const {
  file.Write(&x, sizeof(double));
  file.Write(&y, sizeof(double));
  file.Write(&z, sizeof(double));
}

[[nodiscard]] EoGeVector3d ComputeArbitraryAxis(const EoGeVector3d& normal) {
  const double epsilon = 1.0 / 64.0;

  EoGeVector3d arbitraryAxis;
  if ((fabs(normal.x) < epsilon) && (fabs(normal.y) < epsilon)) {
    arbitraryAxis = CrossProduct(EoGeVector3d::positiveUnitY, normal);
  } else {
    arbitraryAxis = CrossProduct(EoGeVector3d::positiveUnitZ, normal);
  }
  return arbitraryAxis;
}

[[nodiscard]] EoGeVector3d RotateVectorAboutZAxis(const EoGeVector3d& vector, double angle) {
  double SinAngle{0.0};
  double CosineAngle{0.0};

  if (fabs(angle) <= Eo::geometricTolerance || fabs(angle - Eo::TwoPi) <= Eo::geometricTolerance) {
    CosineAngle = 1.0;
  } else if (fabs(angle - Eo::HalfPi) <= Eo::geometricTolerance ||
             fabs(angle + Eo::Pi + Eo::HalfPi) <= Eo::geometricTolerance) {
    SinAngle = 1.0;
  } else if (fabs(angle - Eo::Pi) <= Eo::geometricTolerance) {
    CosineAngle = -1.0;
  } else if (fabs(angle - Eo::Pi - Eo::HalfPi) <= Eo::geometricTolerance ||
             fabs(angle + Eo::HalfPi) <= Eo::geometricTolerance) {
    SinAngle = -1.0;
  } else {
    SinAngle = sin(angle);
    CosineAngle = cos(angle);
  }
  return EoGeVector3d(vector.x * CosineAngle - vector.y * SinAngle, vector.x * SinAngle + vector.y * CosineAngle,
                      vector.z);
}

[[nodiscard]] EoGeVector3d CrossProduct(const EoGeVector3d& vector1, const EoGeVector3d& vector2) noexcept {
  EoGeVector3d crossProduct(vector1.y * vector2.z - vector1.z * vector2.y,
                            vector1.z * vector2.x - vector1.x * vector2.z,
                            vector1.x * vector2.y - vector1.y * vector2.x);
  return crossProduct;
}

[[nodiscard]] double DotProduct(const EoGeVector3d& vector1, const EoGeVector3d& vector2) noexcept {
  return (vector1.x * vector2.x + vector1.y * vector2.y + vector1.z * vector2.z);
}
