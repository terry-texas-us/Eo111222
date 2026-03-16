#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfEntity.h"
#include "EoDxfGeometry.h"
#include "EoDxfLineWidths.h"
#include "EoDxfReader.h"

class EoDxfPolyline;

/** @brief Base class for all DXF entities, containing common properties and methods for parsing and extrusion.
 *
 *  This class serves as the base for all specific DXF entity types (e.g., Point, Line, Circle, etc.). It contains
 * common properties such as layer, line type, color, and extrusion direction, as well as methods for parsing DXF group
 * codes and applying extrusion transformations. Derived classes will implement the specific parsing logic for their
 * respective entity types and may override the ApplyExtrusion method if they have extrusion data.
 */
class EoDxfGraphic : public EoDxfEntity {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfGraphic() = default;

 protected:
  explicit EoDxfGraphic(EoDxf::ETYPE entityType) noexcept : EoDxfEntity{entityType} {}

 public:
  EoDxfGraphic(const EoDxfGraphic& other);
  EoDxfGraphic& operator=(const EoDxfGraphic& other);

  EoDxfGraphic(EoDxfGraphic&&) noexcept = default;
  EoDxfGraphic& operator=(EoDxfGraphic&&) noexcept = default;

  virtual ~EoDxfGraphic() = default;

  void Clear();

  virtual void ApplyExtrusion() = 0;

  [[nodiscard]] double GetThickness() const noexcept { return m_thickness; }

 protected:
  /** @brief Parses dxf code and value to read entity data
   *  @param code dxf code
   *  @param reader pointer to EoDxfReader to read value
   */
  void ParseCode(int code, EoDxfReader& reader);

  /** @brief Calculates the arbitrary extrusion axis (extAxisX and extAxisY) based on the given extrusion direction
   * (extPoint). This follows the DXF specification for handling extrusion directions and their corresponding axes. The
   * calculated axes are unitized for use in extrusion transformations.
   *  @param extrusionDirection The extrusion direction vector from the DXF entity, used to calculate the arbitrary
   * axes.
   */
  void CalculateArbitraryAxis(const EoDxfGeometryBase3d& extrusionDirection);

  /** @brief Applies an extrusion transformation to the given point using the pre-calculated arbitrary axes and the
   * extrusion direction. The transformation is defined as: P' = (Ax * point.x) + (Ay * point.y) + (N * point.z), where
   * Ax and Ay are the arbitrary axes, N is the extrusion direction, and point is the original coordinate. The result is
   * stored back in the provided point reference.
   *
   * @param extrusionDirection The extrusion direction vector (N) used in the transformation.
   * @param[out] point A EoDxfGeometryBase3d representing the original point to be transformed. The transformed
   * coordinates will be stored back in this variable.
   */
  void ExtrudePointInPlace(const EoDxfGeometryBase3d& extrusionDirection, EoDxfGeometryBase3d& point) const noexcept;

 private:
  // Transient cache for ApplyExtrusion() — deliberately NOT copied or cleared.
  // Always recomputed from m_extrusionDirection when needed.
  EoDxfGeometryBase3d extAxisX{};
  EoDxfGeometryBase3d extAxisY{};

 public:
  std::wstring m_layer{L"0"};  // layer name, code 8
  std::wstring m_lineType{L"BYLAYER"};  // line type, code 6
  std::wstring m_proxyEntityGraphicsData{};  // group code 310 (optional) [unused]
  std::wstring m_colorName{};  // Group code 430
  double m_lineTypeScale{1.0};  // Group code 48
  // Soft-pointer ID/handle to owner BLOCK_RECORD object, code 330
  std::uint64_t m_materialHandle{EoDxf::NoHandle};  // hard pointer id to material object, code 347
  std::uint64_t m_plotStyleHandle{EoDxf::NoHandle};  // Group code 390
  enum EoDxfLineWidths::lineWidth m_lineWeight{EoDxfLineWidths::widthByLayer};  // Group code 370
  int m_numberOfBytesInProxyGraphics{};  // Group code 92 (optional) [unused]
  std::int32_t m_color24{-1};  // Group code 420
  std::int16_t m_color{EoDxf::colorByLayer};  // Group code 62
  std::int16_t m_visibilityFlag{0};  // Group code 60 (0 for visible, 1 for invisible)

  EoDxfGeometryBase3d m_extrusionDirection{0.0, 0.0, 1.0};  //  Group codes 210, 220 & 230 (optional)
  double m_thickness{};  // Thickness, code 39
  EoDxf::Space m_space{EoDxf::Space::ModelSpace};  // Group code 67
  EoDxf::ShadowMode m_shadowMode{EoDxf::ShadowMode::CastAndReceiveShadows};  // Group code 284
  EoDxf::TransparencyCodes m_transparency{EoDxf::TransparencyCodes::Opaque};  // Group code 440
  bool m_haveExtrusion{};  // set to true if the entity have extrusion
};

/** @brief Class to handle point entity
 */
class EoDxfPoint : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfPoint(EoDxf::ETYPE entityType = EoDxf::POINT) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_pointLocation{};  //  Group codes 10, 20 & 30
  double m_angleOfXAxis{};  // Group code 50 (optional, in radians)
};

