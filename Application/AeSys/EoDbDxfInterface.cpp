#include "Stdafx.h"

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbConic.h"
#include "EoDbDxfInterface.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbHeaderSection.h"
#include "EoDbLayer.h"
#include "EoDbLine.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPoint.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDxfEntities.h"
#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfHeader.h"
#include "EoGeLine.h"
#include "EoGeOcsTransform.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

void EoDbDxfInterface::SetHeaderSectionVariable(
    const EoDxfHeader* header, std::wstring_view keyToFind, EoDbHeaderSection& headerSection) {
  HeaderVariable value;
  auto it = header->m_variants.find(std::wstring{keyToFind});
  if (it != header->m_variants.end() && it->second != nullptr) {
    const std::wstring& key = it->first;
    auto& second = *(it->second);
    if (const auto* stringValue = second.GetIf<std::wstring>()) {
      value = *stringValue;
    } else if (const auto* int16Value = second.GetIf<std::int16_t>()) {
      value = static_cast<int>(*int16Value);
    } else if (const auto* integerValue = second.GetIf<std::int32_t>()) {
      value = *integerValue;
    } else if (const auto* booleanValue = second.GetIf<bool>()) {
      value = *booleanValue ? 1 : 0;
    } else if (const auto* int64Value = second.GetIf<std::int64_t>()) {
      if (*int64Value >= std::numeric_limits<int>::min() && *int64Value <= std::numeric_limits<int>::max()) {
        value = static_cast<int>(*int64Value);
      } else {
        value = L"";
      }
    } else if (const auto* doubleValue = second.GetIf<double>()) {
      value = *doubleValue;
    } else if (const auto* geometryValue = second.GetIf<EoDxfGeometryBase3d>()) {
      value = EoGePoint3d(*geometryValue);
    } else {
      value = L"";
    }
    headerSection.SetVariable(key, HeaderVariable(value));
  }
}

void EoDbDxfInterface::ConvertHeaderSection(const EoDxfHeader* header, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Converting Header: %i variables\n", header->m_variants.size());

  std::vector<std::wstring> keys{L"$ACADVER", L"$CLAYER", L"$PDMODE", L"$PDSIZE"};

  EoDbHeaderSection& headerSection = document->HeaderSection();
  for (const auto& key : keys) { SetHeaderSectionVariable(header, key, headerSection); }
}

void EoDbDxfInterface::ConvertClassesSection(const EoDxfClass& class_, [[maybe_unused]] AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Class - Name: %s (unsupported in AeSys)\n", class_.m_classDxfRecordName.c_str());
}

void EoDbDxfInterface::ConvertAppIdTable(const EoDxfAppId& appId, [[maybe_unused]] AeSysDoc* document) {
  const auto& appIdName = appId.m_tableName;
  ATLTRACE2(traceGeneral, 3, L"AppId - Name: %s (unsupported in AeSys)\n", appIdName.c_str());
}

void EoDbDxfInterface::ConvertDimStyle(const EoDxfDimensionStyle& dimensionStyle, [[maybe_unused]] AeSysDoc* document) {
  const auto& dimStyleName = dimensionStyle.m_tableName;
  ATLTRACE2(traceGeneral, 3, L"DimStyle - Name: <%s> (unsupported in AeSys)\n", dimStyleName.c_str());
}

