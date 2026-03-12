#pragma once

#include <algorithm>
#include <string>

#include "EoDxfEntities.h"
#include "EoDxfHeader.h"
#include "EoDxfInterface.h"

class EoDxfReader;

class EoDxfRead {
 public:
  EoDxfRead(std::wstring_view name);
  EoDxfRead(const char* name);
  ~EoDxfRead();

  /** @brief Reads a DXF file and populates the provided EoDxfInterface with the parsed data.
   *
   * This method processes the DXF file specified in the constructor, parsing its contents and invoking callbacks
   * on the provided EoDxfInterface for each entity, table entry, and header variable encountered. The `ext` parameter
   * controls whether extrusion should be applied when converting 3D entities to 2D representations.
   *
   * @param interface_ Pointer to a EoDxfInterface implementation that will receive callbacks for parsed entities and
   * data.
   * @param ext Boolean flag indicating whether to apply extrusion when converting 3D entities to 2D (true) or not
   * (false).
   * @return true if the file was successfully read and processed; false if an error occurred during reading or parsing.
   */
  bool Read(EoDxfInterface* interface_, bool ext);
  void SetBinary(bool binaryFile) { m_binaryFile = binaryFile; }

  /** @brief Sets the number of parts to use when rendering an ellipse as a polyline.
   * This method allows you to specify how many segments (parts) should be used to approximate an ellipse when it is
   * rendered as a polyline. The value is clamped between 1 and 1024 to ensure a reasonable level of detail without
   * excessive computational overhead.
   * @param parts The number of parts (segments) to use for rendering the ellipse. Must be between 1 and 1024.
   */
  void SetEllipseParts(int parts) { m_ellipseParts = std::clamp(parts, 1, 1024); }

 private:
  /** @brief Used by Read() to parse the file.  */
  [[nodiscard]] bool ProcessDxf();

  /** @brief Used by Read() to parse the HEADER section of the file.  */
  bool ProcessHeader();

  /** @brief Used by Read() to parse the CLASSES section of the file.  */
  bool ProcessClasses();

  /** @brief Used by Read() to parse the TABLES section of the file.  */
  bool ProcessTables();

  /** @brief Used by Read() to parse the BLOCKS section of the file.  */
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
  bool ProcessVertex(EoDxfPolyline* polyline);
  bool ProcessText();
  bool ProcessMText();
  bool ProcessHatch();
  bool ProcessSpline();
  bool Process3dFace();
  bool ProcessViewport();
  bool ProcessImage();
  bool ProcessImageDef();
  bool ProcessDimension();
  bool ProcessLeader();
  bool ProcessMLeader();

 private:
  EoDxfHeader m_header;
  std::wstring m_fileName;
  std::wstring m_codePage;
  std::wstring m_nextEntity;
  EoDxfReader* m_reader;
  EoDxfInterface* m_interface{};
  int m_ellipseParts;
  bool m_applyExtrusion;
  bool m_binaryFile{};
};
