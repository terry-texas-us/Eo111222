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

bool EoGeVector3d::operator==(const EoGeVector3d& vector) const noexcept { return (EoGeNearEqual(*this, vector)); }

bool EoGeVector3d::operator!=(const EoGeVector3d& vector) const noexcept { return (!EoGeNearEqual(*this, vector)); }

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

EoGeVector3d EoGeVector3d::operator-() const noexcept { return EoGeVector3d(-x, -y, -z); }

EoGeVector3d EoGeVector3d::operator-(const EoGeVector3d& vector) const noexcept {
  return EoGeVector3d(x - vector.x, y - vector.y, z - vector.z);
}

EoGeVector3d EoGeVector3d::operator+(const EoGeVector3d& vector) const noexcept {
  return EoGeVector3d(x + vector.x, y + vector.y, z + vector.z);
}

EoGeVector3d EoGeVector3d::operator*(double t) const noexcept { return EoGeVector3d(x * t, y * t, z * t); }

double EoGeVector3d::Length() const { return (sqrt(SquaredLength())); }

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

EoGeVector3d ComputeArbitraryAxis(const EoGeVector3d& normal) {
  const double epsilon = 1.0 / 64.0;

  EoGeVector3d arbitraryAxis;
  if ((fabs(normal.x) < epsilon) && (fabs(normal.y) < epsilon)) {
    arbitraryAxis = EoGeCrossProduct(EoGeVector3d::positiveUnitY, normal);
  } else {
    arbitraryAxis = EoGeCrossProduct(EoGeVector3d::positiveUnitZ, normal);
  }
  return arbitraryAxis;
}

EoGeVector3d RotateVectorAboutZAxis(const EoGeVector3d& vector, double angle) {
  double SinAngle{0.0};
  double CosineAngle{0.0};

  if (fabs(angle) <= Eo::angularEpsilon || fabs(angle - Eo::TwoPi) <= Eo::angularEpsilon) {
    CosineAngle = 1.0;
  } else if (fabs(angle - Eo::HalfPi) <= Eo::angularEpsilon ||
             fabs(angle + Eo::Pi + Eo::HalfPi) <= Eo::angularEpsilon) {
    SinAngle = 1.0;
  } else if (fabs(angle - Eo::Pi) <= Eo::angularEpsilon) {
    CosineAngle = -1.0;
  } else if (fabs(angle - Eo::Pi - Eo::HalfPi) <= Eo::angularEpsilon ||
             fabs(angle + Eo::HalfPi) <= Eo::angularEpsilon) {
    SinAngle = -1.0;
  } else {
    SinAngle = sin(angle);
    CosineAngle = cos(angle);
  }
  return EoGeVector3d(vector.x * CosineAngle - vector.y * SinAngle, vector.x * SinAngle + vector.y * CosineAngle,
                      vector.z);
}

EoGeVector3d EoGeCrossProduct(const EoGeVector3d& vector1, const EoGeVector3d& vector2) {
  EoGeVector3d CrossProduct(vector1.y * vector2.z - vector1.z * vector2.y,
                            vector1.z * vector2.x - vector1.x * vector2.z,
                            vector1.x * vector2.y - vector1.y * vector2.x);
  return CrossProduct;
}

double EoGeDotProduct(const EoGeVector3d& vector1, const EoGeVector3d& vector2) {
  return (vector1.x * vector2.x + vector1.y * vector2.y + vector1.z * vector2.z);
}
