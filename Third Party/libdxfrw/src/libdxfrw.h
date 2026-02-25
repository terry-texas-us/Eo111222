#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "drw_interface.h"
#include "drw_objects.h"

class dxfReader;
class dxfWriter;

class dxfRW {
 public:
  dxfRW(const char* name);
  ~dxfRW();
  void SetDebug(DRW::DebugTraceLevel traceLevel);

  /** @brief Reads a DXF file and populates the provided DRW_Interface with the parsed data.
   *
   * This method processes the DXF file specified in the constructor, parsing its contents and invoking callbacks
   * on the provided DRW_Interface for each entity, table entry, and header variable encountered. The `ext` parameter
   * controls whether extrusion should be applied when converting 3D entities to 2D representations.
   *
   * @param interface_ Pointer to a DRW_Interface implementation that will receive callbacks for parsed entities and data.
   * @param ext Boolean flag indicating whether to apply extrusion when converting 3D entities to 2D (true) or not (false).
   * @return true if the file was successfully read and processed; false if an error occurred during reading or parsing.
   */
  bool Read(DRW_Interface* interface_, bool ext);
  void SetBinary(bool binaryFile) { m_binaryFile = binaryFile; }

  /** @brief Writes the current state of the DRW_Interface to a DXF file in the specified version and format (binary or ASCII).
   *
   * This method generates a DXF file based on the data provided through the DRW_Interface. It writes the necessary sections
   * (HEADER, CLASSES, TABLES, BLOCKS, ENTITIES, OBJECTS) according to the specified DXF version. The `binaryFile` parameter
   * determines whether the output should be in binary format (DXB) or ASCII format (DXF). The method handles writing of entities,
   * tables, and other components as required by the DXF specification for the given version.
   *
   * @param interface_ Pointer to a DRW_Interface implementation that provides access to the data to be written to the DXF file.
   * @param version The version of the DXF format to use for writing (e.g., AC1009, AC1015, etc.).
   * @param binaryFile Boolean flag indicating whether to write in binary format (true) or ASCII format (false).
   * @return true if the file was successfully written; false if an error occurred during writing.
   */
  bool Write(DRW_Interface* interface_, DRW::Version version, bool binaryFile);
  bool WriteLineType(DRW_LType* ent);
  bool WriteLayer(DRW_Layer* ent);
  bool WriteDimstyle(DRW_Dimstyle* ent);
  bool WriteTextstyle(DRW_Textstyle* ent);
  bool WriteVport(DRW_Vport* ent);
  bool WriteAppId(DRW_AppId* ent);
  bool WritePoint(DRW_Point* ent);
  bool WriteLine(DRW_Line* ent);
  bool WriteRay(DRW_Ray* ent);
  bool WriteXline(DRW_Xline* ent);
  bool WriteCircle(DRW_Circle* ent);
  bool WriteArc(DRW_Arc* ent);
  bool WriteEllipse(DRW_Ellipse* ent);
  bool WriteTrace(DRW_Trace* ent);
  bool WriteSolid(DRW_Solid* ent);
  bool Write3dface(DRW_3Dface* ent);
  bool WriteLWPolyline(DRW_LWPolyline* ent);
  bool WritePolyline(DRW_Polyline* ent);
  bool WriteSpline(DRW_Spline* ent);
  bool WriteBlockRecord(std::string name);
  bool WriteBlock(DRW_Block* ent);
  bool WriteInsert(DRW_Insert* ent);
  bool WriteMText(DRW_MText* ent);
  bool WriteText(DRW_Text* ent);
  bool WriteHatch(DRW_Hatch* ent);
  bool WriteViewport(DRW_Viewport* ent);
  DRW_ImageDef* WriteImage(DRW_Image* ent, std::string name);
  bool WriteLeader(DRW_Leader* ent);
  bool WriteDimension(DRW_Dimension* ent);
  /** @brief Sets the number of parts to use when rendering an ellipse as a polyline.
   * This method allows you to specify how many segments (parts) should be used to approximate an ellipse when it is rendered as a polyline. The value is clamped between 1 and 1024 to ensure a reasonable level of detail without excessive computational overhead.
   * @param parts The number of parts (segments) to use for rendering the ellipse. Must be between 1 and 1024.
   */
  void SetEllipseParts(int parts) { m_ellipseParts = std::clamp(parts, 1, 1024); }  

 private:
  /** @brief Used by read() to parse the file.  */
  [[nodiscard]] bool ProcessDxf();

  /** @brief Used by read() to parse the HEADER section of the file.  */
  bool ProcessHeader();

  /** @brief Used by read() to parse the CLASSES section of the file.  */
  bool ProcessClasses();

  /** @brief Used by read() to parse the TABLES section of the file.  */
  bool ProcessTables();

  /** @brief Used by read() to parse the BLOCKS section of the file.  */
  bool ProcessBlocks();

  bool ProcessBlock();
  bool ProcessEntities(bool isblock);
  bool ProcessObjects();

  bool ProcessLType();
  bool ProcessLayer();
  bool ProcessDimStyle();
  bool ProcessTextStyle();
  bool ProcessVports();
  bool ProcessAppId();

  bool ProcessPoint();
  bool ProcessLine();
  bool ProcessRay();
  bool ProcessXline();
  bool ProcessCircle();
  bool ProcessArc();
  bool ProcessEllipse();
  bool ProcessTrace();
  bool ProcessSolid();
  bool ProcessInsert();
  bool ProcessLWPolyline();
  bool ProcessPolyline();
  bool ProcessVertex(DRW_Polyline* pl);
  bool ProcessText();
  bool ProcessMText();
  bool ProcessHatch();
  bool ProcessSpline();
  bool Process3dface();
  bool ProcessViewport();
  bool ProcessImage();
  bool ProcessImageDef();
  bool ProcessDimension();
  bool ProcessLeader();

  bool WriteEntity(DRW_Entity* ent);
  bool WriteTables();
  bool WriteBlocks();
  bool WriteObjects();
  bool WriteExtData(const std::vector<DRW_Variant*>& ed);

  /** @brief Convert integer to hex string
   * @param n integer number
   * @return hex string
   */
  std::string ToHexString(int n);

 private:
  DRW_Header m_header;
  std::string m_fileName;
  std::string m_codePage;
  std::string m_nextEntity;
  std::vector<DRW_ImageDef*> m_imageDef;  // list of image definitions to write at the end of file
  std::map<std::string, int> m_blockMap;
  dxfReader* m_reader;
  dxfWriter* m_writer;
  DRW_Interface* m_interface{};
  int m_entityCount{};
  int m_ellipseParts;  // number of parts when rendering ellipse as polyline
  DRW::Version m_version{};
  int m_currentHandle{};
  bool wlayer0{};
  bool m_standardDimensionStyle{};
  bool m_applyExtrusion;
  bool m_writingBlock{};
  bool m_binaryFile{};
};