#pragma once

#include <cstdint>
#include <list>
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
  //        SEQEND,//not needed?? used in polyline and insert/attrib
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

  /** @brief Calculates the arbitrary extrusion axis (extAxisX and extAxisY) based on the given extrusion direction (extPoint).
   *  This follows the DXF specification for handling extrusion directions and their corresponding axes.
   *  The calculated axes are unitized for use in extrusion transformations.
   *  @param extrusionDirection The extrusion direction vector from the DXF entity, used to calculate the arbitrary axes.
   */
  void CalculateArbitraryAxis(const DRW_Coord& extrusionDirection);

  /** @brief Applies an extrusion transformation to the given point using the pre-calculated arbitrary axes and the extrusion direction.
   * The transformation is defined as: 
   *  P' = (Ax * point.x) + (Ay * point.y) + (N * point.z), where Ax and Ay are the arbitrary axes,
   *  N is the extrusion direction, and point is the original coordinate.
   *  The result is stored back in the provided point reference.
   *
   * @param extrusionDirection The extrusion direction vector (N) used in the transformation.
   * @param[out] point A DRW_Coord representing the original point to be transformed. The transformed coordinates will be stored back in this variable.
   */
  void ExtrudePoint(const DRW_Coord& extrusionDirection, DRW_Coord& point) const noexcept;

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
  DRW_Coord m_secondPoint;  // code 11, 21 & 31
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
  [[nodiscard]] double Radius() const { return m_radius; }
  // start angle in radians
  [[nodiscard]] double StartAngle() const { return m_startAngle; }
  // end angle in radians
  [[nodiscard]] double EndAngle() const { return m_endAngle; }
  // thickness
  [[nodiscard]] double Thickness() const { return m_thickness; }
  // extrusion
  [[nodiscard]] const DRW_Coord& ExtrusionDirection() const { return m_extrusionDirection; }

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
  double m_ratio{};  // code 40
  double m_startParam{};  // code 41, 0.0 for full ellipse
  double m_endParam{};  // code 42, 2*PI for full ellipse
  int m_isCounterClockwise{1};  // code 73 (only used in hatch)
};

/** @@brief Class to handle trace entity
 * 
 *  A trace is a four-sided polyline that is always planar. 
 *  It is defined by its four corner points, which are specified in the DXF file using group codes 10, 20, 30 for the first point; 
 *  11, 21, 31 for the second point; 12, 22, 32 for the third point; and 13, 23, 33 for the fourth point. 
 *  The trace entity can also include properties such as thickness and extrusion direction, which can affect how it is rendered in 3D space.
 */
class DRW_Trace : public DRW_Line {
  friend class dxfRW;

 public:
  explicit DRW_Trace(DRW::ETYPE entityType = DRW::TRACE) noexcept : DRW_Line{entityType} {}

  void ApplyExtrusion() override;

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  DRW_Coord m_thirdPoint{};  // code 12, 22 & 32
  DRW_Coord m_fourthPoint{};  // code 13, 23 & 33
};

/** @brief Class to handle solid entity
 * 
 *  A solid is a four-sided polyline that is always planar and filled. 
 *  It is defined by its four corner points, which are specified in the DXF file using group codes 10, 20, 30 for the first point; 
 *  11, 21, 31 for the second point; 12, 22, 32 for the third point; and 13, 23, 33 for the fourth point. 
 *  The solid entity can also include properties such as thickness and extrusion direction, which can affect how it is rendered in 3D space.
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
 *  The entity can also include properties such as thickness and extrusion direction, which can affect how it is rendered in 3D space.
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
  int m_invisibleFlag{};  // code 70
};

/** @brief Class to handle block entries
 * 
 *  A block is a collection of entities that are grouped together and can be inserted into a drawing as a single unit. 
 *  It is defined by its name, which is specified in the DXF file using group code 2, and its type, which is specified using group code 70. 
 *  The block entity can also include properties such as layer, line type, and extrusion direction, which can affect how it is rendered in the drawing.
 */
class DRW_Block : public DRW_Point {
  friend class dxfRW;

