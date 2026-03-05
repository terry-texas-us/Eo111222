#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "drw_base.h"

class dxfReader;
class DRW_Polyline;

namespace DRW {

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
  //        MLEADER,
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

}  // namespace DRW

/** @brief Base class for entities
 */
class DRW_Entity {
  friend class dxfRW;

 public:
  DRW_Entity() = default;

 protected:
  explicit DRW_Entity(DRW::ETYPE entityType) noexcept : m_entityType{entityType} {}

 public:
  DRW_Entity(const DRW_Entity& other);
  DRW_Entity& operator=(const DRW_Entity& other);

  DRW_Entity(DRW_Entity&&) noexcept = default;
  DRW_Entity& operator=(DRW_Entity&&) noexcept = default;

  virtual ~DRW_Entity();

  void Clear();

  virtual void ApplyExtrusion() = 0;

 protected:
  /** @brief Parses dxf code and value to read entity data
   *  @param code dxf code
   *  @param reader pointer to dxfReader to read value
   */
  void ParseCode(int code, dxfReader* reader);

  /** @brief Parses application-defined group (code 102) and its associated data until the closing tag is reached.
   *  @param reader pointer to dxfReader to read value
   *  @return true if group is successfully parsed, false if group is not recognized or an error occurs
   */
  bool ParseAppDataGroup(dxfReader* reader);

  /** @brief Calculates the arbitrary extrusion axis (extAxisX and extAxisY) based on the given extrusion direction
   * (extPoint). This follows the DXF specification for handling extrusion directions and their corresponding axes. The
   * calculated axes are unitized for use in extrusion transformations.
   *  @param extrusionDirection The extrusion direction vector from the DXF entity, used to calculate the arbitrary
   * axes.
   */
  void CalculateArbitraryAxis(const DRW_Coord& extrusionDirection);

  /** @brief Applies an extrusion transformation to the given point using the pre-calculated arbitrary axes and the
   * extrusion direction. The transformation is defined as: P' = (Ax * point.x) + (Ay * point.y) + (N * point.z), where
   * Ax and Ay are the arbitrary axes, N is the extrusion direction, and point is the original coordinate. The result is
   * stored back in the provided point reference.
   *
   * @param extrusionDirection The extrusion direction vector (N) used in the transformation.
   * @param[out] point A DRW_Coord representing the original point to be transformed. The transformed coordinates will
   * be stored back in this variable.
   */
  void ExtrudePointInPlace(const DRW_Coord& extrusionDirection, DRW_Coord& point) const noexcept;

 private:
  // Transient cache for ApplyExtrusion() — deliberately NOT copied or cleared.
  // Always recomputed from m_extrusionDirection when needed.
  DRW_Coord extAxisX{};
  DRW_Coord extAxisY{};

 public:
  std::list<std::list<DRW_Variant>> m_appData{};  // list of application data, code 102
  std::vector<DRW_Variant*> m_extendedData{};  // codes 1000 to 1071
  UTF8STRING m_layer{"0"};  // layer name, code 8
  UTF8STRING m_lineType{"BYLAYER"};  // line type, code 6
  std::string m_proxyEntityGraphicsData{};  // group code 310 (optional) [unused]
  std::string m_colorName{};  // group code 430
  double m_lineTypeScale{1.0};  // linetype scale, code 48
  enum DRW::ETYPE m_entityType{DRW::UNKNOWN};  // entity type, code 0
  std::uint32_t m_handle{DRW::HandleCodes::NoHandle};  // entity identifier, code 5
  // Soft-pointer ID/handle to owner BLOCK_RECORD object, code 330
  std::uint32_t m_ownerHandle{DRW::HandleCodes::NoHandle};
  std::uint32_t m_material{DRW::MaterialCodes::MaterialByLayer};  // hard pointer id to material object, code 347
  int m_color{DRW::ColorCodes::ColorByLayer};  // entity color, code 62
  enum DRW_LW_Conv::lineWidth m_lineWeight{DRW_LW_Conv::widthByLayer};  // entity lineweight, code 370
  int m_numberOfBytesInProxyGraphics{};  // group code 92 (optional) [unused]
  int m_color24{-1};  // 24-bit color, code 420
  int m_transparency{DRW::TransparencyCodes::Opaque};  // group code 440
  int m_plotStyle{DRW::PlotStyleCodes::DefaultPlotStyle};  // hard pointer id to plot style object, code 390
  DRW::ShadowMode m_shadowMode{DRW::ShadowMode::CastAndReceiveShadows};  // group code 284
  DRW::Space m_space{DRW::Space::ModelSpace};  // space indicator, code 67
  bool m_visible{true};  // entity visibility, code 60
  bool m_haveExtrusion{};  // set to true if the entity have extrusion
 private:
  void clearExtendedData() noexcept;

  DRW_Variant* m_currentVariant{};
};

/** @brief Class to handle point entity
 */
class DRW_Point : public DRW_Entity {
  friend class dxfRW;

 public:
  explicit DRW_Point(DRW::ETYPE entityType = DRW::POINT) noexcept : DRW_Entity{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  DRW_Coord m_firstPoint{};  //  base point, code 10, 20 & 30
  double m_thickness{};  // Thickness, code 39
  //  Extrusion direction, code 210, 220 & 230 (optional, default 0,0,1)
  DRW_Coord m_extrusionDirection{0.0, 0.0, 1.0};
  //  Angle of the X axis for the UCS in effect when the point was drawn, code 50 (optional, default 0.0)
  double m_angleX{};
};

/** @brief Class to handle line entity
 */
class DRW_Line : public DRW_Point {
  friend class dxfRW;

