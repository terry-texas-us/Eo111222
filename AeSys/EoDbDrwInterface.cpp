#include "stdafx.h"
#include <afxstr.h>
#include <atltrace.h>
#include <codecvt>
#include <cstdlib>
#include <locale>
#include <string>
#include <variant>
#include <vector>

#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDbDrwInterface.h"
#include "EoDbHeaderSection.h"
#include "EoDbLayer.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "drw_base.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"

/** Convert a std::wstring to a UTF-8 encoded std::string.
 *
 * @param wstr The std::wstring to convert.
 * @return The converted UTF-8 encoded std::string.
 */
std::string EoDbDrwInterface::WStringToString(const std::wstring& wstr) {
  if (wstr.empty()) { return std::string(); }
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  return conv.to_bytes(wstr);
}

/** Convert a UTF-8 encoded std::string to a std::wstring.
 *
 * @param str The UTF-8 encoded std::string to convert.
 * @return The converted std::wstring.
 */
std::wstring EoDbDrwInterface::StringToWString(const std::string& str) {
  if (str.empty()) { return std::wstring(); }
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  return conv.from_bytes(str);
}

void EoDbDrwInterface::SetHeaderSectionVariable(const DRW_Header* header, const std::string& keyToFind, EoDbHeaderSection& headerSection) {
  HeaderVariable value;
  auto it = header->vars.find(keyToFind);
  if (it != header->vars.end() && it->second != nullptr) {
    std::wstring key = StringToWString(it->first);
    auto& second = *(it->second);
    switch (second.type()) {
      case DRW_Variant::STRING:
        value = StringToWString(*second.content.s);
        break;
      case DRW_Variant::INTEGER:
        value = second.content.i;
        break;
      case DRW_Variant::DOUBLE:
        value = second.content.d;
        break;
      case DRW_Variant::COORD:
        // TODO: Does a COORD variant ever populate a EoGeVector3d
        value = EoGePoint3d(second.content.v->x, second.content.v->y, second.content.v->z);
        break;
      case DRW_Variant::INVALID:
      default:
        value = L"";
        break;
    }
    headerSection.SetVariable(key, HeaderVariable(value));
  }
}

void EoDbDrwInterface::ConvertHeaderSection(const DRW_Header* header, AeSysDoc* document) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"Converting Header: %i variables\n", header->vars.size());

  std::vector<std::string> keys{"$ACADVER", "$CLAYER", "$PDMODE", "$PDSIZE"};

  EoDbHeaderSection& headerSection = document->HeaderSection();
  for (const auto& key : keys) { SetHeaderSectionVariable(header, key, headerSection); }
}

void EoDbDrwInterface::ConvertAppIdTable(const DRW_AppId& appId, AeSysDoc* /* document */) {
  std::wstring appIdName = StringToWString(appId.name.c_str());
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"AppId - Name: %s (unsupported in AeSys)\n", appIdName.c_str());
}

void EoDbDrwInterface::ConvertDimStyle(const DRW_Dimstyle& dimStyle, AeSysDoc* /* document */) {
  std::wstring dimStyleName = StringToWString(dimStyle.name.c_str());
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DimStyle - Name: <%s> (unsupported in AeSys)\n", dimStyleName.c_str());
}

