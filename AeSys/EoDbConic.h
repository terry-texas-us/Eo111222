#pragma once

#include <utility>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "drw_base.h"
#include "drw_interface.h"

class EoDbConic : public EoDbPrimitive {
  EoGePoint3d m_center{};
  EoGeVector3d m_majorAxis{};
  EoGeVector3d m_extrusion{EoGeVector3d::positiveUnitZ};
  double m_ratio{1.0};    // Ratio of minor axis to major axis [0.0 to 1.0]
  double m_startAngle{};  // Start parameter angle in radians
  double m_endAngle{};    // End parameter angle in radians

  // @todo sweep angle are to be removed from class data members
  double m_sweepAngle{0.0};

  /** @brief Private constructor for EoDbConic class.
   *
   * This constructor initializes an EoDbConic object with the specified parameters.
   * It is private to enforce the use of factory methods for object creation.
   * @param center The center point of the conic.
   * @param extrusion The extrusion direction defining the plane of the conic.
   * @param majorAxis The major axis vector of the conic.
   * @param ratio The ratio of the minor axis to the major axis (0.0 < ratio <= 1.0).
   * @param startAngle The starting angle of the conic in radians.
   * @param endAngle The ending angle of the conic in radians.
   */
  EoDbConic(const EoGePoint3d& center, const EoGeVector3d& extrusion, const EoGeVector3d& majorAxis, double ratio,
            double startAngle, double endAngle);

 public:
  /** @brief Creates a radial arc conic primitive.
   *
   * This static method constructs a radial arc conic primitive using the specified center point,
   * extrusion vector, radius, start angle, and end angle. The major axis is computed as an arbitrary
   * axis perpendicular to the extrusion vector.
   * @param center The center point of the radial arc.
   * @param extrusion The extrusion direction defining the plane of the radial arc.
   * @param radius The radius of the radial arc.
   * @param startAngle The starting angle of the arc in radians.
   * @param endAngle The ending angle of the arc in radians.
   * @return A pointer to the created EoDbConic primitive representing the radial arc.
   * @note This is definition used by autoCAD for arc entity.
   */
  static EoDbConic* CreateRadialArc(const EoGePoint3d& center, const EoGeVector3d& extrusion, double radius,
                                    double startAngle, double endAngle);
  /**
   * @brief Creates a radial arc defined by three points in 3D space.
   *
   * This static method constructs a radial arc (a segment of a circle) that passes through three specified points:
   * the start point, an intermediate point, and the end point. The arc is created in a counter-clockwise direction
   * when viewed from the positive Z direction of the computed normal vector.
   * @param start The starting point of the arc.
   * @param intermediate A point on the arc between the start and end points.
   * @param end The ending point of the arc.
   * @return A pointer to the created EoDbConic representing the radial arc, or nullptr if the points are collinear.
   */
  static EoDbConic* CreateRadialArcFrom3Points(EoGePoint3d start, EoGePoint3d intermediate, EoGePoint3d end);

  /** @brief Creates a circle conic primitive.
   * This static method constructs a circle conic primitive using the specified center point,
   * extrusion vector, and radius. The major axis is computed as an arbitrary axis perpendicular
   * to the extrusion vector.
   * @param center The center point of the circle.
   * @param extrusion The extrusion direction defining the plane of the circle.
   * @param radius The radius of the circle.
   * @return A pointer to the created EoDbConic primitive representing the circle.
   * @note This is definition used by autoCAD for circle entity.
   */
  static EoDbConic* CreateCircle(const EoGePoint3d& center, const EoGeVector3d& extrusion, double radius);

  /** @brief Creates a circle in the active view's plane.
   *
   * This static method creates a circle centered at the specified point with the given radius,
   * oriented according to the active view's camera direction.
   * @param center The center point of the circle.
   * @param radius The radius of the circle.
   * @return A pointer to the newly created EoDbConic representing the circle.
   */
  static EoDbConic* CreateCircleInView(const EoGePoint3d& center, double radius);