/** @brief Class to handle line entity
 */
class EoDxfLine : public EoDxfGraphic {
  friend class EoDxfRead;

 public:
  explicit EoDxfLine(EoDxf::ETYPE entityType = EoDxf::LINE) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_startPoint;  // Group code 10, 20 & 30
  EoDxfGeometryBase3d m_endPoint;  // Group code 11, 21 & 31
};

/** @brief Class to handle ray entity
 *
 *  A ray is a line that starts at a specific point and extends infinitely in one direction.
 *  It differs from a line entity in that a line has two endpoints,
 *  while a ray has only one starting point and extends infinitely in the direction defined by its second point.
 */
class EoDxfRay : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfRay(EoDxf::ETYPE entityType = EoDxf::RAY) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

  EoDxfGeometryBase3d m_startPoint{};  // Group code 10, 20 & 30
  EoDxfGeometryBase3d m_unitDirectionVector{};  // Group code 11, 21 & 31
};

/** @brief Class to handle xline entity
 *
 *  An xline is an infinite line that extends infinitely in both directions.
 *  It differs from a line entity in that a line has two endpoints,
 *  while an xline extends infinitely in both directions defined by its two points.
 */
class EoDxfXline : public EoDxfGraphic {
  friend class EoDxfRead;

 public:
  explicit EoDxfXline(EoDxf::ETYPE entityType = EoDxf::XLINE) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_startPoint{};  // Group code 10, 20 & 30
  EoDxfGeometryBase3d m_unitDirectionVector{};  // Group code 11, 21 & 31
};

/** @brief Class to handle ACAD_PROXY_ENTITY
 *
 *  An ACAD_PROXY_ENTITY is a placeholder for a custom entity whose defining ObjectARX application is not currently
 *  loaded. AutoCAD preserves the entity's binary data so that it can be round-tripped without loss even when the
 *  application is unavailable. DXF group codes specific to the proxy include the class IDs, graphics/entity data
 *  sizes, binary chunk records (code 310), and object-ID handle references (codes 330, 340, 350, 360).
 */
class EoDxfAcadProxyEntity : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfAcadProxyEntity(EoDxf::ETYPE entityType = EoDxf::ACAD_PROXY_ENTITY) noexcept
      : EoDxfGraphic{entityType} {}
  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  std::int32_t m_proxyEntityClassId{498};  // Group code 90 (always 498 for proxy entities)
  std::int32_t m_applicationEntityClassId{};  // Group code 91 (class ID from CLASSES section, 500-based)
  std::int32_t m_graphicsDataSizeInBytes{};  // Group code 92 (byte count of graphics data in following 310 groups)
  std::int32_t m_entityDataSizeInBits{};  // Group code 93 (bit count of entity data in following 310 groups)
  std::int32_t m_objectIdSectionEnd{};  // Group code 94 (always 0, marks end of object ID section)
  std::int32_t m_objectDrawingFormat{};  // Group code 95 (0 = R13, 14 = R14, ...)
  std::int16_t m_originalDataFormatFlag{};  // Group code 70 (0 = DWG format, 1 = DXF format)
  std::vector<std::wstring> m_graphicsDataChunks;  // Group code 310 (binary chunk records for proxy graphics)
  std::vector<std::wstring> m_entityDataChunks;  // Group code 310 (binary chunk records for entity data)
  std::vector<std::uint64_t> m_softPointerHandles;  // Group code 330 (soft pointer IDs, after entity-level ones)
  std::vector<std::uint64_t> m_hardPointerHandles;  // Group code 340 (hard pointer IDs)
  std::vector<std::uint64_t> m_softOwnerHandles;  // Group code 350 (soft owner IDs)
  std::vector<std::uint64_t> m_hardOwnerHandles;  // Group code 360 (hard owner IDs)

 private:
  bool m_readingGraphicsData{true};  // Tracks whether 310 groups belong to graphics data or entity data
};

/** @brief Class to handle circle entity
 *
 *  A circle is a closed curve where all points are equidistant from a fixed center point.
 *  It is defined by its center point and radius.
 *  The circle entity in DXF can also include additional properties such as thickness and extrusion direction,
 *  which can affect how the circle is rendered in 3D space.
 */
class EoDxfCircle : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfCircle(EoDxf::ETYPE entityType = EoDxf::CIRCLE) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override;

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_centerPoint{};  // Group codes 10, 20 & 30
  double m_radius{};  // Group code 40
};

