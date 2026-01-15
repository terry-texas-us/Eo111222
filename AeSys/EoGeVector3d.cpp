#include "Stdafx.h"
#include "Eo.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

const EoGeVector3d EoGeVector3d::kXAxis(1.0, 0.0, 0.0);
const EoGeVector3d EoGeVector3d::kYAxis(0.0, 1.0, 0.0);
const EoGeVector3d EoGeVector3d::kZAxis(0.0, 0.0, 1.0);

EoGeVector3d::EoGeVector3d() : x(0.0), y(0.0), z(0.0) {}

EoGeVector3d::EoGeVector3d(double initialX, double initialY, double initialZ) : x(initialX), y(initialY), z(initialZ) {}

EoGeVector3d::EoGeVector3d(const EoGePoint3d& ptA, const EoGePoint3d& ptB) : x(ptB.x - ptA.x), y(ptB.y - ptA.y), z(ptB.z - ptA.z) {}

#if defined(USING_ODA)
EoGeVector3d::EoGeVector3d(const OdGeVector3d& initialVector) {
  x = initialVector.x;
  y = initialVector.y;
  z = initialVector.z;
}
EoGeVector3d::EoGeVector3d(const OdGeScale3d& initialScale) {
  x = initialScale.sx;
  y = initialScale.sy;
  z = initialScale.sz;
}
#endif

bool EoGeVector3d::operator==(const EoGeVector3d& vector) const { return (EoGeNearEqual(*this, vector, FLT_EPSILON)); }

bool EoGeVector3d::operator!=(const EoGeVector3d& vector) const { return (!EoGeNearEqual(*this, vector, FLT_EPSILON)); }

void EoGeVector3d::operator+=(const EoGeVector3d& offset) {
  x += offset.x;
  y += offset.y;
  z += offset.z;
}

void EoGeVector3d::operator-=(const EoGeVector3d& offset) {
  x -= offset.x;
  y -= offset.y;
  z -= offset.z;
}

void EoGeVector3d::operator*=(double t) {
  x *= t;
  y *= t;
  z *= t;
}

void EoGeVector3d::operator/=(double t) {
  x /= t;
  y /= t;
  z /= t;
}

void EoGeVector3d::operator()(double xNew, double yNew, double zNew) {
  x = xNew;
  y = yNew;
  z = zNew;
}

EoGeVector3d EoGeVector3d::operator-() const { return EoGeVector3d(-x, -y, -z); }

EoGeVector3d EoGeVector3d::operator-(const EoGeVector3d& vector) const {
  return EoGeVector3d(x - vector.x, y - vector.y, z - vector.z);
}

EoGeVector3d EoGeVector3d::operator+(const EoGeVector3d& vector) const {
  return EoGeVector3d(x + vector.x, y + vector.y, z + vector.z);
}

EoGeVector3d EoGeVector3d::operator*(double t) const { return EoGeVector3d(x * t, y * t, z * t); }

bool EoGeVector3d::IsNearNull() const {
  return (fabs(x) > DBL_EPSILON || fabs(y) > DBL_EPSILON || fabs(z) > DBL_EPSILON ? false : true);
}

double EoGeVector3d::Length() const { return (sqrt(SquaredLength())); }

void EoGeVector3d::Normalize() {
  double dLength = Length();

  if (dLength <= FLT_EPSILON) { dLength = FLT_EPSILON; }
  *this /= dLength;
}

void EoGeVector3d::Read(CFile& file) {
  file.Read(&x, sizeof(double));
  file.Read(&y, sizeof(double));
  file.Read(&z, sizeof(double));
}

void EoGeVector3d::RotAboutArbAx(const EoGeVector3d& axis, double angle) {
  if (axis == kZAxis) {
    *this = RotateVectorAboutZAxis(*this, angle);
  } else {
    EoGeTransformMatrix tm(EoGePoint3d::kOrigin, axis, angle);

    *this = tm * (*this);
  }
}

double EoGeVector3d::SquaredLength() const { return (x * x + y * y + z * z); }

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
  const double Epsilon = 1.0 / 64.0;

  EoGeVector3d ArbitraryAxis;
  if ((fabs(normal.x) < Epsilon) && (fabs(normal.y) < Epsilon)) {
    ArbitraryAxis = EoGeCrossProduct(EoGeVector3d::kYAxis, normal);
  } else {
    ArbitraryAxis = EoGeCrossProduct(EoGeVector3d::kZAxis, normal);
  }
  return ArbitraryAxis;
}

EoGeVector3d RotateVectorAboutZAxis(const EoGeVector3d& vector, double angle) {
  double SinAngle = 0.0;
  double CosineAngle = 0.0;

  if (fabs(angle) <= FLT_EPSILON || fabs(angle - Eo::TwoPi) <= FLT_EPSILON) {
    CosineAngle = 1.0;
  } else if (fabs(angle - Eo::HalfPi) <= FLT_EPSILON || fabs(angle + Eo::Pi + Eo::HalfPi) <= FLT_EPSILON) {
    SinAngle = 1.0;
  } else if (fabs(angle - Eo::Pi) <= FLT_EPSILON) {
    CosineAngle = -1.0;
  } else if (fabs(angle - Eo::Pi - Eo::HalfPi) <= FLT_EPSILON || fabs(angle + Eo::HalfPi) <= FLT_EPSILON) {
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

bool EoGeNearEqual(const EoGeVector3d& vector1, const EoGeVector3d& vector2, double tolerance) {
  return (fabs(vector1.x - vector2.x) > tolerance || fabs(vector1.y - vector2.y) > tolerance ||
                  fabs(vector1.z - vector2.z) > tolerance
              ? false
              : true);
}
