#include "Stdafx.h"

#include <cstdint>
#include <cstdlib>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbConic.h"
#include "EoDbDrwInterface.h"
#include "EoDbGroup.h"
#include "EoDbHeaderSection.h"
#include "EoDbLayer.h"
#include "EoDbLine.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPoint.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "drw_base.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"

void EoDbDrwInterface::SetHeaderSectionVariable(
    const DRW_Header* header, const std::string& keyToFind, EoDbHeaderSection& headerSection) {
  HeaderVariable value;
  auto it = header->m_variants.find(keyToFind);
  if (it != header->m_variants.end() && it->second != nullptr) {
    std::wstring key = Eo::MultiByteToWString(it->first.c_str());
    auto& second = *(it->second);
    switch (second.type()) {
      case DRW_Variant::Type::String:
        value = Eo::MultiByteToWString(second.content.s->c_str());
        break;
      case DRW_Variant::Type::Integer:
        value = second.content.i;
        break;
      case DRW_Variant::Type::Double:
        value = second.content.d;
        break;
      case DRW_Variant::Type::Coord:
        value = EoGePoint3d(*second.content.v);
        break;
      case DRW_Variant::Type::Invalid:
      default:
        value = L"";
        break;
    }
    headerSection.SetVariable(key, HeaderVariable(value));
  }
}

void EoDbDrwInterface::ConvertHeaderSection(const DRW_Header* header, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Converting Header: %i variables\n", header->m_variants.size());

  std::vector<std::string> keys{"$ACADVER", "$CLAYER", "$PDMODE", "$PDSIZE"};

  EoDbHeaderSection& headerSection = document->HeaderSection();
  for (const auto& key : keys) { SetHeaderSectionVariable(header, key, headerSection); }
}

void EoDbDrwInterface::ConvertClassesSection(const DRW_Class& class_, [[maybe_unused]] AeSysDoc* document) {
  std::wstring recordName = Eo::MultiByteToWString(class_.recName.c_str());
  ATLTRACE2(traceGeneral, 2, L"Class - Name: %s (unsupported in AeSys)\n", recordName.c_str());
}

void EoDbDrwInterface::ConvertAppIdTable(const DRW_AppId& appId, [[maybe_unused]] AeSysDoc* document) {
  std::wstring appIdName = Eo::MultiByteToWString(appId.name.c_str());
  ATLTRACE2(traceGeneral, 3, L"AppId - Name: %s (unsupported in AeSys)\n", appIdName.c_str());
}

void EoDbDrwInterface::ConvertDimStyle(const DRW_Dimstyle& dimStyle, [[maybe_unused]] AeSysDoc* document) {
  std::wstring dimStyleName = Eo::MultiByteToWString(dimStyle.name.c_str());
  ATLTRACE2(traceGeneral, 3, L"DimStyle - Name: <%s> (unsupported in AeSys)\n", dimStyleName.c_str());
}

