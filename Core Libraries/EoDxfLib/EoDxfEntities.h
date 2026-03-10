#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfReader.h"

class EoDxfPolyline;

namespace EoDxf {

//! Entity's type.
enum ETYPE {
  E3DFACE,
  //        E3DSOLID, //encripted propietry data
  //        ACAD_PROXY_ENTITY,
  ARC,
  //        ATTDEF,
  //        ATTRIB,
  BLOCK,  // and ENDBLK
  //        BODY, //encripted propietry data
  CIRCLE,
  DIMENSION,
  DIMALIGNED,
  DIMLINEAR,
  DIMRADIAL,
  DIMDIAMETRIC,
  DIMANGULAR,
  DIMANGULAR3P,
  DIMORDINATE,
  ELLIPSE,
  HATCH,
  //        HELIX,
  IMAGE,
  INSERT,
  LEADER,
  //        LIGHT,
  LINE,
  LWPOLYLINE,
  //        MESH,
  //        MLINE,
  //        MLEADERSTYLE,
  MLEADER,
  MTEXT,
  //        OLEFRAME,
  //        OLE2FRAME,
  POINT,
  POLYLINE,
  RAY,
  //        REGION, //encripted propietry data
  //        SECTION,
  SEQEND,
  //        SHAPE,
  SOLID,
  SPLINE,
  //        SUN,
  //        SURFACE, //encripted propietry data can be four types
  //        TABLE,
  TEXT,
  //        TOLERANCE,
  TRACE,
  UNDERLAY,
  VERTEX,
  VIEWPORT,
  //        WIPEOUT, //WIPEOUTVARIABLE
  XLINE,
  UNKNOWN
};

}  // namespace EoDxf

/** @brief Base class for entities
 */
class EoDxfEntity {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfEntity() = default;

 protected:
  explicit EoDxfEntity(EoDxf::ETYPE entityType) noexcept : m_entityType{entityType} {}

 public:
  EoDxfEntity(const EoDxfEntity& other);
  EoDxfEntity& operator=(const EoDxfEntity& other);

  EoDxfEntity(EoDxfEntity&&) noexcept = default;
  EoDxfEntity& operator=(EoDxfEntity&&) noexcept = default;

  virtual ~EoDxfEntity();

  void Clear();

  virtual void ApplyExtrusion() = 0;

 protected:
  /** @brief Parses dxf code and value to read entity data
   *  @param code dxf code
   *  @param reader pointer to EoDxfReader to read value
   */
  void ParseCode(int code, EoDxfReader* reader);

  /** @brief Parses application-defined group (code 102) and its associated data until the closing tag is reached.
   *  @param reader pointer to EoDxfReader to read value
   *  @return true if group is successfully parsed, false if group is not recognized or an error occurs
   */
  bool ParseAppDataGroup(EoDxfReader* reader);

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
  std::list<std::list<EoDxfGroupCodeValuesVariant>> m_appData{};  // list of application data, code 102
  std::vector<EoDxfGroupCodeValuesVariant*> m_extendedData{};  // codes 1000 to 1071
  std::string m_layer{"0"};  // layer name, code 8
  std::string m_lineType{"BYLAYER"};  // line type, code 6
  std::string m_proxyEntityGraphicsData{};  // group code 310 (optional) [unused]
  std::string m_colorName{};  // group code 430
  double m_lineTypeScale{1.0};  // linetype scale, code 48
  enum EoDxf::ETYPE m_entityType{EoDxf::UNKNOWN};  // entity type, code 0
  std::uint64_t m_handle{EoDxf::NoHandle};  // entity identifier, code 5
  // Soft-pointer ID/handle to owner BLOCK_RECORD object, code 330
  std::uint64_t m_ownerHandle{EoDxf::NoHandle};
  std::uint64_t m_materialHandle{EoDxf::NoHandle};  // hard pointer id to material object, code 347
  std::int16_t m_color{EoDxf::colorByLayer};  // entity color, code 62
  enum EoDxfLineWidths::lineWidth m_lineWeight{EoDxfLineWidths::widthByLayer};  // entity lineweight, code 370
  int m_numberOfBytesInProxyGraphics{};  // group code 92 (optional) [unused]
  int m_color24{-1};  // 24-bit color, code 420
  EoDxf::TransparencyCodes m_transparency{EoDxf::TransparencyCodes::Opaque};  // group code 440
  std::uint64_t m_plotStyleHandle{EoDxf::NoHandle};  // hard pointer id to plot style object, code 390
  EoDxf::ShadowMode m_shadowMode{EoDxf::ShadowMode::CastAndReceiveShadows};  // group code 284
  EoDxf::Space m_space{EoDxf::Space::ModelSpace};  // space indicator, code 67
  bool m_visible{true};  // entity visibility, code 60
  bool m_haveExtrusion{};  // set to true if the entity have extrusion
 private:
  void clearExtendedData() noexcept;

  EoDxfGroupCodeValuesVariant* m_currentVariant{};
};

/** @brief Class to handle point entity
 */