 public:
  explicit DRW_Block(DRW::ETYPE entityType = DRW::BLOCK) noexcept : DRW_Point{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  UTF8STRING name{"*U0"};  // code 2
  int flags{};  // code 70
};

/** @brief Class to handle insert entries
 * 
 *  An insert is an instance of a block that is placed in a drawing. 
 *  It is defined by the name of the block it references, which is specified in the DXF file using group code 2, and its insertion point, which is specified using group codes 10, 20, and 30. 
 *  The insert entity can also include properties such as scale factors (code 41, 42, 43), rotation angle (code 50), and number of columns and rows for array inserts (code 70 and 71), which can affect how it is rendered in the drawing.
 */
class DRW_Insert : public DRW_Point {
  friend class dxfRW;

 public:
  explicit DRW_Insert(DRW::ETYPE entityType = DRW::INSERT) noexcept : DRW_Point{entityType} {}

  void ApplyExtrusion() override { DRW_Point::ApplyExtrusion(); }

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  UTF8STRING name;  // code 2
  double xscale{1.0};  // code 41
  double yscale{1.0};  // code 42
  double zscale{1.0};  // code 43
  double angle{};  // code 50 (in radians)
  int colcount{1};  // code 70
  int rowcount{1};  // code 71
  double colspace{};  // code 44
  double rowspace{};  // code 45
};

//! Class to handle lwpolyline entity
/*!
*  Class to handle lwpolyline entity
*/
class DRW_LWPolyline : public DRW_Entity {
  friend class dxfRW;

 public:
  DRW_LWPolyline() {
    m_entityType = DRW::LWPOLYLINE;
    elevation = thickness = width = 0.0;
    flags = 0;
    extPoint.x = extPoint.y = 0;
    extPoint.z = 1;
    vertex = nullptr;
  }

  DRW_LWPolyline(const DRW_LWPolyline& p) : DRW_Entity(p) {
    this->m_entityType = DRW::LWPOLYLINE;
    this->elevation = p.elevation;
    this->thickness = p.thickness;
    this->width = p.width;
    this->flags = p.flags;
    this->extPoint = p.extPoint;
    this->vertex = nullptr;
    for (unsigned i = 0; i < p.vertlist.size(); i++) {
      this->vertlist.push_back(new DRW_Vertex2D(*(p.vertlist.at(i))));
    }
    this->vertex = nullptr;
  }

  ~DRW_LWPolyline() {
    for (auto* vert : vertlist) { delete vert; }
    vertlist.clear();
    /* @bugfix TAS 2026-02-03: memory leak fix - pop_back does not delete the pointers
    while (!vertlist.empty()) {
      vertlist.pop_back();
    }
    */
  }
  void ApplyExtrusion() override;

  void addVertex(DRW_Vertex2D v) {
    auto* vert = new DRW_Vertex2D();
    vert->x = v.x;
    vert->y = v.y;
    vert->stawidth = v.stawidth;
    vert->endwidth = v.endwidth;
    vert->bulge = v.bulge;
    vertlist.push_back(vert);
  }

  DRW_Vertex2D* addVertex() {
    auto* vert = new DRW_Vertex2D();
    vertlist.push_back(vert);
    return vert;
  }

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  int vertexnum{}; /*!< number of vertex, code 90 */
  int flags; /*!< polyline flag, code 70, default 0 */
  double width; /*!< constant width, code 43 */
  double elevation; /*!< elevation, code 38 */
  double thickness; /*!< thickness, code 39 */
  DRW_Coord extPoint; /*!<  Dir extrusion normal vector, code 210, 220 & 230 */
  DRW_Vertex2D* vertex; /*!< current vertex to add data */
  std::vector<DRW_Vertex2D*> vertlist; /*!< vertex list */
};

/** @brief Class to handle text entity
 * 
 *  A text entity represents a single line of text in a drawing. 
 *  It is defined by its insertion point (code 10, 20, 30), height (code 40), and the text string itself (code 1). 
 *  The text entity can also include properties such as rotation angle (code 50), width scale factor (code 41), oblique angle (code 51), and text style name (code 7), which can affect how the text is rendered in the drawing.
 */
class DRW_Text : public DRW_Line {
  friend class dxfRW;

 public:
  
  enum VAlign {
    VBaseLine = 0,
    VBottom,
    VMiddle,
    VTop
  };

  enum HAlign {
    HLeft = 0,
    HCenter,
    HRight,
    HAligned, // = 3 (if VAlign is 0)
    HMiddle, // = 4 (if VAlign is 0) */
    HFit // fit into point = 5 (if VAlign is 0)
  };