void EoDbDrwInterface::ConvertLayerTable(const DRW_Layer& layer, AeSysDoc* document) {
  std::wstring layerName = Eo::MultiByteToWString(layer.name.c_str());

  ATLTRACE2(traceGeneral, 3, L"%s   Loading layer definition\n", layerName.c_str());

  if (document->FindLayerTableLayer(layerName.c_str()) >= 0) { return; }

  constexpr EoDbLayer::State commonState =
      EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive;

  EoDbLayer* newLayer = new EoDbLayer(layerName.c_str(), commonState);

  // Color number (if negative the layer is off) group code 62
  newLayer->SetColorIndex(static_cast<std::int16_t>(abs(layer.color)));
  if (layer.color < 0) { newLayer->SetStateOff(); }

  // Linetype name group code 6
  std::wstring lineTypeName = Eo::MultiByteToWString(layer.lineType.c_str());
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
  auto isFrozen = (layer.flags & 0x01) == 0x01;
  auto isLocked = (layer.flags & 0x04) == 0x04;

  if (isFrozen) { newLayer->SetStateOff(); }
  document->AddLayerTableLayer(newLayer);

  /**
  Lineweight enum value (not supported directly in AeSys)  group code 370
    This enumerated type provides the line weight (thickness between 0 and 211) values used to specify how lines will be
  displayed and plotted. The lineweights are in 100ths of a millimeter, except for the negative values. The negative
  values denote the default indicated by their constant's name.
  */
  ATLTRACE2(traceGeneral, 2, L"Line weight: %i\n", layer.lWeight);
  ATLTRACE2(traceGeneral, 2, L"Layer is locked: %i\n", isLocked);
  ATLTRACE2(traceGeneral, 2, L"Layer is plottable: %i\n", layer.plotF);

  // Hard-pointer to ID/handle of PlotStyleName object (not supported in AeSys) group code 390
  ATLTRACE2(traceGeneral, 3, L"Plot style name objects not supported\n");

  // Hard-pointer to ID/handle of Material object (not supported in AeSys) group code 347
  ATLTRACE2(traceGeneral, 3, L"Material objects not supported\n");

  // It is possible to have an Extension Dictionary associated with layer. (not supported in AeSys) group code 102
  // Not used very often. The most common application is for per-viewport overrides of layer properties.
  // Each override type is stored under a distinct key in the dictionary, referencing an XRECORD that contains subdata
  // for affected viewports.
}

void EoDbDrwInterface::ConvertLinetypesTable(const DRW_LType& data, AeSysDoc* document) {
  std::wstring lineTypeName = Eo::MultiByteToWString(data.name.c_str());  // Linetype name (group code 2)
  std::wstring lineTypeDesc =
      Eo::MultiByteToWString(data.desc.c_str());  // Descriptive text for linetype (group code 3)

  ATLTRACE2(traceGeneral, 3, L"Converting Linetype: %s\n", lineTypeName.c_str());

  EoDbLineTypeTable* lineTypeTable = document->LineTypeTable();
  EoDbLineType* LineType{};

  if (!lineTypeTable->Lookup(lineTypeName.c_str(), LineType)) {
    auto numberOfElements = static_cast<std::uint16_t>(data.size);  // Number of linetype elements (group code 73)
    // double patternLength = data.length;                        // group code 40

    std::vector<double> dashLengths(numberOfElements);

    for (std::uint16_t index = 0; index < numberOfElements; index++) {
      dashLengths[index] = data.path[index];  // group code 49
    }
    CString name(lineTypeName.c_str());
    CString desc(lineTypeDesc.c_str());
    auto lineTypeIndex = lineTypeTable->LegacyLineTypeIndex(name);

    LineType = new EoDbLineType(lineTypeIndex, name, desc, numberOfElements, dashLengths.data());
    lineTypeTable->SetAt(name, LineType);
  }
}

void EoDbDrwInterface::ConvertTextStyleTable(const DRW_Textstyle& textStyle, [[maybe_unused]] AeSysDoc* document) {
  std::wstring textStyleName = Eo::MultiByteToWString(textStyle.name.c_str());
  ATLTRACE2(traceGeneral, 3, L"Text Style - Name: %s (unsupported in AeSys)\n", textStyleName.c_str());

  // auto height = textStyle.height;         // Fixed text height; 0 if not fixed (group code 40)
  // auto width = textStyle.width;           // Width factor (group code 41)
  // auto obliqueAngle = textStyle.oblique;  // Oblique angle (group code 50)
  // auto textGenerationFlags =
  textStyle.flags;  // Text generation flags (group code 71) 0x02 - text is backward, mirrored in X - 0x04 - text is
                    // upside down, mirrored in Y
  // auto lastHeight = textStyle.lastHeight;  // Last height used (group code 42)

  // auto& font = textStyle.font;        // Primary font file name (group code 3)
  // auto& bigFont = textStyle.bigFont;  // Bigfont file name; blank if none (group code 4)
  // auto fontFamily = textStyle.fontFamily;  // A long value which contains a truetype font's pitch and family,
  // charset, and italic and bold flags (group code 1071)
}

