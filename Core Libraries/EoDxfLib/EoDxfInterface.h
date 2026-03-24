#pragma once

#include "EoDxfAttributes.h"
#include "EoDxfClasses.h"
#include "EoDxfDimension.h"
#include "EoDxfEntities.h"
#include "EoDxfHatch.h"
#include "EoDxfHeader.h"
#include "EoDxfObjects.h"
#include "EoDxfTables.h"
#include "EoDxfText.h"

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
  virtual void AddHeader(const EoDxfHeader* header) = 0;

  /** Called for every class.  */
  virtual void AddClass(const EoDxfClass& class_) = 0;

  /** Called for every line Type.  */
  virtual void AddLinetype(const EoDxfLinetype& linetype) = 0;
  /** Called for every layer. */
  virtual void AddLayer(const EoDxfLayer& layer) = 0;
  /** Called for every dim style. */
  virtual void AddDimStyle(const EoDxfDimensionStyle& dimensionStyle) = 0;
  /** Called for every VPORT table. */
  virtual void AddVport(const EoDxfVPort& viewport) = 0;
  /** Called for every text style. */
  virtual void AddTextStyle(const EoDxfTextStyle& textStyle) = 0;
  /** Called for every AppId entry. */
  virtual void AddAppId(const EoDxfAppId& appId) = 0;
  /**
   * Called for every block. Note: all entities added after this
   * command go into this block until endBlock() is called.
   *
   * @see endBlock()
   */
  virtual void AddBlock(const EoDxfBlock& block) = 0;

  /**
   * Called when the following entities corresponding to a
   * block different from the current. Note: all entities added after this
   * command go into this block until setBlock() is called already.
   *
   * int handle are the value of EoDxfBlock::handleBlock added with AddBlock()
   */
  virtual void SetBlock(const int handle) = 0;

  /** Called to end the current block */
  virtual void EndBlock() = 0;

  /** Called for every 3dFace start */
  virtual void Add3dFace(const EoDxf3dFace& _3dFace) = 0;

  /** Called for every acad proxy entity */
  virtual void AddAcadProxyEntity(const EoDxfAcadProxyEntity& proxyEntity) = 0;

  /** Called for every arc */
  virtual void AddArc(const EoDxfArc& arc) = 0;

  /** Called for every attribute definition */
  virtual void AddAttDef(const EoDxfAttDef& attdef) = 0;

  /** Called for every attribute */
  virtual void AddAttrib(const EoDxfAttrib& attrib) = 0;

  /** Called for every circle */
  virtual void AddCircle(const EoDxfCircle& data) = 0;

  /** Called for every aligned dimension entity. */
  virtual void AddDimAlign(const EoDxfAlignedDimension* data) = 0;

  /** Called for every angular dimension (2 lines version) entity. */
  virtual void AddDimAngular(const EoDxf2LineAngularDimension* data) = 0;

  /** Called for every angular dimension (3 points version) entity. */
  virtual void AddDimAngular3P(const EoDxf3PointAngularDimension* data) = 0;

  /** Called for every diametric dimension entity. */
  virtual void AddDimDiametric(const EoDxfDiametricDimension* data) = 0;

  /** Called for every linear or rotated dimension entity. */
  virtual void AddDimLinear(const EoDxfDimLinear* data) = 0;

  /** Called for every ordinate dimension entity. */
  virtual void AddDimOrdinate(const EoDxfOrdinateDimension* data) = 0;

  /** Called for every radial dimension entity. */
  virtual void AddDimRadial(const EoDxfRadialDimension* data) = 0;

  /** Called for every ellipse */
  virtual void AddEllipse(const EoDxfEllipse& ellipse) = 0;

  /** Called for every hatch entity. */
  virtual void AddHatch(const EoDxfHatch& data) = 0;

  /** Called for every image entity. */
  virtual void AddImage(const EoDxfImage* image) = 0;

  /** Called for every insert. */
  virtual void AddInsert(const EoDxfInsert& blockReference) = 0;

  /** Called for every spline knot value */
  virtual void AddKnot(const EoDxfGraphic& data) = 0;

  /** Called for every leader start. */
  virtual void AddLeader(const EoDxfLeader* leader) = 0;

  /** Called for every line */
  virtual void AddLine(const EoDxfLine& line) = 0;

  /** Called for every lwpolyline */
  virtual void AddLWPolyline(const EoDxfLwPolyline& polyline) = 0;

  /** Called for every multileader entity. */
  virtual void AddMLeader(const EoDxfMLeader* mLeader) = 0;

  /** Called for every Multi Text entity. */
  virtual void AddMText(const EoDxfMText& mText) = 0;

  /** Called for every point */
  virtual void AddPoint(const EoDxfPoint& point) = 0;

  /** Called for every polyline start */
  virtual void AddPolyline(const EoDxfPolyline& polyline) = 0;

  /** Called for every ray */
  virtual void AddRay(const EoDxfRay& ray) = 0;

  /** Called for every solid start */
  virtual void AddSolid(const EoDxfSolid& data) = 0;

  /** Called for every spline start */
  virtual void AddSpline(const EoDxfSpline& spline) = 0;

  /** Called for every Text entity. */
  virtual void AddText(const EoDxfText& text) = 0;

  /** Called for every trace start */
  virtual void AddTrace(const EoDxfTrace& trace) = 0;

  /** Called for every viewport entity. */
  virtual void AddViewport(const EoDxfViewport& viewport) = 0;

  /** Called for every xline */
  virtual void AddXline(const EoDxfXline& xline) = 0;

  /**
   * Called for every image definition.
   */
  virtual void LinkImage(const EoDxfImageDefinition* data) = 0;

  /**
   * Called for unsupported object records that should be preserved for rewrite fidelity.
   */
  virtual void AddUnsupportedObject(const EoDxfUnsupportedObject& objectData) = 0;

  /**
   * Called for every comment in the DXF file (code 999).
   */
  virtual void AddComment(std::wstring_view comment) = 0;

  /// @brief Returns the application's next available entity handle.
  /// The DXF writer uses this to initialize its internal counter above
  /// all existing entity handles, preventing collisions between preserved
  /// imported handles and newly allocated table/object handles.
  /// @return Next handle value (0 = no application handle manager).
  [[nodiscard]] virtual std::uint64_t GetHandleSeed() const { return 0; }

  /// @brief Returns whether the interface has imported OBJECTS section data.
  /// When true, the writer skips hardcoded root/ACAD_GROUP dictionaries and
  /// delegates entirely to WriteUnsupportedObjects() to avoid duplicates.
  [[nodiscard]] virtual bool HasUnsupportedObjects() const { return false; }

  virtual void WriteHeader(EoDxfHeader& data) = 0;
  virtual void WriteClasses() = 0;
  virtual void WriteBlocks() = 0;
  virtual void WriteBlockRecords() = 0;
  virtual void WriteEntities() = 0;
  virtual void WriteObjects() = 0;
  virtual void WriteUnsupportedObjects() = 0;
  virtual void WriteLTypes() = 0;
  virtual void WriteLayers() = 0;
  virtual void WriteTextstyles() = 0;
  virtual void WriteVports() = 0;
  virtual void WriteDimstyles() = 0;
  virtual void WriteAppId() = 0;
};