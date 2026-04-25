#pragma once

#include <cstdint>
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

class EoDbEllipse : public EoDbPrimitive {
  EoGePoint3d m_center;
  EoGeVector3d m_majorAxis;
  EoGeVector3d m_minorAxis;
  double m_sweepAngle;

 public:
  EoDbEllipse() : m_center{}, m_majorAxis{}, m_minorAxis{}, m_sweepAngle{0.0} {}

  /**
   * @brief Constructs a circle (as ellipse) primitive defined by a center point and a start point in the current view.
   *
   * This constructor initializes a circle primitive using the specified center point and a start point that defines the
   * radius. The major and minor axes are calculated based on the view's camera direction. The pen color and line type
   * are set based on the current primitive state.
   *
   * @param center The center point of the circle.
   * @param start The start point that defines the radius of the circle.
   */
  EoDbEllipse(EoGePoint3d& center, EoGePoint3d& start);

  // EoDbEllipse(EoGePoint3d& center, EoGeVector3d& extrusion, double radius);

  // EoDbEllipse(const EoGePoint3d& center, double radius, double startAngle, double endAngle);

  EoDbEllipse(
      const EoGePoint3d& center, const EoGeVector3d& majorAxis, const EoGeVector3d& minorAxis, double sweepAngle);

  EoDbEllipse(EoGePoint3d& center, EoGeVector3d& majorAxis, EoGeVector3d& minorAxis, double sweepAngle,
      std::int16_t color, std::int16_t lineTypeIndex);

  EoDbEllipse(EoGePoint3d& center, double radius, std::int16_t color = COLOR_BYLAYER,
      std::int16_t lineTypeIndex = LINETYPE_BYLAYER);

  /**
   * @brief Constructs a circle (as ellipse) primitive defined by a center point, plane normal, and radius.
   *
   * This constructor initializes a circle primitive using the specified center point, plane normal, and radius.
   * The major and minor axes are calculated based on the provided normal vector.
   * The pen color and line type are set based on the provided parameters.
   *
   * @param center The center point of the circle.
   * @param normal The normal vector defining the plane of the circle.
   * @param radius The radius of the circle.
   * @param color The pen color for the ellipse.
   * @param lineType The line type index for the ellipse.
   */
  EoDbEllipse(EoGePoint3d& center, EoGeVector3d& normal, double radius, std::int16_t color, std::int16_t lineType);

  EoDbEllipse(EoGePoint3d start, EoGePoint3d intermediate, EoGePoint3d end);

  EoDbEllipse(const EoDbEllipse&);

  ~EoDbEllipse() override = default;

 public:
  const EoDbEllipse& operator=(const EoDbEllipse&);

 public:  // Methods - absolute virtuals
  void AddReportToMessageList(const EoGePoint3d&) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbEllipse*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, EoGsRenderDevice* renderDevice) override;
  void ExportToDxf(EoDxfInterface* writer) const override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  [[nodiscard]] EoGePoint3d GetControlPoint() override { return m_center; }
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(std::uint16_t type) noexcept override { return type == EoDb::kEllipsePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(const EoGeTransformMatrix&) override;
  void Translate(const EoGeVector3d& v) override { m_center += v; }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override;

 public:
  void CutAtPoint(const EoGePoint3d& point, EoDbGroup*) override;
  void CutAt2Points(
      const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList*, EoDbGroupList*) override;

  /// @brief Generates a set of points which may be used to represent a arc using a double angle algorithm.
  void GenPts(EoGePoint3d centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis, double sweepAngle) const;
  EoGePoint3d PointAtStartAngle();

  /// @brief Determines the bounding region. This is always a quad, but it may not be xy oriented.
  void GetBoundingBox(EoGePoint3dArray&) const;
  [[nodiscard]] EoGePoint3d PointAtEndAngle() const;
  [[nodiscard]] const EoGeVector3d& MajorAxis() const noexcept { return m_majorAxis; }
  [[nodiscard]] const EoGeVector3d& MinorAxis() const noexcept { return m_minorAxis; }
  [[nodiscard]] const EoGePoint3d& CenterPoint() const noexcept { return m_center; }
  [[nodiscard]] double SweepAngle() const noexcept { return m_sweepAngle; }
  void GetXYExtents(EoGePoint3d, EoGePoint3d, EoGePoint3d*, EoGePoint3d*) const;
  int IsWithinArea(const EoGePoint3d& lowerLeft, const EoGePoint3d& upperRight, EoGePoint3d*) override;
  void SetCenterPoint(EoGePoint3d centerPoint) { m_center = std::move(centerPoint); }
  void SetMajorAxis(EoGeVector3d majorAxis) { m_majorAxis = std::move(majorAxis); }
  void SetMinorAxis(EoGeVector3d minorAxis) { m_minorAxis = std::move(minorAxis); }
  void SetSweepAngle(double sweepAngle) { m_sweepAngle = sweepAngle; }

  double SweepAngleToPoint(EoGePoint3d point);

 private:
  static double NormalizeTo2Pi(double angle);
};