class EoDxfPoint : public EoDxfEntity {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfPoint(EoDxf::ETYPE entityType = EoDxf::POINT) noexcept : EoDxfEntity{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  EoDxfGeometryBase3d m_firstPoint{};  //  base point, code 10, 20 & 30
  double m_thickness{};  // Thickness, code 39
  //  Extrusion direction, code 210, 220 & 230 (optional, default 0,0,1)
  EoDxfGeometryBase3d m_extrusionDirection{0.0, 0.0, 1.0};
  //  Angle of the X axis for the UCS in effect when the point was drawn, code 50 (optional, default 0.0)
  double m_angleX{};
};

/** @brief Class to handle line entity
 */
class EoDxfLine : public EoDxfPoint {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfLine(EoDxf::ETYPE entityType = EoDxf::LINE) noexcept : EoDxfPoint{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  EoDxfGeometryBase3d m_secondPoint;  // Group code 11, 21 & 31
};

/** @brief Class to handle ray entity
 *
 *  A ray is a line that starts at a specific point and extends infinitely in one direction.
 *  It differs from a line entity in that a line has two endpoints,
 *  while a ray has only one starting point and extends infinitely in the direction defined by its second point.
 */
class EoDxfRay : public EoDxfLine {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfRay(EoDxf::ETYPE entityType = EoDxf::RAY) noexcept : EoDxfLine{entityType} {}
};

/** @brief Class to handle xline entity
 *
 *  An xline is an infinite line that extends infinitely in both directions.
 *  It differs from a line entity in that a line has two endpoints,
 *  while an xline extends infinitely in both directions defined by its two points.
 */
class EoDxfXline : public EoDxfRay {
 public:
  explicit EoDxfXline(EoDxf::ETYPE entityType = EoDxf::XLINE) noexcept : EoDxfRay{entityType} {}
};

/** @brief Class to handle circle entity
 *
 *  A circle is a closed curve where all points are equidistant from a fixed center point.
 *  It is defined by its center point and radius.
 *  The circle entity in DXF can also include additional properties such as thickness and extrusion direction,
 *  which can affect how the circle is rendered in 3D space.
 */
class EoDxfCircle : public EoDxfPoint {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfCircle(EoDxf::ETYPE entityType = EoDxf::CIRCLE) noexcept : EoDxfPoint{entityType} {}

  void ApplyExtrusion() override;

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  double m_radius{};  // Radius, code 40
};

class EoDxfArc : public EoDxfCircle {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfArc(EoDxf::ETYPE entityType = EoDxf::ARC) noexcept : EoDxfCircle{entityType} {}

  void ApplyExtrusion() override;

  // center point in OCS
  const EoDxfGeometryBase3d& Center() const { return m_firstPoint; }
  // the radius of the circle
  [[nodiscard]] double Radius() const noexcept { return m_radius; }
  // start angle in radians
  [[nodiscard]] double StartAngle() const noexcept { return m_startAngle; }
  // end angle in radians
  [[nodiscard]] double EndAngle() const noexcept { return m_endAngle; }
  // thickness
  [[nodiscard]] double Thickness() const noexcept { return m_thickness; }
  // extrusion
  [[nodiscard]] const EoDxfGeometryBase3d& ExtrusionDirection() const noexcept { return m_extrusionDirection; }

 protected:
  // interpret code in dxf reading process or dispatch to inherited class
  void ParseCode(int code, EoDxfReader* reader);

 public:
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
class EoDxfEllipse : public EoDxfLine {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfEllipse(EoDxf::ETYPE entityType = EoDxf::ELLIPSE) noexcept : EoDxfLine{entityType} {}

  void ToPolyline(EoDxfPolyline* polyline, int parts = 128);
  void ApplyExtrusion() override;

 protected:
  /** @brief Parses dxf code and value to read ellipse entity data
   *  @param code dxf code
   *  @param reader pointer to EoDxfReader to read value
   */
  void ParseCode(int code, EoDxfReader* reader);

 private:
  void CorrectAxis();

 public:
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
class EoDxfTrace : public EoDxfLine {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfTrace(EoDxf::ETYPE entityType = EoDxf::TRACE) noexcept : EoDxfLine{entityType} {}

  void ApplyExtrusion() override;

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  EoDxfGeometryBase3d m_thirdPoint{};  // Group code 12, 22 & 32
  EoDxfGeometryBase3d m_fourthPoint{};  // Group code 13, 23 & 33
};

/** @brief Class to handle solid entity
 *
 *  A solid is a four-sided polyline that is always planar and filled.
 *  It is defined by its four corner points, which are specified in the DXF file using group codes 10, 20, 30 for the
 * first point; 11, 21, 31 for the second point; 12, 22, 32 for the third point; and 13, 23, 33 for the fourth point.
 *  The solid entity can also include properties such as thickness and extrusion direction, which can affect how it is
 * rendered in 3D space.
 */
class EoDxfSolid : public EoDxfTrace {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfSolid() { m_entityType = EoDxf::SOLID; }

 protected:
  //! interpret code in dxf reading process or dispatch to inherited class
  void ParseCode(int code, EoDxfReader* reader);

 public:
  [[nodiscard]] const EoDxfGeometryBase3d& FirstCorner() const noexcept { return m_firstPoint; }
  [[nodiscard]] const EoDxfGeometryBase3d& SecondCorner() const noexcept { return m_secondPoint; }
  [[nodiscard]] const EoDxfGeometryBase3d& ThirdCorner() const noexcept { return m_thirdPoint; }
  [[nodiscard]] const EoDxfGeometryBase3d& FourthCorner() const noexcept { return m_fourthPoint; }
  [[nodiscard]] double thick() const noexcept { return m_thickness; }
  [[nodiscard]] double elevation() const noexcept { return m_firstPoint.z; }
  [[nodiscard]] const EoDxfGeometryBase3d& extrusion() const noexcept { return m_extrusionDirection; }
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
class EoDxf3dFace : public EoDxfTrace {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  enum InvisibleEdgeFlags {
    NoEdge = 0x00,
    FirstEdge = 0x01,
    SecodEdge = 0x02,
    ThirdEdge = 0x04,
    FourthEdge = 0x08,
    AllEdges = 0x0F
  };

