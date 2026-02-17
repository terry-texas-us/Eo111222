#include "Stdafx.h"

#include <algorithm>
#include <cmath>

#include "AeSysView.h"
#include "Eo.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderState.h"

inline EoGeLine::EoGeLine(const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint)
    : begin(beginPoint), end(endPoint) {}

EoGeLine::EoGeLine(const EoGeLine& other) {
  begin = other.begin;
  end = other.end;
}

bool EoGeLine::operator==(const EoGeLine& line) const { return (Identical(line, Eo::geometricTolerance)); }

inline bool EoGeLine::operator!=(const EoGeLine& line) const { return (!Identical(line, Eo::geometricTolerance)); }

inline EoGeLine& EoGeLine::operator=(const EoGeLine& other) {
  begin = other.begin;
  end = other.end;
  return (*this);
}

inline void EoGeLine::operator+=(const EoGeVector3d& v) {
  begin += v;
  end += v;
}

inline void EoGeLine::operator-=(const EoGeVector3d& v) {
  begin -= v;
  end -= v;
}

EoGePoint3d& EoGeLine::operator[](int i) { return (i == 0 ? begin : end); }

const EoGePoint3d& EoGeLine::operator[](int i) const { return (i == 0 ? begin : end); }

void EoGeLine::operator()(const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint) {
  begin = beginPoint;
  end = endPoint;
}

EoGeLine EoGeLine::operator-(const EoGeVector3d& offset) const { return (EoGeLine(begin - offset, end - offset)); }

EoGeLine EoGeLine::operator+(const EoGeVector3d& offset) const { return (EoGeLine(begin + offset, end + offset)); }

double EoGeLine::AngleFromXAxisXY() const {
  EoGeVector3d thisAsVector((*this).begin, (*this).end);

  double angle{};

  if (fabs(thisAsVector.x) > Eo::geometricTolerance || fabs(thisAsVector.y) > Eo::geometricTolerance) {
    angle = atan2(thisAsVector.y, thisAsVector.x);

    if (angle < 0.0) { angle += Eo::TwoPi; }
  }
  return angle;
}

EoGePoint3d EoGeLine::ConstrainToAxis(double influenceAngle, double axisOffsetAngle) const {
  EoGeTransformMatrix transformMatrix{};
  transformMatrix.Translate(EoGeVector3d(begin, EoGePoint3d::kOrigin));
  transformMatrix *= EoGeTransformMatrix::ZAxisRotation(
      -sin(Eo::DegreeToRadian(axisOffsetAngle)), cos(Eo::DegreeToRadian(axisOffsetAngle)));

  EoGePoint3d pt = end;

  pt = transformMatrix * pt;

  double dX = pt.x * pt.x;
  double dY = pt.y * pt.y;
  double dZ = pt.z * pt.z;

  double dLen = sqrt(dX + dY + dZ);

  if (dLen > Eo::geometricTolerance) {  // Not a zero length line
    if (dX >= std::max(dY, dZ)) {       // Major component of line is along x-axis
      dLen = sqrt(dY + dZ);
      if (dLen > Eo::geometricTolerance)  // Not already on the x-axis
        if (dLen / fabs(pt.x) <
            tan(Eo::DegreeToRadian(influenceAngle))) {  // Within cone of influence .. snap to x-axis
          pt.y = 0.;
          pt.z = 0.;
        }
    } else if (dY >= dZ) {  // Major component of line is along y-axis
      dLen = sqrt(dX + dZ);
      if (dLen > Eo::geometricTolerance)  // Not already on the y-axis
        if (dLen / fabs(pt.y) <
            tan(Eo::DegreeToRadian(influenceAngle))) {  // Within cone of influence .. snap to y-axis
          pt.x = 0.;
          pt.z = 0.;
        }
    } else {
      dLen = sqrt(dX + dY);
      if (dLen > Eo::geometricTolerance)  // Not already on the z-axis
        if (dLen / fabs(pt.z) <
            tan(Eo::DegreeToRadian(influenceAngle))) {  // Within cone of influence .. snap to z-axis
          pt.x = 0.;
          pt.y = 0.;
        }
    }
  }
  transformMatrix.Inverse();
  pt = transformMatrix * pt;
  return pt;
}

std::uint16_t EoGeLine::CutAtPoint(const EoGePoint3d& point, EoGeLine& line) {
  std::uint16_t result{};

  line = *this;

  if (point != begin && point != end) {
    line.end = point;
    begin = point;

    result++;
  }
  return result;
}

