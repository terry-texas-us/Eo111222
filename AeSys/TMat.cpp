#include "stdafx.h"

CTMat::CTMat(const EoGePoint3d& refPoint, EoGeVector3d refAxis, const double angle)
{	
	double SinAng = sin(angle);
	double CosAng = cos(angle);

	double dXSqrd = refAxis.x * refAxis.x;
	double dYSqrd = refAxis.y * refAxis.y;
	double dZSqrd = refAxis.z * refAxis.z;

	m_row[0][0] = (dXSqrd + (1. - dXSqrd) * CosAng);
	m_row[0][1] = (refAxis.x * refAxis.y * (1. - CosAng) - refAxis.z * SinAng);
	m_row[0][2] = (refAxis.x * refAxis.z * (1. - CosAng) + refAxis.y * SinAng);
	m_row[0][3] = - m_row[0][0] * refPoint.x - m_row[0][1] * refPoint.y - m_row[0][2] * refPoint.z + refPoint.x;

	m_row[1][0] = (refAxis.x * refAxis.y * (1. - CosAng) + refAxis.z * SinAng);
	m_row[1][1] = (dYSqrd + (1. - dYSqrd) * CosAng);
	m_row[1][2] = (refAxis.y * refAxis.z * (1. - CosAng) - refAxis.x * SinAng);
	m_row[1][3] = - m_row[1][0] * refPoint.x - m_row[1][1] * refPoint.y - m_row[1][2] * refPoint.z + refPoint.y;

	m_row[2][0] = (refAxis.x * refAxis.z * (1. - CosAng) - refAxis.y * SinAng);
	m_row[2][1] = (refAxis.y * refAxis.z * (1. - CosAng) + refAxis.x * SinAng);
	m_row[2][2] = (dZSqrd + (1. - dZSqrd) * CosAng);
	m_row[2][3] = - m_row[2][0] * refPoint.x - m_row[2][1] * refPoint.y - m_row[2][2] * refPoint.z + refPoint.z;

	m_row[3](0., 0., 0., 1.);
}
CTMat::CTMat(EoGePoint3d ptO, EoGeVector3d vXAx, EoGeVector3d vYAx)
{	
	EoGeVector3d vN = vXAx.CrossProduct(vYAx);
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
	CTMat tm;
	*this *= tm.RotateZ(- vXAxT.y, vXAxT.x);
		
	// Transform y-axis reference vector onto z=0 plane
	EoGeVector3d vYAxT = vYAx;
	vYAxT = *this * vYAxT;
	
	if (fabs(vYAxT.y) <= DBL_EPSILON) 
		return;
	
	vScale.y = 1. / vYAxT.y;
	vScale.z = 1.;
	
	// Add shear to matrix which gets positive y-axis reference vector as y-axis
	if (fabs(vYAxT.x) > DBL_EPSILON)
	{
		double dShrFac = - vYAxT.x / vYAxT.y;
		for (int i = 0; i < 4; i++)
			m_row[0][i] += m_row[1][i] * dShrFac;
	}
	Scale(vScale);
}
CTMat::CTMat(int* iOrd, EoGeVector3d angleVector)
{	
	Identity();
	
	for (int i = 0; i < 3; i++) 
	{
		double Angle = EoArcLength(angleVector[iOrd[i]]);
		if (fabs(Angle) > FLT_EPSILON)
		{
			double SinAng = sin(Angle);
			double CosAng = cos(Angle);

			CTMat tm;
			if (iOrd[i] == 0)
			{
				*this *= tm.RotateX(SinAng, CosAng);
			}
			else if (iOrd[i] == 1)
			{
				*this *= tm.RotateY(SinAng, CosAng);
			}
			else
			{
				*this *= tm.RotateZ(SinAng, CosAng);
			}
		}
	}
}
void CTMat::DefUsingArbPtAndAx(EoGePoint3d ptO, EoGeVector3d vN) 	
{	
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

	CTMat tm;
	if (d > DBL_EPSILON)
	{
		tm.RotateX(vN.y / d, vN.z / d);
		*this *= tm;
	}
	// sin(Ry) = Nx; cos(Ry) = d;

	if (fabs(vN.x) > DBL_EPSILON)
	{
		tm.RotateY(- vN.x, d);
		*this *= tm;
	}
}
CTMat CTMat::RotateX(const double sinAngle, const double cosAngle)
{	
	m_row[0](1., 0., 0., 0.);
	m_row[1](0., cosAngle, - sinAngle, 0.);
	m_row[2](0., sinAngle, cosAngle, 0.);
	m_row[3](0., 0., 0., 1.); 

	return (*this);
}	
CTMat CTMat::RotateY(const double sinAngle, const double cosAngle)
{	
	m_row[0](cosAngle, 0., sinAngle, 0.);
	m_row[1](0., 1., 0., 0.);
	m_row[2](- sinAngle, 0., cosAngle, 0.);
	m_row[3](0., 0., 0., 1.); 

	return (*this);
}	
CTMat CTMat::RotateZ(const double sinAngle, const double cosAngle)
{
	m_row[0](cosAngle, - sinAngle, 0., 0.);
	m_row[1](sinAngle, cosAngle, 0., 0.);
	m_row[2](0., 0., 1., 0.);
	m_row[3](0., 0., 0., 1.);
	
	return (*this);
}
void CTMat::Scale(EoGeVector3d v)
{
	for (int i = 0; i < 4; i++)
	{
		m_row[0][i] *= v.x; m_row[1][i] *= v.y; m_row[2][i] *= v.z;
	}
}
void CTMat::Translate(EoGeVector3d v)
{
	m_row[0][3] += v.x; 
	m_row[1][3] += v.y; 
	m_row[2][3] += v.z;
}
CLine CTMat::operator*(CLine line)
{
	CLine lineOut;

	lineOut.begin = *this * line.begin;
	lineOut.end = *this * line.end;
	
	return lineOut;
}
EoGePoint3d CTMat::operator*(const EoGePoint3d& point)
{
	EoGePoint3d ptOut;

	ptOut.x = point.x * m_row[0][0] + point.y * m_row[0][1] + point.z * m_row[0][2] + m_row[0][3];
	ptOut.y = point.x * m_row[1][0] + point.y * m_row[1][1] + point.z * m_row[1][2] + m_row[1][3];
	ptOut.z = point.x * m_row[2][0] + point.y * m_row[2][1] + point.z * m_row[2][2] + m_row[2][3];

	return ptOut;
}
EoGePoint4d CTMat::operator*(EoGePoint4d& point)
{
	EoGePoint4d ptOut;

	for (int i = 0; i < 4; i++)
	{
		ptOut[i] = point[0] * m_row[i][0] + point[1] * m_row[i][1] + point[2] * m_row[i][2] + point[3] * m_row[i][3];
	}

	return ptOut;
}
EoGeVector3d CTMat::operator*(EoGeVector3d vector)
{
	EoGeVector3d vOut;

	vOut.x = vector.x * m_row[0][0] + vector.y * m_row[0][1] + vector.z * m_row[0][2];
	vOut.y = vector.x * m_row[1][0] + vector.y * m_row[1][1] + vector.z * m_row[1][2];
	vOut.z = vector.x * m_row[2][0] + vector.y * m_row[2][1] + vector.z * m_row[2][2];
	
	return vOut;
}
