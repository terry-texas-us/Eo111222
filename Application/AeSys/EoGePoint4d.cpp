#include "Stdafx.h"

#include <algorithm>

#include "EoGeLine.h"

void EoGePoint4d::operator/=(double t) {
  assert(Eo::IsGeometricallyNonZero(t) && "Division by near-zero in EoGePoint4d::operator/=");
  if (Eo::IsGeometricallyNonZero(t)) {
    x /= t;
    y /= t;
    z /= t;
    w /= t;
  }
  // Silently unchanged in release if t ≈ 0
}

/** @brief Divides the point by a scalar value, returning a new point.
 * @param t The scalar value to divide the point by.
 * @return A new EoGePoint4d resulting from the division.
 * @note This operator does not modify the original point; it returns a new point with the result of the division.
 */
EoGePoint4d EoGePoint4d::operator/(double t) const {
  assert(Eo::IsGeometricallyNonZero(t) && "Division by near-zero in EoGePoint4d::operator/");
  if (Eo::IsGeometricallyNonZero(t)) { return {x / t, y / t, z / t, w / t}; }
  // Silently unchanged in release if t ≈ 0
  return *this;
}

bool EoGePoint4d::operator==(const EoGePoint4d& point) const noexcept {
  return IsEqualTo(point, Eo::geometricTolerance);
}

bool EoGePoint4d::operator!=(const EoGePoint4d& point) const noexcept {
  return !IsEqualTo(point, Eo::geometricTolerance);
}

bool EoGePoint4d::IsEqualTo(const EoGePoint4d& p, double tolerance) const noexcept {
  return std::abs(x - p.x) <= tolerance && std::abs(y - p.y) <= tolerance && std::abs(z - p.z) <= tolerance
      && std::abs(w - p.w) <= tolerance;
}

bool EoGePoint4d::ClipLine(EoGePoint4d& ptA, EoGePoint4d& ptB) {
  const double boundaryCodeA[] = {
      ptA.w + ptA.x, ptA.w - ptA.x, ptA.w + ptA.y, ptA.w - ptA.y, ptA.w + ptA.z, ptA.w - ptA.z};
  const double boundaryCodeB[] = {
      ptB.w + ptB.x, ptB.w - ptB.x, ptB.w + ptB.y, ptB.w - ptB.y, ptB.w + ptB.z, ptB.w - ptB.z};

  int outCodeA = 0;
  int outCodeB = 0;

  for (int iBC = 0; iBC < 6; iBC++) {
    if (boundaryCodeA[iBC] <= 0.0) { outCodeA |= (1 << iBC); }
    if (boundaryCodeB[iBC] <= 0.0) { outCodeB |= (1 << iBC); }
  }

  if ((outCodeA & outCodeB) != 0) { return false; }
  if ((outCodeA | outCodeB) == 0) { return true; }

  double dTIn{};
  double dTOut{1.0};

  double dTHit;

  for (int i = 0; i < 6; i++) {
    if (boundaryCodeB[i] < 0.0) {
      dTHit = boundaryCodeA[i] / (boundaryCodeA[i] - boundaryCodeB[i]);
      dTOut = std::min(dTOut, dTHit);
    } else if (boundaryCodeA[i] < 0.0) {
      dTHit = boundaryCodeA[i] / (boundaryCodeA[i] - boundaryCodeB[i]);
      dTIn = std::max(dTIn, dTHit);
    }
    if (dTIn > dTOut) { return false; }
  }
  const EoGePoint4d pt(ptA);

  if (outCodeA != 0) { ptA = pt + (ptB - pt) * dTIn; }
  if (outCodeB != 0) { ptB = pt + (ptB - pt) * dTOut; }
  return true;
}