void EoDbDxfInterface::ConvertLayerTable(const EoDxfLayer& layer, AeSysDoc* document) {
  const auto& layerName = layer.m_tableName;

  ATLTRACE2(traceGeneral, 3, L"%s   Loading layer definition\n", layerName.c_str());

  if (document->FindLayerTableLayer(layerName.c_str()) >= 0) { return; }

  constexpr EoDbLayer::State commonState =
      EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive;

  EoDbLayer* newLayer = new EoDbLayer(layerName.c_str(), commonState);

  // Color number (if negative the layer is off) group code 62
  newLayer->SetColorIndex(static_cast<std::int16_t>(abs(layer.m_colorNumber)));
  if (layer.m_colorNumber < 0) { newLayer->SetStateOff(); }

  // Linetype name group code 6
  const auto& lineTypeName = layer.m_linetypeName;
  EoDbLineType* lineType;
  if (document->LineTypeTable()->Lookup(lineTypeName.c_str(), lineType)) { newLayer->SetLineType(lineType); }

  /**
  Standard flags (bit-coded values) group code 70:
    1 = Layer is frozen; otherwise layer is thawed
    2 = Layer is frozen by default in new viewports
    4 = Layer is locked
    16 = If set, table entry is externally dependent on an xref
    32 = If both this bit and bit 16 are set, the externally dependent xref has been successfully resolved
    64 = If set, the table entry was referenced by at least one entity in the drawing the last time the drawing was
  edited. (This flag is for the benefit of AutoCAD commands. It can be ignored by most programs that read DXF files and
  need not be set by programs that write DXF files)
   */
  auto isFrozen = (layer.m_flagValues & 0x01) == 0x01;
  auto isLocked = (layer.m_flagValues & 0x04) == 0x04;

  if (isFrozen) { newLayer->SetStateOff(); }
  document->AddLayerTableLayer(newLayer);

  /**
  Lineweight enum value (not supported directly in AeSys)  group code 370
    This enumerated type provides the line weight (thickness between 0 and 211) values used to specify how lines will be
  displayed and plotted. The lineweights are in 100ths of a millimeter, except for the negative values. The negative
  values denote the default indicated by their constant's name.
  */
  ATLTRACE2(traceGeneral, 2, L"Line weight: %i\n", layer.m_lineweightEnumValue);
  ATLTRACE2(traceGeneral, 2, L"Layer is locked: %i\n", isLocked);
  ATLTRACE2(traceGeneral, 2, L"Layer is plottable: %i\n", layer.m_plottingFlag);

  // Hard-pointer to ID/handle of PlotStyleName object (not supported in AeSys) group code 390
  ATLTRACE2(traceGeneral, 3, L"Plot style name objects not supported\n");

  // Hard-pointer to ID/handle of Material object (not supported in AeSys) group code 347
  ATLTRACE2(traceGeneral, 3, L"Material objects not supported\n");

  // It is possible to have an Extension Dictionary associated with layer. (not supported in AeSys) group code 102
  // Not used very often. The most common application is for per-viewport overrides of layer properties.
  // Each override type is stored under a distinct key in the dictionary, referencing an XRECORD that contains subdata
  // for affected viewports.
}

void EoDbDxfInterface::ConvertLinetypesTable(const EoDxfLinetype& linetype, AeSysDoc* document) {
  const auto& lineTypeName = linetype.m_tableName;  // Linetype name (group code 2)
  const auto& lineTypeDesc = linetype.desc;  // Descriptive text for linetype (group code 3)

  ATLTRACE2(traceGeneral, 3, L"Converting Linetype: %s\n", lineTypeName.c_str());

  EoDbLineTypeTable* lineTypeTable = document->LineTypeTable();
  EoDbLineType* convertedLinetype{};

  if (!lineTypeTable->Lookup(lineTypeName.c_str(), convertedLinetype)) {
    auto numberOfElements =
        static_cast<std::uint16_t>(linetype.m_numberOfLinetypeElements);  // Number of linetype elements (group code 73)
    // double patternLength = linetype.length;                        // group code 40

    std::vector<double> dashLengths(numberOfElements);

    for (std::uint16_t index = 0; index < numberOfElements; index++) {
      dashLengths[index] = linetype.path[index];  // group code 49
    }
    CString name(lineTypeName.c_str());
    CString desc(lineTypeDesc.c_str());
    auto lineTypeIndex = lineTypeTable->LegacyLineTypeIndex(name);

    convertedLinetype = new EoDbLineType(lineTypeIndex, name, desc, numberOfElements, dashLengths.data());
    lineTypeTable->SetAt(name, convertedLinetype);
  }
}

