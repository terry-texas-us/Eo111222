#pragma once

#include <filesystem>
#include <ios>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfHatch.h"
#include "EoDxfInterface.h"
#include "EoDxfMLeader.h"
#include "EoDxfObjects.h"
#include "EoDxfWriter.h"

class EoDxfWrite {
 public:
  explicit EoDxfWrite(const std::filesystem::path& fileName);
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
  bool WriteClass(EoDxfClass* class_);
  bool WriteLinetype(EoDxfLinetype* linetype);
  bool WriteLayer(EoDxfLayer* layer);
  bool WriteDimStyle(EoDxfDimensionStyle* dimensionStyle);
  bool WriteTextstyle(EoDxfTextStyle* textStyle);
  bool WriteVport(EoDxfVPort* viewport);
  bool WriteAppId(EoDxfAppId* appId);

  void WriteCodePoint3d(int code, const EoDxfGeometryBase3d& point);
  void WriteCodeVector3d(int code, const EoDxfGeometryBase3d& vector);

  /** @brief Writes the extrusion direction of a graphic entity to the DXF file if it is not the default normal vector.
   *
   * This method checks if the extrusion direction of the given graphic entity is effectively different from the default
   * normal vector (0.0, 0.0, 1.0) within a specified tolerance. If it is different, it writes the corresponding DXF
   * group codes (210, 220, 230) and their values (x, y, z components of the extrusion direction) to the output stream.
   * This is necessary for entities that have an extrusion direction defined, as it affects how they are rendered in 3D
   * space.
   *
   * @param entity A reference to the EoDxfGraphic entity whose extrusion direction is to be written.
   */
  void WriteExtrusionDirection(const EoDxfGraphic& entity);

  /** @brief Writes the thickness of a graphic entity to the DXF file if it is greater than a specified geometric
   * tolerance.
   *
   * This method checks if the thickness of the given graphic entity is effectively greater than a defined geometric
   * tolerance (EoDxf::geometricTolerance). If the thickness is greater than this tolerance, it writes the corresponding
   * DXF group code (39) and its value (the thickness) to the output stream. This is important for entities that have a
   * non-default thickness, as it affects how they are rendered in 3D space.
   *
   * @param entity A reference to the EoDxfGraphic entity whose thickness is to be written.
   */
  void WriteThickness(const EoDxfGraphic& entity);

  bool Write3dFace(EoDxf3dFace* _3dFace);

  /** @brief Writes an ACAD_PROXY_ENTITY to the DXF file.
   *
   *  Proxy entities preserve custom entity data for round-trip fidelity when the defining ObjectARX application is
   *  not loaded. The method writes the class IDs, graphics and entity binary chunk data (code 310), object-ID handle
   *  references, and the original data format flag.
   *
   *  @param proxyEntity A pointer to the EoDxfAcadProxyEntity object containing the proxy entity's properties.
   *  @return true if the entity was successfully written; otherwise, false.
   */
  bool WriteAcadProxyEntity(EoDxfAcadProxyEntity* proxyEntity);

  bool WriteArc(EoDxfArc* arc);
  bool WriteCircle(EoDxfCircle* circle);
 
  /** @brief Writes a DIMENSION entity to the DXF file.
   *
   * This method handles the writing of a DIMENSION entity, which represents various types of dimensions (linear,
   * aligned, angular, etc.) in a DXF file. The method writes the necessary group codes and values to represent the
   * dimension correctly according to the DXF specification. It also takes into account the specific properties of the
   * dimension, such as its type, measurement points, and associated styles.
   *
   * @param dimension A pointer to the EoDxfDimension object containing the properties of the dimension to be written.
   * @return true if the dimension was successfully written; otherwise, false.
   */
  bool WriteDimension(EoDxfDimension* dimension);

  bool WriteEllipse(EoDxfEllipse* ellipse);
  bool WriteHatch(EoDxfHatch* hatch);
  bool WriteInsert(EoDxfInsert* blockReference);
  bool WriteLeader(EoDxfLeader* leader);
  bool WriteLine(EoDxfLine* line);
  bool WriteLWPolyline(EoDxfLwPolyline* polyline);
 