class EoDxfArc : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfArc(EoDxf::ETYPE entityType = EoDxf::ARC) noexcept : EoDxfGraphic{entityType} {}

  /** @brief Applies extrusion to the arc's center point and adjusts start and end angles if necessary.
   *
   *  If the arc has an extrusion direction defined, this method calculates the arbitrary axis based on the extrusion
   * direction and extrudes the center point of the arc accordingly. Additionally, if the extrusion direction has a z
   * value less than 0, it mirrors the start and end angles of the arc to account for the right-hand rule used in DXF
   * files.
   *
   *  Note: Commenting out the calls to CalculateArbitraryAxis and ExtrudePointInPlace will cause arcs being tested to
   * be located on the other side of the y axis (all x dimensions are negated).
   */
  void ApplyExtrusion() override;

  const EoDxfGeometryBase3d& Center() const { return m_centerPoint; }
  [[nodiscard]] double Radius() const noexcept { return m_radius; }
  [[nodiscard]] double StartAngle() const noexcept { return m_startAngle; }
  [[nodiscard]] double EndAngle() const noexcept { return m_endAngle; }
  [[nodiscard]] const EoDxfGeometryBase3d& ExtrusionDirection() const noexcept { return m_extrusionDirection; }

 protected:
  /** @brief Parses dxf code and value to read arc entity data
   *  @param code dxf code
   *  @param reader pointer to EoDxfReader to read value
   */
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_centerPoint{};  // Group codes 10, 20 & 30
  double m_radius{};  // Group code 40
  double m_startAngle{};  // group code 50 (in radians)
  double m_endAngle{};  // group code 51 (in radians)
  std::int16_t m_isCounterClockwise{1};  // group code 73
};

/** @brief Class to handle ellipse entity
 *
 *  An ellipse is a closed curve that resembles a flattened circle.
 *  It is defined by its center point, major axis, and ratio of minor axis to major axis.
 *  The ellipse entity in DXF can also include additional properties such as thickness and extrusion direction,
 *  which can affect how the ellipse is rendered in 3D space.
 */
class EoDxfEllipse : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfEllipse(EoDxf::ETYPE entityType = EoDxf::ELLIPSE) noexcept : EoDxfGraphic{entityType} {}

  void ToPolyline(EoDxfPolyline* polyline, int parts = 128);
  void ApplyExtrusion() override;

 protected:
  /** @brief Parses dxf code and value to read ellipse entity data
   *  @param code dxf code
   *  @param reader pointer to EoDxfReader to read value
   */
  void ParseCode(int code, EoDxfReader& reader);

 private:
  void CorrectAxis();

 public:
  EoDxfGeometryBase3d m_centerPoint{};  // Group codes 10, 20 & 30
  EoDxfGeometryBase3d m_endPointOfMajorAxis{};  // Group codes 11, 21 & 31 (defines the major axis direction and length)
  double m_ratio{};  // Group code 40
  double m_startParam{};  // Group code 41, 0.0 for full ellipse
  double m_endParam{};  // Group code 42, 2*PI for full ellipse
  std::int16_t m_isCounterClockwise{1};  // Group code 73 (only used in hatch)
};

/** @@brief Class to handle trace entity
 *
 *  A trace is a four-sided polyline that is always planar.
 *  It is defined by its four corner points, which are specified in the DXF file using group codes 10, 20, 30 for the
 * first point; 11, 21, 31 for the second point; 12, 22, 32 for the third point; and 13, 23, 33 for the fourth point.
 *  The trace entity can also include properties such as thickness and extrusion direction, which can affect how it is
 * rendered in 3D space.
 */
class EoDxfTrace : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfTrace(EoDxf::ETYPE entityType = EoDxf::TRACE) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override;

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_firstCorner{};  // Group code 10, 20 & 30
  EoDxfGeometryBase3d m_secondCorner{};  // Group code 11, 21 & 31
  EoDxfGeometryBase3d m_thirdCorner{};  // Group code 12, 22 & 32
  EoDxfGeometryBase3d m_fourthCorner{};  // Group code 13, 23 & 33
};

/** @brief Class to handle solid entity
 *
 *  A solid is a four-sided polyline that is always planar and filled.
 *  It is defined by its four corner points, which are specified in the DXF file using group codes 10, 20, 30 for the
 * first point; 11, 21, 31 for the second point; 12, 22, 32 for the third point; and 13, 23, 33 for the fourth point.
 *  The solid entity can also include properties such as thickness and extrusion direction, which can affect how it is
 * rendered in 3D space.
 */
class EoDxfSolid : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfSolid(EoDxf::ETYPE entityType = EoDxf::SOLID) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override;

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  [[nodiscard]] const EoDxfGeometryBase3d& FirstCorner() const noexcept { return m_firstCorner; }
  [[nodiscard]] const EoDxfGeometryBase3d& SecondCorner() const noexcept { return m_secondCorner; }
  [[nodiscard]] const EoDxfGeometryBase3d& ThirdCorner() const noexcept { return m_thirdCorner; }
  [[nodiscard]] const EoDxfGeometryBase3d& FourthCorner() const noexcept { return m_fourthCorner; }
  [[nodiscard]] double elevation() const noexcept { return m_firstCorner.z; }
  [[nodiscard]] const EoDxfGeometryBase3d& extrusion() const noexcept { return m_extrusionDirection; }

 public:
  EoDxfGeometryBase3d m_firstCorner{};  // Group code 10, 20 & 30
  EoDxfGeometryBase3d m_secondCorner{};  // Group code 11, 21 & 31
  EoDxfGeometryBase3d m_thirdCorner{};  // Group code 12, 22 & 32
  EoDxfGeometryBase3d m_fourthCorner{};  // Group code 13, 23 & 33
};

