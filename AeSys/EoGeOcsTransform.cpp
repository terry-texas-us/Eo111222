#include "Stdafx.h"

#include "EoGeOcsTransform.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

EoGeOcsTransform::EoGeOcsTransform(const EoGeVector3d& extrusionNormal)
    : EoGeTransformMatrix(), m_extrusionNormal(extrusionNormal) {
  BuildOcsTransformation(extrusionNormal);
}

EoGeOcsTransform::EoGeOcsTransform(const EoGePoint3d& origin, const EoGeVector3d& extrusionNormal)
    : EoGeTransformMatrix(), m_extrusionNormal(extrusionNormal) {
  BuildOcsTransformation(extrusionNormal);

  // Add translation to origin
  m_4X4[0][3] = origin.x;
  m_4X4[1][3] = origin.y;
  m_4X4[2][3] = origin.z;
}

void EoGeOcsTransform::SetExtrusionNormal(const EoGeVector3d& extrusionNormal) {
  m_extrusionNormal = extrusionNormal;
  BuildOcsTransformation(extrusionNormal);
}

void EoGeOcsTransform::BuildOcsTransformation(const EoGeVector3d& extrusionNormal) {
  // Normalize the extrusion normal (OCS Z-axis)
  EoGeVector3d n = extrusionNormal;
  if (n.IsNearNull()) { n = EoGeVector3d::positiveUnitZ; }
  n.Normalize();
  m_extrusionNormal = n;

  // Use the DXF arbitrary axis algorithm to compute OCS X-axis
  EoGeVector3d u = ComputeArbitraryAxis(n);
  if (u.IsNearNull()) { u = EoGeVector3d::positiveUnitX; }
  u.Normalize();

  // Compute OCS Y-axis as cross product
  auto v = CrossProduct(n, u);
  if (v.IsNearNull()) {
    // Fallback if cross product failed
    if (fabs(n.x) < fabs(n.y)) {
      v = CrossProduct(n, EoGeVector3d::positiveUnitX);
    } else {
      v = CrossProduct(n, EoGeVector3d::positiveUnitY);
    }
  }
  v.Normalize();

  // Ensure u is exactly perpendicular (Gram-Schmidt orthogonalization)
  u = CrossProduct(v, n);
  u.Normalize();

  // Build the transformation matrix
  // Matrix columns are the basis vectors: [u v n]
  // This transforms OCS coordinates to WCS: WCS = [u v n] * OCS
  Identity();

  m_4X4[0][0] = u.x;
  m_4X4[0][1] = v.x;
  m_4X4[0][2] = n.x;
  m_4X4[1][0] = u.y;
  m_4X4[1][1] = v.y;
  m_4X4[1][2] = n.y;
  m_4X4[2][0] = u.z;
  m_4X4[2][1] = v.z;
  m_4X4[2][2] = n.z;
}

EoGeVector3d EoGeOcsTransform::GetOcsXAxis() const noexcept { return EoGeVector3d(m_4X4[0][0], m_4X4[1][0], m_4X4[2][0]); }

EoGeVector3d EoGeOcsTransform::GetOcsYAxis() const noexcept { return EoGeVector3d(m_4X4[0][1], m_4X4[1][1], m_4X4[2][1]); }

EoGeOcsTransform EoGeOcsTransform::GetInverseOcsTransform() const {
  EoGeOcsTransform inverse(*this);
  inverse.Inverse();  // Use base class inverse
  return inverse;
}

bool EoGeOcsTransform::IsWorldCoordinateSystem(double tolerance) const noexcept {
  return m_extrusionNormal.IsEqualTo(EoGeVector3d::positiveUnitZ, tolerance);
}

EoGeOcsTransform EoGeOcsTransform::CreateOcsToWcs(const EoGeVector3d& extrusionNormal) {
  return EoGeOcsTransform(extrusionNormal);
}

EoGeOcsTransform EoGeOcsTransform::CreateWcsToOcs(const EoGeVector3d& extrusionNormal) {
  EoGeOcsTransform transform(extrusionNormal);
  transform.Inverse();  // Invert for WCS to OCS
  return transform;
}