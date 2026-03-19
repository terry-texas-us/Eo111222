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

/// @brief Generalized polyline primitive supporting straight segments, bulge arcs, and per-vertex widths.
///
/// Handles both DXF LWPOLYLINE and 2D/3D POLYLINE entities. Bulge and width data are stored in
/// parallel vectors that remain empty for simple polylines (zero overhead). The `m_flags` bit field
/// indicates which optional data channels are populated.
///
/// PEG serialization format (type code 0x2002):
/// @code
///   Type code <0x2002>   uint16_t
///   Pen color            uint16_t
///   Line type            uint16_t
///   Flags                uint16_t   (sm_Closed | sm_HasBulge | sm_HasWidth | sm_Plinegen)
///   Constant width       double     (DXF group code 43; 0.0 when not set)
///   Number of vertices   uint16_t
///   {vertices}           point3d[]
///   if (flags & sm_HasBulge):
///     {bulge values}     double[]   (one per vertex)
///   if (flags & sm_HasWidth):
///     {start widths}     double[]   (one per vertex)
///     {end widths}       double[]   (one per vertex)
/// @endcode
class EoDbPolyline : public EoDbPrimitive {
  static std::uint16_t sm_EdgeToEvaluate;
  static std::uint16_t sm_Edge;
  static int sm_pivotVertex;

 public:
  /// @name Flag bit constants
  /// @{
  static constexpr std::int16_t sm_Closed = 0x0001;    ///< Last vertex connects back to first
  static constexpr std::int16_t sm_HasBulge = 0x0002;   ///< m_bulges contains per-vertex bulge values
  static constexpr std::int16_t sm_HasWidth = 0x0004;   ///< m_startWidths / m_endWidths are populated
  static constexpr std::int16_t sm_Plinegen = 0x0008;   ///< Generate linetype pattern across vertices
  /// @}

 private:
  std::int16_t m_flags{};
  EoGePoint3dArray m_pts;
  std::vector<double> m_bulges;       ///< Per-vertex bulge: tan(θ/4). Empty when all segments are straight.
  std::vector<double> m_startWidths;  ///< Per-vertex start width. Empty when all widths are zero.
  std::vector<double> m_endWidths;    ///< Per-vertex end width. Empty when all widths are zero.
  double m_constantWidth{};            ///< DXF LWPOLYLINE constant width (group code 43). Zero when not set.

 public:
  EoDbPolyline();
  EoDbPolyline(
      std::int16_t penColor, std::int16_t lineType, EoGePoint3d& centerPoint, double radius, int numberOfSides);
  EoDbPolyline(std::int16_t penColor, std::int16_t lineType, EoGePoint3dArray& pts);
  EoDbPolyline(EoGePoint3dArray& pts);
  EoDbPolyline(const EoDbPolyline& polyline);

  ~EoDbPolyline() override = default;

  const EoDbPolyline& operator=(const EoDbPolyline& polyline);