void EoDbDxfInterface::ConvertTextStyleTable(const EoDxfTextStyle& textStyle, [[maybe_unused]] AeSysDoc* document) {
  const auto& textStyleName = textStyle.m_tableName;
  ATLTRACE2(traceGeneral, 3, L"Text Style - Name: %s (unsupported in AeSys)\n", textStyleName.c_str());

  // auto height = textStyle.height;         // Fixed text height; 0 if not fixed (group code 40)
  // auto width = textStyle.width;           // Width factor (group code 41)
  // auto obliqueAngle = textStyle.oblique;  // Oblique angle (group code 50)
  // auto textGenerationFlags =
  textStyle
      .m_flagValues;  // Text generation flags (group code 71) 0x02 - text is backward, mirrored in X - 0x04 - text is
  // upside down, mirrored in Y
  // auto lastHeight = textStyle.lastHeight;  // Last height used (group code 42)

  // auto& font = textStyle.font;        // Primary font file name (group code 3)
  // auto& bigFont = textStyle.bigFont;  // Bigfont file name; blank if none (group code 4)
  // auto fontFamily = textStyle.fontFamily;  // A long value which contains a truetype font's pitch and family,
  // charset, and italic and bold flags (group code 1071)
}

void EoDbDxfInterface::ConvertVPortTable(const EoDxfVPort& viewport, [[maybe_unused]] AeSysDoc* document) {
  const auto& viewportName = viewport.m_tableName;
  ATLTRACE2(traceGeneral, 3, L"Viewport - Name: %s (unsupported in AeSys)\n", viewportName.c_str());

  auto lowerLeft = EoGePoint3d(viewport.m_lowerLeftCorner.x, viewport.m_lowerLeftCorner.y,
      0.0);  // code 10 & 20 (2D point)
  auto upperRight = EoGePoint3d(viewport.m_upperRightCorner.x, viewport.m_upperRightCorner.y,
      0.0);  // code 11 & 21 (2D point)
  auto center = EoGePoint3d(viewport.m_viewCenter.x, viewport.m_viewCenter.y,
      0.0);  // code 12 & 22 (2D point in DCS)
  auto snapBase = EoGePoint3d(viewport.m_snapBasePoint.x, viewport.m_snapBasePoint.y,
      0.0);  // code 13 & 23 (2D point in DCS)
  auto snapSpacing = EoGePoint3d(viewport.m_snapSpacing.x, viewport.m_snapSpacing.y,
      0.0);  // code 14 & 24 (2D point in DCS)
  auto gridSpacing = EoGePoint3d(viewport.m_gridSpacing.x, viewport.m_gridSpacing.y,
      0.0);  // code 15 & 25 (2D point in DCS)
  auto viewDirection = EoGeVector3d(viewport.m_viewDirection.x, viewport.m_viewDirection.y,
      viewport.m_viewDirection.z);  // code 16, 26 & 36 (3D point in WCS)
  auto viewTarget = EoGeVector3d(viewport.m_viewTargetPoint.x, viewport.m_viewTargetPoint.y,
      viewport.m_viewTargetPoint.z);  // code 17, 27 & 37 (3D point in WCS)

  // auto viewHeight = viewport.m_viewHeight;                   // code 40
  // auto viewAspectRatio = viewport.m_viewAspectRatio;         // code 41
  // auto lensLength = viewport.m_lensLength;                   // code 42
  // auto frontClipPlane = viewport.m_frontClipPlane;           // code 43
  // auto backClipPlane = viewport.m_backClipPlane;             // code 44
  // auto snapRotationAngle = viewport.m_snapRotationAngle;    // code 50
  // auto viewTwistAngle = viewport.m_viewTwistAngle;          // code 51

  // auto viewMode = viewport.m_viewMode;  // code 71
  // auto ucsIcon = viewport.m_ucsIcon;    // code 74
}

/** @brief This method is invoked when a new block definition is encountered in the file.
 *
 *  The EoDxfBlock parameter provides header information such as the block name, flags, base point, and handle. In your
 * implementation, use this to initialize a new block object in your application's data model. All subsequent entity
 * callbacks (e.g., for lines, arcs) will apply to this block until endBlock is called.
 * @note
 * 0x01 = This is an anonymous block generated by hatching, associative dimensioning, other internal operations, or an
 * application 0x02 = This block has non-constant attribute definitions (this bit is not set if the block has any
 * attribute definitions that are constant, or has no attribute definitions at all) 0x04 = This block is an external
 * reference (xref) 0x08 = This block is an xref overlay 0x10 = This block is externally dependent 0x20 = This is a
 * resolved external reference, or dependent of an external reference (ignored on input) 0x40 = This definition is a
 * referenced external reference (ignored on input)
 *
 * @note Three empty definitions always appear in the BLOCKS section. They are titled *Model_Space, *Paper_Space and
 * *Paper_Space0. These definitions manifest the representations of model space and paper space as block definitions
 * internally. The internal name of the first paper space layout is *Paper_Space, the second is *Paper_Space0, the third
 * is *Paper_Space1, and so on. The interleaving between model space and paper space no longer occurs. Instead, all
 * paper space entities are output, followed by model space entities. The flag distinguishing them is the group code 67.
 */
