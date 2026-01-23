#pragma once

#include "EoGePoint3d.h"
#include "EoGePoint4d.h"

class AeSysView;
class CDC;
class CFile;
class EoGeVector3d;

/** @brief Represents a line segment in 3D space defined by two endpoints. */
class EoGeLine {
 public:
  EoGePoint3d begin;
  EoGePoint3d end;

  EoGeLine() {}
  EoGeLine(const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint);
  EoGeLine(const EoGeLine& line);

  ~EoGeLine() {};

  bool operator==(const EoGeLine& other) const;
  bool operator!=(const EoGeLine& other) const;
  EoGeLine& operator=(const EoGeLine& line);
  void operator+=(const EoGeVector3d& offset);
  void operator-=(const EoGeVector3d& offset);
  EoGePoint3d& operator[](int index);
  const EoGePoint3d& operator[](int index) const;
  void operator()(const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint);
  EoGeLine operator+(const EoGeVector3d& offset) const;
  EoGeLine operator-(const EoGeVector3d& offset) const;

  /** @brief Determines the angle of the line segment from the X-axis in the XY plane.
  * @return The angle (in radians) from the X axis (0 to Eo::TwoPi) to the line segment.
  * note If null length or parallel to z-axis, angle is 0.
  */
  double AngleFromXAxisXY() const;

  /** @brief Constrains a line to the nearest axis pivoting on the first endpoint.
  * @param influenceAngle The angle to influence the constraint.
  * @param offsetAngle The angle to offset the constraint.
  * @return The constrained point.
  * @note Offset angle only supported about z-axis.
  */
  EoGePoint3d ConstrainToAxis(double influenceAngle, double offsetAngle) const;

  /** @brief Cuts the line at a specified point, modifying the line segment.
  * @param point The point at which to cut the line.
  * @param line Output parameter that receives the modified line segment.
  * @return An unsigned integer indicating the result of the cut operation:
  * @code
    0 No cut made (point coincides with an endpoint)
    1 Cut made (line modified)
  @endcode
  */
  EoUInt16 CutAtPt(EoGePoint3d& point, EoGeLine& line);

  /** @brief Determines the relative position (which side) of a point to the directed line segment.
  * @param point The point to evaluate.
  * @return An integer indicating the position of the point relative to the directed line segment:
  * @code
    1 point is to left of line
    0 point is on line
   -1 point is to right of line
  @endcode
  @note The relationship is found using the determinant (3rd order):
  @code
    d  = begx * (endy - y) - endx * (begy - y) + x * (begy - endy)
  @endcode
  */
  int DirRelOfPt(EoGePoint3d point) const;

  void Display(AeSysView* view, CDC* context) const;

  /** @brief Determines the extents of a line.
  * @param minExtent The minimum extent point of the line.
  * @param maxExtent The maximum extent point of the line.
  */
  void Extents(EoGePoint3d& minExtent, EoGePoint3d& maxExtent) const;

  /** @brief Generates coordinate sets for parallel lines.
  * @param distance The distance between the parallel lines.
  * @param eccentricity The eccentricity factor for the parallel lines. In general; 
  * @code 
    left is (eccentricity * distanceBetweenLines) to the left of this line. 
    right is distanceBetweenLines to the right of the left line. 
    Left Justifification (0.0) left line on this line and right line is distanceBetweenLines to right of this line. 
    Center Justification (0.5) left and right lines the same distance from this line. 
    Right Justifification (1.0) right line on this line and left line is distanceBetweenLines to left of this line.
    @endcode
  * @param leftLine The generated left parallel line.
  * @param rightLine The generated right parallel line.
  * @return true if the parallel lines were successfully generated; otherwise, false.
  * @note The first of the two parallel lines lies to the left of line, and the second to the right.
  */
  bool GetParallels(double distance, double eccentricity, EoGeLine& leftLine, EoGeLine& rightLine) const;

  bool Identical(const EoGeLine& line, double tolerance) const;

  /** @brief Determines if the line segment is wholly or partially contained within a specified rectangular window in the XY plane.
  * @param lowerLeftPoint The lower-left corner of the rectangular window.
  * @param upperRightPoint The upper-right corner of the rectangular window.
  * @return true if the line segment is wholly or partially within the window; otherwise, false.
  */
  bool IsContainedXY(const EoGePoint3d& lowerLeftPoint, const EoGePoint3d& upperRightPoint) const;

  /** @brief Determines if a point is selected by the line segment within a specified aperture in the XY plane.
  * @param point The point to evaluate.
  * @param aperture The selection aperture distance.
  * @param projectedPoint Output parameter that receives the projected point on the line segment.
  * @param relationship Output parameter that receives the parametric relationship of the point to the line segment:
  * @code
      < 0 point to left of directed segment
      = 0 point same as first endpoint of line
      > 0 < 1 point between endpoints of line
      = 1 point same as second endpoint of line
      > 1 point to right of directed segment
  @endcode
  * @return true if the point is within the aperture of the line segment; otherwise, false.
  * @note The relationship parameter will indicate the position of the point relative to the line segment.
  * @code
      The value of parameter t is found from:
  rel = -[(Begx - Px)(Endx - Begx) + (Begy - Py)(Endy - Begy)] / [(Endx - Begx)² + (Endy - Begy)²]
    @endcode
  */
  bool IsSelectedByPointXY(EoGePoint3d point, const double aperture, EoGePoint3d& projectedPoint,
                           double* relationship) const;

  double Length() const;

  EoGePoint3d Midpoint() const;

  /** @brief Determines if this line is parallel to another line.
  * @param line The line to compare against.
  * @return true if the lines are parallel; otherwise, false.
  */
  bool ParallelTo(const EoGeLine& line) const;

