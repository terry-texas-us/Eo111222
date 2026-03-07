#include <algorithm>
#include <cctype>
#include <fstream>
#include <ios>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "EoDxfLib.h"

#include "EoDxfBase.h"
#include "EoDxfClasses.h"
#include "EoDxfEntities.h"
#include "EoDxfHeader.h"
#include "EoDxfInterface.h"
#include "EoDxfObjects.h"
#include "intern/EoDxfReader.h"
#include "intern/EoDxfWriter.h"

namespace {
constexpr auto FIRSTHANDLE{48};
}  // namespace

dxfRW::dxfRW(const char* name) {
  m_fileName = name;
  m_reader = nullptr;
  m_writer = nullptr;
  m_applyExtrusion = false;
  m_ellipseParts = 128;  // parts munber when convert ellipse to polyline
}
dxfRW::~dxfRW() {
  if (m_reader != nullptr) { delete m_reader; }
  if (m_writer != nullptr) { delete m_writer; }
  for (std::vector<EoDxfImageDefinition*>::iterator it = m_imageDef.begin(); it != m_imageDef.end(); ++it) { delete *it; }

  m_imageDef.clear();
}

bool dxfRW::Read(EoDxfInterface* interface_, bool ext) {
  drw_assert(fileName.empty() == false);
  bool isOk = false;
  m_applyExtrusion = ext;
  std::ifstream filestr;
  if (interface_ == nullptr) { return isOk; }
  filestr.open(m_fileName.c_str(), std::ios_base::in | std::ios::binary);
  if (!filestr.is_open()) { return isOk; }
  if (!filestr.good()) { return isOk; }

  char line[22]{};
  filestr.read(line, 22);
  filestr.close();

  // Declare sentinel indicating a binary DXF(DXB) file in AutoCAD, regardless
  // of the file extension, is a 22-byte sequence at the beginning of the file :
  // the ASCII string "AutoCAD Binary DXF" (18 bytes) followed by
  // a carriage return (CR, 0x0D), line feed(LF, 0x0A), substitute
  // character(SUB, 0x1A), and null byte(NUL, 0x00).
  char line2[22] = "AutoCAD Binary DXF\r\n";
  line2[20] = static_cast<char>(26);
  line2[21] = '\0';

  m_interface = interface_;
  if (strcmp(line, line2) == 0) {
    filestr.open(m_fileName.c_str(), std::ios_base::in | std::ios::binary);
    m_binaryFile = true;
    // skip sentinel
    filestr.seekg(22, std::ios::beg);
    m_reader = new dxfReaderBinary(&filestr);
  } else {
    m_binaryFile = false;
    filestr.open(m_fileName.c_str(), std::ios_base::in);
    m_reader = new dxfReaderAscii(&filestr);
  }
  isOk = ProcessDxf();
  filestr.close();
  delete m_reader;
  m_reader = nullptr;
  return isOk;
}

bool dxfRW::Write(EoDxfInterface* interface_, EoDxf::Version version, bool binaryFile) {
  bool isOk = false;
  std::ofstream filestr;
  m_version = version;
  m_binaryFile = binaryFile;
  m_interface = interface_;
  if (m_binaryFile) {
    filestr.open(m_fileName.c_str(), std::ios_base::out | std::ios::binary | std::ios::trunc);
    // write sentinel
    filestr << "AutoCAD Binary DXF\r\n" << (char)26 << '\0';
    m_writer = new dxfWriterBinary(&filestr);
  } else {
    filestr.open(m_fileName.c_str(), std::ios_base::out | std::ios::trunc);
    m_writer = new dxfWriterAscii(&filestr);
    std::string comm = std::string("dxfrw ") + std::string(DRW_VERSION);
    m_writer->WriteString(999, comm);
  }
  EoDxfHeader header;
  m_interface->writeHeader(header);
  m_writer->WriteString(0, "SECTION");
  m_entityCount = FIRSTHANDLE;
  header.Write(m_writer, m_version);
  m_writer->WriteString(0, "ENDSEC");

  m_writer->WriteString(0, "SECTION");
  m_writer->WriteString(2, "CLASSES");
  m_writer->WriteString(0, "ENDSEC");

  m_writer->WriteString(0, "SECTION");
  m_writer->WriteString(2, "TABLES");
  WriteTables();
  m_writer->WriteString(0, "ENDSEC");
  m_writer->WriteString(0, "SECTION");
  m_writer->WriteString(2, "BLOCKS");
  WriteBlocks();
  m_writer->WriteString(0, "ENDSEC");

  m_writer->WriteString(0, "SECTION");
  m_writer->WriteString(2, "ENTITIES");
  m_interface->writeEntities();
  m_writer->WriteString(0, "ENDSEC");

  m_writer->WriteString(0, "SECTION");
  m_writer->WriteString(2, "OBJECTS");
  WriteObjects();
  m_writer->WriteString(0, "ENDSEC");

  m_writer->WriteString(0, "EOF");
  filestr.flush();
  filestr.close();
  isOk = true;
  delete m_writer;
  m_writer = nullptr;
  return isOk;
}

bool dxfRW::WriteEntity(EoDxfEntity* entity) {
  entity->m_handle = ++m_entityCount;
  m_writer->WriteString(5, ToHexString(entity->m_handle));
  m_writer->WriteString(100, "AcDbEntity");
  if (entity->m_space == 1) { m_writer->WriteInt16(67, 1); }

  m_writer->WriteUtf8String(8, entity->m_layer);
  m_writer->WriteUtf8String(6, entity->m_lineType);

  m_writer->WriteInt16(62, entity->m_color);
  if (m_version > EoDxf::Version::AC1015 && entity->m_color24 >= 0) { m_writer->WriteInt32(420, entity->m_color24); }
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteInt16(370, DRW_LW_Conv::lineWidth2dxfInt(entity->m_lineWeight)); }
  return true;
}

bool dxfRW::WriteLinetype(EoDxfLinetype* lineType) {
  std::string strname = lineType->m_tableName;

  transform(strname.begin(), strname.end(), strname.begin(), ::toupper);
  // do not write linetypes handled by library
  if (strname == "BYLAYER" || strname == "BYBLOCK" || strname == "CONTINUOUS") { return true; }
  m_writer->WriteString(0, "LTYPE");

  m_writer->WriteString(5, ToHexString(++m_entityCount));
  m_writer->WriteString(330, "5");
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbLinetypeTableRecord");
  m_writer->WriteUtf8String(2, lineType->m_tableName);

  m_writer->WriteInt16(70, lineType->m_flagValues);
  m_writer->WriteUtf8String(3, lineType->desc);
  lineType->Update();
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, lineType->size);
  m_writer->WriteDouble(40, lineType->length);

  for (unsigned int i = 0; i < lineType->path.size(); i++) {
    m_writer->WriteDouble(49, lineType->path.at(i));
    m_writer->WriteInt16(74, 0);
  }
  return true;
}

bool dxfRW::WriteLayer(EoDxfLayer* layer) {
  m_writer->WriteString(0, "LAYER");
  if (!wlayer0 && layer->m_tableName == "0") {
    wlayer0 = true;
    m_writer->WriteString(5, "10");
  } else {
    m_writer->WriteString(5, ToHexString(++m_entityCount));
  }
  m_writer->WriteString(330, "2");

  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbLayerTableRecord");
  m_writer->WriteUtf8String(2, layer->m_tableName);

  m_writer->WriteInt16(70, layer->m_flagValues);
  m_writer->WriteInt16(62, layer->m_colorNumber);
  if (m_version > EoDxf::Version::AC1015 && layer->color24 >= 0) { m_writer->WriteInt32(420, layer->color24); }
  m_writer->WriteUtf8String(6, layer->m_linetypeName);
  if (!layer->m_plottingFlag) { m_writer->WriteBool(290, layer->m_plottingFlag); }
  m_writer->WriteInt16(370, DRW_LW_Conv::lineWidth2dxfInt(layer->m_lineweightEnumValue));
  m_writer->WriteString(390, "F");

  if (!layer->m_extensionData.empty()) { WriteExtData(layer->m_extensionData); }
  //    m_writer->WriteString(347, "10012");
  return true;
}

bool dxfRW::WriteTextstyle(EoDxfTextStyle* textStyle) {
  m_writer->WriteString(0, "STYLE");
  if (!m_standardDimensionStyle) {
    std::string name = textStyle->m_tableName;
    transform(name.begin(), name.end(), name.begin(), toupper);
    if (name == "STANDARD") { m_standardDimensionStyle = true; }
  }
  m_writer->WriteString(5, ToHexString(++m_entityCount));

  m_writer->WriteString(330, "2");

  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbTextStyleTableRecord");
  m_writer->WriteUtf8String(2, textStyle->m_tableName);

  m_writer->WriteInt16(70, textStyle->m_flagValues);
  m_writer->WriteDouble(40, textStyle->height);
  m_writer->WriteDouble(41, textStyle->width);
  m_writer->WriteDouble(50, textStyle->oblique);
  m_writer->WriteInt16(71, textStyle->genFlag);
  m_writer->WriteDouble(42, textStyle->lastHeight);

  m_writer->WriteUtf8String(3, textStyle->font);
  m_writer->WriteUtf8String(4, textStyle->bigFont);
  if (textStyle->fontFamily != 0) { m_writer->WriteInt32(1071, textStyle->fontFamily); }
  return true;
}

