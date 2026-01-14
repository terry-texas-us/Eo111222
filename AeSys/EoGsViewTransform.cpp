#include "stdafx.h"

#include <DirectXMath.h>

#include "EoGeMatrix.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGsViewTransform.h"
#include "EoGsViewport.h"

EoGsViewTransform::EoGsViewTransform() {
  m_UMin = -1.0;
  m_VMin = -1.0;
  m_UMax = 1.0;
  m_VMax = 1.0;
}

EoGsViewTransform::EoGsViewTransform(EoGsViewTransform& src) : EoGsAbstractView(src) {
  m_UMin = src.m_UMin;
  m_VMin = src.m_VMin;
  m_UMax = src.m_UMax;
  m_VMax = src.m_VMax;

  m_Matrix = src.m_Matrix;
  m_InverseMatrix = src.m_InverseMatrix;
}

EoGsViewTransform& EoGsViewTransform::operator=(const EoGsViewTransform& src) {
  EoGsAbstractView::operator=(src);

  m_UMin = src.m_UMin;
  m_VMin = src.m_VMin;
  m_UMax = src.m_UMax;
  m_VMax = src.m_VMax;

  m_Matrix = src.m_Matrix;
  m_InverseMatrix = src.m_InverseMatrix;

  return *this;
}
void EoGsViewTransform::AdjustWindow(const double aspectRatio) {
  double UExtent = m_UMax - m_UMin;
  double VExtent = m_VMax - m_VMin;

  if (UExtent <= FLT_EPSILON || VExtent / UExtent > aspectRatio) {
    double Adjustment = (VExtent / aspectRatio - UExtent) * 0.5;
    m_UMin -= Adjustment;
    m_UMax += Adjustment;
  } else {
    double Adjustment = (UExtent * aspectRatio - VExtent) * 0.5;
    m_VMin -= Adjustment;
    m_VMax += Adjustment;
  }
  BuildTransformMatrix();
}
void EoGsViewTransform::BuildTransformMatrix() {
  // View space, sometimes called camera space, is similar to world space in that it is typically used for the entire scene.
  // However, in view space, the origin is at the viewer or camera. The view direction (where the viewer is looking) defines the positive Z axis.
  // An "up" direction defined by the application becomes the positive Y axis.

  m_Matrix.Identity();

  EoGeVector3d vN = Target() - Position();
  vN.Normalize();

  EoGeVector3d vU = EoGeCrossProduct(ViewUp(), vN);
  vU.Normalize();

  EoGeVector3d vV = EoGeCrossProduct(vN, vU);
  vV.Normalize();

  EoGeVector3d vector = EoGeVector3d(Position(), EoGePoint3d::kOrigin);

  m_Matrix[0][0] = vU.x;
  m_Matrix[0][1] = vU.y;
  m_Matrix[0][2] = vU.z;
  m_Matrix[0][3] = EoGeDotProduct(vector, vU);

  m_Matrix[1][0] = vV.x;
  m_Matrix[1][1] = vV.y;
  m_Matrix[1][2] = vV.z;
  m_Matrix[1][3] = EoGeDotProduct(vector, vV);

  m_Matrix[2][0] = vN.x;
  m_Matrix[2][1] = vN.y;
  m_Matrix[2][2] = vN.z;
  m_Matrix[2][3] = EoGeDotProduct(vector, vN);

  m_Matrix[3][0] = 0.;
  m_Matrix[3][1] = 0.;
  m_Matrix[3][2] = 0.;
  m_Matrix[3][3] = 1.;

  DirectX::XMVECTOR XMPosition = DirectX::XMLoadFloat3(&mx_Position);
  DirectX::XMVECTOR XMTarget = DirectX::XMLoadFloat3(&mx_Target);
  DirectX::XMVECTOR XMViewUp = DirectX::XMLoadFloat3(&mx_ViewUp);
  DirectX::XMMATRIX XViewMatrix = DirectX::XMMatrixLookAtRH(XMPosition, XMTarget, XMViewUp);

  XViewMatrix = DirectX::XMMatrixTranspose(XViewMatrix);

  // Projection space refers to the space after applying projection transformation from view space.
  // In this space, visible content has X and Y coordinates ranging from -1 to 1, and Z coordinate ranging from 0 to 1.

  m_ProjectionMatrix.Identity();

  double UExtent = m_UMax - m_UMin;
  double VExtent = m_VMax - m_VMin;
  double NExtent = m_FarClipDistance - m_NearClipDistance;

  if (IsPerspectiveOn()) {
    m_ProjectionMatrix[0][0] = 2.0f * m_NearClipDistance / UExtent;
    m_ProjectionMatrix[0][1] = 0.0f;
    m_ProjectionMatrix[0][2] = (m_UMax + m_UMin) / UExtent;
    m_ProjectionMatrix[0][3] = 0.0f;

    m_ProjectionMatrix[1][0] = 0.0f;
    m_ProjectionMatrix[1][1] = (2.0f * m_NearClipDistance) / VExtent;
    m_ProjectionMatrix[1][2] = (m_VMax + m_VMin) / VExtent;
    m_ProjectionMatrix[1][3] = 0.0f;

    m_ProjectionMatrix[2][0] = 0.0f;
    m_ProjectionMatrix[2][1] = 0.0f;
    m_ProjectionMatrix[2][2] = -(m_FarClipDistance + m_NearClipDistance) / NExtent;
    m_ProjectionMatrix[2][3] = -2.0f * m_FarClipDistance * m_NearClipDistance / NExtent;

    m_ProjectionMatrix[3][0] = 0.0f;
    m_ProjectionMatrix[3][1] = 0.0f;
    m_ProjectionMatrix[3][2] = -1.0f;
    m_ProjectionMatrix[3][3] = 0.0f;
  } else {
    m_ProjectionMatrix[0][0] = 2.0f / UExtent;
    m_ProjectionMatrix[0][1] = 0.0f;
    m_ProjectionMatrix[0][2] = 0.0f;
    m_ProjectionMatrix[0][3] = -(m_UMax + m_UMin) / UExtent;

    m_ProjectionMatrix[1][0] = 0.0f;
    m_ProjectionMatrix[1][1] = 2.0f / VExtent;
    m_ProjectionMatrix[1][2] = 0.0f;
    m_ProjectionMatrix[1][3] = -(m_VMax + m_VMin) / VExtent;

    m_ProjectionMatrix[2][0] = 0.0f;
    m_ProjectionMatrix[2][1] = 0.0f;
    m_ProjectionMatrix[2][2] = -2.0f / NExtent;
    m_ProjectionMatrix[2][3] = -(m_FarClipDistance + m_NearClipDistance) / NExtent;

    m_ProjectionMatrix[3][0] = 0.0f;
    m_ProjectionMatrix[3][1] = 0.0f;
    m_ProjectionMatrix[3][2] = 0.0f;
    m_ProjectionMatrix[3][3] = 1.0f;

    DirectX::XMMATRIX XProjectionMatrix = DirectX::XMMatrixOrthographicRH(static_cast<float>(UExtent), static_cast<float>(VExtent),
                                                                          static_cast<float>(m_NearClipDistance), static_cast<float>(m_FarClipDistance));
    XProjectionMatrix = XMMatrixTranspose(XProjectionMatrix);
  }
  m_Matrix *= m_ProjectionMatrix;

  m_InverseMatrix = m_Matrix;
  m_InverseMatrix.Inverse();
}
EoGeTransformMatrix& EoGsViewTransform::GetMatrix() { return m_Matrix; }
EoGeTransformMatrix& EoGsViewTransform::GetMatrixInverse() { return m_InverseMatrix; }
void EoGsViewTransform::Initialize(const EoGsViewport& viewport) {
  SetCenteredWindow(viewport, 44.0, 34.0);

  EoGePoint3d Target = EoGePoint3d(UExtent() / 2.0, VExtent() / 2.0, 0.0);
  EoGePoint3d Position = Target + (EoGeVector3d::kZAxis * m_LensLength);

  SetView(Position, Target, EoGeVector3d::kYAxis);
  SetDirection(EoGeVector3d::kZAxis);

  SetNearClipDistance(-100.0f);
  SetFarClipDistance(100.0f);

  EnablePerspective(false);

  BuildTransformMatrix();
}
void EoGsViewTransform::LoadIdentity() { m_Matrix.Identity(); }
void EoGsViewTransform::ZAxisRotation(double dSinAng, double dCosAng) {
  EoGeTransformMatrix tm;
  tm.ZAxisRotation(dSinAng, dCosAng);
  m_Matrix *= tm;
}
void EoGsViewTransform::Scale(EoGeVector3d v) {
  EoGeTransformMatrix tm;
  tm.Scale(v);
  m_Matrix *= tm;
}
void EoGsViewTransform::SetCenteredWindow(const EoGsViewport& viewport, double uExtent, double vExtent) {
  if (uExtent == 0.0) { uExtent = UExtent(); }
  if (vExtent == 0.0) { vExtent = VExtent(); }
  double AspectRatio = viewport.HeightInInches() / viewport.WidthInInches();

  if (AspectRatio < vExtent / uExtent) {
    uExtent = vExtent / AspectRatio;
  } else {
    vExtent = uExtent * AspectRatio;
  }
  SetWindow(-uExtent * 0.5, -vExtent * 0.5, uExtent * 0.5, vExtent * 0.5);
}
void EoGsViewTransform::SetMatrix(EoGeTransformMatrix& tm) { m_Matrix = tm; }
void EoGsViewTransform::SetWindow(const double uMin, const double vMin, const double uMax, const double vMax) {
  m_UMin = uMin;
  m_VMin = vMin;
  m_UMax = uMax;
  m_VMax = vMax;

  BuildTransformMatrix();
}
void EoGsViewTransform::TransformPoint(EoGePoint4d& point) { point = m_Matrix * point; }
void EoGsViewTransform::TransformPoints(EoGePoint4dArray& pointsArray) {
  int iPts = (int)pointsArray.GetSize();
  for (int i = 0; i < iPts; i++) { pointsArray[i] = m_Matrix * pointsArray[i]; }
}
void EoGsViewTransform::TransformPoints(int numberOfPoints, EoGePoint4d* points) {
  for (int i = 0; i < numberOfPoints; i++) { points[i] = m_Matrix * points[i]; }
}
void EoGsViewTransform::TransformVector(EoGeVector3d& vector) { vector = m_Matrix * vector; }
void EoGsViewTransform::Translate(EoGeVector3d v) { m_Matrix.Translate(v); }
double EoGsViewTransform::UExtent() const { return static_cast<double>(m_UMax - m_UMin); }
double EoGsViewTransform::UMax() const { return static_cast<double>(m_UMax); }
double EoGsViewTransform::UMin() const { return static_cast<double>(m_UMin); }
double EoGsViewTransform::VExtent() const { return static_cast<double>(m_VMax - m_VMin); }
double EoGsViewTransform::VMax() const { return static_cast<double>(m_VMax); }
double EoGsViewTransform::VMin() const { return static_cast<double>(m_VMin); }
