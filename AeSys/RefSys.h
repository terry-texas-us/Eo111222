#pragma once

class CCharCellDef;
class CRefSys
{
protected:
	EoGePoint3d	m_Origin;
	EoGeVector3d m_XDirection;
	EoGeVector3d m_YDirection;

public: // Constructors and destructor
	
	CRefSys() 
	{
		m_Origin(0., 0., 0.); 
		m_XDirection(1., 0., 0.); 
		m_YDirection(0., 1., 0.);
	}
	CRefSys(const EoGePoint3d& origin, EoGeVector3d xDirection, EoGeVector3d yDirection);
	CRefSys(const EoGePoint3d& origin, CCharCellDef& ccd);
	
	CRefSys(const CRefSys& cs);
	
	~CRefSys() 
	{
	}

public: // Operators
	
	CRefSys& operator=(const CRefSys&);

public: // Methods

	EoGeVector3d XDirection() 
	{
		return m_XDirection;
	}
	EoGeVector3d YDirection() 
	{
		return m_YDirection;
	}
	CTMat GetTransformMatrix();
	void GetUnitNormal(EoGeVector3d& normal) 
	{
		normal = m_XDirection.CrossProduct(m_YDirection); 
		normal.Normalize();
	}
	EoGePoint3d& Origin() 
	{
		return m_Origin;
	}
	void Read(CFile& file);
	/// <summary>Takes the current reference directions and rescales using passed character cell state.</summary>
	void Rescale(CCharCellDef& ccd);
	void Set(const EoGePoint3d& origin, const EoGeVector3d xDirection, const EoGeVector3d yDirection) 
	{
		m_Origin = origin; 
		m_XDirection = xDirection; 
		m_YDirection = yDirection;
	}
	void SetOrigin(const EoGePoint3d origin) 
	{
		m_Origin = origin;
	}
	void SetXDirection(const EoGeVector3d xDirection) 
	{
		m_XDirection = xDirection;
	}
	void SetYDirection(const EoGeVector3d yDirection) 
	{
		m_YDirection = yDirection;
	}
	void Transform(CTMat& tm);
	void Write(CFile& file);
};
