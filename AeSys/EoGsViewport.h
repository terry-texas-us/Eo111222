#pragma once

class EoGsViewport {
	float m_DeviceHeightInPixels;
	float m_DeviceWidthInPixels;
	float m_DeviceHeightInInches;
	float m_DeviceWidthInInches;
	float m_Height;
	float m_Width;

public: // Constructors and destructors

	EoGsViewport() {
	}
	EoGsViewport(const EoGsViewport& src);

	~EoGsViewport() {
	};
	EoGsViewport& operator=(const EoGsViewport& src);

public: // Methods
	/// <summary>Transforms a point from view coordinates to point in window coordinates.</summary>
	/// <remarks> Window coordinates are rounded to nearest whole number.</remarks>
	CPoint DoProjection(const EoGePoint4d& pt);
	/// <summary>Transforms a point from view coordinates to a point in window coordinates.</summary>
	/// <remarks>Window coordinates are rounded to nearest whole number. Perspective division to yield normalized device coordinates.</remarks>
	void DoProjection(CPoint* pnt, int iPts, EoGePoint4d* pt);
	/// <summary>Transforms a point from view coordinates to a point in window coordinates.</summary>
	/// <remarks>Window coordinates are rounded to nearest whole number. Perspective division to yield normalized device coordinates.</remarks>
	void DoProjection(CPoint* pnt, EoGePoint4dArray& pointsArray);
	void DoProjectionInverse(EoGePoint3d& pt);

	float Height() const;
	float HeightInInches() const;
	float Width() const;
	float WidthInInches() const;
	void SetDeviceHeightInInches(const float height);
	void SetDeviceWidthInInches(const float width);
	void SetSize(int width, int height);
	void SetDeviceHeightInPixels(const float height);
	void SetDeviceWidthInPixels(const float width);
};
typedef CList<EoGsViewport> CViewports;
