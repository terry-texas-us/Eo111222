#pragma once
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

namespace EoGeOcs {
  // Convert a point given in OCS (x,y,z) to WCS using the extrusion normal.
  void OcsToWcs(const EoGeVector3d& extrusionNormal, const EoGePoint3d& ocsPt, EoGePoint3d& wcsPt);

  // Convert a WCS point to OCS coordinates for a given extrusion normal.
  void WcsToOcs(const EoGeVector3d& extrusionNormal, const EoGePoint3d& wcsPt, EoGePoint3d& ocsPt);
}