 /** @brief Writes an MLEADER entity to the DXF file.
   *
   * This method handles the writing of an MLEADER (multileader) entity, which represents a leader with multiple
   * segments and associated content (such as text or blocks) in a DXF file. The method writes the necessary group codes
   * and values to represent the MLEADER correctly according to the DXF specification. It also takes into account the
   * specific properties of the MLEADER, such as its leader type, line color, line type, content type, and other
   * attributes that define its appearance and behavior in the drawing.
   *
   * @param mLeader A pointer to the EoDxfMLeader object containing the properties of the MLEADER to be written.
   * @return true if the MLEADER was successfully written; otherwise, false.
   */
  bool WriteMLeader(EoDxfMLeader* mLeader);

  bool WriteMText(EoDxfMText* mText);
  bool WritePoint(EoDxfPoint* point);
  bool WritePolyline(EoDxfPolyline* polyline);

  /** @brief Writes a RAY entity to the DXF file (AC1012+).
   *
   * A RAY is defined by a starting point and a direction vector. The direction vector is derived from the second point
   * of the RAY, which is normalized to ensure it represents only the direction without affecting the length. The method
   * writes the necessary DXF group codes and values to represent the RAY entity correctly in the DXF format.
   *
   * @param ray A pointer to the EoDxfRay object containing the RAY's properties.
   * @return true if the RAY was successfully written; otherwise, false.
   */
  bool WriteRay(EoDxfRay* ray);

  bool WriteSolid(EoDxfSolid* solid);
  bool WriteSpline(EoDxfSpline* spline);
  bool WriteText(EoDxfText* text);
  bool WriteTrace(EoDxfTrace* trace);
  bool WriteViewport(EoDxfViewport* viewport);

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

  bool WriteBlockRecord(std::wstring_view name);
  bool WriteBlock(EoDxfBlock* block);
  void AddImageDefinition(const EoDxfImageDefinition& imageDefinition);
  bool WriteUnsupportedObject(const EoDxfUnsupportedObject& objectData);
  EoDxfImageDefinition* WriteImage(EoDxfImage* image, std::wstring_view name);
 
 private:
  bool TrackWriteResult(bool isOk) noexcept {
    m_writeOk = m_writeOk && isOk;
    return m_writeOk;
  }
  bool TrackStreamState(const std::ios& stream) noexcept { return TrackWriteResult(stream.good()); }
  bool WriteCodeString(int code, std::wstring_view text) {
    return TrackWriteResult(m_writer != nullptr && m_writer->WriteWideString(code, text));
  }
  bool WriteCodeWideString(int code, std::wstring_view text) {
    return TrackWriteResult(m_writer != nullptr && m_writer->WriteWideString(code, text));
  }
  bool WriteCodeInt16(int code, std::int16_t value) {
    return TrackWriteResult(m_writer != nullptr && m_writer->WriteInt16(code, value));
  }
  bool WriteCodeInt32(int code, std::int32_t value) {
    return TrackWriteResult(m_writer != nullptr && m_writer->WriteInt32(code, value));
  }
  bool WriteCodeInt64(int code, std::int64_t value) {
    return TrackWriteResult(m_writer != nullptr && m_writer->WriteInt64(code, value));
  }
  bool WriteCodeDouble(int code, double value) {
    return TrackWriteResult(m_writer != nullptr && m_writer->WriteDouble(code, value));
  }
  bool WriteCodeBool(int code, bool value) { return TrackWriteResult(m_writer != nullptr && m_writer->WriteBool(code, value)); }

  bool WriteEntity(EoDxfGraphic* entity);

  bool WriteTables();
  bool WriteBlocks();
  bool WriteObjects();
  bool WriteAppData(const std::list<std::list<EoDxfGroupCodeValuesVariant>>& appData);
  bool WriteExtData(const std::vector<std::unique_ptr<EoDxfGroupCodeValuesVariant>>& extensionData);
  bool WriteVariantValue(const EoDxfGroupCodeValuesVariant& value);

  /** @brief Convert integer to hex string
   * @param hexValue integer number
   * @return hex string
   */
  std::wstring ToHexString(uint64_t hexValue);

 private:
  std::wstring m_fileName;
  std::vector<EoDxfImageDefinition*> m_imageDef;  // list of image definitions to write at the end of file
  std::map<std::wstring, int> m_blockMap;
  EoDxfWriter* m_writer;
  EoDxfInterface* m_interface{};
  int m_entityCount{};
  EoDxf::Version m_version{};
  std::uint64_t m_currentHandle{};
  bool wlayer0{};
  bool m_standardDimensionStyle{};
  bool m_writingBlock{};
  bool m_binaryFile{};
  bool m_writeOk{true};
};