 public:
  explicit DRW_Line(DRW::ETYPE entityType = DRW::LINE) noexcept : DRW_Point{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  DRW_Coord m_secondPoint;  // Group code 11, 21 & 31
};

/** @brief Class to handle ray entity
 *
 *  A ray is a line that starts at a specific point and extends infinitely in one direction.
 *  It differs from a line entity in that a line has two endpoints,
 *  while a ray has only one starting point and extends infinitely in the direction defined by its second point.
 */
class DRW_Ray : public DRW_Line {
  friend class dxfRW;

 public:
  explicit DRW_Ray(DRW::ETYPE entityType = DRW::RAY) noexcept : DRW_Line{entityType} {}
};

/** @brief Class to handle xline entity
 *
 *  An xline is an infinite line that extends infinitely in both directions.
 *  It differs from a line entity in that a line has two endpoints,
 *  while an xline extends infinitely in both directions defined by its two points.
 */
class DRW_Xline : public DRW_Ray {
 public:
  explicit DRW_Xline(DRW::ETYPE entityType = DRW::XLINE) noexcept : DRW_Ray{entityType} {}
};

/** @brief Class to handle circle entity
 *
 *  A circle is a closed curve where all points are equidistant from a fixed center point.
 *  It is defined by its center point and radius.
 *  The circle entity in DXF can also include additional properties such as thickness and extrusion direction,
 *  which can affect how the circle is rendered in 3D space.
 */
class DRW_Circle : public DRW_Point {
  friend class dxfRW;

 public:
  explicit DRW_Circle(DRW::ETYPE entityType = DRW::CIRCLE) noexcept : DRW_Point{entityType} {}

  void ApplyExtrusion() override;

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  double m_radius{};  // Radius, code 40
};

class DRW_Arc : public DRW_Circle {
  friend class dxfRW;

 public:
  explicit DRW_Arc(DRW::ETYPE entityType = DRW::ARC) noexcept : DRW_Circle{entityType} {}

  void ApplyExtrusion() override;

  // center point in OCS
  const DRW_Coord& Center() const { return m_firstPoint; }
  // the radius of the circle
  [[nodiscard]] double Radius() const noexcept { return m_radius; }
  // start angle in radians
  [[nodiscard]] double StartAngle() const noexcept { return m_startAngle; }
  // end angle in radians
  [[nodiscard]] double EndAngle() const noexcept { return m_endAngle; }
  // thickness
  [[nodiscard]] double Thickness() const noexcept { return m_thickness; }
  // extrusion
  [[nodiscard]] const DRW_Coord& ExtrusionDirection() const noexcept { return m_extrusionDirection; }

 protected:
  // interpret code in dxf reading process or dispatch to inherited class
  void ParseCode(int code, dxfReader* reader);

 public:
  double m_startAngle{};  // group code 50 (in radians)
  double m_endAngle{};  // group code 51 (in radians)
  int m_isCounterClockwise{1};  // group code 73
};

/** @brief Class to handle ellipse entity
 *
 *  An ellipse is a closed curve that resembles a flattened circle.
 *  It is defined by its center point, major axis, and ratio of minor axis to major axis.
 *  The ellipse entity in DXF can also include additional properties such as thickness and extrusion direction,
 *  which can affect how the ellipse is rendered in 3D space.
 */
class DRW_Ellipse : public DRW_Line {
  friend class dxfRW;

 public:
  explicit DRW_Ellipse(DRW::ETYPE entityType = DRW::ELLIPSE) noexcept : DRW_Line{entityType} {}

  void ToPolyline(DRW_Polyline* polyline, int parts = 128);
  void ApplyExtrusion() override;

 protected:
  /** @brief Parses dxf code and value to read ellipse entity data
   *  @param code dxf code
   *  @param reader pointer to dxfReader to read value
   */
  void ParseCode(int code, dxfReader* reader);

 private:
  void CorrectAxis();

 public:
  double m_ratio{};  // Group code 40
  double m_startParam{};  // Group code 41, 0.0 for full ellipse
  double m_endParam{};  // Group code 42, 2*PI for full ellipse
  int m_isCounterClockwise{1};  // Group code 73 (only used in hatch)
};

/** @@brief Class to handle trace entity
 *
 *  A trace is a four-sided polyline that is always planar.
 *  It is defined by its four corner points, which are specified in the DXF file using group codes 10, 20, 30 for the
 * first point; 11, 21, 31 for the second point; 12, 22, 32 for the third point; and 13, 23, 33 for the fourth point.
 *  The trace entity can also include properties such as thickness and extrusion direction, which can affect how it is
 * rendered in 3D space.
 */
class DRW_Trace : public DRW_Line {
  friend class dxfRW;

 public:
  explicit DRW_Trace(DRW::ETYPE entityType = DRW::TRACE) noexcept : DRW_Line{entityType} {}

