#include "stdafx.h"

EoGeTransformMatrix::EoGeTransformMatrix(const EoGePoint3d& refPoint, EoGeVector3d refAxis, const double angle) {
  double SinAng = sin(angle);
  double CosAng = cos(angle);

  double dXSqrd = refAxis.x * refAxis.x;
  double dYSqrd = refAxis.y * refAxis.y;
  double dZSqrd = refAxis.z * refAxis.z;

  m_4X4[0][0] = (dXSqrd + (1. - dXSqrd) * CosAng);
  m_4X4[0][1] = (refAxis.x * refAxis.y * (1. - CosAng) - refAxis.z * SinAng);
  m_4X4[0][2] = (refAxis.x * refAxis.z * (1. - CosAng) + refAxis.y * SinAng);
  m_4X4[0][3] = -m_4X4[0][0] * refPoint.x - m_4X4[0][1] * refPoint.y - m_4X4[0][2] * refPoint.z + refPoint.x;

  m_4X4[1][0] = (refAxis.x * refAxis.y * (1. - CosAng) + refAxis.z * SinAng);
  m_4X4[1][1] = (dYSqrd + (1. - dYSqrd) * CosAng);
  m_4X4[1][2] = (refAxis.y * refAxis.z * (1. - CosAng) - refAxis.x * SinAng);
  m_4X4[1][3] = -m_4X4[1][0] * refPoint.x - m_4X4[1][1] * refPoint.y - m_4X4[1][2] * refPoint.z + refPoint.y;

  m_4X4[2][0] = (refAxis.x * refAxis.z * (1. - CosAng) - refAxis.y * SinAng);
  m_4X4[2][1] = (refAxis.y * refAxis.z * (1. - CosAng) + refAxis.x * SinAng);
  m_4X4[2][2] = (dZSqrd + (1. - dZSqrd) * CosAng);
  m_4X4[2][3] = -m_4X4[2][0] * refPoint.x - m_4X4[2][1] * refPoint.y - m_4X4[2][2] * refPoint.z + refPoint.z;

  m_4X4[3][0] = 0.;
  m_4X4[3][1] = 0.;
  m_4X4[3][2] = 0.;
  m_4X4[3][3] = 1.;
}
EoGeTransformMatrix::EoGeTransformMatrix(EoGePoint3d ptO, EoGeVector3d vXAx, EoGeVector3d vYAx) {
  EoGeVector3d vN = EoGeCrossProduct(vXAx, vYAx);
  vN.Normalize();

  DefUsingArbPtAndAx(ptO, vN);

  // Transform x-axis reference vector onto z=0 plane
  EoGeVector3d vXAxT = vXAx;
  vXAxT = *this * vXAxT;

  double dX = vXAxT.x;
  double dY = vXAxT.y;

  EoGeVector3d vScale(1. / sqrt(dX * dX + dY * dY), 1., 1.);

  vXAxT.Normalize();

  // To get x-axis reference vector as x-axis
  EoGeTransformMatrix tm;
  *this *= tm.ZAxisRotation(-vXAxT.y, vXAxT.x);

  // Transform y-axis reference vector onto z=0 plane
  EoGeVector3d vYAxT = vYAx;
  vYAxT = *this * vYAxT;

  if (fabs(vYAxT.y) <= DBL_EPSILON) return;

  vScale.y = 1. / vYAxT.y;
  vScale.z = 1.;

  // Add shear to matrix which gets positive y-axis reference vector as y-axis
  if (fabs(vYAxT.x) > DBL_EPSILON) {
    double dShrFac = -vYAxT.x / vYAxT.y;
    for (int i = 0; i < 4; i++) { m_4X4[0][i] += m_4X4[1][i] * dShrFac; }
  }
  Scale(vScale);
}
//EoGeTransformMatrix::EoGeTransformMatrix(int* iOrd, EoGeVector3d angleVector) {
//	Identity();
//	double AngleVector[3] = {angleVector.x, angleVector.y, angleVector.z};
//
//	for (int i = 0; i < 3; i++) {
//		double Angle = EoArcLength(AngleVector[iOrd[i]]);
//		if (fabs(Angle) > FLT_EPSILON) {
//			double SinAng = sin(Angle);
//			double CosAng = cos(Angle);
//
//			EoGeTransformMatrix tm;
//			if (iOrd[i] == 0) {
//				*this *= tm.XAxisRotation(SinAng, CosAng);
//			}
//			else if (iOrd[i] == 1) {
//				*this *= tm.YAxisRotation(SinAng, CosAng);
//			}
//			else {
//				*this *= tm.ZAxisRotation(SinAng, CosAng);
//			}
//		}
//	}
//}