  explicit DRW_Text(DRW::ETYPE entityType = DRW::TEXT) noexcept : DRW_Line{entityType} {}

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  double m_textHeight{};  // code 40
  UTF8STRING m_string;  // code 1
  double m_textRotation{};  // code 50
  double m_scaleFactorWidth{1.0};  // code 41
  double m_obliqueAngle{};  // code 51
  UTF8STRING m_textStyleName{"STANDARD"};  // code 7
  int m_textGenerationFlags{};  // code 71
  enum HAlign m_horizontalAlignment { HLeft };  // code 72
  enum VAlign m_verticalAlignment { VBaseLine };  // code 73
};

/** @brief Class to handle mtext entity
 * 
 *  An mtext entity represents multiline text in a drawing. 
 *  It is defined by its insertion point (code 10, 20, 30), height (code 40), and the text string itself (code 1). 
 *  The mtext entity can also include properties such as rotation angle (code 50), width scale factor (code 41), oblique angle (code 51), and text style name (code 7), which can affect how the text is rendered in the drawing. 
 *  Additionally, mtext supports more complex formatting options such as multiple lines of text, different alignments, and special characters.
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
  void UpdateAngle();  //recalculate angle if 'm_haveXAxisDirection' is true

 public:
  double m_lineSpacingFactor{1.0};  // code 44 (optional)
 private:
  bool m_haveXAxisDirection{};
};

class DRW_Vertex : public DRW_Point {
  friend class dxfRW;

 public:
  DRW_Vertex() {
    m_entityType = DRW::VERTEX;
    stawidth = endwidth = bulge = 0;
    vindex1 = vindex2 = vindex3 = vindex4 = 0;
    flags = identifier = 0;
  }
  DRW_Vertex(double sx, double sy, double sz, double b) {
    stawidth = endwidth = 0;
    vindex1 = vindex2 = vindex3 = vindex4 = 0;
    flags = identifier = 0;
    m_firstPoint.x = sx;
    m_firstPoint.y = sy;
    m_firstPoint.z = sz;
    bulge = b;
  }

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  double stawidth; /*!< Start width, code 40 */
  double endwidth; /*!< End width, code 41 */
  double bulge; /*!< bulge, code 42 */

  int flags; /*!< vertex flag, code 70, default 0 */
  double tgdir{}; /*!< curve fit tangent direction, code 50 */
  int vindex1; /*!< polyface mesh vertex index, code 71, default 0 */
  int vindex2; /*!< polyface mesh vertex index, code 72, default 0 */
  int vindex3; /*!< polyface mesh vertex index, code 73, default 0 */
  int vindex4; /*!< polyface mesh vertex index, code 74, default 0 */
  int identifier; /*!< vertex identifier, code 91, default 0 */
};

class DRW_Polyline : public DRW_Point {
  friend class dxfRW;

 public:
  DRW_Polyline() {
    m_entityType = DRW::POLYLINE;
    defstawidth = defendwidth = 0.0;
    m_firstPoint.x = m_firstPoint.y = 0.0;
    flags = vertexcount = facecount = 0;
    smoothM = smoothN = curvetype = 0;
  }
  ~DRW_Polyline() {
    for (DRW_Vertex* vert : vertlist) { delete vert; }
    vertlist.clear();
    /* @bugfix TAS 2026-02-03: memory leak fix - pop_back does not delete pointers
    while (!vertlist.empty()) {
      vertlist.pop_back();
    }
    */
  }
  void addVertex(DRW_Vertex v) {
    DRW_Vertex* vert = new DRW_Vertex();
    vert->m_firstPoint.x = v.m_firstPoint.x;
    vert->m_firstPoint.y = v.m_firstPoint.y;
    vert->m_firstPoint.z = v.m_firstPoint.z;
    vert->stawidth = v.stawidth;
    vert->endwidth = v.endwidth;
    vert->bulge = v.bulge;
    vertlist.push_back(vert);
  }
  void appendVertex(DRW_Vertex* v) { vertlist.push_back(v); }

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  int flags; /*!< polyline flag, code 70, default 0 */
  double defstawidth; /*!< Start width, code 40, default 0 */
  double defendwidth; /*!< End width, code 41, default 0 */
  int vertexcount; /*!< polygon mesh M vertex or  polyface vertex num, code 71, default 0 */
  int facecount; /*!< polygon mesh N vertex or  polyface face num, code 72, default 0 */
  int smoothM; /*!< smooth surface M density, code 73, default 0 */
  int smoothN; /*!< smooth surface M density, code 74, default 0 */
  int curvetype; /*!< curves & smooth surface type, code 75, default 0 */

