#pragma once

#include <utility>

#include "AeSysView.h"
#include "drw_base.h"
#include "drw_interface.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

class EoDbConic : public EoDbPrimitive {
  EoGePoint3d m_center{};
  EoGeVector3d m_majorAxis{};
  EoGeVector3d m_extrusion{EoGeVector3d::positiveUnitZ};
  double m_ratio{1.0};    // Ratio of minor axis to major axis [0.0 to 1.0]
  double m_startAngle{};  // Start parameter angle in radians
  double m_endAngle{};    // End parameter angle in radians

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
  [[nodiscard]] static EoDbConic* CreateCircle(const EoGePoint3d& center, const EoGeVector3d& extrusion, double radius);

  /** @brief Creates a circle in the active view's plane.
   *
   * This static method creates a circle centered at the specified point with the given radius,
   * oriented according to the active view's camera direction.
   * @param center The center point of the circle.
   * @param radius The radius of the circle.
   * @return A pointer to the newly created EoDbConic representing the circle.
   */
  [[nodiscard]] static EoDbConic* CreateCircleInView(const EoGePoint3d& center, double radius);

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
  [[nodiscard]] static EoDbConic* CreateConic(const EoGePoint3d& center, const EoGeVector3d& extrusion,
                                              const EoGeVector3d& majorAxis, double ratio, double startAngle,
                                              double endAngle);

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
  [[nodiscard]] static EoDbConic* CreateConicFromEllipsePrimitive(const EoGePoint3d& center,
                                                                  const EoGeVector3d& majorAxis,
                                                                  const EoGeVector3d& minorAxis, double sweepAngle);

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
  [[nodiscard]] static EoDbConic* CreateEllipse(const EoGePoint3d& center, const EoGeVector3d& extrusion,
                                                const EoGeVector3d& majorAxis, double ratio);
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
  [[nodiscard]] static EoDbConic* CreateRadialArc(const EoGePoint3d& center, const EoGeVector3d& extrusion,
                                                  double radius, double startAngle, double endAngle);
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
  [[nodiscard]] static EoDbConic* CreateRadialArcFrom3Points(EoGePoint3d& start, const EoGePoint3d& intermediate,
                                                             EoGePoint3d& end);

 public:
  EoDbConic()
      : m_center{},
        m_majorAxis{},
        m_extrusion{EoGeVector3d::positiveUnitZ},
        m_ratio{1.0},
        m_startAngle{},
        m_endAngle{} {}

  EoDbConic(const EoDbConic& other);

  ~EoDbConic() override {}

 public:
  const EoDbConic& operator=(const EoDbConic&);

 public:  // Methods - absolute virtuals
  void AddReportToMessageList(EoGePoint3d) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbConic*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;

  /** @brief Displays the conic on the given view and device context.
   *
   * This method renders the conic on the specified view and device context. It first checks for null pointers
   * and skips degenerate arcs or major axes. It sets the pen state and generates approximation vertices for rendering.
   *
   * @param view Pointer to the AeSysView where the conic will be displayed.
   * @param deviceContext Pointer to the CDC device context used for rendering.
   */
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
  void Transform(EoGeTransformMatrix& transformMatrix) override;
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
  void CutAtPoint(EoGePoint3d& point, EoDbGroup* group) override;

  /**
   * @brief Cuts the conic at two specified points, creating new groups for the cut sections.
   *
   * This method modifies the current conic by cutting it at two specified points. It creates new groups for the sections
   * that are cut out and adds them to the provided group lists.
   *
   * @param firstPoint The first point at which to cut the conic.
   * @param secondPoint The second point at which to cut the conic.
   * @param groups A pointer to a list of groups where the remaining sections of the conic will be added.
   * @param newGroups A pointer to a list of groups where the cut-out sections of the conic will be added.
   */
  void CutAt2Points(const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList*,
                    EoDbGroupList*) override;

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
        /// @todoSetDxfBaseProperties(&circle);
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
        /// @todo SetDxfBaseProperties(&arc);
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
        /// @todo SetDxfBaseProperties(&ellipse);
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

  /** @brief Determines if the conic represents a full circle or ellipse.
   *
   * This method checks whether the conic is a complete circle or ellipse by evaluating
   * the sweep angle derived from the start and end angles. It accounts for floating-point
   * precision using a geometric tolerance.
   *
   * @return True if the conic is a full circle or ellipse; otherwise, false.
   */
  [[nodiscard]] bool IsFullConic() const noexcept {
    double sweep = NormalizeTo2Pi(m_endAngle) - NormalizeTo2Pi(m_startAngle);
    if (sweep <= 0.0) sweep += Eo::TwoPi;
    return fabs(sweep - Eo::TwoPi) < Eo::geometricTolerance || fabs(m_endAngle - m_startAngle) < Eo::geometricTolerance;
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

  /** @brief Writes the conic as a legacy ellipse primitive for backward compatibility.
   *
   * This method writes the conic in the format expected by PEG file version 11 and earlier,
   * which only understands the EoDbEllipse primitive format.
   *
   * @param file The file to write to.
   * @return true if the write was successful.
   */
  bool WriteLegacyEllipse(CFile& file);

  /// <summary>Determines the bounding region. This is always a quad, but it may not be xy oriented.</summary>
  void GetBoundingBox(EoGePoint3dArray&);

  void GetXYExtents(EoGePoint3d, EoGePoint3d, EoGePoint3d*, EoGePoint3d*) const;
  int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*) override;

