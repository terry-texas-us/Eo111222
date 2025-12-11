#include "stdafx.h"

CViewport::CViewport(const CViewport& src) 
{
	m_DeviceHeightInPixels = src.m_DeviceHeightInPixels;
	m_DeviceWidthInPixels = src.m_DeviceWidthInPixels;
	m_DeviceHeightInInches = src.m_DeviceHeightInInches;
	m_DeviceWidthInInches = src.m_DeviceWidthInInches;
	m_Height = src.m_Height;
	m_Width = src.m_Width;
}
CViewport& CViewport::operator=(const CViewport& src) 
{
	m_DeviceHeightInPixels = src.m_DeviceHeightInPixels;
	m_DeviceWidthInPixels = src.m_DeviceWidthInPixels;
	m_Width = src.m_Width;
	m_Height = src.m_Height;
	m_DeviceHeightInInches = src.m_DeviceHeightInInches;
	m_DeviceWidthInInches = src.m_DeviceWidthInInches;
	
	return *this;
}
CPoint CViewport::DoProjection(const CPnt4& pt)
{	
	CPoint pnt;
	
	pnt.x = EoRound((pt[0] / pt[3] + 1.) * ((m_Width - 1.) / 2.));
	pnt.y = EoRound((- pt[1] / pt[3] + 1.) * ((m_Height - 1.) / 2.));

	return pnt;
}
void CViewport::DoProjection(CPoint* pnt, int iPts, CPnt4* pt)
{	
	for (int i = 0; i < iPts; i++)
	{
		pt[i] /= pt[i][3];
		
		pnt[i].x = EoRound((pt[i][0] + 1.) * ((m_Width - 1.) / 2.));
		pnt[i].y = EoRound((- pt[i][1] + 1.) * ((m_Height - 1.) / 2.));
	}	 
}
void CViewport::DoProjection(CPoint* pnt, CPnt4s& pta)
{	
	int iPts = (int) pta.GetSize();

	for (int i = 0; i < iPts; i++)
	{
		pta[i] /= pta[i][3];
		
		pnt[i].x = EoRound((pta[i][0] + 1.) * ((m_Width - 1.) / 2.));
		pnt[i].y = EoRound((- pta[i][1] + 1.) * ((m_Height - 1.) / 2.));
	}	 
}
void CViewport::DoProjectionInverse(EoGePoint3d& pt)
{	
	pt.x = (pt.x * 2.) / (m_Width - 1.) - 1.;
	pt.y = - ((pt.y * 2.) / (m_Height - 1.) - 1.);
}
double CViewport::Height() const
{
	return m_Height;
}
double CViewport::HeightInInches() const
{
	return m_Height / (m_DeviceHeightInPixels / m_DeviceHeightInInches);
}
double CViewport::Width() const
{
	return m_Width;
}
double CViewport::WidthInInches() const 
{
	return m_Width / (m_DeviceWidthInPixels / m_DeviceWidthInInches);
}
void CViewport::SetDeviceHeightInInches(const double height) 
{
	m_DeviceHeightInInches = height;
}
void CViewport::SetDeviceWidthInInches(const double width) 
{
	m_DeviceWidthInInches = width;
}
void CViewport::SetSize(int width, int height)
{
	m_Width = double(width);
	m_Height = double(height);
}
void CViewport::SetDeviceHeightInPixels(const int height) 
{
	m_DeviceHeightInPixels = height;
}
void CViewport::SetDeviceWidthInPixels(const int width) 
{
	m_DeviceWidthInPixels = width;
}