  std::vector<DRW_Vertex*> vertlist; /*!< vertex list */
};

//! Class to handle spline entity
/*!
*  Class to handle spline entity
*/
class DRW_Spline : public DRW_Entity {
  friend class dxfRW;

 public:
  DRW_Spline() {
    m_entityType = DRW::SPLINE;
    flags = nknots = ncontrol = nfit = 0;
    tolknot = tolcontrol = tolfit = 0.0000001;
  }
  ~DRW_Spline() {
    for (DRW_Coord* pt : controllist) { delete pt; }
    controllist.clear();
    for (DRW_Coord* pt : fitlist) { delete pt; }
    fitlist.clear();

    /* @bugfix TAS 2026-02-03: memory leak fix - pop_back does not delete pointers
    while (!controllist.empty()) {
      controllist.pop_back();
    }
    while (!fitlist.empty()) {
      fitlist.pop_back();
    }
    */
  }
  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  //    double ex;                /*!< normal vector x coordinate, code 210 */
  //    double ey;                /*!< normal vector y coordinate, code 220 */
  //    double ez;                /*!< normal vector z coordinate, code 230 */
  DRW_Coord normalVec; /*!< normal vector, code 210, 220, 230 */
  DRW_Coord tgStart; /*!< start tangent, code 12, 22, 32 */
  //    double tgsx;              /*!< start tangent x coordinate, code 12 */
  //    double tgsy;              /*!< start tangent y coordinate, code 22 */
  //    double tgsz;              /*!< start tangent z coordinate, code 32 */
  DRW_Coord tgEnd; /*!< end tangent, code 13, 23, 33 */
  //    double tgex;              /*!< end tangent x coordinate, code 13 */
  //    double tgey;              /*!< end tangent y coordinate, code 23 */
  //    double tgez;              /*!< end tangent z coordinate, code 33 */
  int flags; /*!< spline flag, code 70 */
  int degree{}; /*!< degree of the spline, code 71 */
  std::int32_t nknots; /*!< number of knots, code 72, default 0 */
  std::int32_t ncontrol; /*!< number of control points, code 73, default 0 */
  std::int32_t nfit; /*!< number of fit points, code 74, default 0 */
  double tolknot; /*!< knot tolerance, code 42, default 0.0000001 */
  double tolcontrol; /*!< control point tolerance, code 43, default 0.0000001 */
  double tolfit; /*!< fit point tolerance, code 44, default 0.0000001 */

  std::vector<double> knotslist; /*!< knots list, code 40 */
  std::vector<DRW_Coord*> controllist; /*!< control points list, code 10, 20 & 30 */
  std::vector<DRW_Coord*> fitlist; /*!< fit points list, code 11, 21 & 31 */

 private:
  DRW_Coord* controlpoint{}; /*!< current control point to add data */
  DRW_Coord* fitpoint{}; /*!< current fit point to add data */
};

//! Class to handle hatch loop
/*!
*  Class to handle hatch loop
*/
class DRW_HatchLoop {
 public:
  DRW_HatchLoop(int t) {
    type = t;
    numedges = 0;
  }

  ~DRW_HatchLoop() {
    for (DRW_Entity* obj : objlist) { delete obj; }
    objlist.clear();
    /* TAS 2026-02-03: memory leak fix - pop_back does not delete pointers
    while (!objlist.empty()) {
      objlist.pop_back();
    }
    */
  }

  void update() { numedges = static_cast<int>(objlist.size()); }

 public:
  int type; /*!< boundary path type, code 92, polyline=2, default=0 */
  int numedges; /*!< number of edges (if not a polyline), code 93 */
  //TODO: store lwpolylines as entities
  //    std::vector<DRW_LWPolyline *> pollist;  /*!< polyline list */
  std::vector<DRW_Entity*> objlist; /*!< entities list */
};

//! Class to handle hatch entity
/*!
*  Class to handle hatch entity
*/
//TODO: handle lwpolylines, splines and ellipses
class DRW_Hatch : public DRW_Point {
  friend class dxfRW;

 public:
  DRW_Hatch() {
    m_entityType = DRW::HATCH;
    angle = scale = 0.0;
    m_firstPoint.x = m_firstPoint.y = m_firstPoint.z = 0.0;
    loopsnum = hstyle = associative = 0;
    solid = hpattern = 1;
    deflines = doubleflag = 0;
    loop = nullptr;
    clearEntities();
  }