  explicit EoDxf3dFace(EoDxf::ETYPE entityType = EoDxf::E3DFACE) noexcept : EoDxfTrace{entityType} {}

  void ApplyExtrusion() override {}

  [[nodiscard]] const EoDxfGeometryBase3d& FirstCorner() const noexcept { return m_firstPoint; }
  [[nodiscard]] const EoDxfGeometryBase3d& SecondCorner() const noexcept { return m_secondPoint; }
  [[nodiscard]] const EoDxfGeometryBase3d& ThirdCorner() const noexcept { return m_thirdPoint; }
  [[nodiscard]] const EoDxfGeometryBase3d& FourthCorner() const noexcept { return m_fourthPoint; }
  [[nodiscard]] InvisibleEdgeFlags edgeFlags() const noexcept {
    return static_cast<InvisibleEdgeFlags>(m_invisibleFlag);
  }

 protected:
  //! interpret code in dxf reading process or dispatch to inherited class
  void ParseCode(int code, EoDxfReader* reader);

 public:
  std::int16_t m_invisibleFlag{};  // Group code 70
};

/** @brief Class to handle block entries
 *
 *  A block is a collection of entities that are grouped together and can be inserted into a drawing as a single unit.
 *  It is defined by its name, which is specified in the DXF file using group code 2, and its type, which is specified
 * using group code 70. The block entity can also include properties such as layer, line type, and extrusion direction,
 * which can affect how it is rendered in the drawing.
 */
class EoDxfBlock : public EoDxfPoint {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfBlock(EoDxf::ETYPE entityType = EoDxf::BLOCK) noexcept : EoDxfPoint{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  std::string name{"*U0"};  // Group code 2
  std::int16_t m_flags{};  // Group code 70
};

/** @brief Class to handle insert entries
 *
 *  An insert is an instance of a block that is placed in a drawing.
 *  It is defined by the name of the block it references, which is specified in the DXF file using group code 2, and its
 * insertion point, which is specified using group codes 10, 20, and 30. The insert entity can also include properties
 * such as scale factors (code 41, 42, 43), rotation angle (code 50), and number of columns and rows for array inserts
 * (code 70 and 71), which can affect how it is rendered in the drawing.
 */
class EoDxfInsert : public EoDxfPoint {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfInsert(EoDxf::ETYPE entityType = EoDxf::INSERT) noexcept : EoDxfPoint{entityType} {}

  void ApplyExtrusion() override { EoDxfPoint::ApplyExtrusion(); }

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  std::string m_blockName;  // Group code 2
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
class EoDxfLwPolyline : public EoDxfEntity {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfLwPolyline(EoDxf::ETYPE entityType = EoDxf::LWPOLYLINE) noexcept : EoDxfEntity{entityType} {}
  EoDxfLwPolyline(const EoDxfLwPolyline&) = default;
  EoDxfLwPolyline(EoDxfLwPolyline&&) noexcept = default;
  EoDxfLwPolyline& operator=(const EoDxfLwPolyline&) = default;
  EoDxfLwPolyline& operator=(EoDxfLwPolyline&&) noexcept = default;
  ~EoDxfLwPolyline() = default;

  void ApplyExtrusion() override;

  void AddVertex(const EoDxfPolylineVertex2d& vertex) { m_vertices.push_back(vertex); }

  [[nodiscard]] EoDxfPolylineVertex2d& AddVertex() { return m_vertices.emplace_back(); }

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  int m_numberOfVertices{};  // Group code 90
  std::int16_t m_polylineFlag{};  // Group code 70, (1 = Closed; 128 = Plinegen)
  double m_constantWidth{};  // Group code 43
  double m_elevation{};  // Group code 38
  double m_thickness{};  // Group code 39
  EoDxfGeometryBase3d m_extrusionDirection{0.0, 0.0, 1.0};  //  code 210, 220 & 230
  std::vector<EoDxfPolylineVertex2d> m_vertices;

 private:
  // Index of the current vertex being populated during parsing (-1 = none).
  // Using an index instead of a pointer avoids dangling after vector reallocation.
  int m_currentVertexIndex{-1};
};

/** @brief Class to handle text entity
 *
 *  A text entity represents a single line of text in a drawing.
 *  It is defined by its insertion point (code 10, 20, 30), height (code 40), and the text string itself (code 1).
 *  The text entity can also include properties such as rotation angle (code 50), width scale factor (code 41), oblique
 * angle (code 51), and text style name (code 7), which can affect how the text is rendered in the drawing.
 */
class EoDxfText : public EoDxfLine {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  enum VAlign { BaseLine = 0, Bottom, Middle, Top };

  enum HAlign { Left = 0, Center, Right, AlignedIfBaseLine, MiddleIfBaseLine, FitIfBaseLine };

