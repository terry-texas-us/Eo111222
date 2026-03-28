#include "Stdafx.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbAttrib.h"
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
#include "EoDbPolygon.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoDbSpline.h"
#include "EoDbText.h"
#include "EoDbViewport.h"
#include "EoDxfEntities.h"
#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfHeader.h"
#include "EoGeLine.h"
#include "EoGeOcsTransform.h"
#include "EoGePoint3d.h"
#include "EoGePolyline.h"
#include "EoGeVector3d.h"

void EoDbDxfInterface::SetHeaderSectionVariable(
    const EoDxfHeader* header, std::wstring_view keyToFind, EoDbHeaderSection& headerSection) {
  auto it = header->m_variants.find(std::wstring{keyToFind});
  if (it == header->m_variants.end() || it->second == nullptr) { return; }

  const std::wstring& key = it->first;
  const auto& variant = *(it->second);
  const int groupCode = variant.Code();
  HeaderVariable value;

  if (const auto* stringValue = variant.GetIf<std::wstring>()) {
    value = *stringValue;
  } else if (const auto* int16Value = variant.GetIf<std::int16_t>()) {
    value = static_cast<int>(*int16Value);
  } else if (const auto* integerValue = variant.GetIf<std::int32_t>()) {
    value = *integerValue;
  } else if (const auto* booleanValue = variant.GetIf<bool>()) {
    value = *booleanValue;
  } else if (const auto* int64Value = variant.GetIf<std::int64_t>()) {
    if (*int64Value >= std::numeric_limits<int>::min() && *int64Value <= std::numeric_limits<int>::max()) {
      value = static_cast<int>(*int64Value);
    } else {
      value = L"";
    }
  } else if (const auto* handleValue = variant.GetIf<std::uint64_t>()) {
    value = *handleValue;
  } else if (const auto* doubleValue = variant.GetIf<double>()) {
    value = *doubleValue;
  } else if (const auto* geometryValue = variant.GetIf<EoDxfGeometryBase3d>()) {
    value = EoGePoint3d(*geometryValue);
  } else {
    value = L"";
  }
  headerSection.SetVariable(key, value, groupCode);
}

void EoDbDxfInterface::ConvertHeaderSection(const EoDxfHeader* header, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Converting Header: %i variables\n", static_cast<int>(header->m_variants.size()));

  EoDbHeaderSection& headerSection = document->HeaderSection();

  // Import all header variables as-is for passthrough round-trip.
  for (const auto& [key, variantPtr] : header->m_variants) {
    if (variantPtr == nullptr) { continue; }
    SetHeaderSectionVariable(header, key, headerSection);
  }

  // Apply $HANDSEED to the document handle manager so new handles do not collide.
  if (const auto* handSeed = headerSection.SetVariable(L"$HANDSEED")) {
    if (const auto* handleValue = std::get_if<std::uint64_t>(handSeed)) {
      document->HandleManager().SetNextHandle(*handleValue);
    }
  }
}

void EoDbDxfInterface::ConvertClassesSection(const EoDxfClass& class_, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Class - Name: %s\n", class_.m_classDxfRecordName.c_str());

  EoDbClassEntry entry;
  entry.m_classDxfRecordName = class_.m_classDxfRecordName;
  entry.m_cppClassName = class_.m_cppClassName;
  entry.m_applicationName = class_.m_applicationName;
  entry.m_proxyCapabilitiesFlag = class_.m_proxyCapabilitiesFlag;
  entry.m_instanceCount = class_.m_instanceCount;
  entry.m_wasAProxyFlag = class_.m_wasAProxyFlag;
  entry.m_isAnEntityFlag = class_.m_isAnEntityFlag;

  document->AddClassEntry(std::move(entry));
}

void EoDbDxfInterface::ConvertAppIdTable(const EoDxfAppId& appId, AeSysDoc* document) {
  const auto& appIdName = appId.m_tableName;
  ATLTRACE2(traceGeneral, 3, L"AppId - Name: %s\n", appIdName.c_str());

  EoDbAppIdEntry entry;
  entry.m_name = appIdName;
  entry.m_flagValues = appId.m_flagValues;
  entry.m_handle = appId.m_handle;
  entry.m_ownerHandle = appId.m_ownerHandle;

  document->AddAppIdEntry(std::move(entry));
}

void EoDbDxfInterface::ConvertDimStyle(const EoDxfDimensionStyle& dimensionStyle, AeSysDoc* document) {
  const auto& dimStyleName = dimensionStyle.m_tableName;
  ATLTRACE2(traceGeneral, 3, L"DimStyle - Name: <%s>\n", dimStyleName.c_str());

  EoDbDimStyle entry;
  entry.m_name = dimStyleName;
  entry.m_flagValues = dimensionStyle.m_flagValues;
  entry.m_handle = dimensionStyle.m_handle;
  entry.m_ownerHandle = dimensionStyle.m_ownerHandle;

  entry.dimpost = dimensionStyle.dimpost;
  entry.dimapost = dimensionStyle.dimapost;
  entry.dimblk = dimensionStyle.dimblk;
  entry.dimblk1 = dimensionStyle.dimblk1;
  entry.dimblk2 = dimensionStyle.dimblk2;

  entry.dimscale = dimensionStyle.dimscale;
  entry.dimasz = dimensionStyle.dimasz;
  entry.dimexo = dimensionStyle.dimexo;
  entry.dimdli = dimensionStyle.dimdli;
  entry.dimexe = dimensionStyle.dimexe;
  entry.dimrnd = dimensionStyle.dimrnd;
  entry.dimdle = dimensionStyle.dimdle;
  entry.dimtp = dimensionStyle.dimtp;
  entry.dimtm = dimensionStyle.dimtm;
  entry.dimfxl = dimensionStyle.dimfxl;
  entry.dimtxt = dimensionStyle.dimtxt;
  entry.dimcen = dimensionStyle.dimcen;
  entry.dimtsz = dimensionStyle.dimtsz;
  entry.dimaltf = dimensionStyle.dimaltf;
  entry.dimlfac = dimensionStyle.dimlfac;
  entry.dimtvp = dimensionStyle.dimtvp;
  entry.dimtfac = dimensionStyle.dimtfac;
  entry.dimgap = dimensionStyle.dimgap;
  entry.dimaltrnd = dimensionStyle.dimaltrnd;

  entry.dimtol = dimensionStyle.dimtol;
  entry.dimlim = dimensionStyle.dimlim;
  entry.dimtih = dimensionStyle.dimtih;
  entry.dimtoh = dimensionStyle.dimtoh;
  entry.dimse1 = dimensionStyle.dimse1;
  entry.dimse2 = dimensionStyle.dimse2;
  entry.dimtad = dimensionStyle.dimtad;
  entry.dimzin = dimensionStyle.dimzin;
  entry.dimazin = dimensionStyle.dimazin;
  entry.dimalt = dimensionStyle.dimalt;
  entry.dimaltd = dimensionStyle.dimaltd;
  entry.dimtofl = dimensionStyle.dimtofl;
  entry.dimsah = dimensionStyle.dimsah;
  entry.dimtix = dimensionStyle.dimtix;
  entry.dimsoxd = dimensionStyle.dimsoxd;
  entry.dimclrd = dimensionStyle.dimclrd;
  entry.dimclre = dimensionStyle.dimclre;
  entry.dimclrt = dimensionStyle.dimclrt;
  entry.dimadec = dimensionStyle.dimadec;
  entry.dimunit = dimensionStyle.dimunit;
  entry.dimdec = dimensionStyle.dimdec;
  entry.dimtdec = dimensionStyle.dimtdec;
  entry.dimaltu = dimensionStyle.dimaltu;
  entry.dimalttd = dimensionStyle.dimalttd;
  entry.dimaunit = dimensionStyle.dimaunit;
  entry.dimfrac = dimensionStyle.dimfrac;
  entry.dimlunit = dimensionStyle.dimlunit;
  entry.dimdsep = dimensionStyle.dimdsep;
  entry.dimtmove = dimensionStyle.dimtmove;
  entry.dimjust = dimensionStyle.dimjust;
  entry.dimsd1 = dimensionStyle.dimsd1;
  entry.dimsd2 = dimensionStyle.dimsd2;
  entry.dimtolj = dimensionStyle.dimtolj;
  entry.dimtzin = dimensionStyle.dimtzin;
  entry.dimaltz = dimensionStyle.dimaltz;
  entry.dimaltttz = dimensionStyle.dimaltttz;
  entry.dimfit = dimensionStyle.dimfit;
  entry.dimupt = dimensionStyle.dimupt;
  entry.dimatfit = dimensionStyle.dimatfit;

  entry.dimfxlon = dimensionStyle.dimfxlon;

  entry.dimtxsty = dimensionStyle.dimtxsty;
  entry.dimldrblk = dimensionStyle.dimldrblk;

  entry.dimlwd = dimensionStyle.dimlwd;
  entry.dimlwe = dimensionStyle.dimlwe;

  document->AddDimStyleEntry(std::move(entry));
}

void EoDbDxfInterface::ConvertLayerTable(const EoDxfLayer& layer, AeSysDoc* document) {
  const auto& layerName = layer.m_tableName;

  ATLTRACE2(traceGeneral, 3, L"%s   Loading layer definition\n", layerName.c_str());

  constexpr EoDbLayer::State commonState =
      EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive;

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

  // Resolve the line type once for both space layers
  const auto& lineTypeName = layer.m_linetypeName;
  EoDbLineType* lineType{};
  [[maybe_unused]] const bool lineTypeFound = document->LineTypeTable()->Lookup(lineTypeName.c_str(), lineType);

  // Helper: configure a newly created layer with DXF properties
  auto configureLayer = [&](EoDbLayer* newLayer) {
    // Color number (if negative the layer is off) group code 62
    newLayer->SetColorIndex(static_cast<std::int16_t>(abs(layer.m_colorNumber)));
    if (layer.m_colorNumber < 0) { newLayer->SetStateOff(); }
    if (lineType != nullptr) { newLayer->SetLineType(lineType); }
    // Frozen layers are displayed as off, but the frozen state is preserved separately for export
    if (isFrozen) {
      newLayer->SetStateOff();
      newLayer->SetFrozen(true);
    }
    newLayer->SetLocked(isLocked);
    newLayer->SetLineWeight(layer.m_lineweightEnumValue);
    newLayer->SetPlottingFlag(layer.m_plottingFlag);
    newLayer->SetColor24(layer.color24);
  };

  // Create / configure the layer in model space if not already present
  if (document->FindLayerInSpace(layerName.c_str(), EoDxf::Space::ModelSpace) == nullptr) {
    auto* modelLayer = new EoDbLayer(layerName.c_str(), commonState);
    configureLayer(modelLayer);
    modelLayer->SetHandle(layer.m_handle);
    modelLayer->SetOwnerHandle(layer.m_ownerHandle);
    document->AddLayerToSpace(modelLayer, EoDxf::Space::ModelSpace);
  }

  // Create / configure the layer in paper space if not already present
  if (document->FindLayerInSpace(layerName.c_str(), EoDxf::Space::PaperSpace) == nullptr) {
    auto* paperLayer = new EoDbLayer(layerName.c_str(), commonState);
    configureLayer(paperLayer);
    paperLayer->SetHandle(layer.m_handle);
    paperLayer->SetOwnerHandle(layer.m_ownerHandle);
    document->AddLayerToSpace(paperLayer, EoDxf::Space::PaperSpace);
  }

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
    convertedLinetype->SetHandle(linetype.m_handle);
    convertedLinetype->SetOwnerHandle(linetype.m_ownerHandle);
    lineTypeTable->SetAt(name, convertedLinetype);
    document->RegisterHandle(convertedLinetype);
  }
}

void EoDbDxfInterface::ConvertTextStyleTable(const EoDxfTextStyle& textStyle, AeSysDoc* document) {
  const auto& textStyleName = textStyle.m_tableName;
  ATLTRACE2(traceGeneral, 3, L"Text Style - Name: %s\n", textStyleName.c_str());

  EoDbTextStyle entry;
  entry.m_name = textStyleName;
  entry.m_height = textStyle.height;
  entry.m_widthFactor = textStyle.width;
  entry.m_obliqueAngle = Eo::DegreeToRadian(textStyle.oblique);
  entry.m_textGenerationFlags = textStyle.m_textGenerationFlag;
  entry.m_lastHeight = textStyle.lastHeight;
  entry.m_font = textStyle.font;
  entry.m_bigFont = textStyle.bigFont;
  entry.m_fontFamily = textStyle.fontFamily;
  entry.m_flagValues = textStyle.m_flagValues;
  entry.m_handle = textStyle.m_handle;
  entry.m_ownerHandle = textStyle.m_ownerHandle;

  document->AddTextStyleEntry(std::move(entry));
}