EoGeTransformMatrix EoGeTransformMatrix::BuildRotationTransformMatrix(const EoGeVector3d& rotationAngles) const {
  EoGeTransformMatrix Matrix;

  Matrix.Identity();
  Matrix.AppendXAxisRotation(rotationAngles.x);
  Matrix.AppendYAxisRotation(rotationAngles.y);
  Matrix.AppendZAxisRotation(rotationAngles.z);

  return Matrix;
}
void EoGeTransformMatrix::AppendXAxisRotation(double xAxisAngle) {
  if (fabs(xAxisAngle) != 0.0) {
    double Angle = EoArcLength(xAxisAngle);
    EoGeTransformMatrix Matrix;
    *this *= Matrix.XAxisRotation(sin(Angle), cos(Angle));
  }
}
void EoGeTransformMatrix::AppendYAxisRotation(double yAxisAngle) {
  if (fabs(yAxisAngle) != 0.0) {
    double Angle = EoArcLength(yAxisAngle);
    EoGeTransformMatrix Matrix;
    *this *= Matrix.YAxisRotation(sin(Angle), cos(Angle));
  }
}
void EoGeTransformMatrix::AppendZAxisRotation(double zAxisAngle) {
  if (fabs(zAxisAngle) != 0.0) {
    double Angle = EoArcLength(zAxisAngle);
    EoGeTransformMatrix Matrix;
    *this *= Matrix.ZAxisRotation(sin(Angle), cos(Angle));
  }
}
void EoGeTransformMatrix::DefUsingArbPtAndAx(EoGePoint3d ptO, EoGeVector3d vN) {
  Identity();
  Translate(EoGeVector3d(ptO, EoGePoint3d::kOrigin));

  double dAbsNy = fabs(vN.y);
  double dAbsNz = fabs(vN.z);

  // avoid sqrt use if Ny or Nz are 0.

  double d = 0.;
  if (dAbsNz <= DBL_EPSILON)
    d = dAbsNy;
  else if (dAbsNy <= DBL_EPSILON)
    d = dAbsNz;
  else
    d = sqrt(dAbsNy * dAbsNy + dAbsNz * dAbsNz);

  // sin(Rx) = Ny / d; cos(Rx) = Nz / d

  EoGeTransformMatrix tm;
  if (d > DBL_EPSILON) {
    tm.XAxisRotation(vN.y / d, vN.z / d);
    *this *= tm;
  }
  // sin(Ry) = Nx; cos(Ry) = d;

  if (fabs(vN.x) > DBL_EPSILON) {
    tm.YAxisRotation(-vN.x, d);
    *this *= tm;
  }
}
EoGeTransformMatrix EoGeTransformMatrix::XAxisRotation(const double sinAngle, const double cosAngle) {
  m_4X4[0][0] = 1.;
  m_4X4[0][1] = 0.;
  m_4X4[0][2] = 0.;
  m_4X4[0][3] = 0.;

  m_4X4[1][0] = 0.;
  m_4X4[1][1] = cosAngle;
  m_4X4[1][2] = -sinAngle;
  m_4X4[1][3] = 0.;

  m_4X4[2][0] = 0.;
  m_4X4[2][1] = sinAngle;
  m_4X4[2][2] = cosAngle;
  m_4X4[2][3] = 0.;

  m_4X4[3][0] = 0.;
  m_4X4[3][1] = 0.;
  m_4X4[3][2] = 0.;
  m_4X4[3][3] = 1.;

  return (*this);
}
EoGeTransformMatrix EoGeTransformMatrix::YAxisRotation(const double sinAngle, const double cosAngle) {
  m_4X4[0][0] = cosAngle;
  m_4X4[0][1] = 0.;
  m_4X4[0][2] = sinAngle;
  m_4X4[0][3] = 0.;

  m_4X4[1][0] = 0.;
  m_4X4[1][1] = 1.;
  m_4X4[1][2] = 0.;
  m_4X4[1][3] = 0.;

  m_4X4[2][0] = -sinAngle;
  m_4X4[2][1] = 0.;
  m_4X4[2][2] = cosAngle;
  m_4X4[2][3] = 0.;

  m_4X4[3][0] = 0.;
  m_4X4[3][1] = 0.;
  m_4X4[3][2] = 0.;
  m_4X4[3][3] = 1.;

  return (*this);
}
EoGeTransformMatrix EoGeTransformMatrix::ZAxisRotation(const double sinAngle, const double cosAngle) {
  m_4X4[0][0] = cosAngle;
  m_4X4[0][1] = -sinAngle;
  m_4X4[0][2] = 0.;
  m_4X4[0][3] = 0.;

  m_4X4[1][0] = sinAngle;
  m_4X4[1][1] = cosAngle;
  m_4X4[1][2] = 0.;
  m_4X4[1][3] = 0.;

  m_4X4[2][0] = 0.;
  m_4X4[2][1] = 0.;
  m_4X4[2][2] = 1.;
  m_4X4[2][3] = 0.;

  m_4X4[3][0] = 0.;
  m_4X4[3][1] = 0.;
  m_4X4[3][2] = 0.;
  m_4X4[3][3] = 1.;

  return (*this);
}
void EoGeTransformMatrix::Scale(EoGeVector3d v) {
  for (int i = 0; i < 4; i++) {
    m_4X4[0][i] *= v.x;
    m_4X4[1][i] *= v.y;
    m_4X4[2][i] *= v.z;
  }
}