  /**
   * @brief Creates a conic primitive defined by center, extrusion, major axis, ratio, start angle, and end angle.
   *
   * This static method constructs a conic primitive (circle, ellipse, radial arc, or elliptical arc) using the specified parameters.
   * @param center The center point of the conic.
   * @param extrusion The extrusion direction defining the plane of the conic.
   * @param majorAxis The major axis vector of the conic.
   * @param ratio The ratio of the minor axis to the major axis (0.0 < ratio <= 1.0).
   * @param startAngle The starting angle of the conic in radians.
   * @param endAngle The ending angle of the conic in radians.
   * @return A pointer to the created EoDbConic primitive representing the conic.
   * @note This is the general definition used to expose the private constructor.
   * @note This is definition used by autoCAD for ellipse entity.
   */
  static EoDbConic* CreateConic(EoGePoint3d& center, EoGeVector3d& extrusion, EoGeVector3d& majorAxis, double ratio,
                                double startAngle, double endAngle);

  /**
   * @brief Creates a conic primitive from given parameters which are used to define deprecated ellipse primitive.
   * 
   * This function constructs an conic primitive using the specified center point, major axis vector,
   * minor axis vector, and sweep angle. The extrusion vector is computed as the cross product of the major
   * and minor axes, and the ratio of the minor axis length to the major axis length is calculated.
   * @param center The center point of the ellipse.
   * @param majorAxis The major axis vector of the ellipse.
   * @param minorAxis The minor axis vector of the ellipse.
   * @param sweepAngle The sweep angle of the ellipse in radians.
   * @return A pointer to the created EoDbConic primitive.
   */
  static EoDbConic* CreateConicFromEllipsePrimitive(EoGePoint3d& center, EoGeVector3d& majorAxis,
                                                    EoGeVector3d& minorAxis, double sweepAngle);

  /** @brief Creates an ellipse conic primitive.
   *
   * This static method constructs an ellipse conic primitive using the specified center point,
   * extrusion vector, major axis, and ratio of minor axis to major axis.
   * @param center The center point of the ellipse.
   * @param extrusion The extrusion direction defining the plane of the ellipse.
   * @param majorAxis The major axis vector of the ellipse.
   * @param ratio The ratio of the minor axis to the major axis (0.0 < ratio <= 1.0).
   * @return A pointer to the created EoDbConic primitive representing the ellipse.
   */
  static EoDbConic* CreateEllipse(const EoGePoint3d& center, const EoGeVector3d& extrusion,
                                  const EoGeVector3d& majorAxis, double ratio);

 public:
  EoDbConic()
      : m_center{},
        m_majorAxis{},
        m_extrusion{EoGeVector3d::positiveUnitZ},
        m_ratio{1.0},
        m_startAngle{},
        m_endAngle{},
        // @todo sweep angle are to be removed from class data members
        m_sweepAngle{} {}

  // EoDbConic(const EoGePoint3d& center, const EoGeVector3d& extrusion, double radius, double startAngle = 0.0, double endAngle = Eo::TwoPi);

  // EoDbConic(const EoGePoint3d& center, const EoGeVector3d& extrusion, const EoGeVector3d& majorAxis, double ratio);

  // EoDbConic(EoGePoint3d& center, EoGePoint3d& start);

  // EoDbConic(const EoGePoint3d& center, double radius, double startAngle, double endAngle);

  // EoDbConic(EoGePoint3d& center, EoGeVector3d& majorAxis, EoGeVector3d& minorAxis, double sweepAngle);

  // EoDbConic(EoGePoint3d& center, double radius, EoInt16 color = COLOR_BYLAYER, EoInt16 lineTypeIndex = LINETYPE_BYLAYER);

  // EoDbConic(EoGePoint3d& center, EoGeVector3d& planeNormal, double radius, EoInt16 penColor, EoInt16 lineTypeIndex);

  // EoDbConic(EoGePoint3d begin, EoGePoint3d intermediate, EoGePoint3d end);

  EoDbConic(const EoDbConic& other);

  ~EoDbConic() override {}

 public:
  const EoDbConic& operator=(const EoDbConic&);