  ~DRW_Hatch() {
    for (DRW_HatchLoop* lp : looplist) { delete lp; }
    looplist.clear();
    /* TAS 2026-02-03: memory leak fix - pop_back does not delete pointers
    while (!looplist.empty()) {
      looplist.pop_back();
    }
    */
  }

  void appendLoop(DRW_HatchLoop* v) { looplist.push_back(v); }

  void ApplyExtrusion() override {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  UTF8STRING name; /*!< hatch pattern name, code 2 */
  int solid; /*!< solid fill flag, code 70, solid=1, pattern=0 */
  int associative; /*!< associativity, code 71, associatve=1, non-assoc.=0 */
  int hstyle; /*!< hatch style, code 75 */
  int hpattern; /*!< hatch pattern type, code 76 */
  int doubleflag; /*!< hatch pattern double flag, code 77, double=1, single=0 */
  int loopsnum; /*!< namber of boundary paths (loops), code 91 */
  double angle; /*!< hatch pattern angle, code 52 */
  double scale; /*!< hatch pattern scale, code 41 */
  int deflines; /*!< number of pattern definition lines, code 78 */

  std::vector<DRW_HatchLoop*> looplist; /*!< polyline list */

 private:
  void clearEntities() {
    pt = line = nullptr;
    m_polyline = nullptr;
    arc = nullptr;
    ellipse = nullptr;
    spline = nullptr;
    plvert = nullptr;
  }

  void addLine() {
    clearEntities();
    if (loop) {
      pt = line = new DRW_Line;
      loop->objlist.push_back(line);
    }
  }

  void addArc() {
    clearEntities();
    if (loop) {
      pt = arc = new DRW_Arc;
      loop->objlist.push_back(arc);
    }
  }

  void addEllipse() {
    clearEntities();
    if (loop) {
      pt = ellipse = new DRW_Ellipse;
      loop->objlist.push_back(ellipse);
    }
  }

  void addSpline() {
    clearEntities();
    if (loop) {
      pt = nullptr;
      spline = new DRW_Spline;
      loop->objlist.push_back(spline);
    }
  }

  DRW_HatchLoop* loop; /*!< current loop to add data */
  DRW_Line* line;
  DRW_Arc* arc;
  DRW_Ellipse* ellipse;
  DRW_Spline* spline;
  DRW_LWPolyline* m_polyline;
  DRW_Point* pt;
  DRW_Vertex2D* plvert;
  bool m_isPolyline{};
};

//! Class to handle image entity
/*!
*  Class to handle image entity
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
  std::uint32_t ref{}; /*!< Hard reference to imagedef object, code 340 */
  DRW_Coord vVector; /*!< V-vector of single pixel, x coordinate, code 12, 22 & 32 */
  //    double vx;                 /*!< V-vector of single pixel, x coordinate, code 12 */
  //    double vy;                 /*!< V-vector of single pixel, y coordinate, code 22 */
  //    double vz;                 /*!< V-vector of single pixel, z coordinate, code 32 */
  double sizeu{}; /*!< image size in pixels, U value, code 13 */
  double sizev{}; /*!< image size in pixels, V value, code 23 */
  double dz{}; /*!< z coordinate, code 33 */
  int clip; /*!< Clipping state, code 280, 0=off 1=on */
  int brightness; /*!< Brightness value, code 281, (0-100) default 50 */
  int contrast; /*!< Brightness value, code 282, (0-100) default 50 */
  int fade; /*!< Brightness value, code 283, (0-100) default 0 */
};