void EoDbDxfInterface::ConvertVPortTable(const EoDxfVPort& viewport, AeSysDoc* document) {
  const auto& viewportName = viewport.m_tableName;
  ATLTRACE2(traceGeneral, 3, L"Viewport - Name: %s\n", viewportName.c_str());

  EoDbVPortTableEntry entry;
  entry.m_handle = viewport.m_handle;
  entry.m_ownerHandle = viewport.m_ownerHandle;
  entry.m_name = viewportName;
  entry.m_lowerLeftCorner.Set(viewport.m_lowerLeftCorner.x, viewport.m_lowerLeftCorner.y, 0.0);
  entry.m_upperRightCorner.Set(viewport.m_upperRightCorner.x, viewport.m_upperRightCorner.y, 0.0);
  entry.m_viewCenter.Set(viewport.m_viewCenter.x, viewport.m_viewCenter.y, 0.0);
  entry.m_snapBasePoint.Set(viewport.m_snapBasePoint.x, viewport.m_snapBasePoint.y, 0.0);
  entry.m_snapSpacing.Set(viewport.m_snapSpacing.x, viewport.m_snapSpacing.y, 0.0);
  entry.m_gridSpacing.Set(viewport.m_gridSpacing.x, viewport.m_gridSpacing.y, 0.0);
  entry.m_viewDirection.Set(viewport.m_viewDirection.x, viewport.m_viewDirection.y, viewport.m_viewDirection.z);
  entry.m_viewTargetPoint.Set(viewport.m_viewTargetPoint.x, viewport.m_viewTargetPoint.y, viewport.m_viewTargetPoint.z);
  entry.m_viewHeight = viewport.m_viewHeight;
  entry.m_viewAspectRatio = viewport.m_viewAspectRatio;
  entry.m_lensLength = viewport.m_lensLength;
  entry.m_frontClipPlane = viewport.m_frontClipPlane;
  entry.m_backClipPlane = viewport.m_backClipPlane;
  entry.m_snapRotationAngle = viewport.m_snapRotationAngle;
  entry.m_viewTwistAngle = viewport.m_viewTwistAngle;
  entry.m_viewMode = viewport.m_viewMode;
  entry.m_circleZoomPercent = viewport.m_circleZoomPercent;
  entry.m_fastZoom = viewport.m_fastZoom;
  entry.m_ucsIcon = viewport.m_ucsIcon;
  entry.m_snapOn = viewport.m_snapOn;
  entry.m_gridOn = viewport.m_gridOn;
  entry.m_snapStyle = viewport.m_snapStyle;
  entry.m_snapIsopair = viewport.m_snapIsopair;
  entry.m_gridBehavior = viewport.m_gridBehavior;

  document->AddVPortTableEntry(std::move(entry));
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

  // Group codes 3, 1 and 4 are for XREF definition.
  // presence of group code 1

  // @todo Check if block already exists and clean it up first

  //  Block-type bit-coded (see note) which may be combined (group code 70)
  auto* newBlock = new EoDbBlock(block.m_blockTypeFlags,
      EoGePoint3d(block.m_basePoint.x, block.m_basePoint.y, block.m_basePoint.z),  // group codes 10, 20 and 30
      m_blockName.c_str());

  newBlock->SetHandle(block.m_handle);
  newBlock->SetOwnerHandle(block.m_ownerHandle);

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

void EoDbDxfInterface::AddAttrib(const EoDxfAttrib& attrib) {
  if (m_dxfWriter) {
    m_dxfWriter->WriteAttrib(attrib);
    return;
  }
  countOfAttrib++;
  ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddAttrib - entities section\n");
  auto* attribPrimitive = ConvertAttribEntity(attrib, m_document);
  if (attribPrimitive == nullptr) { return; }

  if (m_currentInsertPrimitive != nullptr) {
    auto insertHandle = m_currentInsertPrimitive->Handle();
    attribPrimitive->SetInsertHandle(insertHandle);
    attribPrimitive->SetOwnerHandle(insertHandle);
    m_currentInsertPrimitive->AddAttributeHandle(attribPrimitive->Handle());
  }

  // Add to the same group as the parent INSERT when available
  if (m_currentInsertGroup != nullptr) {
    m_document->RegisterHandle(attribPrimitive);
    m_currentInsertGroup->AddTail(attribPrimitive);
  } else {
    // Orphan ATTRIB or block-definition context — fall back to AddToDocument
    AddToDocument(attribPrimitive, m_document, attrib.m_space);
  }
}

/** @brief Adds the given primitive to the appropriate layer in the document.
 *
 * @param primitive Pointer to the EoDbPrimitive to be added.
 * @param document Pointer to the AeSysDoc where the primitive will be added.
 * @param space The DXF space (model or paper) in which the entity resides.
 */
EoDbGroup* EoDbDxfInterface::AddToDocument(EoDbPrimitive* primitive, AeSysDoc* document, EoDxf::Space space) {
  auto layerName = primitive->LayerName().c_str();
  auto* layer = document->FindLayerInSpace(layerName, space);
  if (layer == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Layer '%s' not found.\n", layerName);
    delete primitive;
    return nullptr;
  }

  ATLTRACE2(traceGeneral, 3, L"AddToDocument: primitive=%p, inBlock=%d, currentBlock=%p, layer='%s'\n", primitive,
      m_inBlockDefinition ? 1 : 0, m_currentOpenBlockDefinition, layerName);

  document->RegisterHandle(primitive);

  if (m_currentOpenBlockDefinition == nullptr) {
    auto* group = new EoDbGroup();

    ATLTRACE2(traceGeneral, 3, L"  -> Creating MODEL SPACE group %p for primitive %p\n", group, primitive);

    group->AddTail(primitive);
    layer->AddTail(group);
    return group;
  }
  ATLTRACE2(traceGeneral, 3, L"  -> Adding to BLOCK at %p\n", m_currentOpenBlockDefinition);

  m_currentOpenBlockDefinition->AddTail(primitive);
  return nullptr;
}

/** @brief Converts a DXF 3DFACE entity to an AeSys closed EoDbPolyline primitive.
 *
 *  A 3DFACE is a three- or four-sided planar surface defined by corner points in WCS (no OCS
 *  transform needed — 3DFACE has no extrusion direction). When the fourth corner coincides with
 *  the third, the face is a triangle; otherwise it is a quadrilateral.
 *
 *  ## Mapping Strategy
 *  - Corners are stored directly in WCS — no OCS→WCS transform is required.
 *  - The face is represented as a closed `EoDbPolyline` (wireframe outline).
 *  - Invisible edge flags (group code 70) are logged but not preserved — `EoDbPolyline` does not
 *    support per-edge visibility. All edges of the resulting polyline are visible.
 *
 *  ## Limitations
 *  - Per-edge visibility (invisible edge flags) is lost. In DXF meshes built from 3DFACEs,
 *    interior edges are typically marked invisible; those will appear in AeSys.
 *  - Fill/shading is not represented — the face renders as wireframe only.
 *
 *  @param _3dFace The parsed DXF 3DFACE entity.
 *  @param document The AeSys document receiving the created primitive.
 */
void EoDbDxfInterface::Convert3dFaceEntity(const EoDxf3dFace& _3dFace, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"3DFACE entity conversion\n");

  const auto& firstCorner = _3dFace.m_firstCorner;
  const auto& secondCorner = _3dFace.m_secondCorner;
  const auto& thirdCorner = _3dFace.m_thirdCorner;
  const auto& fourthCorner = _3dFace.m_fourthCorner;

  // Detect degenerate faces: all corners coincident
  if (firstCorner.IsEqualTo(secondCorner) && secondCorner.IsEqualTo(thirdCorner)) {
    ATLTRACE2(traceGeneral, 1, L"3DFACE entity skipped: degenerate (all corners coincident)\n");
    return;
  }

  // Determine triangle vs quadrilateral: 4th corner == 3rd corner means triangle
  const bool isTriangle = thirdCorner.IsEqualTo(fourthCorner);
  const auto vertexCount = isTriangle ? 3 : 4;

  // Build point array — 3DFACE corners are already in WCS (no OCS transform)
  EoGePoint3dArray points;
  points.SetSize(vertexCount);
  points[0] = EoGePoint3d{firstCorner.x, firstCorner.y, firstCorner.z};
  points[1] = EoGePoint3d{secondCorner.x, secondCorner.y, secondCorner.z};
  points[2] = EoGePoint3d{thirdCorner.x, thirdCorner.y, thirdCorner.z};
  if (!isTriangle) { points[3] = EoGePoint3d{fourthCorner.x, fourthCorner.y, fourthCorner.z}; }

  auto* polylinePrimitive = new EoDbPolyline(points);
  polylinePrimitive->SetBaseProperties(&_3dFace, document);
  polylinePrimitive->SetFlag(EoDbPolyline::sm_Closed);

  if (_3dFace.m_invisibleFlag != 0) {
    ATLTRACE2(traceGeneral, 2,
        L"  3DFACE invisible edge flags=0x%02X (not preserved — EoDbPolyline has no per-edge visibility)\n",
        _3dFace.m_invisibleFlag);
  }

  AddToDocument(polylinePrimitive, document, _3dFace.m_space);

  ATLTRACE2(traceGeneral, 3, L"  3DFACE → closed EoDbPolyline with %d vertices\n", vertexCount);
}

/** @brief Parses an AcGi proxy graphics metafile stream and creates AeSys primitives.
 *
 *  The proxy entity's graphics data (code 92 + code 310 hex chunks) contains a binary recording of
 *  AcGiWorldDraw geometry calls made by the original custom entity's worldDraw() implementation.
 *  This function decodes the binary stream and converts recognized drawing operations into
 *  EoDbLine, EoDbPolyline, and EoDbConic primitives for pseudo-rendering.
 *
 *  ## AcGi Proxy Graphics Metafile Format (ODA Reversed, R2000+ DWG/DXF)
 *
 *  All multi-byte values are little-endian. The stream is a sequence of records, each starting
 *  with an int32 (RL) type code:
 *
 *  | Type  | Operation       | Payload                                            | Status  |
 *  |-------|-----------------|----------------------------------------------------|---------|
 *  | 1     | Extents         | 6 × double (48 bytes): minX,minY,minZ,maxX,maxY,maxZ | Skip  |
 *  | 2     | Circle          | Point3d center, double radius, Vector3d normal (56 bytes) | Render |
 *  | 3     | Circle (3pt)    | Point3d pt1, Point3d pt2, Point3d pt3 (72 bytes)   | Skip    |
 *  | 4     | CircularArc     | Point3d center, double radius, Vector3d normal,    | Render  |
 *  |       |                 | Vector3d startVector, double sweepAngle, int32 type (92 bytes) |  |
 *  | 5     | CircularArc(3pt)| Point3d start, Point3d point, Point3d end (72 bytes) | Skip   |
 *  | 6     | Polyline        | int32 numVertices, Point3d[numVertices]             | Render  |
 *  | 7     | Polygon         | int32 numVertices, Point3d[numVertices]             | Render  |
 *  | 8     | Mesh            | int32 rows, int32 cols, Point3d[rows×cols]          | Skip    |
 *  | 9     | Shell           | int32 numVerts, Point3d[numVerts], int32 numFaces, int32[numFaces] | Skip |
 *  | 10/11 | Text            | Point3d pos, Vector3d normal, Vector3d dir, string  | Skip    |
 *  | 12    | Xline           | Point3d basePoint, Vector3d direction (48 bytes)    | Skip    |
 *  | 13    | Ray             | Point3d basePoint, Vector3d direction (48 bytes)    | Skip    |
 *  | 14–26 | SUBENT mods     | Per-entity attribute overrides (color, layer, etc.) | Skip    |
 *  | 27/28 | Clip Boundary   | Push/Pop (no payload)                               | Skip    |
 *  | 29    | Model Transform | 4×3 matrix (96 bytes)                               | Skip    |
 *  | 33    | LwPolyline      | int32 numPts, int32 flags, Point3d[numPts]          | Skip    |
 *
 *  Point3d = 3 consecutive doubles (24 bytes): x, y, z.
 *  Unknown type codes cause parsing to abort (record lengths are type-dependent).
 *
 *  @param proxyEntity The parsed DXF proxy entity with hex-encoded graphics data.
 *  @param document    The AeSys document receiving the created primitives.
 */