  explicit EoDxfText(EoDxf::ETYPE entityType = EoDxf::TEXT) noexcept : EoDxfLine{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  double m_textHeight{};  // Group code 40
  std::string m_string;  // Group code 1
  double m_textRotation{};  // Group code 50
  double m_scaleFactorWidth{1.0};  // Group code 41
  double m_obliqueAngle{};  // Group code 51
  std::string m_textStyleName{"STANDARD"};  // Group code 7
  std::int16_t m_textGenerationFlags{};  // Group code 71
  enum HAlign m_horizontalAlignment { Left };  // Group code 72
  enum VAlign m_verticalAlignment { BaseLine };  // Group code 73
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
class EoDxfMText : public EoDxfText {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  enum Attach {
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

  explicit EoDxfMText(EoDxf::ETYPE entityType = EoDxf::MTEXT) noexcept : EoDxfText{entityType} {
    m_verticalAlignment = (VAlign)TopLeft;
    m_textGenerationFlags = 1;
  }

 protected:
  void ParseCode(int code, EoDxfReader* reader);
  void UpdateAngle();  // recalculate angle if 'm_haveXAxisDirection' is true

 public:
  double m_lineSpacingFactor{1.0};  // Group code 44 (optional)
 private:
  bool m_haveXAxisDirection{};
};

class EoDxfVertex : public EoDxfPoint {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfVertex() noexcept : EoDxfPoint{EoDxf::VERTEX} {}

  EoDxfVertex(double sx, double sy, double sz, double bulge) noexcept : EoDxfPoint{EoDxf::VERTEX}, m_bulge{bulge} {
    m_firstPoint = {sx, sy, sz};
  }

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  double m_startingWidth{};  // Group code 40
  double m_endingWidth{};  // Group code 41
  double m_bulge{};  // Group code 42

  std::int16_t m_vertexFlags{};  // Group code 70
  double m_curveFitTangentDirection{};  // Group code 50
  std::int16_t m_polyfaceMeshVertexIndex1{};  // Group code 71
  std::int16_t m_polyfaceMeshVertexIndex2{};  // Group code 72
  std::int16_t m_polyfaceMeshVertexIndex3{};  // Group code 73
  std::int16_t m_polyfaceMeshVertexIndex4{};  // Group code 74
  int m_identifier{};  // Group code 91
};

/** @brief Class to handle seqend entity
 *
 *  A SEQEND entity is used to indicate the end of a sequence of vertices in a POLYLINE or INSERT entity.
 *  It does not contain any geometric data itself, but serves as a marker to signify the conclusion of the vertex list
 * for the associated entity. The SEQEND entity can inherit properties such as layer and display settings from its
 * owning POLYLINE or INSERT entity, but it does not have its own unique properties.
 */
class EoDxfSeqEnd : public EoDxfEntity {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfSeqEnd() noexcept : EoDxfEntity{EoDxf::SEQEND} {}

  /** @brief Constructs a SEQEND that inherits layer and display properties
   *         from the owning POLYLINE or INSERT entity.
   *  @param owner The entity whose sequence this SEQEND terminates.
   */
  explicit EoDxfSeqEnd(const EoDxfEntity& owner) noexcept : EoDxfEntity{EoDxf::SEQEND} {
    m_layer = owner.m_layer;
    m_lineType = owner.m_lineType;
    m_color = owner.m_color;
    m_lineWeight = owner.m_lineWeight;
  }

  void ApplyExtrusion() override {}
};

class EoDxfPolyline : public EoDxfPoint {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfPolyline() noexcept : EoDxfPoint{EoDxf::POLYLINE} {}

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
    vertex->m_firstPoint = v.m_firstPoint;
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

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
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
class EoDxfSpline : public EoDxfEntity {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfSpline() noexcept : EoDxfEntity{EoDxf::SPLINE} {}

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

 protected:
  void ParseCode(int code, EoDxfReader* reader);

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
  int m_boundaryPathType{};  // Group code 92
  int m_numberOfEdges{};  // Group code 93

  std::vector<std::unique_ptr<EoDxfEntity>> m_entities;
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
class EoDxfHatch : public EoDxfPoint {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfHatch() : EoDxfPoint{EoDxf::HATCH} {}

  ~EoDxfHatch() {
    for (auto* hatchLoop : m_hatchLoops) { delete hatchLoop; }
  }

  EoDxfHatch(const EoDxfHatch&) = delete;
  EoDxfHatch& operator=(const EoDxfHatch&) = delete;
  EoDxfHatch(EoDxfHatch&&) = delete;
  EoDxfHatch& operator=(EoDxfHatch&&) = delete;

  void AppendLoop(EoDxfHatchLoop* hatchLoop) { m_hatchLoops.push_back(hatchLoop); }

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  std::string m_hatchPatternName;  // Group code 2
  std::int16_t m_solidFillFlag{1};  // Group code 70
  std::int16_t m_associativityFlag{};  // Group code 71
  std::int16_t m_hatchStyle{};  // Group code 75
  std::int16_t m_hatchPatternType{1};  // Group code 76
  std::int16_t m_hatchPatternDoubleFlag{};  // Group code 77
  int m_numberOfBoundaryPaths{};  // Group code 91
  double m_hatchPatternAngle{};  // Group code 52
  double m_hatchPatternScaleOrSpacing{};  // Group code 41
  std::int16_t m_numberOfPatternDefinitionLines{};  //   Group code 78

  std::vector<EoDxfHatchLoop*> m_hatchLoops;

 private:
  void ClearEntities() noexcept;

  void AddLine();

  void AddArc();

  void AddEllipse();

  void AddSpline();

  EoDxfHatchLoop* m_hatchLoop{};  // current loop to add data
  EoDxfLine* m_line{};
  EoDxfArc* m_arc{};
  EoDxfEllipse* m_ellipse{};
  EoDxfSpline* m_spline{};
  EoDxfLwPolyline* m_polyline{};
  EoDxfPoint* m_point{};
  EoDxfPolylineVertex2d* m_polylineVertex{};
  bool m_isPolyline{};
};

/** @brief Class to handle image entity
 *
 *  An image entity represents a raster image that is embedded in a drawing.
 *  It is defined by its reference to an image definition object (code 340), the V-vector of a single pixel (code 12,
 * 22, 32), the size of the image in pixels (code 13 and 23), and the z coordinate (code 33). The image entity can also
 * include properties such as clipping state (code 280), brightness (code 281), contrast (code 282), and fade (code
 * 283), which can affect how the image is rendered in the drawing.
 */
class EoDxfImage : public EoDxfLine {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfImage() {
    m_entityType = EoDxf::IMAGE;
    fade = clip = 0;
    brightness = contrast = 50;
  }

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  std::uint64_t m_imageDefinitionHandle{};  // Hard reference to imagedef object, code 340
  EoDxfGeometryBase3d vVector;  // V-vector of single pixel, x coordinate, code 12, 22 & 32
  double sizeu{};  // image size in pixels, U value, code 13
  double sizev{};  // image size in pixels, V value, code 23
  double dz{};  // z coordinate, code 33
  int clip;  // Clipping state, code 280, 0=off 1=on
  int brightness;  // Brightness value, code 281, (0-100) default 50
  int contrast;  // Brightness value, code 282, (0-100) default 50
  int fade;  // Brightness value, code 283, (0-100) default 0
};

/** @brief Class to handle dimension entity
 *
 *  A dimension entity represents a measurement annotation in a drawing.
 *  It is defined by its type (code 70), definition point (code 10, 20, 30), and text point (code 11, 21, 31).
 *  The dimension entity can also include properties such as the dimension style (code 3), attachment point (code 71),
 * text line spacing style (code 72), dimension text (code 1), text line spacing factor (code 41), rotation angle of
 * the dimension text (code 53), and extrusion direction (code 210, 220, 230), which can affect how it is rendered in
 * the drawing.
 */
class EoDxfDimension : public EoDxfEntity {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfDimension() {
    m_entityType = EoDxf::DIMENSION;
    m_dimensionType = 0;
    m_dimensionTextLineSpacingStyle = 1;
    linefactor = extPoint.z = 1.0;
    angle = oblique = rot = 0.0;
    m_attachmentPoint = 5;
    style = "STANDARD";
    defPoint.z = extPoint.x = extPoint.y = 0;
    textPoint.z = rot = 0;
    clonePoint.x = clonePoint.y = clonePoint.z = 0;
  }

  EoDxfDimension(const EoDxfDimension& d) : EoDxfEntity(d) {
    m_entityType = EoDxf::DIMENSION;
    m_dimensionType = d.m_dimensionType;
    name = d.name;
    defPoint = d.defPoint;
    textPoint = d.textPoint;
    text = d.text;
    style = d.style;
    m_attachmentPoint = d.m_attachmentPoint;
    m_dimensionTextLineSpacingStyle = d.m_dimensionTextLineSpacingStyle;
    linefactor = d.linefactor;
    rot = d.rot;
    extPoint = d.extPoint;
    clonePoint = d.clonePoint;
    def1 = d.def1;
    def2 = d.def2;
    angle = d.angle;
    oblique = d.oblique;
    arcPoint = d.arcPoint;
    circlePoint = d.circlePoint;
    length = d.length;
  }
  virtual ~EoDxfDimension() = default;

  virtual void ApplyExtrusion() {}

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  EoDxfGeometryBase3d getDefPoint() const { return defPoint; }  // Definition point, code 10, 20 & 30
  void setDefPoint(const EoDxfGeometryBase3d p) { defPoint = p; }
  EoDxfGeometryBase3d getTextPoint() const { return textPoint; }  // Middle point of text, code 11, 21 & 31
  void setTextPoint(const EoDxfGeometryBase3d p) { textPoint = p; }
  std::string getStyle() const { return style; }  // Dimension style, code 3
  void setStyle(const std::string s) { style = s; }
  [[nodiscard]] std::int16_t GetAttachmentPoint() const noexcept { return m_attachmentPoint; }  // Group code 71
  void SetAttachmentPoint(const std::int16_t attachmentPoint) { m_attachmentPoint = attachmentPoint; }
  [[nodiscard]] std::int16_t getTextLineStyle() const noexcept { return m_dimensionTextLineSpacingStyle; }  // Group code 72
  void setTextLineStyle(const std::int16_t dimensionTextLineSpacingStyle) {
    m_dimensionTextLineSpacingStyle = dimensionTextLineSpacingStyle;
  }
  std::string getText() const { return text; }  // Dimension text explicitly entered by the user, code 1
  void setText(const std::string t) { text = t; }
  double getTextLineFactor() const { return linefactor; }  // Dimension text line spacing factor, code 41, default 1?
  void setTextLineFactor(const double l) { linefactor = l; }
  double getDir() const { return rot; }  // rotation angle of the dimension text, code 53 (optional) default 0
  void setDir(const double d) { rot = d; }

  EoDxfGeometryBase3d getExtrusion() { return extPoint; }  // extrusion, code 210, 220 & 230
  void setExtrusion(const EoDxfGeometryBase3d p) { extPoint = p; }
  std::string getName() { return name; }  // Name of the block that contains the entities, code 2
  void setName(const std::string s) { name = s; }
  //    int getType(){ return type;}                      // Dimension type, code 70

 protected:
  EoDxfGeometryBase3d getPt2() const { return clonePoint; }
  void setPt2(const EoDxfGeometryBase3d p) { clonePoint = p; }
  EoDxfGeometryBase3d getPt3() const { return def1; }
  void setPt3(const EoDxfGeometryBase3d p) { def1 = p; }
  EoDxfGeometryBase3d getPt4() const { return def2; }
  void setPt4(const EoDxfGeometryBase3d p) { def2 = p; }
  EoDxfGeometryBase3d getPt5() const { return circlePoint; }
  void setPt5(const EoDxfGeometryBase3d p) { circlePoint = p; }
  EoDxfGeometryBase3d getPt6() const { return arcPoint; }
  void setPt6(const EoDxfGeometryBase3d p) { arcPoint = p; }
  double getAn50() const { return angle; }  // Angle of rotated, horizontal, or vertical dimensions, code 50
  void setAn50(const double d) { angle = d; }
  double getOb52() const { return oblique; }  // oblique angle, code 52
  void setOb52(const double d) { oblique = d; }
  double getRa40() const { return length; }  // Leader length, code 40
  void setRa40(const double d) { length = d; }

 public:
  std::int16_t m_dimensionType;  // Dimension type, code 70
 private:
  std::string name;  // Name of the block that contains the entities, code 2
  EoDxfGeometryBase3d defPoint;  //  definition point, code 10, 20 & 30 (WCS)
  EoDxfGeometryBase3d textPoint;  // Middle point of text, code 11, 21 & 31 (OCS)
  std::string text;  // Dimension text explicitly entered by the user, code 1
  std::string style;  // Dimension style, code 3
  std::int16_t m_attachmentPoint;  // attachment point, code 71
  std::int16_t m_dimensionTextLineSpacingStyle{1};  // Group code 72 (optional)
  double linefactor;  // Dimension text line spacing factor, code 41, default 1? (value range 0.25 to 4.00*/
  double rot;  // rotation angle of the dimension text, code 53
  EoDxfGeometryBase3d extPoint;  //  extrusion normal vector, code 210, 220 & 230

  double hdir{};  // horizontal direction for the dimension, code 51, default ?
  EoDxfGeometryBase3d clonePoint;  // Insertion point for clones (Baseline & Continue), code 12, 22 & 32 (OCS)
  EoDxfGeometryBase3d def1;  // Definition point 1for linear & angular, code 13, 23 & 33 (WCS)
  EoDxfGeometryBase3d def2;  // Definition point 2, code 14, 24 & 34 (WCS)
  double angle;  // Angle of rotated, horizontal, or vertical dimensions, code 50
  double oblique;  // oblique angle, code 52

  EoDxfGeometryBase3d circlePoint;  // Definition point for diameter, radius & angular dims code 15, 25 & 35 (WCS)
  EoDxfGeometryBase3d arcPoint;  // Point defining dimension arc, x coordinate, code 16, 26 & 36 (OCS)
  double length{};  // Leader length, code 40
};

/** @brief Class to handle aligned dimension entity
 *
 *  An aligned dimension is a type of dimension that measures the distance between two points along a specified
 * direction. It is defined by its definition point (code 10, 20, 30), text point (code 11, 21, 31), and the two
 * definition points (code 13, 23, 33 and code 14, 24, 34). The aligned dimension can also include properties such as
 * the dimension style (code 3), attachment point (code 71), text line spacing style (code 72), dimension text (code 1),
 * text line spacing factor (code 41), rotation angle of the dimension text (code 53), and extrusion direction (code
 * 210, 220, 230), which can affect how it is rendered in the drawing.
 */
class EoDxfAlignedDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfAlignedDimension() { m_entityType = EoDxf::DIMALIGNED; }
  EoDxfAlignedDimension(const EoDxfDimension& d) : EoDxfDimension(d) { m_entityType = EoDxf::DIMALIGNED; }

  EoDxfGeometryBase3d getClonepoint() const {
    return getPt2();
  }  // Insertion for clones (Baseline & Continue), 12, 22 & 32
  void setClonePoint(EoDxfGeometryBase3d c) { setPt2(c); }

  EoDxfGeometryBase3d getDimPoint() const { return getDefPoint(); }  // dim line location point, code 10, 20 & 30
  void setDimPoint(const EoDxfGeometryBase3d p) { setDefPoint(p); }
  EoDxfGeometryBase3d getDef1Point() const { return getPt3(); }  // Definition point 1, code 13, 23 & 33
  void setDef1Point(const EoDxfGeometryBase3d p) { setPt3(p); }
  EoDxfGeometryBase3d getDef2Point() const { return getPt4(); }  // Definition point 2, code 14, 24 & 34
  void setDef2Point(const EoDxfGeometryBase3d p) { setPt4(p); }
};

/** @brief Class to handle linear dimension entity
 *
 *  A linear dimension is a type of dimension that measures the distance between two points along a specified direction.
 *  It is defined by its definition point (code 10, 20, 30), text point (code 11, 21, 31), and the two definition points
 * (code 13, 23, 33 and code 14, 24, 34). The linear dimension can also include properties such as the dimension style
 * (code 3), attachment point (code 71), text line spacing style (code 72), dimension text (code 1), text line spacing
 * factor (code 41), rotation angle of the dimension text (code 53), and extrusion direction (code 210, 220, 230),
 * which can affect how it is rendered in the drawing. Additionally, linear dimensions can have an oblique angle
 * (code 52) that specifies the angle of the dimension line relative to the horizontal plane.
 */
class EoDxfDimLinear : public EoDxfAlignedDimension {
 public:
  EoDxfDimLinear() { m_entityType = EoDxf::DIMLINEAR; }
  EoDxfDimLinear(const EoDxfDimension& dimension) : EoDxfAlignedDimension(dimension) {
    m_entityType = EoDxf::DIMLINEAR;
  }

  double getAngle() const { return getAn50(); }  // Angle of rotated, horizontal, or vertical dimensions, code 50
  void setAngle(const double d) { setAn50(d); }
  double getOblique() const { return getOb52(); }  // oblique angle, code 52
  void setOblique(const double d) { setOb52(d); }
};

/** @brief Class to handle radial dimension entity
 *
 *  A radial dimension is a type of dimension that measures the radius of a circle or arc.
 *  It is defined by its center point (code 10, 20, 30), text point (code 11, 21, 31), and the definition point for the
 * radius (code 15, 25, 35). The radial dimension can also include properties such as the dimension style (code 3),
 * attachment point (code 71), text line spacing style (code 72), dimension text (code 1), text line spacing factor
 * (code 41), rotation angle of the dimension text (code 53), and extrusion direction (code 210, 220, 230), which can
 * affect how it is rendered in the drawing.
 */
class EoDxfRadialDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfRadialDimension() { m_entityType = EoDxf::DIMRADIAL; }
  EoDxfRadialDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) { m_entityType = EoDxf::DIMRADIAL; }

  EoDxfGeometryBase3d getCenterPoint() const { return getDefPoint(); }  // center point, code 10, 20 & 30
  void setCenterPoint(const EoDxfGeometryBase3d p) { setDefPoint(p); }
  EoDxfGeometryBase3d getDiameterPoint() const { return getPt5(); }  // Definition point for radius, code 15, 25 & 35
  void setDiameterPoint(const EoDxfGeometryBase3d p) { setPt5(p); }
  double getLeaderLength() const { return getRa40(); }  // Leader length, code 40
  void setLeaderLength(const double d) { setRa40(d); }
};

/** @brief Class to handle diametric dimension entity
 *
 *  A diametric dimension is a type of dimension that measures the diameter of a circle or arc.
 *  It is defined by its center point (code 10, 20, 30), text point (code 11, 21, 31), and the definition point for the
 * diameter (code 15, 25, 35). The diametric dimension can also include properties such as the dimension style (code 3),
 * attachment point (code 71), text line spacing style (code 72), dimension text (code 1), text line spacing factor
 * (code 41), rotation angle of the dimension text (code 53), and extrusion direction (code 210, 220, 230), which can
 * affect how it is rendered in the drawing.
 */
class EoDxfDiametricDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfDiametricDimension() { m_entityType = EoDxf::DIMDIAMETRIC; }
  EoDxfDiametricDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMDIAMETRIC;
  }