/** @brief Class to handle 3dFace entity
 *
 *  A 3D face is a three- or four-sided planar surface defined by its corner points.
 *  It is specified in the DXF file using group codes 10, 20, 30 for the first point;
 *  11, 21, 31 for the second point; 12, 22, 32 for the third point; and optionally 13, 23, 33 for the fourth point.
 *  The fourth point is optional because a 3D face can be a triangle (three-sided) or a quadrilateral (four-sided).
 *  The entity can also include properties such as thickness and extrusion direction, which can affect how it is
 * rendered in 3D space.
 */
class EoDxf3dFace : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  enum InvisibleEdgeFlags {
    NoEdge = 0x00,
    FirstEdge = 0x01,
    SecondEdge = 0x02,
    ThirdEdge = 0x04,
    FourthEdge = 0x08,
    AllEdges = 0x0F
  };

  explicit EoDxf3dFace(EoDxf::ETYPE entityType = EoDxf::E3DFACE) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override {}

  [[nodiscard]] const EoDxfGeometryBase3d& FirstCorner() const noexcept { return m_firstCorner; }
  [[nodiscard]] const EoDxfGeometryBase3d& SecondCorner() const noexcept { return m_secondCorner; }
  [[nodiscard]] const EoDxfGeometryBase3d& ThirdCorner() const noexcept { return m_thirdCorner; }
  [[nodiscard]] const EoDxfGeometryBase3d& FourthCorner() const noexcept { return m_fourthCorner; }
  [[nodiscard]] InvisibleEdgeFlags edgeFlags() const noexcept {
    return static_cast<InvisibleEdgeFlags>(m_invisibleFlag);
  }

 protected:
  //! interpret code in dxf reading process or dispatch to inherited class
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_firstCorner{};  // Group code 10, 20 & 30
  EoDxfGeometryBase3d m_secondCorner{};  // Group code 11, 21 & 31
  EoDxfGeometryBase3d m_thirdCorner{};  // Group code 12, 22 & 32
  EoDxfGeometryBase3d m_fourthCorner{};  // Group code 13, 23 & 33
  std::int16_t m_invisibleFlag{};  // Group code 70
};

/** @brief Class to handle block entries
 *
 *  A block is a collection of entities that are grouped together and can be inserted into a drawing as a single unit.
 *  It is defined by its name, which is specified in the DXF file using group code 2, and its type, which is specified
 * using group code 70. The block entity can also include properties such as layer, line type, and extrusion direction,
 * which can affect how it is rendered in the drawing.
 */
class EoDxfBlock : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfBlock(EoDxf::ETYPE entityType = EoDxf::BLOCK) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  std::wstring m_blockName{L"*U0"};  // Group code 2 or 3
  std::int16_t m_blockTypeFlags{};  // Group code 70
  EoDxfGeometryBase3d m_basePoint{};  // Group codes 10, 20 & 30
  std::wstring m_xrefPathName{};  // Group code 1 (optional, only for external reference blocks)
  std::wstring m_blockDescription{};  // Group code 4 (optional)
};

/** @brief Class to handle insert entries
 *
 *  An insert is an instance of a block that is placed in a drawing.
 *  It is defined by the name of the block it references, which is specified in the DXF file using group code 2, and its
 * insertion point, which is specified using group codes 10, 20, and 30. The insert entity can also include properties
 * such as scale factors (code 41, 42, 43), rotation angle (code 50), and number of columns and rows for array inserts
 * (code 70 and 71), which can affect how it is rendered in the drawing.
 */
class EoDxfInsert : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfInsert(EoDxf::ETYPE entityType = EoDxf::INSERT) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  std::wstring m_blockName;  // Group code 2
  EoDxfGeometryBase3d m_insertionPoint;  // Group codes 10, 20 & 30
  double m_xScaleFactor{1.0};  // Group code 41
  double m_yScaleFactor{1.0};  // Group code 42
  double m_zScaleFactor{1.0};  // Group code 43
  double m_rotationAngle{};  // Group code 50 (in radians)
  std::int16_t m_columnCount{1};  // Group code 70
  std::int16_t m_rowCount{1};  // Group code 71
  double m_columnSpacing{};  // Group code 44
  double m_rowSpacing{};  // Group code 45
};

/** @brief Class to handle lightweight polyline entity
 *
 *  A lightweight polyline is a series of connected line segments that can be either open or closed.
 *  It is defined by its vertices, which are specified in the DXF file using group codes 10, 20 for the vertex
 * coordinates, and optionally 30 for elevation. The lightweight polyline entity can also include properties such as
 * width (code 43), flags (code 70), and extrusion direction (code 210, 220, 230), which can affect how it is rendered
 * in the drawing.
 */
class EoDxfLwPolyline : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfLwPolyline(EoDxf::ETYPE entityType = EoDxf::LWPOLYLINE) noexcept : EoDxfGraphic{entityType} {}
  EoDxfLwPolyline(const EoDxfLwPolyline&) = default;
  EoDxfLwPolyline(EoDxfLwPolyline&&) noexcept = default;
  EoDxfLwPolyline& operator=(const EoDxfLwPolyline&) = default;
  EoDxfLwPolyline& operator=(EoDxfLwPolyline&&) noexcept = default;
  ~EoDxfLwPolyline() = default;

  void ApplyExtrusion() override;

  void AddVertex(const EoDxfPolylineVertex2d& vertex) { m_vertices.push_back(vertex); }

  [[nodiscard]] EoDxfPolylineVertex2d& AddVertex() { return m_vertices.emplace_back(); }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  std::int32_t m_numberOfVertices{};  // Group code 90
  std::int16_t m_polylineFlag{};  // Group code 70, (1 = Closed; 128 = Plinegen)
  double m_constantWidth{};  // Group code 43
  double m_elevation{};  // Group code 38
  std::vector<EoDxfPolylineVertex2d> m_vertices;

 private:
  // Index of the current vertex being populated during parsing (-1 = none).
  // Using an index instead of a pointer avoids dangling after vector reallocation.
  int m_currentVertexIndex{-1};
};

