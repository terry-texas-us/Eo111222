#include "Stdafx.h"
#include <algorithm>

#include "EoGeLine.h"

EoGePoint4d::EoGePoint4d() { x = y = z = w = 0.; }
EoGePoint4d::EoGePoint4d(const double initialX, const double initialY, const double initialZ, const double initialW) {
  x = initialX;
  y = initialY;
  z = initialZ;
  w = initialW;
}
EoGePoint4d::EoGePoint4d(const EoGePoint3d& initialPoint) {
  x = initialPoint.x;
  y = initialPoint.y;
  z = initialPoint.z;
  w = 1.;
}
void EoGePoint4d::operator+=(EoGeVector3d& v) {
  x += v.x;
  y += v.y;
  z += v.z;
}
void EoGePoint4d::operator-=(EoGeVector3d& v) {
  x -= v.x;
  y -= v.y;
  z -= v.z;
}
void EoGePoint4d::operator*=(const double d) {
  x *= d;
  y *= d;
  z *= d;
  w *= d;
}
void EoGePoint4d::operator/=(const double d) {
  x /= d;
  y /= d;
  z /= d;
  w /= d;
}
void EoGePoint4d::operator()(const double newX, const double newY, const double newZ, const double newW) {
  x = newX;
  y = newY;
  z = newZ;
  w = newW;
}
EoGeVector3d EoGePoint4d::operator-(const EoGePoint4d& point) {
  return EoGeVector3d(x - point.x, y - point.y, z - point.z);
}
EoGePoint4d EoGePoint4d::operator-(EoGeVector3d vector) {
  return EoGePoint4d(x - vector.x, y - vector.y, z - vector.z, w);
}
EoGePoint4d EoGePoint4d::operator+(EoGeVector3d vector) {
  return EoGePoint4d(x + vector.x, y + vector.y, z + vector.z, w);
}
EoGePoint4d EoGePoint4d::operator*(const double t) { return EoGePoint4d(x * t, y * t, z * t, w * t); }
EoGePoint4d EoGePoint4d::operator/(const double t) { return EoGePoint4d(x / t, y / t, z / t, w / t); }
bool EoGePoint4d::ClipLine(EoGePoint4d& ptA, EoGePoint4d& ptB) {
  double BoundaryCodeA[] = {ptA.w + ptA.x, ptA.w - ptA.x, ptA.w + ptA.y, ptA.w - ptA.y, ptA.w + ptA.z, ptA.w - ptA.z};
  double BoundaryCodeB[] = {ptB.w + ptB.x, ptB.w - ptB.x, ptB.w + ptB.y, ptB.w - ptB.y, ptB.w + ptB.z, ptB.w - ptB.z};

  int OutCodeA = 0;
  int OutCodeB = 0;

  for (int iBC = 0; iBC < 6; iBC++) {
    if (BoundaryCodeA[iBC] <= 0.0) OutCodeA |= (1 << iBC);
    if (BoundaryCodeB[iBC] <= 0.0) OutCodeB |= (1 << iBC);
  }

  if ((OutCodeA & OutCodeB) != 0) return false;
  if ((OutCodeA | OutCodeB) == 0) return true;

  double dTIn = 0.;
  double dTOut = 1.;

  double dTHit;

  for (int i = 0; i < 6; i++) {
    if (BoundaryCodeB[i] < 0.0) {
      dTHit = BoundaryCodeA[i] / (BoundaryCodeA[i] - BoundaryCodeB[i]);
      dTOut = std::min(dTOut, dTHit);
    } else if (BoundaryCodeA[i] < 0.0) {
      dTHit = BoundaryCodeA[i] / (BoundaryCodeA[i] - BoundaryCodeB[i]);
      dTIn = std::max(dTIn, dTHit);
    }
    if (dTIn > dTOut) return false;
  }
  EoGePoint4d pt(ptA);

  if (OutCodeA != 0) { ptA = pt + (ptB - pt) * dTIn; }
  if (OutCodeB != 0) { ptB = pt + (ptB - pt) * dTOut; }
  return true;
}
void EoGePoint4d::ClipPolygon(EoGePoint4dArray& pointsArray) {
  static EoGePoint4d ptPln[] = {EoGePoint4d(-1.0, 0.0, 0.0, 1.0), EoGePoint4d(1.0, 0.0, 0.0, 1.0),
                                EoGePoint4d(0.0, -1.0, 0.0, 1.0), EoGePoint4d(0.0, 1.0, 0.0, 1.0),
                                EoGePoint4d(0.0, 0.0, -1.0, 1.0), EoGePoint4d(0.0, 0.0, 1.0, 1.0)};

  static EoGeVector3d vPln[] = {EoGeVector3d(1.0, 0.0, 0.0), EoGeVector3d(-1.0, 0.0, 0.0),
                                EoGeVector3d(0.0, 1.0, 0.0), EoGeVector3d(0.0, -1.0, 0.0),
                                EoGeVector3d(0.0, 0.0, 1.0), EoGeVector3d(0.0, 0.0, -1.0)};

  EoGePoint4dArray PointsArrayOut;

  for (int iPln = 0; iPln < 6; iPln++) {
    IntersectionWithPln(pointsArray, ptPln[iPln], vPln[iPln], PointsArrayOut);

    int iPtsOut = (int)PointsArrayOut.GetSize();
    pointsArray.SetSize(iPtsOut);

    if (iPtsOut == 0) break;

    pointsArray.Copy(PointsArrayOut);
    PointsArrayOut.RemoveAll();
  }
}
void EoGePoint4d::IntersectionWithPln(EoGePoint4dArray& pointsArrayIn, const EoGePoint4d& ptQ,
                                      EoGeVector3d& planeNormal, EoGePoint4dArray& pointsArrayOut) {
  if (pointsArrayIn.IsEmpty()) return;

  EoGePoint4d pt;
  EoGePoint4d ptEdge[2]{};
  bool bEdgeVis[2]{};

  bool bVisVer0 = DotProduct(EoGeVector3d(ptQ, pointsArrayIn[0]), planeNormal) >= -DBL_EPSILON ? true : false;

  ptEdge[0] = pointsArrayIn[0];
  bEdgeVis[0] = bVisVer0;

  if (bVisVer0) { pointsArrayOut.Add(pointsArrayIn[0]); }
  int iPtsIn = (int)pointsArrayIn.GetSize();
  for (int i = 1; i < iPtsIn; i++) {
    ptEdge[1] = pointsArrayIn[i];
    bEdgeVis[1] = DotProduct(EoGeVector3d(ptQ, ptEdge[1]), planeNormal) >= -DBL_EPSILON ? true : false;

    if (bEdgeVis[0] != bEdgeVis[1]) {  // Vetices of edge on opposite sides of clip plane
      pt = EoGeLine::IntersectionWithPln4(ptEdge[0], ptEdge[1], ptQ, planeNormal);
      pointsArrayOut.Add(pt);
    }
    if (bEdgeVis[1]) { pointsArrayOut.Add(pointsArrayIn[i]); }
    ptEdge[0] = ptEdge[1];
    bEdgeVis[0] = bEdgeVis[1];
  }
  if (pointsArrayOut.GetSize() != 0 &&
      bEdgeVis[0] != bVisVer0) {  // first and last vertices on opposite sides of clip plane
    pt = EoGeLine::IntersectionWithPln4(ptEdge[0], pointsArrayIn[0], ptQ, planeNormal);
    pointsArrayOut.Add(pt);
  }
}
double EoGePoint4d::DistanceToPointXY(const EoGePoint4d& ptQ) const {
  double X = ptQ.x / ptQ.w - x / w;
  double Y = ptQ.y / ptQ.w - y / w;

  return sqrt(X * X + Y * Y);
}
bool EoGePoint4d::IsInView() {
  if (w + x <= 0. || w - x <= 0.0) return false;
  if (w + y <= 0. || w - y <= 0.0) return false;
  if (w + z <= 0. || w - z <= 0.0) return false;

  return true;
}
EoGePoint4d EoGePoint4d::Max(EoGePoint4d& ptA, EoGePoint4d& ptB) {
  return EoGePoint4d(std::max(ptA.x, ptB.x), std::max(ptA.y, ptB.y), std::max(ptA.z, ptB.z), std::max(ptA.w, ptB.w));
}
EoGePoint4d EoGePoint4d::Min(EoGePoint4d& ptA, EoGePoint4d& ptB) {
  return EoGePoint4d(std::min(ptA.x, ptB.x), std::min(ptA.y, ptB.y), std::min(ptA.z, ptB.z), std::min(ptA.w, ptB.w));
}