bool dxfRW::WriteVport(EoDxfViewport* viewport) {
  if (!m_standardDimensionStyle) {
    viewport->m_tableName = "*ACTIVE";
    m_standardDimensionStyle = true;
  }
  m_writer->WriteString(0, "VPORT");

  m_writer->WriteString(5, ToHexString(++m_entityCount));
  m_writer->WriteString(330, "2");
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbViewportTableRecord");
  m_writer->WriteUtf8String(2, viewport->m_tableName);

  m_writer->WriteInt16(70, viewport->m_flagValues);
  m_writer->WriteDouble(10, viewport->lowerLeft.x);
  m_writer->WriteDouble(20, viewport->lowerLeft.y);
  m_writer->WriteDouble(11, viewport->upperRight.x);
  m_writer->WriteDouble(21, viewport->upperRight.y);
  m_writer->WriteDouble(12, viewport->center.x);
  m_writer->WriteDouble(22, viewport->center.y);
  m_writer->WriteDouble(13, viewport->snapBase.x);
  m_writer->WriteDouble(23, viewport->snapBase.y);
  m_writer->WriteDouble(14, viewport->snapSpacing.x);
  m_writer->WriteDouble(24, viewport->snapSpacing.y);
  m_writer->WriteDouble(15, viewport->gridSpacing.x);
  m_writer->WriteDouble(25, viewport->gridSpacing.y);
  m_writer->WriteDouble(16, viewport->viewDir.x);
  m_writer->WriteDouble(26, viewport->viewDir.y);
  m_writer->WriteDouble(36, viewport->viewDir.z);
  m_writer->WriteDouble(17, viewport->viewTarget.x);
  m_writer->WriteDouble(27, viewport->viewTarget.y);
  m_writer->WriteDouble(37, viewport->viewTarget.z);
  m_writer->WriteDouble(40, viewport->height);
  m_writer->WriteDouble(41, viewport->ratio);
  m_writer->WriteDouble(42, viewport->lensHeight);
  m_writer->WriteDouble(43, viewport->frontClip);
  m_writer->WriteDouble(44, viewport->backClip);
  m_writer->WriteDouble(50, viewport->snapAngle);
  m_writer->WriteDouble(51, viewport->twistAngle);
  m_writer->WriteInt16(71, viewport->viewMode);
  m_writer->WriteInt16(72, viewport->circleZoom);
  m_writer->WriteInt16(73, viewport->fastZoom);
  m_writer->WriteInt16(74, viewport->ucsIcon);
  m_writer->WriteInt16(75, viewport->snap);
  m_writer->WriteInt16(76, viewport->grid);
  m_writer->WriteInt16(77, viewport->snapStyle);
  m_writer->WriteInt16(78, viewport->snapIsopair);
  if (m_version > EoDxf::Version::AC1014) {
    m_writer->WriteInt16(281, 0);
    m_writer->WriteInt16(65, 1);
    m_writer->WriteDouble(110, 0.0);
    m_writer->WriteDouble(120, 0.0);
    m_writer->WriteDouble(130, 0.0);
    m_writer->WriteDouble(111, 1.0);
    m_writer->WriteDouble(121, 0.0);
    m_writer->WriteDouble(131, 0.0);
    m_writer->WriteDouble(112, 0.0);
    m_writer->WriteDouble(122, 1.0);
    m_writer->WriteDouble(132, 0.0);
    m_writer->WriteInt16(79, 0);
    m_writer->WriteDouble(146, 0.0);
    if (m_version > EoDxf::Version::AC1018) {
      m_writer->WriteString(348, "10020");
      m_writer->WriteInt16(60, viewport->gridBehavior);
      m_writer->WriteInt16(61, 5);
      m_writer->WriteBool(292, 1);
      m_writer->WriteInt16(282, 1);
      m_writer->WriteDouble(141, 0.0);
      m_writer->WriteDouble(142, 0.0);
      m_writer->WriteInt16(63, 250);
      m_writer->WriteInt32(421, 3358443);
    }
  }
  return true;
}

bool dxfRW::WriteDimStyle(EoDxfDimensionStyle* dimensionStyle) {
  m_writer->WriteString(0, "DIMSTYLE");
  if (!m_standardDimensionStyle) {
    std::string name = dimensionStyle->m_tableName;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    if (name == "STANDARD") { m_standardDimensionStyle = true; }
  }
  m_writer->WriteString(105, ToHexString(++m_entityCount));

  m_writer->WriteString(330, "A");

  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbDimStyleTableRecord");
  m_writer->WriteUtf8String(2, dimensionStyle->m_tableName);

  m_writer->WriteInt16(70, dimensionStyle->m_flagValues);
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimpost.empty())) { m_writer->WriteUtf8String(3, dimensionStyle->dimpost); }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimapost.empty())) { m_writer->WriteUtf8String(4, dimensionStyle->dimapost); }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk.empty())) { m_writer->WriteUtf8String(5, dimensionStyle->dimblk); }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk1.empty())) { m_writer->WriteUtf8String(6, dimensionStyle->dimblk1); }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk2.empty())) { m_writer->WriteUtf8String(7, dimensionStyle->dimblk2); }
  m_writer->WriteDouble(40, dimensionStyle->dimscale);
  m_writer->WriteDouble(41, dimensionStyle->dimasz);
  m_writer->WriteDouble(42, dimensionStyle->dimexo);
  m_writer->WriteDouble(43, dimensionStyle->dimdli);
  m_writer->WriteDouble(44, dimensionStyle->dimexe);
  m_writer->WriteDouble(45, dimensionStyle->dimrnd);
  m_writer->WriteDouble(46, dimensionStyle->dimdle);
  m_writer->WriteDouble(47, dimensionStyle->dimtp);
  m_writer->WriteDouble(48, dimensionStyle->dimtm);
  if (m_version > EoDxf::Version::AC1018 || dimensionStyle->dimfxl != 0) { m_writer->WriteDouble(49, dimensionStyle->dimfxl); }
  m_writer->WriteDouble(140, dimensionStyle->dimtxt);
  m_writer->WriteDouble(141, dimensionStyle->dimcen);
  m_writer->WriteDouble(142, dimensionStyle->dimtsz);
  m_writer->WriteDouble(143, dimensionStyle->dimaltf);
  m_writer->WriteDouble(144, dimensionStyle->dimlfac);
  m_writer->WriteDouble(145, dimensionStyle->dimtvp);
  m_writer->WriteDouble(146, dimensionStyle->dimtfac);
  m_writer->WriteDouble(147, dimensionStyle->dimgap);
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteDouble(148, dimensionStyle->dimaltrnd); }
  m_writer->WriteInt16(71, dimensionStyle->dimtol);
  m_writer->WriteInt16(72, dimensionStyle->dimlim);
  m_writer->WriteInt16(73, dimensionStyle->dimtih);
  m_writer->WriteInt16(74, dimensionStyle->dimtoh);
  m_writer->WriteInt16(75, dimensionStyle->dimse1);
  m_writer->WriteInt16(76, dimensionStyle->dimse2);
  m_writer->WriteInt16(77, dimensionStyle->dimtad);
  m_writer->WriteInt16(78, dimensionStyle->dimzin);
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteInt16(79, dimensionStyle->dimazin); }
  m_writer->WriteInt16(170, dimensionStyle->dimalt);
  m_writer->WriteInt16(171, dimensionStyle->dimaltd);
  m_writer->WriteInt16(172, dimensionStyle->dimtofl);
  m_writer->WriteInt16(173, dimensionStyle->dimsah);
  m_writer->WriteInt16(174, dimensionStyle->dimtix);
  m_writer->WriteInt16(175, dimensionStyle->dimsoxd);
  m_writer->WriteInt16(176, dimensionStyle->dimclrd);
  m_writer->WriteInt16(177, dimensionStyle->dimclre);
  m_writer->WriteInt16(178, dimensionStyle->dimclrt);
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteInt16(179, dimensionStyle->dimadec); }

  if (m_version < EoDxf::Version::AC1015) { m_writer->WriteInt16(270, dimensionStyle->dimunit); }
  m_writer->WriteInt16(271, dimensionStyle->dimdec);
  m_writer->WriteInt16(272, dimensionStyle->dimtdec);
  m_writer->WriteInt16(273, dimensionStyle->dimaltu);
  m_writer->WriteInt16(274, dimensionStyle->dimalttd);
  m_writer->WriteInt16(275, dimensionStyle->dimaunit);

  if (m_version > EoDxf::Version::AC1014) {
    m_writer->WriteInt16(276, dimensionStyle->dimfrac);
    m_writer->WriteInt16(277, dimensionStyle->dimlunit);
    m_writer->WriteInt16(278, dimensionStyle->dimdsep);
    m_writer->WriteInt16(279, dimensionStyle->dimtmove);
  }

  m_writer->WriteInt16(280, dimensionStyle->dimjust);
  m_writer->WriteInt16(281, dimensionStyle->dimsd1);
  m_writer->WriteInt16(282, dimensionStyle->dimsd2);
  m_writer->WriteInt16(283, dimensionStyle->dimtolj);
  m_writer->WriteInt16(284, dimensionStyle->dimtzin);
  m_writer->WriteInt16(285, dimensionStyle->dimaltz);
  m_writer->WriteInt16(286, dimensionStyle->dimaltttz);
  if (m_version < EoDxf::Version::AC1015) { m_writer->WriteInt16(287, dimensionStyle->dimfit); }
  m_writer->WriteInt16(288, dimensionStyle->dimupt);

  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteInt16(289, dimensionStyle->dimatfit); }
  if (m_version > EoDxf::Version::AC1018 && dimensionStyle->dimfxlon != 0) { m_writer->WriteInt16(290, dimensionStyle->dimfxlon); }
  m_writer->WriteUtf8String(340, dimensionStyle->dimtxsty);
  if (m_version > EoDxf::Version::AC1014) {
    m_writer->WriteUtf8String(341, dimensionStyle->dimldrblk);
    m_writer->WriteInt16(371, dimensionStyle->dimlwd);
    m_writer->WriteInt16(372, dimensionStyle->dimlwe);
  }
  return true;
}

bool dxfRW::WriteAppId(EoDxfAppId* appId) {
  std::string strname = appId->m_tableName;
  transform(strname.begin(), strname.end(), strname.begin(), ::toupper);
  // do not write mandatory ACAD appId, handled by library
  if (strname == "ACAD") { return true; }
  m_writer->WriteString(0, "APPID");

  m_writer->WriteString(5, ToHexString(++m_entityCount));
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "9"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbRegAppTableRecord");
  m_writer->WriteUtf8String(2, appId->m_tableName);

  m_writer->WriteInt16(70, appId->m_flagValues);
  return true;
}

bool dxfRW::WritePoint(EoDxfPoint* point) {
  m_writer->WriteString(0, "POINT");
  WriteEntity(point);
  m_writer->WriteString(100, "AcDbPoint");
  m_writer->WriteDouble(10, point->m_firstPoint.x);
  m_writer->WriteDouble(20, point->m_firstPoint.y);
  if (point->m_firstPoint.z != 0.0) { m_writer->WriteDouble(30, point->m_firstPoint.z); }
  return true;
}

bool dxfRW::WriteLine(EoDxfLine* line) {
  m_writer->WriteString(0, "LINE");
  WriteEntity(line);
  m_writer->WriteString(100, "AcDbLine");
  m_writer->WriteDouble(10, line->m_firstPoint.x);
  m_writer->WriteDouble(20, line->m_firstPoint.y);
  if (line->m_firstPoint.z != 0.0 || line->m_secondPoint.z != 0.0) {
    m_writer->WriteDouble(30, line->m_firstPoint.z);
    m_writer->WriteDouble(11, line->m_secondPoint.x);
    m_writer->WriteDouble(21, line->m_secondPoint.y);
    m_writer->WriteDouble(31, line->m_secondPoint.z);
  } else {
    m_writer->WriteDouble(11, line->m_secondPoint.x);
    m_writer->WriteDouble(21, line->m_secondPoint.y);
  }
  return true;
}

bool dxfRW::WriteRay(EoDxfRay* ray) {
  m_writer->WriteString(0, "RAY");
  WriteEntity(ray);
  m_writer->WriteString(100, "AcDbRay");
  EoDxfGeometryBase3d crd = ray->m_secondPoint;
  crd.unitize();
  m_writer->WriteDouble(10, ray->m_firstPoint.x);
  m_writer->WriteDouble(20, ray->m_firstPoint.y);
  if (ray->m_firstPoint.z != 0.0 || ray->m_secondPoint.z != 0.0) {
    m_writer->WriteDouble(30, ray->m_firstPoint.z);
    m_writer->WriteDouble(11, crd.x);
    m_writer->WriteDouble(21, crd.y);
    m_writer->WriteDouble(31, crd.z);
  } else {
    m_writer->WriteDouble(11, crd.x);
    m_writer->WriteDouble(21, crd.y);
  }
  return true;
}

