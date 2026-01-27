#include "Stdafx.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

const EoGePoint3d EoGePoint3d::kOrigin(0.0, 0.0, 0.0);

EoGePoint3d::EoGePoint3d(const EoGePoint4d& initialPoint) {
  x = initialPoint.x / initialPoint.w;
  y = initialPoint.y / initialPoint.w;
  z = initialPoint.z / initialPoint.w;
}
bool EoGePoint3d::operator==(const EoGePoint3d& point) const { return (IsEqualTo(point, FLT_EPSILON)); }
bool EoGePoint3d::operator!=(const EoGePoint3d& point) const { return (!IsEqualTo(point, FLT_EPSILON)); }
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
  if (fabs(t) > FLT_EPSILON) {
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
EoGeVector3d EoGePoint3d::operator-(const EoGePoint3d& ptQ) const { return EoGeVector3d(ptQ.x - x, ptQ.y - y, ptQ.z - z); }
EoGePoint3d EoGePoint3d::operator-(const EoGeVector3d& vector) const { return EoGePoint3d(x - vector.x, y - vector.y, z - vector.z); }
EoGePoint3d EoGePoint3d::operator+(const EoGeVector3d& vector) const { return EoGePoint3d(x + vector.x, y + vector.y, z + vector.z); }
EoGePoint3d EoGePoint3d::operator*(double t) const { return EoGePoint3d(x * t, y * t, z * t); }

EoGePoint3d EoGePoint3d::operator/(double t) const {
  if (fabs(t) > FLT_EPSILON) { return EoGePoint3d(x / t, y / t, z / t); }
  return EoGePoint3d(x, y, z);
}

double EoGePoint3d::DistanceTo(const EoGePoint3d& point) const {
  double X = point.x - x;
  double Y = point.y - y;
  double Z = point.z - z;
  return sqrt(X * X + Y * Y + Z * Z);
}
bool EoGePoint3d::IsEqualTo(const EoGePoint3d& pt, double tolerance) const {
  return (fabs(x - pt.x) > tolerance || fabs(y - pt.y) > tolerance || fabs(z - pt.z) > tolerance ? false : true);
}
bool EoGePoint3d::IsContained(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const {
  double dReps = DBL_EPSILON + fabs(DBL_EPSILON * x);

  if (lowerLeftPoint.x > x + dReps || upperRightPoint.x < x - dReps) return false;

  dReps = DBL_EPSILON + fabs(DBL_EPSILON * y);
  if (lowerLeftPoint.y > y + dReps || upperRightPoint.y < y - dReps) return false;

  return true;
}
EoGePoint3d EoGePoint3d::Max(EoGePoint3d& ptA, EoGePoint3d& ptB) { return EoGePoint3d(std::max(ptA.x, ptB.x), std::max(ptA.y, ptB.y), std::max(ptA.z, ptB.z)); }
EoGePoint3d EoGePoint3d::Mid(EoGePoint3d& ptA, EoGePoint3d& ptB) { return ptA + (ptA - ptB) * 0.5; }
EoGePoint3d EoGePoint3d::Min(EoGePoint3d& ptA, EoGePoint3d& ptB) { return EoGePoint3d(std::min(ptA.x, ptB.x), std::min(ptA.y, ptB.y), std::min(ptA.z, ptB.z)); }
EoGePoint3d EoGePoint3d::ProjectToward(const EoGePoint3d& ptQ, const double distance) {
  EoGeVector3d v(*this, ptQ);

  double Length = v.Length();
  if (Length > DBL_EPSILON) {
    v *= distance / Length;
    return (*this + v);
  }
  return (*this);
}
void EoGePoint3d::Read(CFile& file) {
  file.Read(&x, sizeof(double));
  file.Read(&y, sizeof(double));
  file.Read(&z, sizeof(double));
}
int EoGePoint3d::RelationshipToRectangle(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const {
  int returnValue = 0;

  if (y > upperRightPoint.y + DBL_EPSILON) {
    returnValue = 1;
  } else if (y < lowerLeftPoint.y - DBL_EPSILON) {
    returnValue = 2;
  }

  if (x > upperRightPoint.x + DBL_EPSILON) {
    returnValue |= 4;
  } else if (x < lowerLeftPoint.x - DBL_EPSILON) {
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