void EoDbDrwInterface::ConvertViewportTable(const DRW_Vport& viewport, [[maybe_unused]] AeSysDoc* document) {
  std::wstring viewportName = Eo::MultiByteToWString(viewport.name.c_str());
  ATLTRACE2(traceGeneral, 3, L"Viewport - Name: %s (unsupported in AeSys)\n", viewportName.c_str());

  auto lowerLeft = EoGePoint3d(viewport.lowerLeft.x, viewport.lowerLeft.y,
      viewport.lowerLeft.z);  // group codes 10 and 20 (2D point)
  auto upperRight = EoGePoint3d(viewport.upperRight.x, viewport.upperRight.y,
      viewport.upperRight.z);  // group codes 11 and 21 (2D point)
  auto center =
      EoGePoint3d(viewport.center.x, viewport.center.y, viewport.center.z);  // group codes 12 and 22 (2D point in DCS)
  auto snapBase = EoGePoint3d(viewport.snapBase.x, viewport.snapBase.y,
      viewport.snapBase.z);  // group codes 13 and 23 (2D point in DCS)
  auto snapSpacing = EoGePoint3d(viewport.snapSpacing.x, viewport.snapSpacing.y,
      viewport.snapSpacing.z);  // group codes 14 and 24 (2D point in DCS)
  auto gridSpacing = EoGePoint3d(viewport.gridSpacing.x, viewport.gridSpacing.y,
      viewport.gridSpacing.z);  // group codes 15 and 25 (2D point in DCS)
  auto viewDirection = EoGeVector3d(viewport.viewDir.x, viewport.viewDir.y,
      viewport.viewDir.z);  // group codes 16, 26 and 36 (3D point in WCS)
  auto viewTarget = EoGeVector3d(viewport.viewTarget.x, viewport.viewTarget.y,
      viewport.viewTarget.z);  // group codes 17, 27 and 37 (3D point in WCS)

  // auto height = viewport.height;  // group code 45
  // auto ratio = viewport.ratio;
  // auto lensHeight = viewport.lensHeight;        // group code 42
  // auto frontClip = viewport.frontClip;          // group code 43
  // auto backClip = viewport.backClip;            // group code 44
  // auto snapRotationAngle = viewport.snapAngle;  // group code 50
  // auto viewTwistAngle = viewport.twistAngle;    // group code 51

  // auto viewMode = viewport.viewMode;  // group code 71
  // auto ucsIcon = viewport.ucsIcon;    // group code 74
}

/** @brief This method is invoked when a new block definition is encountered in the file.
 *
 *  The DRW_Block parameter provides header information such as the block name, flags, base point, and handle. In your
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
EoDbBlock* EoDbDrwInterface::ConvertBlock(const DRW_Block& block, AeSysDoc* document) {
  blockName = Eo::MultiByteToWString(block.name.c_str());  // Block Name (group code 2)

  // auto handle = block.handle;              // group code 5
  // auto parentHandle = block.parentHandle;  // Soft-pointer ID/handle to owner object (group code 330)

  // Group codes 3, 1 and 4 are for XREF definition. Modern XREF indicated by group 70 with 0x04 bit set and the
  // presence of group code 1

  // @todo Check if block already exists and clean it up first

  auto* newBlock =
      new EoDbBlock(static_cast<std::uint16_t>(
                        block.flags),  //  Block-type bit-coded (see note) which may be combined (group code 70)
          EoGePoint3d(block.basePoint.x, block.basePoint.y, block.basePoint.z),  // group codes 10, 20 and 30
          blockName.c_str());

  document->InsertBlock(blockName.c_str(), newBlock);
  return newBlock;
}

/** @brief This method is primarily used in DWG files when the parser switches to entities belonging to a different
 * block than the current one. The handle parameter corresponds to the block handle previously provided via addBlock
 * (accessible as DRW_Block::handleBlock). In your implementation, switch the current block context to the one matching
 * this handle. For DXF files, this callback may not be triggered, or it may be used sparingly if blocks are referenced
 * out of sequence. */