bool dxfRW::WriteXline(EoDxfXline* xline) {
  m_writer->WriteString(0, "XLINE");
  WriteEntity(xline);
  m_writer->WriteString(100, "AcDbXline");
  EoDxfGeometryBase3d crd = xline->m_secondPoint;
  crd.unitize();
  m_writer->WriteDouble(10, xline->m_firstPoint.x);
  m_writer->WriteDouble(20, xline->m_firstPoint.y);
  if (xline->m_firstPoint.z != 0.0 || xline->m_secondPoint.z != 0.0) {
    m_writer->WriteDouble(30, xline->m_firstPoint.z);
    m_writer->WriteDouble(11, crd.x);
    m_writer->WriteDouble(21, crd.y);
    m_writer->WriteDouble(31, crd.z);
  } else {
    m_writer->WriteDouble(11, crd.x);
    m_writer->WriteDouble(21, crd.y);
  }
  return true;
}

bool dxfRW::WriteCircle(EoDxfCircle* circle) {
  m_writer->WriteString(0, "CIRCLE");
  WriteEntity(circle);
  m_writer->WriteString(100, "AcDbCircle");
  m_writer->WriteDouble(10, circle->m_firstPoint.x);
  m_writer->WriteDouble(20, circle->m_firstPoint.y);
  if (circle->m_firstPoint.z != 0.0) { m_writer->WriteDouble(30, circle->m_firstPoint.z); }
  m_writer->WriteDouble(40, circle->m_radius);
  return true;
}

bool dxfRW::WriteArc(EoDxfArc* arc) {
  m_writer->WriteString(0, "ARC");
  WriteEntity(arc);
  m_writer->WriteString(100, "AcDbCircle");
  m_writer->WriteDouble(10, arc->m_firstPoint.x);
  m_writer->WriteDouble(20, arc->m_firstPoint.y);
  if (arc->m_firstPoint.z != 0.0) { m_writer->WriteDouble(30, arc->m_firstPoint.z); }
  m_writer->WriteDouble(40, arc->m_radius);
  m_writer->WriteString(100, "AcDbArc");
  m_writer->WriteDouble(50, arc->m_startAngle * EoDxf::RadiansToDegrees);
  m_writer->WriteDouble(51, arc->m_endAngle * EoDxf::RadiansToDegrees);
  return true;
}

bool dxfRW::WriteEllipse(EoDxfEllipse* ellipse) {
  // verify axis/ratio and params for full ellipse
  ellipse->CorrectAxis();

  m_writer->WriteString(0, "ELLIPSE");
  WriteEntity(ellipse);
  m_writer->WriteString(100, "AcDbEllipse");
  m_writer->WriteDouble(10, ellipse->m_firstPoint.x);
  m_writer->WriteDouble(20, ellipse->m_firstPoint.y);
  m_writer->WriteDouble(30, ellipse->m_firstPoint.z);
  m_writer->WriteDouble(11, ellipse->m_secondPoint.x);
  m_writer->WriteDouble(21, ellipse->m_secondPoint.y);
  m_writer->WriteDouble(31, ellipse->m_secondPoint.z);
  m_writer->WriteDouble(40, ellipse->m_ratio);
  m_writer->WriteDouble(41, ellipse->m_startParam);
  m_writer->WriteDouble(42, ellipse->m_endParam);

  return true;
}

bool dxfRW::WriteTrace(EoDxfTrace* trace) {
  m_writer->WriteString(0, "TRACE");
  WriteEntity(trace);
  m_writer->WriteString(100, "AcDbTrace");
  m_writer->WriteDouble(10, trace->m_firstPoint.x);
  m_writer->WriteDouble(20, trace->m_firstPoint.y);
  m_writer->WriteDouble(30, trace->m_firstPoint.z);
  m_writer->WriteDouble(11, trace->m_secondPoint.x);
  m_writer->WriteDouble(21, trace->m_secondPoint.y);
  m_writer->WriteDouble(31, trace->m_secondPoint.z);
  m_writer->WriteDouble(12, trace->m_thirdPoint.x);
  m_writer->WriteDouble(22, trace->m_thirdPoint.y);
  m_writer->WriteDouble(32, trace->m_thirdPoint.z);
  m_writer->WriteDouble(13, trace->m_fourthPoint.x);
  m_writer->WriteDouble(23, trace->m_fourthPoint.y);
  m_writer->WriteDouble(33, trace->m_fourthPoint.z);
  return true;
}

bool dxfRW::WriteSolid(EoDxfSolid* solid) {
  m_writer->WriteString(0, "SOLID");
  WriteEntity(solid);
  m_writer->WriteString(100, "AcDbTrace");  // SOLID shares the same subclass as TRACE
  m_writer->WriteDouble(10, solid->m_firstPoint.x);
  m_writer->WriteDouble(20, solid->m_firstPoint.y);
  m_writer->WriteDouble(30, solid->m_firstPoint.z);
  m_writer->WriteDouble(11, solid->m_secondPoint.x);
  m_writer->WriteDouble(21, solid->m_secondPoint.y);
  m_writer->WriteDouble(31, solid->m_secondPoint.z);
  m_writer->WriteDouble(12, solid->m_thirdPoint.x);
  m_writer->WriteDouble(22, solid->m_thirdPoint.y);
  m_writer->WriteDouble(32, solid->m_thirdPoint.z);
  m_writer->WriteDouble(13, solid->m_fourthPoint.x);
  m_writer->WriteDouble(23, solid->m_fourthPoint.y);
  m_writer->WriteDouble(33, solid->m_fourthPoint.z);
  return true;
}

bool dxfRW::Write3dFace(EoDxf3dFace* face) {
  m_writer->WriteString(0, "3DFACE");
  WriteEntity(face);
  m_writer->WriteString(100, "AcDbFace");
  m_writer->WriteDouble(10, face->m_firstPoint.x);
  m_writer->WriteDouble(20, face->m_firstPoint.y);
  m_writer->WriteDouble(30, face->m_firstPoint.z);
  m_writer->WriteDouble(11, face->m_secondPoint.x);
  m_writer->WriteDouble(21, face->m_secondPoint.y);
  m_writer->WriteDouble(31, face->m_secondPoint.z);
  m_writer->WriteDouble(12, face->m_thirdPoint.x);
  m_writer->WriteDouble(22, face->m_thirdPoint.y);
  m_writer->WriteDouble(32, face->m_thirdPoint.z);
  m_writer->WriteDouble(13, face->m_fourthPoint.x);
  m_writer->WriteDouble(23, face->m_fourthPoint.y);
  m_writer->WriteDouble(33, face->m_fourthPoint.z);
  m_writer->WriteInt16(70, face->m_invisibleFlag);
  return true;
}

bool dxfRW::WriteLWPolyline(EoDxfLwPolyline* polyline) {
  m_writer->WriteString(0, "LWPOLYLINE");
  WriteEntity(polyline);
  m_writer->WriteString(100, "AcDbPolyline");

  // Modern container – no more raw pointers
  polyline->m_numberOfVertices = static_cast<int>(polyline->m_vertices.size());
  m_writer->WriteInt32(90, polyline->m_numberOfVertices);
  m_writer->WriteInt16(70, polyline->m_polylineFlag);
  m_writer->WriteDouble(43, polyline->m_constantWidth);
  if (polyline->m_elevation != 0.0) { m_writer->WriteDouble(38, polyline->m_elevation); }
  if (polyline->m_thickness != 0.0) { m_writer->WriteDouble(39, polyline->m_thickness); }
  for (const auto& vertex : polyline->m_vertices) {
    m_writer->WriteDouble(10, vertex.x);
    m_writer->WriteDouble(20, vertex.y);
    if (vertex.stawidth != 0.0) { m_writer->WriteDouble(40, vertex.stawidth); }
    if (vertex.endwidth != 0.0) { m_writer->WriteDouble(41, vertex.endwidth); }
    if (vertex.bulge != 0.0) { m_writer->WriteDouble(42, vertex.bulge); }
  }

  return true;
}

bool dxfRW::WritePolyline(EoDxfPolyline* polyline) {
  m_writer->WriteString(0, "POLYLINE");
  WriteEntity(polyline);

  const bool is3dPolyline = (polyline->m_polylineFlag & (8 | 16 | 64)) != 0;
  m_writer->WriteString(100, is3dPolyline ? "AcDb3dPolyline" : "AcDb2dPolyline");

  // Group code 66, `entities follow flag` is optional in AC1015, ignored in AC1018 and later)
  if (m_version == EoDxf::Version::AC1014) { m_writer->WriteInt16(66, 1); }

  m_writer->WriteDouble(10, 0.0);
  m_writer->WriteDouble(20, 0.0);
  m_writer->WriteDouble(30, polyline->m_firstPoint.z);
  if (polyline->m_thickness != 0) { m_writer->WriteDouble(39, polyline->m_thickness); }
  m_writer->WriteInt16(70, polyline->m_polylineFlag);
  if (polyline->m_defaultStartWidth != 0) { m_writer->WriteDouble(40, polyline->m_defaultStartWidth); }
  if (polyline->m_defaultEndWidth != 0) { m_writer->WriteDouble(41, polyline->m_defaultEndWidth); }
  if ((polyline->m_polylineFlag & 16) || (polyline->m_polylineFlag & 32)) {
    m_writer->WriteInt16(71, polyline->m_polygonMeshVertexCountM);
    m_writer->WriteInt16(72, polyline->m_polygonMeshVertexCountN);
  }
  if (polyline->m_smoothSurfaceDensityM != 0) { m_writer->WriteInt16(73, polyline->m_smoothSurfaceDensityM); }
  if (polyline->m_smoothSurfaceDensityN != 0) { m_writer->WriteInt16(74, polyline->m_smoothSurfaceDensityN); }
  if (polyline->m_curvesAndSmoothSurfaceType != 0) { m_writer->WriteInt16(75, polyline->m_curvesAndSmoothSurfaceType); }
  auto extrusionDirection = polyline->m_extrusionDirection;
  if (extrusionDirection.x > EoDxf::geometricTolerance || extrusionDirection.y < -EoDxf::geometricTolerance ||
      std::abs(extrusionDirection.z - 1.0) > EoDxf::geometricTolerance) {
    m_writer->WriteDouble(210, extrusionDirection.x);
    m_writer->WriteDouble(220, extrusionDirection.y);
    m_writer->WriteDouble(230, extrusionDirection.z);
  }

  // VERTEX entities
  const auto polylineHandle = polyline->m_handle;
  for (auto* vertex : polyline->m_vertices) {
    m_writer->WriteString(0, "VERTEX");
    // WriteEntity assigns a new handle and writes AcDbEntity subclass
    WriteEntity(vertex);

    // Base subclass marker required before the specific subclass
    if (vertex->m_vertexFlags & 128) {
      m_writer->WriteString(100, "AcDbPolyFaceMeshVertex");
    } else if (vertex->m_vertexFlags & 64) {
      m_writer->WriteString(100, "AcDbPolygonMeshVertex");
    } else if (vertex->m_vertexFlags & 32) {
      m_writer->WriteString(100, "AcDb3dPolylineVertex");
    } else {
      m_writer->WriteString(100, "AcDb2dVertex");
    }

    if ((vertex->m_vertexFlags & 128) != 0 && (vertex->m_vertexFlags & 64) == 0) {
      // Polyface face records (flag 128 without flag 64): dummy coordinates
      m_writer->WriteDouble(10, 0.0);
      m_writer->WriteDouble(20, 0.0);
      m_writer->WriteDouble(30, 0.0);
    } else {
      m_writer->WriteDouble(10, vertex->m_firstPoint.x);
      m_writer->WriteDouble(20, vertex->m_firstPoint.y);
      m_writer->WriteDouble(30, vertex->m_firstPoint.z);
    }
    if (vertex->m_startingWidth != 0) { m_writer->WriteDouble(40, vertex->m_startingWidth); }
    if (vertex->m_endingWidth != 0) { m_writer->WriteDouble(41, vertex->m_endingWidth); }
    if (vertex->m_bulge != 0.0) { m_writer->WriteDouble(42, vertex->m_bulge); }
    if (vertex->m_vertexFlags != 0) { m_writer->WriteInt16(70, vertex->m_vertexFlags); }
    if ((vertex->m_vertexFlags & 2) != 0) { m_writer->WriteDouble(50, vertex->m_curveFitTangentDirection); }

    if ((vertex->m_vertexFlags & 128) != 0) {
      if (vertex->m_polyfaceMeshVertexIndex1 != 0) { m_writer->WriteInt16(71, vertex->m_polyfaceMeshVertexIndex1); }
      if (vertex->m_polyfaceMeshVertexIndex2 != 0) { m_writer->WriteInt16(72, vertex->m_polyfaceMeshVertexIndex2); }
      if (vertex->m_polyfaceMeshVertexIndex3 != 0) { m_writer->WriteInt16(73, vertex->m_polyfaceMeshVertexIndex3); }
      if (vertex->m_polyfaceMeshVertexIndex4 != 0) { m_writer->WriteInt16(74, vertex->m_polyfaceMeshVertexIndex4); }

      if ((vertex->m_vertexFlags & 64) == 0 && vertex->m_identifier != 0) {
        m_writer->WriteInt32(91, vertex->m_identifier);
      }
    }
  }

  // SEQEND entity
  m_writer->WriteString(0, "SEQEND");

  EoDxfSeqEnd seqEnd{*polyline};
  seqEnd.m_handle = ++m_entityCount;
  m_writer->WriteString(5, ToHexString(seqEnd.m_handle));
  m_writer->WriteString(330, ToHexString(polylineHandle));
  m_writer->WriteString(100, "AcDbEntity");
  m_writer->WriteUtf8String(8, seqEnd.m_layer);

  return true;
}

