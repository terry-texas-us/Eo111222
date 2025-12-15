#include "stdafx.h"

#include "AeSysView.h"
#include "EoDbPrimitive.h"
#include "PrimState.h"

EoGeLine EoGeLine::operator-(EoGeVector3d v) { return (EoGeLine(begin - v, end - v)); }
EoGeLine EoGeLine::operator+(EoGeVector3d v) { return (EoGeLine(begin + v, end + v)); }
double EoGeLine::AngleFromXAxisXY() {
  EoGeVector3d Vector((*this).begin, (*this).end);

  double Angle = 0.;

  if (fabs(Vector.x) > DBL_EPSILON || fabs(Vector.y) > DBL_EPSILON) {
    Angle = atan2(Vector.y, Vector.x);

    if (Angle < 0.0) Angle += TWOPI;
  }
  return (Angle);
}
EoGePoint3d EoGeLine::ConstrainToAxis(double dInfAng, double dAxOffAng) {
  EoGeTransformMatrix tm;
  tm.Translate(EoGeVector3d(begin, EoGePoint3d::kOrigin));

  EoGeTransformMatrix tmZRot;
  tm *= tmZRot.ZAxisRotation(-sin(EoToRadian(dAxOffAng)), cos(EoToRadian(dAxOffAng)));

  EoGePoint3d pt = end;

  pt = tm * pt;

  double dX = pt.x * pt.x;
  double dY = pt.y * pt.y;
  double dZ = pt.z * pt.z;

  double dLen = sqrt(dX + dY + dZ);

  if (dLen > DBL_EPSILON) {     // Not a zero length line
    if (dX >= EoMax(dY, dZ)) {  // Major component of line is along x-axis
      dLen = sqrt(dY + dZ);
      if (dLen > DBL_EPSILON)                                // Not already on the x-axis
        if (dLen / fabs(pt.x) < tan(EoToRadian(dInfAng))) {  // Within cone of influence .. snap to x-axis
          pt.y = 0.;
          pt.z = 0.;
        }
    } else if (dY >= dZ) {  // Major component of line is along y-axis
      dLen = sqrt(dX + dZ);
      if (dLen > DBL_EPSILON)                                // Not already on the y-axis
        if (dLen / fabs(pt.y) < tan(EoToRadian(dInfAng))) {  // Within cone of influence .. snap to y-axis
          pt.x = 0.;
          pt.z = 0.;
        }
    } else {
      dLen = sqrt(dX + dY);
      if (dLen > DBL_EPSILON)                                // Not already on the z-axis
        if (dLen / fabs(pt.z) < tan(EoToRadian(dInfAng))) {  // Within cone of influence .. snap to z-axis
          pt.x = 0.;
          pt.y = 0.;
        }
    }
  }
  tm.Inverse();
  pt = tm * pt;
  return (pt);
}
EoUInt16 EoGeLine::CutAtPt(EoGePoint3d& pt, EoGeLine& ln) {
  EoUInt16 wRet = 0;

  ln = *this;

  if (pt != begin && pt != end) {
    ln.end = pt;
    begin = pt;

    wRet++;
  }
  return (wRet);
}
int EoGeLine::DirRelOfPt(EoGePoint3d pt) {
  double dDet = begin.x * (end.y - pt.y) - end.x * (begin.y - pt.y) + pt.x * (begin.y - end.y);

  if (dDet > DBL_EPSILON)
    return (1);
  else if (dDet < -DBL_EPSILON)
    return (-1);
  else
    return 0;
}
void EoGeLine::Display(AeSysView* view, CDC* deviceContext) {
  EoInt16 LineType = pstate.LineType();

  if (EoDbPrimitive::IsSupportedTyp(LineType)) {
    EoGePoint4d pt[] = {EoGePoint4d(begin), EoGePoint4d(end)};

    view->ModelViewTransformPoints(2, pt);

    if (EoGePoint4d::ClipLine(pt[0], pt[1])) {
      CPoint pnt[2];
      view->DoProjection(pnt, 2, pt);
      deviceContext->Polyline(pnt, 2);
    }
  } else {
    polyline::BeginLineStrip();
    polyline::SetVertex(begin);
    polyline::SetVertex(end);
    polyline::__End(view, deviceContext, LineType);
  }
}
void EoGeLine::Extents(EoGePoint3d& minExtent, EoGePoint3d& maxExtent) {
  minExtent(EoMin(begin.x, end.x), EoMin(begin.y, end.y), EoMin(begin.z, end.z));
  maxExtent(EoMax(begin.x, end.x), EoMax(begin.y, end.y), EoMax(begin.z, end.z));
}
bool EoGeLine::Identical(const EoGeLine& line, double tolerance) {
  return (begin.IsEqualTo(line.begin, tolerance) && end.IsEqualTo(line.end, tolerance)) ||
         (end.IsEqualTo(line.begin, tolerance) && begin.IsEqualTo(line.end, tolerance));
}
double EoGeLine::Length() {
  EoGeVector3d v(begin, end);

  return (v.Length());
}
bool EoGeLine::GetParallels(double distanceBetweenLines, double eccentricity, EoGeLine& leftLine, EoGeLine& rightLine) {
  leftLine = *this;
  rightLine = *this;

  double LengthOfLines = Length();

  if (LengthOfLines > FLT_EPSILON) {
    double X = (end.y - begin.y) * distanceBetweenLines / LengthOfLines;
    double Y = (end.x - begin.x) * distanceBetweenLines / LengthOfLines;

    leftLine += EoGeVector3d(-X * eccentricity, Y * eccentricity, 0.0);
    rightLine += EoGeVector3d(X * (1. - eccentricity), -Y * (1. - eccentricity), 0.0);

    return true;
  }
  return false;
}
bool EoGeLine::IsContainedXY(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) {
  EoGePoint3d pt[2];
  pt[0] = begin;
  pt[1] = end;

  double dX = end.x - begin.x;
  double dY = end.y - begin.y;
  int i = 1;

  int iOut[2];
  iOut[0] = pt[0].RelationshipToRectangle(lowerLeftPoint, upperRightPoint);

  for (;;) {
    iOut[i] = pt[i].RelationshipToRectangle(lowerLeftPoint, upperRightPoint);

    if (iOut[0] == 0 && iOut[1] == 0) return true;
    if ((iOut[0] & iOut[1]) != 0) return false;
    i = (iOut[0] == 0) ? 1 : 0;

    if ((iOut[i] & 1) == 1) {  // Above window
      pt[i].x = pt[i].x + dX * (upperRightPoint.y - pt[i].y) / dY;
      pt[i].y = upperRightPoint.y;
    } else if ((iOut[i] & 2) == 2) {  // Below window
      pt[i].x = pt[i].x + dX * (lowerLeftPoint.y - pt[i].y) / dY;
      pt[i].y = lowerLeftPoint.y;
    } else if ((iOut[i] & 4) == 4) {
      pt[i].y = pt[i].y + dY * (upperRightPoint.x - pt[i].x) / dX;
      pt[i].x = upperRightPoint.x;
    } else {
      pt[i].y = pt[i].y + dY * (lowerLeftPoint.x - pt[i].x) / dX;
      pt[i].x = lowerLeftPoint.x;
    }
  }
}
bool EoGeLine::IsSelectedByPointXY(EoGePoint3d pt, const double apert, EoGePoint3d& ptProj, double* rel) {
  if (pt.x < EoMin(begin.x, end.x) - apert) return false;
  if (pt.x > EoMax(begin.x, end.x) + apert) return false;
  if (pt.y < EoMin(begin.y, end.y) - apert) return false;
  if (pt.y > EoMax(begin.y, end.y) + apert) return false;

  double dPBegX = begin.x - pt.x;
  double dPBegY = begin.y - pt.y;

  double dBegEndX = end.x - begin.x;
  double dBegEndY = end.y - begin.y;

  double dDivr = dBegEndX * dBegEndX + dBegEndY * dBegEndY;
  double DistanceSquared;

  if (dDivr <= DBL_EPSILON) {
    *rel = 0.;
    DistanceSquared = dPBegX * dPBegX + dPBegY * dPBegY;
  } else {
    *rel = -(dPBegX * dBegEndX + dPBegY * dBegEndY) / dDivr;
    *rel = EoMax(0.0, EoMin(1., *rel));
    double dx = dPBegX + *rel * dBegEndX;
    double dy = dPBegY + *rel * dBegEndY;
    DistanceSquared = dx * dx + dy * dy;
  }
  if (DistanceSquared > apert * apert) return false;

  ptProj.x = begin.x + (*rel * dBegEndX);
  ptProj.y = begin.y + (*rel * dBegEndY);

  return true;
}
bool EoGeLine::ParallelTo(const EoGeLine& line) {
  EoGeVector3d Line1(begin, end);
  EoGeVector3d Line2(line.begin, line.end);

  double Determinant = Line1.x * Line2.y - Line2.x * Line1.y;

  return (fabs(Determinant) > DBL_EPSILON) ? false : true;
}
EoGePoint3d EoGeLine::ProjPt(EoGePoint3d pt) {
  EoGeVector3d vBegEnd(begin, end);

  double dSum = vBegEnd.SquaredLength();

  if (dSum > DBL_EPSILON) {
    EoGeVector3d vBegPt(begin, pt);

    double dScale = EoGeDotProduct(vBegPt, vBegEnd) / dSum;

    vBegEnd *= dScale;
  }
  return (begin + vBegEnd);
}
EoGePoint3d EoGeLine::ProjectBeginPointToEndPoint(const double t) { return begin + (begin - end) * t; }
int EoGeLine::ProjPtFrom_xy(double parallelDistance, double perpendicularDistance, EoGePoint3d* projectedPoint) {
  double dX = end.x - begin.x;
  double dY = end.y - begin.y;

  double dLen = sqrt(dX * dX + dY * dY);

  if (dLen <= DBL_EPSILON) return (FALSE);

  double dRatio;
  *projectedPoint = begin;
  if (fabs(parallelDistance) > DBL_EPSILON) {
    dRatio = parallelDistance / dLen;
    dLen = parallelDistance;
    dX = dRatio * dX;
    dY = dRatio * dY;
    (*projectedPoint).x = begin.x + dX;
    (*projectedPoint).y = begin.y + dY;
  }
  if (fabs(perpendicularDistance) > DBL_EPSILON) {
    dRatio = perpendicularDistance / dLen;
    (*projectedPoint).x -= dRatio * dY;
    (*projectedPoint).y += dRatio * dX;
  }
  return (TRUE);
}
EoGePoint3d EoGeLine::ProjToBegPt(double dDis) {
  EoGeVector3d vEndBeg(end, begin);

  double dLen = vEndBeg.Length();

  if (dLen > DBL_EPSILON) vEndBeg *= dDis / dLen;

  return (end + vEndBeg);
}
// Effect: Projects begin point toward or beyond the end point of line.
EoGePoint3d EoGeLine::ProjToEndPt(double dDis) {
  EoGeVector3d vBegEnd(begin, end);

  double dLen = vBegEnd.Length();

  if (dLen > DBL_EPSILON) vBegEnd *= dDis / dLen;

  return (begin + vBegEnd);
}
void EoGeLine::Read(CFile& file) {
  begin.Read(file);
  end.Read(file);
}
bool EoGeLine::RelOfPtToEndPts(EoGePoint3d pt, double& dRel) {
  EoGeVector3d Vector(begin, end);

  if (fabs(Vector.x) > DBL_EPSILON) {
    dRel = (pt.x - begin.x) / Vector.x;
    return true;
  }

  if (fabs(Vector.y) > DBL_EPSILON) {
    dRel = (pt.y - begin.y) / Vector.y;
    return true;
  }

  if (fabs(Vector.z) > DBL_EPSILON) {
    dRel = (pt.z - begin.z) / Vector.z;
    return true;
  }

  return false;
}
void EoGeLine::Write(CFile& file) {
  begin.Write(file);
  end.Write(file);
}
