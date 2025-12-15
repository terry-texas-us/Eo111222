#pragma once

#include "EoGeTransformMatrix.h"

class AeSysView;
class EoGeLine {
 public:
  EoGePoint3d begin;
  EoGePoint3d end;

 public:  // Constructors and destructor
  EoGeLine() {}
  EoGeLine(const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint) : begin(beginPoint), end(endPoint) {}
  EoGeLine(const EoGeLine& ln) {
    begin = ln.begin;
    end = ln.end;
  }

  ~EoGeLine() {};

 public:  // Operators
  bool operator==(const EoGeLine& line) { return (Identical(line, FLT_EPSILON)); }
  bool operator!=(const EoGeLine& line) { return (!Identical(line, FLT_EPSILON)); }
  EoGeLine& operator=(const EoGeLine& ln) {
    begin = ln.begin;
    end = ln.end;
    return (*this);
  }
  void operator+=(EoGeVector3d v) {
    begin += v;
    end += v;
  }
  void operator-=(EoGeVector3d v) {
    begin -= v;
    end -= v;
  }
  EoGePoint3d& operator[](int i) { return (i == 0 ? begin : end); }
  const EoGePoint3d& operator[](int i) const { return (i == 0 ? begin : end); }
  void operator()(const EoGePoint3d& pt0, const EoGePoint3d& pt1) {
    begin = pt0;
    end = pt1;
  }
  EoGeLine operator+(EoGeVector3d v);
  EoGeLine operator-(EoGeVector3d v);

 public:  // Methods
  /// <summary> Determines the angle of a line defined by 2 points. </summary>
  /// <remarks> /// If null length or parallel to z-axis, angle is 0. </remarks>
  /// <returns> The angle (in radians) from the X axis (0 to TWOPI) to a point (x,y). </returns>
  double AngleFromXAxisXY();
  /// <summary>Constrains a line to nearest axis pivoting on first endpoint.</summary>
  // Notes:	Offset angle only support about z-axis
  // Returns: Point after snap
  EoGePoint3d ConstrainToAxis(double influenceAngle, double offsetAngle);
  /// <summary>Cuts a line a point.</summary>
  EoUInt16 CutAtPt(EoGePoint3d&, EoGeLine&);
  /// <summary>Determines which side of a directed line a point is on.</summary>
  /// <remarks>
  ///Relation is found using determinant (3rd order).
  ///   d  = begx * (endy - y) - endx * (begy - y) + x * (begy - endy)
  /// </remarks>
  /// <returns>
  /// 1 point is to left of line
  /// 0 point is on line
  /// - 1 point is to right of line
  /// </returns>
  int DirRelOfPt(EoGePoint3d);
  void Display(AeSysView* view, CDC* deviceContext);
  /// <summary>Determines the extents of a line.</summary>
  void Extents(EoGePoint3d& minExtent, EoGePoint3d& maxExtent);
  /// <summary>Generates coordinate sets for parallel lines.</summary>
  /// <remarks>
  /// Eccentricity is a function of the distance between the lines.
  /// The first of the two parallel lines lies to the left of line, and the second to the right.
  /// </remarks>
  /// <param name="eccentricity">
  /// In general; left is (eccentricity * distanceBetweenLines) to the left of this line.
  ///			 right is distanceBetweenLines to the right of the left line
  /// Left Justifification (0.0) left line on this line and right line is distanceBetweenLines to right of this line
  /// Center Justification (0.5) left and right lines the same distance from this line
  /// Right Justifification (1.) right line on this line and left line is distanceBetweenLines to left of this line
  /// </param>
  bool GetParallels(double dDis, double eccentricity, EoGeLine& leftLine, EoGeLine& rightLine);
  bool Identical(const EoGeLine& line, double tolerance);
  /// <summary>Determines if line segment in wholly or partially contained within window passed.</summary>
  // Notes:  Assumes window passed with min/max corners correct.
  // Returns: TRUE line is wholly or partially within window
  //			FALSE otherwise
  bool IsContainedXY(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint);
  /// <summary>
  ///Evaluates the proximity of a point to a line segment.
  /// </summary>
  /// <remarks>
  ///The value of parameter t is found from:
  ///		   Rel = -[(Begx - Px)(Endx - Begx) + (Begy - Py)(Endy - Begy)]
  ///				 ------------------------------------------------------
  ///						(Endx - Begx)^2 + (Endy - Begy)^2
  /// </remarks>
  /// <returns>
  /// true if point is within acceptance aperture of line segment
  ///	false otherwise
  /// </returns>
  bool IsSelectedByPointXY(EoGePoint3d pt, const double apert, EoGePoint3d& ptProj, double* rel);
  double Length();
  EoGePoint3d Midpoint() { return ProjectBeginPointToEndPoint(0.5); }
  /// <summary>Determines if lines are parallel.</summary>
  bool ParallelTo(const EoGeLine& line);
  /// <summary>Projects a point onto line.</summary>
  EoGePoint3d ProjPt(EoGePoint3d pt);
  /// <summary>Determines the coordinates of point projected along a line.</summary>
  /// <remarks>
  ///t = 0 point is the begin point
  ///t = 1 point is the end point
  ///The range of values for t is not clamped to this interval
  /// </remarks>
  /// <returns>
  ///The begin point of the line projected toward (if positive t) the end point of the line.
  /// </returns>
  EoGePoint3d ProjectBeginPointToEndPoint(const double t);
  /// <summary>
  ///Projects a point from begin point toward end point the parallel projection distance.
  ///The resulting point is then projected perpendicular to the line defined by the two points the
  ///perpendicular projection distance.
  /// </summary>
  /// <remarks>
  ///	A positive perpendicular projection distance will result in a point to
  /// the left of the direction vector defined by the two points.
  /// Projected point is undefined if point one and point two coincide.
  /// </remarks>
  /// <returns>TRUE  successful completion and FALSE failure (p1 and p2 coincide)</returns>
  int ProjPtFrom_xy(double parallelDistance, double perpendicularDistance, EoGePoint3d* projectedPoint);
  /// <summary>Projects end point toward or beyond the begin point of line.</summary>
  EoGePoint3d ProjToBegPt(double);
  EoGePoint3d ProjToEndPt(double);
  void Read(CFile&);
  /// <summary>
  ///Determines the relationship of a point on a line to the endpoints defining the line.
  /// </summary>
  //			parametric relationship of point to line endpoints
  //			< 0 point to left of directed segment
  //			= 0 point same as first endpoint of line
  //			> 0 < 1 point between endpoints of line
  //			= 1 point same as second endpoint of line
  //			> 1 point to right of directed segment
  // Notes:  Results unpredictable if point does not lie on the line.