  void ApplyExtrusion() override;

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  DRW_Coord m_thirdPoint{};  // Group code 12, 22 & 32
  DRW_Coord m_fourthPoint{};  // Group code 13, 23 & 33
};

/** @brief Class to handle solid entity
 *
 *  A solid is a four-sided polyline that is always planar and filled.
 *  It is defined by its four corner points, which are specified in the DXF file using group codes 10, 20, 30 for the
 * first point; 11, 21, 31 for the second point; 12, 22, 32 for the third point; and 13, 23, 33 for the fourth point.
 *  The solid entity can also include properties such as thickness and extrusion direction, which can affect how it is
 * rendered in 3D space.
 */
class DRW_Solid : public DRW_Trace {
  friend class dxfRW;

 public:
  DRW_Solid() { m_entityType = DRW::SOLID; }

 protected:
  //! interpret code in dxf reading process or dispatch to inherited class
  void ParseCode(int code, dxfReader* reader);

 public:
  [[nodiscard]] const DRW_Coord& FirstCorner() const noexcept { return m_firstPoint; }
  [[nodiscard]] const DRW_Coord& SecondCorner() const noexcept { return m_secondPoint; }
  [[nodiscard]] const DRW_Coord& ThirdCorner() const noexcept { return m_thirdPoint; }
  [[nodiscard]] const DRW_Coord& FourthCorner() const noexcept { return m_fourthPoint; }
  [[nodiscard]] double thick() const noexcept { return m_thickness; }
  [[nodiscard]] double elevation() const noexcept { return m_firstPoint.z; }
  [[nodiscard]] const DRW_Coord& extrusion() const noexcept { return m_extrusionDirection; }
};

/** @brief Class to handle 3dface entity
 *
 *  A 3D face is a three- or four-sided planar surface defined by its corner points.
 *  It is specified in the DXF file using group codes 10, 20, 30 for the first point;
 *  11, 21, 31 for the second point; 12, 22, 32 for the third point; and optionally 13, 23, 33 for the fourth point.
 *  The fourth point is optional because a 3D face can be a triangle (three-sided) or a quadrilateral (four-sided).
 *  The entity can also include properties such as thickness and extrusion direction, which can affect how it is
 * rendered in 3D space.
 */
class DRW_3Dface : public DRW_Trace {
  friend class dxfRW;

 public:
  enum InvisibleEdgeFlags {
    NoEdge = 0x00,
    FirstEdge = 0x01,
    SecodEdge = 0x02,
    ThirdEdge = 0x04,
    FourthEdge = 0x08,
    AllEdges = 0x0F
  };

  explicit DRW_3Dface(DRW::ETYPE entityType = DRW::E3DFACE) noexcept : DRW_Trace{entityType} {}

  void ApplyExtrusion() override {}

  [[nodiscard]] const DRW_Coord& FirstCorner() const noexcept { return m_firstPoint; }
  [[nodiscard]] const DRW_Coord& SecondCorner() const noexcept { return m_secondPoint; }
  [[nodiscard]] const DRW_Coord& ThirdCorner() const noexcept { return m_thirdPoint; }
  [[nodiscard]] const DRW_Coord& FourthCorner() const noexcept { return m_fourthPoint; }
  [[nodiscard]] InvisibleEdgeFlags edgeFlags() const noexcept {
    return static_cast<InvisibleEdgeFlags>(m_invisibleFlag);
  }

 protected:
  //! interpret code in dxf reading process or dispatch to inherited class
  void ParseCode(int code, dxfReader* reader);

 public:
  int m_invisibleFlag{};  // Group code 70
};

/** @brief Class to handle block entries
 *
 *  A block is a collection of entities that are grouped together and can be inserted into a drawing as a single unit.
 *  It is defined by its name, which is specified in the DXF file using group code 2, and its type, which is specified
 * using group code 70. The block entity can also include properties such as layer, line type, and extrusion direction,
 * which can affect how it is rendered in the drawing.
 */
class DRW_Block : public DRW_Point {
  friend class dxfRW;

 public:
  explicit DRW_Block(DRW::ETYPE entityType = DRW::BLOCK) noexcept : DRW_Point{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  UTF8STRING name{"*U0"};  // Group code 2
  int m_flags{};  // Group code 70
};

/** @brief Class to handle insert entries
 *
 *  An insert is an instance of a block that is placed in a drawing.
 *  It is defined by the name of the block it references, which is specified in the DXF file using group code 2, and its
 * insertion point, which is specified using group codes 10, 20, and 30. The insert entity can also include properties
 * such as scale factors (code 41, 42, 43), rotation angle (code 50), and number of columns and rows for array inserts
 * (code 70 and 71), which can affect how it is rendered in the drawing.
 */
class DRW_Insert : public DRW_Point {
  friend class dxfRW;

 public:
  explicit DRW_Insert(DRW::ETYPE entityType = DRW::INSERT) noexcept : DRW_Point{entityType} {}

  void ApplyExtrusion() override { DRW_Point::ApplyExtrusion(); }

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  UTF8STRING m_blockName;  // Group code 2
  double m_xScaleFactor{1.0};  // Group code 41
  double m_yScaleFactor{1.0};  // Group code 42
  double m_zScaleFactor{1.0};  // Group code 43
  double m_rotationAngle{};  // Group code 50 (in radians)
  int m_columnCount{1};  // Group code 70
  int m_rowCount{1};  // Group code 71
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
class DRW_LWPolyline : public DRW_Entity {
  friend class dxfRW;

