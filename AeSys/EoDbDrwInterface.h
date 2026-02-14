#pragma once

#include <string>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbBlock.h"
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
    ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addHeader called\n");
    ConvertHeaderSection(header, m_document);
  }

  // Table objects

  void addAppId(const DRW_AppId& appId) override {
    ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addAppId called\n");
    ConvertAppIdTable(appId, m_document);
  }
  void addDimStyle(const DRW_Dimstyle& dimStyle) override {
    ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addDimStyle called\n");
    ConvertDimStyle(dimStyle, m_document);
  }

  void addLayer(const DRW_Layer& layer) override {
    ATLTRACE2(traceGeneral, 1, L"DRW_Interface::addLayer called\n");
    ConvertLayerTable(layer, m_document);
  }
  void addLType(const DRW_LType& lType) override {
    ATLTRACE2(traceGeneral, 1, L"DRW_Interface::addLType called\n");
    ConvertLinetypesTable(lType, m_document);
  }
  void addTextStyle(const DRW_Textstyle& textStyle) override {
    ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addTextStyle called\n");
    ConvertTextStyleTable(textStyle, m_document);
  }

  void addVport(const DRW_Vport& viewport) override {
    ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addVport called\n");
    ConvertViewportTable(viewport, m_document);
  }

  // Blocks
  void addBlock(const DRW_Block& block) override {
    inBlockDefinition = true;
    blockName = Eo::MultiByteToWString(block.name.c_str());
    ATLTRACE2(traceGeneral, 1, L"DRW_Interface::addBlock <%s>\n", blockName.c_str());
    currentOpenBlockDefinition = ConvertBlock(block, m_document);
  }
  void setBlock(const int handle) override {
    ATLTRACE2(traceGeneral, 1, L"DRW_Interface::setBlock\n");
    ConvertBlockSet(handle, m_document);
  }
  void endBlock() override {
    ATLTRACE2(traceGeneral, 1, L"DRW_Interface::endBlock\n");
    inBlockDefinition = false;
    currentOpenBlockDefinition = nullptr;
    blockName.clear();
    ConvertBlockEnd(m_document);
  }

  // AutoDesk DXF Reference for Entities Section
  // https://help.autodesk.com/view/OARX/2018/ENU/?guid=GUID-7D07C886-FD1D-4A0C-A7AB-B4D21F18E484

  void add3dFace(const DRW_3Dface& /* 3dFace */) override { countOf3dFace++; }
  // 3DSOLID not implemented in DRW
  // ACAD_PROXY_ENTITY not implemented in DRW
  
  void addArc(const DRW_Arc& arc) override { 
    countOfArc++; 
    if (inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"DRW_Interface::addArc - block <%s>\n", blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addArc - entities section\n");
    }
    ConvertArcEntity(arc, m_document);
  }
  // ATTDEF not implemented in DRW
  // ATTRIB not implemented in DRW
  // BODY not implemented in DRW
  void addCircle(const DRW_Circle& circle) override { 
    countOfCircle++; 
    if (inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"DRW_Interface::addCircle - block <%s>\n", blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addCircle - entities section\n");
    }
    ConvertCircleEntity(circle, m_document);
  }
    
  // COORDINATION_MODEL not implemented in DRW

  // Dimensions

  void addDimAlign([[maybe_unused]] const DRW_DimAligned* dimAlign) override {
    countOfDimAlign++;
  }

  void addDimAngular([[maybe_unused]] const DRW_DimAngular* dimAngular) override {
    countOfDimAngular++;
  }
  void addDimAngular3P([[maybe_unused]] const DRW_DimAngular3p* dimAngular3P) override {
    countOfDimAngular3P++;
  }

  void addDimLinear([[maybe_unused]] const DRW_DimLinear* dimLinear) override {
    countOfDimLinear++;
  }

  void addDimOrdinate([[maybe_unused]] const DRW_DimOrdinate* dimOrdinate) override {
    countOfDimOrdinate++;
  }

  void addDimRadial([[maybe_unused]] const DRW_DimRadial* dimRadial) override {
    countOfDimRadial++;
  }

  void addDimDiametric([[maybe_unused]] const DRW_DimDiametric* dimDiametric) override {
    countOfDimDiametric++;
  }
  
  void addEllipse(const DRW_Ellipse& ellipse) override { 
    countOfEllipse++; 
    if (inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"DRW_Interface::addCircle - block <%s>\n", blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addCircle - entities section\n");
    }
    ConvertEllipseEntity(ellipse, m_document);

  }
  void addHatch([[maybe_unused]] const DRW_Hatch* hatch) override {
    countOfHatch++;
  }

  // HELIX not implemented in DRW
  void addImage([[maybe_unused]] const DRW_Image* image) override {
    countOfImage++;
  }
  
  void addInsert(const DRW_Insert& insert) override { 
    countOfInsert++; 
    if (inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"DRW_Interface::addLine - block <%s>\n", blockName.c_str());
      ConvertInsertEntity(insert, m_document);
    } else {
      ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addLine - entities section\n");
      ConvertInsertEntity(insert, m_document);
    }
  }
  void addLeader(const DRW_Leader* leader) override { (void)leader; }
  // LIGHT not implemented in DRW  
  
  void addLine(const DRW_Line& line) override {
    countOfLine++;
    if (inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"DRW_Interface::addLine - block <%s>\n", blockName.c_str());
      ConvertLineEntity(line, m_document);
    } else {
      ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addLine - entities section\n");
      ConvertLineEntity(line, m_document);
    }
  }

  void addLWPolyline(const DRW_LWPolyline& lwPolyline) override {
    countOfLWPolyline++;
    if (inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"DRW_Interface::addLine - block <%s>\n", blockName.c_str());
      ConvertLWPolylineEntity(lwPolyline, m_document);
    } else {
      ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addLine - entities section\n");
      ConvertLWPolylineEntity(lwPolyline, m_document);
    }
  }
  // MESH not implemented in DRW
  // MLeader not implemented in DRW
  // MLINE not implemented in DRW

  void addMText(const DRW_MText& /* mText */) override { countOfMText++; }

  // OLEFRAME not implemented in DRW
  // OLE2FRAME not implemented in DRW

  void addPoint(const DRW_Point& point) override {
    countOfPoint++;
    if (inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"DRW_Interface::addPoint - block <%s>\n", blockName.c_str());
      ConvertPointEntity(point, m_document);
    } else {
      ATLTRACE2(traceGeneral, 3, L"DRW_Interface::addPoint - entities section\n");
      ConvertPointEntity(point, m_document);
    }
  }

  void addPolyline(const DRW_Polyline& /* polyline */) override { countOfPolyline++; }
  void addRay(const DRW_Ray& /* ray */) override { countOfRay++; }
  // REGION not implemented in DRW
  // SECTION not implemented in DRW
  // SEQEND not implemented in DRW
  // SHAPE not implemented in DRW

  void addSolid(const DRW_Solid& /* solid */) override { countOfSolid++; }
  void addSpline(const DRW_Spline* spline) override {
    (void)spline;
    
    countOfSpline++;
  }
  // SUN not implemented in DRW
  // SURFACE not implemented in DRW
  // TABLE not implemented in DRW
  void addText(const DRW_Text& /* text */) override { countOfText++; }
  // TOLERANCE not implemented in DRW
  void addTrace(const DRW_Trace& /* trace */) override { countOfTrace++; }
  // UNDERLAY not implemented in DRW
  // VERTEX not implemented in DRW
  void addViewport(const DRW_Viewport& /* viewport */) override { countOfViewport++; }
  // WIPEOUT not implemented in DRW
  void addXline(const DRW_Xline& /* Xline */) override { countOfXline++; }

  // Others
  void addComment(const char* comment) override { ATLTRACE2(traceGeneral, 2, L"DRW_Interface::addComment(%s)\n", comment); }
  void linkImage(const DRW_ImageDef* imageDefinition) override { (void)imageDefinition; }
  void addKnot(const DRW_Entity& /* knot */) override { countOfKnot++; }

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

  EoDbBlock* ConvertBlock(const DRW_Block& block, AeSysDoc* document);
  void ConvertBlockSet(const int handle, AeSysDoc* document);
  void ConvertBlockEnd(AeSysDoc* document);

  void AddToDocument(EoDbPrimitive* primitive, AeSysDoc* document);

  void ConvertArcEntity(const DRW_Arc& arc, AeSysDoc* document);
  void ConvertCircleEntity(const DRW_Circle& circle, AeSysDoc* document);
  void ConvertEllipseEntity(const DRW_Ellipse& ellipse, AeSysDoc* document);
  void ConvertInsertEntity(const DRW_Insert& insert, AeSysDoc* document);
  void ConvertLineEntity(const DRW_Line& line, AeSysDoc* document);
  void ConvertLWPolylineEntity(const DRW_LWPolyline& lwPolyline, AeSysDoc* document);
  void ConvertPointEntity(const DRW_Point& point, AeSysDoc* document);

 private:
  AeSysDoc* m_document{};
  std::wstring blockName{};
  bool inBlockDefinition{};
  EoDbBlock* currentOpenBlockDefinition{};

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