EoDbBlock* EoDbDxfInterface::ConvertBlock(const EoDxfBlock& block, AeSysDoc* document) {
  m_blockName = block.m_blockName;  // Block Name (group code 2)

  // auto handle = block.handle;              // group code 5
  // auto parentHandle = block.parentHandle;  // Soft-pointer ID/handle to owner object (group code 330)

  // Group codes 3, 1 and 4 are for XREF definition. Modern XREF indicated by group 70 with 0x04 bit set and the
  // presence of group code 1

  // @todo Check if block already exists and clean it up first

  //  Block-type bit-coded (see note) which may be combined (group code 70)
  auto* newBlock = new EoDbBlock(block.m_blockTypeFlags,
      EoGePoint3d(block.m_basePoint.x, block.m_basePoint.y, block.m_basePoint.z),  // group codes 10, 20 and 30
      m_blockName.c_str());

  document->InsertBlock(m_blockName.c_str(), newBlock);
  return newBlock;
}

/** @brief This method is primarily used in DWG files when the parser switches to entities belonging to a different
 * block than the current one. The handle parameter corresponds to the block handle previously provided via addBlock
 * (accessible as EoDxfBlock::handleBlock). In your implementation, switch the current block context to the one matching
 * this handle. For DXF files, this callback may not be triggered, or it may be used sparingly if blocks are referenced
 * out of sequence. */
void EoDbDxfInterface::ConvertBlockSet([[maybe_unused]] const int handle, [[maybe_unused]] AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Block set\n");
}

/** @brief This method signals the end of the current block definition. In your implementation, finalize the block
 * (e.g., add it to a document's block table or collection) and reset the context to the default (model space or paper
 * space).*/
void EoDbDxfInterface::ConvertBlockEnd([[maybe_unused]] AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Block end\n");
}

// Entities

namespace {
/** @brief Represents a 2D vertex in a lightweight polyline.
 */
class EoGeVertex2D {
 public:
  EoGeVertex2D() : x(0.0), y(0.0), startWidth(0.0), endWidth(0.0), bulge(0.0) {}
  EoGeVertex2D(double xInitial, double yInitial, double bulgeInitial)
      : x(xInitial), y(yInitial), startWidth(0.0), endWidth(0.0), bulge(bulgeInitial) {}

 public:
  double x;  // group code 10
  double y;  // group code 20
  double startWidth;  // group code 40
  double endWidth;  // group code 41
  double bulge;  // group code 42
};
}  // namespace

/** @brief Adds the given primitive to the appropriate layer in the document.
 *
 * @param primitive Pointer to the EoDbPrimitive to be added.
 * @param document Pointer to the AeSysDoc where the primitive will be added.
 */
void EoDbDxfInterface::AddToDocument(EoDbPrimitive* primitive, AeSysDoc* document) {
  auto layerName = primitive->LayerName().c_str();
  auto* layer = document->GetLayerTableLayer(layerName);
  if (layer == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Layer '%s' not found.\n", layerName);
    delete primitive;
    return;
  }

  ATLTRACE2(traceGeneral, 3, L"AddToDocument: primitive=%p, inBlock=%d, currentBlock=%p, layer='%s'\n", primitive,
      m_inBlockDefinition ? 1 : 0, m_currentOpenBlockDefinition, layerName);

  if (m_currentOpenBlockDefinition == nullptr) {
    auto* group = new EoDbGroup();

    ATLTRACE2(traceGeneral, 3, L"  -> Creating MODEL SPACE group %p for primitive %p\n", group, primitive);

    group->AddTail(primitive);
    layer->AddTail(group);
    return;
  }
  ATLTRACE2(traceGeneral, 3, L"  -> Adding to BLOCK at %p\n", m_currentOpenBlockDefinition);

  m_currentOpenBlockDefinition->AddTail(primitive);
}