void EoDbDrwInterface::ConvertBlockSet([[maybe_unused]] const int handle, [[maybe_unused]] AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Block set\n");
}

/** @brief This method signals the end of the current block definition. In your implementation, finalize the block
 * (e.g., add it to a document's block table or collection) and reset the context to the default (model space or paper
 * space).*/
void EoDbDrwInterface::ConvertBlockEnd([[maybe_unused]] AeSysDoc* document) {
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
  double x;           // group code 10
  double y;           // group code 20
  double startWidth;  // group code 40
  double endWidth;    // group code 41
  double bulge;       // group code 42
};
}  // namespace

/** @brief Adds the given primitive to the appropriate layer in the document.
 *
 * @param primitive Pointer to the EoDbPrimitive to be added.
 * @param document Pointer to the AeSysDoc where the primitive will be added.
 */
void EoDbDrwInterface::AddToDocument(EoDbPrimitive* primitive, AeSysDoc* document) {
  auto layerName = primitive->LayerName().c_str();
  auto* layer = document->GetLayerTableLayer(layerName);
  if (layer == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Layer '%s' not found.\n", layerName);
    delete primitive;
    return;
  }

  ATLTRACE2(traceGeneral, 3, L"AddToDocument: primitive=%p, inBlock=%d, currentBlock=%p, layer='%s'\n", primitive,
      inBlockDefinition ? 1 : 0, currentOpenBlockDefinition, layerName);

  if (currentOpenBlockDefinition == nullptr) {
    auto* group = new EoDbGroup();

    ATLTRACE2(traceGeneral, 3, L"  -> Creating MODEL SPACE group %p for primitive %p\n", group, primitive);

    group->AddTail(primitive);
    layer->AddTail(group);
    return;
  }
  ATLTRACE2(traceGeneral, 3, L"  -> Adding to BLOCK at %p\n", currentOpenBlockDefinition);

  currentOpenBlockDefinition->AddTail(primitive);
}

void EoDbDrwInterface::ConvertArcEntity(const DRW_Arc& arc, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Arc entity conversion\n");

  if (arc.radious < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Arc entity with non-positive radius (%f) skipped.\n", arc.radious);
    return;
  }
  EoGePoint3d center(arc.basePoint.x, arc.basePoint.y, arc.basePoint.z);

  EoGeVector3d extrusion(arc.extPoint.x, arc.extPoint.y, arc.extPoint.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Normalize();
  }
  double startAngle = arc.staangle;
  double endAngle = arc.endangle;

  // For negative Z extrusion, libdxfrw angles need mirroring to match AutoCAD behavior
  const bool isNegativeExtrusion = extrusion.z < -Eo::geometricTolerance;
  if (isNegativeExtrusion) {
    startAngle = Eo::TwoPi - arc.staangle;
    endAngle = Eo::TwoPi - arc.endangle;
    // Swap start and end to maintain CCW sweep direction
    std::swap(startAngle, endAngle);
  }
  startAngle = EoDbConic::NormalizeTo2Pi(startAngle);
  endAngle = EoDbConic::NormalizeTo2Pi(endAngle);

  auto* radialArc = EoDbConic::CreateRadialArc(center, extrusion, arc.radious, startAngle, endAngle);
  if (radialArc == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Failed to create radial arc.\n");
    return;
  }
  radialArc->SetBaseProperties(&arc, document);
  AddToDocument(radialArc, document);
}

