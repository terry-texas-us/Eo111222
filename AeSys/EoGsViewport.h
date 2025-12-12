#pragma once

class EoGsViewport {
  double m_DeviceHeightInPixels;
  double m_DeviceWidthInPixels;
  double m_DeviceHeightInInches;
  double m_DeviceWidthInInches;
  double m_Height;
  double m_Width;

 public:  // Constructors and destructors
  EoGsViewport() {}
  EoGsViewport(const EoGsViewport& src);

  ~EoGsViewport() {};
  EoGsViewport& operator=(const EoGsViewport& src);

 public:  // Methods
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

  double Height() const;
  double HeightInInches() const;
  double Width() const;
  /// @brief Calculates the width of the viewport in inches.
  /// @return The width of the viewport in inches.
  double WidthInInches() const;
  void SetDeviceHeightInInches(const double height);
  void SetDeviceWidthInInches(const double width);
  void SetSize(int width, int height);
  void SetDeviceHeightInPixels(const double height);
  void SetDeviceWidthInPixels(const double width);
};
typedef CList<EoGsViewport> CViewports;