bool dxfRW::WriteSpline(EoDxfSpline* spline) {
  m_writer->WriteString(0, "SPLINE");
  WriteEntity(spline);
  m_writer->WriteString(100, "AcDbSpline");

  m_writer->WriteDouble(210, spline->m_normalVector.x);
  m_writer->WriteDouble(220, spline->m_normalVector.y);
  m_writer->WriteDouble(230, spline->m_normalVector.z);

  m_writer->WriteInt16(70, spline->m_splineFlag);
  m_writer->WriteInt16(71, spline->m_degreeOfTheSplineCurve);

  m_writer->WriteInt16(72, spline->m_numberOfKnots);
  m_writer->WriteInt16(73, spline->m_numberOfControlPoints);
  m_writer->WriteInt16(74, spline->m_numberOfFitPoints);

  m_writer->WriteDouble(42, spline->m_knotTolerance);
  m_writer->WriteDouble(43, spline->m_controlPointTolerance);
  m_writer->WriteDouble(44, spline->m_fitTolerance);

  // Start tangent (12, 22, 32) — write when spline flag bit 1 is set (tangent defined)
  if ((spline->m_splineFlag & 1) != 0) {
    m_writer->WriteDouble(12, spline->m_startTangent.x);
    m_writer->WriteDouble(22, spline->m_startTangent.y);
    m_writer->WriteDouble(32, spline->m_startTangent.z);
  }

  // End tangent (13, 23, 33)
  if ((spline->m_splineFlag & 1) != 0) {
    m_writer->WriteDouble(13, spline->m_endTangent.x);
    m_writer->WriteDouble(23, spline->m_endTangent.y);
    m_writer->WriteDouble(33, spline->m_endTangent.z);
  }

  for (int i = 0; i < spline->m_numberOfKnots; i++) { m_writer->WriteDouble(40, spline->m_knotValues.at(i)); }

  for (int i = 0; i < spline->m_numberOfControlPoints; i++) {
    auto* controlPoint = spline->m_controlPoints.at(i);
    m_writer->WriteDouble(10, controlPoint->x);
    m_writer->WriteDouble(20, controlPoint->y);
    m_writer->WriteDouble(30, controlPoint->z);
  }

  // Fit points (11, 21, 31)
  for (std::int32_t i = 0; i < spline->m_numberOfFitPoints; ++i) {
    const auto* fitPoint = spline->m_fitPoints[static_cast<size_t>(i)];
    m_writer->WriteDouble(11, fitPoint->x);
    m_writer->WriteDouble(21, fitPoint->y);
    m_writer->WriteDouble(31, fitPoint->z);
  }
  return true;
}

bool dxfRW::WriteHatch(EoDxfHatch* hatch) {
  m_writer->WriteString(0, "HATCH");
  WriteEntity(hatch);
  m_writer->WriteString(100, "AcDbHatch");
  m_writer->WriteDouble(10, hatch->m_firstPoint.x);
  m_writer->WriteDouble(20, hatch->m_firstPoint.y);
  m_writer->WriteDouble(30, hatch->m_firstPoint.z);
  m_writer->WriteDouble(210, hatch->m_extrusionDirection.x);
  m_writer->WriteDouble(220, hatch->m_extrusionDirection.y);
  m_writer->WriteDouble(230, hatch->m_extrusionDirection.z);
  m_writer->WriteString(2, hatch->m_hatchPatternName);
  m_writer->WriteInt16(70, hatch->m_solidFillFlag);
  m_writer->WriteInt16(71, hatch->m_associativityFlag);
  hatch->m_numberOfBoundaryPaths = static_cast<int>(hatch->m_hatchLoops.size());
  m_writer->WriteInt32(91, hatch->m_numberOfBoundaryPaths);
  // write paths data
  for (int i = 0; i < hatch->m_numberOfBoundaryPaths; i++) {
    auto* hatchLoop = hatch->m_hatchLoops.at(i);
    m_writer->WriteInt32(92, hatchLoop->m_boundaryPathType);
    if ((hatchLoop->m_boundaryPathType & 2) == 2) {
      // polyline boundary path
      if (!hatchLoop->m_entities.empty() && hatchLoop->m_entities.front()->m_entityType == EoDxf::LWPOLYLINE) {
        auto* polyline = static_cast<EoDxfLwPolyline*>(hatchLoop->m_entities.front().get());
        m_writer->WriteInt16(72, (polyline->m_constantWidth != 0.0) ? 1 : 0);
        m_writer->WriteInt16(73, polyline->m_polylineFlag);
        m_writer->WriteInt32(93, static_cast<int>(polyline->m_vertices.size()));
        for (const auto& v : polyline->m_vertices) {
          m_writer->WriteDouble(10, v.x);
          m_writer->WriteDouble(20, v.y);
          if (v.bulge != 0.0) { m_writer->WriteDouble(42, v.bulge); }
        }
      }
      m_writer->WriteInt32(97, 0);
    } else {
      hatchLoop->Update();
      m_writer->WriteInt32(93, hatchLoop->m_numberOfEdges);
      for (int j = 0; j < hatchLoop->m_numberOfEdges; ++j) {
        switch ((hatchLoop->m_entities.at(j))->m_entityType) {
          case EoDxf::LINE: {
            m_writer->WriteInt16(72, 1);
            auto* line = static_cast<EoDxfLine*>(hatchLoop->m_entities.at(j).get());
            m_writer->WriteDouble(10, line->m_firstPoint.x);
            m_writer->WriteDouble(20, line->m_firstPoint.y);
            m_writer->WriteDouble(11, line->m_secondPoint.x);
            m_writer->WriteDouble(21, line->m_secondPoint.y);
            break;
          }
          case EoDxf::ARC: {
            m_writer->WriteInt16(72, 2);
            auto* arc = static_cast<EoDxfArc*>(hatchLoop->m_entities.at(j).get());
            m_writer->WriteDouble(10, arc->m_firstPoint.x);
            m_writer->WriteDouble(20, arc->m_firstPoint.y);
            m_writer->WriteDouble(40, arc->m_radius);
            m_writer->WriteDouble(50, arc->m_startAngle * EoDxf::RadiansToDegrees);
            m_writer->WriteDouble(51, arc->m_endAngle * EoDxf::RadiansToDegrees);
            m_writer->WriteInt16(73, arc->m_isCounterClockwise);
            break;
          }
          case EoDxf::ELLIPSE: {
            m_writer->WriteInt16(72, 3);
            auto* ellipse = static_cast<EoDxfEllipse*>(hatchLoop->m_entities.at(j).get());
            ellipse->CorrectAxis();
            m_writer->WriteDouble(10, ellipse->m_firstPoint.x);
            m_writer->WriteDouble(20, ellipse->m_firstPoint.y);
            m_writer->WriteDouble(11, ellipse->m_secondPoint.x);
            m_writer->WriteDouble(21, ellipse->m_secondPoint.y);
            m_writer->WriteDouble(40, ellipse->m_ratio);
            m_writer->WriteDouble(50, ellipse->m_startParam);
            m_writer->WriteDouble(51, ellipse->m_endParam);
            m_writer->WriteInt16(73, ellipse->m_isCounterClockwise);
            break;
          }
          case EoDxf::SPLINE:
            break;
          default:
            break;
        }
      }
      m_writer->WriteInt32(97, 0);
    }
  }
  m_writer->WriteInt16(75, hatch->m_hatchStyle);
  m_writer->WriteInt16(76, hatch->m_hatchPatternType);
  if (!hatch->m_solidFillFlag) {
    m_writer->WriteDouble(52, hatch->m_hatchPatternAngle);
    m_writer->WriteDouble(41, hatch->m_hatchPatternScaleOrSpacing);
    m_writer->WriteInt16(77, hatch->m_hatchPatternDoubleFlag);
    m_writer->WriteInt16(78, hatch->m_numberOfPatternDefinitionLines);
  }
  m_writer->WriteInt32(98, 0);

  return true;
}