//! Base class for dimension entity
/*!
*  Base class for dimension entity
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
  DRW_Coord getDefPoint() const { return defPoint; } /*!< Definition point, code 10, 20 & 30 */
  void setDefPoint(const DRW_Coord p) { defPoint = p; }
  DRW_Coord getTextPoint() const { return textPoint; } /*!< Middle point of text, code 11, 21 & 31 */
  void setTextPoint(const DRW_Coord p) { textPoint = p; }
  std::string getStyle() const { return style; } /*!< Dimension style, code 3 */
  void setStyle(const std::string s) { style = s; }
  int getAlign() const { return align; } /*!< attachment point, code 71 */
  void setAlign(const int a) { align = a; }
  int getTextLineStyle() const { return linesty; } /*!< Dimension text line spacing style, code 72, default 1 */
  void setTextLineStyle(const int l) { linesty = l; }
  std::string getText() const { return text; } /*!< Dimension text explicitly entered by the user, code 1 */
  void setText(const std::string t) { text = t; }
  double getTextLineFactor() const {
    return linefactor;
  } /*!< Dimension text line spacing factor, code 41, default 1? */
  void setTextLineFactor(const double l) { linefactor = l; }
  double getDir() const { return rot; } /*!< rotation angle of the dimension text, code 53 (optional) default 0 */
  void setDir(const double d) { rot = d; }

  DRW_Coord getExtrusion() { return extPoint; } /*!< extrusion, code 210, 220 & 230 */
  void setExtrusion(const DRW_Coord p) { extPoint = p; }
  std::string getName() { return name; } /*!< Name of the block that contains the entities, code 2 */
  void setName(const std::string s) { name = s; }
  //    int getType(){ return type;}                      /*!< Dimension type, code 70 */

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
  double getAn50() const { return angle; } /*!< Angle of rotated, horizontal, or vertical dimensions, code 50 */
  void setAn50(const double d) { angle = d; }
  double getOb52() const { return oblique; } /*!< oblique angle, code 52 */
  void setOb52(const double d) { oblique = d; }
  double getRa40() const { return length; } /*!< Leader length, code 40 */
  void setRa40(const double d) { length = d; }

 public:
  int type; /*!< Dimension type, code 70 */
 private:
  std::string name; /*!< Name of the block that contains the entities, code 2 */
  DRW_Coord defPoint; /*!<  definition point, code 10, 20 & 30 (WCS) */
  DRW_Coord textPoint; /*!< Middle point of text, code 11, 21 & 31 (OCS) */
  UTF8STRING text; /*!< Dimension text explicitly entered by the user, code 1 */
  UTF8STRING style; /*!< Dimension style, code 3 */
  int align; /*!< attachment point, code 71 */
  int linesty; /*!< Dimension text line spacing style, code 72, default 1 */
  double linefactor; /*!< Dimension text line spacing factor, code 41, default 1? (value range 0.25 to 4.00*/
  double rot; /*!< rotation angle of the dimension text, code 53 */
  DRW_Coord extPoint; /*!<  extrusion normal vector, code 210, 220 & 230 */

  double hdir{}; /*!< horizontal direction for the dimension, code 51, default ? */
  DRW_Coord clonePoint; /*!< Insertion point for clones (Baseline & Continue), code 12, 22 & 32 (OCS) */
  DRW_Coord def1; /*!< Definition point 1for linear & angular, code 13, 23 & 33 (WCS) */
  DRW_Coord def2; /*!< Definition point 2, code 14, 24 & 34 (WCS) */
  double angle; /*!< Angle of rotated, horizontal, or vertical dimensions, code 50 */
  double oblique; /*!< oblique angle, code 52 */

  DRW_Coord circlePoint; /*!< Definition point for diameter, radius & angular dims code 15, 25 & 35 (WCS) */
  DRW_Coord arcPoint; /*!< Point defining dimension arc, x coordinate, code 16, 26 & 36 (OCS) */
  double length{}; /*!< Leader length, code 40 */
};

//! Class to handle  aligned dimension entity
/*!
*  Class to handle aligned dimension entity
*/
class DRW_DimAligned : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimAligned() { m_entityType = DRW::DIMALIGNED; }
  DRW_DimAligned(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMALIGNED; }

  DRW_Coord getClonepoint() const { return getPt2(); } /*!< Insertion for clones (Baseline & Continue), 12, 22 & 32 */
  void setClonePoint(DRW_Coord c) { setPt2(c); }

  DRW_Coord getDimPoint() const { return getDefPoint(); } /*!< dim line location point, code 10, 20 & 30 */
  void setDimPoint(const DRW_Coord p) { setDefPoint(p); }
  DRW_Coord getDef1Point() const { return getPt3(); } /*!< Definition point 1, code 13, 23 & 33 */
  void setDef1Point(const DRW_Coord p) { setPt3(p); }
  DRW_Coord getDef2Point() const { return getPt4(); } /*!< Definition point 2, code 14, 24 & 34 */
  void setDef2Point(const DRW_Coord p) { setPt4(p); }
};

