#pragma once

class CViewport
{
private:
	int m_DeviceHeightInPixels;
	int	m_DeviceWidthInPixels;
	double m_DeviceHeightInInches;
	double m_DeviceWidthInInches;
	double m_Height;
	double m_Width;
		
public: // Constructors and destructors

	CViewport() 
	{
	}
	CViewport(const CViewport& src);

	~CViewport() 
	{
	};
	CViewport& operator=(const CViewport& src);

public: // Methods
	/// <summary>Transforms a point from view coordinates to point in window coordinates.</summary>
	/// <remarks> Window coordinates are rounded to nearest whole number.</remarks>
	CPoint DoProjection(const CPnt4& pt);
	/// <summary>Transforms a point from view coordinates to a point in window coordinates.</summary>
	/// <remarks>Window coordinates are rounded to nearest whole number. Perspective division to yield normalized device coordinates.</remarks>
	void DoProjection(CPoint* pnt, int iPts, CPnt4* pt);
	/// <summary>Transforms a point from view coordinates to a point in window coordinates.</summary>
	/// <remarks>Window coordinates are rounded to nearest whole number. Perspective division to yield normalized device coordinates.</remarks>
	void DoProjection(CPoint* pnt, CPnt4s& pta);
	void DoProjectionInverse(EoGePoint3d& pt);

	double Height() const;
	double HeightInInches() const;
	double Width() const;
	double WidthInInches() const; 
	void SetDeviceHeightInInches(const double height);
	void SetDeviceWidthInInches(const double width);
	void SetSize(int width, int height);
	void SetDeviceHeightInPixels(const int height);
	void SetDeviceWidthInPixels(const int width);
};
typedef CList<CViewport> CViewports;
