#pragma once

#include <cstdint>
#include <vector>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

class EoDbSpline : public EoDbPrimitive {
  std::int16_t m_degree{3};  ///< Spline degree (default 3 = cubic). Order = degree + 1.
  std::int16_t m_flags{};  ///< DXF spline flags (0x01=Closed, 0x02=Periodic, 0x04=Rational, 0x08=Planar).
  EoGePoint3dArray m_pts;  ///< Control points (WCS).
  std::vector<double> m_knots;  ///< Knot vector. Empty = uniform (regenerated at render/export time).
  std::vector<double> m_weights;  ///< Per-control-point weights. Empty = non-rational (all weights 1.0).

 public:  // Constructors and destructor
  EoDbSpline() {}
  EoDbSpline(std::uint8_t* buffer, int version);
  EoDbSpline(std::uint16_t, EoGePoint3d*);
  EoDbSpline(EoGePoint3dArray& points);
  EoDbSpline(std::int16_t penColor, std::int16_t lineType, EoGePoint3dArray& points);
  EoDbSpline(const EoDbSpline&);

  ~EoDbSpline() override = default;

  EoDbSpline& operator=(const EoDbSpline&);

  void AddReportToMessageList(const EoGePoint3d&) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbSpline*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, EoGsRenderDevice* renderDevice) override;
  void ExportToDxf(EoDxfInterface* writer) const override;
  void GetAllPoints(EoGePoint3dArray& pts) override;
  [[nodiscard]] CString TypeLabel() const override { return L"Spline"; }
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(std::uint16_t type) noexcept override { return type == EoDb::kSplinePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  bool IsWhollyContainedByRectangle(AeSysView* view, EoGePoint3d lowerLeft, EoGePoint3d upperRight) override;
  void Transform(const EoGeTransformMatrix&) override;
  void Translate(const EoGeVector3d& v) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override;

  /// @brief Reads a spline primitive from a PEG file stream (type code kSplinePrimitive).
  /// @param file The CFile object representing the PEG file to read from.
  /// @return A pointer to the constructed EoDbSpline.
  static EoDbSpline* ReadFromPeg(CFile& file);

  // --- DXF spline property accessors ---

  [[nodiscard]] std::int16_t Degree() const noexcept { return m_degree; }
  void SetDegree(std::int16_t degree) noexcept { m_degree = degree; }

  [[nodiscard]] std::int16_t Flags() const noexcept { return m_flags; }
  void SetFlags(std::int16_t flags) noexcept { m_flags = flags; }

  [[nodiscard]] const std::vector<double>& Knots() const noexcept { return m_knots; }
  void SetKnots(std::vector<double>&& knots) noexcept { m_knots = std::move(knots); }
  void SetKnots(const std::vector<double>& knots) { m_knots = knots; }

  [[nodiscard]] const std::vector<double>& Weights() const noexcept { return m_weights; }
  void SetWeights(std::vector<double>&& weights) noexcept { m_weights = std::move(weights); }
  void SetWeights(const std::vector<double>& weights) { m_weights = weights; }

  [[nodiscard]] bool IsClosed() const noexcept { return (m_flags & 0x01) != 0; }
  [[nodiscard]] bool IsPeriodic() const noexcept { return (m_flags & 0x02) != 0; }
  [[nodiscard]] bool IsRational() const noexcept { return (m_flags & 0x04) != 0; }
  [[nodiscard]] bool IsPlanar() const noexcept { return (m_flags & 0x08) != 0; }

  /** @brief Generates a set of points along the spline curve based on the control points and the specified order.
   * @param order The order of the spline (degree + 1).
   * @param controlPoints An array of control points that define the shape of the spline.
   * @return The number of points generated along the spline curve.
   */
  int GenPts(const std::int16_t order, const EoGePoint3dArray& controlPoints);
  void SetPt(std::uint16_t w, EoGePoint3d pt) { m_pts[w] = pt; }
};