  [[nodiscard]] const EoGePoint3d& Center() const noexcept { return (m_center); }
  void SetCenter(EoGePoint3d center) { m_center = std::move(center); }

  [[nodiscard]] double EndAngle() const noexcept { return (m_endAngle); }
  void SetEndAngle(double endAngle) { m_endAngle = endAngle; }

  [[nodiscard]] const EoGeVector3d& MajorAxis() const noexcept { return (m_majorAxis); }
  void SetMajorAxis(EoGeVector3d majorAxis) { m_majorAxis = std::move(majorAxis); }

  [[nodiscard]] const EoGeVector3d& Extrusion() const noexcept { return (m_extrusion); }
  void SetExtrusion(EoGeVector3d extrusion) { m_extrusion = std::move(extrusion); }

  [[nodiscard]] EoGeVector3d MinorAxis() const noexcept { return CrossProduct(m_extrusion, m_majorAxis) * m_ratio; }

  [[nodiscard]] double Ratio() const noexcept { return (m_ratio); }
  void SetRatio(double ratio) { m_ratio = ratio; }

  [[nodiscard]] double StartAngle() const noexcept { return (m_startAngle); }
  void SetStartAngle(double startAngle) { m_startAngle = startAngle; }

  void SetAngles(double startAngle, double endAngle) {
    m_startAngle = startAngle;
    m_endAngle = endAngle;
  }

  [[nodiscard]] double Radius() const noexcept { return m_majorAxis.Length(); }
  [[nodiscard]] double MajorRadius() const noexcept { return m_majorAxis.Length(); }
  [[nodiscard]] double MinorRadius() const noexcept { return Radius() * m_ratio; }

  /** @brief Calculates the sweep angle of the conic.
   *
   * This method computes the sweep angle of the conic (arc or ellipse) based on the
   * start and end angles. It normalizes the angles to the range [0, 2π) and calculates
   * the difference. If the result is non-positive, it adds 2π to ensure a positive sweep angle.
   *
   * @return The sweep angle in radians.
   */
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

  /** @brief Computes the point on the conic at the start angle.
   *
   * This method calculates the point on the conic (ellipse or arc) corresponding to the start angle.
   * It uses a transformation matrix based on the center, major axis, and minor axis of the conic
   * to convert from parametric coordinates to world coordinates.
   *
   * @return The point on the conic at the start angle.
   */
  [[nodiscard]] EoGePoint3d PointAtStartAngle() const { return PointAtAngle(m_startAngle); }

  /**
   * @brief Computes the point on the conic at the end angle.
   *
   * This method calculates the point on the conic (ellipse or arc) corresponding to the end angle.
   * It uses a transformation matrix based on the center, major axis, and minor axis of the conic.
   *
   * @return The point on the conic at the end angle.
   */
  [[nodiscard]] EoGePoint3d PointAtEndAngle() const { return PointAtAngle(m_endAngle); }

  [[nodiscard]] double SweepAngleToPoint(const EoGePoint3d& point);

  static double NormalizeTo2Pi(double angle);

 private:
  [[nodiscard]] EoGePoint3d PointAtAngle(double angle) const {
    const auto minorAxis = MinorAxis();
    if (m_majorAxis.Length() <= Eo::geometricTolerance) { return m_center; }

    EoGeTransformMatrix transformMatrix(m_center, m_majorAxis, minorAxis);
    transformMatrix.Inverse();

    const EoGePoint3d point(cos(angle), sin(angle), 0.0);
    return transformMatrix * point;
  }

  static CString SubClassName(double ratio, double startAngle, double endAngle);
};

/** @brief Given a plane normal and three points (two outside and one inside), find the sweep angle defined by the three points about the center point.
 * @param planeNormal Normal vector of the plane containing the points
 * @param firstOutside First outside point
 * @param inside Inside point
 * @param secondOutside Second outside point
 * @param center Center point about which to measure the sweep angle
 * @param[out] sweepAngle Sweep angle result
 * @return true if successful, false if not.
*/
[[nodiscard]] bool SweepAngleFromNormalAnd3Points(const EoGeVector3d& normal, const EoGePoint3d& firstOutside,
                                                  const EoGePoint3d& inside, const EoGePoint3d& secondOutside,
                                                  const EoGePoint3d& center, double& sweepAngle);