/** @brief Class to handle mtext entity
 *
 *  An mtext entity represents multiline text in a drawing.
 *  It is defined by its insertion point (code 10, 20, 30), height (code 40), and the text string itself (code 1).
 *  The mtext entity can also include properties such as rotation angle (code 50), width scale factor (code 41), oblique
 * angle (code 51), and text style name (code 7), which can affect how the text is rendered in the drawing.
 *  Additionally, mtext supports more complex formatting options such as multiple lines of text, different alignments,
 * and special characters.
 */
class EoDxfMText : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  enum class AttachmentPoint : std::int16_t {
    TopLeft = 1,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
  };
  enum class DrawingDirection : std::int16_t { LeftToRight = 1, TopToBottom = 3, ByStyle = 5 };
  enum class LineSpacingStyle : std::int16_t { AtLeast = 1, Exact = 2 };

  explicit EoDxfMText(EoDxf::ETYPE entityType = EoDxf::MTEXT) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_insertionPoint;  // Group codes 10/20/30 (insertion point in OCS)
  double m_nominalTextHeight{1.0};  // Group code 40
  double m_referenceRectangleWidth{};  // Group code 41 (wrapping box width)
  std::wstring m_textStyleName{L"STANDARD"};  // Group code 7
  double m_rotationAngle{};  // Group code 50 (in radians!)
  EoDxfGeometryBase3d m_xAxisDirectionVector{1.0, 0.0, 0.0};  // Group codes 11/21/31

  AttachmentPoint m_attachmentPoint{AttachmentPoint::TopLeft};  // Group code 71
  DrawingDirection m_drawingDirection{DrawingDirection::LeftToRight};  // Group code 72
  LineSpacingStyle m_lineSpacingStyle{LineSpacingStyle::AtLeast};  // Group 73
  double m_lineSpacingFactor{1.0};  // Group 44 (clamped to 0.25, 4.0)

  std::wstring
      m_textString;  // If the text string is less than 250 characters, all characters appear in Group code 1.
                     // If the text string is greater than 250 characters, the string is divided into
                     // 250-character chunks, which appear in one or more group 3 codes.
                     // If group 3 codes are used, the last group is a group 1 and has fewer than 250 characters

  std::uint32_t m_backgroundFillSetting{};  // Group code 90
  double m_fillBoxScale{1.5};  // Group code 45
  std::int16_t m_backgroundFillColor{};  // Group code 63
  std::uint32_t m_backgroundColor{};  // Group code 421
  std::wstring m_backgroundColorName;  // Group code 431 (rare)

  // calculated (often present)
  std::optional<double> m_horizontalWidth{};  // Group code 42
  std::optional<double> m_verticalHeight{};  // Group code 43

 protected:
  void UpdateAngle();  // recalculate angle if 'm_haveXAxisDirection' is true
 private:
  bool m_haveXAxisDirection{};
};

class EoDxfVertex : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfVertex() noexcept : EoDxfGraphic{EoDxf::VERTEX} {}

  EoDxfVertex(double sx, double sy, double sz, double bulge) noexcept : EoDxfGraphic{EoDxf::VERTEX}, m_bulge{bulge} {
    m_locationPoint = {sx, sy, sz};
  }

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_locationPoint{};  // Group codes 10, 20 & 30
  double m_startingWidth{};  // Group code 40
  double m_endingWidth{};  // Group code 41
  double m_bulge{};  // Group code 42

  std::int16_t m_vertexFlags{};  // Group code 70
  double m_curveFitTangentDirection{};  // Group code 50
  std::int16_t m_polyfaceMeshVertexIndex1{};  // Group code 71
  std::int16_t m_polyfaceMeshVertexIndex2{};  // Group code 72
  std::int16_t m_polyfaceMeshVertexIndex3{};  // Group code 73
  std::int16_t m_polyfaceMeshVertexIndex4{};  // Group code 74
  std::int32_t m_identifier{};  // Group code 91
};

/** @brief Class to handle seqend entity
 *
 *  A SEQEND entity is used to indicate the end of a sequence of vertices in a POLYLINE or INSERT entity.
 *  It does not contain any geometric data itself, but serves as a marker to signify the conclusion of the vertex list
 * for the associated entity. The SEQEND entity can inherit properties such as layer and display settings from its
 * owning POLYLINE or INSERT entity, but it does not have its own unique properties.
 */