  /** @brief Projects a point onto the line defined by the begin and end points.
  * @param point The point to project onto the line.
  * @return The projected point on the line.
  */
  EoGePoint3d ProjPt(const EoGePoint3d& point) const;

  /** @brief Projects a point from the begin point toward (for positive) the end point based on a parametric value t.
  * @param t The parametric value indicating the position along the line.
  * @return The projected point along the line.
  * @note A t value of 0 corresponds to the begin point, and a t value of 1 corresponds to the end point. Values of t outside this range will project points beyond the endpoints.
  */
  EoGePoint3d ProjectBeginPointToEndPoint(const double t) const;

  /** @brief Projects a point from the begin point toward the end point by a specified parallel distance and then perpendicular to the line by a specified perpendicular distance. 
  * @param parallelDistance The distance to project the point parallel to the line.
  * @param perpendicularDistance The distance to project the point perpendicular to the line.
  * @param projectedPoint Output parameter that receives the projected point.
  * @return TRUE on success, or FALSE on failure (begin point and end point coincide).
  * @note A positive perpendicular projection distance results in a point to the left of the direction vector defined by the two points. Projected point is undefined if the begin and end points coincide.
  */
  int ProjPtFrom_xy(double parallelDistance, double perpendicularDistance, EoGePoint3d* projectedPoint) const;

  /** @brief Projects a point along the line from the end point toward the beginning point at a specified distance.
  * @param distance The distance from the end point to project along the line.
  * @return A 3D point located at the specified distance from the end point along the direction toward the beginning point.
  */
  EoGePoint3d ProjToBegPt(double distance) const;

  /** @brief Projects a point along the line from the beginning point toward the end point at a specified distance.
  * @param distance The distance from the beginning point to project along the line.
  * @return A 3D point located at the specified distance from the beginning point along the direction toward the end point.
  */
  EoGePoint3d ProjToEndPt(double distance) const;

  void Read(CFile&);

  /**
   @brief Determines the parametric relationship of a point relative to the endpoints of a geometric entity.
   @param point The 3D point to evaluate.
   @param pointParametricRelationship Output parameter that receives the parametric relationship value of the point relative to the endpoints.
   @code
  			< 0 point to left of directed segment
  			= 0 point same as first endpoint of line
  			> 0 < 1 point between endpoints of line
  			= 1 point same as second endpoint of line
  			> 1 point to right of directed segment
   @endcode
   @return True if the parametric relationship was successfully computed; otherwise, false.
   @note Results are unpredictable if the point does not lie on the line.
  */
  bool RelOfPtToEndPts(EoGePoint3d point, double& pointParametricRelationship) const;

  void Write(CFile& file) const;

  /** @brief Calculates the angle between two lines projected onto the XY plane.
  * @param firstLine The first line to compare.
  * @param secondLine The second line to compare.
  * @return The angle in radians between the two lines when projected onto the XY plane, or 0.0 if either line has near-zero length.
  * @note
  *	Angle is found using the inner product.
  * @code
    Angle = acos ( [v1 dot v2] /  [len(v1) * len(v2)] )
    Angle is between 0 and 2 pi. If angle is 0 lines are in same direction and if angle is pi lines are in opposite direction.
    To get acute angle, all angles greater than half pi must be subtracted from pi.
    @endcode
  */
  static double AngleBetweenLn_xy(EoGeLine firstLine, EoGeLine secondLine);

  /** @brief Computes the intersection point of a line segment with a plane in 4D space.
  * @param beginPoint The starting point of the line segment.
  * @param endPoint The ending point of the line segment.
  * @param pointOnPlane A point that lies on the plane.
  * @param planeNormal The normal vector of the plane.
  * @return The intersection point of the line with the plane. If the line is parallel to the plane, returns the begin point.
  * @note Should only be used if the endpoints of the line segment are known to be on opposite sides of the plane.
  */
  static EoGePoint4d IntersectionWithPln4(EoGePoint4d& beginPoint, EoGePoint4d& endPoint,
                                          const EoGePoint4d& pointOnPlane, EoGeVector3d& planeNormal);

  /** @brief Computes the intersection point between a line and a plane.
  * @param beginPoint The starting point of the line.
  * @param lineVector The direction vector of the line.
  * @param pointOnPlane A point that lies on the plane.
  * @param planeNormal The normal vector of the plane.
  * @param intersection Output parameter that receives the intersection point if it exists.
  * @return Returns true if the line intersects the plane (i.e., they are not parallel), false otherwise.
  * @note Line is defined using parametric representation. Plane is defined by its normal vector and any point on plane.
  */
  static bool IntersectionWithPln(EoGePoint3d& beginPoint, EoGeVector3d lineVector, EoGePoint3d pointOnPlane,
                                  EoGeVector3d planeNormal, EoGePoint3d* intersection);

  /** @brief Computes the intersection point of two 3D line segments, if it exists.
  * @param ln1 The first line segment.
  * @param ln2 The second line segment.
  * @param intersection Output parameter that receives the intersection point if the line segments intersect.
  * @return TRUE if the line segments intersect, FALSE otherwise. Returns FALSE if either line segment is degenerate (endpoints coincide), if the lines are parallel, or if the line segments are skew (not coplanar).
  */
  static int Intersection(EoGeLine ln1, EoGeLine ln2, EoGePoint3d& intersection);

  /** @brief Computes the intersection point of two lines in the XY plane.
  * @param ln1 The first line.
  * @param ln2 The second line.
  * @param intersection Output parameter that receives the intersection point if one exists.
  * @return True if the lines intersect (are not parallel), false otherwise.
  */
  static bool Intersection_xy(EoGeLine ln1, EoGeLine ln2, EoGePoint3d& intersection);
};