  // Returns: true  successful completion
  //			false coincidental endpoints .. relationship undefined
  bool RelOfPtToEndPts(EoGePoint3d, double&);
  void Write(CFile& file);
  /// <summary>Determines the angle between two lines.</summary>
  /// <notes>
  ///	Angle is found using the inner product.
  ///						  v1 dot v2
  ///		 ang = acos ( ----------------- )
  ///					  len(v1) * len(v2)
  ///	   Angle is between 0 and 2 pi. If angle is 0 lines are in same
  ///	   direction and if angle is pi lines are in opposite direction.
  ///	   To get acute angle, all angles greater than half pi must be
  ///	   subtracted from pi.
  /// </notes>
  /// <returns>angle between lines (in radians)</returns>
  /// <param name="ln1">first line</param>
  /// <param name="ln2">second line</param>
  static double AngleBetweenLn_xy(EoGeLine ln1, EoGeLine ln2) {
    EoGeVector3d v1(ln1.begin, ln1.end);
    v1.z = 0.;
    EoGeVector3d v2(ln2.begin, ln2.end);
    v2.z = 0.;

    double dSumProd = v1.SquaredLength() * v2.SquaredLength();

    if (dSumProd > DBL_EPSILON) {
      double dVal = EoGeDotProduct(v1, v2) / sqrt(dSumProd);

      dVal = EoMax(-1., EoMin(1., dVal));

      return (acos(dVal));
    }
    return (0.0);
  }

