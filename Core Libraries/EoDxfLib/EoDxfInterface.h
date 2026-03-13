#pragma once

#include "EoDxfClasses.h"
#include "EoDxfEntities.h"
#include "EoDxfHeader.h"
#include "EoDxfObjects.h"
#include "EoDxfTables.h"

class EoDxfMLeader;
/**
 * Abstract class (interface) for comunicate EoDxfReader with the application.
 * Inherit your class which takes care of the entities in the
 * processed DXF file from this interface.
 *
 */
class EoDxfInterface {
 public:
  EoDxfInterface() {}
  virtual ~EoDxfInterface() = default;

  /** Called when header is parsed.  */
  virtual void addHeader(const EoDxfHeader* header) = 0;

  /** Called for every class.  */
  virtual void addClass(const EoDxfClass& class_) = 0;

  /** Called for every line Type.  */
  virtual void addLinetype(const EoDxfLinetype& linetype) = 0;
  /** Called for every layer. */
  virtual void addLayer(const EoDxfLayer& layer) = 0;
  /** Called for every dim style. */
  virtual void addDimStyle(const EoDxfDimensionStyle& dimensionStyle) = 0;
  /** Called for every VPORT table. */
  virtual void addVport(const EoDxfVPort& viewport) = 0;
  /** Called for every text style. */
  virtual void addTextStyle(const EoDxfTextStyle& textStyle) = 0;
  /** Called for every AppId entry. */
  virtual void addAppId(const EoDxfAppId& appId) = 0;

  /**
   * Called for every block. Note: all entities added after this
   * command go into this block until endBlock() is called.
   *
   * @see endBlock()
   */
  virtual void addBlock(const EoDxfBlock& block) = 0;

  /**
   * Called when the following entities corresponding to a
   * block different from the current. Note: all entities added after this
   * command go into this block until setBlock() is called already.
   *
   * int handle are the value of EoDxfBlock::handleBlock added with addBlock()
   */
  virtual void setBlock(const int handle) = 0;

  /** Called to end the current block */
  virtual void endBlock() = 0;

  /** Called for every point */
  virtual void addPoint(const EoDxfPoint& point) = 0;

  /** Called for every line */
  virtual void addLine(const EoDxfLine& line) = 0;

  /** Called for every ray */
  virtual void addRay(const EoDxfRay& ray) = 0;

  /** Called for every xline */
  virtual void addXline(const EoDxfXline& xline) = 0;

  /** Called for every arc */
  virtual void addArc(const EoDxfArc& arc) = 0;

  /** Called for every circle */
  virtual void addCircle(const EoDxfCircle& data) = 0;

  /** Called for every ellipse */
  virtual void addEllipse(const EoDxfEllipse& ellipse) = 0;

  /** Called for every lwpolyline */
  virtual void addLWPolyline(const EoDxfLwPolyline& polyline) = 0;

  /** Called for every polyline start */
  virtual void addPolyline(const EoDxfPolyline& polyline) = 0;

  /** Called for every spline */
  virtual void addSpline(const EoDxfSpline* spline) = 0;

  /** Called for every spline knot value */
  virtual void addKnot(const EoDxfEntity& data) = 0;

  /** Called for every insert. */
  virtual void addInsert(const EoDxfInsert& blockReference) = 0;

  /** Called for every trace start */
  virtual void addTrace(const EoDxfTrace& trace) = 0;

  /** Called for every 3dFace start */
  virtual void add3dFace(const EoDxf3dFace& face) = 0;

  /** Called for every solid start */
  virtual void addSolid(const EoDxfSolid& data) = 0;

  /** Called for every Multi Text entity. */
  virtual void addMText(const EoDxfMText& mText) = 0;

  /** Called for every Text entity. */
  virtual void addText(const EoDxfText& text) = 0;

  /**
   * Called for every aligned dimension entity.
   */
  virtual void addDimAlign(const EoDxfAlignedDimension* data) = 0;
  /**
   * Called for every linear or rotated dimension entity.
   */
  virtual void addDimLinear(const EoDxfDimLinear* data) = 0;

  /**
   * Called for every radial dimension entity.
   */
  virtual void addDimRadial(const EoDxfRadialDimension* data) = 0;

  /**
   * Called for every diametric dimension entity.
   */
  virtual void addDimDiametric(const EoDxfDiametricDimension* data) = 0;

  /**
   * Called for every angular dimension (2 lines version) entity.
   */
  virtual void addDimAngular(const EoDxf2LineAngularDimension* data) = 0;

  /**
   * Called for every angular dimension (3 points version) entity.
   */
  virtual void addDimAngular3P(const EoDxf3PointAngularDimension* data) = 0;

  /**
   * Called for every ordinate dimension entity.
   */
  virtual void addDimOrdinate(const EoDxfOrdinateDimension* data) = 0;

  /**
   * Called for every leader start.
   */
  virtual void addLeader(const EoDxfLeader* leader) = 0;

  /**
   * Called for every multileader entity.
   */
  virtual void addMLeader(const EoDxfMLeader* mLeader) = 0;

  /**
   * Called for every hatch entity.
   */
  virtual void addHatch(const EoDxfHatch* data) = 0;

  /**
   * Called for every viewport entity.
   */
  virtual void addViewport(const EoDxfViewport& viewport) = 0;

  /**
   * Called for every image entity.
   */
  virtual void addImage(const EoDxfImage* image) = 0;

  /**
   * Called for every image definition.
   */
  virtual void linkImage(const EoDxfImageDefinition* data) = 0;

  /**
   * Called for unsupported object records that should be preserved for rewrite fidelity.
   */
  virtual void addUnsupportedObject(const EoDxfUnsupportedObject& objectData) = 0;

  /**
   * Called for every comment in the DXF file (code 999).
   */
  virtual void addComment(std::wstring_view comment) = 0;

  virtual void writeHeader(EoDxfHeader& data) = 0;
  virtual void writeClasses() = 0;
  virtual void writeBlocks() = 0;
  virtual void writeBlockRecords() = 0;
  virtual void writeEntities() = 0;
  virtual void writeObjects() = 0;
  virtual void writeUnsupportedObjects() = 0;
  virtual void writeLTypes() = 0;
  virtual void writeLayers() = 0;
  virtual void writeTextstyles() = 0;
  virtual void writeVports() = 0;
  virtual void writeDimstyles() = 0;
  virtual void writeAppId() = 0;
};