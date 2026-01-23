#include "Stdafx.h"

#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "EoGsAbstractView.h"

EoGsAbstractView::EoGsAbstractView() {
  m_ViewMode = 0;
  m_RenderMode = 0;
  m_UcsOrthoViewType = 1;

  m_UCSOrigin = EoGePoint3d::kOrigin;
  m_UCSXAxis = EoGeVector3d::positiveUnitX;
  m_UCSYAxis = EoGeVector3d::positiveUnitY;
  m_Elevation = 0.;

  m_LensLength = 50.0;

  SetTarget(EoGePoint3d::kOrigin);
  SetPosition(EoGePoint3d::kOrigin + EoGeVector3d::positiveUnitZ * m_LensLength);
  SetDirection(EoGeVector3d::positiveUnitZ);
  SetViewUp(EoGeVector3d::positiveUnitY);
  m_TwistAngle = 0.0;
  m_Height = 1.0;
  m_Width = 1.0;
  m_NearClipDistance = 20.0;
  m_FarClipDistance = 100.0;
}
EoGsAbstractView::EoGsAbstractView(const EoGsAbstractView& src) {
  m_ViewMode = src.m_ViewMode;
  m_RenderMode = src.m_RenderMode;

  m_UcsOrthoViewType = src.m_UcsOrthoViewType;
  m_UCSOrigin = src.m_UCSOrigin;
  m_UCSXAxis = src.m_UCSXAxis;
  m_UCSYAxis = src.m_UCSYAxis;
  m_Elevation = src.m_Elevation;

  mx_Position = src.mx_Position;
  mx_Target = src.mx_Target;
  mx_Direction = src.mx_Direction;
  mx_ViewUp = src.mx_ViewUp;
  m_TwistAngle = src.m_TwistAngle;
  m_Height = src.m_Height;
  m_Width = src.m_Width;
  m_LensLength = src.m_LensLength;
  m_NearClipDistance = src.m_NearClipDistance;
  m_FarClipDistance = src.m_FarClipDistance;
}

EoGsAbstractView& EoGsAbstractView::operator=(const EoGsAbstractView& src) {
  m_ViewMode = src.m_ViewMode;
  m_RenderMode = src.m_RenderMode;

  m_UcsOrthoViewType = src.m_UcsOrthoViewType;
  m_UCSOrigin = src.m_UCSOrigin;
  m_UCSXAxis = src.m_UCSXAxis;
  m_UCSYAxis = src.m_UCSYAxis;
  m_Elevation = src.m_Elevation;

  mx_Position = src.mx_Position;
  mx_Target = src.mx_Target;
  mx_Direction = src.mx_Direction;
  mx_ViewUp = src.mx_ViewUp;
  m_TwistAngle = src.m_TwistAngle;
  m_Height = src.m_Height;
  m_Width = src.m_Width;
  m_LensLength = src.m_LensLength;
  m_NearClipDistance = src.m_NearClipDistance;
  m_FarClipDistance = src.m_FarClipDistance;

  return *this;
}
double EoGsAbstractView::FarClipDistance() const { return m_FarClipDistance; }
EoGeVector3d EoGsAbstractView::Direction() const {
  return EoGeVector3d(mx_Direction.x, mx_Direction.y, mx_Direction.z);
}
double EoGsAbstractView::NearClipDistance() const { return m_NearClipDistance; }
double EoGsAbstractView::Height() const { return m_Height; }
bool EoGsAbstractView::IsNearClipOn() const { return ((m_ViewMode & AV_NEARCLIPPING) == AV_NEARCLIPPING); }
bool EoGsAbstractView::IsNearClipAtEyeOn() const {
  return ((m_ViewMode & AV_NEARCLIPPINGATEYE) == AV_NEARCLIPPINGATEYE);
}
bool EoGsAbstractView::IsFarClipOn() const { return ((m_ViewMode & AV_FARCLIPPING) == AV_FARCLIPPING); }
bool EoGsAbstractView::IsPerspectiveOn() const { return ((m_ViewMode & AV_PERSPECTIVE) == AV_PERSPECTIVE); }
double EoGsAbstractView::LensLength() const { return m_LensLength; }
EoGePoint3d EoGsAbstractView::Position() const { return EoGePoint3d(mx_Position.x, mx_Position.y, mx_Position.z); }
void EoGsAbstractView::SetFarClipDistance(const double distance) { m_FarClipDistance = distance; }
void EoGsAbstractView::EnableFarClipping(bool enabled) {
  if (enabled) {
    m_ViewMode |= AV_FARCLIPPING;
  } else {
    m_ViewMode &= ~AV_FARCLIPPING;
  }
}
void EoGsAbstractView::SetDirection(const EoGeVector3d& direction) {
  if (direction.Length() > FLT_EPSILON) {
    EoGeVector3d Direction(direction);
    Direction.Normalize();
    mx_Direction.x = static_cast<float>(Direction.x);
    mx_Direction.y = static_cast<float>(Direction.y);
    mx_Direction.z = static_cast<float>(Direction.z);
  }
}
void EoGsAbstractView::SetNearClipDistance(const double distance) { m_NearClipDistance = distance; }
void EoGsAbstractView::EnableNearClipping(bool enabled) {
  if (enabled) {
    m_ViewMode |= AV_NEARCLIPPING;
  } else {
    m_ViewMode &= ~AV_NEARCLIPPING;
  }
}
void EoGsAbstractView::SetLensLength(const double length) { m_LensLength = length; }
void EoGsAbstractView::EnablePerspective(bool enabled) {
  if (enabled) {
    m_ViewMode |= AV_PERSPECTIVE;
  } else {
    m_ViewMode &= ~AV_PERSPECTIVE;
  }
}
void EoGsAbstractView::SetPosition(const EoGePoint3d& position) {
  mx_Position.x = static_cast<float>(position.x);
  mx_Position.y = static_cast<float>(position.y);
  mx_Position.z = static_cast<float>(position.z);
}
void EoGsAbstractView::SetPosition(const EoGeVector3d& direction) {
  SetPosition(Target() + (direction * m_LensLength));
}
void EoGsAbstractView::SetTarget(const EoGePoint3d& target) {
  mx_Target.x = static_cast<float>(target.x);
  mx_Target.y = static_cast<float>(target.y);
  mx_Target.z = static_cast<float>(target.z);
}
void EoGsAbstractView::SetView(const EoGePoint3d& position, const EoGePoint3d& target, const EoGeVector3d& viewUp) {
  SetPosition(position);
  SetTarget(target);
  SetViewUp(viewUp);
}
void EoGsAbstractView::SetViewUp(EoGeVector3d viewUp) {
  if (viewUp.Length() > DBL_EPSILON) {
    viewUp.Normalize();
    mx_ViewUp.x = static_cast<float>(viewUp.x);
    mx_ViewUp.y = static_cast<float>(viewUp.y);
    mx_ViewUp.z = static_cast<float>(viewUp.z);
  }
}
EoGePoint3d EoGsAbstractView::Target() const { return EoGePoint3d(mx_Target.x, mx_Target.y, mx_Target.z); }
EoGeVector3d EoGsAbstractView::ViewUp() const { return EoGeVector3d(mx_ViewUp.x, mx_ViewUp.y, mx_ViewUp.z); }