  void AddReportToMessageList(const EoGePoint3d&) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbPolyline*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive*) noexcept override { return false; }
  bool Is(std::uint16_t type) noexcept override { return type == EoDb::kPolylinePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool PivotOnControlPoint(AeSysView* view, const EoGePoint4d& ptView) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(const EoGeTransformMatrix&) override;
  void Translate(const EoGeVector3d& v) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override { (void)file, (void)buffer; };

  /// @brief Reads a polyline primitive from a PEG file stream (type code kPolylinePrimitive).
  /// @param file The CFile object representing the PEG file to read from.
  /// @return A pointer to the constructed EoDbPolyline.
  static EoDbPolyline* ReadFromPeg(CFile& file);

  /// @brief Reads a legacy CSpline primitive from a PEG file stream and converts it to a polyline.
  /// @param file The CFile object representing the PEG file to read from.
  /// @return A pointer to the constructed EoDbPolyline.
  static EoDbPolyline* ReadFromCSplinePeg(CFile& file);

 private:
  std::uint16_t SwingVertex();

  /// @brief Builds a flattened point array with arc segments tessellated into line approximations.
  ///
  /// When the polyline has bulge data, each bulge arc segment is tessellated via
  /// polyline::TessellateArcSegment. Straight segments contribute only their endpoint.
  /// The closing segment (last→first) is included for closed polylines.
  /// When no bulge data exists, returns a copy of the raw vertices (with the first vertex
  /// appended for closed polylines to form a closed loop).
  ///
  /// @param[out] tessellatedPoints  Output array receiving the complete tessellated polyline.
  void BuildTessellatedPoints(EoGePoint3dArray& tessellatedPoints) const;

 public:
  static std::uint16_t& EdgeToEvaluate() noexcept { return sm_EdgeToEvaluate; }
  static std::uint16_t& Edge() noexcept { return sm_Edge; }

  /// @name Flag accessors
  /// @{
  [[nodiscard]] bool IsClosed() const noexcept { return (m_flags & sm_Closed) != 0; }
  [[nodiscard]] bool HasBulge() const noexcept { return (m_flags & sm_HasBulge) != 0; }
  [[nodiscard]] bool HasWidth() const noexcept { return (m_flags & sm_HasWidth) != 0; }
  [[nodiscard]] bool HasPlinegen() const noexcept { return (m_flags & sm_Plinegen) != 0; }

  void SetClosed(bool closed) noexcept {
    if (closed) {
      m_flags |= sm_Closed;
    } else {
      m_flags &= ~sm_Closed;
    }
  }
  void SetPlinegen(bool plinegen) noexcept {
    if (plinegen) {
      m_flags |= sm_Plinegen;
    } else {
      m_flags &= ~sm_Plinegen;
    }
  }
  /// @}

  /// @brief Legacy compatibility — prefer IsClosed() for new code.
  [[nodiscard]] bool IsLooped() const noexcept { return IsClosed(); }

  void SetFlag(const std::int16_t flags) noexcept { m_flags = flags; }

  void SetNumberOfVertices(const size_t numberOfVertices) { m_pts.SetSize(static_cast<int64_t>(numberOfVertices)); }

  /// @brief Sets vertex coordinates from a 3D point (used by heavy POLYLINE 3D and 2D converters).
  void SetVertex(size_t index, const EoGePoint3d& point) {
    m_pts[static_cast<int64_t>(index)] = point;
  }

  /// @brief Sets vertex coordinates from a lightweight polyline vertex, including computed Z.
  void SetVertexFromLwVertex(size_t index, const EoDxfPolylineVertex2d& vertex) {
    m_pts[static_cast<int64_t>(index)].x = vertex.x;
    m_pts[static_cast<int64_t>(index)].y = vertex.y;
    m_pts[static_cast<int64_t>(index)].z = vertex.z;
  }

  /// @name Bulge and width data
  /// @{

  /// @brief Moves per-vertex bulge values into this polyline and sets the sm_HasBulge flag.
  /// Pass an empty vector to clear bulge data.
  void SetBulges(std::vector<double>&& bulges) noexcept {
    m_bulges = std::move(bulges);
    if (!m_bulges.empty()) {
      m_flags |= sm_HasBulge;
    } else {
      m_flags &= ~sm_HasBulge;
    }
  }

  /// @brief Moves per-vertex start/end widths into this polyline and sets the sm_HasWidth flag.
  /// Pass empty vectors to clear width data.
  void SetWidths(std::vector<double>&& startWidths, std::vector<double>&& endWidths) noexcept {
    m_startWidths = std::move(startWidths);
    m_endWidths = std::move(endWidths);
    if (!m_startWidths.empty() || !m_endWidths.empty()) {
      m_flags |= sm_HasWidth;
    } else {
      m_flags &= ~sm_HasWidth;
    }
  }

  [[nodiscard]] const std::vector<double>& Bulges() const noexcept { return m_bulges; }
  [[nodiscard]] const std::vector<double>& StartWidths() const noexcept { return m_startWidths; }
  [[nodiscard]] const std::vector<double>& EndWidths() const noexcept { return m_endWidths; }

  void SetConstantWidth(double constantWidth) noexcept { m_constantWidth = constantWidth; }
  [[nodiscard]] double ConstantWidth() const noexcept { return m_constantWidth; }
  /// @}
};
