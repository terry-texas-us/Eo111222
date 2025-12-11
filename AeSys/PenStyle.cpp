#include "stdafx.h"

CLineType::CLineType()
{
	m_wLens = 0;
	m_dLen = NULL;
}
CLineType::CLineType(const CString& strName, const CString& strDesc, EoUInt16 wLens, double* dLen)
{
	m_strName = strName;
	m_strDesc = strDesc;
	m_wLens = wLens;
	if (m_wLens == 0)
		m_dLen = 0;
	else
	{
		m_dLen = new double[m_wLens];
		for (int i = 0; i < m_wLens; i++)
		{
			m_dLen[i] = Max(- 99., Min(dLen[i], 99.));
		}
	}
}
CLineType::CLineType(const CLineType& lineType)
{
	m_strName = lineType.m_strName;
	m_strDesc = lineType.m_strDesc;
	m_wLens = lineType.m_wLens;
	m_dLen = new double[m_wLens];
	for (int i = 0; i < m_wLens; i++)
	{
		m_dLen[i] = lineType.m_dLen[i];
	}
}
CLineType::~CLineType()
{
	delete [] m_dLen;
}
CLineType& CLineType::operator=(const CLineType& lineType)
{
	m_strName = lineType.m_strName;
	m_strDesc = lineType.m_strDesc;
	m_wLens = lineType.m_wLens;
	m_dLen = new double[m_wLens];
	for (int i = 0; i < m_wLens; i++)
	{
		m_dLen[i] = lineType.m_dLen[i];
	}
	return *this;
}
double CLineType::GetPatternLen()
{
	double dLen = 0.;

	for (int i = 0; i < m_wLens; i++) 
		dLen += fabs(m_dLen[i]);
	
	return (dLen);
}