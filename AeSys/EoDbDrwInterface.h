#pragma once
#include <atltrace.h>
#include <string>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbHeaderSection.h"
#include "EoDbPrimitive.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "drw_interface.h"
#include "drw_objects.h"

// Minimal implementation of DRW_Interface
// In a real scenario, implement these methods to handle the parsed entities
class EoDbDrwInterface : public DRW_Interface {
 public:
  EoDbDrwInterface(AeSysDoc* document) : m_document(document) {}

  void addHeader(const DRW_Header* header) override {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DRW_Interface::addHeader called\n");
    ConvertHeaderSection(header, m_document);
  }

  // Table objects

  void addAppId(const DRW_AppId& appId) override {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DRW_Interface::addAppId called\n");
    ConvertAppIdTable(appId, m_document);
  }
  void addDimStyle(const DRW_Dimstyle& dimStyle) override {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DRW_Interface::addDimStyle called\n");
    ConvertDimStyle(dimStyle, m_document);
  }

  void addLayer(const DRW_Layer& layer) override {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DRW_Interface::addLayer called\n");
    ConvertLayerTable(layer, m_document);
  }
  void addLType(const DRW_LType& lType) override {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DRW_Interface::addLType called\n");
    ConvertLinetypesTable(lType, m_document);
  }
  void addTextStyle(const DRW_Textstyle& textStyle) override {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DRW_Interface::addTextStyle called\n");
    ConvertTextStyleTable(textStyle, m_document);
  }

  void addVport(const DRW_Vport& viewport) override {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DRW_Interface::addVport called\n");
    ConvertViewportTable(viewport, m_document);
  }

