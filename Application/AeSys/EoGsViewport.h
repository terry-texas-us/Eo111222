#pragma once

class EoGsViewport {
  double m_DeviceHeightInPixels {};
  double m_DeviceWidthInPixels {};
  double m_DeviceHeightInInches {};
  double m_DeviceWidthInInches {};
  double m_height {};
  double m_width {};

  /** @brief Projects a point from normalized device coordinates to client coordinates using the viewport's dimensions.
   * @param x The x-coordinate of the point in normalized device coordinates (NDC).
   * @param y The y-coordinate of the point in normalized device coordinates (NDC).
   * @param w The homogeneous coordinate (w) of the point, used for perspective division.
   * @return A CPoint representing the projected point in client coordinates.
   * @note The function maps NDC coordinates in the range [-1, 1] to client coordinates in the range [0, width] for x and [0, height] for y. The y-coordinate is inverted to account for the difference in coordinate systems between NDC and client coordinates.
   */
  CPoint ProjectToClient(double x, double y, double w) const noexcept;

 public:  // Constructors and destructors
  EoGsViewport() = default;
  EoGsViewport(const EoGsViewport& src);

  ~EoGsViewport() = default;
  EoGsViewport& operator=(const EoGsViewport& src);

  [[nodiscard]] CPoint ProjectToClient(const EoGePoint4d& ndcPoint) const;
  
  void ProjectToClient(CPoint* clientPoints, int numberOfPoints, const EoGePoint4d* ndcPoints) const;
  
  /** @brief Projects NDC points to client coordinates using the viewport's dimensions.
   * @param pnt An array of CPoint objects that will receive the projected client coordinates.
   * @param pointsArray An array of EoGePoint4d objects representing the 3D points in homogeneous coordinates to be projected.
   * @note Each point in pointsArray is expected to be in homogeneous coordinates (x, y, z, w). The function divides each point by its w component to convert it to normalized device coordinates before projecting it to screen space.
   */
  void ProjectToClient(CPoint* clientPoints, EoGePoint4dArray& ndcPoints) const;
  
  void DoProjectionInverse(EoGePoint3d& pt);

  [[nodiscard]] double Height() const noexcept { return m_height; }
  [[nodiscard]] double HeightInInches() const;
  [[nodiscard]] double Width() const noexcept { return m_width; }

  /** @brief Retrieves the width of the viewport in inches.
   * @return The width of the viewport in inches.
   * @note This function returns the physical width of the viewport based on the device's characteristics, which may differ from the logical width used for projection calculations.
   */
  [[nodiscard]] double WidthInInches() const;
  
  void SetDeviceHeightInInches(double height);
  void SetDeviceWidthInInches(double width);
  void SetSize(int width, int height);
  void SetDeviceHeightInPixels(double height);
  void SetDeviceWidthInPixels(double width);
};
typedef CList<EoGsViewport> CViewports;