void EoDbDxfInterface::ConvertArcEntity(const EoDxfArc& arc, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Arc entity conversion\n");

  if (arc.m_radius < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Arc entity with non-positive radius (%f) skipped.\n", arc.m_radius);
    return;
  }
  EoGePoint3d center(arc.m_centerPoint.x, arc.m_centerPoint.y, arc.m_centerPoint.z);

  EoGeVector3d extrusion(arc.m_extrusionDirection.x, arc.m_extrusionDirection.y, arc.m_extrusionDirection.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Unitize();
  }
  double startAngle = arc.m_startAngle;
  double endAngle = arc.m_endAngle;

  // For negative Z extrusion, EoDxf angles need mirroring to match AutoCAD behavior
  const bool isNegativeExtrusion = extrusion.z < -Eo::geometricTolerance;
  if (isNegativeExtrusion) {
    startAngle = Eo::TwoPi - arc.m_startAngle;
    endAngle = Eo::TwoPi - arc.m_endAngle;
    // Swap start and end to maintain CCW sweep direction
    std::swap(startAngle, endAngle);
  }
  startAngle = EoDbConic::NormalizeTo2Pi(startAngle);
  endAngle = EoDbConic::NormalizeTo2Pi(endAngle);

  auto* radialArc = EoDbConic::CreateRadialArc(center, extrusion, arc.m_radius, startAngle, endAngle);
  if (radialArc == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Failed to create radial arc.\n");
    return;
  }
  radialArc->SetBaseProperties(&arc, document);
  AddToDocument(radialArc, document);
}

void EoDbDxfInterface::ConvertCircleEntity(const EoDxfCircle& circle, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Circle entity conversion\n");

  EoGePoint3d center(circle.m_centerPoint.x, circle.m_centerPoint.y, circle.m_centerPoint.z);
  EoGeVector3d extrusion(circle.m_extrusionDirection.x, circle.m_extrusionDirection.y, circle.m_extrusionDirection.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Unitize();
  }
  auto* conic = EoDbConic::CreateCircle(center, extrusion, circle.m_radius);
  conic->SetBaseProperties(&circle, document);
  AddToDocument(conic, document);
}

void EoDbDxfInterface::ConvertEllipseEntity(const EoDxfEllipse& ellipse, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Ellipse entity conversion\n");

  if (ellipse.m_ratio <= 0.0 || ellipse.m_ratio > 1.0) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Ellipse entity with invalid ratio (%f) skipped.\n", ellipse.m_ratio);
    return;
  }
  EoGeVector3d majorAxis(ellipse.m_endPoint.x, ellipse.m_endPoint.y, ellipse.m_endPoint.z);
  if (majorAxis.IsNearNull()) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Zero-length major axis\n");
    return;
  }
  EoGeVector3d extrusion(
      ellipse.m_extrusionDirection.x, ellipse.m_extrusionDirection.y, ellipse.m_extrusionDirection.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Unitize();
  }
  auto center = EoGePoint3d(ellipse.m_startPoint.x, ellipse.m_startPoint.y, ellipse.m_startPoint.z);
  auto* conic =
      EoDbConic::CreateConic(center, majorAxis, extrusion, ellipse.m_ratio, ellipse.m_startParam, ellipse.m_endParam);
  conic->SetBaseProperties(&ellipse, document);
  AddToDocument(conic, document);
}

void EoDbDxfInterface::ConvertHatchEntity(const EoDxfHatch& hatch, [[maybe_unused]] AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Hatch entity conversion - Pattern: %s (under construction)\n",
      hatch.m_hatchPatternName.c_str());
}

void EoDbDxfInterface::ConvertInsertEntity(const EoDxfInsert& blockReference, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Insert entity conversion\n");
  auto insertPrimitive = new EoDbBlockReference();
  insertPrimitive->SetBaseProperties(&blockReference, document);
  insertPrimitive->SetName(CString(blockReference.m_blockName.c_str()));
  insertPrimitive->SetInsertionPoint(blockReference.m_firstPoint);
  insertPrimitive->SetNormal(EoGeVector3d(blockReference.m_extrusionDirection.x, blockReference.m_extrusionDirection.y,
      blockReference.m_extrusionDirection.z));
  insertPrimitive->SetScaleFactors(
      EoGeVector3d(blockReference.m_xScaleFactor, blockReference.m_yScaleFactor, blockReference.m_zScaleFactor));
  insertPrimitive->SetRotation(blockReference.m_rotationAngle);

  AddToDocument(insertPrimitive, document);
}