  // Blocks
  void addBlock(const DRW_Block& block) override {
    inBlockDefinition = true;
    blockName = Eo::MultiByteToWString(block.name.c_str());
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DRW_Interface::addBlock <%s>\n", blockName.c_str());
    ConvertBlock(block, m_document);
  }
  void setBlock(const int handle) override {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DRW_Interface::setBlock\n");
    ConvertBlockSet(handle, m_document);
  }
  void endBlock() override {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"DRW_Interface::endBlock\n");
    inBlockDefinition = false;
    blockName.clear();
    ConvertBlockEnd(m_document);
  }

  // Entities

  void add3dFace(const DRW_3Dface& /* 3dFace */) override { countOf3dFace++; }
  void addArc(const DRW_Arc& /* arc */) override { countOfArc++; }
  void addCircle(const DRW_Circle& /* circle */) override { countOfCircle++; }
  void addEllipse(const DRW_Ellipse& /* ellipse */) override { countOfEllipse++; }
  void addHatch(const DRW_Hatch* /* hatch */) override { countOfHatch++; }
  void addImage(const DRW_Image* /* image */) override { countOfImage++; }
  void addInsert(const DRW_Insert& /* insert */) override { countOfInsert++; }
  void addKnot(const DRW_Entity& /* knot */) override { countOfKnot++; }

  void addLine(const DRW_Line& line) override {
    countOfLine++;
    if (inBlockDefinition) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"DRW_Interface::addLine - block <%s>\n", blockName.c_str());
      ConvertLineEntity(line, m_document);
    } else {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"DRW_Interface::addLine - entities section\n");
      ConvertLineEntity(line, m_document);
    }
  }
  void addLWPolyline(const DRW_LWPolyline& lwPolyline) override {
    countOfLWPolyline++;
    if (inBlockDefinition) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"DRW_Interface::addLine - block <%s>\n", blockName.c_str());
      ConvertLWPolylineEntity(lwPolyline, m_document);
    } else {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"DRW_Interface::addLine - entities section\n");
      ConvertLWPolylineEntity(lwPolyline, m_document);
    }
  }
  void addMText(const DRW_MText& /* mText */) override { countOfMText++; }

  void addPoint(const DRW_Point& point) override {
    countOfPoint++;
    if (inBlockDefinition) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"DRW_Interface::addPoint - block <%s>\n", blockName.c_str());
      ConvertPointEntity(point, m_document);
    } else {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"DRW_Interface::addPoint - entities section\n");
      ConvertPointEntity(point, m_document);
    }
  }

  void addPolyline(const DRW_Polyline& /* polyline */) override { countOfPolyline++; }
  void addRay(const DRW_Ray& /* ray */) override { countOfRay++; }
  void addSolid(const DRW_Solid& /* solid */) override { countOfSolid++; }
  void addSpline(const DRW_Spline* /* spline */) override { countOfSpline++; }
  void addText(const DRW_Text& /* text */) override { countOfText++; }
  void addTrace(const DRW_Trace& /* trace */) override { countOfTrace++; }
  void addViewport(const DRW_Viewport& /* viewport */) override { countOfViewport++; }
  void addXline(const DRW_Xline& /* Xline */) override { countOfXline++; }

  // Dimensions
  void addDimAlign(const DRW_DimAligned* /* dimAlign */) override { countOfDimAlign++; }
  void addDimAngular(const DRW_DimAngular* /* dimAngular */) override { countOfDimAngular++; }
  void addDimAngular3P(const DRW_DimAngular3p* /* dimAngular3P */) override { countOfDimAngular3P++; }
  void addDimDiametric(const DRW_DimDiametric* /* dimDiametric */) override { countOfDimDiametric++; }
  void addDimLinear(const DRW_DimLinear* /* dimLinear */) override { countOfDimLinear++; }
  void addDimOrdinate(const DRW_DimOrdinate* /* dimOrdinate */) override { countOfDimOrdinate++; }
  void addDimRadial(const DRW_DimRadial* /* dimRadial */) override { countOfDimRadial++; }

  // Others
  void addComment(const char* comment) override { ATLTRACE2(static_cast<int>(atlTraceGeneral), 2, L"DRW_Interface::addComment(%s)\n", comment); }
  void addLeader(const DRW_Leader* /* leader */) override {}
  void linkImage(const DRW_ImageDef* /* imageDefinition */) override {}

  // Writing methods
  void writeAppId() override {};
  void writeBlockRecords() override {};
  void writeBlocks() override {};
  void writeDimstyles() override {};
  void writeEntities() override {};
  void writeHeader(DRW_Header& /* header */) override {};
  void writeLayers() override {};
  void writeLTypes() override {};
  void writeTextstyles() override {};
  void writeVports() override {};

  void SetHeaderSectionVariable(const DRW_Header* header, const std::string& keyToFind, EoDbHeaderSection& headerSection);

  void ConvertHeaderSection(const DRW_Header* header, AeSysDoc* document);

  void ConvertAppIdTable(const DRW_AppId& appId, AeSysDoc* document);
  void ConvertDimStyle(const DRW_Dimstyle& dimStyle, AeSysDoc* document);

  /** @brief Converts a DRW_Layer object to the corresponding AeSys document representation.
   *
   * This method takes a DRW_Layer object, which represents layer information from a DXF/DWG file, and converts it into the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param layer The DRW_Layer object containing layer data to be converted.
   * @param document A pointer to the AeSysDoc where the converted layer will be stored.
   */
  void ConvertLayerTable(const DRW_Layer& layer, AeSysDoc* document);

  /** @brief Converts a DRW_LType object to the corresponding AeSys document representation.
   *
   * This method takes a DRW_LType object, which represents line type information from a DXF/DWG file, and converts it into the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param lineType The DRW_LType object containing line type data to be converted.
   * @param document A pointer to the AeSysDoc where the converted line type will be stored.
   * @note unimplemented complex linetype elements (group code 74)
   *  Complex linetype element type (one per element). Default is 0 (no embedded shape/text). The following codes are bit values:
   *    0x01 = If set, code 50 specifies an absolute rotation; else code 50 specifies a relative rotation
   *    0x02 = Embedded element is a text string
   *    0x04 = Embedded element is a shape
   *
   *   group code 75 - Shape number (one per element) if code 74 specifies an embedded shape. If code 74 specifies an embedded text string, this value is set to 0 else if code 74 is set to 0, code 75 is omitted.
   *   group code 340 - Pointer to STYLE object (one per element if code 74 > 0)
   *   group code 46 - S = Scale value (optional); multiple entries can exist
   *   group code 50 - R = (relative) or A = (absolute) rotation value in radians of embedded shape or text; one per element if code 74 specifies an embedded shape or text string
   *   group code 44 - X = X offset value (optional); multiple entries can exist
   *   group code 45 - Y = Y offset value (optional); multiple entries can exist
   *   group code 9 - Text string (one per element if code 74 = 2)
   */
  void ConvertLinetypesTable(const DRW_LType& lineType, AeSysDoc* document);

  /** @brief Converts a DRW_Textstyle object to the corresponding AeSys document representation.
   *
   * This method takes a DRW_Textstyle object, which represents text style information from a DXF/DWG file, and converts it into the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param textStyle The DRW_Textstyle object containing text style data to be converted.
   * @param document A pointer to the AeSysDoc where the converted text style will be stored.
   * @note A style table item is also used to record shape file LOAD command requests. In this case the first bit (0x01) is set in the 70 group flags and only the 3 group (shape file name) is meaningful (all the other groups are output, however).
   */
  void ConvertTextStyleTable(const DRW_Textstyle& textStyle, AeSysDoc* document);

  /** @brief Converts a DRW_Vport object to the corresponding AeSys document representation.
   *
   * This method takes a DRW_Vport object, which represents viewport information from a DXF/DWG file, and converts it into the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param viewport The DRW_Vport object containing viewport data to be converted.
   * @param document A pointer to the AeSysDoc where the converted viewport will be stored.
   */
  void ConvertViewportTable(const DRW_Vport& viewport, AeSysDoc* document);

  void ConvertBlock(const DRW_Block& block, AeSysDoc* document);
  void ConvertBlockSet(const int handle, AeSysDoc* document);
  void ConvertBlockEnd(AeSysDoc* document);

  void AddToDocument(EoDbPrimitive* primitive, AeSysDoc* document);

  void ConvertLineEntity(const DRW_Line& line, AeSysDoc* document);
  void ConvertLWPolylineEntity(const DRW_LWPolyline& lwPolyline, AeSysDoc* document);
  void ConvertPointEntity(const DRW_Point& point, AeSysDoc* document);

 private:
  AeSysDoc* m_document{nullptr};
  std::wstring blockName{};
  bool inBlockDefinition{false};

 public:
  int countOf3dFace{0}; int countOfArc{0}; int countOfCircle{0}; int countOfDimAlign{0}; int countOfDimAngular{0}; int countOfDimAngular3P{0}; int countOfDimDiametric{0}; int countOfDimLinear{0}; int countOfDimOrdinate{0}; int countOfDimRadial{0}; int countOfEllipse{0}; int countOfHatch{0}; int countOfImage{0}; int countOfInsert{0}; int countOfKnot{0}; int countOfLine{0}; int countOfLWPolyline{0}; int countOfMText{0}; int countOfPoint{0}; int countOfPolyline{0}; int countOfRay{0}; int countOfSolid{0}; int countOfSpline{0}; int countOfText{0}; int countOfTrace{0}; int countOfViewport{0}; int countOfXline{0}; 
};