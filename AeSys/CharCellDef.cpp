#include "stdafx.h"

#include "CharCellDef.h"

CCharCellDef::CCharCellDef()
{
	m_dChrHgt = .1;
	m_dChrExpFac = 1.;
	m_dTextRotAng = 0.;
	m_dChrSlantAng = 0.;
}
CCharCellDef::CCharCellDef(double dTxtOffAng, double dChrSlantAng, double dChrExpFac, double dChrHgt)
{
	m_dChrHgt = dChrHgt;
	m_dChrExpFac = dChrExpFac;
	m_dTextRotAng = dTxtOffAng;
	m_dChrSlantAng = dChrSlantAng;
}
CCharCellDef::CCharCellDef(const CCharCellDef& fd) 
{
	m_dChrHgt = fd.m_dChrHgt;
	m_dChrExpFac = fd.m_dChrExpFac;
	m_dTextRotAng = fd.m_dTextRotAng;
	m_dChrSlantAng = fd.m_dChrSlantAng;
}
CCharCellDef& CCharCellDef::operator=(const CCharCellDef& fd)
{
	m_dChrHgt = fd.m_dChrHgt;
	m_dChrExpFac = fd.m_dChrExpFac;
	m_dTextRotAng = fd.m_dTextRotAng;
	m_dChrSlantAng = fd.m_dChrSlantAng;

	return (*this);
}
void CharCellDef_EncdRefSys(const EoGeVector3d& normal, CCharCellDef& ccd, EoGeVector3d& xAxis, EoGeVector3d& yAxis)
{
	xAxis = Prim_ComputeArbitraryAxis(normal);
	xAxis.RotAboutArbAx(normal, ccd.TextRotAngGet());

	yAxis = normal.CrossProduct(xAxis);
	
	xAxis *= .6 * ccd.ChrHgtGet() * ccd.ChrExpFacGet();
	
	yAxis.RotAboutArbAx(normal, ccd.ChrSlantAngGet());
	yAxis *= ccd.ChrHgtGet();
}