#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"

class EoDxfSpline;

/// @brief A single pattern definition line within a HATCH entity.
///
/// DXF group codes per line: 53 (angle in degrees), 43/44 (base point X/Y),
/// 45/46 (offset X/Y), 79 (number of dash items), 49 (dash lengths, repeated).
struct EoDxfHatchPatternDefinitionLine {
  double angle{};       ///< Line angle in degrees (group code 53)
  double basePointX{};  ///< Pattern line base point X (group code 43)
  double basePointY{};  ///< Pattern line base point Y (group code 44)
  double offsetX{};     ///< Pattern line offset X (group code 45)
  double offsetY{};     ///< Pattern line offset Y (group code 46)
  std::vector<double> dashLengths;  ///< Dash lengths (group code 49, repeated per code 79 count)
};

/** @brief Class to handle hatch entity
 *
 *  A hatch entity represents a filled area defined by a hatch pattern.
 *  It is defined by its pattern name (code 2), solid fill flag (code 70), associativity (code 71), hatch style (code
 * 75), hatch pattern type (code 76), and hatch pattern angle and scale (code 52 and 41). The hatch entity can also
 * include properties such as the number of boundary paths (code 91) and the number of pattern definition lines (code
 * 78), which can affect how it is rendered in the drawing. The boundary paths of the hatch are defined by hatch loops,
 * which can include various entities such as lines, arcs, circles, ellipses, splines, and lightweight polylines.
 */
class EoDxfHatch : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfHatch() noexcept : EoDxfGraphic{EoDxf::HATCH} {}

  ~EoDxfHatch() {
    for (auto* hatchLoop : m_hatchLoops) { delete hatchLoop; }
  }

  EoDxfHatch(const EoDxfHatch&) = delete;
  EoDxfHatch& operator=(const EoDxfHatch&) = delete;
  EoDxfHatch(EoDxfHatch&&) = delete;
  EoDxfHatch& operator=(EoDxfHatch&&) = delete;

  void AppendLoop(EoDxfHatchLoop* hatchLoop) { m_hatchLoops.push_back(hatchLoop); }

  /// @brief Returns a read-only reference to the vector of hatch boundary loops.
  [[nodiscard]] const std::vector<EoDxfHatchLoop*>& HatchLoops() const noexcept { return m_hatchLoops; }

  void ApplyExtrusion() noexcept override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_elevationPoint{};  // Group codes 10, 20 & 30
  std::wstring m_hatchPatternName;  // Group code 2
  double m_hatchPatternAngle{};  // Group code 52
  double m_hatchPatternScaleOrSpacing{};  // Group code 41
  std::int32_t m_numberOfBoundaryPaths{};  // Group code 91
  std::int16_t m_solidFillFlag{1};  // Group code 70
  std::int16_t m_associativityFlag{};  // Group code 71
  std::int16_t m_hatchStyle{};  // Group code 75
  std::int16_t m_hatchPatternType{1};  // Group code 76
  std::int16_t m_hatchPatternDoubleFlag{};  // Group code 77
  std::int16_t m_mPolygonBoundaryAnnotationFlag{};  // Group code 73
  std::int16_t m_numberOfPatternDefinitionLines{};  //   Group code 78
  std::vector<EoDxfHatchPatternDefinitionLine> m_patternDefinitionLines;

 private:
  void ClearEntities() noexcept;
  void AddLine();
  void AddArc();
  void AddEllipse();
  void AddSpline();

  bool m_isElevationPointParsed{};
  bool m_isReadingSeedPoints{};
  std::int32_t m_seedPointsRemaining{};
  std::vector<std::unique_ptr<EoDxfPoint>> m_seedPoints;  // ← one per seed point
  EoDxfPoint* m_currentSeedPoint{};  // temp while reading the pair
  std::vector<EoDxfHatchLoop*> m_hatchLoops;

  EoDxfHatchLoop* m_hatchLoop{};  // current loop to add data
  EoDxfLine* m_line{};
  EoDxfArc* m_arc{};
  EoDxfEllipse* m_ellipse{};
  EoDxfSpline* m_spline{};
  EoDxfLwPolyline* m_polyline{};
  EoDxfPoint* m_point{};
  EoDxfPolylineVertex2d* m_polylineVertex{};
  bool m_isPolyline{};
  EoDxfHatchPatternDefinitionLine* m_currentPatternLine{};  ///< Current pattern line during parsing
};
