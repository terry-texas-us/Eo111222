#pragma once

#include <string>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbBlock.h"
#include "EoDbHeaderSection.h"
#include "EoDbPrimitive.h"
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

  void addHeader(const EoDxfHeader* header) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addHeader called\n");
    ConvertHeaderSection(header, m_document);
  }

  void addClass(const EoDxfClass& class_) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addClass called\n");
    ConvertClassesSection(class_, m_document);
  }

  // Table objects

  void addAppId(const EoDxfAppId& appId) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addAppId called\n");
    ConvertAppIdTable(appId, m_document);
  }
  void addDimStyle(const EoDxfDimensionStyle& dimensionStyle) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addDimStyle called\n");
    ConvertDimStyle(dimensionStyle, m_document);
  }

  void addLayer(const EoDxfLayer& layer) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addLayer called\n");
    ConvertLayerTable(layer, m_document);
  }
  void addLinetype(const EoDxfLinetype& lType) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addLinetype called\n");
    ConvertLinetypesTable(lType, m_document);
  }
  void addTextStyle(const EoDxfTextStyle& textStyle) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addTextStyle called\n");
    ConvertTextStyleTable(textStyle, m_document);
  }

  void addVport(const EoDxfVPort& viewport) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addVport called\n");
    ConvertVPortTable(viewport, m_document);
  }

  // Blocks
  void addBlock(const EoDxfBlock& block) override {
    m_inBlockDefinition = true;
    m_blockName = Eo::MultiByteToWString(block.name.c_str());
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addBlock <%s>\n", m_blockName.c_str());
    m_currentOpenBlockDefinition = ConvertBlock(block, m_document);
  }
  void setBlock(const int handle) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::setBlock\n");
    ConvertBlockSet(handle, m_document);
  }
  void endBlock() override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::endBlock\n");
    m_inBlockDefinition = false;
    m_currentOpenBlockDefinition = nullptr;
    m_blockName.clear();
    ConvertBlockEnd(m_document);
  }

  // AutoDesk DXF Reference for Entities Section
  // https://help.autodesk.com/view/OARX/2018/ENU/?guid=GUID-7D07C886-FD1D-4A0C-A7AB-B4D21F18E484

  void add3dFace(const EoDxf3dFace& /* 3dFace */) override { countOf3dFace++; }
  // 3DSOLID not implemented
  // ACAD_PROXY_ENTITY not implemented

  void addArc(const EoDxfArc& arc) override {
    countOfArc++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addArc - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addArc - entities section\n");
    }
    ConvertArcEntity(arc, m_document);
  }
  // ATTDEF not implemented
  // ATTRIB not implemented
  // BODY not implemented
  void addCircle(const EoDxfCircle& circle) override {
    countOfCircle++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addCircle - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addCircle - entities section\n");
    }
    ConvertCircleEntity(circle, m_document);
  }

  // COORDINATION_MODEL not implemented

  // Dimensions

  void addDimAlign([[maybe_unused]] const EoDxfAlignedDimension* dimAlign) override { countOfDimAlign++; }

  void addDimAngular([[maybe_unused]] const EoDxf2LineAngularDimension* dimAngular) override { countOfDimAngular++; }
  void addDimAngular3P([[maybe_unused]] const EoDxf3PointAngularDimension* dimAngular3P) override { countOfDimAngular3P++; }

  void addDimLinear([[maybe_unused]] const EoDxfDimLinear* dimLinear) override { countOfDimLinear++; }

  void addDimOrdinate([[maybe_unused]] const EoDxfOrdinateDimension* dimOrdinate) override { countOfDimOrdinate++; }

  void addDimRadial([[maybe_unused]] const EoDxfRadialDimension* dimRadial) override { countOfDimRadial++; }

  void addDimDiametric([[maybe_unused]] const EoDxfDiametricDimension* dimDiametric) override { countOfDimDiametric++; }

  void addEllipse(const EoDxfEllipse& ellipse) override {
    countOfEllipse++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addEllipse - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addEllipse - entities section\n");
    }
    ConvertEllipseEntity(ellipse, m_document);
  }
  void addHatch([[maybe_unused]] const EoDxfHatch* hatch) override { countOfHatch++; }

  // HELIX not implemented
  void addImage([[maybe_unused]] const EoDxfImage* image) override { countOfImage++; }

  void addInsert(const EoDxfInsert& blockReference) override {
    countOfInsert++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addInsert - block <%s>\n", m_blockName.c_str());
      ConvertInsertEntity(blockReference, m_document);
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addInsert - entities section\n");
      ConvertInsertEntity(blockReference, m_document);
    }
  }

  void addLeader(const EoDxfLeader* leader) override { (void)leader; }
  // LIGHT not implemented

  void addLine(const EoDxfLine& line) override {
    countOfLine++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addLine - block <%s>\n", m_blockName.c_str());
      ConvertLineEntity(line, m_document);
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addLine - entities section\n");
      ConvertLineEntity(line, m_document);
    }
  }

  void addLWPolyline(const EoDxfLwPolyline& polyline) override {
    countOfLWPolyline++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addLWPolyline - block <%s>\n", m_blockName.c_str());
      ConvertLWPolylineEntity(polyline, m_document);
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addLWPolyline - entities section\n");
      ConvertLWPolylineEntity(polyline, m_document);
    }
  }
  // MESH not implemented
  
  void addMLeader(const EoDxfMLeader* mLeader) override { (void)mLeader; }

  // MLINE not implemented
  void addMText(const EoDxfMText& mText) override {
    (void)mText;
    countOfMText++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addMText - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addMText - entities section\n");
    }
  }

  // OLEFRAME not implemented
  // OLE2FRAME not implemented

  void addPoint(const EoDxfPoint& point) override {
    countOfPoint++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addPoint - block <%s>\n", m_blockName.c_str());
      ConvertPointEntity(point, m_document);
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::addPoint - entities section\n");
      ConvertPointEntity(point, m_document);
    }
  }

  void addPolyline(const EoDxfPolyline& /* polyline */) override { countOfPolyline++; }
  void addRay(const EoDxfRay& /* ray */) override { countOfRay++; }
  // REGION not implemented
  // SECTION not implemented
  // SEQEND not implemented
  // SHAPE not implemented

  void addSolid(const EoDxfSolid& /* solid */) override { countOfSolid++; }
  void addSpline(const EoDxfSpline* spline) override {
    (void)spline;

    countOfSpline++;
  }
  // SUN not implemented
  // SURFACE not implemented
  // TABLE not implemented

  void addText(const EoDxfText& text) override {
    (void)text;
    countOfText++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addText - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addText - entities section\n");
    }
  }

  // TOLERANCE not implemented
  void addTrace(const EoDxfTrace& /* trace */) override { countOfTrace++; }
  // UNDERLAY not implemented
  // VERTEX not implemented
  void addViewport(const EoDxfViewport& /* viewport */) override { countOfViewport++; }
  // WIPEOUT not implemented
  void addXline(const EoDxfXline& /* Xline */) override { countOfXline++; }

  // Others
  void addComment(const char* comment) override {
    ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addComment(%s)\n", comment);
  }
  void linkImage(const EoDxfImageDefinition* imageDefinition) override { (void)imageDefinition; }
  void addKnot(const EoDxfEntity& /* knot */) override { countOfKnot++; }

  // Writing methods
  void writeAppId() override {};
  void writeBlockRecords() override {};
  void writeBlocks() override {};
  void writeDimstyles() override {};
  void writeEntities() override {};
  void writeHeader(EoDxfHeader& /* header */) override {};
  void writeLayers() override {};
  void writeLTypes() override {};
  void writeTextstyles() override {};
  void writeVports() override {};

  void SetHeaderSectionVariable(
      const EoDxfHeader* header, const std::string& keyToFind, EoDbHeaderSection& headerSection);

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
   * This method takes a EoDxfLinetype object, which represents line type information from a DXF/DWG file, and converts it
   * into the appropriate format for storage in the provided AeSysDoc document.
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
   * This method takes a EoDxfTextstyle object, which represents text style information from a DXF/DWG file, and converts
   * it into the appropriate format for storage in the provided AeSysDoc document.
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

  void ConvertArcEntity(const EoDxfArc& arc, AeSysDoc* document);
  void ConvertCircleEntity(const EoDxfCircle& circle, AeSysDoc* document);
  void ConvertEllipseEntity(const EoDxfEllipse& ellipse, AeSysDoc* document);
  void ConvertInsertEntity(const EoDxfInsert& insert, AeSysDoc* document);
  void ConvertLineEntity(const EoDxfLine& line, AeSysDoc* document);
  void ConvertLWPolylineEntity(const EoDxfLwPolyline& lwPolyline, AeSysDoc* document);
  void ConvertPointEntity(const EoDxfPoint& point, AeSysDoc* document);

 private:
  AeSysDoc* m_document{};
  std::wstring m_blockName{};
  bool m_inBlockDefinition{};
  EoDbBlock* m_currentOpenBlockDefinition{};

 public:
  int countOf3dFace{};
  int countOfArc{};
  int countOfCircle{};
  int countOfDimAlign{};
  int countOfDimAngular{};
  int countOfDimAngular3P{};
  int countOfDimDiametric{};
  int countOfDimLinear{};
  int countOfDimOrdinate{};
  int countOfDimRadial{};
  int countOfEllipse{};
  int countOfHatch{};
  int countOfImage{};
  int countOfInsert{};
  int countOfKnot{};
  int countOfLine{};
  int countOfLWPolyline{};
  int countOfMText{};
  int countOfPoint{};
  int countOfPolyline{};
  int countOfRay{};
  int countOfSolid{};
  int countOfSpline{};
  int countOfText{};
  int countOfTrace{};
  int countOfViewport{};
  int countOfXline{};
};