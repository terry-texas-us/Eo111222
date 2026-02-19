#include "Stdafx.h"

#include <cmath>

#include "EoGeOcs.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

namespace EoGeOcs {

static void MakeOrthonormalBasis(EoGeVector3d n, EoGeVector3d& u, EoGeVector3d& v) {
  // normalize n, fallback to Z if degenerate
  if (n.IsNearNull()) { n = EoGeVector3d(0.0, 0.0, 1.0); }
  n.Normalize();
  
  u = ComputeArbitraryAxis(n);
  if (u.IsNearNull()) { u = EoGeVector3d(1.0, 0.0, 0.0); }
  u.Normalize();
  
  v = CrossProduct(n, u);
  if (v.IsNearNull()) {
    // choose a perpendicular if cross failed
    if (std::abs(n.x) < std::abs(n.y)) {
      v = CrossProduct(n, EoGeVector3d(1.0, 0.0, 0.0));
    } else {
      v = CrossProduct(n, EoGeVector3d(0.0, 1.0, 0.0));
    }
  }
  v.Normalize();
  // ensure u is exactly perpendicular again
  u = CrossProduct(v, n);
  u.Normalize();
}

// OCS -> WCS: W = u*x + v*y + n*z
void OcsToWcs(const EoGeVector3d& extrusionNormal, const EoGePoint3d& ocsPt, EoGePoint3d& wcsPt) {
  EoGeVector3d n = extrusionNormal;
  EoGeVector3d u, v;
  MakeOrthonormalBasis(n, u, v);

  wcsPt.x = u.x * ocsPt.x + v.x * ocsPt.y + n.x * ocsPt.z;
  wcsPt.y = u.y * ocsPt.x + v.y * ocsPt.y + n.y * ocsPt.z;
  wcsPt.z = u.z * ocsPt.x + v.z * ocsPt.y + n.z * ocsPt.z;
}

// WCS -> OCS: x = dot(wcs, u), y = dot(wcs, v), z = dot(wcs, n)
void WcsToOcs(const EoGeVector3d& extrusionNormal, const EoGePoint3d& wcsPt, EoGePoint3d& ocsPt) {
  EoGeVector3d n = extrusionNormal;
  EoGeVector3d u, v;
  MakeOrthonormalBasis(n, u, v);

  ocsPt.x = DotProduct(EoGeVector3d(wcsPt.x, wcsPt.y, wcsPt.z), u);
  ocsPt.y = DotProduct(EoGeVector3d(wcsPt.x, wcsPt.y, wcsPt.z), v);
  ocsPt.z = DotProduct(EoGeVector3d(wcsPt.x, wcsPt.y, wcsPt.z), n);
}

}  // namespace EoGeOcs