EoGeLine EoGeTransformMatrix::operator*(EoGeLine line) {
  EoGeLine lineOut;

  lineOut.begin = *this * line.begin;
  lineOut.end = *this * line.end;

  return lineOut;
}
EoGePoint3d EoGeTransformMatrix::operator*(const EoGePoint3d& point) {
  EoGePoint3d ptOut;

  ptOut.x = point.x * m_4X4[0][0] + point.y * m_4X4[0][1] + point.z * m_4X4[0][2] + m_4X4[0][3];
  ptOut.y = point.x * m_4X4[1][0] + point.y * m_4X4[1][1] + point.z * m_4X4[1][2] + m_4X4[1][3];
  ptOut.z = point.x * m_4X4[2][0] + point.y * m_4X4[2][1] + point.z * m_4X4[2][2] + m_4X4[2][3];

  return ptOut;
}
EoGePoint4d EoGeTransformMatrix::operator*(EoGePoint4d& point) {
  EoGePoint4d ptOut;

  ptOut.x = point.x * m_4X4[0][0] + point.y * m_4X4[0][1] + point.z * m_4X4[0][2] + point.w * m_4X4[0][3];
  ptOut.y = point.x * m_4X4[1][0] + point.y * m_4X4[1][1] + point.z * m_4X4[1][2] + point.w * m_4X4[1][3];
  ptOut.z = point.x * m_4X4[2][0] + point.y * m_4X4[2][1] + point.z * m_4X4[2][2] + point.w * m_4X4[2][3];
  ptOut.w = point.x * m_4X4[3][0] + point.y * m_4X4[3][1] + point.z * m_4X4[3][2] + point.w * m_4X4[3][3];

  return ptOut;
}
EoGeVector3d EoGeTransformMatrix::operator*(EoGeVector3d vector) {
  EoGeVector3d vOut;

  vOut.x = vector.x * m_4X4[0][0] + vector.y * m_4X4[0][1] + vector.z * m_4X4[0][2];
  vOut.y = vector.x * m_4X4[1][0] + vector.y * m_4X4[1][1] + vector.z * m_4X4[1][2];
  vOut.z = vector.x * m_4X4[2][0] + vector.y * m_4X4[2][1] + vector.z * m_4X4[2][2];

  return vOut;
}