  EoDxfGeometryBase3d getDiameter1Point() const {
    return getPt5();
  }  // First definition point for diameter, code 15, 25 & 35
  void setDiameter1Point(const EoDxfGeometryBase3d p) { setPt5(p); }
  EoDxfGeometryBase3d getDiameter2Point() const {
    return getDefPoint();
  }  // Oposite point for diameter, code 10, 20 & 30
  void setDiameter2Point(const EoDxfGeometryBase3d p) { setDefPoint(p); }
  double getLeaderLength() const { return getRa40(); }  // Leader length, code 40
  void setLeaderLength(const double d) { setRa40(d); }
};

//! Class to handle angular dimension entity
/*!
 *  Class to handle angular dimension entity
 */
class EoDxf2LineAngularDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxf2LineAngularDimension() { m_entityType = EoDxf::DIMANGULAR; }
  EoDxf2LineAngularDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMANGULAR;
  }

  EoDxfGeometryBase3d getFirstLine1() const { return getPt3(); }  // Definition point line 1-1, code 13, 23 & 33
  void setFirstLine1(const EoDxfGeometryBase3d p) { setPt3(p); }
  EoDxfGeometryBase3d getFirstLine2() const { return getPt4(); }  // Definition point line 1-2, code 14, 24 & 34
  void setFirstLine2(const EoDxfGeometryBase3d p) { setPt4(p); }
  EoDxfGeometryBase3d getSecondLine1() const { return getPt5(); }  // Definition point line 2-1, code 15, 25 & 35
  void setSecondLine1(const EoDxfGeometryBase3d p) { setPt5(p); }
  EoDxfGeometryBase3d getSecondLine2() const { return getDefPoint(); }  // Definition point line 2-2, code 10, 20 & 30
  void setSecondLine2(const EoDxfGeometryBase3d p) { setDefPoint(p); }
  EoDxfGeometryBase3d getDimPoint() const { return getPt6(); }  // Dimension definition point, code 16, 26 & 36
  void setDimPoint(const EoDxfGeometryBase3d p) { setPt6(p); }
};

/** @brief Class to handle 3 point angular dimension entity
 *
 *  A 3 point angular dimension is a type of dimension that measures the angle between three points.
 *  It is defined by its vertex point (code 15, 25, 35), the two definition points for the lines (code 13, 23, 33 and
 * code 14, 24, 34), and the dimension definition point (code 10, 20, 30). The 3 point angular dimension can also
 * include properties such as the dimension style (code 3), attachment point (code 71), text line spacing style (code
 * 72), dimension text (code 1), text line spacing factor (code 41), rotation angle of the dimension text (code 53), and
 * extrusion direction (code 210, 220, 230), which can affect how it is rendered in the drawing.
 */
class EoDxf3PointAngularDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxf3PointAngularDimension() { m_entityType = EoDxf::DIMANGULAR3P; }
  EoDxf3PointAngularDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMANGULAR3P;
  }