void EoDbDxfInterface::ConvertAcadProxyEntity(const EoDxfAcadProxyEntity& proxyEntity, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"ACAD_PROXY_ENTITY conversion (classId=%d, appClassId=%d)\n",
      proxyEntity.m_proxyEntityClassId, proxyEntity.m_applicationEntityClassId);

  if (!proxyEntity.HasGraphicsData()) {
    ATLTRACE2(traceGeneral, 2, L"  No graphics data — proxy entity skipped\n");
    return;
  }

  const auto declaredSize = proxyEntity.m_graphicsDataSizeInBytes;
  const auto computedSize = proxyEntity.ComputedGraphicsDataSizeInBytes();

  ATLTRACE2(traceGeneral, 2, L"  Graphics data: declared=%d bytes, computed=%d bytes, chunks=%zu\n", declaredSize,
      computedSize, proxyEntity.m_graphicsDataChunks.size());
  ATLTRACE2(traceGeneral, 2, L"  Entity data: %d bits, chunks=%zu\n", proxyEntity.m_entityDataSizeInBits,
      proxyEntity.m_entityDataChunks.size());
  ATLTRACE2(traceGeneral, 2, L"  Handle refs: soft=%zu, hard=%zu, softOwner=%zu, hardOwner=%zu\n",
      proxyEntity.m_softPointerHandles.size(), proxyEntity.m_hardPointerHandles.size(),
      proxyEntity.m_softOwnerHandles.size(), proxyEntity.m_hardOwnerHandles.size());

  if (declaredSize != computedSize) {
    ATLTRACE2(traceGeneral, 1,
        L"  WARNING: Graphics data size mismatch (declared=%d, computed=%d) — data may be truncated\n", declaredSize,
        computedSize);
  }

  // Decode the graphics hex chunks into raw binary for AcGi stream parsing.
  const auto hexData = proxyEntity.ConcatenateGraphicsHexChunks();
  const auto binaryData = EoDxfAcadProxyEntity::DecodeHexToBytes(hexData);
  const auto dataSize = binaryData.size();

  ATLTRACE2(traceGeneral, 2, L"  Decoded %zu bytes of proxy graphics data\n", dataSize);

  if (dataSize == 0) { return; }

  const auto* data = binaryData.data();

  // Helper lambdas for reading little-endian values from the binary stream.
  // Using memcpy ensures correct behavior regardless of alignment.
  auto readInt32 = [data, dataSize](std::size_t offset, std::int32_t& value) -> bool {
    if (offset + sizeof(std::int32_t) > dataSize) { return false; }
    std::memcpy(&value, data + offset, sizeof(std::int32_t));
    return true;
  };

  auto readDouble = [data, dataSize](std::size_t offset, double& value) -> bool {
    if (offset + sizeof(double) > dataSize) { return false; }
    std::memcpy(&value, data + offset, sizeof(double));
    return true;
  };

  auto readPoint3d = [&readDouble](std::size_t offset, EoGePoint3d& point) -> bool {
    return readDouble(offset, point.x) && readDouble(offset + 8, point.y) && readDouble(offset + 16, point.z);
  };

  auto readVector3d = [&readDouble](std::size_t offset, EoGeVector3d& vector) -> bool {
    return readDouble(offset, vector.x) && readDouble(offset + 8, vector.y) && readDouble(offset + 16, vector.z);
  };

  int primitiveCount = 0;
  int skippedGeometryCount = 0;
  std::size_t offset = 0;

  while (offset < dataSize) {
    // ODA format: type code is int32 (RL), 4 bytes little-endian
    std::int32_t typeCode = 0;
    if (!readInt32(offset, typeCode)) {
      ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated type code at offset %zu\n", offset);
      break;
    }
    offset += 4;

    switch (typeCode) {
      case 1: {  // Extents: 6 × double (48 bytes) — bounding box, skip for now
        const std::size_t extentsSize = 48;
        if (offset + extentsSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated extents data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 1 → Extents (skipped, 48 bytes)\n");
        offset += extentsSize;
        break;
      }

      case 2: {  // Circle: Point3d center, double radius, Vector3d normal (56 bytes)
        const std::size_t circleSize = 24 + 8 + 24;
        if (offset + circleSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated circle data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }

        EoGePoint3d center;
        double radius = 0.0;
        EoGeVector3d normal;
        readPoint3d(offset, center);
        readDouble(offset + 24, radius);
        readVector3d(offset + 32, normal);

        if (radius > Eo::geometricTolerance) {
          if (normal.IsNearNull()) { normal = EoGeVector3d::positiveUnitZ; }
          auto* conicPrimitive = EoDbConic::CreateCircle(center, normal, radius);
          conicPrimitive->SetBaseProperties(&proxyEntity, document);
          AddToDocument(conicPrimitive, document, proxyEntity.m_space);
          ++primitiveCount;

          ATLTRACE2(traceGeneral, 3, L"  Proxy type 2 → Circle center(%.2f,%.2f,%.2f) r=%.2f\n", center.x, center.y,
              center.z, radius);
        }

        offset += circleSize;
        break;
      }

      case 4: {  // CircularArc: Point3d center, double radius, Vector3d normal, Vector3d startVector,
                 //              double sweepAngle, int32 arcType (92 bytes)
        const std::size_t circularArcSize = 24 + 8 + 24 + 24 + 8 + 4;
        if (offset + circularArcSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated circular arc data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }

        EoGePoint3d center;
        double radius = 0.0;
        EoGeVector3d normal;
        EoGeVector3d startVector;
        double sweepAngle = 0.0;
        // int32 arcType at offset+88 — not used for radial arc creation (0=open, 1=sector, 2=chord)

        readPoint3d(offset, center);
        readDouble(offset + 24, radius);
        readVector3d(offset + 32, normal);
        readVector3d(offset + 56, startVector);
        readDouble(offset + 80, sweepAngle);

        if (radius > Eo::geometricTolerance && Eo::IsGeometricallyNonZero(sweepAngle)) {
          if (normal.IsNearNull()) { normal = EoGeVector3d::positiveUnitZ; }
          normal.Unitize();

          // Convert WCS startVector to OCS angle via arbitrary axis decomposition.
          // The arbitrary axis algorithm produces the OCS X-axis from the normal vector,
          // then OCS Y = normal × OCS X. Projecting the startVector onto these axes gives
          // the start angle in OCS coordinates.
          auto arbitraryX = ComputeArbitraryAxis(normal);
          arbitraryX.Unitize();
          auto arbitraryY = CrossProduct(normal, arbitraryX);
          arbitraryY.Unitize();

          double startAngle = std::atan2(DotProduct(startVector, arbitraryY), DotProduct(startVector, arbitraryX));
          double endAngle = startAngle + sweepAngle;

          // For negative Z extrusion, mirror angles to match AutoCAD behavior (same as ConvertArcEntity)
          const bool isNegativeExtrusion = normal.z < -Eo::geometricTolerance;
          if (isNegativeExtrusion) {
            startAngle = Eo::TwoPi - startAngle;
            endAngle = Eo::TwoPi - endAngle;
            std::swap(startAngle, endAngle);
          }

          startAngle = EoDbConic::NormalizeTo2Pi(startAngle);
          endAngle = EoDbConic::NormalizeTo2Pi(endAngle);

          auto* conicPrimitive = EoDbConic::CreateRadialArc(center, normal, radius, startAngle, endAngle);
          if (conicPrimitive != nullptr) {
            conicPrimitive->SetBaseProperties(&proxyEntity, document);
            AddToDocument(conicPrimitive, document, proxyEntity.m_space);
            ++primitiveCount;

            ATLTRACE2(traceGeneral, 3, L"  Proxy type 4 → Arc center(%.2f,%.2f,%.2f) r=%.2f start=%.4f end=%.4f\n",
                center.x, center.y, center.z, radius, startAngle, endAngle);
          }
        }

        offset += circularArcSize;
        break;
      }

      case 6:  // Polyline: int32 numVertices, Point3d[numVertices]
      case 7: {  // Polygon: same layout as polyline
        std::int32_t numVertices = 0;
        if (!readInt32(offset, numVertices)) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated vertex count at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += 4;

        if (numVertices <= 0 || numVertices > 10000) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: unreasonable vertex count %d at offset %zu\n", numVertices,
              offset - 4);
          offset = dataSize;
          break;
        }

        const auto pointsSize = static_cast<std::size_t>(numVertices) * 24;
        if (offset + pointsSize > dataSize) {
          ATLTRACE2(traceGeneral, 1,
              L"  Proxy graphics: insufficient data for %d vertices (need %zu bytes, have %zu)\n", numVertices,
              pointsSize, dataSize - offset);
          offset = dataSize;
          break;
        }

        if (numVertices == 2) {
          // Two-point polyline → create EoDbLine
          EoGePoint3d startPoint;
          EoGePoint3d endPoint;
          readPoint3d(offset, startPoint);
          readPoint3d(offset + 24, endPoint);

          auto* linePrimitive = new EoDbLine();
          linePrimitive->SetBaseProperties(&proxyEntity, document);
          linePrimitive->SetLine(EoGeLine(startPoint, endPoint));
          AddToDocument(linePrimitive, document, proxyEntity.m_space);
          ++primitiveCount;

          ATLTRACE2(traceGeneral, 3, L"  Proxy type %d → Line (%.2f,%.2f,%.2f)→(%.2f,%.2f,%.2f)\n", typeCode,
              startPoint.x, startPoint.y, startPoint.z, endPoint.x, endPoint.y, endPoint.z);
        } else {
          // Multi-point polyline or polygon → create EoDbPolyline
          EoGePoint3dArray points;
          points.SetSize(numVertices);
          for (std::int32_t vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex) {
            EoGePoint3d point;
            readPoint3d(offset + static_cast<std::size_t>(vertexIndex) * 24, point);
            points[vertexIndex] = point;
          }

          auto* polylinePrimitive = new EoDbPolyline(points);
          polylinePrimitive->SetBaseProperties(&proxyEntity, document);
          if (typeCode == 7) { polylinePrimitive->SetFlag(EoDbPolyline::sm_Closed); }
          AddToDocument(polylinePrimitive, document, proxyEntity.m_space);
          ++primitiveCount;

          ATLTRACE2(traceGeneral, 3, L"  Proxy type %d → Polyline with %d vertices\n", typeCode, numVertices);
        }

        offset += pointsSize;
        break;
      }

      case 3: {  // Circle (3pt): Point3d pt1, Point3d pt2, Point3d pt3 (72 bytes)
        const std::size_t circle3ptSize = 72;
        if (offset + circle3ptSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated 3pt circle data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type 3 → Circle3pt (not rendered, 72 bytes)\n");
        ++skippedGeometryCount;
        offset += circle3ptSize;
        break;
      }

      case 5: {  // CircularArc (3pt): Point3d start, Point3d point, Point3d end (72 bytes)
        const std::size_t arc3ptSize = 72;
        if (offset + arc3ptSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated 3pt arc data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type 5 → CircularArc3pt (not rendered, 72 bytes)\n");
        ++skippedGeometryCount;
        offset += arc3ptSize;
        break;
      }

      case 8: {  // Mesh: int32 rows, int32 cols, Point3d[rows×cols]
        std::int32_t rows = 0;
        std::int32_t cols = 0;
        if (!readInt32(offset, rows) || !readInt32(offset + 4, cols)) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated mesh header at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        const auto meshPayloadSize = 8 + static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols) * 24;
        if (rows <= 0 || cols <= 0 || offset + meshPayloadSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: invalid mesh %dx%d at offset %zu\n", rows, cols, offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(
            traceGeneral, 2, L"  Proxy type 8 → Mesh %dx%d (not rendered, %zu bytes)\n", rows, cols, meshPayloadSize);
        ++skippedGeometryCount;
        offset += meshPayloadSize;
        break;
      }

      case 9: {  // Shell: int32 numVerts, Point3d[numVerts], int32 numFaceEntries, int32[numFaceEntries]
        std::int32_t numVerts = 0;
        if (!readInt32(offset, numVerts)) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated shell header at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += 4;
        if (numVerts <= 0 || numVerts > 100000) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: unreasonable shell vertex count %d\n", numVerts);
          offset = dataSize;
          break;
        }
        const auto verticesSize = static_cast<std::size_t>(numVerts) * 24;
        if (offset + verticesSize + 4 > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated shell vertices at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += verticesSize;
        std::int32_t numFaceEntries = 0;
        if (!readInt32(offset, numFaceEntries)) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated shell face count at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += 4;
        if (numFaceEntries < 0 || numFaceEntries > 1000000) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: unreasonable shell face entries %d\n", numFaceEntries);
          offset = dataSize;
          break;
        }
        const auto facesSize = static_cast<std::size_t>(numFaceEntries) * 4;
        if (offset + facesSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated shell faces at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type 9 → Shell %d verts, %d face entries (not rendered)\n", numVerts,
            numFaceEntries);
        ++skippedGeometryCount;
        offset += facesSize;
        break;
      }

      case 10:
      case 11: {  // Text: Point3d position, Vector3d normal, Vector3d direction, string
        const std::size_t textHeaderSize = 24 + 24 + 24;  // position + normal + direction = 72 bytes
        if (offset + textHeaderSize + 4 > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated text header at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += textHeaderSize;
        // Text string: int32 charCount followed by charCount × int16 (UTF-16 characters)
        std::int32_t charCount = 0;
        if (!readInt32(offset, charCount)) {
          offset = dataSize;
          break;
        }
        offset += 4;
        if (charCount < 0 || charCount > 100000) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: unreasonable text char count %d\n", charCount);
          offset = dataSize;
          break;
        }
        const auto stringSize = static_cast<std::size_t>(charCount) * 2;
        if (offset + stringSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated text string at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type %d → Text (%d chars, not rendered)\n", typeCode, charCount);
        ++skippedGeometryCount;
        offset += stringSize;
        break;
      }

      case 12:  // Xline: Point3d basePoint, Vector3d direction (48 bytes)
      case 13: {  // Ray: same layout as xline
        const std::size_t xlineSize = 48;
        if (offset + xlineSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated xline/ray data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type %d → Xline/Ray (not rendered, 48 bytes)\n", typeCode);
        ++skippedGeometryCount;
        offset += xlineSize;
        break;
      }

      case 14: {  // SUBENT: Color (int16 colorIndex = 2 bytes)
        const std::size_t payloadSize = 2;
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 14 → SUBENT Color (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 15: {  // SUBENT: Layer name (int16 len, char[len])
        if (offset + 2 > dataSize) {
          offset = dataSize;
          break;
        }
        std::int16_t nameLength = 0;
        std::memcpy(&nameLength, data + offset, sizeof(std::int16_t));
        offset += 2;
        if (nameLength < 0 || offset + static_cast<std::size_t>(nameLength) > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 15 → SUBENT Layer (skipped, %d chars)\n", nameLength);
        offset += static_cast<std::size_t>(nameLength);
        break;
      }

      case 16: {  // SUBENT: Linetype name (int16 len, char[len])
        if (offset + 2 > dataSize) {
          offset = dataSize;
          break;
        }
        std::int16_t nameLength = 0;
        std::memcpy(&nameLength, data + offset, sizeof(std::int16_t));
        offset += 2;
        if (nameLength < 0 || offset + static_cast<std::size_t>(nameLength) > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 16 → SUBENT Linetype (skipped, %d chars)\n", nameLength);
        offset += static_cast<std::size_t>(nameLength);
        break;
      }

      case 17: {  // SUBENT: Marker (int32 = 4 bytes)
        const std::size_t payloadSize = 4;
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 17 → SUBENT Marker (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 18: {  // SUBENT: Fill (int16 = 2 bytes)
        const std::size_t payloadSize = 2;
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 18 → SUBENT Fill (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 20: {  // SUBENT: True Color (int32 = 4 bytes)
        const std::size_t payloadSize = 4;
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 20 → SUBENT True Color (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 21: {  // SUBENT: Lineweight (int16 = 2 bytes)
        const std::size_t payloadSize = 2;
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 21 → SUBENT Lineweight (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 22: {  // SUBENT: Linetype Scale (double = 8 bytes)
        const std::size_t payloadSize = 8;
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 22 → SUBENT Linetype Scale (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 23: {  // SUBENT: Thickness (double = 8 bytes)
        const std::size_t payloadSize = 8;
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 23 → SUBENT Thickness (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 24: {  // SUBENT: Plot Style Name (int32 = 4 bytes)
        const std::size_t payloadSize = 4;
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 24 → SUBENT Plot Style (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 19:  // SUBENT: no payload
      case 25:  // SUBENT: no payload
      case 26: {  // SUBENT: no payload
        ATLTRACE2(traceGeneral, 3, L"  Proxy type %d → SUBENT (no payload, skipped)\n", typeCode);
        break;
      }

      case 27:  // Push Clip Boundary (no payload)
      case 28: {  // Pop Clip Boundary (no payload)
        ATLTRACE2(traceGeneral, 3, L"  Proxy type %d → Clip Boundary (skipped)\n", typeCode);
        break;
      }

      case 29: {  // Push Model Transform: 4×3 matrix (12 × double = 96 bytes)
        const std::size_t xformSize = 96;
        if (offset + xformSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated model transform at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 29 → Push Model Transform (skipped, 96 bytes)\n");
        offset += xformSize;
        break;
      }

      case 33: {  // LwPolyline: int32 numPoints, int32 flags, Point3d[numPoints]
        std::int32_t numPoints = 0;
        if (!readInt32(offset, numPoints)) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated lwpolyline header at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += 4;
        if (numPoints <= 0 || numPoints > 100000) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: unreasonable lwpolyline point count %d\n", numPoints);
          offset = dataSize;
          break;
        }
        // Skip flags (int32) + point data
        const auto lwPolyPayloadSize = 4 + static_cast<std::size_t>(numPoints) * 24;
        if (offset + lwPolyPayloadSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated lwpolyline data for %d points at offset %zu\n",
              numPoints, offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type 33 → LwPolyline %d points (not rendered)\n", numPoints);
        ++skippedGeometryCount;
        offset += lwPolyPayloadSize;
        break;
      }

      default:
        // Unknown type code — cannot determine record length, must abort parsing.
        ATLTRACE2(traceGeneral, 1,
            L"  Proxy graphics: unknown type code %d at offset %zu — aborting parse (%d primitives created)\n",
            typeCode, offset - 4, primitiveCount);
        offset = dataSize;
        break;
    }
  }

  ATLTRACE2(traceGeneral, 2, L"  Proxy entity produced %d primitives", primitiveCount);
  if (skippedGeometryCount > 0) {
    ATLTRACE2(traceGeneral, 2, L", %d geometry records not rendered", skippedGeometryCount);
  }
  ATLTRACE2(traceGeneral, 2, L"\n");
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
  // OCS parametric angles pass through unchanged — ComputeArbitraryAxis in CreateRadialArc
  // encodes the OCS→WCS directional flip via the major axis direction.
  double startAngle = EoDbConic::NormalizeTo2Pi(arc.m_startAngle);
  double endAngle = EoDbConic::NormalizeTo2Pi(arc.m_endAngle);

  auto* radialArc = EoDbConic::CreateRadialArc(center, extrusion, arc.m_radius, startAngle, endAngle);
  if (radialArc == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Failed to create radial arc.\n");
    return;
  }
  radialArc->SetBaseProperties(&arc, document);
  AddToDocument(radialArc, document, arc.m_space);
}

void EoDbDxfInterface::ConvertCircleEntity(const EoDxfCircle& circle, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Circle entity conversion\n");

  if (circle.m_radius < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Circle entity with non-positive radius (%f) skipped.\n", circle.m_radius);
    return;
  }
  EoGePoint3d center(circle.m_centerPoint.x, circle.m_centerPoint.y, circle.m_centerPoint.z);
  EoGeVector3d extrusion(circle.m_extrusionDirection.x, circle.m_extrusionDirection.y, circle.m_extrusionDirection.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Unitize();
  }
  auto* conic = EoDbConic::CreateCircle(center, extrusion, circle.m_radius);
  if (conic == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"Warning: Failed to create circle.\n");
    return;
  }
  conic->SetBaseProperties(&circle, document);
  AddToDocument(conic, document, circle.m_space);
}

/** @brief Converts a DXF Linear/Rotated DIMENSION entity to exploded AeSys primitives.
 *
 *  DXF DIMENSION entities are complex: they carry definition points and a reference to a dimension
 *  style, and AutoCAD uses these to generate block geometry (extension lines, dimension line,
 *  arrowheads, text). AeSys does not have a native dimension engine, so this converter produces
 *  **exploded geometry** — the same primitives AutoCAD would generate, computed from definition
 *  points and dimstyle variables.
 *
 *  ## DXF Group Code Summary (Linear/Rotated)
 *  | Code | Field | Notes |
 *  |------|-------|-------|
 *  | 10/20/30 | Definition point | On the dimension line (WCS) |
 *  | 11/21/31 | Text midpoint | Middle of dimension text (OCS) |
 *  | 13/23/33 | Extension line 1 origin | First measured feature point (WCS) |
 *  | 14/24/34 | Extension line 2 origin | Second measured feature point (WCS) |
 *  | 50 | Rotation angle | Degrees — 0=horizontal, 90=vertical |
 *  | 52 | Oblique angle | Extension line oblique angle (degrees, optional) |
 *  | 3 | Dimension style name | Lookup in document dimstyle table |
 *
 *  ## Geometry Construction
 *  1. The measurement direction is defined by the rotation angle.
 *  2. The perpendicular direction defines the extension line run.
 *  3. The dimension line Y-level (in the rotated frame) is derived from the definition point.
 *  4. Extension lines run from the feature origins (offset by dimexo) to the dimension line
 *     (extended by dimexe).
 *  5. Tick marks at dimension line endpoints: parallel 45° oblique diagonals crossing the
 *     dimension line. Rendered when dimtsz > 0 (explicit tick size) or as a fallback when
 *     dimasz > 0 and dimtsz == 0 (arrowhead block names are not rendered).
 *  6. Dimension text is placed at the DXF text midpoint (group 11/21/31) with Middle vertical
 *     alignment — the midpoint already incorporates dimtad and dimgap offsets.
 *  7. Text orientation: when the measurement direction points left (angle 90°–270°), text axes
 *     are flipped 180° so dimension text always reads left-to-right.
 *  8. Measurement formatting: dimlunit controls linear unit format (Decimal, Architectural,
 *     Engineering, Scientific, Fractional). dimlfac scales the raw measurement. dimrnd rounds
 *     to the specified increment. dimzin controls zero suppression for architectural/fractional
 *     formats. dimdsep overrides the decimal separator.
 *
 *  ## dimfrac Clarification
 *  `dimfrac` (group 276) controls **fraction stacking display** in Architectural (dimlunit=4)
 *  and Fractional (dimlunit=5) formats: 0=horizontal bar, 1=diagonal bar, 2=inline (not stacked).
 *  It does NOT switch between fractional and decimal inch display. All three values produce the
 *  same fractional number text; only the visual rendering of the fraction differs. AeSys stroke
 *  font renders fractions inline regardless, so dimfrac is informational only.
 *
 *  ## dimlunit/dimunit Fallback
 *  `dimlunit` (group 277, AC1015+) is the authoritative linear unit format variable. If zero or
 *  absent, the legacy `dimunit` (group 270, AC1012+) is consulted. If both are zero, defaults
 *  to Decimal (2).
 *
 *  ## Limitations
 *  - Arrowhead blocks (dimblk/dimblk1/dimblk2) are not rendered — oblique ticks are used as fallback.
 *  - Oblique extension lines (code 52) are not yet implemented.
 *  - Text is always placed at the DXF text midpoint; dimgap is not clipped from the dimension line.
 *  - Tolerance text (dimtol/dimlim) is not generated.
 *  - Alternate units (dimalt) are not generated.
 *  - Architectural fractions use inline display regardless of dimfrac (stacking not supported).
 *
 *  @param dimension The parsed DXF DIMENSION entity (Linear subtype).
 *  @param document The AeSys document receiving the created primitives.
 */
void EoDbDxfInterface::ConvertDimLinearEntity(const EoDxfDimLinear& dimension, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"DimLinear entity conversion\n");

  // --- Resolve dimension style ---
  const auto* dimStyle = document->FindDimStyle(dimension.GetDimensionStyleName());
  if (dimStyle == nullptr) {
    ATLTRACE2(traceGeneral, 1, L"DimLinear: dimension style '%s' not found, using defaults\n",
        dimension.GetDimensionStyleName().c_str());
  }

  // DimStyle variables with defaults matching the DXF "Standard" style.
  // When dimscale is 0 or negative, treat as 1.0 (DXF convention: 0 = "use layout scale").
  const double rawDimscale = dimStyle != nullptr ? dimStyle->dimscale : 1.0;
  const double dimscale = rawDimscale > Eo::geometricTolerance ? rawDimscale : 1.0;
  const double dimasz = (dimStyle != nullptr ? dimStyle->dimasz : 0.18) * dimscale;
  const double dimexo = (dimStyle != nullptr ? dimStyle->dimexo : 0.0625) * dimscale;
  const double dimexe = (dimStyle != nullptr ? dimStyle->dimexe : 0.18) * dimscale;
  const double dimtxt = (dimStyle != nullptr ? dimStyle->dimtxt : 0.18) * dimscale;
  const double dimtsz = (dimStyle != nullptr ? dimStyle->dimtsz : 0.0) * dimscale;
  const double dimdle = (dimStyle != nullptr ? dimStyle->dimdle : 0.0) * dimscale;
  const std::int16_t dimse1 = dimStyle != nullptr ? dimStyle->dimse1 : 0;
  const std::int16_t dimse2 = dimStyle != nullptr ? dimStyle->dimse2 : 0;
  [[maybe_unused]] const std::int16_t dimtad = dimStyle != nullptr ? dimStyle->dimtad : 0;
  const std::int16_t dimclrd = dimStyle != nullptr ? dimStyle->dimclrd : 0;  // 0 = ByBlock
  const std::int16_t dimclre = dimStyle != nullptr ? dimStyle->dimclre : 0;
  const std::int16_t dimclrt = dimStyle != nullptr ? dimStyle->dimclrt : 0;
  const double dimlfac = dimStyle != nullptr ? dimStyle->dimlfac : 1.0;
  const double dimrnd = dimStyle != nullptr ? dimStyle->dimrnd : 0.0;
  const std::int16_t dimzin = dimStyle != nullptr ? dimStyle->dimzin : 0;
  const std::int16_t dimdsep = dimStyle != nullptr ? dimStyle->dimdsep : 0;  // 0 = default '.'
  const std::wstring dimblk = dimStyle != nullptr ? dimStyle->dimblk : std::wstring{};

  // Resolve dimlunit: prefer dimlunit (group 277, AC1015+), fall back to dimunit (group 270, pre-R2000).
  // Both use the same value encoding: 1=Scientific, 2=Decimal, 3=Engineering, 4=Architectural, 5=Fractional.
  // dimunit (group 270) is obsolete in R2000+ but may be the only value set in older DXF files or converters.
  std::int16_t resolvedDimlunit = dimStyle != nullptr ? dimStyle->dimlunit : 2;
  if (resolvedDimlunit <= 0 && dimStyle != nullptr) {
    resolvedDimlunit = dimStyle->dimunit;
  }
  if (resolvedDimlunit <= 0 || resolvedDimlunit > 6) {
    resolvedDimlunit = 2;  // Default to Decimal
  }

  // --- Extract definition points ---
  const auto defPt = dimension.GetDefinitionPoint();      // On the dimension line (WCS)
  const auto extPt1 = dimension.GetExtensionLinePoint1();  // Feature point 1 (WCS)
  const auto extPt2 = dimension.GetExtensionLinePoint2();  // Feature point 2 (WCS)
  const auto textPtOcs = dimension.GetTextPoint();          // Text midpoint (OCS per DXF spec)

  // Transform text midpoint from OCS → WCS using the entity's extrusion direction.
  // DXF DIMENSION group codes 11/21/31 are in OCS; other definition points (10,13,14) are WCS.
  const EoGeVector3d dimExtrusionDirection{
      dimension.m_extrusionDirection.x, dimension.m_extrusionDirection.y, dimension.m_extrusionDirection.z};
  const bool dimNeedsOcsTransform = Eo::IsGeometricallyNonZero(dimExtrusionDirection.x) ||
      Eo::IsGeometricallyNonZero(dimExtrusionDirection.y) ||
      Eo::IsGeometricallyNonZero(dimExtrusionDirection.z - 1.0);

  EoDxfGeometryBase3d textPt = textPtOcs;
  if (dimNeedsOcsTransform) {
    EoGeOcsTransform dimOcsTransform{dimExtrusionDirection};
    auto transformedTextPt = EoGePoint3d{textPtOcs.x, textPtOcs.y, textPtOcs.z};
    transformedTextPt = dimOcsTransform * transformedTextPt;
    textPt.x = transformedTextPt.x;
    textPt.y = transformedTextPt.y;
    textPt.z = transformedTextPt.z;
  }

  // --- Build measurement direction from rotation angle ---
  const double rotationRadians = Eo::DegreeToRadian(dimension.GetRotationAngle());
  const double cosRot = std::cos(rotationRadians);
  const double sinRot = std::sin(rotationRadians);

  // Measurement direction (along the dimension line) and perpendicular (extension line run)
  const EoGeVector3d measureDir{cosRot, sinRot, 0.0};
  const EoGeVector3d extDir{-sinRot, cosRot, 0.0};

  // --- Project feature points onto the dimension line ---
  // The dimension line passes through defPt perpendicular to extDir.
  // dimLineLevel = how far along extDir the dimension line sits (from origin).
  const EoGeVector3d defVec{defPt.x, defPt.y, defPt.z};
  const double dimLineLevel = DotProduct(EoGeVector3d{defVec}, extDir);

  // Project extension line origins along extDir
  const double ext1Level = DotProduct(EoGeVector3d{extPt1.x, extPt1.y, extPt1.z}, extDir);
  const double ext2Level = DotProduct(EoGeVector3d{extPt2.x, extPt2.y, extPt2.z}, extDir);

  // Positions along measureDir for each extension line origin
  const double ext1Along = DotProduct(EoGeVector3d{extPt1.x, extPt1.y, extPt1.z}, measureDir);
  const double ext2Along = DotProduct(EoGeVector3d{extPt2.x, extPt2.y, extPt2.z}, measureDir);

  // Dimension line endpoint positions (projected feature points at dim line level)
  const EoGePoint3d dimLinePt1{
      extPt1.x + extDir.x * (dimLineLevel - ext1Level),
      extPt1.y + extDir.y * (dimLineLevel - ext1Level),
      extPt1.z};
  const EoGePoint3d dimLinePt2{
      extPt2.x + extDir.x * (dimLineLevel - ext2Level),
      extPt2.y + extDir.y * (dimLineLevel - ext2Level),
      extPt2.z};

  // --- Extension line geometry ---
  // Extension lines run from feature origin (offset by dimexo) toward the dimension line
  // (extended by dimexe past it). The direction is from the feature point toward the dim line.
  auto buildExtensionLine = [&](const EoDxfGeometryBase3d& featurePoint, double featureLevel,
                                 const EoGePoint3d& dimLinePoint) -> std::pair<EoGePoint3d, EoGePoint3d> {
    const double totalRun = dimLineLevel - featureLevel;
    const double sign = totalRun >= 0.0 ? 1.0 : -1.0;

    const EoGePoint3d startPoint{
        featurePoint.x + extDir.x * dimexo * sign,
        featurePoint.y + extDir.y * dimexo * sign,
        featurePoint.z};
    const EoGePoint3d endPoint{
        dimLinePoint.x + extDir.x * dimexe * sign,
        dimLinePoint.y + extDir.y * dimexe * sign,
        dimLinePoint.z};
    return {startPoint, endPoint};
  };

  // --- Create extension line 1 ---
  if (dimse1 == 0) {
    auto [start1, end1] = buildExtensionLine(extPt1, ext1Level, dimLinePt1);
    auto* extLine1 = EoDbLine::CreateLine(start1, end1);
    extLine1->SetBaseProperties(&dimension, document);
    if (dimclre != 0) { extLine1->SetColor(dimclre); }
    AddToDocument(extLine1, document, dimension.m_space);
  }

  // --- Create extension line 2 ---
  if (dimse2 == 0) {
    auto [start2, end2] = buildExtensionLine(extPt2, ext2Level, dimLinePt2);
    auto* extLine2 = EoDbLine::CreateLine(start2, end2);
    extLine2->SetBaseProperties(&dimension, document);
    if (dimclre != 0) { extLine2->SetColor(dimclre); }
    AddToDocument(extLine2, document, dimension.m_space);
  }

  // --- Create dimension line ---
  // When dimdle > 0, the dimension line extends past the extension lines
  const EoGePoint3d dimLineStart{
      dimLinePt1.x - measureDir.x * dimdle,
      dimLinePt1.y - measureDir.y * dimdle,
      dimLinePt1.z};
  const EoGePoint3d dimLineEnd{
      dimLinePt2.x + measureDir.x * dimdle,
      dimLinePt2.y + measureDir.y * dimdle,
      dimLinePt2.z};

  auto* dimLine = EoDbLine::CreateLine(dimLineStart, dimLineEnd);
  dimLine->SetBaseProperties(&dimension, document);
  if (dimclrd != 0) { dimLine->SetColor(dimclrd); }
  AddToDocument(dimLine, document, dimension.m_space);

  // --- Create tick marks or arrowhead stubs ---
  // Tick marks are short diagonal lines at 45° across the dimension line endpoints.
  // When dimtsz > 0, use dimtsz as the tick size (explicit architectural tick).
  // When dimtsz == 0 and dimasz > 0, check dimblk for oblique/tick block names and
  // render tick marks using dimasz. Known tick block names: _OBLIQUE, OBLIQUE, _DOT,
  // _DOTSMALL, _DOTBLANK, _SMALL, _OPEN, _CLOSED, etc.
  // As a general fallback when dimtsz == 0 and dimasz > 0: render oblique ticks using
  // dimasz (better than nothing for non-associative exploded dimensions).
  const bool hasExplicitTick = dimtsz > Eo::geometricTolerance;
  const bool hasArrowFallback = !hasExplicitTick && dimasz > Eo::geometricTolerance;
  const double tickSize = hasExplicitTick ? dimtsz : dimasz;

  if (hasExplicitTick || hasArrowFallback) {
    // AutoCAD oblique ticks are parallel 45° diagonal lines crossing the dimension line.
    // Both endpoints use the same offset direction (measureDir + extDir) so the ticks
    // are parallel rather than forming a cross.
    const EoGeVector3d tickOffset = (measureDir + extDir) * (tickSize * 0.5);

    auto* tick1 = EoDbLine::CreateLine(dimLinePt1 - tickOffset, dimLinePt1 + tickOffset);
    tick1->SetBaseProperties(&dimension, document);
    if (dimclrd != 0) { tick1->SetColor(dimclrd); }
    AddToDocument(tick1, document, dimension.m_space);

    auto* tick2 = EoDbLine::CreateLine(dimLinePt2 - tickOffset, dimLinePt2 + tickOffset);
    tick2->SetBaseProperties(&dimension, document);
    if (dimclrd != 0) { tick2->SetColor(dimclrd); }
    AddToDocument(tick2, document, dimension.m_space);
  }

  // --- Create dimension text ---
  // Use explicit dimension text if provided, otherwise compute the measurement value.
  std::wstring dimensionText;
  const auto& explicitText = dimension.GetExplicitDimensionText();
  if (!explicitText.empty()) {
    dimensionText = explicitText;
  } else {
    // Compute the measurement: distance between feature points projected onto the measurement direction
    const double rawMeasurement = std::abs(ext2Along - ext1Along);
    // Apply linear measurement scale factor (dimlfac)
    double measurement = rawMeasurement * (Eo::IsGeometricallyNonZero(dimlfac) ? dimlfac : 1.0);

    // Apply dimrnd rounding: rounds the measurement to the nearest dimrnd increment.
    // For example, dimrnd=0.25 rounds to nearest quarter unit; dimrnd=0.5 to nearest half.
    // dimrnd=0.0 (default) means no rounding — precision is controlled only by dimdec.
    if (dimrnd > Eo::geometricTolerance) {
      measurement = std::round(measurement / dimrnd) * dimrnd;
    }

    const int decimalPlaces = dimStyle != nullptr ? dimStyle->dimdec : 4;

    ATLTRACE2(traceGeneral, 2,
        L"  DimLinear text: dimlunit=%d (raw=%d, dimunit=%d), dimdec=%d, dimrnd=%.6f, measurement=%.10f\n",
        resolvedDimlunit, dimStyle ? dimStyle->dimlunit : -1, dimStyle ? dimStyle->dimunit : -1, decimalPlaces, dimrnd,
        measurement);

    // Format based on resolvedDimlunit (linear unit format)
    switch (resolvedDimlunit) {
      case 4: {  // Architectural: feet'-inches fraction"
        // dimfrac (group 276) controls fraction stacking display (0=horizontal, 1=diagonal, 2=inline)
        // but does NOT change the number format — all three produce the same fractional text.
        // AeSys stroke font renders fractions inline regardless, so dimfrac is informational only.
        constexpr wchar_t kInchMark = 0x22;  // '"' — avoids escaping issues
        const double totalInches = measurement;
        int feet = static_cast<int>(totalInches / 12.0);
        double remainingInches = totalInches - feet * 12.0;

        // Clean floating-point noise: snap near-zero and near-12 remainders.
        // This eliminates artifacts like 360.0 - 30*12 = 1.137e-12 instead of 0.
        if (std::abs(remainingInches) < 1e-9) {
          remainingInches = 0.0;
        } else if (std::abs(remainingInches - 12.0) < 1e-9) {
          remainingInches = 0.0;
          ++feet;
        }

        const int wholeInches = static_cast<int>(remainingInches);
        const double fractionalPart = remainingInches - wholeInches;

        // Find nearest fraction (1/2, 1/4, 1/8, 1/16, 1/32, 1/64)
        // Use precision from dimdec: 0→1, 1→2, 2→4, 3→8, 4→16, 5→32, 6→64
        const int denominator = 1 << (decimalPlaces > 0 ? decimalPlaces : 4);
        const int numerator = static_cast<int>(std::round(fractionalPart * denominator));

        std::wstring result;
        // dimzin bit 2: suppress 0 feet; bit 3: suppress 0 inches
        const bool suppressLeadingZeroFeet = (dimzin & 0x04) != 0;
        const bool suppressTrailingZeroInches = (dimzin & 0x08) != 0;

        if (feet != 0 || !suppressLeadingZeroFeet) {
          result += std::to_wstring(feet);
          result += L"'-";
        }

        if (numerator == 0 || numerator == denominator) {
          // No fraction — just whole inches
          const int adjustedInches = wholeInches + (numerator == denominator ? 1 : 0);
          if (adjustedInches != 0 || !suppressTrailingZeroInches || result.empty()) {
            result += std::to_wstring(adjustedInches);
            result += kInchMark;
          } else if (!result.empty() && result.back() == L'-') {
            // Remove trailing dash when suppressing zero inches
            result.pop_back();
            result += kInchMark;
          }
        } else {
          // Reduce fraction to lowest terms
          int num = numerator;
          int den = denominator;
          while (num > 0 && den > 0 && num % 2 == 0 && den % 2 == 0) {
            num /= 2;
            den /= 2;
          }
          if (wholeInches != 0) {
            result += std::to_wstring(wholeInches) + L' ';
          }
          result += std::to_wstring(num);
          result += L'/';
          result += std::to_wstring(den);
          result += kInchMark;
        }
        dimensionText = result;
        break;
      }
      case 3: {  // Engineering: feet'-decimal inches"
        constexpr wchar_t kInchMark = 0x22;  // '"' — avoids escaping issues
        const double totalInches = measurement;
        int feet = static_cast<int>(totalInches / 12.0);
        double remainingInches = totalInches - feet * 12.0;

        // Clean floating-point noise: snap near-zero and near-12 remainders
        if (std::abs(remainingInches) < 1e-9) {
          remainingInches = 0.0;
        } else if (std::abs(remainingInches - 12.0) < 1e-9) {
          remainingInches = 0.0;
          ++feet;
        }

        wchar_t buffer[64]{};
        std::swprintf(buffer, std::size(buffer), L"%d'-%.*f", feet, decimalPlaces, remainingInches);
        dimensionText = buffer;
        dimensionText += kInchMark;
        break;
      }
      case 1: {  // Scientific: mantissa + exponent
        wchar_t buffer[64]{};
        std::swprintf(buffer, std::size(buffer), L"%.*E", decimalPlaces, measurement);
        dimensionText = buffer;
        break;
      }
      case 5: {  // Fractional
        const int whole = static_cast<int>(measurement);
        const double fractionalPart = measurement - whole;
        const int denominator = 1 << (decimalPlaces > 0 ? decimalPlaces : 4);
        int numerator = static_cast<int>(std::round(fractionalPart * denominator));
        if (numerator == 0) {
          dimensionText = std::to_wstring(whole);
        } else {
          int num = numerator;
          int den = denominator;
          while (num > 0 && den > 0 && num % 2 == 0 && den % 2 == 0) {
            num /= 2;
            den /= 2;
          }
          if (whole != 0) {
            dimensionText = std::to_wstring(whole) + L' ';
          }
          dimensionText += std::to_wstring(num) + L'/' + std::to_wstring(den);
        }
        break;
      }
      default: {  // case 2: Decimal (default), case 6: Windows Desktop
        wchar_t buffer[64]{};
        std::swprintf(buffer, std::size(buffer), L"%.*f", decimalPlaces, measurement);
        dimensionText = buffer;

        // Apply dimdsep (decimal separator override)
        if (dimdsep != 0 && dimdsep != L'.') {
          for (auto& ch : dimensionText) {
            if (ch == L'.') {
              ch = static_cast<wchar_t>(dimdsep);
              break;
            }
          }
        }
        break;
      }
    }
  }

  if (!dimensionText.empty() && dimtxt > Eo::geometricTolerance) {
    // Build text reference system at the text midpoint, oriented along the measurement direction
    const EoGePoint3d textOrigin{textPt.x, textPt.y, textPt.z};

    // Ensure text reads left-to-right (or bottom-to-top for vertical dimensions).
    // When the measurement direction angle is in the 90°–270° range (pointing generally left),
    // flip both text axes by 180° so the text is not rendered upside down.
    auto textMeasureDir = measureDir;
    auto textExtDir = extDir;
    const double measureAngle = std::atan2(measureDir.y, measureDir.x);
    if (measureAngle > Eo::HalfPi + Eo::geometricTolerance ||
        measureAngle < -(Eo::HalfPi + Eo::geometricTolerance)) {
      textMeasureDir = EoGeVector3d{-measureDir.x, -measureDir.y, -measureDir.z};
      textExtDir = EoGeVector3d{-extDir.x, -extDir.y, -extDir.z};
    }

    auto xAxisDirection = textMeasureDir;
    auto yAxisDirection = textExtDir;

    // Scale directions for the reference system
    // xDirection encodes: widthScale × height × defaultCharacterCellAspectRatio
    // yDirection encodes: text height
    const double widthScale = 1.0;
    yAxisDirection *= dimtxt;
    xAxisDirection *= widthScale * dimtxt * Eo::defaultCharacterCellAspectRatio;

    EoGeReferenceSystem referenceSystem(textOrigin, xAxisDirection, yAxisDirection);

    // Resolve text style from dimstyle (dimtxsty is a handle string — look up the style name)
    EoDbFontDefinition fontDefinition{};
    if (dimStyle != nullptr && !dimStyle->dimtxsty.empty()) {
      // dimtxsty is stored as a handle reference; for now use the document's default text style
      // since resolving handle→name requires handle map lookup which may not be populated for styles
    }

    // Center the text horizontally; always use Middle vertical alignment.
    // The DXF text midpoint (group 11/21/31) is positioned by AutoCAD to be the CENTER
    // of the dimension text, already accounting for dimtad and dimgap offsets. Using
    // Middle alignment places the text correctly at that midpoint regardless of dimtad.
    fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Center);
    fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);

    auto* textPrimitive = new EoDbText(fontDefinition, referenceSystem, dimensionText);
    textPrimitive->SetBaseProperties(&dimension, document);
    if (dimclrt != 0) { textPrimitive->SetColor(dimclrt); }
    AddToDocument(textPrimitive, document, dimension.m_space);
  }
}

/** @brief Converts a DXF ELLIPSE entity to an EoDbConic primitive in the AeSys document.
 *
 *  DXF ELLIPSE entities represent both full ellipses and elliptical arcs. The entity is defined
 *  by a center point, a major axis endpoint (relative to center), a minor-to-major axis ratio,
 *  and start/end parameter angles. When start=0 and end=2π, it is a full ellipse.
 *
 *  ## DXF Group Code Summary
 *  | Code | Field | Notes |
 *  |------|-------|-------|
 *  | 10/20/30 | Center point | In WCS (unlike ARC/CIRCLE which use OCS) |
 *  | 11/21/31 | Major axis endpoint | Relative to center, in WCS |
 *  | 40 | Ratio | Minor/major, (0.0, 1.0] |
 *  | 41 | Start parameter | Radians, 0.0 for full ellipse |
 *  | 42 | End parameter | Radians, 2π for full ellipse |
 *  | 210/220/230 | Extrusion direction | WCS — defines ellipse plane normal |
 *
 *  ## Coordinate System
 *  Per the DXF specification, ELLIPSE center and major axis endpoint are in WCS. No OCS→WCS
 *  transform is applied (unlike ARC/CIRCLE). The extrusion direction defines the ellipse plane
 *  normal and is used only by MinorAxis() = Cross(extrusion, majorAxis) × ratio.
 *
 *  @param ellipse The parsed DXF ELLIPSE entity.
 *  @param document The AeSys document receiving the created primitive.
 */
void EoDbDxfInterface::ConvertEllipseEntity(const EoDxfEllipse& ellipse, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Ellipse entity conversion (ratio=%.4f, startParam=%.4f, endParam=%.4f)\n",
      ellipse.m_ratio, ellipse.m_startParam, ellipse.m_endParam);

  if (ellipse.m_ratio <= 0.0 || ellipse.m_ratio > 1.0) {
    ATLTRACE2(
        traceGeneral, 1, L"Ellipse entity skipped: invalid ratio (%.6f) — must be in (0.0, 1.0]\n", ellipse.m_ratio);
    return;
  }
  EoGeVector3d majorAxis(
      ellipse.m_endPointOfMajorAxis.x, ellipse.m_endPointOfMajorAxis.y, ellipse.m_endPointOfMajorAxis.z);
  if (majorAxis.IsNearNull()) {
    ATLTRACE2(traceGeneral, 1, L"Ellipse entity skipped: zero-length major axis\n");
    return;
  }
  EoGeVector3d extrusion(
      ellipse.m_extrusionDirection.x, ellipse.m_extrusionDirection.y, ellipse.m_extrusionDirection.z);
  if (extrusion.IsNearNull()) {
    extrusion = EoGeVector3d::positiveUnitZ;
  } else {
    extrusion.Unitize();
  }
  auto center = EoGePoint3d(ellipse.m_centerPoint.x, ellipse.m_centerPoint.y, ellipse.m_centerPoint.z);

  ATLTRACE2(traceGeneral, 3, L"  center=(%.4f, %.4f, %.4f) majorAxis=(%.4f, %.4f, %.4f) majorLen=%.4f\n", center.x,
      center.y, center.z, majorAxis.x, majorAxis.y, majorAxis.z, majorAxis.Length());
  ATLTRACE2(traceGeneral, 3, L"  extrusion=(%.4f, %.4f, %.4f) ratio=%.4f\n", extrusion.x, extrusion.y, extrusion.z,
      ellipse.m_ratio);

  auto* conic =
      EoDbConic::CreateConic(center, extrusion, majorAxis, ellipse.m_ratio, ellipse.m_startParam, ellipse.m_endParam);
  if (conic == nullptr) {
    ATLTRACE2(traceGeneral, 1, L"Ellipse entity skipped: CreateConic returned nullptr\n");
    return;
  }
  conic->SetBaseProperties(&ellipse, document);
  AddToDocument(conic, document, ellipse.m_space);

  const bool isFullEllipse = Eo::IsGeometricallyZero(ellipse.m_endParam - ellipse.m_startParam - Eo::TwoPi) ||
      Eo::IsGeometricallyZero(ellipse.m_endParam - ellipse.m_startParam);
  ATLTRACE2(traceGeneral, 2, L"  → EoDbConic %s (majorLen=%.4f, minorLen=%.4f)\n",
      isFullEllipse ? L"Ellipse" : L"EllipticalArc", majorAxis.Length(), majorAxis.Length() * ellipse.m_ratio);
}

/** @brief Maps a DXF hatch pattern name to the AeSys fill style index.
 *
 * Pattern names are matched case-insensitively against the built-in set loaded
 * from DefaultSet.txt.  Returns 0 (no match) when the name is unrecognized,
 * which causes DisplayFilAreaHatch to early-return harmlessly.
 *
 * @param patternName  The DXF pattern name (group code 2).
 * @return The 1-based fill style index, or 0 if no match.
 */
static std::int16_t MapHatchPatternNameToIndex(const std::wstring& patternName) {
  // Table mirrors DefaultSet.txt load order (1-based indices 1–39)
  static const struct {
    const wchar_t* name;
    std::int16_t index;
  } patternTable[] = {{L"PEG1", 1}, {L"PEG2", 2}, {L"ANGLE", 3}, {L"ANSI31", 4}, {L"ANSI32", 5}, {L"ANSI33", 6},
      {L"ANSI34", 7}, {L"ANSI35", 8}, {L"ANSI36", 9}, {L"ANSI37", 10}, {L"ANSI38", 11}, {L"BOX", 12}, {L"BRICK", 13},
      {L"CLAY", 14}, {L"CORK", 15}, {L"CROSS", 16}, {L"DASH", 17}, {L"DOLMIT", 18}, {L"DOTS", 19}, {L"EARTH", 20},
      {L"ESCHER", 21}, {L"FLEX", 22}, {L"GRASS", 23}, {L"GRATE", 24}, {L"HEX", 25}, {L"HONEY", 26}, {L"HOUND", 27},
      {L"INSUL", 28}, {L"MUDST", 29}, {L"NET3", 30}, {L"PLAST", 31}, {L"PLASTI", 32}, {L"SACNCR", 33}, {L"SQUARE", 34},
      {L"STARS", 35}, {L"SWAMP", 36}, {L"TRANS", 37}, {L"TRIAN", 38}, {L"ZIGZAG", 39}, {L"AR-CONC", 40},
      {L"AR-SAND", 41}};

  for (const auto& entry : patternTable) {
    if (_wcsicmp(patternName.c_str(), entry.name) == 0) { return entry.index; }
  }
  return 0;  // Unrecognized pattern — caller falls back to Hollow
}

/** @brief Converts a DXF HATCH entity to one or more EoDbPolygon primitives.
 *
 * Each hatch boundary loop becomes a separate EoDbPolygon. Polyline boundaries
 * are tessellated (bulge arcs expanded); edge-type boundaries chain line, arc,
 * and ellipse edges into a closed vertex array. Spline edges are logged and skipped.
 *
 * Solid-fill hatches map to PolygonStyle::Solid. Pattern hatches map to
 * PolygonStyle::Hatch with reference vectors derived from the DXF pattern angle
 * and scale. Hollow hatches (solidFillFlag == 0 with pattern name "SOLID" absent)
 * map to PolygonStyle::Hollow.
 *
 * Boundary points are in OCS; when the extrusion normal differs from positive Z,
 * the OCS-to-WCS transform is applied to all points and reference vectors.
 *
 * Island loops (neither external nor outermost) are rendered as Hollow polygons
 * when the outer hatch is solid-filled, creating the visual "hole" effect.
 *
 * @param hatch  The parsed DXF HATCH entity.
 * @param document  The AeSysDoc receiving the converted primitives.
 */
void EoDbDxfInterface::ConvertHatchEntity(const EoDxfHatch& hatch, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Hatch entity conversion - Pattern: %s, Loops: %d, Solid: %d\n",
      hatch.m_hatchPatternName.c_str(), static_cast<int>(hatch.HatchLoops().size()), hatch.m_solidFillFlag);

  if (hatch.HatchLoops().empty()) { return; }

  // Build OCS→WCS transform from the entity's extrusion direction
  const EoGeVector3d extrusionNormal{
      hatch.m_extrusionDirection.x, hatch.m_extrusionDirection.y, hatch.m_extrusionDirection.z};
  const EoGeOcsTransform ocsToWcs = EoGeOcsTransform::CreateOcsToWcs(extrusionNormal);
  const bool needsOcsTransform = !ocsToWcs.IsWorldCoordinateSystem();

  // Determine polygon style from DXF hatch properties
  EoDb::PolygonStyle polygonStyle = EoDb::PolygonStyle::Hollow;
  std::int16_t fillStyleIndex = 0;

  if (hatch.m_solidFillFlag == 1) {
    polygonStyle = EoDb::PolygonStyle::Solid;
  } else if (hatch.m_hatchPatternName == L"HOLLOW" || hatch.m_hatchPatternName.empty()) {
    polygonStyle = EoDb::PolygonStyle::Hollow;
  } else {
    // Pattern hatch — map DXF name to AeSys fill style index
    polygonStyle = EoDb::PolygonStyle::Hatch;
    fillStyleIndex = MapHatchPatternNameToIndex(hatch.m_hatchPatternName);
    if (fillStyleIndex == 0) {
      ATLTRACE2(traceGeneral, 1, L"  Unrecognized hatch pattern name \"%s\" — falling back to Hollow\n",
          hatch.m_hatchPatternName.c_str());
      polygonStyle = EoDb::PolygonStyle::Hollow;
    }
  }

  if (hatch.m_hatchPatternDoubleFlag != 0) {
    ATLTRACE2(traceGeneral, 1, L"  Hatch pattern double flag set — second 90° pass not implemented in PEG V1\n");
  }

  // Hatch origin from the elevation point (OCS origin for the hatch plane)
  EoGePoint3d hatchOrigin{hatch.m_elevationPoint.x, hatch.m_elevationPoint.y, hatch.m_elevationPoint.z};

  // Build reference vectors from pattern angle and scale
  // For solid fills, use identity-like reference vectors (unit X and Y)
  EoGeVector3d xAxis = EoGeVector3d::positiveUnitX;
  EoGeVector3d yAxis = EoGeVector3d::positiveUnitY;

  if (polygonStyle == EoDb::PolygonStyle::Hatch && hatch.m_hatchPatternScaleOrSpacing > Eo::geometricTolerance) {
    const double scale = hatch.m_hatchPatternScaleOrSpacing;
    const double angle = hatch.m_hatchPatternAngle * Eo::Radian;  // DXF pattern angle is in degrees
    const double cosAngle = std::cos(angle);
    const double sinAngle = std::sin(angle);
    xAxis = EoGeVector3d{cosAngle * scale, sinAngle * scale, 0.0};
    yAxis = EoGeVector3d{-sinAngle * scale, cosAngle * scale, 0.0};
  }

  // Transform hatch origin from OCS to WCS when extrusion is non-default.
  // Pattern reference vectors (xAxis, yAxis) are NOT transformed — they define
  // the visual pattern orientation in the rendering plane. Transforming them
  // through OCS would mirror the pattern for negative-Z extrusion because the
  // OCS X-axis reverses direction (e.g., [0,0,-1] → OCS X = [-1,0,0]).
  // AutoCAD renders hatch patterns with the same visual orientation regardless
  // of extrusion direction; preserving untransformed vectors matches this behavior.
  if (needsOcsTransform) { hatchOrigin = ocsToWcs * hatchOrigin; }

  int loopIndex = 0;
  for (const auto* hatchLoop : hatch.HatchLoops()) {
    ++loopIndex;

    if (hatchLoop->m_entities.empty()) {
      ATLTRACE2(traceGeneral, 1, L"  Loop %d: empty entity list, skipping\n", loopIndex);
      continue;
    }

    // Determine per-loop polygon style: island boundaries become Hollow when the
    // outer hatch is solid-filled, creating the visual "hole" effect.
    const bool isIslandLoop =
        (hatchLoop->m_boundaryPathType & 0x01) == 0 && (hatchLoop->m_boundaryPathType & 0x10) == 0;
    EoDb::PolygonStyle loopPolygonStyle = polygonStyle;
    if (isIslandLoop && polygonStyle == EoDb::PolygonStyle::Solid) {
      loopPolygonStyle = EoDb::PolygonStyle::Hollow;
      ATLTRACE2(traceGeneral, 2, L"  Loop %d: island boundary (type 0x%X) — rendered as Hollow\n", loopIndex,
          hatchLoop->m_boundaryPathType);
    } else if (isIslandLoop) {
      ATLTRACE2(traceGeneral, 2, L"  Loop %d: island boundary (type 0x%X) — converted as independent polygon\n",
          loopIndex, hatchLoop->m_boundaryPathType);
    }

    EoGePoint3dArray boundaryPoints;

    if (hatchLoop->m_boundaryPathType & 2) {
      // ── Polyline boundary ────────────────────────────────
      const auto* polylineEntity = (hatchLoop->m_entities.front()->m_entityType == EoDxf::LWPOLYLINE)
          ? static_cast<const EoDxfLwPolyline*>(hatchLoop->m_entities.front().get())
          : nullptr;
      if (polylineEntity == nullptr || polylineEntity->m_vertices.empty()) {
        ATLTRACE2(traceGeneral, 1, L"  Loop %d: polyline boundary with no vertices, skipping\n", loopIndex);
        continue;
      }

      const auto& vertices = polylineEntity->m_vertices;
      const auto vertexCount = vertices.size();
      const double elevation = hatch.m_elevationPoint.z;

      // Check if any vertex has a non-zero bulge
      const bool hasAnyBulge = std::any_of(vertices.begin(), vertices.end(),
          [](const EoDxfPolylineVertex2d& vertex) noexcept { return Eo::IsGeometricallyNonZero(vertex.bulge); });

      if (hasAnyBulge) {
        // Tessellate bulge arcs into straight segments
        std::vector<EoGePoint3d> arcPoints;
        boundaryPoints.Add(EoGePoint3d{vertices[0].x, vertices[0].y, elevation});

        for (size_t i = 0; i < vertexCount - 1; ++i) {
          const EoGePoint3d startPt{vertices[i].x, vertices[i].y, elevation};
          const EoGePoint3d endPt{vertices[i + 1].x, vertices[i + 1].y, elevation};
          polyline::TessellateArcSegment(startPt, endPt, vertices[i].bulge, arcPoints);
          for (const auto& arcPoint : arcPoints) { boundaryPoints.Add(arcPoint); }
        }

        // Handle closing segment if polyline is closed
        if (polylineEntity->m_polylineFlag & 0x01) {
          const EoGePoint3d startPt{vertices[vertexCount - 1].x, vertices[vertexCount - 1].y, elevation};
          const EoGePoint3d endPt{vertices[0].x, vertices[0].y, elevation};
          const double closingBulge = vertices[vertexCount - 1].bulge;
          if (Eo::IsGeometricallyNonZero(closingBulge)) {
            polyline::TessellateArcSegment(startPt, endPt, closingBulge, arcPoints);
            // Exclude last point (it duplicates the first vertex)
            for (size_t j = 0; j + 1 < arcPoints.size(); ++j) { boundaryPoints.Add(arcPoints[j]); }
          }
        }
      } else {
        // No bulges — straight segments only
        for (size_t i = 0; i < vertexCount; ++i) {
          boundaryPoints.Add(EoGePoint3d{vertices[i].x, vertices[i].y, elevation});
        }
      }

      ATLTRACE2(traceGeneral, 2, L"  Loop %d: polyline boundary → %d tessellated vertices\n", loopIndex,
          static_cast<int>(boundaryPoints.GetSize()));
    } else {
      // ── Edge-type boundary ───────────────────────────────
      const double elevation = hatch.m_elevationPoint.z;

      for (const auto& edgeEntity : hatchLoop->m_entities) {
        switch (edgeEntity->m_entityType) {
          case EoDxf::LINE: {
            const auto* line = static_cast<const EoDxfLine*>(edgeEntity.get());
            // Add start point of first edge; subsequent edges share endpoints
            if (boundaryPoints.IsEmpty()) {
              boundaryPoints.Add(EoGePoint3d{line->m_startPoint.x, line->m_startPoint.y, elevation});
            }
            boundaryPoints.Add(EoGePoint3d{line->m_endPoint.x, line->m_endPoint.y, elevation});
            break;
          }
          case EoDxf::ARC: {
            const auto* arc = static_cast<const EoDxfArc*>(edgeEntity.get());
            const double centerX = arc->m_centerPoint.x;
            const double centerY = arc->m_centerPoint.y;
            const double radius = arc->m_radius;
            double startAngle = arc->m_startAngle;
            double endAngle = arc->m_endAngle;

            // Determine sweep direction from CCW flag
            double sweepAngle;
            if (arc->m_isCounterClockwise) {
              sweepAngle = endAngle - startAngle;
              if (sweepAngle <= 0.0) { sweepAngle += Eo::TwoPi; }
            } else {
              sweepAngle = endAngle - startAngle;
              if (sweepAngle >= 0.0) { sweepAngle -= Eo::TwoPi; }
            }

            // Adaptive tessellation
            const int numberOfSegments = std::max(Eo::arcTessellationMinimumSegments,
                static_cast<int>(
                    std::ceil(std::abs(sweepAngle) / Eo::TwoPi * Eo::arcTessellationSegmentsPerFullCircle)));

            // Add start point if this is the first edge
            if (boundaryPoints.IsEmpty()) {
              boundaryPoints.Add(EoGePoint3d{
                  centerX + radius * std::cos(startAngle), centerY + radius * std::sin(startAngle), elevation});
            }

            const double angleStep = sweepAngle / numberOfSegments;
            for (int seg = 1; seg <= numberOfSegments; ++seg) {
              const double angle = startAngle + angleStep * seg;
              boundaryPoints.Add(
                  EoGePoint3d{centerX + radius * std::cos(angle), centerY + radius * std::sin(angle), elevation});
            }

            ATLTRACE2(traceGeneral, 3, L"    Arc edge: center=(%.4f,%.4f) r=%.4f → %d segments\n", centerX, centerY,
                radius, numberOfSegments);
            break;
          }
          case EoDxf::ELLIPSE: {
            const auto* ellipse = static_cast<const EoDxfEllipse*>(edgeEntity.get());
            const double cx = ellipse->m_centerPoint.x;
            const double cy = ellipse->m_centerPoint.y;
            const double majorX = ellipse->m_endPointOfMajorAxis.x;
            const double majorY = ellipse->m_endPointOfMajorAxis.y;
            const double ratio = ellipse->m_ratio;
            double startParam = ellipse->m_startParam;
            double endParam = ellipse->m_endParam;

            // Minor axis perpendicular to major axis in 2D
            const double minorX = -majorY * ratio;
            const double minorY = majorX * ratio;

            // Determine sweep
            double sweepParam;
            if (ellipse->m_isCounterClockwise) {
              sweepParam = endParam - startParam;
              if (sweepParam <= 0.0) { sweepParam += Eo::TwoPi; }
            } else {
              sweepParam = endParam - startParam;
              if (sweepParam >= 0.0) { sweepParam -= Eo::TwoPi; }
            }

            const int numberOfSegments = std::max(Eo::arcTessellationMinimumSegments,
                static_cast<int>(
                    std::ceil(std::abs(sweepParam) / Eo::TwoPi * Eo::arcTessellationSegmentsPerFullCircle)));

            if (boundaryPoints.IsEmpty()) {
              const double cosStart = std::cos(startParam);
              const double sinStart = std::sin(startParam);
              boundaryPoints.Add(EoGePoint3d{
                  cx + majorX * cosStart + minorX * sinStart, cy + majorY * cosStart + minorY * sinStart, elevation});
            }

            const double paramStep = sweepParam / numberOfSegments;
            for (int seg = 1; seg <= numberOfSegments; ++seg) {
              const double param = startParam + paramStep * seg;
              const double cosParam = std::cos(param);
              const double sinParam = std::sin(param);
              boundaryPoints.Add(EoGePoint3d{
                  cx + majorX * cosParam + minorX * sinParam, cy + majorY * cosParam + minorY * sinParam, elevation});
            }

            ATLTRACE2(traceGeneral, 3, L"    Ellipse edge: center=(%.4f,%.4f) ratio=%.4f → %d segments\n", cx, cy,
                ratio, numberOfSegments);
            break;
          }
          case EoDxf::SPLINE:
            ATLTRACE2(traceGeneral, 1, L"    Spline edge in hatch boundary — skipped (not supported in PEG V1)\n");
            break;
          default:
            ATLTRACE2(traceGeneral, 1, L"    Unknown edge type %d in hatch boundary — skipped\n",
                static_cast<int>(edgeEntity->m_entityType));
            break;
        }
      }

      ATLTRACE2(traceGeneral, 2, L"  Loop %d: edge boundary → %d tessellated vertices\n", loopIndex,
          static_cast<int>(boundaryPoints.GetSize()));
    }

    // Need at least 3 points for a valid polygon
    if (boundaryPoints.GetSize() < 3) {
      ATLTRACE2(traceGeneral, 1, L"  Loop %d: insufficient vertices (%d), skipping\n", loopIndex,
          static_cast<int>(boundaryPoints.GetSize()));
      continue;
    }

    // Remove duplicate closing vertex if present (EoDbPolygon is implicitly closed)
    const auto lastIndex = boundaryPoints.GetSize() - 1;
    const auto& firstPt = boundaryPoints[0];
    const auto& lastPt = boundaryPoints[lastIndex];
    if (Eo::IsGeometricallyZero(firstPt.x - lastPt.x) && Eo::IsGeometricallyZero(firstPt.y - lastPt.y) &&
        Eo::IsGeometricallyZero(firstPt.z - lastPt.z)) {
      boundaryPoints.SetSize(lastIndex);
    }

    if (boundaryPoints.GetSize() < 3) {
      ATLTRACE2(traceGeneral, 1, L"  Loop %d: degenerate after dedup (%d vertices), skipping\n", loopIndex,
          static_cast<int>(boundaryPoints.GetSize()));
      continue;
    }

    // Transform boundary points from OCS to WCS when extrusion is non-default
    if (needsOcsTransform) {
      for (INT_PTR i = 0; i < boundaryPoints.GetSize(); ++i) { boundaryPoints[i] = ocsToWcs * boundaryPoints[i]; }
    }

    auto* polygon = new EoDbPolygon(static_cast<std::int16_t>(hatch.m_color), loopPolygonStyle, fillStyleIndex,
        hatchOrigin, xAxis, yAxis, boundaryPoints);

    // Passthrough: preserve DXF pattern definition lines and double flag for round-trip export
    polygon->SetHatchPatternDoubleFlag(hatch.m_hatchPatternDoubleFlag);
    polygon->SetPatternDefinitionLines(hatch.m_patternDefinitionLines);

    // Set layer and line type from the DXF entity for correct document placement
    polygon->SetBaseProperties(&hatch, document);

    AddToDocument(polygon, document, hatch.m_space);

    ATLTRACE2(traceGeneral, 2, L"  Loop %d: created EoDbPolygon (%s, %d vertices)\n", loopIndex,
        loopPolygonStyle == EoDb::PolygonStyle::Solid       ? L"Solid"
            : loopPolygonStyle == EoDb::PolygonStyle::Hatch ? L"Hatch"
                                                            : L"Hollow",
        static_cast<int>(boundaryPoints.GetSize()));
  }
}

EoDbBlockReference* EoDbDxfInterface::ConvertInsertEntity(const EoDxfInsert& blockReference, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Insert entity conversion\n");
  auto insertPrimitive = new EoDbBlockReference();
  insertPrimitive->SetBaseProperties(&blockReference, document);
  insertPrimitive->SetName(CString(blockReference.m_blockName.c_str()));
  insertPrimitive->SetInsertionPoint(blockReference.m_insertionPoint);
  insertPrimitive->SetNormal(EoGeVector3d(blockReference.m_extrusionDirection.x, blockReference.m_extrusionDirection.y,
      blockReference.m_extrusionDirection.z));
  insertPrimitive->SetScaleFactors(
      EoGeVector3d(blockReference.m_xScaleFactor, blockReference.m_yScaleFactor, blockReference.m_zScaleFactor));
  insertPrimitive->SetRotation(blockReference.m_rotationAngle);
  insertPrimitive->SetColumns(blockReference.m_columnCount);
  insertPrimitive->SetColumnSpacing(blockReference.m_columnSpacing);
  insertPrimitive->SetRows(blockReference.m_rowCount);
  insertPrimitive->SetRowSpacing(blockReference.m_rowSpacing);

  m_currentInsertGroup = AddToDocument(insertPrimitive, document, blockReference.m_space);
  return insertPrimitive;
}

void EoDbDxfInterface::ConvertLineEntity(const EoDxfLine& line, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Line entity conversion\n");

  auto linePrimitive = new EoDbLine();
  linePrimitive->SetBaseProperties(&line, document);

  linePrimitive->SetLine(EoGeLine(EoGePoint3d{line.m_startPoint}, EoGePoint3d{line.m_endPoint}));
  AddToDocument(linePrimitive, document, line.m_space);
}

void EoDbDxfInterface::ConvertLWPolylineEntity(const EoDxfLwPolyline& polyline, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"LWPolyline entity conversion\n");

  auto* polylinePrimitive = new EoDbPolyline();
  polylinePrimitive->SetBaseProperties(&polyline, document);

  const auto numVerts = static_cast<std::uint16_t>(polyline.m_vertices.size());
  polylinePrimitive->SetNumberOfVertices(numVerts);

  for (std::uint16_t index = 0; index < numVerts; ++index) {
    polylinePrimitive->SetVertexFromLwVertex(index, polyline.m_vertices[index]);
  }

  if (polyline.m_polylineFlag & 0x01) { polylinePrimitive->SetClosed(true); }
  if (polyline.m_polylineFlag & 0x80) { polylinePrimitive->SetPlinegen(true); }

  // Store constant width for round-trip fidelity
  polylinePrimitive->SetConstantWidth(polyline.m_constantWidth);

  // Populate per-vertex bulge values when any vertex has a non-zero bulge
  const bool hasAnyBulge = std::any_of(polyline.m_vertices.begin(), polyline.m_vertices.end(),
      [](const EoDxfPolylineVertex2d& vertex) noexcept { return vertex.bulge != 0.0; });
  if (hasAnyBulge) {
    std::vector<double> bulges(numVerts);
    for (std::uint16_t index = 0; index < numVerts; ++index) { bulges[index] = polyline.m_vertices[index].bulge; }
    polylinePrimitive->SetBulges(std::move(bulges));
  }

  // Populate per-vertex width values: use per-vertex widths if present, else expand constant width.
  // DXF convention: per-vertex width 0 means "use the constant width" when a constant width is set.
  // When any vertex has an explicit non-zero width, we still fill zero-width vertices with the
  // constant width as fallback (mixed usage).
  const bool hasAnyPerVertexWidth = std::any_of(polyline.m_vertices.begin(), polyline.m_vertices.end(),
      [](const EoDxfPolylineVertex2d& vertex) { return vertex.stawidth != 0.0 || vertex.endwidth != 0.0; });
  if (hasAnyPerVertexWidth) {
    std::vector<double> startWidths(numVerts);
    std::vector<double> endWidths(numVerts);
    for (std::uint16_t index = 0; index < numVerts; ++index) {
      const auto& vertex = polyline.m_vertices[index];
      startWidths[index] = (Eo::IsGeometricallyNonZero(vertex.stawidth)) ? vertex.stawidth : polyline.m_constantWidth;
      endWidths[index] = (Eo::IsGeometricallyNonZero(vertex.endwidth)) ? vertex.endwidth : polyline.m_constantWidth;
    }
    polylinePrimitive->SetWidths(std::move(startWidths), std::move(endWidths));
  } else if (Eo::IsGeometricallyNonZero(polyline.m_constantWidth)) {
    // Expand constant width into per-vertex start/end widths
    std::vector<double> startWidths(numVerts, polyline.m_constantWidth);
    std::vector<double> endWidths(numVerts, polyline.m_constantWidth);
    polylinePrimitive->SetWidths(std::move(startWidths), std::move(endWidths));
  }

  AddToDocument(polylinePrimitive, document, polyline.m_space);
}

void EoDbDxfInterface::ConvertPolyline3DEntity(const EoDxfPolyline& polyline, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Polyline 3D entity conversion (%zu vertices)\n", polyline.m_vertices.size());

  if (polyline.m_vertices.empty()) { return; }

  auto* polylinePrimitive = new EoDbPolyline();
  polylinePrimitive->SetBaseProperties(&polyline, document);

  const auto numVerts = static_cast<std::uint16_t>(polyline.m_vertices.size());
  polylinePrimitive->SetNumberOfVertices(numVerts);

  for (std::uint16_t index = 0; index < numVerts; ++index) {
    const auto& vertex = polyline.m_vertices[index];
    // 3D polyline vertices are truly 3D — no OCS/elevation mapping needed
    polylinePrimitive->SetVertex(
        index, EoGePoint3d{vertex->m_locationPoint.x, vertex->m_locationPoint.y, vertex->m_locationPoint.z});
  }

  if (polyline.m_polylineFlag & 0x01) { polylinePrimitive->SetClosed(true); }
  if (polyline.m_polylineFlag & 0x80) { polylinePrimitive->SetPlinegen(true); }
  polylinePrimitive->Set3D(true);

  AddToDocument(polylinePrimitive, document, polyline.m_space);
}

void EoDbDxfInterface::ConvertPolyline2DEntity(const EoDxfPolyline& polyline, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Polyline 2D entity conversion (%zu vertices)\n", polyline.m_vertices.size());

  if (polyline.m_vertices.empty()) { return; }

  // For curve-fit (0x02) and spline-fit (0x04) polylines, keep all vertices including generated
  // points. The curve-fit/spline-fit structure is lost, but the rendered geometry is preserved.

  auto* polylinePrimitive = new EoDbPolyline();
  polylinePrimitive->SetBaseProperties(&polyline, document);

  const auto numVerts = static_cast<std::uint16_t>(polyline.m_vertices.size());
  polylinePrimitive->SetNumberOfVertices(numVerts);

  // 2D POLYLINE: vertex x,y are in OCS; z is the polyline elevation
  const double elevation = polyline.m_polylineElevation.z;

  // Resolve extrusion direction for OCS → WCS transform
  EoGeVector3d extrusionDirection{
      polyline.m_extrusionDirection.x, polyline.m_extrusionDirection.y, polyline.m_extrusionDirection.z};
  if (extrusionDirection.IsNearNull()) {
    extrusionDirection = EoGeVector3d::positiveUnitZ;
  } else {
    extrusionDirection.Unitize();
  }
  const bool needsOcsTransform = Eo::IsGeometricallyNonZero(extrusionDirection.x) ||
      Eo::IsGeometricallyNonZero(extrusionDirection.y) || Eo::IsGeometricallyNonZero(extrusionDirection.z - 1.0);
  EoGeOcsTransform transformOcs{extrusionDirection};

  for (std::uint16_t index = 0; index < numVerts; ++index) {
    const auto& vertex = polyline.m_vertices[index];
    auto point = EoGePoint3d{vertex->m_locationPoint.x, vertex->m_locationPoint.y, elevation};
    if (needsOcsTransform) { point = transformOcs * point; }
    polylinePrimitive->SetVertex(index, point);
  }

  if (polyline.m_polylineFlag & 0x01) { polylinePrimitive->SetClosed(true); }
  if (polyline.m_polylineFlag & 0x80) { polylinePrimitive->SetPlinegen(true); }

  // Populate per-vertex bulge values when any vertex has a non-zero bulge
  const bool hasAnyBulge = std::any_of(polyline.m_vertices.begin(), polyline.m_vertices.end(),
      [](const EoDxfVertex* vertex) { return vertex->m_bulge != 0.0; });
  if (hasAnyBulge) {
    std::vector<double> bulges(numVerts);
    for (std::uint16_t index = 0; index < numVerts; ++index) { bulges[index] = polyline.m_vertices[index]->m_bulge; }
    polylinePrimitive->SetBulges(std::move(bulges));
  }

  // Populate per-vertex width values: use per-vertex widths if present, else expand default widths.
  // DXF convention: per-vertex width 0 means "use default width" when default widths are set.
  const bool hasAnyPerVertexWidth = std::any_of(polyline.m_vertices.begin(), polyline.m_vertices.end(),
      [](const EoDxfVertex* vertex) { return vertex->m_startingWidth != 0.0 || vertex->m_endingWidth != 0.0; });
  if (hasAnyPerVertexWidth) {
    std::vector<double> startWidths(numVerts);
    std::vector<double> endWidths(numVerts);
    for (std::uint16_t index = 0; index < numVerts; ++index) {
      const auto* vertex = polyline.m_vertices[index];
      startWidths[index] = (Eo::IsGeometricallyNonZero(vertex->m_startingWidth)) ? vertex->m_startingWidth
                                                                                 : polyline.m_defaultStartWidth;
      endWidths[index] =
          (Eo::IsGeometricallyNonZero(vertex->m_endingWidth)) ? vertex->m_endingWidth : polyline.m_defaultEndWidth;
    }
    polylinePrimitive->SetWidths(std::move(startWidths), std::move(endWidths));
  } else if (Eo::IsGeometricallyNonZero(polyline.m_defaultStartWidth) ||
      Eo::IsGeometricallyNonZero(polyline.m_defaultEndWidth)) {
    // Expand default widths into per-vertex start/end widths
    std::vector<double> startWidths(numVerts, polyline.m_defaultStartWidth);
    std::vector<double> endWidths(numVerts, polyline.m_defaultEndWidth);
    polylinePrimitive->SetWidths(std::move(startWidths), std::move(endWidths));
  }

  AddToDocument(polylinePrimitive, document, polyline.m_space);
}

void EoDbDxfInterface::ConvertMTextEntity(const EoDxfMText& mtext, [[maybe_unused]] AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"MText entity conversion\n");

  if (mtext.m_nominalTextHeight < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 1, L"MText entity skipped: zero or near-zero text height\n");
    return;
  }
  if (mtext.m_textString.empty()) {
    ATLTRACE2(traceGeneral, 1, L"MText entity skipped: empty text string\n");
    return;
  }

  auto textHeight = mtext.m_nominalTextHeight;  // Group code 40

  // MTEXT rotation (group code 50) is already in radians; UpdateAngle() has resolved xAxisDirection if present
  auto textRotation = mtext.m_rotationAngle;

  std::wstring textStyleName = mtext.m_textStyleName;  // Group code 7

  auto insertionPointInOcs = EoGePoint3d{mtext.m_insertionPoint.x, mtext.m_insertionPoint.y, mtext.m_insertionPoint.z};
  EoGeVector3d extrusionDirection{
      mtext.m_extrusionDirection.x, mtext.m_extrusionDirection.y, mtext.m_extrusionDirection.z};
  if (!mtext.m_haveExtrusion || extrusionDirection.IsNearNull()) {
    extrusionDirection = EoGeVector3d::positiveUnitZ;
  } else {
    extrusionDirection.Unitize();
  }

  // Transform insertion point OCS → WCS
  EoGeOcsTransform transformOcs{extrusionDirection};
  auto insertionPointInWcs = transformOcs * insertionPointInOcs;

  // Build baseline direction from rotation angle
  auto baselineDirection = EoGeVector3d::positiveUnitX;
  baselineDirection.RotateAboutArbitraryAxis(extrusionDirection, textRotation);

  auto xAxisDirection = baselineDirection;
  auto yAxisDirection = CrossProduct(extrusionDirection, xAxisDirection);

  yAxisDirection *= textHeight;
  xAxisDirection *= textHeight * Eo::defaultCharacterCellAspectRatio;

  // Map MTEXT AttachmentPoint (9-point grid) to AeSys horizontal and vertical alignment
  EoDb::HorizontalAlignment horizontalAlignment = EoDb::HorizontalAlignment::Left;
  EoDb::VerticalAlignment verticalAlignment = EoDb::VerticalAlignment::Top;

  switch (mtext.m_attachmentPoint) {
    case EoDxfMText::AttachmentPoint::TopLeft:
      horizontalAlignment = EoDb::HorizontalAlignment::Left;
      verticalAlignment = EoDb::VerticalAlignment::Top;
      break;
    case EoDxfMText::AttachmentPoint::TopCenter:
      horizontalAlignment = EoDb::HorizontalAlignment::Center;
      verticalAlignment = EoDb::VerticalAlignment::Top;
      break;
    case EoDxfMText::AttachmentPoint::TopRight:
      horizontalAlignment = EoDb::HorizontalAlignment::Right;
      verticalAlignment = EoDb::VerticalAlignment::Top;
      break;
    case EoDxfMText::AttachmentPoint::MiddleLeft:
      horizontalAlignment = EoDb::HorizontalAlignment::Left;
      verticalAlignment = EoDb::VerticalAlignment::Middle;
      break;
    case EoDxfMText::AttachmentPoint::MiddleCenter:
      horizontalAlignment = EoDb::HorizontalAlignment::Center;
      verticalAlignment = EoDb::VerticalAlignment::Middle;
      break;
    case EoDxfMText::AttachmentPoint::MiddleRight:
      horizontalAlignment = EoDb::HorizontalAlignment::Right;
      verticalAlignment = EoDb::VerticalAlignment::Middle;
      break;
    case EoDxfMText::AttachmentPoint::BottomLeft:
      horizontalAlignment = EoDb::HorizontalAlignment::Left;
      verticalAlignment = EoDb::VerticalAlignment::Bottom;
      break;
    case EoDxfMText::AttachmentPoint::BottomCenter:
      horizontalAlignment = EoDb::HorizontalAlignment::Center;
      verticalAlignment = EoDb::VerticalAlignment::Bottom;
      break;
    case EoDxfMText::AttachmentPoint::BottomRight:
      horizontalAlignment = EoDb::HorizontalAlignment::Right;
      verticalAlignment = EoDb::VerticalAlignment::Bottom;
      break;
    default:
      break;
  }

  /** @brief Strip only MTEXT formatting codes that AeSys cannot render; preserve codes the renderer handles.
   *
   * AeSys DisplayTextWithFormattingCharacters() handles at render time:
   *  - \P  → hard line break (paragraph)
   *  - \A  → alignment change (bottom/center/top)
   *  - \S  → stacked fractions (numerator/denominator with / or ^ delimiter)
   *
   * Everything else is stripped here at import time:
   *  - \fFontName|...; \HValue; \CValue; \TValue; \QValue; \WValue; → skip to semicolon
   *  - \L \l \O \o → underline/overline start/stop (dropped)
   *  - \~ → non-breaking space (converted to regular space)
   *  - \\ \{ \} → escaped literals (resolved to the literal character)
   *  - { } → brace grouping (braces stripped, content preserved)
   */
  const auto& rawText = mtext.m_textString;
  std::wstring cleanedText;
  cleanedText.reserve(rawText.size());

  for (std::size_t i = 0; i < rawText.size(); ++i) {
    const wchar_t currentCharacter = rawText[i];

    if (currentCharacter == L'\\' && i + 1 < rawText.size()) {
      const wchar_t nextCharacter = rawText[i + 1];
      switch (nextCharacter) {
        case L'P':
        case L'S':
          // Renderer handles \P and \S — pass through verbatim
          cleanedText += L'\\';
          cleanedText += nextCharacter;
          ++i;
          break;
        case L'A': {
          // Renderer handles \A — pass through the complete \Avalue; sequence
          auto semicolonPosition = rawText.find(L';', i + 2);
          if (semicolonPosition != std::wstring::npos) {
            cleanedText += rawText.substr(i, semicolonPosition - i + 1);
            i = semicolonPosition;
          } else {
            cleanedText += L'\\';
            cleanedText += nextCharacter;
            ++i;
          }
          break;
        }
        case L'\\':
        case L'{':
        case L'}':
          // Escaped literal characters — resolve to the literal
          cleanedText += nextCharacter;
          ++i;
          break;
        case L'~':
          // Non-breaking space → regular space
          cleanedText += L' ';
          ++i;
          break;
        case L'L':
        case L'l':
        case L'O':
        case L'o':
          // Underline/overline start/stop → strip
          ++i;
          break;
        case L'f':
        case L'F':
        case L'H':
        case L'C':
        case L'T':
        case L'Q':
        case L'W':
        case L'p': {
          // Unsupported formatting commands with value ending in semicolon → skip to semicolon
          auto semicolonPosition = rawText.find(L';', i + 2);
          if (semicolonPosition != std::wstring::npos) {
            i = semicolonPosition;
          } else {
            ++i;
          }
          break;
        }
        default:
          // Unknown escape → keep as-is
          cleanedText += currentCharacter;
          cleanedText += nextCharacter;
          ++i;
          break;
      }
    } else if (currentCharacter == L'{' || currentCharacter == L'}') {
      // Brace grouping → strip braces, content preserved
      continue;
    } else {
      cleanedText += currentCharacter;
    }
  }

  if (cleanedText.empty()) {
    ATLTRACE2(traceGeneral, 1, L"MText entity skipped: no text content after formatting strip\n");
    return;
  }

  EoDbFontDefinition fontDefinition{};
  fontDefinition.SetFontName(textStyleName);
  fontDefinition.SetHorizontalAlignment(horizontalAlignment);
  fontDefinition.SetVerticalAlignment(verticalAlignment);

  // Map MTEXT drawing direction (group 72) to AeSys font path
  if (mtext.m_drawingDirection == EoDxfMText::DrawingDirection::TopToBottom) {
    fontDefinition.SetPath(EoDb::Path::Down);
  }

  EoGeReferenceSystem referenceSystem(insertionPointInWcs, xAxisDirection, yAxisDirection);

  auto* textPrimitive = new EoDbText(fontDefinition, referenceSystem, cleanedText);
  textPrimitive->SetBaseProperties(&mtext, document);
  textPrimitive->SetExtrusion(extrusionDirection);

  // Preserve MTEXT-specific DXF properties for round-trip export
  EoDbMTextProperties mtextProperties;
  mtextProperties.attachmentPoint = static_cast<std::int16_t>(mtext.m_attachmentPoint);
  mtextProperties.drawingDirection = static_cast<std::int16_t>(mtext.m_drawingDirection);
  mtextProperties.lineSpacingStyle = static_cast<std::int16_t>(mtext.m_lineSpacingStyle);
  mtextProperties.lineSpacingFactor = mtext.m_lineSpacingFactor;
  mtextProperties.referenceRectangleWidth = mtext.m_referenceRectangleWidth;
  textPrimitive->SetMTextProperties(mtextProperties);

  AddToDocument(textPrimitive, document, mtext.m_space);
}

void EoDbDxfInterface::ConvertPointEntity(const EoDxfPoint& point, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 3, L"Point entity conversion\n");

  auto pointPrimitive = new EoDbPoint();
  pointPrimitive->SetBaseProperties(&point, document);
  pointPrimitive->SetPoint(point.m_pointLocation.x, point.m_pointLocation.y, point.m_pointLocation.z);
  AddToDocument(pointPrimitive, document, point.m_space);
}

/** @brief Converts a DXF SPLINE entity to an EoDbSpline primitive in the AeSys document.
 *
 *  DXF SPLINE entities (AutoCAD 13+) define B-splines with degree, knot vector, control points,
 *  and optional fit points and weight values. AeSys `EoDbSpline` stores only control points and
 *  uses `GenPts(3, m_pts)` to tessellate a uniform cubic B-spline at render time.
 *
 *  ## Mapping Strategy
 *  - **Control points present**: Use them directly. The control polygon defines the spline shape.
 *    `EoDbSpline::GenPts` applies its own uniform knot vector with order 3 (quadratic), so the
 *    curve is an approximation of the original DXF spline when the DXF degree differs from 2 or
 *    the knot vector is non-uniform. This is acceptable for display-quality rendering.
 *  - **Fit points only (no control points)**: Use fit points as control points. The resulting
 *    B-spline will approximate the fit points rather than interpolating them exactly.
 *  - **Closed splines** (flag bit 0x01): Not specially handled; the control point polygon is used
 *    as-is. AutoCAD wraps control points for closed splines, so the parsed points already encode
 *    closure.
 *
 *  ## Limitations
 *  - Rational splines (NURBS with non-unit weights) are rendered as non-rational approximations.
 *
 *  ## Coordinate System
 *  - DXF SPLINE control/fit points are in WCS (same as ELLIPSE). `ApplyExtrusion()` is a no-op.
 *  - No OCS → WCS transform is needed or applied.
 *
 *  @param spline The parsed DXF spline entity.
 *  @param document The AeSys document receiving the created primitive.
 */
void EoDbDxfInterface::ConvertSplineEntity(const EoDxfSpline& spline, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2,
      L"Spline entity conversion (degree=%d, controlPts=%d, fitPts=%d, knots=%d, flags=0x%04X)\n",
      spline.m_degreeOfTheSplineCurve, spline.m_numberOfControlPoints, spline.m_numberOfFitPoints,
      spline.m_numberOfKnots, spline.m_splineFlag);

  // Determine which point set to use: control points preferred, fit points as fallback
  const auto& controlPoints = spline.m_controlPoints;
  const auto& fitPoints = spline.m_fitPoints;

  const bool haveControlPoints = !controlPoints.empty();
  const bool haveFitPoints = !fitPoints.empty();

  if (!haveControlPoints && !haveFitPoints) {
    ATLTRACE2(traceGeneral, 1, L"Spline entity skipped: no control points and no fit points\n");
    return;
  }

  // Select point source
  const auto& sourcePoints = haveControlPoints ? controlPoints : fitPoints;
  const auto pointCount = static_cast<std::uint16_t>(sourcePoints.size());

  if (pointCount < 2) {
    ATLTRACE2(traceGeneral, 1, L"Spline entity skipped: fewer than 2 points (%d)\n", pointCount);
    return;
  }

  if (!haveControlPoints) {
    ATLTRACE2(
        traceGeneral, 2, L"  Spline: using %d fit points as control points (no control points defined)\n", pointCount);
  }

  // DXF SPLINE control/fit points are in WCS (same as ELLIPSE) — no OCS transform needed.
  EoGePoint3dArray points;
  points.SetSize(pointCount);

  for (std::uint16_t i = 0; i < pointCount; ++i) {
    const auto* sourcePoint = sourcePoints[i];
    points[i] = EoGePoint3d{sourcePoint->x, sourcePoint->y, sourcePoint->z};
  }

  auto* splinePrimitive = new EoDbSpline(points);
  splinePrimitive->SetBaseProperties(&spline, document);

  // Preserve DXF spline properties for faithful round-trip export and correct-degree rendering.
  splinePrimitive->SetDegree(spline.m_degreeOfTheSplineCurve > 0 ? spline.m_degreeOfTheSplineCurve
                                                                  : static_cast<std::int16_t>(3));
  splinePrimitive->SetFlags(spline.m_splineFlag);

  // Import knot vector when control points are used (knots are meaningless with fit-point fallback).
  if (haveControlPoints && !spline.m_knotValues.empty()) {
    splinePrimitive->SetKnots(std::vector<double>(spline.m_knotValues.begin(), spline.m_knotValues.end()));
  }

  // Import weights for rational splines (NURBS).
  if (haveControlPoints && spline.IsRational() && !spline.m_weightValues.empty()) {
    splinePrimitive->SetWeights(std::vector<double>(spline.m_weightValues.begin(), spline.m_weightValues.end()));
  }

  AddToDocument(splinePrimitive, document, spline.m_space);

  ATLTRACE2(traceGeneral, 2, L"  Spline → EoDbSpline with %d control points (degree=%d, flags=0x%04X, knots=%zu)\n",
      pointCount, splinePrimitive->Degree(), splinePrimitive->Flags(), splinePrimitive->Knots().size());
}

void EoDbDxfInterface::ConvertTextEntity(const EoDxfText& text, [[maybe_unused]] AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Text entity conversion\n");

  // Guard: skip degenerate entities with zero height or empty string
  if (text.m_textHeight < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 1, L"Text entity skipped: zero or near-zero text height\n");
    return;
  }
  if (text.m_string.empty()) {
    ATLTRACE2(traceGeneral, 1, L"Text entity skipped: empty text string\n");
    return;
  }

  [[maybe_unused]] auto thickness = text.m_thickness;  // Group code 39 (not supported in AeSys)
  auto firstAlignmentPointInOcs =
      EoGePoint3d{text.m_firstAlignmentPoint.x, text.m_firstAlignmentPoint.y, text.m_firstAlignmentPoint.z};
  auto textHeight = text.m_textHeight;  // Group code 40

  std::wstring string{text.m_string};  // Group code 1

  auto textRotation = Eo::DegreeToRadian(text.m_textRotation);  // Group code 50 (degrees → radians)
  auto xScaleFactorWidth = text.m_scaleFactorWidth;  // Group code 41
  auto obliqueAngle = Eo::DegreeToRadian(text.m_obliqueAngle);  // Group code 51 (degrees → radians)

  std::wstring textStyleName = text.m_textStyleName;  // Group code 7

  auto textGenerationFlags = text.m_textGenerationFlags;  // Group code 71 (2=backward, 4=upside-down)
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

  const bool hasSecondAlignmentPoint = text.HasSecondAlignmentPoint();

  EoGePoint3d firstAlignmentPointInWcs;
  EoGePoint3d secondAlignmentPointInWcs;

  // Always transform points to WCS (simplifies branches)
  EoGeOcsTransform transformOcs{extrusionDirection};
  firstAlignmentPointInWcs = transformOcs * firstAlignmentPointInOcs;
  secondAlignmentPointInWcs = transformOcs * secondAlignmentPointInOcs;

  // Determine if this is the default alignment (Left + Baseline)
  const bool isDefaultAlignment = (horizontalAlignment == EoDxfText::HorizontalAlignment::Left &&
      verticalAlignment == EoDxfText::VerticalAlignment::BaseLine);
  const bool isAlignedOrFit = (horizontalAlignment == EoDxfText::HorizontalAlignment::AlignedIfBaseLine ||
      horizontalAlignment == EoDxfText::HorizontalAlignment::FitIfBaseLine);

  // Compute baseline direction – respect DXF rules for Aligned/Fit
  auto baselineDirection = EoGeVector3d::positiveUnitX;

  if (hasSecondAlignmentPoint && isAlignedOrFit) {
    // Spec: for Aligned/Fit, ignore textRotation; baseline direction is defined by both points
    auto alignedDirection = secondAlignmentPointInWcs - firstAlignmentPointInWcs;
    if (!alignedDirection.IsNearNull()) {
      baselineDirection = alignedDirection;
      baselineDirection.Unitize();
    }
  } else {
    // Normal case: rotation defines direction
    baselineDirection.RotateAboutArbitraryAxis(extrusionDirection, textRotation);
  }

  /** DXF origin selection rules:
   *  - Default alignment (Left + Baseline): first alignment point is the insertion point
   *  - Aligned/Fit: first alignment point is the baseline start
   *  - All other non-default alignments: second alignment point is the reference point
   */
  EoGePoint3d referenceOrigin;
  if (isDefaultAlignment || isAlignedOrFit) {
    referenceOrigin = firstAlignmentPointInWcs;
  } else if (hasSecondAlignmentPoint) {
    referenceOrigin = secondAlignmentPointInWcs;
  } else {
    // Fallback: if second point was not parsed, use first (malformed DXF data)
    ATLTRACE2(traceGeneral, 1, L"Text entity: non-default alignment but no second alignment point; using first\n");
    referenceOrigin = firstAlignmentPointInWcs;
  }

  auto xAxisDirection = baselineDirection;
  auto yAxisDirection = CrossProduct(extrusionDirection, xAxisDirection);

  // Apply oblique angle: shear the Y-axis by rotating it toward the baseline
  if (Eo::IsGeometricallyNonZero(obliqueAngle)) {
    yAxisDirection.RotateAboutArbitraryAxis(extrusionDirection, -obliqueAngle);
  }

  yAxisDirection *= textHeight;
  xAxisDirection *= xScaleFactorWidth * textHeight * Eo::defaultCharacterCellAspectRatio;
  EoGeReferenceSystem referenceSystem(referenceOrigin, xAxisDirection, yAxisDirection);

  EoDbFontDefinition fontDefinition{};
  fontDefinition.SetFontName(textStyleName);

  /** Only Left(0), Center(1) and Right(2) horizontal alignment options are supported in AeSys so options
   * Aligned(3) → Left, Middle(4) → Center, and Fit(5) → Right; (should only pair with Baseline(0) vertical alignment
   * and when the second alignment point is defined).
   * @todo AeSys does not support the stretching behavior of Aligned/Fit options.
   */

  /** DXF horizontal alignment 4 ("Middle") is a composite alignment: horizontally centered AND
   *  vertically at the middle of the text height. It is only valid paired with vertical alignment 0
   *  (Baseline). Override the vertical to Middle when this special case is detected.
   */
  const bool isMiddleComposite = (horizontalAlignment == EoDxfText::HorizontalAlignment::MiddleIfBaseLine &&
      verticalAlignment == EoDxfText::VerticalAlignment::BaseLine);

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

  if (isMiddleComposite) {
    fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
  } else {
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
  }

  auto* textPrimitive = new EoDbText(fontDefinition, referenceSystem, string);
  textPrimitive->SetBaseProperties(&text, document);
  textPrimitive->SetTextGenerationFlags(textGenerationFlags);
  textPrimitive->SetExtrusion(extrusionDirection);

  AddToDocument(textPrimitive, document, text.m_space);
}

void EoDbDxfInterface::ConvertAttDefEntity(const EoDxfAttDef& attdef, [[maybe_unused]] AeSysDoc* document) {
  // ATTDEFs define attribute templates inside block definitions. In AutoCAD they are NOT rendered
  // in entity references — only ATTRIBs following INSERT entities are rendered. Skipping conversion
  // to EoDbPrimitive prevents the default value from overlapping with the actual ATTRIB text at the
  // same position.
  //
  // When inside a block definition, store the parsed EoDxfAttDef directly in the block so that
  // the full entity property set (handle, owner, layer, color, linetype, etc.) is preserved for
  // DXF round-trip export and future interactive attribute prompting.
  if (m_currentOpenBlockDefinition != nullptr) {
    m_currentOpenBlockDefinition->AddAttributeDefinition(attdef);
    ATLTRACE2(traceGeneral, 2, L"AttDef stored in block (tag='%s', default='%s', prompt='%s')\n",
        attdef.m_tagString.c_str(), attdef.m_defaultValue.c_str(), attdef.m_promptString.c_str());
  } else {
    ATLTRACE2(traceGeneral, 2, L"AttDef entity skipped — not inside block (tag='%s', default='%s')\n",
        attdef.m_tagString.c_str(), attdef.m_defaultValue.c_str());
  }
}

EoDbAttrib* EoDbDxfInterface::ConvertAttribEntity(const EoDxfAttrib& attrib, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Attrib entity conversion (tag='%s', value='%s')\n", attrib.m_tagString.c_str(),
      attrib.m_attributeValue.c_str());

  // Skip invisible attributes (flag bit 0)
  if (attrib.m_attributeFlags & 1) {
    ATLTRACE2(traceGeneral, 2, L"Attrib entity skipped: invisible flag set\n");
    return nullptr;
  }

  if (attrib.m_textHeight < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 1, L"Attrib entity skipped: zero or near-zero text height\n");
    return nullptr;
  }
  if (attrib.m_attributeValue.empty()) {
    ATLTRACE2(traceGeneral, 1, L"Attrib entity skipped: empty attribute value\n");
    return nullptr;
  }

  auto firstAlignmentPointInOcs =
      EoGePoint3d{attrib.m_firstAlignmentPoint.x, attrib.m_firstAlignmentPoint.y, attrib.m_firstAlignmentPoint.z};
  auto textHeight = attrib.m_textHeight;
  std::wstring string{attrib.m_attributeValue};
  auto textRotation = Eo::DegreeToRadian(attrib.m_textRotation);
  auto xScaleFactorWidth = attrib.m_relativeXScaleFactor;
  auto obliqueAngle = Eo::DegreeToRadian(attrib.m_obliqueAngle);
  std::wstring textStyleName = attrib.m_textStyleName;

  auto horizontalAlignment = attrib.m_horizontalTextJustification;
  auto verticalAlignment = attrib.m_verticalTextJustification;

  ATLTRACE2(traceGeneral, 2, L"  Attrib alignment: h=%d, v=%d, hasSecondPt=%d\n", horizontalAlignment,
      verticalAlignment, attrib.HasSecondAlignmentPoint() ? 1 : 0);

  auto secondAlignmentPointInOcs =
      EoGePoint3d{attrib.m_secondAlignmentPoint.x, attrib.m_secondAlignmentPoint.y, attrib.m_secondAlignmentPoint.z};
  EoGeVector3d extrusionDirection{
      attrib.m_extrusionDirection.x, attrib.m_extrusionDirection.y, attrib.m_extrusionDirection.z};
  if (!attrib.m_haveExtrusion || extrusionDirection.IsNearNull()) {
    extrusionDirection = EoGeVector3d::positiveUnitZ;
  } else {
    extrusionDirection.Unitize();
  }

  const bool hasSecondAlignmentPoint = attrib.HasSecondAlignmentPoint();

  EoGeOcsTransform transformOcs{extrusionDirection};
  auto firstAlignmentPointInWcs = transformOcs * firstAlignmentPointInOcs;
  auto secondAlignmentPointInWcs = transformOcs * secondAlignmentPointInOcs;

  const bool isDefaultAlignment = (horizontalAlignment == 0 && verticalAlignment == 0);
  const bool isAlignedOrFit = (horizontalAlignment == 3 || horizontalAlignment == 5);

  auto baselineDirection = EoGeVector3d::positiveUnitX;

  if (hasSecondAlignmentPoint && isAlignedOrFit) {
    auto alignedDirection = secondAlignmentPointInWcs - firstAlignmentPointInWcs;
    if (!alignedDirection.IsNearNull()) {
      baselineDirection = alignedDirection;
      baselineDirection.Unitize();
    }
  } else {
    baselineDirection.RotateAboutArbitraryAxis(extrusionDirection, textRotation);
  }

  EoGePoint3d referenceOrigin;
  if (isDefaultAlignment || isAlignedOrFit) {
    referenceOrigin = firstAlignmentPointInWcs;
  } else if (hasSecondAlignmentPoint) {
    referenceOrigin = secondAlignmentPointInWcs;
  } else {
    ATLTRACE2(traceGeneral, 1, L"Attrib entity: non-default alignment but no second alignment point; using first\n");
    referenceOrigin = firstAlignmentPointInWcs;
  }

  auto xAxisDirection = baselineDirection;
  auto yAxisDirection = CrossProduct(extrusionDirection, xAxisDirection);

  if (Eo::IsGeometricallyNonZero(obliqueAngle)) {
    yAxisDirection.RotateAboutArbitraryAxis(extrusionDirection, -obliqueAngle);
  }

  yAxisDirection *= textHeight;
  xAxisDirection *= xScaleFactorWidth * textHeight * Eo::defaultCharacterCellAspectRatio;
  EoGeReferenceSystem referenceSystem(referenceOrigin, xAxisDirection, yAxisDirection);

  EoDbFontDefinition fontDefinition{};
  fontDefinition.SetFontName(textStyleName);

  /** DXF horizontal alignment 4 ("Middle") is a composite alignment: horizontally centered AND
   *  vertically at the middle of the text height. It is only valid paired with vertical alignment 0
   *  (Baseline). Override the vertical to Middle when this special case is detected.
   */
  const bool isMiddleComposite = (horizontalAlignment == 4 && verticalAlignment == 0);

  switch (horizontalAlignment) {
    case 1:  // Center
    case 4:  // Middle (paired with Baseline)
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Center);
      break;
    case 2:  // Right
    case 5:  // Fit (paired with Baseline)
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Right);
      break;
    case 0:  // Left
    case 3:  // Aligned (paired with Baseline)
    default:
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Left);
      break;
  }

  if (isMiddleComposite) {
    fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
  } else {
    switch (verticalAlignment) {
      case 2:  // Middle
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
        break;
      case 3:  // Top
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Top);
        break;
      case 0:  // Baseline
      case 1:  // Bottom
      default:
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Bottom);
        break;
    }
  }

  auto* attribPrimitive = new EoDbAttrib(fontDefinition, referenceSystem, string, attrib.m_tagString, attrib.m_attributeFlags);
  attribPrimitive->SetBaseProperties(&attrib, document);
  attribPrimitive->SetTextGenerationFlags(attrib.m_textGenerationFlags);
  attribPrimitive->SetExtrusion(extrusionDirection);

  return attribPrimitive;
}