bool dxfRW::WriteLeader(EoDxfLeader* leader) {
  m_writer->WriteString(0, "LEADER");
  WriteEntity(leader);
  m_writer->WriteString(100, "AcDbLeader");
  m_writer->WriteUtf8String(3, leader->style);
  m_writer->WriteInt16(71, leader->arrow);
  m_writer->WriteInt16(72, leader->leadertype);
  m_writer->WriteInt16(73, leader->flag);
  m_writer->WriteInt16(74, leader->hookline);
  m_writer->WriteInt16(75, leader->hookflag);
  m_writer->WriteDouble(40, leader->textheight);
  m_writer->WriteDouble(41, leader->textwidth);
  m_writer->WriteDouble(76, leader->vertnum);
  m_writer->WriteDouble(76, static_cast<double>(leader->vertexlist.size()));
  for (unsigned int i = 0; i < leader->vertexlist.size(); i++) {
    auto* vertex = leader->vertexlist.at(i);
    m_writer->WriteDouble(10, vertex->x);
    m_writer->WriteDouble(20, vertex->y);
    m_writer->WriteDouble(30, vertex->z);
  }

  return true;
}
bool dxfRW::WriteDimension(EoDxfDimension* dimension) {
  m_writer->WriteString(0, "DIMENSION");
  WriteEntity(dimension);
  m_writer->WriteString(100, "AcDbDimension");
  if (!dimension->getName().empty()) { m_writer->WriteString(2, dimension->getName()); }
  m_writer->WriteDouble(10, dimension->getDefPoint().x);
  m_writer->WriteDouble(20, dimension->getDefPoint().y);
  m_writer->WriteDouble(30, dimension->getDefPoint().z);
  m_writer->WriteDouble(11, dimension->getTextPoint().x);
  m_writer->WriteDouble(21, dimension->getTextPoint().y);
  m_writer->WriteDouble(31, dimension->getTextPoint().z);
  if (!(dimension->type & 32)) { dimension->type = dimension->type + 32; }
  m_writer->WriteInt16(70, dimension->type);
  if (!(dimension->getText().empty())) { m_writer->WriteUtf8String(1, dimension->getText()); }
  m_writer->WriteInt16(71, dimension->getAlign());
  if (dimension->getTextLineStyle() != 1) { m_writer->WriteInt16(72, dimension->getTextLineStyle()); }
  if (dimension->getTextLineFactor() != 1) { m_writer->WriteDouble(41, dimension->getTextLineFactor()); }
  m_writer->WriteUtf8String(3, dimension->getStyle());
  if (dimension->getTextLineFactor() != 0) { m_writer->WriteDouble(53, dimension->getDir()); }
  m_writer->WriteDouble(210, dimension->getExtrusion().x);
  m_writer->WriteDouble(220, dimension->getExtrusion().y);
  m_writer->WriteDouble(230, dimension->getExtrusion().z);

  switch (dimension->m_entityType) {
    case EoDxf::DIMALIGNED:
    case EoDxf::DIMLINEAR: {
      auto* alignedDimension = dynamic_cast<EoDxfAlignedDimension*>(dimension);
      m_writer->WriteString(100, "AcDbAlignedDimension");
      auto crd = alignedDimension->getClonepoint();
      if (crd.x != 0 || crd.y != 0 || crd.z != 0) {
        m_writer->WriteDouble(12, crd.x);
        m_writer->WriteDouble(22, crd.y);
        m_writer->WriteDouble(32, crd.z);
      }
      m_writer->WriteDouble(13, alignedDimension->getDef1Point().x);
      m_writer->WriteDouble(23, alignedDimension->getDef1Point().y);
      m_writer->WriteDouble(33, alignedDimension->getDef1Point().z);
      m_writer->WriteDouble(14, alignedDimension->getDef2Point().x);
      m_writer->WriteDouble(24, alignedDimension->getDef2Point().y);
      m_writer->WriteDouble(34, alignedDimension->getDef2Point().z);
      if (dimension->m_entityType == EoDxf::DIMLINEAR) {
        auto* dl = dynamic_cast<EoDxfDimLinear*>(dimension);
        if (dl->getAngle() != 0) { m_writer->WriteDouble(50, dl->getAngle()); }
        if (dl->getOblique() != 0) { m_writer->WriteDouble(52, dl->getOblique()); }
        m_writer->WriteString(100, "AcDbRotatedDimension");
      }
      break;
    }
    case EoDxf::DIMRADIAL: {
      auto* radialDimension = dynamic_cast<EoDxfRadialDimension*>(dimension);
      m_writer->WriteString(100, "AcDbRadialDimension");
      m_writer->WriteDouble(15, radialDimension->getDiameterPoint().x);
      m_writer->WriteDouble(25, radialDimension->getDiameterPoint().y);
      m_writer->WriteDouble(35, radialDimension->getDiameterPoint().z);
      m_writer->WriteDouble(40, radialDimension->getLeaderLength());
      break;
    }
    case EoDxf::DIMDIAMETRIC: {
      auto* diametricDimension = dynamic_cast<EoDxfDiametricDimension*>(dimension);
      m_writer->WriteString(100, "AcDbDiametricDimension");
      m_writer->WriteDouble(15, diametricDimension->getDiameter1Point().x);
      m_writer->WriteDouble(25, diametricDimension->getDiameter1Point().y);
      m_writer->WriteDouble(35, diametricDimension->getDiameter1Point().z);
      m_writer->WriteDouble(40, diametricDimension->getLeaderLength());
      break;
    }
    case EoDxf::DIMANGULAR: {
      auto* _2LineAngularDimension = dynamic_cast<EoDxf2LineAngularDimension*>(dimension);
      m_writer->WriteString(100, "AcDb2LineAngularDimension");
      m_writer->WriteDouble(13, _2LineAngularDimension->getFirstLine1().x);
      m_writer->WriteDouble(23, _2LineAngularDimension->getFirstLine1().y);
      m_writer->WriteDouble(33, _2LineAngularDimension->getFirstLine1().z);
      m_writer->WriteDouble(14, _2LineAngularDimension->getFirstLine2().x);
      m_writer->WriteDouble(24, _2LineAngularDimension->getFirstLine2().y);
      m_writer->WriteDouble(34, _2LineAngularDimension->getFirstLine2().z);
      m_writer->WriteDouble(15, _2LineAngularDimension->getSecondLine1().x);
      m_writer->WriteDouble(25, _2LineAngularDimension->getSecondLine1().y);
      m_writer->WriteDouble(35, _2LineAngularDimension->getSecondLine1().z);
      m_writer->WriteDouble(16, _2LineAngularDimension->getDimPoint().x);
      m_writer->WriteDouble(26, _2LineAngularDimension->getDimPoint().y);
      m_writer->WriteDouble(36, _2LineAngularDimension->getDimPoint().z);
      break;
    }
    case EoDxf::DIMANGULAR3P: {
      auto* _3PointAngularDimension = dynamic_cast<EoDxf3PointAngularDimension*>(dimension);
      m_writer->WriteString(100, "AcDb3PointAngularDimension");
      m_writer->WriteDouble(13, _3PointAngularDimension->getFirstLine().x);
      m_writer->WriteDouble(23, _3PointAngularDimension->getFirstLine().y);
      m_writer->WriteDouble(33, _3PointAngularDimension->getFirstLine().z);
      m_writer->WriteDouble(14, _3PointAngularDimension->getSecondLine().x);
      m_writer->WriteDouble(24, _3PointAngularDimension->getSecondLine().y);
      m_writer->WriteDouble(34, _3PointAngularDimension->getSecondLine().z);
      m_writer->WriteDouble(15, _3PointAngularDimension->getVertexPoint().x);
      m_writer->WriteDouble(25, _3PointAngularDimension->getVertexPoint().y);
      m_writer->WriteDouble(35, _3PointAngularDimension->getVertexPoint().z);
      break;
    }
    case EoDxf::DIMORDINATE: {
      auto* ordinateDimension = dynamic_cast<EoDxfOrdinateDimension*>(dimension);
      m_writer->WriteString(100, "AcDbOrdinateDimension");
      m_writer->WriteDouble(13, ordinateDimension->getFirstLine().x);
      m_writer->WriteDouble(23, ordinateDimension->getFirstLine().y);
      m_writer->WriteDouble(33, ordinateDimension->getFirstLine().z);
      m_writer->WriteDouble(14, ordinateDimension->getSecondLine().x);
      m_writer->WriteDouble(24, ordinateDimension->getSecondLine().y);
      m_writer->WriteDouble(34, ordinateDimension->getSecondLine().z);
      break;
    }
    default:
      break;
  }

  return true;
}

bool dxfRW::WriteInsert(EoDxfInsert* blockReference) {
  m_writer->WriteString(0, "INSERT");
  WriteEntity(blockReference);

  m_writer->WriteString(100, "AcDbBlockReference");
  m_writer->WriteUtf8String(2, blockReference->m_blockName);

  m_writer->WriteDouble(10, blockReference->m_firstPoint.x);
  m_writer->WriteDouble(20, blockReference->m_firstPoint.y);
  m_writer->WriteDouble(30, blockReference->m_firstPoint.z);
  m_writer->WriteDouble(41, blockReference->m_xScaleFactor);
  m_writer->WriteDouble(42, blockReference->m_yScaleFactor);
  m_writer->WriteDouble(43, blockReference->m_zScaleFactor);
  m_writer->WriteDouble(50, (blockReference->m_rotationAngle) * EoDxf::RadiansToDegrees);  // in dxf angle is written in degrees
  m_writer->WriteInt16(70, blockReference->m_columnCount);
  m_writer->WriteInt16(71, blockReference->m_rowCount);
  m_writer->WriteDouble(44, blockReference->m_columnSpacing);
  m_writer->WriteDouble(45, blockReference->m_rowSpacing);
  return true;
}

bool dxfRW::WriteText(EoDxfText* text) {
  m_writer->WriteString(0, "TEXT");
  WriteEntity(text);
  m_writer->WriteString(100, "AcDbText");

  m_writer->WriteDouble(10, text->m_firstPoint.x);
  m_writer->WriteDouble(20, text->m_firstPoint.y);
  m_writer->WriteDouble(30, text->m_firstPoint.z);
  m_writer->WriteDouble(40, text->m_textHeight);
  m_writer->WriteUtf8String(1, text->m_string);
  m_writer->WriteDouble(50, text->m_textRotation);
  m_writer->WriteDouble(41, text->m_scaleFactorWidth);
  m_writer->WriteDouble(51, text->m_obliqueAngle);

  m_writer->WriteUtf8String(7, text->m_textStyleName);

  m_writer->WriteInt16(71, text->m_textGenerationFlags);
  if (text->m_horizontalAlignment != EoDxfText::HAlign::Left) { m_writer->WriteInt16(72, text->m_horizontalAlignment); }
  if (text->m_horizontalAlignment != EoDxfText::HAlign::Left || text->m_verticalAlignment != EoDxfText::VAlign::BaseLine) {
    m_writer->WriteDouble(11, text->m_secondPoint.x);
    m_writer->WriteDouble(21, text->m_secondPoint.y);
    m_writer->WriteDouble(31, text->m_secondPoint.z);
  }
  m_writer->WriteDouble(210, text->m_extrusionDirection.x);
  m_writer->WriteDouble(220, text->m_extrusionDirection.y);
  m_writer->WriteDouble(230, text->m_extrusionDirection.z);
  m_writer->WriteString(100, "AcDbText");
  if (text->m_verticalAlignment != EoDxfText::VAlign::BaseLine) { m_writer->WriteInt16(73, text->m_verticalAlignment); }
  return true;
}

