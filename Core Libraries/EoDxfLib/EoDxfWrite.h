#pragma once

#include <map>
#include <string>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfInterface.h"
#include "EoDxfMLeader.h"
#include "EoDxfObjects.h"

class EoDxfWriter;

class EoDxfWrite {
 public:
  EoDxfWrite(const char* name);
  ~EoDxfWrite();

  /** @brief Writes the current state of the EoDxfInterface to a DXF file in the specified version and format (binary or
   * ASCII).
   *
   * This method generates a DXF file based on the data provided through the EoDxfInterface. It writes the necessary
   * sections (HEADER, CLASSES, TABLES, BLOCKS, ENTITIES, OBJECTS) according to the specified DXF version. The
   * `binaryFile` parameter determines whether the output should be in binary format (DXB) or ASCII format (DXF). The
   * method handles writing of entities, tables, and other components as required by the DXF specification for the given
   * version.
   *
   * @param interface_ Pointer to a EoDxfInterface implementation that provides access to the data to be written to the
   * DXF file.
   * @param version The version of the DXF format to use for writing (e.g., AC1009, AC1015, etc.).
   * @param binaryFile Boolean flag indicating whether to write in binary format (true) or ASCII format (false).
   * @return true if the file was successfully written; false if an error occurred during writing.
   */
  bool Write(EoDxfInterface* interface_, EoDxf::Version version, bool binaryFile);
  bool WriteLinetype(EoDxfLinetype* linetype);
  bool WriteLayer(EoDxfLayer* layer);
  bool WriteDimStyle(EoDxfDimensionStyle* dimensionStyle);
  bool WriteTextstyle(EoDxfTextStyle* textStyle);
  bool WriteVport(EoDxfViewport* viewport);
  bool WriteAppId(EoDxfAppId* appId);
  bool WritePoint(EoDxfPoint* point);
  bool WriteLine(EoDxfLine* ent);

  /** @brief Writes a RAY entity to the DXF file (R13+).
   *
   * A RAY is defined by a starting point and a direction vector. The direction vector is derived from the second point
   * of the RAY, which is normalized to ensure it represents only the direction without affecting the length. The method
   * writes the necessary DXF group codes and values to represent the RAY entity correctly in the DXF format.
   *
   * @param ray A pointer to the EoDxfRay object containing the RAY's properties.
   * @return true if the RAY was successfully written; otherwise, false.
   */
  bool WriteRay(EoDxfRay* ray);

  /** @brief Writes an XLINE entity to the DXF file.
   *
   * An XLINE (construction line) is defined by a starting point and a direction vector. The direction vector is derived
   * from the second point of the XLINE, which is normalized to ensure it represents only the direction without
   * affecting the length. The method writes the necessary DXF group codes and values to represent the XLINE entity
   * correctly in the DXF format.
   *
   * @param xline A pointer to the EoDxfXline object containing the XLINE's properties.
   * @return true if the XLINE was successfully written; otherwise, false.
   */
  bool WriteXline(EoDxfXline* xline);
  bool WriteCircle(EoDxfCircle* circle);
  bool WriteArc(EoDxfArc* arc);
  bool WriteEllipse(EoDxfEllipse* ellipse);
  bool WriteTrace(EoDxfTrace* trace);
  bool WriteSolid(EoDxfSolid* solid);
  bool Write3dFace(EoDxf3dFace* face);
  bool WriteLWPolyline(EoDxfLwPolyline* polyline);
  bool WritePolyline(EoDxfPolyline* polyline);
  bool WriteSpline(EoDxfSpline* spline);
  bool WriteBlockRecord(std::string name);
  bool WriteBlock(EoDxfBlock* block);
  bool WriteInsert(EoDxfInsert* blockReference);
  bool WriteMText(EoDxfMText* mText);
  bool WriteText(EoDxfText* text);
  bool WriteHatch(EoDxfHatch* hatch);
  bool WriteViewport(EoDxfViewPort* viewport);
  EoDxfImageDefinition* WriteImage(EoDxfImage* image, std::string name);
  bool WriteLeader(EoDxfLeader* leader);
  
  /** @brief Writes an MLEADER entity to the DXF file.
   *
   * This method handles the writing of an MLEADER (multileader) entity, which represents a leader with multiple segments
   * and associated content (such as text or blocks) in a DXF file. The method writes the necessary group codes and
   * values to represent the MLEADER correctly according to the DXF specification. It also takes into account the
   * specific properties of the MLEADER, such as its leader type, line color, line type, content type, and other
   * attributes that define its appearance and behavior in the drawing.
   *
   * @param mLeader A pointer to the EoDxfMLeader object containing the properties of the MLEADER to be written.
   * @return true if the MLEADER was successfully written; otherwise, false.
   */
  bool WriteMLeader(EoDxfMLeader* mLeader);
  
  /** @brief Writes a DIMENSION entity to the DXF file.
   *
   * This method handles the writing of a DIMENSION entity, which represents various types of dimensions (linear, aligned,
   * angular, etc.) in a DXF file. The method writes the necessary group codes and values to represent the dimension
   * correctly according to the DXF specification. It also takes into account the specific properties of the dimension,
   * such as its type, measurement points, and associated styles.
   *
   * @param dimension A pointer to the EoDxfDimension object containing the properties of the dimension to be written.
   * @return true if the dimension was successfully written; otherwise, false.
   */
  bool WriteDimension(EoDxfDimension* dimension);

 private:
  bool WriteEntity(EoDxfEntity* ent);
  bool WriteTables();
  bool WriteBlocks();
  bool WriteObjects();
  bool WriteExtData(const std::vector<EoDxfGroupCodeValuesVariant*>& extensionData);

  /** @brief Convert integer to hex string
   * @param hexValue integer number
   * @return hex string
   */
  std::string ToHexString(uint64_t hexValue);

 private:
  std::string m_fileName;
  std::vector<EoDxfImageDefinition*> m_imageDef;  // list of image definitions to write at the end of file
  std::map<std::string, int> m_blockMap;
  EoDxfWriter* m_writer;
  EoDxfInterface* m_interface{};
  int m_entityCount{};
  EoDxf::Version m_version{};
  std::uint64_t m_currentHandle{};
  bool wlayer0{};
  bool m_standardDimensionStyle{};
  bool m_writingBlock{};
  bool m_binaryFile{};
};
