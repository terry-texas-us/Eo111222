#include "stdafx.h"
#include "AeSysView.h"

CRefSys::CRefSys(const EoGePoint3d& ptOrigin, EoGeVector3d vDirX, EoGeVector3d vDirY)
{
	m_Origin = ptOrigin;
	m_XDirection = vDirX;
	m_YDirection = vDirY;
}
CRefSys::CRefSys(const EoGePoint3d& ptOrigin, CCharCellDef& ccd)
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	
	m_Origin = ptOrigin;

	EoGeVector3d vNorm = ActiveView->ModelViewGetDirection();

	m_YDirection = ActiveView->ModelViewGetViewUp();
	m_YDirection.RotAboutArbAx(vNorm, ccd.TextRotAngGet());
	
	m_XDirection = m_YDirection;
	m_XDirection.RotAboutArbAx(vNorm, - HALF_PI);
	m_YDirection.RotAboutArbAx(vNorm, ccd.ChrSlantAngGet());
	m_XDirection *= .6 * ccd.ChrHgtGet() * ccd.ChrExpFacGet();
	m_YDirection *= ccd.ChrHgtGet();
}
CRefSys::CRefSys(const CRefSys& src)
{
	m_Origin = src.m_Origin;
	m_XDirection = src.m_XDirection;
	m_YDirection = src.m_YDirection;
}
CRefSys& CRefSys::operator=(const CRefSys& src)
{
	m_Origin = src.m_Origin;
	m_XDirection = src.m_XDirection;
	m_YDirection = src.m_YDirection;
	
	return (*this);
}
CTMat CRefSys::GetTransformMatrix()
{
	return CTMat(m_Origin, m_XDirection, m_YDirection);
}

void CRefSys::Read(CFile& file)
{
	m_Origin.Read(file);
	m_XDirection.Read(file);
	m_YDirection.Read(file);
}
void CRefSys::Rescale(CCharCellDef& ccd)
{
	EoGeVector3d vNorm;
	GetUnitNormal(vNorm);
	m_XDirection.Normalize();
	m_YDirection = m_XDirection;
	m_YDirection.RotAboutArbAx(vNorm, HALF_PI + ccd.ChrSlantAngGet());
	m_XDirection *= .6 * ccd.ChrHgtGet() * ccd.ChrExpFacGet();
	m_YDirection *= ccd.ChrHgtGet();
}
void CRefSys::Transform(CTMat& tm)
{
	m_Origin = tm * m_Origin;
	m_XDirection = tm * m_XDirection;
	m_YDirection = tm * m_YDirection;
}
void CRefSys::Write(CFile& file)
{
	m_Origin.Write(file);
	m_XDirection.Write(file);
	m_YDirection.Write(file);
}