class EoDxfSeqEnd : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfSeqEnd() noexcept : EoDxfGraphic{EoDxf::SEQEND} {}

  /** @brief Constructs a SEQEND that inherits layer and display properties
   *         from the owning POLYLINE or INSERT entity.
   *  @param owner The entity whose sequence this SEQEND terminates.
   */
  explicit EoDxfSeqEnd(const EoDxfGraphic& owner) : EoDxfGraphic{EoDxf::SEQEND} {
    m_layer = owner.m_layer;
    m_lineType = owner.m_lineType;
    m_color = owner.m_color;
    m_lineWeight = owner.m_lineWeight;
  }

  void ApplyExtrusion() override {}
};

class EoDxfPolyline : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfPolyline() noexcept : EoDxfGraphic{EoDxf::POLYLINE} {}

  ~EoDxfPolyline() {
    for (EoDxfVertex* vertex : m_vertices) { delete vertex; }
  }

  // Prevent double-free from shallow copy of raw-pointer vector
  EoDxfPolyline(const EoDxfPolyline&) = delete;
  EoDxfPolyline& operator=(const EoDxfPolyline&) = delete;

  EoDxfPolyline(EoDxfPolyline&& other) noexcept = default;
  EoDxfPolyline& operator=(EoDxfPolyline&& other) noexcept = default;

  void addVertex(const EoDxfVertex& v) {
    auto* vertex = new EoDxfVertex();
    vertex->m_locationPoint = v.m_locationPoint;
    vertex->m_startingWidth = v.m_startingWidth;
    vertex->m_endingWidth = v.m_endingWidth;
    vertex->m_bulge = v.m_bulge;

    vertex->m_vertexFlags = v.m_vertexFlags;
    vertex->m_curveFitTangentDirection = v.m_curveFitTangentDirection;
    vertex->m_polyfaceMeshVertexIndex1 = v.m_polyfaceMeshVertexIndex1;
    vertex->m_polyfaceMeshVertexIndex2 = v.m_polyfaceMeshVertexIndex2;
    vertex->m_polyfaceMeshVertexIndex3 = v.m_polyfaceMeshVertexIndex3;
    vertex->m_polyfaceMeshVertexIndex4 = v.m_polyfaceMeshVertexIndex4;
    vertex->m_identifier = v.m_identifier;

    m_vertices.push_back(vertex);
  }

  void appendVertex(EoDxfVertex* v) { m_vertices.push_back(v); }

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_polylineElevation{};  // Group codes 10, 20 & 30
  std::int16_t m_polylineFlag{};  // Group code 70
  double m_defaultStartWidth{};  // Group code 40
  double m_defaultEndWidth{};  // Group code 41
  std::int16_t m_polygonMeshVertexCountM{};  // Group code 71
  std::int16_t m_polygonMeshVertexCountN{};  // Group code 72
  std::int16_t m_smoothSurfaceDensityM{};  // Group code 73
  std::int16_t m_smoothSurfaceDensityN{};  // Group code 74
  std::int16_t m_curvesAndSmoothSurfaceType{};  // Group code 75

  std::vector<EoDxfVertex*> m_vertices;  // vertex list
};

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

  [[nodiscard]] const bool IsTangentValid() const noexcept { return m_splineFlag & 0x01; }

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
  std::vector<EoDxfGeometryBase3d*> m_controlPoints;  // Group codes 10, 20 & 30 (one entry per control point)
  std::vector<EoDxfGeometryBase3d*> m_fitPoints;  // Group codes 11, 21 & 31 (one entry per fit point)

 private:
  EoDxfGeometryBase3d* m_controlPoint{};  // current control point to add data
  EoDxfGeometryBase3d* m_fitPoint{};  // current fit point to add data
};

/** @brief Class to handle hatch loop
 *
 *  A hatch loop represents a closed boundary path that defines the area to be filled with a hatch pattern.
 *  It is defined by its type (code 92) and the number of edges (code 93). The hatch loop can include various
 * entities such as lines, arcs, circles, ellipses, splines, and lightweight polylines that make up the boundary of the
 * hatch.
 */
class EoDxfHatchLoop {
 public:
  explicit EoDxfHatchLoop(int boundaryPathType) : m_boundaryPathType{boundaryPathType} {}

  ~EoDxfHatchLoop() = default;

  EoDxfHatchLoop(const EoDxfHatchLoop&) = delete;
  EoDxfHatchLoop& operator=(const EoDxfHatchLoop&) = delete;
  EoDxfHatchLoop(EoDxfHatchLoop&&) = delete;
  EoDxfHatchLoop& operator=(EoDxfHatchLoop&&) = delete;

  void Update() { m_numberOfEdges = static_cast<int>(m_entities.size()); }

 public:
  std::int32_t m_boundaryPathType{};  // Group code 92
  std::int32_t m_numberOfEdges{};  // Group code 93

  std::vector<std::unique_ptr<EoDxfGraphic>> m_entities;
};

/** @brief Class to handle image entity
 *
 *  An image entity represents a raster image that is embedded in a drawing.
 *  It is defined by its reference to an image definition object (code 340), the V-vector of a single pixel (code 12,
 * 22, 32), the size of the image in pixels (code 13 and 23), and the z coordinate (code 33). The image entity can also
 * include properties such as clipping state (code 280), brightness (code 281), contrast (code 282), and fade (code
 * 283), which can affect how the image is rendered in the drawing.
 */