int EoGeLine::DirRelOfPt(EoGePoint3d pt) const {
  double dDet = begin.x * (end.y - pt.y) - end.x * (begin.y - pt.y) + pt.x * (begin.y - end.y);

  if (dDet > Eo::geometricTolerance)
    return (1);
  else if (dDet < -Eo::geometricTolerance)
    return (-1);
  else
    return 0;
}

void EoGeLine::Display(AeSysView* view, CDC* deviceContext) const {
  std::int16_t lineTypeIndex = renderState.LineTypeIndex();

  if (EoDbPrimitive::IsSupportedTyp(lineTypeIndex)) {
    EoGePoint4d pt[] = {EoGePoint4d(begin), EoGePoint4d(end)};

    view->ModelViewTransformPoints(2, pt);

    if (EoGePoint4d::ClipLine(pt[0], pt[1])) {
      CPoint pnt[2]{};
      view->DoProjection(pnt, 2, pt);
      deviceContext->Polyline(pnt, 2);
    }
  } else {
    polyline::BeginLineStrip();
    polyline::SetVertex(begin);
    polyline::SetVertex(end);
    polyline::__End(view, deviceContext, lineTypeIndex);
  }
}

void EoGeLine::Extents(EoGePoint3d& minExtent, EoGePoint3d& maxExtent) const {
  minExtent(std::min(begin.x, end.x), std::min(begin.y, end.y), std::min(begin.z, end.z));
  maxExtent(std::max(begin.x, end.x), std::max(begin.y, end.y), std::max(begin.z, end.z));
}

bool EoGeLine::Identical(const EoGeLine& line, double tolerance) const {
  return (begin.IsEqualTo(line.begin, tolerance) && end.IsEqualTo(line.end, tolerance)) ||
         (end.IsEqualTo(line.begin, tolerance) && begin.IsEqualTo(line.end, tolerance));
}

double EoGeLine::Length() const {
  EoGeVector3d vector(begin, end);

  return (vector.Length());
}

EoGePoint3d EoGeLine::Midpoint() const { return ProjectBeginPointToEndPoint(0.5); }

bool EoGeLine::GetParallels(
    double distanceBetweenLines, double eccentricity, EoGeLine& leftLine, EoGeLine& rightLine) const {
  leftLine = *this;
  rightLine = *this;

  double lengthOfLines = Length();

  if (lengthOfLines > Eo::geometricTolerance) {
    double X = (end.y - begin.y) * distanceBetweenLines / lengthOfLines;
    double Y = (end.x - begin.x) * distanceBetweenLines / lengthOfLines;

    leftLine += EoGeVector3d(-X * eccentricity, Y * eccentricity, 0.0);
    rightLine += EoGeVector3d(X * (1.0 - eccentricity), -Y * (1.0 - eccentricity), 0.0);

    return true;
  }
  return false;
}

