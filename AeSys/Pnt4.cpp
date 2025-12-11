#include "stdafx.h"

CPnt4::CPnt4() 
{
	m_d[0] = m_d[1] = m_d[2] = m_d[3] = 0.;
}
CPnt4::CPnt4(const double* values)
{
	ASSERT(values);

	m_d[0] = values[0]; 
	m_d[1] = values[1]; 
	m_d[2] = values[2]; 
	m_d[3] = values[3];
}
CPnt4::CPnt4(const double x, const double y, const double z, const double w)
{
	m_d[0] = x; 
	m_d[1] = y; 
	m_d[2] = z; 
	m_d[3] = w;
}
CPnt4::CPnt4(const CPnt4& pt) 
{
	m_d[0] = pt.m_d[0]; 
	m_d[1] = pt.m_d[1]; 
	m_d[2] = pt.m_d[2]; 
	m_d[3] = pt.m_d[3];
}
CPnt4::CPnt4(const EoGePoint3d& pt) 
{
	m_d[0] = pt.x; 
	m_d[1] = pt.y; 
	m_d[2] = pt.z; 
	m_d[3] = 1.;
}
CPnt4::operator double*()
{
	return (double*) m_d;
}
CPnt4::operator const double*() const
{
	return (const double*) m_d;
}
CPnt4& CPnt4::operator=(const CPnt4& pt) 
{
	m_d[0] = pt.m_d[0];
	m_d[1] = pt.m_d[1];
	m_d[2] = pt.m_d[2];
	m_d[3] = pt.m_d[3]; 
	
	return *this;
}
bool CPnt4::operator==(const CPnt4& pt) 
{
	return (Identical(pt, DBL_EPSILON));
}
bool CPnt4::operator!=(const CPnt4& pt) 
{
	return (!Identical(pt, DBL_EPSILON));
}

