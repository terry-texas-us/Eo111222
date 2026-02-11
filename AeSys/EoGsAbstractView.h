#pragma once

#include <DirectXMath.h>

#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
/*
The view target and the direction create a display coordinate system (DCS).
The z-direction equals the direction from target to camera (points toward viewer)
The DCS x-direction is calculated as cross product of (0,0,1) and the view direction.
If the view direction is parallel to (0,0,1), the x-direction is (1,0,0).
The DCS origin is the target point.

Parallel projections:
	The view may be offset from the line of sight (target point may be off the screen)
	A view need additional data to define DCS rectangle to be displayed. The rectangle's
	center is a 2d point (located on the DCS xy plane). Its size is given by its height and width;
	its rotation around its center is given be the twist angle.

Perspective projections:
	The DCS rectangle is always centered around the target point (DCS origin).
	Its size is taken from the views length. Start with a rectangle of 42 units diagonal length
	located at lens length distance from camera point.
	By dividing 42 times the view distance by the lens length, getting the diagonal length of the DCS rectangle.
	Next, select a rectangle with the same proportions as the views width and height, and rotate this by view twist angle.
*/

class EoGsAbstractView {
 public:
  static const std::int16_t AV_PERSPECTIVE = 0x01;        // bit 1 Perspective mode flag for this view
  static const std::int16_t AV_NEARCLIPPING = 0x02;       // bit 2 Near (Front) clipping plane status for this view
  static const std::int16_t AV_FARCLIPPING = 0x04;        // bit 3 Far (Back) clipping plane status for this view
  static const std::int16_t AV_NEARCLIPPINGATEYE = 0x10;  // bit 16 Front clipping plane is located at the camera

 protected:
  std::int16_t m_ViewMode;
  std::int16_t m_RenderMode;        // (not used)
  std::int16_t m_UcsOrthoViewType;  // Orthographic type of UCS; top 1, bottom 2, front 3, back 4, left 5, right 6 (not used)

  EoGePoint3d m_UCSOrigin;
  EoGeVector3d m_UCSXAxis;
  EoGeVector3d m_UCSYAxis;

  double m_Elevation;  // elevation of the UCS plane for this view

  DirectX::XMFLOAT3 mx_Position;
  DirectX::XMFLOAT3 mx_Target;
  DirectX::XMFLOAT3 mx_Direction;
  DirectX::XMFLOAT3 mx_ViewUp;

  // View-Specific coordinate systems
  double m_Height;
  double m_Width;
  double m_TwistAngle;  // in radians

  double m_LensLength;  // lens length used for perspective mode in this view
  double m_NearClipDistance;   // distance from the target to the near (front) clipping plane along the target-camera line.
  double m_FarClipDistance;  // distance from the target to the far (back) clipping plane along the target-camera line

 public:  // Constructors and destructor
  EoGsAbstractView();
  EoGsAbstractView(const EoGsAbstractView& av);
  virtual ~EoGsAbstractView() {}

 public:  // Operators
  EoGsAbstractView& operator=(const EoGsAbstractView& av);

 public:  // Methods
  [[nodiscard]] EoGeVector3d Direction() const;
  void EnableFarClipping(bool enabled);
  void EnableNearClipping(bool enabled);
  void EnablePerspective(bool enabled);
  /// <summary> Returns the far (back) clip distance from the target of this Viewport object.</summary>
  double FarClipDistance() const;
  double Height() const;
  bool IsFarClipOn() const;
  bool IsNearClipAtEyeOn() const;
  bool IsNearClipOn() const;
  bool IsPerspectiveOn() const;
  double LensLength() const;
  /// <summary> Returns the near (front) clip distance from the target of this Viewport object. </summary>
  double NearClipDistance() const;
  /// <summary> Returns the WCS camera (eye) location for this Viewport object.</summary>
  EoGePoint3d Position() const;
  void SetFarClipDistance(const double distance);
  void SetDirection(const EoGeVector3d& direction);
  void SetLensLength(const double length);
  void SetNearClipDistance(const double distance);
  void SetPosition(const EoGePoint3d& position);
  void SetPosition(const EoGeVector3d& direction);
  void SetTarget(const EoGePoint3d& target);
  void SetView(const EoGePoint3d& position, const EoGePoint3d& target, const EoGeVector3d& viewUp);
  void SetViewUp(EoGeVector3d viewUp);

  /// <summary> Returns the WCS camera target for this Viewport object.</summary>
  EoGePoint3d Target() const;
  /// <summary> Returns the WCS camera �up� vector for this Viewport object.</summary>
  EoGeVector3d ViewUp() const;
};