void EoDbDrwInterface::ConvertLayerTable(const DRW_Layer& layer, AeSysDoc* document) {
  std::wstring layerName = StringToWString(layer.name.c_str());

  ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"%s   Loading layer definition\n", layerName.c_str());

  if (document->FindLayerTableLayer(layerName.c_str()) >= 0) { return; }

  EoDbLayer* newLayer = new EoDbLayer(layerName.c_str(), EoDbLayer::kIsResident | EoDbLayer::kIsInternal | EoDbLayer::kIsActive);

  // Color number (if negative the layer is off) group code 62
  newLayer->SetColorIndex(static_cast<EoInt16>(abs(layer.color)));
  if (layer.color < 0) { newLayer->SetStateOff(); }

  // Linetype name group code 6
  std::wstring lineTypeName = StringToWString(layer.lineType.c_str());
  EoDbLineType* lineType;
  document->LineTypeTable()->Lookup(lineTypeName.c_str(), lineType);
  newLayer->SetLineType(lineType);

  /**
  Standard flags (bit-coded values) group code 70:
    1 = Layer is frozen; otherwise layer is thawed
    2 = Layer is frozen by default in new viewports
    4 = Layer is locked
    16 = If set, table entry is externally dependent on an xref
    32 = If both this bit and bit 16 are set, the externally dependent xref has been successfully resolved
    64 = If set, the table entry was referenced by at least one entity in the drawing the last time the drawing was edited. (This flag is for the benefit of AutoCAD commands. It can be ignored by most programs that read DXF files and need not be set by programs that write DXF files)
   */
  auto isFrozen = (layer.flags & 0x01) == 0x01;
  auto isLocked = (layer.flags & 0x04) == 0x04;

  if (isFrozen) { newLayer->SetStateOff(); }
  document->AddLayerTableLayer(newLayer);

  /** 
  Lineweight enum value (not supported directly in AeSys)  group code 370
    This enumerated type provides the line weight (thickness between 0 and 211) values used to specify how lines will be displayed and plotted.
    The lineweights are in 100ths of a millimeter, except for the negative values.
    The negative values denote the default indicated by their constant's name.
  */
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"Line weight: %i\n", layer.lWeight);
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"Layer is locked: %i\n", isLocked);
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"Layer is plottable: %i\n", layer.plotF);

  // Hard-pointer to ID/handle of PlotStyleName object (not supported in AeSys) group code 390
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"Plot style name objects not supported\n");

  // Hard-pointer to ID/handle of Material object (not supported in AeSys) group code 347
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"Material objects not supported\n");

  // It is possible to have an Extension Dictionary associated with layer. (not supported in AeSys) group code 102
  // Not used very often. The most common application is for per-viewport overrides of layer properties.
  // Each override type is stored under a distinct key in the dictionary, referencing an XRECORD that contains subdata for affected viewports.
}

void EoDbDrwInterface::ConvertLinetypesTable(const DRW_LType& data, AeSysDoc* document) {
  std::wstring lineTypeName = StringToWString(data.name.c_str());  // Linetype name (group code 2)
  std::wstring lineTypeDesc = StringToWString(data.desc.c_str());  // Descriptive text for linetype (group code 3)

  ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"Converting Linetype: %s\n", lineTypeName.c_str());

  EoDbLineTypeTable* lineTypeTable = document->LineTypeTable();
  EoDbLineType* LineType{nullptr};

  if (!lineTypeTable->Lookup(lineTypeName.c_str(), LineType)) {
    auto numberOfElements = static_cast<EoUInt16>(data.size);  // Number of linetype elements (group code 73)
    // double patternLength = data.length;                        // group code 40

    std::vector<double> dashLengths(numberOfElements);

    for (EoUInt16 index = 0; index < numberOfElements; index++) {
      dashLengths[index] = data.path[index];  // group code 49
    }
    CString name(lineTypeName.c_str());
    CString desc(lineTypeDesc.c_str());
    EoUInt16 lineTypeIndex = lineTypeTable->LegacyLineTypeIndex(name);

    LineType = new EoDbLineType(lineTypeIndex, name, desc, numberOfElements, dashLengths.data());
    lineTypeTable->SetAt(name, LineType);
  }
}

void EoDbDrwInterface::ConvertTextStyleTable(const DRW_Textstyle& textStyle, AeSysDoc* /* document */) {
  std::wstring textStyleName = StringToWString(textStyle.name.c_str());
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Text Style - Name: %s (unsupported in AeSys)\n", textStyleName.c_str());

  // auto height = textStyle.height;         // Fixed text height; 0 if not fixed (group code 40)
  // auto width = textStyle.width;           // Width factor (group code 41)
  // auto obliqueAngle = textStyle.oblique;  // Oblique angle (group code 50)
  // auto textGenerationFlags =
      textStyle.flags;  // Text generation flags (group code 71) 0x02 - text is backward, mirrored in X - 0x04 - text is upside down, mirrored in Y
  // auto lastHeight = textStyle.lastHeight;  // Last height used (group code 42)

  // auto& font = textStyle.font;        // Primary font file name (group code 3)
  // auto& bigFont = textStyle.bigFont;  // Bigfont file name; blank if none (group code 4)
  //auto fontFamily = textStyle.fontFamily;  // A long value which contains a truetype font's pitch and family, charset, and italic and bold flags (group code 1071)
}

