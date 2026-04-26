#pragma once

#include <algorithm>
#include <cstdint>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

/// @brief Unified planar face primitive — internal representation for DXF 3DFACE, SOLID, and TRACE entities.
///
/// Stores 3 or 4 WCS vertices in sequential CCW order with per-edge visibility flags.
/// On DXF import, SOLID/TRACE bowtie vertex order is normalized to sequential; on DXF export,
/// the original entity type is preserved via SourceType dispatch.
///
/// ## Coordinate System
/// All vertices are stored in WCS. SOLID and TRACE OCS coordinates are transformed to WCS
/// during import (via ApplyExtrusion in the DXF parser). The extrusion direction is preserved
/// for round-trip export of SOLID/TRACE entities. 3DFACE is natively WCS with no extrusion.
///
/// ## PEG Serialization (type code kFacePrimitive = 0x0500)
/// @code
///  Type code         uint16_t   (0x0500)
///  Pen color         int16_t
///  Line type index   int16_t
///  Source type       uint8_t    (0=Face3d, 1=Solid, 2=Trace)
///  Edge flags        uint8_t    (bits 0-3: per-edge invisible flags)
///  Vertex count      uint8_t    (3 or 4)
///  Padding           uint8_t    (reserved, write 0)
///  Extrusion         vector3d   (unit normal; default 0,0,1)
///  Vertices[count]   point3d[]  (WCS, sequential CCW order)
/// @endcode
///
/// ## AE2011 (V1) Backward Compatibility
/// kFacePrimitive does not exist in V1. V1 export is not supported — face entities are a V2-only
/// feature. This is consistent with the fact that V1 never imported 3DFACE, SOLID, or TRACE.
///
/// ## DXF Round-Trip
/// ExportToDxf() dispatches on SourceType to emit the correct DXF entity:
/// - Face3d → 3DFACE with AcDbFace subclass and edge flags
/// - Solid → SOLID with AcDbTrace subclass, bowtie vertex reorder, extrusion/thickness
/// - Trace → TRACE with AcDbTrace subclass, bowtie vertex reorder, extrusion/thickness
class EoDbFace : public EoDbPrimitive {
 public:
  /// @brief DXF source entity type — controls export dispatch and coordinate transform.
  enum class SourceType : std::uint8_t {
    Face3d = 0,  ///< DXF 3DFACE: WCS, no extrusion, AcDbFace subclass, edge flags active
    Solid = 1,  ///< DXF SOLID: OCS→WCS on import, AcDbTrace subclass, always filled, bowtie export
    Trace = 2  ///< DXF TRACE: OCS→WCS on import, AcDbTrace subclass, always filled, bowtie export
  };

  /// @brief Per-edge invisible flags (DXF 3DFACE group code 70).
  /// Bit layout matches DXF exactly — no translation needed on import/export.
  enum EdgeFlags : std::uint8_t {
    AllVisible = 0x00,  ///< Default: all edges visible
    Edge1Invisible = 0x01,  ///< Edge v0→v1 invisible
    Edge2Invisible = 0x02,  ///< Edge v1→v2 invisible
    Edge3Invisible = 0x04,  ///< Edge v2→v3 invisible
    Edge4Invisible = 0x08  ///< Edge v3→v0 invisible (quad only)
  };

 private:
  EoGePoint3d m_vertices[4]{};  ///< WCS vertices, sequential CCW. For triangles, m_vertices[3] is unused.
  EoGeVector3d m_extrusion{EoGeVector3d::positiveUnitZ};  ///< Extrusion direction (SOLID/TRACE OCS round-trip)
  SourceType m_sourceType{SourceType::Face3d};
  std::uint8_t m_edgeFlags{AllVisible};  ///< Per-edge invisible flags (3DFACE only; always 0 for SOLID/TRACE)
  std::uint8_t m_vertexCount{4};  ///< 3 (triangle) or 4 (quad)

 public:
  EoDbFace() = default;

  EoDbFace(const EoDbFace& other);

  ~EoDbFace() override = default;

  const EoDbFace& operator=(const EoDbFace& other);

  // --- Factory methods ---

  /// @brief Creates an EoDbFace from a parsed DXF 3DFACE quad entity.
  /// Vertices are already WCS. Edge flags preserved.
  [[nodiscard]] static EoDbFace* CreateFrom3dFace(const EoGePoint3d& v0,
      const EoGePoint3d& v1,
      const EoGePoint3d& v2,
      const EoGePoint3d& v3,
      std::uint8_t edgeFlags);