  /// <summary>
  ///Calculates intersection point of line and a plane. Should only be used if points are
  ///known to be on opposite sides of the plane.
  /// </summary>
  /// <param name="beginPoint">begin point of line</param>
  /// <param name="endPoint">end point of line</param>
  /// <param name="pointOnPlane">any point on clip plane</param>
  /// <param name="planeNormal">clip plane normal vector</param>
  /// <returns>Intersection point. If line and plane are parallel begin point of line is returned. Not good!</returns>
  static EoGePoint4d IntersectionWithPln4(EoGePoint4d& beginPoint, EoGePoint4d& endPoint,
                                          const EoGePoint4d& pointOnPlane, EoGeVector3d& planeNormal) {
    EoGeVector3d LineVector(beginPoint, endPoint);
    double DotProduct = EoGeDotProduct(planeNormal, LineVector);

    if (fabs(DotProduct) > DBL_EPSILON) {
      EoGeVector3d vPtPt0(pointOnPlane, beginPoint);
      LineVector *= (EoGeDotProduct(planeNormal, vPtPt0)) / DotProduct;
    } else {  // Line and the plane are parallel .. force return to begin point
      LineVector *= 0.;
    }
    return (beginPoint - LineVector);
  }
  /// <summary>Determines the intersection of a line and a plane.</summary>
  /// <remarks>
  /// Line is defined using parametric representation. Plane is defined by its normal
  /// vector and any point on plane.
  /// </remarks>
  /// <param name="beginPoint">begin point of line</param>
  /// <param name="lineVector">defines the direction and length of line</param>
  /// <param name="pointOnPlane">arbitrary point on plane</param>
  /// <param name="planeNormal">vector normal to plane</param>
  /// <param name="intersection">intersection of line and plane</param>
  /// <returns>true if intersection determined and false if line parallel to plane</returns>
  static bool IntersectionWithPln(EoGePoint3d& beginPoint, EoGeVector3d lineVector, EoGePoint3d pointOnPlane,
                                  EoGeVector3d planeNormal, EoGePoint3d* intersection) {
    double dDotProd = EoGeDotProduct(planeNormal, lineVector);

    if (fabs(dDotProd) > DBL_EPSILON) {  // Line and plane are not parallel
      EoGeVector3d v(lineVector);
      EoGeVector3d vOnPln(EoGePoint3d::kOrigin, pointOnPlane);
      EoGeVector3d vEnd(EoGePoint3d::kOrigin, beginPoint);

      double dD = -EoGeDotProduct(planeNormal, vOnPln);  // Coefficient of plane on which point lies
      double dT =
          -(EoGeDotProduct(planeNormal, vEnd) + dD) / dDotProd;  // Parameter on the line where it intersects the plane

      v *= dT;
      *intersection = beginPoint + v;
      return true;
    }
    return false;  // Line and plane are parallel
  }
  /// <summary> Finds intersection of two lines in space.</summary>
  // Returns:  TRUE intersection determined
  //		 FALSE endpoints of line 1 or 2 coincide or
  //		   2 lines are parallel or 4 points are not coplanar
  static int Intersection(EoGeLine ln1, EoGeLine ln2, EoGePoint3d& ptInt) {
    EoGeVector3d v1(ln1.begin, ln1.end);
    if (v1.IsNearNull()) {  // Endpoints of first line coincide
      return (FALSE);
}

    EoGeVector3d v2(ln2.begin, ln2.end);
    if (v2.IsNearNull()) {  // Endpoints of second line coincide
      return (FALSE);
}

    EoGeVector3d vPlnNorm = EoGeCrossProduct(v1, v2);
    vPlnNorm.Normalize();
    if (vPlnNorm.IsNearNull()) {  // Two lines are parallel
      return (FALSE);
}

    EoGeVector3d v3(ln1.begin, ln2.begin);
    if (fabs(EoGeDotProduct(vPlnNorm, v3)) > DBL_EPSILON) {  // Four points are not coplanar
      return (FALSE);
}

    EoGeTransformMatrix tm(ln1.begin, vPlnNorm);

    EoGePoint3d rL1P1;
    EoGePoint3d rL1P2(ln1.end);
    rL1P2 = tm * rL1P2;
    EoGePoint3d rL2P1(ln2.begin);
    rL2P1 = tm * rL2P1;
    EoGePoint3d rL2P2(ln2.end);
    rL2P2 = tm * rL2P2;

    EoGeLine Line1 = EoGeLine(rL1P1, rL1P2);
    if (EoGeLine::Intersection_xy(Line1, EoGeLine(rL2P1, rL2P2), ptInt)) {
      ptInt.z = 0.;
      tm.Inverse();
      ptInt = tm * ptInt;

      return (TRUE);
    }
    return (FALSE);
  }
  /// <summary>Determines intersection of two lines.</summary>
  // Returns: true successful completion
  //			false ohterwise (parallel lines)
  static bool Intersection_xy(EoGeLine ln1, EoGeLine ln2, EoGePoint3d& ptInt) {
    EoGeVector3d vBeg1End1(ln1.begin, ln1.end);
    EoGeVector3d vBeg2End2(ln2.begin, ln2.end);

    double dDet = vBeg1End1.x * vBeg2End2.y - vBeg2End2.x * vBeg1End1.y;

    if (fabs(dDet) > DBL_EPSILON) {
      EoGeVector3d vBeg1Beg2(ln1.begin, ln2.begin);

      double dT = (vBeg1Beg2.y * vBeg2End2.x - vBeg2End2.y * vBeg1Beg2.x) / dDet;

      vBeg1End1 *= dT;
      ptInt = ln1.begin - vBeg1End1;
      return true;
    }
    return false;
  }
};
