#include "Stdafx.h"

#include <cstdint>
#include <limits>
#include <string>
#include <variant>
#include <vector>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbBlock.h"
#include "EoDbDxfInterface.h"
#include "EoDbGroup.h"
#include "EoDbHeaderSection.h"
#include "EoDbLayer.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"
#include "EoDxfEntities.h"
#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfHeader.h"
#include "EoGePoint3d.h"
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
  const auto isFrozen = (layer.m_flagValues & 0x01) == 0x01;
  const auto isLocked = (layer.m_flagValues & 0x04) == 0x04;

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
    const auto numberOfElements =
        static_cast<std::uint16_t>(linetype.m_numberOfLinetypeElements);  // Number of linetype elements (group code 73)
    // double patternLength = linetype.length;                        // group code 40

    std::vector<double> dashLengths(numberOfElements);

    for (std::uint16_t index = 0; index < numberOfElements; index++) {
      dashLengths[index] = linetype.path[index];  // group code 49
    }
    const CString name(lineTypeName.c_str());
    const CString desc(lineTypeDesc.c_str());
    const auto lineTypeIndex = lineTypeTable->LegacyLineTypeIndex(name);

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

/** @brief Adds the given primitive to the appropriate layer in the document.
 *
 * For paper-space entities, the ownerHandle determines which layout's layer collection
 * receives the entity. If the target layer does not exist in the layout, it is created
 * on-demand by cloning properties from the default paper-space layout or model space.
 * As a last resort, a minimal layer with default properties is created so that no entity
 * is silently discarded.
 *
 * @param primitive Pointer to the EoDbPrimitive to be added.
 * @param document Pointer to the AeSysDoc where the primitive will be added.
 * @param space The DXF space (model or paper) in which the entity resides.
 * @param ownerHandle The entity's owner BLOCK_RECORD handle (code 330). For paper-space
 *   entities this identifies the layout. NoHandle falls back to the default layout (0x1E).
 */
EoDbGroup* EoDbDxfInterface::AddToDocument(EoDbPrimitive* primitive, AeSysDoc* document, EoDxf::Space space, std::uint64_t ownerHandle) const {
  // Override space for entities inside *Paper_Space layout pseudo-blocks.
  // Many DXF writers (including ODA Converter) don't set group code 67 for entities
  // inside *Paper_Space blocks — the block context already implies paper space.
  // Without this override, m_space defaults to ModelSpace and the entity is routed
  // to model-space layers, causing paper-space graphics to appear in both views.
  if (m_currentOpenBlockDefinition == nullptr && m_blockName.starts_with(L"*Paper_Space")) {
    space = EoDxf::Space::PaperSpace;
  }

  const auto layerName = primitive->LayerName().c_str();

  EoDbLayer* layer{};
  if (space == EoDxf::Space::PaperSpace) {
    // Determine the layout handle from the entity's owner handle
    const auto layoutHandle = (ownerHandle != EoDxf::NoHandle) ? ownerHandle : EoDxf::Handles::PaperSpaceBlockRecord;

    layer = document->FindLayerInLayout(layerName, layoutHandle);
    if (layer == nullptr) {
      // On-demand layer creation: clone properties from default layout or model space
      EoDbLayer* sourceLayer = document->FindLayerInLayout(layerName, EoDxf::Handles::PaperSpaceBlockRecord);
      if (sourceLayer == nullptr) {
        sourceLayer = document->FindLayerInSpace(layerName, EoDxf::Space::ModelSpace);
      }
      auto* newLayer = [&]() -> EoDbLayer* {
        if (sourceLayer != nullptr) {
          auto* cloned = new EoDbLayer(CString(layerName), sourceLayer->GetState());
          cloned->SetTracingState(sourceLayer->GetTracingState());
          cloned->SetColorIndex(sourceLayer->ColorIndex());
          cloned->SetLineType(sourceLayer->LineType());
          cloned->SetLineWeight(sourceLayer->LineWeight());
          cloned->SetLineTypeScale(sourceLayer->LineTypeScale());
          cloned->SetFrozen(sourceLayer->IsFrozen());
          cloned->SetLocked(sourceLayer->IsLocked());
          cloned->SetPlottingFlag(sourceLayer->PlottingFlag());
          cloned->SetColor24(sourceLayer->Color24());
          return cloned;
        }
        // No source layer found anywhere — create a minimal layer with default properties
        ATLTRACE2(traceGeneral, 3, L"AddToDocument: creating minimal paper-space layer '%s' for layout 0x%I64X\n",
            layerName, layoutHandle);
        constexpr auto defaultState =
            EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive;
        return new EoDbLayer(CString(layerName), defaultState);
      }();
      newLayer->SetHandle(document->HandleManager().AssignHandle());
      document->AddLayerToLayout(newLayer, layoutHandle);
      layer = newLayer;
    }
  } else {
    layer = document->FindLayerInSpace(layerName, space);
    if (layer == nullptr) {
      // Create a minimal model-space layer on demand so the entity is not discarded
      ATLTRACE2(traceGeneral, 3, L"AddToDocument: creating minimal model-space layer '%s'\n", layerName);
      constexpr auto defaultState =
          EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive;
      auto* newLayer = new EoDbLayer(CString(layerName), defaultState);
      newLayer->SetHandle(document->HandleManager().AssignHandle());
      document->AddLayerToSpace(newLayer, space);
      layer = newLayer;
    }
  }

  ATLTRACE2(traceGeneral, 3, L"AddToDocument: primitive=%p, inBlock=%d, currentBlock=%p, layer='%s'\n", primitive,
      m_inBlockDefinition ? 1 : 0, m_currentOpenBlockDefinition, layerName);

  document->RegisterHandle(primitive);

  if (m_currentOpenBlockDefinition == nullptr) {
    auto* group = new EoDbGroup();

    ATLTRACE2(traceGeneral, 3, L"  -> Creating %s group %p for primitive %p\n",
        (space == EoDxf::Space::PaperSpace) ? L"PAPER SPACE" : L"MODEL SPACE", group, primitive);

    group->AddTail(primitive);
    layer->AddTail(group);
    return group;
  }
  ATLTRACE2(traceGeneral, 3, L"  -> Adding to BLOCK at %p\n", m_currentOpenBlockDefinition);

  m_currentOpenBlockDefinition->AddTail(primitive);
  return nullptr;
}