 public:  // Methods - absolute virtuals
  void AddReportToMessageList(EoGePoint3d) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbConic*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  void FormatExtra(CString& extra) override;
  void FormatGeometry(CString& geometry) override;
  EoGePoint3d GetControlPoint() override { return (m_center); }
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(EoUInt16 type) override { return type == EoDb::kConicPrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d v) override { m_center += v; }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoUInt8* buffer) override;

 public:
  /** @brief Cuts the conic at a specified point, creating a new conic segment.
 *
 * This method cuts the conic at the given point and creates a new conic segment
 * that starts from the cut point to the original end angle. The original conic
 * is modified to end at the cut point.
 *
 * @param point The point at which to cut the conic.
 * @param group The group to which the new conic segment will be added.
 */
  void CutAtPt(EoGePoint3d& point, EoDbGroup* group) override;

  void CutAt2Pts(EoGePoint3d*, EoDbGroupList*, EoDbGroupList*) override;

  /** @brief Generates approximation vertices for the ellipse segment.
   *
   * This method generates a set of vertices that approximates a smooth conic curve with discrete points defined
   * by the center point and major axis. The number of points is determined based on the sweep angle and the lengths
   * of the major and minor axes to ensure a smooth representation.
   *
   * @param center The center point of the ellipse.
   * @param majorAxis The major axis vector of the ellipse.
   */
  void GenerateApproximationVertices(EoGePoint3d center, EoGeVector3d majorAxis) const;

  void ExportToDxf(DRW_Interface* writer) const {
    switch (Subclass()) {
      case ConicType::Circle: {
        DRW_Circle circle;
        circle.basePoint = {m_center.x, m_center.y, m_center.z};
        circle.extPoint = {m_extrusion.x, m_extrusion.y, m_extrusion.z};
        circle.radious = Radius();
        //SetDxfBaseProperties(&circle);
        writer->addCircle(circle);
        break;
      }

      case ConicType::RadialArc: {
        DRW_Arc arc;
        arc.basePoint = {m_center.x, m_center.y, m_center.z};
        arc.extPoint = {m_extrusion.x, m_extrusion.y, m_extrusion.z};
        arc.radious = Radius();
        arc.staangle = m_startAngle;
        arc.endangle = m_endAngle;
        //SetDxfBaseProperties(&arc);
        writer->addArc(arc);
        break;
      }

      case ConicType::Ellipse:
      case ConicType::EllipticalArc: {
        DRW_Ellipse ellipse;
        ellipse.basePoint = {m_center.x, m_center.y, m_center.z};
        ellipse.secPoint = {m_majorAxis.x, m_majorAxis.y, m_majorAxis.z};
        ellipse.extPoint = {m_extrusion.x, m_extrusion.y, m_extrusion.z};
        ellipse.ratio = m_ratio;
        ellipse.staparam = m_startAngle;
        ellipse.endparam = m_endAngle;
        //SetDxfBaseProperties(&ellipse);
        writer->addEllipse(ellipse);
        break;
      }
    }
  }

  [[nodiscard]] bool IsCircle() const noexcept { return fabs(1.0 - m_ratio) <= Eo::numericEpsilon && IsFullConic(); }

  [[nodiscard]] bool IsRadialArc() const noexcept {
    return fabs(1.0 - m_ratio) <= Eo::numericEpsilon && !IsFullConic();
  }

  [[nodiscard]] bool IsEllipse() const noexcept { return m_ratio < 1.0 - Eo::numericEpsilon && IsFullConic(); }

  [[nodiscard]] bool IsEllipticalArc() const noexcept { return m_ratio < 1.0 - Eo::numericEpsilon && !IsFullConic(); }

  [[nodiscard]] bool IsFullConic() const noexcept {
    double sweep = NormalizeTo2Pi(m_endAngle) - NormalizeTo2Pi(m_startAngle);
    if (sweep <= 0.0) sweep += Eo::TwoPi;
    return fabs(sweep - Eo::TwoPi) <= Eo::geometricTolerance ||
           fabs(m_endAngle - m_startAngle) <= Eo::geometricTolerance;
  }

  // Enum for cleaner switch statements
  enum class ConicType { Circle, RadialArc, Ellipse, EllipticalArc };

  [[nodiscard]] ConicType Subclass() const noexcept {
    bool isCircular = fabs(1.0 - m_ratio) <= Eo::numericEpsilon;
    bool isFull = IsFullConic();

    if (isCircular && isFull) return ConicType::Circle;
    if (isCircular && !isFull) return ConicType::RadialArc;
    if (!isCircular && isFull) return ConicType::Ellipse;
    return ConicType::EllipticalArc;
  }

  /** @brief Computes the point on the conic at the start angle.
   *
   * This method calculates the point on the conic (ellipse or arc) corresponding to the start angle.
   * It uses a transformation matrix based on the center, major axis, and minor axis of the conic
   * to convert from parametric coordinates to world coordinates.
   *
   * @return The point on the conic at the start angle.
   */
  EoGePoint3d PointAtStartAngle();

  /// <summary>Determines the bounding region. This is always a quad, but it may not be xy oriented.</summary>
  void GetBoundingBox(EoGePoint3dArray&);

  /**
   * @brief Computes the point on the conic at the end angle.
   *
   * This method calculates the point on the conic (ellipse or arc) corresponding to the end angle.
   * It uses a transformation matrix based on the center, major axis, and minor axis of the conic.
   *
   * @return The point on the conic at the end angle.
   */
  EoGePoint3d PointAtEndAngle();

  void GetXYExtents(EoGePoint3d, EoGePoint3d, EoGePoint3d*, EoGePoint3d*) const;
  int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*) override;

  const EoGePoint3d& Center() const noexcept { return (m_center); }
  void SetCenter(EoGePoint3d center) { m_center = std::move(center); }

  double EndAngle() const noexcept { return (m_endAngle); }
  void SetEndAngle(double endAngle) { m_endAngle = endAngle; }

  const EoGeVector3d& MajorAxis() const noexcept { return (m_majorAxis); }
  void SetMajorAxis(EoGeVector3d majorAxis) { m_majorAxis = std::move(majorAxis); }

  const EoGeVector3d& Extrusion() const noexcept { return (m_extrusion); }
  void SetExtrusion(EoGeVector3d extrusion) { m_extrusion = std::move(extrusion); }

  [[nodiscard]] EoGeVector3d MinorAxis() const noexcept { return CrossProduct(m_extrusion, m_majorAxis) * m_ratio; }

  double Ratio() const noexcept { return (m_ratio); }
  void SetRatio(double ratio) { m_ratio = ratio; }

  double StartAngle() const noexcept { return (m_startAngle); }
  void SetStartAngle(double startAngle) { m_startAngle = startAngle; }

  void SetAngles(double startAngle, double endAngle) {
    m_startAngle = startAngle;
    m_endAngle = endAngle;
  }

  void SetSweepAngle(double sweepAngle) { m_sweepAngle = sweepAngle; }

  [[nodiscard]] double Radius() const noexcept { return m_majorAxis.Length(); }
  [[nodiscard]] double MajorRadius() const noexcept { return m_majorAxis.Length(); }
  [[nodiscard]] double MinorRadius() const noexcept { return Radius() * m_ratio; }

  [[nodiscard]] double SweepAngle() const noexcept {
    double sweepAngle = NormalizeTo2Pi(m_endAngle) - NormalizeTo2Pi(m_startAngle);
    if (sweepAngle <= 0.0) sweepAngle += Eo::TwoPi;
    return sweepAngle;
  }
  [[nodiscard]] double ArcLength() const noexcept {
    if (IsCircle() || IsRadialArc()) { return Radius() * SweepAngle(); }
    // Ramanujan's approximation for ellipse arc length
    double a = MajorRadius();
    double b = MinorRadius();
    double h = pow((a - b) / (a + b), 2);
    double circumference = Eo::Pi * (a + b) * (1 + (3 * h) / (10 + sqrt(4 - 3 * h)));
    return circumference * (SweepAngle() / Eo::TwoPi);
  }

  double SweepAngleToPoint(EoGePoint3d point);

  static double NormalizeTo2Pi(double angle);

 private:
  static CString SubClassName(double ratio, double startAngle, double endAngle);
};

int SweepAngleFromNormalAnd3Points(EoGeVector3d, EoGePoint3d, EoGePoint3d, EoGePoint3d, EoGePoint3d&, double*);
