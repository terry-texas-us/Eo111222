#include "Stdafx.h"

#include <DirectXMath.h>

#include "EoGeMatrix.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsAbstractView.h"
#include "EoGsViewTransform.h"
#include "EoGsViewport.h"

EoGsViewTransform::EoGsViewTransform() {
  m_UMin = -1.0;
  m_VMin = -1.0;
  m_UMax = 1.0;
  m_VMax = 1.0;
}

EoGsViewTransform::EoGsViewTransform(const EoGsViewTransform& other) : EoGsAbstractView(other) {
  m_UMin = other.m_UMin;
  m_VMin = other.m_VMin;
  m_UMax = other.m_UMax;
  m_VMax = other.m_VMax;

  m_Matrix = other.m_Matrix;
  m_InverseMatrix = other.m_InverseMatrix;
}

EoGsViewTransform& EoGsViewTransform::operator=(const EoGsViewTransform& other) {
  EoGsAbstractView::operator=(other);

  m_UMin = other.m_UMin;
  m_VMin = other.m_VMin;
  m_UMax = other.m_UMax;
  m_VMax = other.m_VMax;

  m_Matrix = other.m_Matrix;
  m_InverseMatrix = other.m_InverseMatrix;

  return *this;
}
void EoGsViewTransform::AdjustWindow(const double aspectRatio) {
  double uExtent = m_UMax - m_UMin;
  double vExtent = m_VMax - m_VMin;

  if (uExtent < Eo::geometricTolerance || vExtent / uExtent > aspectRatio) {
    double adjustment = (vExtent / aspectRatio - uExtent) * 0.5;
    m_UMin -= adjustment;
    m_UMax += adjustment;
  } else {
    double adjustment = (uExtent * aspectRatio - vExtent) * 0.5;
    m_VMin -= adjustment;
    m_VMax += adjustment;
  }
  BuildTransformMatrix();
}
void EoGsViewTransform::BuildTransformMatrix() {

  m_Matrix.Identity();

  EoGeVector3d n = Position() - Target();
  n.Normalize();

  auto u = CrossProduct(ViewUp(), n);
  u.Normalize();

  auto v = CrossProduct(n, u);
  v.Normalize();

  EoGeVector3d vector = EoGeVector3d(Position(), EoGePoint3d::kOrigin);

  m_Matrix[0][0] = u.x;
  m_Matrix[0][1] = u.y;
  m_Matrix[0][2] = u.z;
  m_Matrix[0][3] = DotProduct(vector, u);

  m_Matrix[1][0] = v.x;
  m_Matrix[1][1] = v.y;
  m_Matrix[1][2] = v.z;
  m_Matrix[1][3] = DotProduct(vector, v);

  m_Matrix[2][0] = n.x;
  m_Matrix[2][1] = n.y;
  m_Matrix[2][2] = n.z;
  m_Matrix[2][3] = DotProduct(vector, n);

  m_Matrix[3][0] = 0.0;
  m_Matrix[3][1] = 0.0;
  m_Matrix[3][2] = 0.0;
  m_Matrix[3][3] = 1.0;

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

    DirectX::XMMATRIX XProjectionMatrix =
        DirectX::XMMatrixOrthographicRH(static_cast<float>(UExtent), static_cast<float>(VExtent),
                                        static_cast<float>(m_NearClipDistance), static_cast<float>(m_FarClipDistance));
    XProjectionMatrix = XMMatrixTranspose(XProjectionMatrix);
  }
  m_Matrix *= m_ProjectionMatrix;

  m_InverseMatrix = m_Matrix;
  m_InverseMatrix.Inverse();
}

[[nodiscard]] EoGeTransformMatrix& EoGsViewTransform::GetMatrix() { return m_Matrix; }

[[nodiscard]] EoGeTransformMatrix& EoGsViewTransform::GetMatrixInverse() { return m_InverseMatrix; }

void EoGsViewTransform::Initialize(const EoGsViewport& viewport) {
  SetCenteredWindow(viewport, 44.0, 34.0);

  auto target = EoGePoint3d(UExtent() / 2.0, VExtent() / 2.0, 0.0);
  auto position = target + (EoGeVector3d::positiveUnitZ * m_LensLength);

  SetView(position, target, EoGeVector3d::positiveUnitY);
  SetDirection(EoGeVector3d::positiveUnitZ);

  SetNearClipDistance(-100.0);
  SetFarClipDistance(100.0);

  EnablePerspective(false);

  BuildTransformMatrix();
}

void EoGsViewTransform::LoadIdentity() { m_Matrix.Identity(); }

void EoGsViewTransform::ZAxisRotation(double sinAngle, double cosAngle) {
  m_Matrix *= EoGeTransformMatrix::ZAxisRotation(sinAngle, cosAngle);
}

void EoGsViewTransform::Scale(EoGeVector3d v) {
  EoGeTransformMatrix transformMatrix;
  transformMatrix.Scale(v);
  m_Matrix *= transformMatrix;
}

void EoGsViewTransform::SetCenteredWindow(const EoGsViewport& viewport, double uExtent, double vExtent) {
  if (uExtent == 0.0) { uExtent = UExtent(); }
  if (vExtent == 0.0) { vExtent = VExtent(); }
  double aspectRatio = viewport.HeightInInches() / viewport.WidthInInches();

  if (aspectRatio < vExtent / uExtent) {
    uExtent = vExtent / aspectRatio;
  } else {
    vExtent = uExtent * aspectRatio;
  }
  SetWindow(-uExtent * 0.5, -vExtent * 0.5, uExtent * 0.5, vExtent * 0.5);
}

void EoGsViewTransform::SetMatrix(EoGeTransformMatrix& transformMatrix) { m_Matrix = transformMatrix; }

void EoGsViewTransform::SetWindow(const double uMin, const double vMin, const double uMax, const double vMax) {
  m_UMin = uMin;
  m_VMin = vMin;
  m_UMax = uMax;
  m_VMax = vMax;

  BuildTransformMatrix();
}

void EoGsViewTransform::TransformPoint(EoGePoint4d& point) { point = m_Matrix * point; }

void EoGsViewTransform::TransformPoints(EoGePoint4dArray& points) {
  int iPts = (int)points.GetSize();
  for (int i = 0; i < iPts; i++) { points[i] = m_Matrix * points[i]; }
}

void EoGsViewTransform::TransformPoints(int numberOfPoints, EoGePoint4d* points) {
  for (int i = 0; i < numberOfPoints; i++) { points[i] = m_Matrix * points[i]; }
}

void EoGsViewTransform::TransformVector(EoGeVector3d& vector) { vector = m_Matrix * vector; }
void EoGsViewTransform::Translate(EoGeVector3d v) { m_Matrix.Translate(v); }
[[nodiscard]] double EoGsViewTransform::UExtent() const { return static_cast<double>(m_UMax - m_UMin); }
[[nodiscard]] double EoGsViewTransform::UMax() const { return static_cast<double>(m_UMax); }
[[nodiscard]] double EoGsViewTransform::UMin() const { return static_cast<double>(m_UMin); }
[[nodiscard]] double EoGsViewTransform::VExtent() const { return static_cast<double>(m_VMax - m_VMin); }
[[nodiscard]] double EoGsViewTransform::VMax() const { return static_cast<double>(m_VMax); }
[[nodiscard]] double EoGsViewTransform::VMin() const { return static_cast<double>(m_VMin); }