void EoDbDxfInterface::ConvertLineEntity(const EoDxfLine& line, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Line entity conversion\n");

  auto linePrimitive = new EoDbLine();
  linePrimitive->SetBaseProperties(&line, document);

  linePrimitive->SetLine(EoGeLine(EoGePoint3d{line.m_startPoint}, EoGePoint3d{line.m_endPoint}));
  AddToDocument(linePrimitive, document);
}

void EoDbDxfInterface::ConvertLWPolylineEntity(const EoDxfLwPolyline& polyline, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"LWPolyline entity conversion\n");

  auto* polylinePrimitive = new EoDbPolyline();
  polylinePrimitive->SetBaseProperties(&polyline, document);

  const auto numVerts = static_cast<std::uint16_t>(polyline.m_vertices.size());
  polylinePrimitive->SetNumberOfVertices(numVerts);

  for (std::uint16_t index = 0; index < numVerts; ++index) {
    polylinePrimitive->SetVertex2D(index, polyline.m_vertices[index]);  // value vector, no raw pointer
  }

  if (polyline.m_polylineFlag & 0x01) { polylinePrimitive->SetFlag(EoDbPolyline::sm_Closed); }

  AddToDocument(polylinePrimitive, document);
}

void EoDbDxfInterface::ConvertMTextEntity(const EoDxfMText& mtext, [[maybe_unused]] AeSysDoc* document) {
  const auto& textValue = mtext.m_textString;
  ATLTRACE2(traceGeneral, 3, L"MText entity conversion - Value: %s (under construction)\n", textValue.c_str());
}

void EoDbDxfInterface::ConvertPointEntity(const EoDxfPoint& point, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Point entity conversion\n");

  auto pointPrimitive = new EoDbPoint();
  pointPrimitive->SetBaseProperties(&point, document);
  pointPrimitive->SetPoint(point.m_firstPoint.x, point.m_firstPoint.y, point.m_firstPoint.z);
  AddToDocument(pointPrimitive, document);
}

