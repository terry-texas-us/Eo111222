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

class EoDbConic : public EoDbPrimitive {
  EoGePoint3d m_center;
  EoGeVector3d m_majorAxis;
  EoGeVector3d m_extrusion;
  double m_ratio;  // Ratio of minor axis to major axis [0.0 to 1.0]
  double m_startAngle;
  double m_endAngle;

  EoGeVector3d m_minorAxis;
  double m_sweepAngle;

 public:
  EoDbConic()
      : m_center{},
        m_majorAxis{},
        m_extrusion{EoGeVector3d::positiveUnitZ},
        m_ratio{1.0},
        m_startAngle{0.0},
        m_endAngle{0.0} {}

  EoDbConic(EoGePoint3d& center, EoGePoint3d& start);

  EoDbConic(EoGePoint3d& center, EoGeVector3d& extrusion, double radius);

  EoDbConic(const EoGePoint3d& center, double radius, double startAngle, double endAngle);

  // EoDbConic(const EoGePoint3d& center, const EoGeVector3d& majorAxis, const EoGeVector3d& minorAxis, double sweepAngle);

  // EoDbConic(EoGePoint3d& center, double radius, EoInt16 color = COLOR_BYLAYER, EoInt16 lineTypeIndex = LINETYPE_BYLAYER);

  // EoDbConic(EoGePoint3d& center, EoGeVector3d& majorAxis, EoGeVector3d& minorAxis, double sweepAngle, EoInt16 penColor, EoInt16 lineTypeIndex);

  // EoDbConic(EoGePoint3d& center, EoGeVector3d& planeNormal, double radius, EoInt16 penColor, EoInt16 lineTypeIndex);

  /** 
 * @brief Constructs a radial arc (as ellipse) primitive from three points that define an elliptical arc.
 *
 * This constructor initializes an ellipse segment using three points: a beginning point, an intermediate point, and an end point.
 * It calculates the center point, major axis, minor axis, and sweep angle of the ellipse based on the provided points.
 * The pen color and line type are set based on the current primitive state.
 *
 * @param begin The starting point of the elliptical arc.
 * @param intermediate A point on the elliptical arc between the start and end points.
 * @param end The ending point of the elliptical arc.
 */
  EoDbConic(EoGePoint3d begin, EoGePoint3d intermediate, EoGePoint3d end);

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
  void CutAtPt(EoGePoint3d&, EoDbGroup*) override;
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

  EoGePoint3d GetBegPt();
  /// <summary>Determines the bounding region. This is always a quad, but it may not be xy oriented.</summary>
  void GetBoundingBox(EoGePoint3dArray&);
  EoGePoint3d GetEndPt();

  void GetXYExtents(EoGePoint3d, EoGePoint3d, EoGePoint3d*, EoGePoint3d*) const;
  int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*) override;

  const EoGePoint3d& CenterPoint() const noexcept { return (m_center); }
  void SetCenterPoint(EoGePoint3d centerPoint) { m_center = std::move(centerPoint); }

  double EndAngle() const noexcept { return (m_endAngle); }
  void SetEndAngle(double endAngle) { m_endAngle = endAngle; }

  const EoGeVector3d& MajorAxis() const noexcept { return (m_majorAxis); }
  void SetMajorAxis(EoGeVector3d majorAxis) { m_majorAxis = std::move(majorAxis); }

  [[nodiscard]] EoGeVector3d MinorAxis() const noexcept { return EoGeCrossProduct(m_extrusion, m_majorAxis) * m_ratio; }

  void SetMinorAxis(EoGeVector3d minorAxis) { m_minorAxis = std::move(minorAxis); }

  double Ratio() const noexcept { return (m_ratio); }
  void SetRatio(double ratio) { m_ratio = ratio; }

  double StartAngle() const noexcept { return (m_startAngle); }
  void SetStartAngle(double startAngle) { m_startAngle = startAngle; }

  void SetSweepAngle(double sweepAngle) { m_sweepAngle = sweepAngle; }
  [[nodescard]] double SweepAngle() const noexcept { return m_endAngle - m_startAngle; }

  double SweepAngleToPoint(EoGePoint3d point);

 private:
  static double NormalizeTo2Pi(double angle);
  static CString SubClassName(double ratio, double startAngle, double endAngle);
};