void EoDbDrwInterface::ConvertCircleEntity(const DRW_Circle& circle, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Circle entity conversion\n");

  EoGePoint3d center(circle.basePoint.x, circle.basePoint.y, circle.basePoint.z);
  EoGeVector3d extrusion(circle.extPoint.x, circle.extPoint.y, circle.extPoint.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Normalize();
  }
  auto* conic = EoDbConic::CreateCircle(center, extrusion, circle.radious);
  conic->SetBaseProperties(&circle, document);
  AddToDocument(conic, document);
}

void EoDbDrwInterface::ConvertEllipseEntity(const DRW_Ellipse& ellipse, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Ellipse entity conversion\n");

  if (ellipse.ratio <= 0.0 || ellipse.ratio > 1.0) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Ellipse entity with invalid ratio (%f) skipped.\n", ellipse.ratio);
    return;
  }
  EoGeVector3d majorAxis(ellipse.secPoint.x, ellipse.secPoint.y, ellipse.secPoint.z);
  if (majorAxis.IsNearNull()) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Zero-length major axis\n");
    return;
  }
  EoGeVector3d extrusion(ellipse.extPoint.x, ellipse.extPoint.y, ellipse.extPoint.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Normalize();
  }
  auto center = EoGePoint3d(ellipse.basePoint.x, ellipse.basePoint.y, ellipse.basePoint.z);
  auto* conic = EoDbConic::CreateConic(center, majorAxis, extrusion, ellipse.ratio, ellipse.staparam, ellipse.endparam);
  conic->SetBaseProperties(&ellipse, document);
  AddToDocument(conic, document);
}

void EoDbDrwInterface::ConvertInsertEntity(const DRW_Insert& insert, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Insert entity conversion\n");
  auto insertPrimitive = new EoDbBlockReference();
  insertPrimitive->SetBaseProperties(&insert, document);
  auto name = Eo::MultiByteToWString(insert.name.c_str());
  insertPrimitive->SetName(CString(name.c_str()));
  insertPrimitive->SetInsertionPoint(insert.basePoint);
  insertPrimitive->SetNormal(EoGeVector3d(insert.extPoint.x, insert.extPoint.y, insert.extPoint.z));
  insertPrimitive->SetScaleFactors(EoGeVector3d(insert.xscale, insert.yscale, insert.zscale));
  insertPrimitive->SetRotation(insert.angle);

  AddToDocument(insertPrimitive, document);
}

void EoDbDrwInterface::ConvertLineEntity(const DRW_Line& line, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Line entity conversion\n");

  auto linePrimitive = new EoDbLine();
  linePrimitive->SetBaseProperties(&line, document);

  linePrimitive->SetLine(EoGeLine(EoGePoint3d{line.basePoint}, EoGePoint3d{line.secPoint}));
  AddToDocument(linePrimitive, document);
}

void EoDbDrwInterface::ConvertLWPolylineEntity(const DRW_LWPolyline& lwPolyline, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"LWPolyline entity conversion\n");
  auto polylinePrimitive = new EoDbPolyline();
  polylinePrimitive->SetBaseProperties(&lwPolyline, document);

  polylinePrimitive->SetNumberOfVertices(lwPolyline.vertlist.size());

  for (size_t index = 0; index < lwPolyline.vertlist.size(); index++) {
    polylinePrimitive->SetVertex2D(index, *lwPolyline.vertlist.at(index));
  }
  if (lwPolyline.flags & 0x01) { polylinePrimitive->SetFlag(EoDbPolyline::sm_Closed); }

  AddToDocument(polylinePrimitive, document);
}

void EoDbDrwInterface::ConvertPointEntity(const DRW_Point& point, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Point entity conversion\n");

  auto pointPrimitive = new EoDbPoint();
  pointPrimitive->SetBaseProperties(&point, document);
  pointPrimitive->SetPoint(point.basePoint.x, point.basePoint.y, point.basePoint.z);
  AddToDocument(pointPrimitive, document);
}