class EoDxfImage : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfImage(EoDxf::ETYPE entityType = EoDxf::IMAGE) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  std::uint64_t m_imageDefinitionHandle{};  // Hard reference to imagedef object, code 340
  EoDxfGeometryBase3d m_insertionPoint;  // Insertion point, code 10, 20 & 30
  EoDxfGeometryBase3d m_uVector;  // Group codes 11, 21 & 31
  EoDxfGeometryBase3d m_vVector;  // Group codes 12, 22 & 32
  double m_uImageSizeInPixels{};  // Group code 13
  double m_vImageSizeInPixels{};  // Group code 23
  std::int16_t m_clippingState{};  // Group code 280, 0=off 1=on
  std::int16_t m_brightnessValue{50};  // Group code 281, (0-100)
  std::int16_t m_contrastValue{50};  // Group code 282, (0-100)
  std::int16_t m_fadeValue{};  // Group code 283, (0-100)
};

/** @brief Class to handle leader entity
 *
 *  A leader entity represents a line or curve that connects an annotation to a feature in a drawing.
 *  It is defined by its dimension style (code 3), arrowhead flag (code 71), leader path type (code 72), and leader
 * creation flag (code 73). The leader entity can also include properties such as the hook line direction flag (code
 * 74), hook line flag (code 75), text annotation height (code 40), text annotation width (code 41), number of vertices
 * (code 76), color to use if leader's DIMCLRD = BYBLOCK (code 77), hard reference to associated annotation (code 340),
 * extrusion direction (code 210, 220, 230), "horizontal" direction for leader (code 211, 221, 231), offset of last
 * leader vertex from block (code 212, 222, 232), and offset of last leader vertex from annotation (code 213, 223, 233).
 * The geometry of the leader is defined by a list of vertices (code 10, 20, 30).
 */
class EoDxfLeader : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfLeader() noexcept : EoDxfGraphic{EoDxf::LEADER} {}

  EoDxfLeader(const EoDxfLeader&) = delete;
  EoDxfLeader& operator=(const EoDxfLeader&) = delete;

  EoDxfLeader(EoDxfLeader&&) noexcept = default;
  EoDxfLeader& operator=(EoDxfLeader&&) noexcept = default;

  ~EoDxfLeader() = default;

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  std::wstring m_dimensionStyleName{};  // Group code 3
  std::int16_t m_arrowheadFlag{1};  // Group code 71, 0=Disabled; 1=Enabled
  std::int16_t m_leaderPathType{};  // Group code 72, 0=Straight line segments; 1=Spline
  std::int16_t m_leaderCreationFlag{3};  // Group code 73
  std::int16_t m_hookLineDirection{1};  // Hook line direction flag, code 74, default 1
  std::int16_t m_hookLineFlag{};  // Hook line flag, code 75
  double m_textAnnotationHeight{};  // Group code 40
  double m_textAnnotationWidth{};  // Group code 41
  std::int16_t m_numberOfVertices{};  // Number of vertices, code 76

  std::vector<EoDxfGeometryBase3d> m_vertexList;  // vertex points list, code 10, 20 & 30

  std::int16_t m_colorToUse{};  // Group code 77
  std::uint64_t m_associatedAnnotationHandle{};  // Group code 340
  EoDxfGeometryBase3d m_normalVector{0.0, 0.0, 1.0};  // Group codes 210, 220 & 230
  EoDxfGeometryBase3d m_horizontalDirectionForLeader{1.0, 0.0, 0.0};  // Group codes 211, 221 & 231
  EoDxfGeometryBase3d m_offsetFromBlockInsertionPoint;  // Group codes 212, 222 & 232
  EoDxfGeometryBase3d m_offsetFromAnnotationPlacementPoint;  // Group codes 213, 223 & 233

 private:
  int m_currentVertexIndex{-1};
};

/** @brief A single leader line within an MLEADER leader branch.
 *
 *  Delimited in DXF by group code 304 "LEADER_LINE{" and 305 "}".
 *  Contains vertex points (code 10, 20, 30 — repeated) and an
 *  index (code 91) that identifies the line within its branch.
 */
struct EoDxfMLeaderLine {
  std::vector<EoDxfGeometryBase3d> m_vertices;  // code 10, 20, 30 (repeated)
  std::int32_t m_leaderLineIndex{};  // code 91
  std::int32_t m_leaderLineColorOverride{EoDxf::colorByLayer};  // code 92 (optional)
  std::uint64_t m_leaderLineTypeHandle{EoDxf::NoHandle};  // code 340 (optional)
  std::int16_t m_leaderLineWeightOverride{-1};  // code 171 (optional, -1 = no override)
  double m_arrowheadSize{};  // code 40 (optional, 0 = use context default)
  std::uint64_t m_arrowheadHandle{EoDxf::NoHandle};  // code 341 (optional)
};

/** @brief A leader branch within an MLEADER entity.
 *
 *  Delimited in DXF by group code 302 "LEADER{" and 303 "}".
 *  Each branch has a last leader line point, a dogleg vector,
 *  a dogleg length, and one or more leader lines.
 */