//! Class to handle  linear or rotated dimension entity
/*!
*  Class to handle linear or rotated dimension entity
*/
class DRW_DimLinear : public DRW_DimAligned {
 public:
  DRW_DimLinear() { m_entityType = DRW::DIMLINEAR; }
  DRW_DimLinear(const DRW_Dimension& d) : DRW_DimAligned(d) { m_entityType = DRW::DIMLINEAR; }

  double getAngle() const { return getAn50(); } /*!< Angle of rotated, horizontal, or vertical dimensions, code 50 */
  void setAngle(const double d) { setAn50(d); }
  double getOblique() const { return getOb52(); } /*!< oblique angle, code 52 */
  void setOblique(const double d) { setOb52(d); }
};

//! Class to handle radial dimension entity
/*!
*  Class to handle aligned, linear or rotated dimension entity
*/
class DRW_DimRadial : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimRadial() { m_entityType = DRW::DIMRADIAL; }
  DRW_DimRadial(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMRADIAL; }

  DRW_Coord getCenterPoint() const { return getDefPoint(); } /*!< center point, code 10, 20 & 30 */
  void setCenterPoint(const DRW_Coord p) { setDefPoint(p); }
  DRW_Coord getDiameterPoint() const { return getPt5(); } /*!< Definition point for radius, code 15, 25 & 35 */
  void setDiameterPoint(const DRW_Coord p) { setPt5(p); }
  double getLeaderLength() const { return getRa40(); } /*!< Leader length, code 40 */
  void setLeaderLength(const double d) { setRa40(d); }
};

//! Class to handle radial dimension entity
/*!
*  Class to handle aligned, linear or rotated dimension entity
*/
class DRW_DimDiametric : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimDiametric() { m_entityType = DRW::DIMDIAMETRIC; }
  DRW_DimDiametric(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMDIAMETRIC; }

  DRW_Coord getDiameter1Point() const { return getPt5(); } /*!< First definition point for diameter, code 15, 25 & 35 */
  void setDiameter1Point(const DRW_Coord p) { setPt5(p); }
  DRW_Coord getDiameter2Point() const { return getDefPoint(); } /*!< Oposite point for diameter, code 10, 20 & 30 */
  void setDiameter2Point(const DRW_Coord p) { setDefPoint(p); }
  double getLeaderLength() const { return getRa40(); } /*!< Leader length, code 40 */
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

  DRW_Coord getFirstLine1() const { return getPt3(); } /*!< Definition point line 1-1, code 13, 23 & 33 */
  void setFirstLine1(const DRW_Coord p) { setPt3(p); }
  DRW_Coord getFirstLine2() const { return getPt4(); } /*!< Definition point line 1-2, code 14, 24 & 34 */
  void setFirstLine2(const DRW_Coord p) { setPt4(p); }
  DRW_Coord getSecondLine1() const { return getPt5(); } /*!< Definition point line 2-1, code 15, 25 & 35 */
  void setSecondLine1(const DRW_Coord p) { setPt5(p); }
  DRW_Coord getSecondLine2() const { return getDefPoint(); } /*!< Definition point line 2-2, code 10, 20 & 30 */
  void setSecondLine2(const DRW_Coord p) { setDefPoint(p); }
  DRW_Coord getDimPoint() const { return getPt6(); } /*!< Dimension definition point, code 16, 26 & 36 */
  void setDimPoint(const DRW_Coord p) { setPt6(p); }
};

//! Class to handle angular 3p dimension entity
/*!
*  Class to handle angular 3p dimension entity
*/
class DRW_DimAngular3p : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimAngular3p() { m_entityType = DRW::DIMANGULAR3P; }
  DRW_DimAngular3p(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMANGULAR3P; }

  DRW_Coord getFirstLine() const { return getPt3(); } /*!< Definition point line 1, code 13, 23 & 33 */
  void setFirstLine(const DRW_Coord p) { setPt3(p); }
  DRW_Coord getSecondLine() const { return getPt4(); } /*!< Definition point line 2, code 14, 24 & 34 */
  void setSecondLine(const DRW_Coord p) { setPt4(p); }
  DRW_Coord getVertexPoint() const { return getPt5(); } /*!< Vertex point, code 15, 25 & 35 */
  void SetVertexPoint(const DRW_Coord p) { setPt5(p); }
  DRW_Coord getDimPoint() const { return getDefPoint(); } /*!< Dimension definition point, code 10, 20 & 30 */
  void setDimPoint(const DRW_Coord p) { setDefPoint(p); }
};