  /// @brief Creates a triangle EoDbFace from a parsed DXF 3DFACE entity.
  [[nodiscard]] static EoDbFace* CreateTriangleFrom3dFace(const EoGePoint3d& v0,
      const EoGePoint3d& v1,
      const EoGePoint3d& v2,
      std::uint8_t edgeFlags);

  /// @brief Creates an EoDbFace from a parsed DXF SOLID entity.
  /// Caller must pass vertices AFTER OCS→WCS transform and AFTER bowtie→sequential reorder.
  [[nodiscard]] static EoDbFace* CreateFromSolid(const EoGePoint3d& v0,
      const EoGePoint3d& v1,
      const EoGePoint3d& v2,
      const EoGePoint3d& v3,
      const EoGeVector3d& extrusion);

  /// @brief Creates a triangle EoDbFace from a parsed DXF SOLID entity (3rd == 4th corner).
  [[nodiscard]] static EoDbFace* CreateTriangleFromSolid(const EoGePoint3d& v0,
      const EoGePoint3d& v1,
      const EoGePoint3d& v2,
      const EoGeVector3d& extrusion);

  /// @brief Creates an EoDbFace from a parsed DXF TRACE entity.
  /// Caller must pass vertices AFTER OCS→WCS transform and AFTER bowtie→sequential reorder.
  [[nodiscard]] static EoDbFace* CreateFromTrace(const EoGePoint3d& v0,
      const EoGePoint3d& v1,
      const EoGePoint3d& v2,
      const EoGePoint3d& v3,
      const EoGeVector3d& extrusion);

  // --- EoDbPrimitive virtual contract ---
  void AddReportToMessageList(const EoGePoint3d& point) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbFace*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, EoGsRenderDevice* renderDevice) override;
  void ExportToDxf(EoDxfInterface* writer) const override;
  void FormatExtra(CString& extra) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view,
      EoGePoint3d& minPoint,
      EoGePoint3d& maxPoint,
      const EoGeTransformMatrix& transformMatrix) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive* primitive) override;
  bool Is(std::uint16_t type) noexcept override { return type == EoDb::kFacePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& foundPoint) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeft, EoGePoint3d upperRight) override;
  void Transform(const EoGeTransformMatrix& transformMatrix) override;
  void Translate(const EoGeVector3d& v) override;
  void TranslateUsingMask(EoGeVector3d v, const DWORD mask) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override;

  /// @brief Reads a face primitive from a PEG file stream (type code kFacePrimitive).
  /// @param file The CFile object representing the PEG file to read from.
  /// @return A pointer to the constructed EoDbFace.
  [[nodiscard]] static EoDbFace* ReadFromPeg(CFile& file);

  // --- Face-specific accessors ---
  [[nodiscard]] const EoGePoint3d& Vertex(int index) const noexcept { return m_vertices[index]; }
  [[nodiscard]] std::uint8_t VertexCount() const noexcept { return m_vertexCount; }
  [[nodiscard]] bool IsTriangle() const noexcept { return m_vertexCount == 3; }
  [[nodiscard]] bool IsQuad() const noexcept { return m_vertexCount == 4; }
  [[nodiscard]] SourceType Source() const noexcept { return m_sourceType; }
  [[nodiscard]] std::uint8_t EdgeVisibilityFlags() const noexcept { return m_edgeFlags; }
  [[nodiscard]] const EoGeVector3d& Extrusion() const noexcept { return m_extrusion; }

  /// @brief Returns true if this face renders as a filled surface (SOLID or TRACE source).
  /// 3DFACE renders as wireframe edges only.
  [[nodiscard]] bool IsFilled() const noexcept {
    return m_sourceType == SourceType::Solid || m_sourceType == SourceType::Trace;
  }

  /// @brief Tests whether a specific edge is visible.
  /// @param edgeIndex 0-based edge index (0 = v0→v1, 1 = v1→v2, etc.).
  [[nodiscard]] bool IsEdgeVisible(int edgeIndex) const noexcept { return (m_edgeFlags & (1 << edgeIndex)) == 0; }

  void SetEdgeFlags(std::uint8_t flags) noexcept { m_edgeFlags = flags; }

  /// @brief Computes the face normal as CrossProduct(v1-v0, v2-v0), unitized.
  /// Returns zero vector for degenerate faces.
  [[nodiscard]] EoGeVector3d ComputeNormal() const;

  /// @brief Tests whether the quad is planar within geometric tolerance.
  /// Always returns true for triangles.
  [[nodiscard]] bool IsPlanar() const;

  /// @brief Returns the centroid of the face vertices.
  [[nodiscard]] EoGePoint3d Centroid() const noexcept;
};
