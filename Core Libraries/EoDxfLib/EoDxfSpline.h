#pragma once

#include <cstdint>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"

/** @brief Class to handle spline entity
 *
 *  A spline is a smooth curve defined by control points and a degree.
 *  It is specified in the DXF file using group codes for its control points (code 10, 20, 30), degree (code 71),
 * and number of control points (code 73). The spline entity can also include properties such as knot values (code 40),
 * flags (code 70), and extrusion direction (code 210, 220, 230), which can affect how it is rendered in the drawing.
 */
class EoDxfSpline : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfSpline() noexcept : EoDxfGraphic{EoDxf::SPLINE} {}

  ~EoDxfSpline() {
    for (EoDxfGeometryBase3d* point : m_controlPoints) { delete point; }
    for (EoDxfGeometryBase3d* point : m_fitPoints) { delete point; }
  }

  // Prevent double-free from shallow copy of raw-pointer vectors
  EoDxfSpline(const EoDxfSpline&) = delete;
  EoDxfSpline& operator=(const EoDxfSpline&) = delete;

  EoDxfSpline(EoDxfSpline&& other) noexcept = default;
  EoDxfSpline& operator=(EoDxfSpline&& other) noexcept = default;

  void ApplyExtrusion() override {}

  [[nodiscard]] bool IsClosed() const noexcept { return (m_splineFlag & 0x01) != 0; }
  [[nodiscard]] bool IsPeriodic() const noexcept { return (m_splineFlag & 0x02) != 0; }
  [[nodiscard]] bool IsRational() const noexcept { return (m_splineFlag & 0x04) != 0; }

  /// Start/end tangents are valid when either vector is non-zero.
  [[nodiscard]] bool IsTangentValid() const noexcept {
    return !m_startTangent.IsZero() || !m_endTangent.IsZero();
  }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_normalVector;  // Group codes 210, 220, 230
  EoDxfGeometryBase3d m_startTangent;  // Group codes 12, 22, 32
  EoDxfGeometryBase3d m_endTangent;  // Group codes 13, 23, 33
  std::int16_t m_splineFlag{};  // Group code 70
  std::int16_t m_degreeOfTheSplineCurve{};  // Group code 71
  std::int16_t m_numberOfKnots{};  // Group code 72
  std::int16_t m_numberOfControlPoints{};  // Group code 73
  std::int16_t m_numberOfFitPoints{};  // Group code 74
  double m_knotTolerance{0.0000001};  // Group code 42
  double m_controlPointTolerance{0.0000001};  // Group code 43
  double m_fitTolerance{0.0000000001};  // Group code 44

  std::vector<double> m_knotValues;  // Group code 40, (one entry per knot)
  std::vector<double> m_weightValues;  // Group code 41, (one entry per control point, rational splines)
  std::vector<EoDxfGeometryBase3d*> m_controlPoints;  // Group codes 10, 20 & 30 (one entry per control point)
  std::vector<EoDxfGeometryBase3d*> m_fitPoints;  // Group codes 11, 21 & 31 (one entry per fit point)

 private:
  EoDxfGeometryBase3d* m_controlPoint{};  // current control point to add data
  EoDxfGeometryBase3d* m_fitPoint{};  // current fit point to add data
};