void EoDbDxfInterface::ConvertTextEntity(const EoDxfText& text, [[maybe_unused]] AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Text entity conversion - (under construction)\n");

  [[maybe_unused]] auto thickness = text.m_thickness;  // Group code 50 (is not supported in AeSys)
  auto firstAlignmentPointInOcs =
      EoGePoint3d{text.m_firstAlignmentPoint.x, text.m_firstAlignmentPoint.y, text.m_firstAlignmentPoint.z};
  auto textHeight = text.m_textHeight;  // Group code 40

  /** This is the text string itself, which may contain embedded formatting codes:
   *    \P for new line, \L and \l for overline and underline start, \S for stacked fraction, etc.).
   * The handling of these formatting codes is complex and may require a dedicated parser to interpret them correctly.
   * For now, we will treat the entire string as plain text without special formatting.
   */
  std::wstring string{text.m_string.c_str()};  // Group code 1

  auto textRotation = Eo::DegreeToRadian(text.m_textRotation);  // Group code 50
  auto xScaleFactorWidth = text.m_scaleFactorWidth;  // Group code 41
  [[maybe_unused]] auto obliqueAngle = Eo::DegreeToRadian(text.m_obliqueAngle);

  std::wstring textStyleName = text.m_textStyleName;  // Group code 7

  [[maybe_unused]] auto textGenerationFlags = text.m_textGenerationFlags;  // Group code 71 (is not supported in AeSys)
  auto horizontalAlignment = text.m_horizontalAlignment;  // Group code 72

  auto secondAlignmentPointInOcs =
      EoGePoint3d{text.m_secondAlignmentPoint.x, text.m_secondAlignmentPoint.y, text.m_secondAlignmentPoint.z};
  EoGeVector3d extrusionDirection{
      text.m_extrusionDirection.x, text.m_extrusionDirection.y, text.m_extrusionDirection.z};
  if (!text.m_haveExtrusion || extrusionDirection.IsNearNull()) {
    extrusionDirection = EoGeVector3d::positiveUnitZ;
  } else {
    extrusionDirection.Unitize();
  }

  auto verticalAlignment = text.m_verticalAlignment;  // Group code 73

  auto baselineDirection = secondAlignmentPointInOcs - firstAlignmentPointInOcs;
  [[maybe_unused]] auto haveBaselineDirection = !baselineDirection.IsNearNull();

  EoGePoint3d firstAlignmentPointInWcs;
  EoGePoint3d secondAlignmentPointInWcs;

  // Always transform points to WCS (simplifies branches)
  EoGeOcsTransform transformOcs{extrusionDirection};
  firstAlignmentPointInWcs = transformOcs * firstAlignmentPointInOcs;
  secondAlignmentPointInWcs = transformOcs * secondAlignmentPointInOcs;

  // Compute baseline direction – respect DXF rules for Aligned/Fit
  baselineDirection = EoGeVector3d::positiveUnitX;
  const bool isAlignedOrFit = (horizontalAlignment == EoDxfText::HorizontalAlignment::AlignedIfBaseLine ||
                               horizontalAlignment == EoDxfText::HorizontalAlignment::FitIfBaseLine);

  if (haveBaselineDirection && isAlignedOrFit) {
    // Spec: ignore textRotation; use points only
    baselineDirection = (secondAlignmentPointInWcs - firstAlignmentPointInWcs);
    baselineDirection.Unitize();
  } else {
    // Normal case: rotation defines direction (in OCS plane, but we already transformed)
    // Rotate default X around the (now unit) extrusion
    baselineDirection.RotateAboutArbitraryAxis(extrusionDirection, textRotation);
    // If second point exists but not Aligned/Fit, it is just the alignment reference point
    // (we still use firstWcs as origin per your alignment-flag approach)
  }

  auto xAxisDirection = baselineDirection;
  auto yAxisDirection = CrossProduct(extrusionDirection, xAxisDirection);

  yAxisDirection *= textHeight;
  xAxisDirection *= xScaleFactorWidth * textHeight * Eo::defaultCharacterCellAspectRatio;
  EoGeReferenceSystem referenceSystem(firstAlignmentPointInWcs, xAxisDirection, yAxisDirection);

  EoDbFontDefinition fontDefinition{};
  fontDefinition.SetFontName(textStyleName);

  /** Only Left(0), Center(1) and Right(2) horizontal alignment options are supported in AeSys so options
   * Aligned(3) → Left, Middle(4) → Center, and Fit(5) → Right; (should only pair with Baseline(0) vertical alignment
   * and when the second alignment point is defined).
   * @todo: AeSys does not support the stretching behavior of these options.
   * Consider adding support for the stretching behavior in AeSys and mapping these options directly.
   */
  switch (horizontalAlignment) {
    case EoDxfText::HorizontalAlignment::Center:
      [[fallthrough]];
    case EoDxfText::HorizontalAlignment::MiddleIfBaseLine:
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Center);
      break;
    case EoDxfText::HorizontalAlignment::Right:
      [[fallthrough]];
    case EoDxfText::HorizontalAlignment::FitIfBaseLine:
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Right);
      break;
    case EoDxfText::HorizontalAlignment::Left:
      [[fallthrough]];
    case EoDxfText::HorizontalAlignment::AlignedIfBaseLine:
      [[fallthrough]];
    default:
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Left);
      break;
  }

  switch (verticalAlignment) {
    case EoDxfText::VerticalAlignment::Middle:
      fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
      break;
    case EoDxfText::VerticalAlignment::Top:
      fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Top);
      break;
    case EoDxfText::VerticalAlignment::BaseLine:
      [[fallthrough]];
    case EoDxfText::VerticalAlignment::Bottom:
      [[fallthrough]];
    default:
      fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Bottom);
      break;
  }

  auto* textPrimitive = new EoDbText(fontDefinition, referenceSystem, string);
  textPrimitive->SetBaseProperties(&text, document);

  AddToDocument(textPrimitive, document);
}