bool dxfRW::WriteMText(EoDxfMText* mText) {
  m_writer->WriteString(0, "MTEXT");
  WriteEntity(mText);
  m_writer->WriteString(100, "AcDbMText");
  m_writer->WriteDouble(10, mText->m_firstPoint.x);
  m_writer->WriteDouble(20, mText->m_firstPoint.y);
  m_writer->WriteDouble(30, mText->m_firstPoint.z);
  m_writer->WriteDouble(40, mText->m_textHeight);
  m_writer->WriteDouble(41, mText->m_scaleFactorWidth);
  m_writer->WriteInt16(71, mText->m_textGenerationFlags);
  m_writer->WriteInt16(72, mText->m_horizontalAlignment);
  std::string text = m_writer->FromUtf8String(mText->m_string);

  int i;
  for (i = 0; (text.size() - i) > 250;) {
    m_writer->WriteString(3, text.substr(i, 250));
    i += 250;
  }
  m_writer->WriteString(1, text.substr(i));
  m_writer->WriteString(7, mText->m_textStyleName);
  m_writer->WriteDouble(210, mText->m_extrusionDirection.x);
  m_writer->WriteDouble(220, mText->m_extrusionDirection.y);
  m_writer->WriteDouble(230, mText->m_extrusionDirection.z);
  m_writer->WriteDouble(50, mText->m_textRotation);
  m_writer->WriteInt16(73, mText->m_verticalAlignment);
  m_writer->WriteDouble(44, mText->m_lineSpacingFactor);

  return true;
}

bool dxfRW::WriteViewport(EoDxfViewPort* viewport) {
  m_writer->WriteString(0, "VIEWPORT");
  WriteEntity(viewport);
  m_writer->WriteString(100, "AcDbViewport");
  m_writer->WriteDouble(10, viewport->m_firstPoint.x);
  m_writer->WriteDouble(20, viewport->m_firstPoint.y);
  if (viewport->m_firstPoint.z != 0.0) { m_writer->WriteDouble(30, viewport->m_firstPoint.z); }
  m_writer->WriteDouble(40, viewport->pswidth);
  m_writer->WriteDouble(41, viewport->psheight);
  m_writer->WriteInt16(68, viewport->vpstatus);
  m_writer->WriteInt16(69, viewport->vpID);
  m_writer->WriteDouble(12, viewport->centerPX);
  m_writer->WriteDouble(22, viewport->centerPY);
  return true;
}

EoDxfImageDefinition* dxfRW::WriteImage(EoDxfImage* rasterImage, std::string name) {
  // search if exist imagedef with this mane (image inserted more than 1 time)
  EoDxfImageDefinition* id = nullptr;
  for (unsigned int i = 0; i < m_imageDef.size(); i++) {
    if (m_imageDef.at(i)->m_fileNameOfImage == name) {
      id = m_imageDef.at(i);
      continue;
    }
  }
  if (id == nullptr) {
    id = new EoDxfImageDefinition();
    m_imageDef.push_back(id);
    id->m_handle = ++m_entityCount;
  }
  id->m_fileNameOfImage = name;
  std::string idReactor = ToHexString(++m_entityCount);

  m_writer->WriteString(0, "IMAGE");
  WriteEntity(rasterImage);
  m_writer->WriteString(100, "AcDbRasterImage");
  m_writer->WriteDouble(10, rasterImage->m_firstPoint.x);
  m_writer->WriteDouble(20, rasterImage->m_firstPoint.y);
  m_writer->WriteDouble(30, rasterImage->m_firstPoint.z);
  m_writer->WriteDouble(11, rasterImage->m_secondPoint.x);
  m_writer->WriteDouble(21, rasterImage->m_secondPoint.y);
  m_writer->WriteDouble(31, rasterImage->m_secondPoint.z);
  m_writer->WriteDouble(12, rasterImage->vVector.x);
  m_writer->WriteDouble(22, rasterImage->vVector.y);
  m_writer->WriteDouble(32, rasterImage->vVector.z);
  m_writer->WriteDouble(13, rasterImage->sizeu);
  m_writer->WriteDouble(23, rasterImage->sizev);
  m_writer->WriteString(340, ToHexString(id->m_handle));
  m_writer->WriteInt16(70, 1);
  m_writer->WriteInt16(280, rasterImage->clip);
  m_writer->WriteInt16(281, rasterImage->brightness);
  m_writer->WriteInt16(282, rasterImage->contrast);
  m_writer->WriteInt16(283, rasterImage->fade);
  m_writer->WriteString(360, idReactor);
  id->reactors[idReactor] = ToHexString(rasterImage->m_handle);
  return id;
}

bool dxfRW::WriteBlockRecord(std::string name) {
  m_writer->WriteString(0, "BLOCK_RECORD");
  m_writer->WriteString(5, ToHexString(++m_entityCount));

  m_blockMap[name] = m_entityCount;
  m_entityCount = 2 + m_entityCount;  // reserve 2 for BLOCK & ENDBLOCK
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "1"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbBlockTableRecord");
  m_writer->WriteUtf8String(2, name);
  if (m_version > EoDxf::Version::AC1018) {
    //    m_writer->WriteInt16(340, 22);
    m_writer->WriteInt16(70, 0);
    m_writer->WriteInt16(280, 1);
    m_writer->WriteInt16(281, 0);
  }

  return true;
}

bool dxfRW::WriteBlock(EoDxfBlock* block) {
  if (m_writingBlock) {
    m_writer->WriteString(0, "ENDBLK");

    m_writer->WriteString(5, ToHexString(m_currentHandle + 2));
    if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, ToHexString(m_currentHandle)); }
    m_writer->WriteString(100, "AcDbEntity");

    m_writer->WriteString(8, "0");
    m_writer->WriteString(100, "AcDbBlockEnd");
  }
  m_writingBlock = true;
  m_writer->WriteString(0, "BLOCK");

  m_currentHandle = (*(m_blockMap.find(block->name))).second;
  m_writer->WriteString(5, ToHexString(m_currentHandle + 1));
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, ToHexString(m_currentHandle)); }
  m_writer->WriteString(100, "AcDbEntity");

  m_writer->WriteString(8, "0");

  m_writer->WriteString(100, "AcDbBlockBegin");
  m_writer->WriteUtf8String(2, block->name);

  m_writer->WriteInt16(70, block->m_flags);
  m_writer->WriteDouble(10, block->m_firstPoint.x);
  m_writer->WriteDouble(20, block->m_firstPoint.y);
  if (block->m_firstPoint.z != 0.0) { m_writer->WriteDouble(30, block->m_firstPoint.z); }

  m_writer->WriteUtf8String(3, block->name);

  m_writer->WriteString(1, "");

  return true;
}

bool dxfRW::WriteTables() {
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "VPORT");

  m_writer->WriteString(5, "8");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 1);  // end table def
  /*** VPORT ***/
  m_standardDimensionStyle = false;
  m_interface->writeVports();
  if (!m_standardDimensionStyle) {
    EoDxfViewport activeViewport;
    activeViewport.m_tableName = "*ACTIVE";
    WriteVport(&activeViewport);
  }
  m_writer->WriteString(0, "ENDTAB");
  /*** LTYPE ***/
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "LTYPE");

  m_writer->WriteString(5, "5");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 4);  // end table def
  // Mandatory linetypes
  m_writer->WriteString(0, "LTYPE");

  m_writer->WriteString(5, "14");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "5"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbLinetypeTableRecord");
  m_writer->WriteString(2, "ByBlock");

  m_writer->WriteInt16(70, 0);
  m_writer->WriteString(3, "");
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, 0);
  m_writer->WriteDouble(40, 0.0);

  m_writer->WriteString(0, "LTYPE");

  m_writer->WriteString(5, "15");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "5"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbLinetypeTableRecord");
  m_writer->WriteString(2, "ByLayer");

  m_writer->WriteInt16(70, 0);
  m_writer->WriteString(3, "");
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, 0);
  m_writer->WriteDouble(40, 0.0);

  m_writer->WriteString(0, "LTYPE");

  m_writer->WriteString(5, "16");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "5"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbLinetypeTableRecord");
  m_writer->WriteString(2, "Continuous");

  m_writer->WriteInt16(70, 0);
  m_writer->WriteString(3, "Solid line");
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, 0);
  m_writer->WriteDouble(40, 0.0);
  // Aplication linetypes
  m_interface->writeLTypes();
  m_writer->WriteString(0, "ENDTAB");
  /*** LAYER ***/
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "LAYER");

  m_writer->WriteString(5, "2");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 1);  // end table def
  wlayer0 = false;
  m_interface->writeLayers();
  if (!wlayer0) {
    EoDxfLayer defaultLayer;
    defaultLayer.m_tableName = "0";
    WriteLayer(&defaultLayer);
  }
  m_writer->WriteString(0, "ENDTAB");
  /*** STYLE ***/
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "STYLE");

  m_writer->WriteString(5, "3");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 3);  // end table def
  m_standardDimensionStyle = false;
  m_interface->writeTextstyles();
  if (!m_standardDimensionStyle) {
    EoDxfTextStyle textStyle;
    textStyle.m_tableName = "Standard";
    WriteTextstyle(&textStyle);
  }
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "VIEW");

  m_writer->WriteString(5, "6");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 0);  // end table def
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "UCS");

  m_writer->WriteString(5, "7");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 0);  // end table def
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "APPID");

  m_writer->WriteString(5, "9");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 1);  // end table def
  m_writer->WriteString(0, "APPID");

  m_writer->WriteString(5, "12");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "9"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbRegAppTableRecord");

  m_writer->WriteString(2, "ACAD");
  m_writer->WriteInt16(70, 0);
  m_interface->writeAppId();
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "DIMSTYLE");

  m_writer->WriteString(5, "A");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 1);  // end table def
  if (m_version > EoDxf::Version::AC1014) {
    m_writer->WriteString(100, "AcDbDimStyleTable");
    m_writer->WriteInt16(71, 1);  // end table def
  }
  m_standardDimensionStyle = false;
  m_interface->writeDimstyles();
  if (!m_standardDimensionStyle) {
    EoDxfDimensionStyle dimensionStyle;
    dimensionStyle.m_tableName = "Standard";
    WriteDimStyle(&dimensionStyle);
  }
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "BLOCK_RECORD");
  m_writer->WriteString(5, "1");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");
  m_writer->WriteInt16(70, 2);  // end table def
  m_writer->WriteString(0, "BLOCK_RECORD");
  m_writer->WriteString(5, "1F");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "1"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbBlockTableRecord");
  m_writer->WriteString(2, "*Model_Space");
  if (m_version > EoDxf::Version::AC1018) {
    //    m_writer->WriteInt16(340, 22);
    m_writer->WriteInt16(70, 0);
    m_writer->WriteInt16(280, 1);
    m_writer->WriteInt16(281, 0);
  }
  m_writer->WriteString(0, "BLOCK_RECORD");
  m_writer->WriteString(5, "1E");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "1"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbBlockTableRecord");
  m_writer->WriteString(2, "*Paper_Space");
  if (m_version > EoDxf::Version::AC1018) {
    //    m_writer->WriteInt16(340, 22);
    m_writer->WriteInt16(70, 0);
    m_writer->WriteInt16(280, 1);
    m_writer->WriteInt16(281, 0);
  }

  /* allways call writeBlockRecords to interface for prepare unnamed blocks */
  m_interface->writeBlockRecords();
  m_writer->WriteString(0, "ENDTAB");
  return true;
}