void EoDbDxfInterface::ConvertViewportEntity(const EoDxfViewport& viewport, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Viewport entity conversion (id=%d, status=%d)\n", viewport.m_viewportId,
      viewport.m_viewportStatus);

  auto* viewportPrimitive = new EoDbViewport();
  viewportPrimitive->SetBaseProperties(&viewport, document);

  // Paper-space geometry
  viewportPrimitive->SetCenterPoint(
      EoGePoint3d{viewport.m_centerPoint.x, viewport.m_centerPoint.y, viewport.m_centerPoint.z});
  viewportPrimitive->SetWidth(viewport.m_width);
  viewportPrimitive->SetHeight(viewport.m_height);

  // Viewport identity
  viewportPrimitive->SetViewportStatus(viewport.m_viewportStatus);
  viewportPrimitive->SetViewportId(viewport.m_viewportId);

  // Model-space view parameters (round-trip preservation)
  viewportPrimitive->SetViewCenter(EoGePoint3d{viewport.m_viewCenter.x, viewport.m_viewCenter.y, 0.0});
  viewportPrimitive->SetSnapBasePoint(EoGePoint3d{viewport.m_snapBasePoint.x, viewport.m_snapBasePoint.y, 0.0});
  viewportPrimitive->SetSnapSpacing(EoGePoint3d{viewport.m_snapSpacing.x, viewport.m_snapSpacing.y, 0.0});
  viewportPrimitive->SetGridSpacing(EoGePoint3d{viewport.m_gridSpacing.x, viewport.m_gridSpacing.y, 0.0});
  viewportPrimitive->SetViewDirection(
      EoGePoint3d{viewport.m_viewDirection.x, viewport.m_viewDirection.y, viewport.m_viewDirection.z});
  viewportPrimitive->SetViewTargetPoint(
      EoGePoint3d{viewport.m_viewTargetPoint.x, viewport.m_viewTargetPoint.y, viewport.m_viewTargetPoint.z});
  viewportPrimitive->SetLensLength(viewport.m_lensLength);
  viewportPrimitive->SetFrontClipPlane(viewport.m_frontClipPlane);
  viewportPrimitive->SetBackClipPlane(viewport.m_backClipPlane);
  viewportPrimitive->SetViewHeight(viewport.m_viewHeight);
  viewportPrimitive->SetSnapAngle(viewport.m_snapAngle);
  viewportPrimitive->SetTwistAngle(viewport.m_twistAngle);

  AddToDocument(viewportPrimitive, document, viewport.m_space);

  ATLTRACE2(traceGeneral, 2, L"  Viewport id=%d → EoDbViewport (%.1f x %.1f at (%.4f, %.4f))\n", viewport.m_viewportId,
      viewport.m_width, viewport.m_height, viewport.m_centerPoint.x, viewport.m_centerPoint.y);
}