  EoDxfGeometryBase3d getFirstLine() const { return getPt3(); }  // Definition point line 1, code 13, 23 & 33
  void setFirstLine(const EoDxfGeometryBase3d p) { setPt3(p); }
  EoDxfGeometryBase3d getSecondLine() const { return getPt4(); }  // Definition point line 2, code 14, 24 & 34
  void setSecondLine(const EoDxfGeometryBase3d p) { setPt4(p); }
  EoDxfGeometryBase3d getVertexPoint() const { return getPt5(); }  // Vertex point, code 15, 25 & 35
  void SetVertexPoint(const EoDxfGeometryBase3d p) { setPt5(p); }
  EoDxfGeometryBase3d getDimPoint() const { return getDefPoint(); }  // Dimension definition point, code 10, 20 & 30
  void setDimPoint(const EoDxfGeometryBase3d p) { setDefPoint(p); }
};

/** @brief Class to handle ordinate dimension entity
 *
 *  An ordinate dimension is a type of dimension that measures the distance from a specified origin point to a feature
 * location point along a specified direction. It is defined by its origin point (code 10, 20, 30), the feature location
 * point (code 13, 23, 33), and the leader end point (code 14, 24, 34). The ordinate dimension can also include
 * properties such as the dimension style (code 3), attachment point (code 71), text line spacing style (code 72),
 * dimension text (code 1), text line spacing factor (code 41), rotation angle of the dimension text (code 53), and
 * extrusion direction (code 210, 220, 230), which can affect how it is rendered in the drawing.
 */
class EoDxfOrdinateDimension : public EoDxfDimension {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfOrdinateDimension() { m_entityType = EoDxf::DIMORDINATE; }
  EoDxfOrdinateDimension(const EoDxfDimension& dimension) : EoDxfDimension(dimension) {
    m_entityType = EoDxf::DIMORDINATE;
  }