void EoDbDrwInterface::ConvertViewportTable(const DRW_Vport& viewport, AeSysDoc* /* document */) {
  std::wstring viewportName = StringToWString(viewport.name.c_str());
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Viewport - Name: %s (unsupported in AeSys)\n", viewportName.c_str());

  auto lowerLeft = EoGePoint3d(viewport.lowerLeft.x, viewport.lowerLeft.y, viewport.lowerLeft.z);          // group codes 10 and 20 (2D point)
  auto upperRight = EoGePoint3d(viewport.upperRight.x, viewport.upperRight.y, viewport.upperRight.z);      // group codes 11 and 21 (2D point)
  auto center = EoGePoint3d(viewport.center.x, viewport.center.y, viewport.center.z);                      // group codes 12 and 22 (2D point in DCS)
  auto snapBase = EoGePoint3d(viewport.snapBase.x, viewport.snapBase.y, viewport.snapBase.z);              // group codes 13 and 23 (2D point in DCS)
  auto snapSpacing = EoGePoint3d(viewport.snapSpacing.x, viewport.snapSpacing.y, viewport.snapSpacing.z);  // group codes 14 and 24 (2D point in DCS)
  auto gridSpacing = EoGePoint3d(viewport.gridSpacing.x, viewport.gridSpacing.y, viewport.gridSpacing.z);  // group codes 15 and 25 (2D point in DCS)
  auto viewDirection = EoGeVector3d(viewport.viewDir.x, viewport.viewDir.y, viewport.viewDir.z);           // group codes 16, 26 and 36 (3D point in WCS)
  auto viewTarget = EoGeVector3d(viewport.viewTarget.x, viewport.viewTarget.y, viewport.viewTarget.z);     // group codes 17, 27 and 37 (3D point in WCS)

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
 *  The DRW_Block parameter provides header information such as the block name, flags, base point, and handle. In your implementation, use this to initialize a new block object in your application's data model. All subsequent entity callbacks (e.g., for lines, arcs) will apply to this block until endBlock is called.
 * @note 
 * 0x01 = This is an anonymous block generated by hatching, associative dimensioning, other internal operations, or an application
 * 0x02 = This block has non-constant attribute definitions (this bit is not set if the block has any attribute definitions that are constant, or has no attribute definitions at all)
 * 0x04 = This block is an external reference (xref)
 * 0x08 = This block is an xref overlay
 * 0x10 = This block is externally dependent
 * 0x20 = This is a resolved external reference, or dependent of an external reference (ignored on input)
 * 0x40 = This definition is a referenced external reference (ignored on input)
 *
 * @note Three empty definitions always appear in the BLOCKS section. They are titled *Model_Space, *Paper_Space and *Paper_Space0. These definitions manifest the representations of model space and paper space as block definitions internally. The internal name of the first paper space layout is *Paper_Space, the second is *Paper_Space0, the third is *Paper_Space1, and so on.
 * The interleaving between model space and paper space no longer occurs. Instead, all paper space entities are output, followed by model space entities. The flag distinguishing them is the group code 67.
 */
void EoDbDrwInterface::ConvertBlock(const DRW_Block& block, AeSysDoc* document) {
  std::wstring blockName = StringToWString(block.name.c_str()); // Block Name (group code 2)
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"Block - Name: %s\n", blockName.c_str());

  // auto handle = block.handle;              // group code 5
  // auto parentHandle = block.parentHandle;  // Soft-pointer ID/handle to owner object (group code 330)

  // Group codes 3, 1 and 4 are for XREF definition. Modern XREF indicated by group 70 with 0x04 bit set and the presence of group code 1

  EoDbBlock* newBlock = new EoDbBlock(static_cast<EoUInt16>(block.flags),  //  Block-type bit-coded (see note) which may be combined (group code 70)
                                   EoGePoint3d(block.basePoint.x, block.basePoint.y, block.basePoint.z),  // group codes 10, 20 and 30
                                   blockName.c_str());

  document->InsertBlock(blockName.c_str(), newBlock);
}
/** @brief This method is primarily used in DWG files when the parser switches to entities belonging to a different block than the current one. The handle parameter corresponds to the block handle previously provided via addBlock (accessible as DRW_Block::handleBlock). In your implementation, switch the current block context to the one matching this handle. For DXF files, this callback may not be triggered, or it may be used sparingly if blocks are referenced out of sequence. */
void EoDbDrwInterface::ConvertBlockSet(const int /* handle */, AeSysDoc* /* document */) { ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Block set\n"); }

/** @brief This method signals the end of the current block definition. In your implementation, finalize the block (e.g., add it to a document's block table or collection) and reset the context to the default (model space or paper space).*/
void EoDbDrwInterface::ConvertBlockEnd(AeSysDoc* /* document */) { ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Block end\n"); }

// Entities
/*
void ConvertPrimitiveData(const EoDbPrimitive* primitive, OdDbBlockTableRecordPtr block, OdDbEntity* entity) {
  OdDbDatabase* Database = entity->database();
  entity->setDatabaseDefaults(Database);

  entity->setColorIndex(primitive->PenColor());

  OdDbLinetypeTablePtr Linetypes = Database->getLinetypeTableId().safeOpenObject(OdDb::kForWrite);
  OdDbObjectId Linetype = 0;
  if (primitive->LineType() == EoDbPrimitive::LINETYPE_BYLAYER) {
    Linetype = Linetypes->getLinetypeByLayerId();
  } else if (primitive->LineType() == EoDbPrimitive::LINETYPE_BYBLOCK) {
    Linetype = Linetypes->getLinetypeByBlockId();
  } else {
    EoDbLineTypeTable* LineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();

    EoDbLineType* LineType;
    LineTypeTable->LookupUsingLegacyIndex(primitive->LineType(), LineType);

    OdString Name = LineType->Name();
    Linetype = Linetypes->getAt(Name);
  }
  entity->setLinetype(Linetype);
}

OdDbEntity* EoDbLine::Convert(const OdDbObjectId& blockTableRecord) {
  OdDbBlockTableRecordPtr Block = blockTableRecord.safeOpenObject(OdDb::kForWrite);

  OdDbLinePtr LineEntity = OdDbLine::createObject();
  Block->appendOdDbEntity(LineEntity);

  ConvertPrimitiveData(this, Block, LineEntity);

  LineEntity->setStartPoint(OdGePoint3d(m_ln[0].x, m_ln[0].y, m_ln[0].z));
  LineEntity->setEndPoint(OdGePoint3d(m_ln[1].x, m_ln[1].y, m_ln[1].z));
  return LineEntity;
}
*/
#include "EoDbLine.h"
#include "EoDbGroup.h"

void EoDbDrwInterface::ConvertLineEntity(const DRW_Line& line, AeSysDoc* document) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Line entity conversion\n");

  auto penColor = line.color;

  auto* lineTypeTable = document->LineTypeTable();
  auto lineTypeName = CString(StringToWString(line.lineType.c_str()).c_str());
  auto lineTypeIndex = lineTypeTable->LegacyLineTypeIndex(lineTypeName);
  
  auto beginPoint = EoGePoint3d{line.basePoint.x, line.basePoint.y, line.basePoint.z};
  auto endPoint = EoGePoint3d{line.secPoint.x, line.secPoint.y, line.secPoint.z};

  auto newLine = new EoDbLine(beginPoint, endPoint);
  newLine->PenColor(static_cast<EoInt16>(penColor));
  newLine->LineType(static_cast<EoInt16>(lineTypeIndex));

  auto* group = new EoDbGroup();
  group->AddTail(newLine);

  auto layerName = StringToWString(line.layer.c_str());
  auto* layer = document->GetLayerTableLayer(layerName.c_str());
  layer->AddTail(group);
}