 public:
  explicit DRW_LWPolyline(DRW::ETYPE entityType = DRW::LWPOLYLINE) noexcept : DRW_Entity{entityType} {}

  DRW_LWPolyline(const DRW_LWPolyline&) = default;
  DRW_LWPolyline(DRW_LWPolyline&&) noexcept = default;
  DRW_LWPolyline& operator=(const DRW_LWPolyline&) = default;
  DRW_LWPolyline& operator=(DRW_LWPolyline&&) noexcept = default;

  ~DRW_LWPolyline() = default;

  void ApplyExtrusion() override;

  void AddVertex(const DRW_Vertex2D& vertex) { m_vertices.push_back(vertex); }

  [[nodiscard]] DRW_Vertex2D& AddVertex() { return m_vertices.emplace_back(); }

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  int m_numberOfVertices{};  // Group code 90
  int m_polylineFlag{};  // Group code 70, (1 = Closed; 128 = Plinegen)
  double m_constantWidth{};  // Group code 43
  double m_elevation{};  // Group code 38
  double m_thickness{};  // Group code 39
  DRW_Coord m_extrusionDirection{0.0, 0.0, 1.0};  //  code 210, 220 & 230
  std::vector<DRW_Vertex2D> m_vertices;

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
class DRW_Text : public DRW_Line {
  friend class dxfRW;

 public:
  enum VAlign { BaseLine = 0, Bottom, Middle, Top };

  enum HAlign { Left = 0, Center, Right, AlignedIfBaseLine, MiddleIfBaseLine, FitIfBaseLine };