void EoGePoint4d::ClipPolygon(EoGePoint4dArray& pointsArray) {
  static EoGePoint4d ptPln[] = {EoGePoint4d(-1.0, 0.0, 0.0, 1.0),
      EoGePoint4d(1.0, 0.0, 0.0, 1.0),
      EoGePoint4d(0.0, -1.0, 0.0, 1.0),
      EoGePoint4d(0.0, 1.0, 0.0, 1.0),
      EoGePoint4d(0.0, 0.0, -1.0, 1.0),
      EoGePoint4d(0.0, 0.0, 1.0, 1.0)};

  static EoGeVector3d vPln[] = {EoGeVector3d(1.0, 0.0, 0.0),
      EoGeVector3d(-1.0, 0.0, 0.0),
      EoGeVector3d(0.0, 1.0, 0.0),
      EoGeVector3d(0.0, -1.0, 0.0),
      EoGeVector3d(0.0, 0.0, 1.0),
      EoGeVector3d(0.0, 0.0, -1.0)};

  EoGePoint4dArray pointsArrayOut;

  for (int iPln = 0; iPln < 6; iPln++) {
    IntersectionWithPln(pointsArray, ptPln[iPln], vPln[iPln], pointsArrayOut);

    const auto numberOfPointsOut = static_cast<int>(pointsArrayOut.GetSize());
    pointsArray.SetSize(numberOfPointsOut);

    if (numberOfPointsOut == 0) { break; }

    pointsArray.Copy(pointsArrayOut);
    pointsArrayOut.RemoveAll();
  }
}

void EoGePoint4d::IntersectionWithPln(EoGePoint4dArray& pointsArrayIn,
    const EoGePoint4d& pointOnClipPlane,
    const EoGeVector3d& clipPlaneNormal,
    EoGePoint4dArray& pointsArrayOut) {
  if (pointsArrayIn.IsEmpty()) { return; }

  EoGePoint4d pt;
  EoGePoint4d ptEdge[2]{};
  bool bEdgeVis[2]{};

  const bool bVisVer0 =
      DotProduct(EoGeVector3d(EoGePoint3d{pointOnClipPlane}, EoGePoint3d{pointsArrayIn[0]}), clipPlaneNormal)
          >= -Eo::geometricTolerance
      ? true
      : false;

  ptEdge[0] = pointsArrayIn[0];
  bEdgeVis[0] = bVisVer0;

  if (bVisVer0) { pointsArrayOut.Add(pointsArrayIn[0]); }
  const auto iPtsIn = static_cast<int>(pointsArrayIn.GetSize());
  for (int i = 1; i < iPtsIn; i++) {
    ptEdge[1] = pointsArrayIn[i];
    bEdgeVis[1] = DotProduct(EoGeVector3d(EoGePoint3d{pointOnClipPlane}, EoGePoint3d{ptEdge[1]}), clipPlaneNormal)
            >= -Eo::geometricTolerance
        ? true
        : false;

    if (bEdgeVis[0] != bEdgeVis[1]) {  // Vetices of edge on opposite sides of clip plane
      pt = EoGeLine::IntersectionWithPlane(ptEdge[0], ptEdge[1], pointOnClipPlane, clipPlaneNormal);
      pointsArrayOut.Add(pt);
    }
    if (bEdgeVis[1]) { pointsArrayOut.Add(pointsArrayIn[i]); }
    ptEdge[0] = ptEdge[1];
    bEdgeVis[0] = bEdgeVis[1];
  }
  if (pointsArrayOut.GetSize() != 0
      && bEdgeVis[0] != bVisVer0) {  // first and last vertices on opposite sides of clip plane
    pt = EoGeLine::IntersectionWithPlane(ptEdge[0], pointsArrayIn[0], pointOnClipPlane, clipPlaneNormal);
    pointsArrayOut.Add(pt);
  }
}

double EoGePoint4d::DistanceToPointXY(const EoGePoint4d& q) const noexcept {
  // Just dehomogenize the x and y components to calculate the distance, ignoring z.
  const double xDelta = q.x / q.w - x / w;
  const double yDelta = q.y / q.w - y / w;

  return std::sqrt(xDelta * xDelta + yDelta * yDelta);
}

bool EoGePoint4d::IsInView() const noexcept {
  return (w + x > 0.0 && w - x > 0.0 && w + y > 0.0 && w - y > 0.0 && w + z > 0.0 && w - z > 0.0);
}