  EoDxfGeometryBase3d getOriginPoint() const { return getDefPoint(); }  // Origin definition point, code 10, 20 & 30
  void setOriginPoint(const EoDxfGeometryBase3d p) { setDefPoint(p); }
  EoDxfGeometryBase3d getFirstLine() const { return getPt3(); }  // Feature location point, code 13, 23 & 33
  void setFirstLine(const EoDxfGeometryBase3d p) { setPt3(p); }
  EoDxfGeometryBase3d getSecondLine() const { return getPt4(); }  // Leader end point, code 14, 24 & 34
  void setSecondLine(const EoDxfGeometryBase3d p) { setPt4(p); }
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
class EoDxfLeader : public EoDxfEntity {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfLeader() noexcept : EoDxfEntity{EoDxf::LEADER} {}

  EoDxfLeader(const EoDxfLeader&) = delete;
  EoDxfLeader& operator=(const EoDxfLeader&) = delete;

  EoDxfLeader(EoDxfLeader&&) noexcept = default;
  EoDxfLeader& operator=(EoDxfLeader&&) noexcept = default;

  ~EoDxfLeader() = default;

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  std::string m_dimensionStyleName{};  // Group code 3
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
  int m_leaderLineIndex{};  // code 91
  int m_leaderLineColorOverride{EoDxf::colorByLayer};  // code 92 (optional)
  std::uint64_t m_leaderLineTypeHandle{EoDxf::NoHandle};  // code 340 (optional)
  int m_leaderLineWeightOverride{-1};  // code 171 (optional, -1 = no override)
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
  int m_leaderBranchIndex{};  // code 90
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
  int m_textLeftAttachment{1};  // code 174
  int m_textRightAttachment{1};  // code 175
  int m_textAlignmentType{};  // code 176
  int m_blockContentConnectionType{};  // code 177
  bool m_hasMText{};  // code 290
  bool m_hasContent{true};  // code 296