bool dxfRW::WriteBlocks() {
  m_writer->WriteString(0, "BLOCK");

  m_writer->WriteString(5, "20");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "1F"); }
  m_writer->WriteString(100, "AcDbEntity");

  m_writer->WriteString(8, "0");

  m_writer->WriteString(100, "AcDbBlockBegin");
  m_writer->WriteString(2, "*Model_Space");

  m_writer->WriteInt16(70, 0);
  m_writer->WriteDouble(10, 0.0);
  m_writer->WriteDouble(20, 0.0);
  m_writer->WriteDouble(30, 0.0);

  m_writer->WriteString(3, "*Model_Space");

  m_writer->WriteString(1, "");
  m_writer->WriteString(0, "ENDBLK");

  m_writer->WriteString(5, "21");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "1F"); }
  m_writer->WriteString(100, "AcDbEntity");

  m_writer->WriteString(8, "0");
  m_writer->WriteString(100, "AcDbBlockEnd");
  m_writer->WriteString(0, "BLOCK");

  m_writer->WriteString(5, "1C");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "1B"); }
  m_writer->WriteString(100, "AcDbEntity");

  m_writer->WriteString(8, "0");

  m_writer->WriteString(100, "AcDbBlockBegin");
  m_writer->WriteString(2, "*Paper_Space");

  m_writer->WriteInt16(70, 0);
  m_writer->WriteDouble(10, 0.0);
  m_writer->WriteDouble(20, 0.0);
  m_writer->WriteDouble(30, 0.0);

  m_writer->WriteString(3, "*Paper_Space");

  m_writer->WriteString(1, "");
  m_writer->WriteString(0, "ENDBLK");

  m_writer->WriteString(5, "1D");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "1F"); }
  m_writer->WriteString(100, "AcDbEntity");

  m_writer->WriteString(8, "0");
  m_writer->WriteString(100, "AcDbBlockEnd");
  m_writingBlock = false;
  m_interface->writeBlocks();
  if (m_writingBlock) {
    m_writingBlock = false;
    m_writer->WriteString(0, "ENDBLK");

    m_writer->WriteString(5, ToHexString(m_currentHandle + 2));
    //            m_writer->WriteString(5, "1D");
    if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, ToHexString(m_currentHandle)); }
    m_writer->WriteString(100, "AcDbEntity");

    m_writer->WriteString(8, "0");
    m_writer->WriteString(100, "AcDbBlockEnd");
  }
  return true;
}

bool dxfRW::WriteObjects() {
  m_writer->WriteString(0, "DICTIONARY");
  std::string imgDictH;
  m_writer->WriteString(5, "C");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbDictionary");
  m_writer->WriteInt16(281, 1);
  m_writer->WriteString(3, "ACAD_GROUP");
  m_writer->WriteString(350, "D");
  if (m_imageDef.size() != 0) {
    m_writer->WriteString(3, "ACAD_IMAGE_DICT");
    imgDictH = ToHexString(++m_entityCount);
    m_writer->WriteString(350, imgDictH);
  }
  m_writer->WriteString(0, "DICTIONARY");
  m_writer->WriteString(5, "D");
  m_writer->WriteString(330, "C");
  m_writer->WriteString(100, "AcDbDictionary");
  m_writer->WriteInt16(281, 1);
  // write IMAGEDEF_REACTOR
  for (unsigned int i = 0; i < m_imageDef.size(); i++) {
    EoDxfImageDefinition* id = m_imageDef.at(i);
    std::map<std::string, std::string>::iterator it;
    for (it = id->reactors.begin(); it != id->reactors.end(); ++it) {
      m_writer->WriteString(0, "IMAGEDEF_REACTOR");
      m_writer->WriteString(5, it->first);
      m_writer->WriteString(330, it->second);
      m_writer->WriteString(100, "AcDbRasterImageDefReactor");
      m_writer->WriteInt32(90, 2);  // version 2=R14 to v2010
      m_writer->WriteString(330, it->second);
    }
  }
  if (m_imageDef.size() != 0) {
    m_writer->WriteString(0, "DICTIONARY");
    m_writer->WriteString(5, imgDictH);
    m_writer->WriteString(330, "C");
    m_writer->WriteString(100, "AcDbDictionary");
    m_writer->WriteInt16(281, 1);
    for (unsigned int i = 0; i < m_imageDef.size(); i++) {
      size_t f1, f2;
      f1 = m_imageDef.at(i)->m_fileNameOfImage.find_last_of("/\\");
      f2 = m_imageDef.at(i)->m_fileNameOfImage.find_last_of('.');
      ++f1;
      m_writer->WriteString(3, m_imageDef.at(i)->m_fileNameOfImage.substr(f1, f2 - f1));
      m_writer->WriteString(350, ToHexString(m_imageDef.at(i)->m_handle));
    }
  }
  for (unsigned int i = 0; i < m_imageDef.size(); i++) {
    EoDxfImageDefinition* id = m_imageDef.at(i);
    m_writer->WriteString(0, "IMAGEDEF");
    m_writer->WriteString(5, ToHexString(id->m_handle));
    if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, imgDictH); }
    m_writer->WriteString(102, "{ACAD_REACTORS");
    std::map<std::string, std::string>::iterator it;
    for (it = id->reactors.begin(); it != id->reactors.end(); ++it) { m_writer->WriteString(330, it->first); }
    m_writer->WriteString(102, "}");
    m_writer->WriteString(100, "AcDbRasterImageDef");
    m_writer->WriteInt32(90, 0);  // version 0=R14 to v2010
    m_writer->WriteUtf8String(1, id->m_fileNameOfImage);
    m_writer->WriteDouble(10, id->m_uImageSizeInPixels);
    m_writer->WriteDouble(20, id->m_vImageSizeInPixels);
    m_writer->WriteDouble(11, id->m_uSizeOfOnePixel);
    m_writer->WriteDouble(21, id->m_vSizeOfOnePixel);
    m_writer->WriteInt16(280, id->m_imageIsLoadedFlag);
    m_writer->WriteInt16(281, id->m_resolutionUnits);
  }
  // no more needed imageDef, delete it
  for (auto* id_ : m_imageDef) { delete id_; }
  m_imageDef.clear();

  return true;
}

bool dxfRW::WriteExtData(const std::vector<EoDxfGroupCodeValuesVariant*>& ed) {
  for (std::vector<EoDxfGroupCodeValuesVariant*>::const_iterator it = ed.begin(); it != ed.end(); ++it) {
    switch ((*it)->code()) {
      case 1000:
      case 1001:
      case 1002:
      case 1003:
      case 1004:
      case 1005: {
        int cc = (*it)->code();
        if ((*it)->type() == EoDxfGroupCodeValuesVariant::Type::String) { m_writer->WriteUtf8String(cc, *(*it)->content.s); }
        //            m_writer->WriteUtf8String((*it)->code, (*it)->content.s);
        break;
      }
      case 1010:
      case 1011:
      case 1012:
      case 1013:
        if ((*it)->type() == EoDxfGroupCodeValuesVariant::Type::Coord) {
          m_writer->WriteDouble((*it)->code(), (*it)->content.v->x);
          m_writer->WriteDouble((*it)->code() + 10, (*it)->content.v->y);
          m_writer->WriteDouble((*it)->code() + 20, (*it)->content.v->z);
        }
        break;
      case 1040:
      case 1041:
      case 1042:
        if ((*it)->type() == EoDxfGroupCodeValuesVariant::Type::Double) { m_writer->WriteDouble((*it)->code(), (*it)->content.d); }
        break;
      case 1070:
        if ((*it)->type() == EoDxfGroupCodeValuesVariant::Type::Integer) { m_writer->WriteInt16((*it)->code(), (*it)->content.i); }
        break;
      case 1071:
        if ((*it)->type() == EoDxfGroupCodeValuesVariant::Type::Integer) { m_writer->WriteInt32((*it)->code(), (*it)->content.i); }
        break;
      default:
        break;
    }
  }
  return true;
}

/********* Reader Process *********/

bool dxfRW::ProcessDxf() {
  int code;
  bool more = true;
  std::string sectionstr;
  //    section = secUnknown;
  while (m_reader->ReadRec(&code)) {
    if (code == 999) {
      m_header.AddComment(m_reader->GetString());
    } else if (code == 0) {
      sectionstr = m_reader->GetString();
      if (sectionstr == "EOF") {
        return true;  // found EOF terminate
      }
      if (sectionstr == "SECTION") {
        more = m_reader->ReadRec(&code);
        if (!more) {
          return false;  // wrong dxf file
        }
        if (code == 2) {
          sectionstr = m_reader->GetString();
          // found section, process it
          if (sectionstr == "HEADER") {
            ProcessHeader();
          } else if (sectionstr == "CLASSES") {
            ProcessClasses();
          } else if (sectionstr == "TABLES") {
            ProcessTables();
          } else if (sectionstr == "BLOCKS") {
            ProcessBlocks();
          } else if (sectionstr == "ENTITIES") {
            ProcessEntities(false);
          } else if (sectionstr == "OBJECTS") {
            ProcessObjects();
          }
        }
      }
    }
    /*    if (!more)
            return true;*/
  }
  return true;
}

/********* Header Section *********/

bool dxfRW::ProcessHeader() {
  std::string zeroGroupTag;
  int code{};
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      zeroGroupTag = m_reader->GetString();
      if (zeroGroupTag == "ENDSEC") {
        m_interface->addHeader(&m_header);
        return true;
      }
    } else {
      m_header.ParseCode(code, m_reader);
    }
  }
  return true;
}

/********* Classes Section *********/

bool dxfRW::ProcessClasses() {
  std::string zeroGroupTag;
  int code{};
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      zeroGroupTag = m_reader->GetString();
      if (zeroGroupTag == "CLASS") {
        EoDxfClass class_;
        while (m_reader->ReadRec(&code)) {
          if (code == 0) {
            zeroGroupTag = m_reader->GetString();
            if (zeroGroupTag == "CLASS") {
              m_interface->addClass(class_);
              class_.clear();
            } else if (zeroGroupTag == "ENDSEC") {
              m_interface->addClass(class_);
              return true;
            }
          } else {
            class_.ParseCode(code, m_reader);
          }
        }
      } else if (zeroGroupTag == "ENDSEC") {
        return true;
      }
    }
  }
  return false;
}

/********* Tables Section *********/

bool dxfRW::ProcessTables() {
  int code;
  std::string sectionstr;
  bool more = true;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      sectionstr = m_reader->GetString();
      if (sectionstr == "TABLE") {
        more = m_reader->ReadRec(&code);
        if (!more) {
          return false;  // wrong dxf file
        }
        if (code == 2) {
          sectionstr = m_reader->GetString();

          if (sectionstr == "LTYPE") {
            ProcessLType();
          } else if (sectionstr == "LAYER") {
            ProcessLayer();
          } else if (sectionstr == "STYLE") {
            ProcessTextStyle();
          } else if (sectionstr == "VPORT") {
            ProcessVports();
          } else if (sectionstr == "VIEW") {
            // processView();
          } else if (sectionstr == "UCS") {
            // processUCS();
          } else if (sectionstr == "APPID") {
            ProcessAppId();
          } else if (sectionstr == "DIMSTYLE") {
            ProcessDimStyle();
          } else if (sectionstr == "BLOCK_RECORD") {
            // processBlockRecord();
          }
        }
      } else if (sectionstr == "ENDSEC") {
        return true;
      }
    }
  }
  return true;
}

