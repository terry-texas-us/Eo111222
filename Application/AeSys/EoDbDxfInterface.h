#pragma once

#include <string>

#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDbHeaderSection.h"
#include "EoDbPrimitive.h"
#include "EoDxfAttributes.h"
#include "EoDxfClasses.h"
#include "EoDxfEntities.h"
#include "EoDxfHeader.h"
#include "EoDxfInterface.h"
#include "EoDxfMLeader.h"
#include "EoDxfObjects.h"

// Minimal implementation of EoDxfInterface
// In a real scenario, implement these methods to handle the parsed entities
class EoDbDxfInterface : public EoDxfInterface {
 public:
  EoDbDxfInterface(AeSysDoc* document) : m_document(document) {}

  void AddHeader(const EoDxfHeader* header) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddHeader called\n");
    ConvertHeaderSection(header, m_document);
  }

  void AddClass(const EoDxfClass& class_) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddClass called\n");
    ConvertClassesSection(class_, m_document);
  }

  // Table objects

  void AddAppId(const EoDxfAppId& appId) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddAppId called\n");
    ConvertAppIdTable(appId, m_document);
  }
  void AddDimStyle(const EoDxfDimensionStyle& dimensionStyle) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddDimStyle called\n");
    ConvertDimStyle(dimensionStyle, m_document);
  }

  void AddLayer(const EoDxfLayer& layer) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddLayer called\n");
    ConvertLayerTable(layer, m_document);
  }
  void AddLinetype(const EoDxfLinetype& lType) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddLinetype called\n");
    ConvertLinetypesTable(lType, m_document);
  }
  void AddTextStyle(const EoDxfTextStyle& textStyle) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddTextStyle called\n");
    ConvertTextStyleTable(textStyle, m_document);
  }

  void AddVport(const EoDxfVPort& viewport) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddVport called\n");
    ConvertVPortTable(viewport, m_document);
  }

  // Blocks
  void AddBlock(const EoDxfBlock& block) override {
    m_inBlockDefinition = true;
    m_blockName = block.m_blockName;
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddBlock <%s>\n", m_blockName.c_str());
    m_currentOpenBlockDefinition = ConvertBlock(block, m_document);
  }
  void SetBlock(const int handle) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::SetBlock\n");
    ConvertBlockSet(handle, m_document);
  }
  void EndBlock() override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::EndBlock\n");
    m_inBlockDefinition = false;
    m_currentOpenBlockDefinition = nullptr;
    m_blockName.clear();
    ConvertBlockEnd(m_document);
  }

  // AutoDesk DXF Reference
  // https://help.autodesk.com/view/OARX/2026/ENU/?guid=GUID-235B22E0-A567-4CF6-92D3-38A2306D73F3

  void Add3dFace([[maybe_unused]] const EoDxf3dFace& _3dFace) override { countOf3dFace--; }

  // 3DSOLID not implemented

  void AddAcadProxyEntity(const EoDxfAcadProxyEntity& proxyEntity) override {
    countOfAcadProxyEntity++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddAcadProxyEntity - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddAcadProxyEntity - entities section\n");
    }
    ConvertAcadProxyEntity(proxyEntity, m_document);
  }

  void AddArc(const EoDxfArc& arc) override {
    countOfArc++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddArc - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddArc - entities section\n");
    }
    ConvertArcEntity(arc, m_document);
  }

  void AddAttDef(const EoDxfAttDef& attdef) override {
    countOfAttDef++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddAttDef - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddAttDef - entities section\n");
    }
    ConvertAttDefEntity(attdef, m_document);
  }

  void AddAttrib(const EoDxfAttrib& attrib) override {
    countOfAttrib++;
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddAttrib - entities section\n");
    ConvertAttribEntity(attrib, m_document);
  }

  // BODY not implemented
  void AddCircle(const EoDxfCircle& circle) override {
    countOfCircle++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddCircle - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddCircle - entities section\n");
    }
    ConvertCircleEntity(circle, m_document);
  }

  // COORDINATION_MODEL not implemented

  // Dimensions

  void AddDimAlign([[maybe_unused]] const EoDxfAlignedDimension* dimAlign) override { countOfDimAlign--; }

  void AddDimAngular([[maybe_unused]] const EoDxf2LineAngularDimension* dimAngular) override { countOfDimAngular--; }
  void AddDimAngular3P([[maybe_unused]] const EoDxf3PointAngularDimension* dimAngular3P) override {
    countOfDimAngular3P--;
  }

  void AddDimLinear([[maybe_unused]] const EoDxfDimLinear* dimLinear) override { countOfDimLinear--; }
  void AddDimOrdinate([[maybe_unused]] const EoDxfOrdinateDimension* dimOrdinate) override { countOfDimOrdinate--; }

  void AddDimRadial([[maybe_unused]] const EoDxfRadialDimension* dimRadial) override { countOfDimRadial--; }

  void AddDimDiametric([[maybe_unused]] const EoDxfDiametricDimension* dimDiametric) override { countOfDimDiametric--; }

  void AddEllipse(const EoDxfEllipse& ellipse) override {
    countOfEllipse++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddEllipse - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddEllipse - entities section\n");
    }
    ConvertEllipseEntity(ellipse, m_document);
  }

  void AddHatch(const EoDxfHatch& hatch) override {
    countOfHatch++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddHatch - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddHatch - entities section\n");
    }
    ConvertHatchEntity(hatch, m_document);
  }

  // HELIX not implemented

  void AddImage([[maybe_unused]] const EoDxfImage* image) override { countOfImage--; }

  void AddUnsupportedObject(const EoDxfUnsupportedObject& /* objectData */) override {}

  void AddInsert(const EoDxfInsert& blockReference) override {
    countOfInsert++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddInsert - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddInsert - entities section\n");
    }
    ConvertInsertEntity(blockReference, m_document);
  }

  void AddLeader(const EoDxfLeader* leader) override { (void)leader; }

  // LIGHT not implemented

  void AddLine(const EoDxfLine& line) override {
    countOfLine++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddLine - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddLine - entities section\n");
    }
    ConvertLineEntity(line, m_document);
  }

  void AddLWPolyline(const EoDxfLwPolyline& polyline) override {
    countOfLWPolyline++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddLWPolyline - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddLWPolyline - entities section\n");
    }
    ConvertLWPolylineEntity(polyline, m_document);
  }
  // MESH not implemented

  void AddMLeader(const EoDxfMLeader* mLeader) override { (void)mLeader; }

  // MLINE not implemented

  void AddMText(const EoDxfMText& mText) override {
    countOfMText++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addMText - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addMText - entities section\n");
    }
    ConvertMTextEntity(mText, m_document);
  }

  // OLEFRAME not implemented
  // OLE2FRAME not implemented

  void AddPoint(const EoDxfPoint& point) override {
    countOfPoint++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddPoint - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddPoint - entities section\n");
    }
    ConvertPointEntity(point, m_document);
  }

  void AddPolyline([[maybe_unused]] const EoDxfPolyline& polyline) override { countOfPolyline--; }
  void AddRay([[maybe_unused]] const EoDxfRay& ray) override { countOfRay--; }

  // REGION not implemented
  // SECTION not implemented
  // SEQEND not implemented
  // SHAPE not implemented

  void AddSolid([[maybe_unused]] const EoDxfSolid& solid) override { countOfSolid--; }

  void AddSpline([[maybe_unused]] const EoDxfSpline& spline) override {
    countOfSpline--;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddSpline - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddSpline - entities section\n");
    }
    ConvertSplineEntity(spline, m_document);
  }

  // SUN not implemented
  // SURFACE not implemented
  // TABLE not implemented

  void AddText(const EoDxfText& text) override {
    countOfText++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddText - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddText - entities section\n");
    }
    ConvertTextEntity(text, m_document);
  }

  // TOLERANCE not implemented
  void AddTrace([[maybe_unused]] const EoDxfTrace& trace) override { countOfTrace--; }
  // UNDERLAY not implemented
  // VERTEX not implemented
  void AddViewport([[maybe_unused]] const EoDxfViewport& viewport) override { countOfViewport--; }
  // WIPEOUT not implemented
  void AddXline([[maybe_unused]] const EoDxfXline& Xline) override { countOfXline--; }

  // Others
  void AddComment(std::wstring_view comment) override {
    ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddComment(%.*s)\n", static_cast<int>(comment.size()), comment.data());
  }
  void LinkImage([[maybe_unused]] const EoDxfImageDefinition* imageDefinition) override {}
  void AddKnot([[maybe_unused]] const EoDxfGraphic& knot) override { countOfKnot--; }

  // Writing methods
  void WriteAppId() override {};
  void WriteBlockRecords() override {};
  void WriteBlocks() override {};
  void WriteClasses() override {};
  void WriteDimstyles() override {};
  void WriteEntities() override {};
  void WriteHeader(EoDxfHeader& /* header */) override {};
  void WriteObjects() override {};
  void WriteUnsupportedObjects() override {};
  void WriteLayers() override {};
  void WriteLTypes() override {};
  void WriteTextstyles() override {};
  void WriteVports() override {};

  void SetHeaderSectionVariable(
      const EoDxfHeader* header, std::wstring_view keyToFind, EoDbHeaderSection& headerSection);

  void ConvertHeaderSection(const EoDxfHeader* header, AeSysDoc* document);
  void ConvertClassesSection(const EoDxfClass& class_, AeSysDoc* document);

  void ConvertAppIdTable(const EoDxfAppId& appId, AeSysDoc* document);
  void ConvertDimStyle(const EoDxfDimensionStyle& dimensionStyle, AeSysDoc* document);

  /** @brief Converts a EoDxfLayer object to the corresponding AeSys document representation.
   *
   * This method takes a EoDxfLayer object, which represents layer information from a DXF/DWG file, and converts it into
   * the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param layer The EoDxfLayer object containing layer data to be converted.
   * @param document A pointer to the AeSysDoc where the converted layer will be stored.
   */
  void ConvertLayerTable(const EoDxfLayer& layer, AeSysDoc* document);

  /** @brief Converts a EoDxfLinetype object to the corresponding AeSys document representation.
   *
   * This method takes a EoDxfLinetype object, which represents line type information from a DXF/DWG file, and converts
   * it into the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param lineType The EoDxfLinetype object containing line type data to be converted.
   * @param document A pointer to the AeSysDoc where the converted line type will be stored.
   * @note unimplemented complex linetype elements (group code 74)
   *  Complex linetype element type (one per element). Default is 0 (no embedded shape/text). The following codes are
   * bit values: 0x01 = If set, code 50 specifies an absolute rotation; else code 50 specifies a relative rotation 0x02
   * = Embedded element is a text string 0x04 = Embedded element is a shape
   *
   *   group code 75 - Shape number (one per element) if code 74 specifies an embedded shape. If code 74 specifies an
   * embedded text string, this value is set to 0 else if code 74 is set to 0, code 75 is omitted. group code 340 -
   * Pointer to STYLE object (one per element if code 74 > 0) group code 46 - S = Scale value (optional); multiple
   * entries can exist group code 50 - R = (relative) or A = (absolute) rotation value in radians of embedded shape or
   * text; one per element if code 74 specifies an embedded shape or text string group code 44 - X = X offset value
   * (optional); multiple entries can exist group code 45 - Y = Y offset value (optional); multiple entries can exist
   *   group code 9 - Text string (one per element if code 74 = 2)
   */
  void ConvertLinetypesTable(const EoDxfLinetype& lineType, AeSysDoc* document);

  /** @brief Converts a EoDxfTextstyle object to the corresponding AeSys document representation.
   *
   * This method takes a EoDxfTextstyle object, which represents text style information from a DXF/DWG file, and
   * converts it into the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param textStyle The EoDxfTextStyle object containing text style data to be converted.
   * @param document A pointer to the AeSysDoc where the converted text style will be stored.
   * @note A style table item is also used to record shape file LOAD command requests. In this case the first bit (0x01)
   * is set in the 70 group flags and only the 3 group (shape file name) is meaningful (all the other groups are output,
   * however).
   */
  void ConvertTextStyleTable(const EoDxfTextStyle& textStyle, AeSysDoc* document);

  /** @brief Converts a EoDxfVPort object to the corresponding AeSys document representation.
   *
   * This method takes a EoDxfVPort object, which represents viewport information from a DXF/DWG file, and converts it
   * into the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param viewport The EoDxfVPort object containing viewport data to be converted.
   * @param document A pointer to the AeSysDoc where the converted viewport will be stored.
   */
  void ConvertVPortTable(const EoDxfVPort& viewport, AeSysDoc* document);

  EoDbBlock* ConvertBlock(const EoDxfBlock& block, AeSysDoc* document);
  void ConvertBlockSet(const int handle, AeSysDoc* document);
  void ConvertBlockEnd(AeSysDoc* document);

  void AddToDocument(EoDbPrimitive* primitive, AeSysDoc* document);

  void ConvertAcadProxyEntity(const EoDxfAcadProxyEntity& proxyEntity, AeSysDoc* document);
  void ConvertArcEntity(const EoDxfArc& arc, AeSysDoc* document);
  void ConvertAttDefEntity(const EoDxfAttDef& attdef, [[maybe_unused]] AeSysDoc* document);
  void ConvertAttribEntity(const EoDxfAttrib& attrib, AeSysDoc* document);
  void ConvertCircleEntity(const EoDxfCircle& circle, AeSysDoc* document);
  void ConvertEllipseEntity(const EoDxfEllipse& ellipse, AeSysDoc* document);
  void ConvertHatchEntity(const EoDxfHatch& hatch, [[maybe_unused]] AeSysDoc* document);
  void ConvertInsertEntity(const EoDxfInsert& insert, AeSysDoc* document);
  void ConvertLineEntity(const EoDxfLine& line, AeSysDoc* document);
  void ConvertLWPolylineEntity(const EoDxfLwPolyline& lwPolyline, AeSysDoc* document);
  void ConvertMTextEntity(const EoDxfMText& mText, [[maybe_unused]] AeSysDoc* document);
  void ConvertPointEntity(const EoDxfPoint& point, AeSysDoc* document);
  void ConvertSplineEntity(const EoDxfSpline& spline, [[maybe_unused]] AeSysDoc* document);
  void ConvertTextEntity(const EoDxfText& text, [[maybe_unused]] AeSysDoc* document);

 private:
  AeSysDoc* m_document{};
  std::wstring m_blockName{};
  bool m_inBlockDefinition{};
  EoDbBlock* m_currentOpenBlockDefinition{};

 public:
  std::int16_t countOf3dFace{};
  std::int16_t countOfAcadProxyEntity{};
  std::int16_t countOfArc{};
  std::int16_t countOfAttDef{};
  std::int16_t countOfAttrib{};
  std::int16_t countOfCircle{};
  std::int16_t countOfDimAlign{};
  std::int16_t countOfDimAngular{};
  std::int16_t countOfDimAngular3P{};
  std::int16_t countOfDimDiametric{};
  std::int16_t countOfDimLinear{};
  std::int16_t countOfDimOrdinate{};
  std::int16_t countOfDimRadial{};
  std::int16_t countOfEllipse{};
  std::int16_t countOfHatch{};
  std::int16_t countOfImage{};
  std::int16_t countOfInsert{};
  std::int16_t countOfKnot{};
  std::int16_t countOfLine{};
  std::int16_t countOfLWPolyline{};
  std::int16_t countOfMText{};
  std::int16_t countOfPoint{};
  std::int16_t countOfPolyline{};
  std::int16_t countOfRay{};
  std::int16_t countOfSolid{};
  std::int16_t countOfSpline{};
  std::int16_t countOfText{};
  std::int16_t countOfTrace{};
  std::int16_t countOfViewport{};
  std::int16_t countOfXline{};
};