//! Class to handle ordinate dimension entity
/*!
*  Class to handle ordinate dimension entity
*/
class DRW_DimOrdinate : public DRW_Dimension {
  friend class dxfRW;

 public:
  DRW_DimOrdinate() { m_entityType = DRW::DIMORDINATE; }
  DRW_DimOrdinate(const DRW_Dimension& d) : DRW_Dimension(d) { m_entityType = DRW::DIMORDINATE; }

  DRW_Coord getOriginPoint() const { return getDefPoint(); } /*!< Origin definition point, code 10, 20 & 30 */
  void setOriginPoint(const DRW_Coord p) { setDefPoint(p); }
  DRW_Coord getFirstLine() const { return getPt3(); } /*!< Feature location point, code 13, 23 & 33 */
  void setFirstLine(const DRW_Coord p) { setPt3(p); }
  DRW_Coord getSecondLine() const { return getPt4(); } /*!< Leader end point, code 14, 24 & 34 */
  void setSecondLine(const DRW_Coord p) { setPt4(p); }
};

//! Class to handle leader entity
/*!
*  Class to handle leader entity
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
    /* TAS 2026-02-03: memory leak fix - pop_back does not delete pointers
    while (!vertexlist.empty()) {
      vertexlist.pop_back();
    }
    */
  }

  virtual void ApplyExtrusion() {}

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  UTF8STRING style; /*!< Dimension style name, code 3 */
  int arrow; /*!< Arrowhead flag, code 71, 0=Disabled; 1=Enabled */
  int leadertype; /*!< Leader path type, code 72, 0=Straight line segments; 1=Spline */
  int flag; /*!< Leader creation flag, code 73, default 3 */
  int hookline{}; /*!< Hook line direction flag, code 74, default 1 */
  int hookflag; /*!< Hook line flag, code 75 */
  double textheight{}; /*!< Text annotation height, code 40 */
  double textwidth{}; /*!< Text annotation width, code 41 */
  int vertnum; /*!< Number of vertices, code 76 */
  int coloruse{}; /*!< Color to use if leader's DIMCLRD = BYBLOCK, code 77 */
  std::uint32_t annotHandle{}; /*!< Hard reference to associated annotation, code 340 */
  DRW_Coord extrusionPoint; /*!< Normal vector, code 210, 220 & 230 */
  DRW_Coord horizdir; /*!< "Horizontal" direction for leader, code 211, 221 & 231 */
  DRW_Coord offsetblock; /*!< Offset of last leader vertex from block, code 212, 222 & 232 */
  DRW_Coord offsettext; /*!< Offset of last leader vertex from annotation, code 213, 223 & 233 */

  std::vector<DRW_Coord*> vertexlist; /*!< vertex points list, code 10, 20 & 30 */

 private:
  DRW_Coord* vertexpoint{}; /*!< current control point to add data */
};

//! Class to handle viewport entity
/*!
*  Class to handle viewport entity
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
  double pswidth; /*!< Width in paper space units, code 40 */
  double psheight; /*!< Height in paper space units, code 41 */
  int vpstatus; /*!< Viewport status, code 68 */
  int vpID{}; /*!< Viewport ID, code 69 */
  double centerPX; /*!< view center point X, code 12 */
  double centerPY; /*!< view center point Y, code 22 */
  double snapPX{}; /*!< Snap base point X, code 13 */
  double snapPY{}; /*!< Snap base point Y, code 23 */
  double snapSpPX{}; /*!< Snap spacing X, code 14 */
  double snapSpPY{}; /*!< Snap spacing Y, code 24 */
  //TODO: complete in dxf
  DRW_Coord viewDir; /*!< View direction vector, code 16, 26 & 36 */
  DRW_Coord viewTarget; /*!< View target point, code 17, 27, 37 */
  double viewLength{}; /*!< Perspective lens length, code 42 */
  double frontClip{}; /*!< Front clip plane Z value, code 43 */
  double backClip{}; /*!< Back clip plane Z value, code 44 */
  double viewHeight{}; /*!< View height in model space units, code 45 */
  double snapAngle{}; /*!< Snap angle, code 50 */
  double twistAngle{}; /*!< view twist angle, code 51 */

 private:
  std::uint32_t frozenLyCount{};
};