  // --- MText content (delimited by code 304 "{" ... 305 "}") ---
  EoDxfGeometryBase3d m_textLocation;  // code 12, 22, 32
  EoDxfGeometryBase3d m_textDirection;  // code 13, 23, 33
  double m_textRotation{};  // code 42
  double m_textWidth{};  // code 43
  double m_textDefinedWidth{};  // code 44
  double m_textDefinedHeight{};  // code 45
  int m_textLineSpacingStyle{1};  // code 171
  double m_textLineSpacingFactor{1.0};  // code 141
  int m_textFlowDirection{1};  // code 90
  int m_textColor{EoDxf::colorByLayer};  // code 91
  int m_textAttachment{1};  // code 170
  int m_textBackgroundFill{};  // code 172
  std::uint64_t m_textStyleHandle{EoDxf::NoHandle};  // code 340
  std::string m_textString;  // code 1 (and 3 for overflow)

  // --- Block content ---
  std::uint64_t m_blockContentHandle{EoDxf::NoHandle};  // code 341
  EoDxfGeometryBase3d m_blockContentNormalDirection{0.0, 0.0, 1.0};  // code 14, 24, 34
  EoDxfGeometryBase3d m_blockContentScale{1.0, 1.0, 1.0};  // code 15, 25, 35
  double m_blockContentRotation{};  // code 46
  int m_blockContentColor{EoDxf::colorByLayer};  // code 93

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
class EoDxfViewport : public EoDxfPoint {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfViewport() noexcept { m_entityType = EoDxf::VIEWPORT; }

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  double m_width{205.0};  // Width in paper space units, code 40
  double m_height{156.0};  // Height in paper space units, code 41
  std::int16_t m_viewportStatus{};  // Viewport status field, code 68
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
  std::uint32_t m_frozenLayerCount{};  // Number of frozen layers, code 90
};