void CPnt4::operator+=(EoGeVector3d& v) 
{
	m_d[0] += v.x; 
	m_d[1] += v.y; 
	m_d[2] += v.z; 
}
void CPnt4::operator-=(EoGeVector3d& v) 
{
	m_d[0] -= v.x; 
	m_d[1] -= v.y; 
	m_d[2] -= v.z; 
}
void CPnt4::operator*=(const double d) 
{
	m_d[0] *= d; 
	m_d[1] *= d; 
	m_d[2] *= d; 
	m_d[3] *= d;
}
void CPnt4::operator/=(const double d) 
{
	m_d[0] /= d; 
	m_d[1] /= d; 
	m_d[2] /= d; 
	m_d[3] /= d;
}
double& CPnt4::operator[](int i) 
{
	return m_d[i];
}
const double& CPnt4::operator[](int i) const 
{
	return m_d[i];
}	
void CPnt4::operator()(const double x, const double y, const double z, const double w) 
{
	m_d[0] = x;
	m_d[1] = y;
	m_d[2] = z;
	m_d[3] = w;
}
EoGeVector3d CPnt4::operator-(const CPnt4& point)
{
	return EoGeVector3d(m_d[0] - point[0], m_d[1] - point[1], m_d[2] - point[2]);
}	
CPnt4 CPnt4::operator-(EoGeVector3d vector)
{
	return CPnt4(m_d[0] - vector.x, m_d[1] - vector.y, m_d[2] - vector.z, m_d[3]);
}	
CPnt4 CPnt4::operator+(EoGeVector3d vector)
{
	return CPnt4(m_d[0] + vector.x, m_d[1] + vector.y, m_d[2] + vector.z, m_d[3]);
}	
CPnt4 CPnt4::operator*(const double t)
{ 
	return CPnt4(m_d[0] * t, m_d[1] * t, m_d[2] * t, m_d[3] * t); 
}
CPnt4 CPnt4::operator/(const double t)
{ 
	return CPnt4(m_d[0] / t, m_d[1] / t, m_d[2] / t, m_d[3] / t); 
}
bool CPnt4::ClipLine(CPnt4& ptA, CPnt4& ptB)
{	
	double BoundaryCodeA[] =
	{	
		ptA.m_d[3] + ptA.m_d[0], ptA.m_d[3] - ptA.m_d[0],
		ptA.m_d[3] + ptA.m_d[1], ptA.m_d[3] - ptA.m_d[1],
		ptA.m_d[3] + ptA.m_d[2], ptA.m_d[3] - ptA.m_d[2]
	};
	double BoundaryCodeB[] = 
	{
		ptB.m_d[3] + ptB.m_d[0], ptB.m_d[3] - ptB.m_d[0],
		ptB.m_d[3] + ptB.m_d[1], ptB.m_d[3] - ptB.m_d[1],
		ptB.m_d[3] + ptB.m_d[2], ptB.m_d[3] - ptB.m_d[2]
	};
	
	int OutCodeA = 0;
	int OutCodeB = 0;
	
	for (int iBC = 0; iBC < 6; iBC++)
	{
		if (BoundaryCodeA[iBC] <= 0.)
			OutCodeA |= (1 << iBC);
		if (BoundaryCodeB[iBC] <= 0.)
			OutCodeB |= (1 << iBC);
	}
	
	if ((OutCodeA & OutCodeB) != 0)
		return false;
	if ((OutCodeA | OutCodeB) == 0)
		return true;

	double dTIn = 0.;
	double dTOut = 1.;
	
	double dTHit;

	for (int i = 0; i < 6; i++)
	{
		if (BoundaryCodeB[i] < 0.)
		{
			dTHit = BoundaryCodeA[i] / (BoundaryCodeA[i] - BoundaryCodeB[i]);
			dTOut = EoMin(dTOut, dTHit);
		}
		else if (BoundaryCodeA[i] < 0.)
		{
			dTHit = BoundaryCodeA[i] / (BoundaryCodeA[i] - BoundaryCodeB[i]);
			dTIn = EoMax(dTIn, dTHit);
		}
		if (dTIn > dTOut)
			return false;
	}
	CPnt4 pt(ptA);

	if (OutCodeA != 0)
	{
		ptA = pt +  (ptB - pt) * dTIn;
	}
	if (OutCodeB != 0)
	{
		ptB = pt + (ptB - pt) * dTOut;
	}
	return true;
}
void CPnt4::ClipPolygon(CPnt4s& pta)
{	
	static CPnt4 ptPln[] = 
	{
		CPnt4(- 1., 0., 0., 1.), CPnt4(1., 0., 0., 1.),
		CPnt4(0., - 1., 0., 1.), CPnt4(0., 1., 0., 1.),
		CPnt4(0., 0., - 1., 1.), CPnt4(0., 0., 1., 1.)
	}; 
	
	static EoGeVector3d vPln[] = 
	{
		EoGeVector3d(1., 0., 0.), EoGeVector3d(- 1., 0., 0.),
		EoGeVector3d(0., 1., 0.), EoGeVector3d(0., - 1., 0.),
		EoGeVector3d(0., 0., 1.), EoGeVector3d(0., 0., - 1.)
	};
	
	CPnt4s ptaOut;
	
	for (int iPln = 0; iPln < 6; iPln++)
	{
		Polygon_IntersectionWithPln(pta, ptPln[iPln], vPln[iPln], ptaOut);
		
		int iPtsOut = (int) ptaOut.GetSize();
		pta.SetSize(iPtsOut);
		
		if (iPtsOut == 0)
			break;
		
		pta.Copy(ptaOut);
		ptaOut.RemoveAll();
	}
}
double CPnt4::DistanceToPointXY(const CPnt4& ptQ) const
{	
	double X = ptQ.m_d[0] / ptQ.m_d[3] - m_d[0] / m_d[3];
	double Y = ptQ.m_d[1] / ptQ.m_d[3] - m_d[1] / m_d[3];

	return sqrt(X * X + Y * Y);
}
bool CPnt4::Identical(const CPnt4& pt, double dTol)
{
    return 
	(
		fabs(m_d[0] - pt.m_d[0]) > dTol ||
		fabs(m_d[1] - pt.m_d[1]) > dTol ||
		fabs(m_d[2] - pt.m_d[2]) > dTol ||
		fabs(m_d[3] - pt.m_d[3]) > dTol ? false : true
	);
}
bool CPnt4::IsInView()
{	
	if (m_d[3] + m_d[0] <= 0. || m_d[3] - m_d[0] <= 0.) return false;
	if (m_d[3] + m_d[1] <= 0. || m_d[3] - m_d[1] <= 0.) return false;
	if (m_d[3] + m_d[2] <= 0. || m_d[3] - m_d[2] <= 0.) return false;

	return true;
}
CPnt4 CPnt4::Max(CPnt4& ptA, CPnt4& ptB)
{ 
	return CPnt4(EoMax(ptA[0], ptB[0]), EoMax(ptA[1], ptB[1]), EoMax(ptA[2], ptB[2]), EoMax(ptA[3], ptB[3])); 
}
CPnt4 CPnt4::Min(CPnt4& ptA, CPnt4& ptB)
{ 
	return CPnt4(EoMin(ptA[0], ptB[0]), EoMin(ptA[1], ptB[1]), EoMin(ptA[2], ptB[2]), EoMin(ptA[3], ptB[3])); 
}