struct EoDxfMLeaderBranch {
  bool m_hasSetLastLeaderLinePoint{};  // code 290
  bool m_hasSetDoglegVector{};  // code 291
  EoDxfGeometryBase3d m_lastLeaderLinePoint;  // code 10, 20, 30
  EoDxfGeometryBase3d m_doglegVector;  // code 11, 21, 31
  std::int32_t m_leaderBranchIndex{};  // code 90
  double m_doglegLength{};  // code 40
  std::vector<EoDxfMLeaderLine> m_leaderLines;
};

/** @brief Context data for an MLEADER entity.
 *
 *  Delimited in DXF by group code 300 "CONTEXT_DATA{" and 301 "}".
 *  Contains the overall layout parameters plus nested LEADER branches
 *  and optional MText / Block content fields.
 */
struct EoDxfMLeaderContextData {
  // --- General context ---
  double m_contentScale{1.0};  // code 40
  EoDxfGeometryBase3d m_contentBasePoint;  // code 10, 20, 30
  double m_textHeight{};  // code 41
  double m_arrowheadSize{};  // code 140
  double m_landingGap{};  // code 145
  std::int16_t m_textLeftAttachment{1};  // code 174
  std::int16_t m_textRightAttachment{1};  // code 175
  std::int16_t m_textAlignmentType{};  // code 176
  std::int16_t m_blockContentConnectionType{};  // code 177
  bool m_hasMText{};  // code 290
  bool m_hasContent{true};  // code 296

  // --- MText content (delimited by code 304 "{" ... 305 "}") ---
  EoDxfGeometryBase3d m_textLocation;  // code 12, 22, 32
  EoDxfGeometryBase3d m_textDirection;  // code 13, 23, 33
  double m_textRotation{};  // code 42
  double m_textWidth{};  // code 43
  double m_textDefinedWidth{};  // code 44
  double m_textDefinedHeight{};  // code 45
  std::int16_t m_textLineSpacingStyle{1};  // code 171
  double m_textLineSpacingFactor{1.0};  // code 141
  std::int32_t m_textFlowDirection{1};  // code 90
  std::int32_t m_textColor{EoDxf::colorByLayer};  // code 91
  std::int16_t m_textAttachment{1};  // code 170
  std::int16_t m_textBackgroundFill{};  // code 172
  std::uint64_t m_textStyleHandle{EoDxf::NoHandle};  // code 340
  std::wstring m_textString;  // code 1 (and 3 for overflow)

  // --- Block content ---
  std::uint64_t m_blockContentHandle{EoDxf::NoHandle};  // code 341
  EoDxfGeometryBase3d m_blockContentNormalDirection{0.0, 0.0, 1.0};  // code 14, 24, 34
  EoDxfGeometryBase3d m_blockContentScale{1.0, 1.0, 1.0};  // code 15, 25, 35
  double m_blockContentRotation{};  // code 46
  std::int32_t m_blockContentColor{EoDxf::colorByLayer};  // code 93

  // --- Leader branches ---
  std::vector<EoDxfMLeaderBranch> m_leaders;
};

/** @brief Class to handle viewport entity
 *
 *  A viewport entity represents a window in paper space that displays a view of the model space.
 *  It is defined by its width and height in paper space units (code 40 and 41), viewport status (code 68), and viewport
 * ID (code 69). The viewport entity can also include properties such as the view center point (code 12 and 22), snap
 * base point (code 13 and 23), snap spacing (code 14 and 24), grid spacing (code 15 and 25), view direction vector
 * (code 16, 26, and 36), view target point (code 17, 27, and 37), perspective lens length (code 42), front clip plane Z
 * value (code 43), back clip plane Z value (code 44), view height in model space units (code 45), snap angle (code 50),
 * and view twist angle (code 51), which can affect how the viewport displays the model space.
 */
class EoDxfViewport : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfViewport(EoDxf::ETYPE entityType = EoDxf::VIEWPORT) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_centerPoint{};  // Group codes 10, 20 & 30 (in DCS)
  double m_width{205.0};  // Group code 40
  double m_height{156.0};  // Group code 41
  std::int16_t m_viewportStatus{};  // Groupcode 68
  std::int16_t m_viewportId{};  // Viewport ID, code 69
  EoDxfGeometryBase2d m_viewCenter{128.5, 97.5};  // Group codes 12 and 22 (in DCS)
  EoDxfGeometryBase2d m_snapBasePoint{};  // Group codes 13 and 23
  EoDxfGeometryBase2d m_snapSpacing{};  // Group codes 14 and 24
  EoDxfGeometryBase2d m_gridSpacing{};  // Group codes 15 and 25
  EoDxfGeometryBase3d m_viewDirection{0.0, 0.0, 1.0};  // View direction from target point (WCS), code 16, 26, 36
  EoDxfGeometryBase3d m_viewTargetPoint;  // View target point (WCS), code 17, 27, 37
  double m_lensLength{};  // Perspective lens length, code 42
  double m_frontClipPlane{};  // Front clip plane Z value, code 43
  double m_backClipPlane{};  // Back clip plane Z value, code 44
  double m_viewHeight{};  // View height in model space units, code 45
  double m_snapAngle{};  // Snap angle, code 50
  double m_twistAngle{};  // View twist angle, code 51

 private:
  std::int32_t m_viewportStatusBitCodedFlags{};  // Group code 90
};