bool dxfRW::ProcessLType() {
  int code;
  std::string sectionstr;
  bool reading = false;
  EoDxfLinetype linetype;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) {
        linetype.Update();
        m_interface->addLinetype(linetype);
      }
      sectionstr = m_reader->GetString();
      if (sectionstr == "LTYPE") {
        reading = true;
        linetype.Reset();
      } else if (sectionstr == "ENDTAB") {
        return true;
      }
    } else if (reading) {
      linetype.ParseCode(code, m_reader);
    }
  }
  return true;
}

bool dxfRW::ProcessLayer() {
  int code;
  std::string sectionstr;
  bool reading = false;
  EoDxfLayer layer;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) { m_interface->addLayer(layer); }
      sectionstr = m_reader->GetString();
      if (sectionstr == "LAYER") {
        reading = true;
        layer.Reset();
      } else if (sectionstr == "ENDTAB") {
        return true;
      }
    } else if (reading) {
      layer.ParseCode(code, m_reader);
    }
  }
  return true;
}

bool dxfRW::ProcessDimStyle() {
  int code;
  std::string sectionstr;
  bool reading{};
  EoDxfDimensionStyle dimensionStyle;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) { m_interface->addDimStyle(dimensionStyle); }
      sectionstr = m_reader->GetString();
      if (sectionstr == "DIMSTYLE") {
        reading = true;
        dimensionStyle.Reset();
      } else if (sectionstr == "ENDTAB") {
        return true;
      }
    } else if (reading) {
      dimensionStyle.ParseCode(code, m_reader);
    }
  }
  return true;
}

bool dxfRW::ProcessTextStyle() {
  int code;
  std::string sectionstr;
  bool reading{};
  EoDxfTextStyle textStyle;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) { m_interface->addTextStyle(textStyle); }
      sectionstr = m_reader->GetString();
      if (sectionstr == "STYLE") {
        reading = true;
        textStyle.Reset();
      } else if (sectionstr == "ENDTAB") {
        return true;
      }
    } else if (reading) {
      textStyle.ParseCode(code, m_reader);
    }
  }
  return true;
}

bool dxfRW::ProcessVports() {
  int code;
  std::string sectionstr;
  bool reading{};
  EoDxfViewport viewport;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) { m_interface->addVport(viewport); }
      sectionstr = m_reader->GetString();
      if (sectionstr == "VPORT") {
        reading = true;
        viewport.Reset();
      } else if (sectionstr == "ENDTAB") {
        return true;
      }
    } else if (reading) {
      viewport.ParseCode(code, m_reader);
    }
  }
  return true;
}

bool dxfRW::ProcessAppId() {
  int code;
  std::string sectionstr;
  bool reading = false;
  EoDxfAppId appId;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) { m_interface->addAppId(appId); }
      sectionstr = m_reader->GetString();
      if (sectionstr == "APPID") {
        reading = true;
        appId.Reset();
      } else if (sectionstr == "ENDTAB") {
        return true;
      }
    } else if (reading) {
      appId.ParseCode(code, m_reader);
    }
  }
  return true;
}

/********* Block Section *********/

bool dxfRW::ProcessBlocks() {
  int code;
  std::string sectionstr;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      sectionstr = m_reader->GetString();
      if (sectionstr == "BLOCK") {
        ProcessBlock();
      } else if (sectionstr == "ENDSEC") {
        return true;
      }
    }
  }
  return true;
}

bool dxfRW::ProcessBlock() {
  int code;
  EoDxfBlock block;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addBlock(block);
        if (m_nextEntity == "ENDBLK") {
          m_interface->endBlock();
          return true;  // found ENDBLK, terminate
        } else {
          ProcessEntities(true);
          m_interface->endBlock();
          return true;  // found ENDBLK, terminate
        }
      }
      default:
        block.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

/********* Entities Section *********/

bool dxfRW::ProcessEntities(bool isblock) {
  int code;
  if (!m_reader->ReadRec(&code)) { return false; }
  bool next = true;
  if (code == 0) {
    m_nextEntity = m_reader->GetString();
  } else if (!isblock) {
    return false;  // first record in entities is 0
  }
  do {
    if (m_nextEntity == "ENDSEC" || m_nextEntity == "ENDBLK") {
      return true;
    } else if (m_nextEntity == "POINT") {
      ProcessPoint();
    } else if (m_nextEntity == "LINE") {
      ProcessLine();
    } else if (m_nextEntity == "CIRCLE") {
      ProcessCircle();
    } else if (m_nextEntity == "ARC") {
      ProcessArc();
    } else if (m_nextEntity == "ELLIPSE") {
      ProcessEllipse();
    } else if (m_nextEntity == "TRACE") {
      ProcessTrace();
    } else if (m_nextEntity == "SOLID") {
      ProcessSolid();
    } else if (m_nextEntity == "INSERT") {
      ProcessInsert();
    } else if (m_nextEntity == "LWPOLYLINE") {
      ProcessLWPolyline();
    } else if (m_nextEntity == "POLYLINE") {
      ProcessPolyline();
    } else if (m_nextEntity == "TEXT") {
      ProcessText();
    } else if (m_nextEntity == "MTEXT") {
      ProcessMText();
    } else if (m_nextEntity == "HATCH") {
      ProcessHatch();
    } else if (m_nextEntity == "SPLINE") {
      ProcessSpline();
    } else if (m_nextEntity == "3DFACE") {
      Process3dFace();
    } else if (m_nextEntity == "VIEWPORT") {
      ProcessViewport();
    } else if (m_nextEntity == "IMAGE") {
      ProcessImage();
    } else if (m_nextEntity == "DIMENSION") {
      ProcessDimension();
    } else if (m_nextEntity == "LEADER") {
      ProcessLeader();
    } else if (m_nextEntity == "RAY") {
      ProcessRay();
    } else if (m_nextEntity == "XLINE") {
      ProcessXline();
    } else {
      if (m_reader->ReadRec(&code)) {
        if (code == 0) { m_nextEntity = m_reader->GetString(); }
      } else {
        return false;  // end of file without ENDSEC
      }
    }

  } while (next);
  return true;
}

bool dxfRW::ProcessEllipse() {
  int code;
  EoDxfEllipse ellipse;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        if (m_applyExtrusion) { ellipse.ApplyExtrusion(); }
        m_interface->addEllipse(ellipse);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        ellipse.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessTrace() {
  int code;
  EoDxfTrace trace;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        if (m_applyExtrusion) { trace.ApplyExtrusion(); }
        m_interface->addTrace(trace);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        trace.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessSolid() {
  int code;
  EoDxfSolid solid;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        if (m_applyExtrusion) { solid.ApplyExtrusion(); }
        m_interface->addSolid(solid);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        solid.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::Process3dFace() {
  int code;
  EoDxf3dFace face;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->add3dFace(face);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        face.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessViewport() {
  int code;
  EoDxfViewPort viewport;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addViewport(viewport);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        viewport.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessPoint() {
  int code;
  EoDxfPoint point;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addPoint(point);
        return true;
      }
      default:
        point.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessLine() {
  int code;
  EoDxfLine line;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addLine(line);
        return true;
      }
      default:
        line.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessRay() {
  int code;
  EoDxfRay ray;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addRay(ray);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        ray.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessXline() {
  int code;
  EoDxfXline line;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addXline(line);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        line.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessCircle() {
  int code;
  EoDxfCircle circle;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        if (m_applyExtrusion) { circle.ApplyExtrusion(); }
        m_interface->addCircle(circle);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        circle.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessArc() {
  int code;
  EoDxfArc arc;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        if (m_applyExtrusion) { arc.ApplyExtrusion(); }
        m_interface->addArc(arc);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        arc.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessInsert() {
  int code;
  EoDxfInsert insert;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addInsert(insert);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        insert.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessLWPolyline() {
  int code;
  EoDxfLwPolyline polyline;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        if (m_applyExtrusion) { polyline.ApplyExtrusion(); }
        m_interface->addLWPolyline(polyline);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        polyline.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessPolyline() {
  int code;
  EoDxfPolyline polyline;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        if (m_nextEntity == "VERTEX") { ProcessVertex(&polyline); }
        m_interface->addPolyline(polyline);
        return true;
      }
      default:
        polyline.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessVertex(EoDxfPolyline* polyline) {
  int code;
  auto* vertex = new EoDxfVertex();
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        polyline->appendVertex(vertex);
        m_nextEntity = m_reader->GetString();
        if (m_nextEntity == "SEQEND") { return true; }
        if (m_nextEntity == "VERTEX") {
          vertex = new EoDxfVertex();
          break;
        }
        // Unexpected entity, stop processing vertices
        return true;
      }
      default:
        vertex->ParseCode(code, m_reader);
        break;
    }
  }
  // Unexpected end of file while processing vertices - avoid leaking in-process vertex
  delete vertex;
  return true;
}

bool dxfRW::ProcessText() {
  int code;
  EoDxfText text;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addText(text);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        text.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessMText() {
  int code;
  EoDxfMText txt;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        txt.UpdateAngle();
        m_interface->addMText(txt);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        txt.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessHatch() {
  int code;
  EoDxfHatch hatch;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addHatch(&hatch);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        hatch.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessSpline() {
  int code;
  EoDxfSpline sp;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addSpline(&sp);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        sp.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessImage() {
  int code;
  EoDxfImage image;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addImage(&image);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        image.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessDimension() {
  int code;
  EoDxfDimension dimension;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        int type = dimension.type & 0x0F;
        switch (type) {
          case 0: {
            EoDxfDimLinear d(dimension);
            m_interface->addDimLinear(&d);
            break;
          }
          case 1: {
            EoDxfAlignedDimension d(dimension);
            m_interface->addDimAlign(&d);
            break;
          }
          case 2: {
            EoDxf2LineAngularDimension d(dimension);
            m_interface->addDimAngular(&d);
            break;
          }
          case 3: {
            EoDxfDiametricDimension d(dimension);
            m_interface->addDimDiametric(&d);
            break;
          }
          case 4: {
            EoDxfRadialDimension d(dimension);
            m_interface->addDimRadial(&d);
            break;
          }
          case 5: {
            EoDxf3PointAngularDimension d(dimension);
            m_interface->addDimAngular3P(&d);
            break;
          }
          case 6: {
            EoDxfOrdinateDimension d(dimension);
            m_interface->addDimOrdinate(&d);
            break;
          }
        }
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        dimension.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessLeader() {
  int code;
  EoDxfLeader leader;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addLeader(&leader);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        leader.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

/********* Objects Section *********/

bool dxfRW::ProcessObjects() {
  int code;
  if (!m_reader->ReadRec(&code)) { return false; }
  if (code == 0) {
    m_nextEntity = m_reader->GetString();
  } else {
    return false;  // first record in objects is 0
  }
  for (;;) {
    if (m_nextEntity == "ENDSEC") {
      return true;  // found ENDSEC terminate
    } else if (m_nextEntity == "IMAGEDEF") {
      ProcessImageDef();
    } else {
      if (m_reader->ReadRec(&code)) {
        if (code == 0) { m_nextEntity = m_reader->GetString(); }
      } else {
        return false;  // end of file without ENDSEC
      }
    }
  }
}

bool dxfRW::ProcessImageDef() {
  int code;
  EoDxfImageDefinition imageDefinition;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->linkImage(&imageDefinition);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        imageDefinition.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

std::string dxfRW::ToHexString(int n) {
  std::ostringstream convert;
  convert << std::uppercase << std::hex << n;
  return convert.str();
}