  explicit DRW_Text(DRW::ETYPE entityType = DRW::TEXT) noexcept : DRW_Line{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  double m_textHeight{};  // Group code 40
  UTF8STRING m_string;  // Group code 1
  double m_textRotation{};  // Group code 50
  double m_scaleFactorWidth{1.0};  // Group code 41
  double m_obliqueAngle{};  // Group code 51
  UTF8STRING m_textStyleName{"STANDARD"};  // Group code 7
  int m_textGenerationFlags{};  // Group code 71
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
class DRW_MText : public DRW_Text {
  friend class dxfRW;

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

  explicit DRW_MText(DRW::ETYPE entityType = DRW::MTEXT) noexcept : DRW_Text{entityType} {
    m_verticalAlignment = (VAlign)TopLeft;
    m_textGenerationFlags = 1;
  }

 protected:
  void ParseCode(int code, dxfReader* reader);
  void UpdateAngle();  // recalculate angle if 'm_haveXAxisDirection' is true

 public:
  double m_lineSpacingFactor{1.0};  // Group code 44 (optional)
 private:
  bool m_haveXAxisDirection{};
};

class DRW_Vertex : public DRW_Point {
  friend class dxfRW;

 public:
  DRW_Vertex() noexcept : DRW_Point{DRW::VERTEX} {}

  DRW_Vertex(double sx, double sy, double sz, double bulge) noexcept : DRW_Point{DRW::VERTEX}, m_bulge{bulge} {
    m_firstPoint = {sx, sy, sz};
  }

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  double m_startingWidth{};  // Group code 40
  double m_endingWidth{};  // Group code 41
  double m_bulge{};  // Group code 42

  int m_vertexFlags{};  // Group code 70
  double m_curveFitTangentDirection{};  // Group code 50
  int m_polyfaceMeshVertexIndex1{};  // Group code 71
  int m_polyfaceMeshVertexIndex2{};  // Group code 72
  int m_polyfaceMeshVertexIndex3{};  // Group code 73
  int m_polyfaceMeshVertexIndex4{};  // Group code 74
  int m_identifier{};  // Group code 91
};

/** @brief Class to handle seqend entity
 *
 *  A SEQEND entity is used to indicate the end of a sequence of vertices in a POLYLINE or INSERT entity.
 *  It does not contain any geometric data itself, but serves as a marker to signify the conclusion of the vertex list
 * for the associated entity. The SEQEND entity can inherit properties such as layer and display settings from its
 * owning POLYLINE or INSERT entity, but it does not have its own unique properties.
 */
class DRW_SeqEnd : public DRW_Entity {
  friend class dxfRW;

 public:
  DRW_SeqEnd() noexcept : DRW_Entity{DRW::SEQEND} {}

  /** @brief Constructs a SEQEND that inherits layer and display properties
   *         from the owning POLYLINE or INSERT entity.
   *  @param owner The entity whose sequence this SEQEND terminates.
   */
  explicit DRW_SeqEnd(const DRW_Entity& owner) noexcept : DRW_Entity{DRW::SEQEND} {
    m_layer = owner.m_layer;
    m_lineType = owner.m_lineType;
    m_color = owner.m_color;
    m_lineWeight = owner.m_lineWeight;
  }

  void ApplyExtrusion() override {}
};

class DRW_Polyline : public DRW_Point {
  friend class dxfRW;

 public:
  DRW_Polyline() noexcept : DRW_Point{DRW::POLYLINE} {}

  ~DRW_Polyline() {
    for (DRW_Vertex* vertex : m_vertices) { delete vertex; }
  }

  // Prevent double-free from shallow copy of raw-pointer vector
  DRW_Polyline(const DRW_Polyline&) = delete;
  DRW_Polyline& operator=(const DRW_Polyline&) = delete;

  DRW_Polyline(DRW_Polyline&& other) noexcept = default;
  DRW_Polyline& operator=(DRW_Polyline&& other) noexcept = default;

  void addVertex(const DRW_Vertex& v) {
    auto* vertex = new DRW_Vertex();
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

  void appendVertex(DRW_Vertex* v) { m_vertices.push_back(v); }

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  int m_polylineFlag{};  // Group code 70
  double m_defaultStartWidth{};  // Group code 40
  double m_defaultEndWidth{};  // Group code 41
  int m_polygonMeshVertexCountM{};  // Group code 71
  int m_polygonMeshVertexCountN{};  // Group code 72
  int m_smoothSurfaceDensityM{};  // Group code 73
  int m_smoothSurfaceDensityN{};  // Group code 74
  int m_curvesAndSmoothSurfaceType{};  // Group code 75

  std::vector<DRW_Vertex*> m_vertices;  // vertex list
};

/** @brief Class to handle spline entity
 *
 *  A spline is a smooth curve defined by control points and a degree.
 *  It is specified in the DXF file using group codes for its control points (code 10, 20, 30), degree (code 71),
 * and number of control points (code 73). The spline entity can also include properties such as knot values (code 40),
 * flags (code 70), and extrusion direction (code 210, 220, 230), which can affect how it is rendered in the drawing.
 */
class DRW_Spline : public DRW_Entity {
  friend class dxfRW;

 public:
  DRW_Spline() noexcept : DRW_Entity{DRW::SPLINE} {}

  ~DRW_Spline() {
    for (DRW_Coord* point : m_controlPoints) { delete point; }
    for (DRW_Coord* point : m_fitPoints) { delete point; }
  }

  // Prevent double-free from shallow copy of raw-pointer vectors
  DRW_Spline(const DRW_Spline&) = delete;
  DRW_Spline& operator=(const DRW_Spline&) = delete;

  DRW_Spline(DRW_Spline&& other) noexcept = default;
  DRW_Spline& operator=(DRW_Spline&& other) noexcept = default;

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  DRW_Coord m_normalVector;  // Group codes 210, 220, 230
  DRW_Coord m_startTangent;  // Group codes 12, 22, 32
  DRW_Coord m_endTangent;  // Group codes 13, 23, 33
  int m_splineFlag{};  // Group code 70
  int m_degreeOfTheSplineCurve{};  // Group code 71
  std::int32_t m_numberOfKnots{};  // Group code 72
  std::int32_t m_numberOfControlPoints{};  // Group code 73
  std::int32_t m_numberOfFitPoints{};  // Group code 74
  double m_knotTolerance{0.0000001};  // Group code 42
  double m_controlPointTolerance{0.0000001};  // Group code 43
  double m_fitTolerance{0.0000000001};  // Group code 44

  std::vector<double> m_knotValues;  // Group code 40, (one entry per knot)
  std::vector<DRW_Coord*> m_controlPoints;  // Group codes 10, 20 & 30 (one entry per control point)
  std::vector<DRW_Coord*> m_fitPoints;  // Group codes 11, 21 & 31 (one entry per fit point)

 private:
  DRW_Coord* m_controlPoint{};  // current control point to add data
  DRW_Coord* m_fitPoint{};  // current fit point to add data
};

/** @brief Class to handle hatch loop
 *
 *  A hatch loop represents a closed boundary path that defines the area to be filled with a hatch pattern.
 *  It is defined by its type (code 92) and the number of edges (code 93). The hatch loop can include various
 * entities such as lines, arcs, circles, ellipses, splines, and lightweight polylines that make up the boundary of the
 * hatch.
 */
class DRW_HatchLoop {
 public:
  explicit DRW_HatchLoop(int boundaryPathType) : m_boundaryPathType{boundaryPathType} {}

  ~DRW_HatchLoop() = default;

  DRW_HatchLoop(const DRW_HatchLoop&) = delete;
  DRW_HatchLoop& operator=(const DRW_HatchLoop&) = delete;
  DRW_HatchLoop(DRW_HatchLoop&&) = delete;
  DRW_HatchLoop& operator=(DRW_HatchLoop&&) = delete;


  void Update() { m_numberOfEdges = static_cast<int>(m_entities.size()); }

 public:
  int m_boundaryPathType{};  // Group code 92
  int m_numberOfEdges{};  // Group code 93

  std::vector<std::unique_ptr<DRW_Entity>> m_entities;
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
class DRW_Hatch : public DRW_Point {
  friend class dxfRW;

 public:
  DRW_Hatch() : DRW_Point{DRW::HATCH} {}

  ~DRW_Hatch() {
    for (auto* hatchLoop : m_hatchLoops) { delete hatchLoop; }
  }

  DRW_Hatch(const DRW_Hatch&) = delete;
  DRW_Hatch& operator=(const DRW_Hatch&) = delete;
  DRW_Hatch(DRW_Hatch&&) = delete;
  DRW_Hatch& operator=(DRW_Hatch&&) = delete;

  void AppendLoop(DRW_HatchLoop* hatchLoop) { m_hatchLoops.push_back(hatchLoop); }

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  UTF8STRING m_hatchPatternName;  // Group code 2
  int m_solidFillFlag{1};  // Group code 70
  int m_associativityFlag{};  // Group code 71
  int m_hatchStyle{};  // Group code 75
  int m_hatchPatternType{1};  // Group code 76
  int m_hatchPatternDoubleFlag{};  // Group code 77
  int m_numberOfBoundaryPaths{};  // Group code 91
  double m_hatchPatternAngle{};  // Group code 52
  double m_hatchPatternScaleOrSpacing{};  // Group code 41
  int m_numberOfPatternDefinitionLines{};  //   Group code 78

  std::vector<DRW_HatchLoop*> m_hatchLoops;

 private:
  void ClearEntities() noexcept;

  void AddLine();
  
  void AddArc();

  void AddEllipse();

  void AddSpline();

  DRW_HatchLoop* m_hatchLoop{};  // current loop to add data
  DRW_Line* m_line{};
  DRW_Arc* m_arc{};
  DRW_Ellipse* m_ellipse{};
  DRW_Spline* m_spline{};
  DRW_LWPolyline* m_polyline{};
  DRW_Point* m_point{};
  DRW_Vertex2D* m_polylineVertex{};
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
class DRW_Image : public DRW_Line {
  friend class dxfRW;

 public:
  DRW_Image() {
    m_entityType = DRW::IMAGE;
    fade = clip = 0;
    brightness = contrast = 50;
  }

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  std::uint32_t ref{};  // Hard reference to imagedef object, code 340
  DRW_Coord vVector;  // V-vector of single pixel, x coordinate, code 12, 22 & 32
  //    double vx;                 // V-vector of single pixel, x coordinate, code 12
  //    double vy;                 // V-vector of single pixel, y coordinate, code 22
  //    double vz;                 // V-vector of single pixel, z coordinate, code 32
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
class DRW_Dimension : public DRW_Entity {
  friend class dxfRW;

 public:
  DRW_Dimension() {
    m_entityType = DRW::DIMENSION;
    type = 0;
    linesty = 1;
    linefactor = extPoint.z = 1.0;
    angle = oblique = rot = 0.0;
    align = 5;
    style = "STANDARD";
    defPoint.z = extPoint.x = extPoint.y = 0;
    textPoint.z = rot = 0;
    clonePoint.x = clonePoint.y = clonePoint.z = 0;
  }

  DRW_Dimension(const DRW_Dimension& d) : DRW_Entity(d) {
    m_entityType = DRW::DIMENSION;
    type = d.type;
    name = d.name;
    defPoint = d.defPoint;
    textPoint = d.textPoint;
    text = d.text;
    style = d.style;
    align = d.align;
    linesty = d.linesty;
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
  virtual ~DRW_Dimension() = default;

  virtual void ApplyExtrusion() {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  DRW_Coord getDefPoint() const { return defPoint; }  // Definition point, code 10, 20 & 30
  void setDefPoint(const DRW_Coord p) { defPoint = p; }
  DRW_Coord getTextPoint() const { return textPoint; }  // Middle point of text, code 11, 21 & 31
  void setTextPoint(const DRW_Coord p) { textPoint = p; }
  std::string getStyle() const { return style; }  // Dimension style, code 3
  void setStyle(const std::string s) { style = s; }
  int getAlign() const { return align; }  // attachment point, code 71
  void setAlign(const int a) { align = a; }
  int getTextLineStyle() const { return linesty; }  // Dimension text line spacing style, code 72, default 1
  void setTextLineStyle(const int l) { linesty = l; }
  std::string getText() const { return text; }  // Dimension text explicitly entered by the user, code 1
  void setText(const std::string t) { text = t; }
  double getTextLineFactor() const { return linefactor; }  // Dimension text line spacing factor, code 41, default 1?
  void setTextLineFactor(const double l) { linefactor = l; }
  double getDir() const { return rot; }  // rotation angle of the dimension text, code 53 (optional) default 0
  void setDir(const double d) { rot = d; }

  DRW_Coord getExtrusion() { return extPoint; }  // extrusion, code 210, 220 & 230
  void setExtrusion(const DRW_Coord p) { extPoint = p; }
  std::string getName() { return name; }  // Name of the block that contains the entities, code 2
  void setName(const std::string s) { name = s; }
  //    int getType(){ return type;}                      // Dimension type, code 70

 protected:
  DRW_Coord getPt2() const { return clonePoint; }
  void setPt2(const DRW_Coord p) { clonePoint = p; }
  DRW_Coord getPt3() const { return def1; }
  void setPt3(const DRW_Coord p) { def1 = p; }
  DRW_Coord getPt4() const { return def2; }
  void setPt4(const DRW_Coord p) { def2 = p; }
  DRW_Coord getPt5() const { return circlePoint; }
  void setPt5(const DRW_Coord p) { circlePoint = p; }
  DRW_Coord getPt6() const { return arcPoint; }
  void setPt6(const DRW_Coord p) { arcPoint = p; }
  double getAn50() const { return angle; }  // Angle of rotated, horizontal, or vertical dimensions, code 50
  void setAn50(const double d) { angle = d; }
  double getOb52() const { return oblique; }  // oblique angle, code 52
  void setOb52(const double d) { oblique = d; }
  double getRa40() const { return length; }  // Leader length, code 40
  void setRa40(const double d) { length = d; }

 public:
  int type;  // Dimension type, code 70
 private:
  std::string name;  // Name of the block that contains the entities, code 2
  DRW_Coord defPoint;  //  definition point, code 10, 20 & 30 (WCS)
  DRW_Coord textPoint;  // Middle point of text, code 11, 21 & 31 (OCS)
  UTF8STRING text;  // Dimension text explicitly entered by the user, code 1
  UTF8STRING style;  // Dimension style, code 3
  int align;  // attachment point, code 71
  int linesty;  // Dimension text line spacing style, code 72, default 1
  double linefactor;  // Dimension text line spacing factor, code 41, default 1? (value range 0.25 to 4.00*/
  double rot;  // rotation angle of the dimension text, code 53
  DRW_Coord extPoint;  //  extrusion normal vector, code 210, 220 & 230

  double hdir{};  // horizontal direction for the dimension, code 51, default ?
  DRW_Coord clonePoint;  // Insertion point for clones (Baseline & Continue), code 12, 22 & 32 (OCS)
  DRW_Coord def1;  // Definition point 1for linear & angular, code 13, 23 & 33 (WCS)
  DRW_Coord def2;  // Definition point 2, code 14, 24 & 34 (WCS)
  double angle;  // Angle of rotated, horizontal, or vertical dimensions, code 50
  double oblique;  // oblique angle, code 52

  DRW_Coord circlePoint;  // Definition point for diameter, radius & angular dims code 15, 25 & 35 (WCS)
  DRW_Coord arcPoint;  // Point defining dimension arc, x coordinate, code 16, 26 & 36 (OCS)
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
class DRW_DimAligned : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimAligned() { m_entityType = DRW::DIMALIGNED; }
  DRW_DimAligned(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMALIGNED; }

  DRW_Coord getClonepoint() const { return getPt2(); }  // Insertion for clones (Baseline & Continue), 12, 22 & 32
  void setClonePoint(DRW_Coord c) { setPt2(c); }

  DRW_Coord getDimPoint() const { return getDefPoint(); }  // dim line location point, code 10, 20 & 30
  void setDimPoint(const DRW_Coord p) { setDefPoint(p); }
  DRW_Coord getDef1Point() const { return getPt3(); }  // Definition point 1, code 13, 23 & 33
  void setDef1Point(const DRW_Coord p) { setPt3(p); }
  DRW_Coord getDef2Point() const { return getPt4(); }  // Definition point 2, code 14, 24 & 34
  void setDef2Point(const DRW_Coord p) { setPt4(p); }
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
class DRW_DimLinear : public DRW_DimAligned {
 public:
  DRW_DimLinear() { m_entityType = DRW::DIMLINEAR; }
  DRW_DimLinear(const DRW_Dimension& d) : DRW_DimAligned(d) { m_entityType = DRW::DIMLINEAR; }

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
class DRW_DimRadial : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimRadial() { m_entityType = DRW::DIMRADIAL; }
  DRW_DimRadial(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMRADIAL; }

  DRW_Coord getCenterPoint() const { return getDefPoint(); }  // center point, code 10, 20 & 30
  void setCenterPoint(const DRW_Coord p) { setDefPoint(p); }
  DRW_Coord getDiameterPoint() const { return getPt5(); }  // Definition point for radius, code 15, 25 & 35
  void setDiameterPoint(const DRW_Coord p) { setPt5(p); }
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
class DRW_DimDiametric : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimDiametric() { m_entityType = DRW::DIMDIAMETRIC; }
  DRW_DimDiametric(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMDIAMETRIC; }

  DRW_Coord getDiameter1Point() const { return getPt5(); }  // First definition point for diameter, code 15, 25 & 35
  void setDiameter1Point(const DRW_Coord p) { setPt5(p); }
  DRW_Coord getDiameter2Point() const { return getDefPoint(); }  // Oposite point for diameter, code 10, 20 & 30
  void setDiameter2Point(const DRW_Coord p) { setDefPoint(p); }
  double getLeaderLength() const { return getRa40(); }  // Leader length, code 40
  void setLeaderLength(const double d) { setRa40(d); }
};

//! Class to handle angular dimension entity
/*!
 *  Class to handle angular dimension entity
 */
class DRW_DimAngular : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimAngular() { m_entityType = DRW::DIMANGULAR; }
  DRW_DimAngular(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMANGULAR; }

  DRW_Coord getFirstLine1() const { return getPt3(); }  // Definition point line 1-1, code 13, 23 & 33
  void setFirstLine1(const DRW_Coord p) { setPt3(p); }
  DRW_Coord getFirstLine2() const { return getPt4(); }  // Definition point line 1-2, code 14, 24 & 34
  void setFirstLine2(const DRW_Coord p) { setPt4(p); }
  DRW_Coord getSecondLine1() const { return getPt5(); }  // Definition point line 2-1, code 15, 25 & 35
  void setSecondLine1(const DRW_Coord p) { setPt5(p); }
  DRW_Coord getSecondLine2() const { return getDefPoint(); }  // Definition point line 2-2, code 10, 20 & 30
  void setSecondLine2(const DRW_Coord p) { setDefPoint(p); }
  DRW_Coord getDimPoint() const { return getPt6(); }  // Dimension definition point, code 16, 26 & 36
  void setDimPoint(const DRW_Coord p) { setPt6(p); }
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
class DRW_DimAngular3p : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimAngular3p() { m_entityType = DRW::DIMANGULAR3P; }
  DRW_DimAngular3p(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMANGULAR3P; }

  DRW_Coord getFirstLine() const { return getPt3(); }  // Definition point line 1, code 13, 23 & 33
  void setFirstLine(const DRW_Coord p) { setPt3(p); }
  DRW_Coord getSecondLine() const { return getPt4(); }  // Definition point line 2, code 14, 24 & 34
  void setSecondLine(const DRW_Coord p) { setPt4(p); }
  DRW_Coord getVertexPoint() const { return getPt5(); }  // Vertex point, code 15, 25 & 35
  void SetVertexPoint(const DRW_Coord p) { setPt5(p); }
  DRW_Coord getDimPoint() const { return getDefPoint(); }  // Dimension definition point, code 10, 20 & 30
  void setDimPoint(const DRW_Coord p) { setDefPoint(p); }
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
class DRW_DimOrdinate : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimOrdinate() { m_entityType = DRW::DIMORDINATE; }
  DRW_DimOrdinate(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMORDINATE; }

  DRW_Coord getOriginPoint() const { return getDefPoint(); }  // Origin definition point, code 10, 20 & 30
  void setOriginPoint(const DRW_Coord p) { setDefPoint(p); }
  DRW_Coord getFirstLine() const { return getPt3(); }  // Feature location point, code 13, 23 & 33
  void setFirstLine(const DRW_Coord p) { setPt3(p); }
  DRW_Coord getSecondLine() const { return getPt4(); }  // Leader end point, code 14, 24 & 34
  void setSecondLine(const DRW_Coord p) { setPt4(p); }
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
class DRW_Leader : public DRW_Entity {
  friend class dxfRW;

 public:
  DRW_Leader() {
    m_entityType = DRW::LEADER;
    flag = 3;
    hookflag = vertnum = leadertype = 0;
    extrusionPoint.x = extrusionPoint.y = 0.0;
    arrow = 1;
    extrusionPoint.z = 1.0;
  }
  ~DRW_Leader() {
    for (DRW_Coord* vert : vertexlist) { delete vert; }
    vertexlist.clear();
  }

  virtual void ApplyExtrusion() {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  UTF8STRING style;  // Dimension style name, code 3
  int arrow;  // Arrowhead flag, code 71, 0=Disabled; 1=Enabled
  int leadertype;  // Leader path type, code 72, 0=Straight line segments; 1=Spline
  int flag;  // Leader creation flag, code 73, default 3
  int hookline{};  // Hook line direction flag, code 74, default 1
  int hookflag;  // Hook line flag, code 75
  double textheight{};  // Text annotation height, code 40
  double textwidth{};  // Text annotation width, code 41
  int vertnum;  // Number of vertices, code 76
  int coloruse{};  // Color to use if leader's DIMCLRD = BYBLOCK, code 77
  std::uint32_t annotHandle{};  // Hard reference to associated annotation, code 340
  DRW_Coord extrusionPoint;  // Normal vector, code 210, 220 & 230
  DRW_Coord horizdir;  // "Horizontal" direction for leader, code 211, 221 & 231
  DRW_Coord offsetblock;  // Offset of last leader vertex from block, code 212, 222 & 232
  DRW_Coord offsettext;  // Offset of last leader vertex from annotation, code 213, 223 & 233

  std::vector<DRW_Coord*> vertexlist;  // vertex points list, code 10, 20 & 30

 private:
  DRW_Coord* vertexpoint{};  // current control point to add data
};

/** @brief Class to handle viewport entity
 *
 *  A viewport entity represents a window in paper space that displays a view of the model space.
 *  It is defined by its width and height in paper space units (code 40 and 41), viewport status (code 68), and viewport
 * ID (code 69). The viewport entity can also include properties such as the view center point (code 12 and 22), snap
 * base point (code 13 and 23), snap spacing (code 14 and 24), view direction vector (code 16, 26, and 36), view target
 * point (code 17, 27, and 37), perspective lens length (code 42), front clip plane Z value (code 43), back clip plane Z
 * value (code 44), view height in model space units (code 45), snap angle (code 50), and view twist angle (code 51),
 * which can affect how the viewport displays the model space.
 */
class DRW_Viewport : public DRW_Point {
  friend class dxfRW;

 public:
  DRW_Viewport() {
    m_entityType = DRW::VIEWPORT;
    vpstatus = 0;
    pswidth = 205;
    psheight = 156;
    centerPX = 128.5;
    centerPY = 97.5;
  }

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  double pswidth;  // Width in paper space units, code 40
  double psheight;  // Height in paper space units, code 41
  int vpstatus;  // Viewport status, code 68
  int vpID{};  // Viewport ID, code 69
  double centerPX;  // view center point X, code 12
  double centerPY;  // view center point Y, code 22
  double snapPX{};  // Snap base point X, code 13
  double snapPY{};  // Snap base point Y, code 23
  double snapSpPX{};  // Snap spacing X, code 14
  double snapSpPY{};  // Snap spacing Y, code 24
  // TODO: complete in dxf
  DRW_Coord viewDir;  // View direction vector, code 16, 26 & 36
  DRW_Coord viewTarget;  // View target point, code 17, 27, 37
  double viewLength{};  // Perspective lens length, code 42
  double frontClip{};  // Front clip plane Z value, code 43
  double backClip{};  // Back clip plane Z value, code 44
  double viewHeight{};  // View height in model space units, code 45
  double snapAngle{};  // Snap angle, code 50
  double twistAngle{};  // view twist angle, code 51

 private:
  std::uint32_t frozenLyCount{};
};