bool EoGeLine::IsContainedXY(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const {
  EoGePoint3d pt[2]{};
  pt[0] = begin;
  pt[1] = end;

  double dX = end.x - begin.x;
  double dY = end.y - begin.y;
  int i = 1;

  int iOut[2]{};
  iOut[0] = pt[0].RelationshipToRectangle(lowerLeftPoint, upperRightPoint);

  for (;;) {
    iOut[i] = pt[i].RelationshipToRectangle(lowerLeftPoint, upperRightPoint);

    if (iOut[0] == 0 && iOut[1] == 0) { return true; }
    if ((iOut[0] & iOut[1]) != 0) { return false; }
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

bool EoGeLine::IsSelectedByPointXY(EoGePoint3d pt, const double apert, EoGePoint3d& ptProj, double* rel) const {
  if (pt.x < std::min(begin.x, end.x) - apert) { return false; }
  if (pt.x > std::max(begin.x, end.x) + apert) { return false; }
  if (pt.y < std::min(begin.y, end.y) - apert) { return false; }
  if (pt.y > std::max(begin.y, end.y) + apert) { return false; }

  double dPBegX = begin.x - pt.x;
  double dPBegY = begin.y - pt.y;

  double dBegEndX = end.x - begin.x;
  double dBegEndY = end.y - begin.y;

  double dDivr = dBegEndX * dBegEndX + dBegEndY * dBegEndY;
  double distanceSquared{};

  if (dDivr < Eo::geometricTolerance) {
    *rel = 0.;
    distanceSquared = dPBegX * dPBegX + dPBegY * dPBegY;
  } else {
    *rel = -(dPBegX * dBegEndX + dPBegY * dBegEndY) / dDivr;
    *rel = std::max(0.0, std::min(1.0, *rel));
    double dx = dPBegX + *rel * dBegEndX;
    double dy = dPBegY + *rel * dBegEndY;
    distanceSquared = dx * dx + dy * dy;
  }
  if (distanceSquared > apert * apert) { return false; }

  ptProj.x = begin.x + (*rel * dBegEndX);
  ptProj.y = begin.y + (*rel * dBegEndY);

  return true;
}

bool EoGeLine::ParallelTo(const EoGeLine& line) const {
  EoGeVector3d firstVector(begin, end);
  EoGeVector3d secondVector(line.begin, line.end);

  double determinant = firstVector.x * secondVector.y - secondVector.x * firstVector.y;

  return (fabs(determinant) > Eo::geometricTolerance) ? false : true;
}

EoGePoint3d EoGeLine::ProjectPointToLine(const EoGePoint3d& point) const {
  EoGeVector3d vBegEnd(begin, end);

  double squaredLength = vBegEnd.SquaredLength();

  if (squaredLength > Eo::geometricTolerance) {
    EoGeVector3d vBegPt(begin, point);

    double scale = DotProduct(vBegPt, vBegEnd) / squaredLength;

    vBegEnd *= scale;
  }
  return (begin + vBegEnd);
}

EoGePoint3d EoGeLine::ProjectBeginPointToEndPoint(const double t) const { return begin + (end - begin) * t; }

int EoGeLine::ProjPtFrom_xy(double parallelDistance, double perpendicularDistance, EoGePoint3d* projectedPoint) const {
  double dX = end.x - begin.x;
  double dY = end.y - begin.y;

  double dLen = sqrt(dX * dX + dY * dY);

  if (dLen < Eo::geometricTolerance) return FALSE;

  double dRatio;
  *projectedPoint = begin;
  if (fabs(parallelDistance) > Eo::geometricTolerance) {
    dRatio = parallelDistance / dLen;
    dLen = parallelDistance;
    dX = dRatio * dX;
    dY = dRatio * dY;
    (*projectedPoint).x = begin.x + dX;
    (*projectedPoint).y = begin.y + dY;
  }
  if (fabs(perpendicularDistance) > Eo::geometricTolerance) {
    dRatio = perpendicularDistance / dLen;
    (*projectedPoint).x -= dRatio * dY;
    (*projectedPoint).y += dRatio * dX;
  }
  return TRUE;
}

EoGePoint3d EoGeLine::ProjectToBeginPoint(double distance) const {
  EoGeVector3d endBeginVector(end, begin);

  double length = endBeginVector.Length();

  if (length > Eo::geometricTolerance) endBeginVector *= distance / length;
  return (end + endBeginVector);
}

EoGePoint3d EoGeLine::ProjectToEndPoint(double distance) const {
  EoGeVector3d beginEndVector(begin, end);

  double length = beginEndVector.Length();

  if (length > Eo::geometricTolerance) beginEndVector *= distance / length;
  return (begin + beginEndVector);
}

void EoGeLine::Read(CFile& file) {
  begin.Read(file);
  end.Read(file);
}

bool EoGeLine::ComputeParametricRelation(const EoGePoint3d& point, double& pointParametricRelationship) const {
  EoGeVector3d beginEndVector(begin, end);

  if (fabs(beginEndVector.x) > Eo::geometricTolerance) {
    pointParametricRelationship = (point.x - begin.x) / beginEndVector.x;
    return true;
  }

  if (fabs(beginEndVector.y) > Eo::geometricTolerance) {
    pointParametricRelationship = (point.y - begin.y) / beginEndVector.y;
    return true;
  }

  if (fabs(beginEndVector.z) > Eo::geometricTolerance) {
    pointParametricRelationship = (point.z - begin.z) / beginEndVector.z;
    return true;
  }
  return false;
}

void EoGeLine::Write(CFile& file) const {
  begin.Write(file);
  end.Write(file);
}

double EoGeLine::AngleBetweenLn_xy(EoGeLine firstLine, EoGeLine secondLine) {
  EoGeVector3d firstVector(firstLine.begin, firstLine.end);
  firstVector.z = 0.0;
  EoGeVector3d secondVector(secondLine.begin, secondLine.end);
  secondVector.z = 0.0;

  double dSumProd = firstVector.SquaredLength() * secondVector.SquaredLength();

  if (dSumProd > Eo::geometricTolerance) {
    double value = DotProduct(firstVector, secondVector) / sqrt(dSumProd);

    value = std::max(-1.0, std::min(1.0, value));

    return (acos(value));
  }
  return (0.0);
}

EoGePoint4d EoGeLine::IntersectionWithPlane(
    const EoGePoint4d& begin, const EoGePoint4d& end, const EoGePoint4d& point, const EoGeVector3d& normal) {
  EoGeVector3d beginEndVector(begin, end);
  double dotProduct = DotProduct(normal, beginEndVector);

  if (fabs(dotProduct) > Eo::geometricTolerance) {
    EoGeVector3d pointBeginVector(point, begin);
    beginEndVector *= (DotProduct(normal, pointBeginVector)) / dotProduct;
  } else {
    // Line and the plane are parallel .. force return to begin point
    beginEndVector *= 0.0;
  }
  return (begin - beginEndVector);
}

bool EoGeLine::IntersectionWithPln(EoGePoint3d& beginPoint, EoGeVector3d lineVector, EoGePoint3d pointOnPlane,
    EoGeVector3d planeNormal, EoGePoint3d* intersection) {
  double dDotProd = DotProduct(planeNormal, lineVector);

  if (fabs(dDotProd) > Eo::geometricTolerance) {  // Line and plane are not parallel
    EoGeVector3d v(lineVector);
    EoGeVector3d vOnPln(EoGePoint3d::kOrigin, pointOnPlane);
    EoGeVector3d vEnd(EoGePoint3d::kOrigin, beginPoint);

    /// @brief Calculate the constant term of the plane equation.
    double dD = -DotProduct(planeNormal, vOnPln);

    /// @brief Calculate the parameter t at which the line intersects the plane.
    double dT = -(DotProduct(planeNormal, vEnd) + dD) / dDotProd;

    v *= dT;
    *intersection = beginPoint + v;
    return true;
  }
  return false;  // Line and plane are parallel
}

bool EoGeLine::Intersection(
    const EoGeLine& firstLine, const EoGeLine& secondLine, EoGePoint3d& intersection) {
  EoGeVector3d firstVector(firstLine.begin, firstLine.end);
  if (firstVector.IsNearNull()) { return false; }

  EoGeVector3d secondVector(secondLine.begin, secondLine.end);
  if (secondVector.IsNearNull()) { return false; }

  auto normal = CrossProduct(firstVector, secondVector);
  if (normal.IsNearNull()) { return false; }
  normal.Normalize();

  EoGeVector3d v3(firstLine.begin, secondLine.begin);

  if (fabs(DotProduct(normal, v3)) > Eo::geometricTolerance) { return false; }

  EoGeTransformMatrix transformMatrix(firstLine.begin, normal);

  EoGePoint3d firstLineEnd(firstLine.end);
  firstLineEnd = transformMatrix * firstLineEnd;
  EoGePoint3d secondLineBegin(secondLine.begin);
  secondLineBegin = transformMatrix * secondLineBegin;
  EoGePoint3d secondLineEnd(secondLine.end);
  secondLineEnd = transformMatrix * secondLineEnd;

  if (EoGeLine::Intersection_xy(
          EoGeLine(EoGePoint3d::kOrigin, firstLineEnd), EoGeLine(secondLineBegin, secondLineEnd), intersection)) {
    intersection.z = 0.0;
    transformMatrix.Inverse();
    intersection = transformMatrix * intersection;

    return true;
  }
  return false;
}

bool EoGeLine::Intersection_xy(EoGeLine firstLine, EoGeLine secondLine, EoGePoint3d& intersection) {
  EoGeVector3d firstVector(firstLine.begin, firstLine.end);
  EoGeVector3d secondVector(secondLine.begin, secondLine.end);

  double determinant = firstVector.x * secondVector.y - secondVector.x * firstVector.y;

  if (fabs(determinant) > Eo::geometricTolerance) {
    EoGeVector3d vBeg1Beg2(firstLine.begin, secondLine.begin);

    double t = (vBeg1Beg2.y * secondVector.x - secondVector.y * vBeg1Beg2.x) / determinant;

    firstVector *= t;
    intersection = firstLine.begin - firstVector;
    return true;
  }
  return false;
}
