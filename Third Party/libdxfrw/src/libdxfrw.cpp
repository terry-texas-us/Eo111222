#include "libdxfrw.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <ios>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_classes.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "drw_interface.h"
#include "drw_objects.h"
#include "intern/drw_dbg.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

namespace {
constexpr auto FIRSTHANDLE{48};
}  // namespace

dxfRW::dxfRW(const char* name) {
  DRW_DBGSL(DRW_dbg::none);
  m_fileName = name;
  m_reader = nullptr;
  m_writer = nullptr;
  m_applyExtrusion = false;
  m_ellipseParts = 128;  // parts munber when convert ellipse to polyline
}
dxfRW::~dxfRW() {
  if (m_reader != nullptr) { delete m_reader; }
  if (m_writer != nullptr) { delete m_writer; }
  for (std::vector<DRW_ImageDef*>::iterator it = m_imageDef.begin(); it != m_imageDef.end(); ++it) { delete *it; }

  m_imageDef.clear();
}

void dxfRW::SetDebug(DRW::DebugTraceLevel traceLevel) {
  switch (traceLevel) {
    case DRW::DebugTraceLevel::Debug:
      DRW_DBGSL(DRW_dbg::debug);
      break;
    default:
      DRW_DBGSL(DRW_dbg::none);
  }
}

bool dxfRW::Read(DRW_Interface* interface_, bool ext) {
  drw_assert(fileName.empty() == false);
  bool isOk = false;
  m_applyExtrusion = ext;
  std::ifstream filestr;
  if (interface_ == nullptr) { return isOk; }
  filestr.open(m_fileName.c_str(), std::ios_base::in | std::ios::binary);
  if (!filestr.is_open()) { return isOk; }
  if (!filestr.good()) { return isOk; }

  char line[22]{};
  DRW_DBG("dxfRW::read first 22 binary bytes looking for binary file sentinel\n");
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
    DRW_DBG("dxfRW::read sentinel match: binary file\n");
  } else {
    m_binaryFile = false;
    filestr.open(m_fileName.c_str(), std::ios_base::in);
    m_reader = new dxfReaderAscii(&filestr);
    DRW_DBG("dxfRW::read confirm ascii file\n");
  }
  isOk = ProcessDxf();
  filestr.close();
  delete m_reader;
  m_reader = nullptr;
  return isOk;
}

bool dxfRW::Write(DRW_Interface* interface_, DRW::Version version, bool binaryFile) {
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
    DRW_DBG("dxfRW::read binary file\n");
  } else {
    filestr.open(m_fileName.c_str(), std::ios_base::out | std::ios::trunc);
    m_writer = new dxfWriterAscii(&filestr);
    std::string comm = std::string("dxfrw ") + std::string(DRW_VERSION);
    m_writer->WriteString(999, comm);
  }
  DRW_Header header;
  m_interface->writeHeader(header);
  m_writer->WriteString(0, "SECTION");
  m_entityCount = FIRSTHANDLE;
  header.write(m_writer, m_version);
  m_writer->WriteString(0, "ENDSEC");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "SECTION");
    m_writer->WriteString(2, "CLASSES");
    m_writer->WriteString(0, "ENDSEC");
  }
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

  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "SECTION");
    m_writer->WriteString(2, "OBJECTS");
    WriteObjects();
    m_writer->WriteString(0, "ENDSEC");
  }
  m_writer->WriteString(0, "EOF");
  filestr.flush();
  filestr.close();
  isOk = true;
  delete m_writer;
  m_writer = nullptr;
  return isOk;
}

bool dxfRW::WriteEntity(DRW_Entity* ent) {
  ent->handle = ++m_entityCount;
  m_writer->WriteString(5, ToHexString(ent->handle));
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbEntity"); }
  if (ent->space == 1) m_writer->WriteInt16(67, 1);
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteUtf8String(8, ent->layer);
    m_writer->WriteUtf8String(6, ent->lineType);
  } else {
    m_writer->WriteUtf8Caps(8, ent->layer);
    m_writer->WriteUtf8Caps(6, ent->lineType);
  }
  m_writer->WriteInt16(62, ent->color);
  if (m_version > DRW::Version::AC1015 && ent->color24 >= 0) { m_writer->WriteInt32(420, ent->color24); }
  if (m_version > DRW::Version::AC1014) { m_writer->WriteInt16(370, DRW_LW_Conv::lineWidth2dxfInt(ent->lWeight)); }
  return true;
}

bool dxfRW::WriteLineType(DRW_LType* ent) {
  std::string strname = ent->name;

  transform(strname.begin(), strname.end(), strname.begin(), ::toupper);
  // do not write linetypes handled by library
  if (strname == "BYLAYER" || strname == "BYBLOCK" || strname == "CONTINUOUS") { return true; }
  m_writer->WriteString(0, "LTYPE");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, ToHexString(++m_entityCount));
    if (m_version > DRW::Version::AC1012) { m_writer->WriteString(330, "5"); }
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbLinetypeTableRecord");
    m_writer->WriteUtf8String(2, ent->name);
  } else
    m_writer->WriteUtf8Caps(2, ent->name);
  m_writer->WriteInt16(70, ent->flags);
  m_writer->WriteUtf8String(3, ent->desc);
  ent->update();
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, ent->size);
  m_writer->WriteDouble(40, ent->length);

  for (unsigned int i = 0; i < ent->path.size(); i++) {
    m_writer->WriteDouble(49, ent->path.at(i));
    if (m_version > DRW::Version::AC1009) { m_writer->WriteInt16(74, 0); }
  }
  return true;
}

bool dxfRW::WriteLayer(DRW_Layer* ent) {
  m_writer->WriteString(0, "LAYER");
  if (!wlayer0 && ent->name == "0") {
    wlayer0 = true;
    if (m_version > DRW::Version::AC1009) { m_writer->WriteString(5, "10"); }
  } else {
    if (m_version > DRW::Version::AC1009) { m_writer->WriteString(5, ToHexString(++m_entityCount)); }
  }
  if (m_version > DRW::Version::AC1012) { m_writer->WriteString(330, "2"); }
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbLayerTableRecord");
    m_writer->WriteUtf8String(2, ent->name);
  } else {
    m_writer->WriteUtf8Caps(2, ent->name);
  }
  m_writer->WriteInt16(70, ent->flags);
  m_writer->WriteInt16(62, ent->color);
  if (m_version > DRW::Version::AC1015 && ent->color24 >= 0) { m_writer->WriteInt32(420, ent->color24); }
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteUtf8String(6, ent->lineType);
    if (!ent->plotF) m_writer->WriteBool(290, ent->plotF);
    m_writer->WriteInt16(370, DRW_LW_Conv::lineWidth2dxfInt(ent->lWeight));
    m_writer->WriteString(390, "F");
  } else
    m_writer->WriteUtf8Caps(6, ent->lineType);
  if (!ent->extData.empty()) { WriteExtData(ent->extData); }
  //    m_writer->WriteString(347, "10012");
  return true;
}

bool dxfRW::WriteTextstyle(DRW_Textstyle* ent) {
  m_writer->WriteString(0, "STYLE");
  if (!m_standardDimensionStyle) {
    std::string name = ent->name;
    transform(name.begin(), name.end(), name.begin(), toupper);
    if (name == "STANDARD") { m_standardDimensionStyle = true; }
  }
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(5, ToHexString(++m_entityCount)); }

  if (m_version > DRW::Version::AC1012) { m_writer->WriteString(330, "2"); }
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbTextStyleTableRecord");
    m_writer->WriteUtf8String(2, ent->name);
  } else {
    m_writer->WriteUtf8Caps(2, ent->name);
  }
  m_writer->WriteInt16(70, ent->flags);
  m_writer->WriteDouble(40, ent->height);
  m_writer->WriteDouble(41, ent->width);
  m_writer->WriteDouble(50, ent->oblique);
  m_writer->WriteInt16(71, ent->genFlag);
  m_writer->WriteDouble(42, ent->lastHeight);
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteUtf8String(3, ent->font);
    m_writer->WriteUtf8String(4, ent->bigFont);
    if (ent->fontFamily != 0) m_writer->WriteInt32(1071, ent->fontFamily);
  } else {
    m_writer->WriteUtf8Caps(3, ent->font);
    m_writer->WriteUtf8Caps(4, ent->bigFont);
  }
  return true;
}

bool dxfRW::WriteVport(DRW_Vport* ent) {
  if (!m_standardDimensionStyle) {
    ent->name = "*ACTIVE";
    m_standardDimensionStyle = true;
  }
  m_writer->WriteString(0, "VPORT");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, ToHexString(++m_entityCount));
    if (m_version > DRW::Version::AC1012) { m_writer->WriteString(330, "2"); }
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbViewportTableRecord");
    m_writer->WriteUtf8String(2, ent->name);
  } else
    m_writer->WriteUtf8Caps(2, ent->name);
  m_writer->WriteInt16(70, ent->flags);
  m_writer->WriteDouble(10, ent->lowerLeft.x);
  m_writer->WriteDouble(20, ent->lowerLeft.y);
  m_writer->WriteDouble(11, ent->upperRight.x);
  m_writer->WriteDouble(21, ent->upperRight.y);
  m_writer->WriteDouble(12, ent->center.x);
  m_writer->WriteDouble(22, ent->center.y);
  m_writer->WriteDouble(13, ent->snapBase.x);
  m_writer->WriteDouble(23, ent->snapBase.y);
  m_writer->WriteDouble(14, ent->snapSpacing.x);
  m_writer->WriteDouble(24, ent->snapSpacing.y);
  m_writer->WriteDouble(15, ent->gridSpacing.x);
  m_writer->WriteDouble(25, ent->gridSpacing.y);
  m_writer->WriteDouble(16, ent->viewDir.x);
  m_writer->WriteDouble(26, ent->viewDir.y);
  m_writer->WriteDouble(36, ent->viewDir.z);
  m_writer->WriteDouble(17, ent->viewTarget.x);
  m_writer->WriteDouble(27, ent->viewTarget.y);
  m_writer->WriteDouble(37, ent->viewTarget.z);
  m_writer->WriteDouble(40, ent->height);
  m_writer->WriteDouble(41, ent->ratio);
  m_writer->WriteDouble(42, ent->lensHeight);
  m_writer->WriteDouble(43, ent->frontClip);
  m_writer->WriteDouble(44, ent->backClip);
  m_writer->WriteDouble(50, ent->snapAngle);
  m_writer->WriteDouble(51, ent->twistAngle);
  m_writer->WriteInt16(71, ent->viewMode);
  m_writer->WriteInt16(72, ent->circleZoom);
  m_writer->WriteInt16(73, ent->fastZoom);
  m_writer->WriteInt16(74, ent->ucsIcon);
  m_writer->WriteInt16(75, ent->snap);
  m_writer->WriteInt16(76, ent->grid);
  m_writer->WriteInt16(77, ent->snapStyle);
  m_writer->WriteInt16(78, ent->snapIsopair);
  if (m_version > DRW::Version::AC1014) {
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
    if (m_version > DRW::Version::AC1018) {
      m_writer->WriteString(348, "10020");
      m_writer->WriteInt16(60, ent->gridBehavior);  // v2007 undocummented see DRW_Vport class
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

bool dxfRW::WriteDimstyle(DRW_Dimstyle* ent) {
  m_writer->WriteString(0, "DIMSTYLE");
  if (!m_standardDimensionStyle) {
    std::string name = ent->name;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    if (name == "STANDARD") {m_standardDimensionStyle = true;}
  }
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(105, ToHexString(++m_entityCount)); }

  if (m_version > DRW::Version::AC1012) { m_writer->WriteString(330, "A"); }
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbDimStyleTableRecord");
    m_writer->WriteUtf8String(2, ent->name);
  } else
    m_writer->WriteUtf8Caps(2, ent->name);
  m_writer->WriteInt16(70, ent->flags);
  if (m_version == DRW::Version::AC1009 || !(ent->dimpost.empty())) { m_writer->WriteUtf8String(3, ent->dimpost); }
  if (m_version == DRW::Version::AC1009 || !(ent->dimapost.empty())) { m_writer->WriteUtf8String(4, ent->dimapost); }
  if (m_version == DRW::Version::AC1009 || !(ent->dimblk.empty())) { m_writer->WriteUtf8String(5, ent->dimblk); }
  if (m_version == DRW::Version::AC1009 || !(ent->dimblk1.empty())) { m_writer->WriteUtf8String(6, ent->dimblk1); }
  if (m_version == DRW::Version::AC1009 || !(ent->dimblk2.empty())) { m_writer->WriteUtf8String(7, ent->dimblk2); }
  m_writer->WriteDouble(40, ent->dimscale);
  m_writer->WriteDouble(41, ent->dimasz);
  m_writer->WriteDouble(42, ent->dimexo);
  m_writer->WriteDouble(43, ent->dimdli);
  m_writer->WriteDouble(44, ent->dimexe);
  m_writer->WriteDouble(45, ent->dimrnd);
  m_writer->WriteDouble(46, ent->dimdle);
  m_writer->WriteDouble(47, ent->dimtp);
  m_writer->WriteDouble(48, ent->dimtm);
  if (m_version > DRW::Version::AC1018 || ent->dimfxl != 0) { m_writer->WriteDouble(49, ent->dimfxl); }
  m_writer->WriteDouble(140, ent->dimtxt);
  m_writer->WriteDouble(141, ent->dimcen);
  m_writer->WriteDouble(142, ent->dimtsz);
  m_writer->WriteDouble(143, ent->dimaltf);
  m_writer->WriteDouble(144, ent->dimlfac);
  m_writer->WriteDouble(145, ent->dimtvp);
  m_writer->WriteDouble(146, ent->dimtfac);
  m_writer->WriteDouble(147, ent->dimgap);
  if (m_version > DRW::Version::AC1014) { m_writer->WriteDouble(148, ent->dimaltrnd); }
  m_writer->WriteInt16(71, ent->dimtol);
  m_writer->WriteInt16(72, ent->dimlim);
  m_writer->WriteInt16(73, ent->dimtih);
  m_writer->WriteInt16(74, ent->dimtoh);
  m_writer->WriteInt16(75, ent->dimse1);
  m_writer->WriteInt16(76, ent->dimse2);
  m_writer->WriteInt16(77, ent->dimtad);
  m_writer->WriteInt16(78, ent->dimzin);
  if (m_version > DRW::Version::AC1014) { m_writer->WriteInt16(79, ent->dimazin); }
  m_writer->WriteInt16(170, ent->dimalt);
  m_writer->WriteInt16(171, ent->dimaltd);
  m_writer->WriteInt16(172, ent->dimtofl);
  m_writer->WriteInt16(173, ent->dimsah);
  m_writer->WriteInt16(174, ent->dimtix);
  m_writer->WriteInt16(175, ent->dimsoxd);
  m_writer->WriteInt16(176, ent->dimclrd);
  m_writer->WriteInt16(177, ent->dimclre);
  m_writer->WriteInt16(178, ent->dimclrt);
  if (m_version > DRW::Version::AC1014) { m_writer->WriteInt16(179, ent->dimadec); }
  if (m_version > DRW::Version::AC1009) {
    if (m_version < DRW::Version::AC1015) { m_writer->WriteInt16(270, ent->dimunit); }
    m_writer->WriteInt16(271, ent->dimdec);
    m_writer->WriteInt16(272, ent->dimtdec);
    m_writer->WriteInt16(273, ent->dimaltu);
    m_writer->WriteInt16(274, ent->dimalttd);
    m_writer->WriteInt16(275, ent->dimaunit);
  }
  if (m_version > DRW::Version::AC1014) {
    m_writer->WriteInt16(276, ent->dimfrac);
    m_writer->WriteInt16(277, ent->dimlunit);
    m_writer->WriteInt16(278, ent->dimdsep);
    m_writer->WriteInt16(279, ent->dimtmove);
  }
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteInt16(280, ent->dimjust);
    m_writer->WriteInt16(281, ent->dimsd1);
    m_writer->WriteInt16(282, ent->dimsd2);
    m_writer->WriteInt16(283, ent->dimtolj);
    m_writer->WriteInt16(284, ent->dimtzin);
    m_writer->WriteInt16(285, ent->dimaltz);
    m_writer->WriteInt16(286, ent->dimaltttz);
    if (m_version < DRW::Version::AC1015) { m_writer->WriteInt16(287, ent->dimfit); }
    m_writer->WriteInt16(288, ent->dimupt);
  }
  if (m_version > DRW::Version::AC1014) { m_writer->WriteInt16(289, ent->dimatfit); }
  if (m_version > DRW::Version::AC1018 && ent->dimfxlon != 0) { m_writer->WriteInt16(290, ent->dimfxlon); }
  if (m_version > DRW::Version::AC1009) { m_writer->WriteUtf8String(340, ent->dimtxsty); }
  if (m_version > DRW::Version::AC1014) {
    m_writer->WriteUtf8String(341, ent->dimldrblk);
    m_writer->WriteInt16(371, ent->dimlwd);
    m_writer->WriteInt16(372, ent->dimlwe);
  }
  return true;
}

bool dxfRW::WriteAppId(DRW_AppId* ent) {
  std::string strname = ent->name;
  transform(strname.begin(), strname.end(), strname.begin(), ::toupper);
  // do not write mandatory ACAD appId, handled by library
  if (strname == "ACAD") return true;
  m_writer->WriteString(0, "APPID");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, ToHexString(++m_entityCount));
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "9"); }
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbRegAppTableRecord");
    m_writer->WriteUtf8String(2, ent->name);
  } else {
    m_writer->WriteUtf8Caps(2, ent->name);
  }
  m_writer->WriteInt16(70, ent->flags);
  return true;
}

bool dxfRW::WritePoint(DRW_Point* ent) {
  m_writer->WriteString(0, "POINT");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbPoint"); }
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  if (ent->basePoint.z != 0.0) { m_writer->WriteDouble(30, ent->basePoint.z); }
  return true;
}

bool dxfRW::WriteLine(DRW_Line* ent) {
  m_writer->WriteString(0, "LINE");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbLine"); }
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
    m_writer->WriteDouble(30, ent->basePoint.z);
    m_writer->WriteDouble(11, ent->secPoint.x);
    m_writer->WriteDouble(21, ent->secPoint.y);
    m_writer->WriteDouble(31, ent->secPoint.z);
  } else {
    m_writer->WriteDouble(11, ent->secPoint.x);
    m_writer->WriteDouble(21, ent->secPoint.y);
  }
  return true;
}

bool dxfRW::WriteRay(DRW_Ray* ent) {
  m_writer->WriteString(0, "RAY");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbRay"); }
  DRW_Coord crd = ent->secPoint;
  crd.unitize();
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
    m_writer->WriteDouble(30, ent->basePoint.z);
    m_writer->WriteDouble(11, crd.x);
    m_writer->WriteDouble(21, crd.y);
    m_writer->WriteDouble(31, crd.z);
  } else {
    m_writer->WriteDouble(11, crd.x);
    m_writer->WriteDouble(21, crd.y);
  }
  return true;
}

bool dxfRW::WriteXline(DRW_Xline* ent) {
  m_writer->WriteString(0, "XLINE");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbXline"); }
  DRW_Coord crd = ent->secPoint;
  crd.unitize();
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
    m_writer->WriteDouble(30, ent->basePoint.z);
    m_writer->WriteDouble(11, crd.x);
    m_writer->WriteDouble(21, crd.y);
    m_writer->WriteDouble(31, crd.z);
  } else {
    m_writer->WriteDouble(11, crd.x);
    m_writer->WriteDouble(21, crd.y);
  }
  return true;
}

bool dxfRW::WriteCircle(DRW_Circle* ent) {
  m_writer->WriteString(0, "CIRCLE");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbCircle"); }
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  if (ent->basePoint.z != 0.0) { m_writer->WriteDouble(30, ent->basePoint.z); }
  m_writer->WriteDouble(40, ent->radious);
  return true;
}

bool dxfRW::WriteArc(DRW_Arc* ent) {
  m_writer->WriteString(0, "ARC");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbCircle"); }
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  if (ent->basePoint.z != 0.0) { m_writer->WriteDouble(30, ent->basePoint.z); }
  m_writer->WriteDouble(40, ent->radious);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbArc"); }
  m_writer->WriteDouble(50, ent->staangle * DRW::ARAD);
  m_writer->WriteDouble(51, ent->endangle * DRW::ARAD);
  return true;
}

bool dxfRW::WriteEllipse(DRW_Ellipse* ent) {
  // verify axis/ratio and params for full ellipse
  ent->correctAxis();
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "ELLIPSE");
    WriteEntity(ent);
    if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbEllipse"); }
    m_writer->WriteDouble(10, ent->basePoint.x);
    m_writer->WriteDouble(20, ent->basePoint.y);
    m_writer->WriteDouble(30, ent->basePoint.z);
    m_writer->WriteDouble(11, ent->secPoint.x);
    m_writer->WriteDouble(21, ent->secPoint.y);
    m_writer->WriteDouble(31, ent->secPoint.z);
    m_writer->WriteDouble(40, ent->ratio);
    m_writer->WriteDouble(41, ent->staparam);
    m_writer->WriteDouble(42, ent->endparam);
  } else {
    DRW_Polyline pol;

    ent->toPolyline(&pol, m_ellipseParts);
    WritePolyline(&pol);
  }
  return true;
}

bool dxfRW::WriteTrace(DRW_Trace* ent) {
  m_writer->WriteString(0, "TRACE");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbTrace"); }
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  m_writer->WriteDouble(30, ent->basePoint.z);
  m_writer->WriteDouble(11, ent->secPoint.x);
  m_writer->WriteDouble(21, ent->secPoint.y);
  m_writer->WriteDouble(31, ent->secPoint.z);
  m_writer->WriteDouble(12, ent->thirdPoint.x);
  m_writer->WriteDouble(22, ent->thirdPoint.y);
  m_writer->WriteDouble(32, ent->thirdPoint.z);
  m_writer->WriteDouble(13, ent->fourPoint.x);
  m_writer->WriteDouble(23, ent->fourPoint.y);
  m_writer->WriteDouble(33, ent->fourPoint.z);
  return true;
}

bool dxfRW::WriteSolid(DRW_Solid* ent) {
  m_writer->WriteString(0, "SOLID");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbTrace"); }
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  m_writer->WriteDouble(30, ent->basePoint.z);
  m_writer->WriteDouble(11, ent->secPoint.x);
  m_writer->WriteDouble(21, ent->secPoint.y);
  m_writer->WriteDouble(31, ent->secPoint.z);
  m_writer->WriteDouble(12, ent->thirdPoint.x);
  m_writer->WriteDouble(22, ent->thirdPoint.y);
  m_writer->WriteDouble(32, ent->thirdPoint.z);
  m_writer->WriteDouble(13, ent->fourPoint.x);
  m_writer->WriteDouble(23, ent->fourPoint.y);
  m_writer->WriteDouble(33, ent->fourPoint.z);
  return true;
}

bool dxfRW::Write3dface(DRW_3Dface* ent) {
  m_writer->WriteString(0, "3DFACE");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbFace"); }
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  m_writer->WriteDouble(30, ent->basePoint.z);
  m_writer->WriteDouble(11, ent->secPoint.x);
  m_writer->WriteDouble(21, ent->secPoint.y);
  m_writer->WriteDouble(31, ent->secPoint.z);
  m_writer->WriteDouble(12, ent->thirdPoint.x);
  m_writer->WriteDouble(22, ent->thirdPoint.y);
  m_writer->WriteDouble(32, ent->thirdPoint.z);
  m_writer->WriteDouble(13, ent->fourPoint.x);
  m_writer->WriteDouble(23, ent->fourPoint.y);
  m_writer->WriteDouble(33, ent->fourPoint.z);
  m_writer->WriteInt16(70, ent->invisibleflag);
  return true;
}

bool dxfRW::WriteLWPolyline(DRW_LWPolyline* ent) {
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "LWPOLYLINE");
    WriteEntity(ent);
    if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbPolyline"); }
    ent->vertexnum = static_cast<int>(ent->vertlist.size());
    m_writer->WriteInt32(90, ent->vertexnum);
    m_writer->WriteInt16(70, ent->flags);
    m_writer->WriteDouble(43, ent->width);
    if (ent->elevation != 0) m_writer->WriteDouble(38, ent->elevation);
    if (ent->thickness != 0) m_writer->WriteDouble(39, ent->thickness);
    for (int i = 0; i < ent->vertexnum; i++) {
      auto* v = ent->vertlist.at(i);
      m_writer->WriteDouble(10, v->x);
      m_writer->WriteDouble(20, v->y);
      if (v->stawidth != 0) m_writer->WriteDouble(40, v->stawidth);
      if (v->endwidth != 0) m_writer->WriteDouble(41, v->endwidth);
      if (v->bulge != 0) m_writer->WriteDouble(42, v->bulge);
    }
  } else {
    // @todo convert lwpolyline in polyline (not exist in acad 12)
  }
  return true;
}

bool dxfRW::WritePolyline(DRW_Polyline* ent) {
  m_writer->WriteString(0, "POLYLINE");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) {
    if (ent->flags & 8 || ent->flags & 16)
      m_writer->WriteString(100, "AcDb2dPolyline");
    else
      m_writer->WriteString(100, "AcDb3dPolyline");
  } else
    m_writer->WriteInt16(66, 1);
  m_writer->WriteDouble(10, 0.0);
  m_writer->WriteDouble(20, 0.0);
  m_writer->WriteDouble(30, ent->basePoint.z);
  if (ent->thickness != 0) { m_writer->WriteDouble(39, ent->thickness); }
  m_writer->WriteInt16(70, ent->flags);
  if (ent->defstawidth != 0) { m_writer->WriteDouble(40, ent->defstawidth); }
  if (ent->defendwidth != 0) { m_writer->WriteDouble(41, ent->defendwidth); }
  if (ent->flags & 16 || ent->flags & 32) {
    m_writer->WriteInt16(71, ent->vertexcount);
    m_writer->WriteInt16(72, ent->facecount);
  }
  if (ent->smoothM != 0) { m_writer->WriteInt16(73, ent->smoothM); }
  if (ent->smoothN != 0) { m_writer->WriteInt16(74, ent->smoothN); }
  if (ent->curvetype != 0) { m_writer->WriteInt16(75, ent->curvetype); }
  DRW_Coord crd = ent->extPoint;
  if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
    m_writer->WriteDouble(210, crd.x);
    m_writer->WriteDouble(220, crd.y);
    m_writer->WriteDouble(230, crd.z);
  }

  int vertexnum = static_cast<int>(ent->vertlist.size());
  for (int i = 0; i < vertexnum; i++) {
    DRW_Vertex* v = ent->vertlist.at(i);
    m_writer->WriteString(0, "VERTEX");
    WriteEntity(ent);
    if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbVertex"); }
    if ((v->flags & 128) && !(v->flags & 64)) {
      m_writer->WriteDouble(10, 0);
      m_writer->WriteDouble(20, 0);
      m_writer->WriteDouble(30, 0);
    } else {
      m_writer->WriteDouble(10, v->basePoint.x);
      m_writer->WriteDouble(20, v->basePoint.y);
      m_writer->WriteDouble(30, v->basePoint.z);
    }
    if (v->stawidth != 0) m_writer->WriteDouble(40, v->stawidth);
    if (v->endwidth != 0) m_writer->WriteDouble(41, v->endwidth);
    if (v->bulge != 0) m_writer->WriteDouble(42, v->bulge);
    if (v->flags != 0) { m_writer->WriteInt16(70, ent->flags); }
    if (v->flags & 2) { m_writer->WriteDouble(50, v->tgdir); }
    if (v->flags & 128) {
      if (v->vindex1 != 0) { m_writer->WriteInt16(71, v->vindex1); }
      if (v->vindex2 != 0) { m_writer->WriteInt16(72, v->vindex2); }
      if (v->vindex3 != 0) { m_writer->WriteInt16(73, v->vindex3); }
      if (v->vindex4 != 0) { m_writer->WriteInt16(74, v->vindex4); }
      if (!(v->flags & 64)) { m_writer->WriteInt32(91, v->identifier); }
    }
  }
  m_writer->WriteString(0, "SEQEND");
  WriteEntity(ent);
  return true;
}

bool dxfRW::WriteSpline(DRW_Spline* ent) {
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "SPLINE");
    WriteEntity(ent);
    if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbSpline"); }
    m_writer->WriteDouble(210, ent->normalVec.x);
    m_writer->WriteDouble(220, ent->normalVec.y);
    m_writer->WriteDouble(230, ent->normalVec.z);
    m_writer->WriteInt16(70, ent->flags);
    m_writer->WriteInt16(71, ent->degree);
    m_writer->WriteInt16(72, ent->nknots);
    m_writer->WriteInt16(73, ent->ncontrol);
    m_writer->WriteInt16(74, ent->nfit);
    m_writer->WriteDouble(42, ent->tolknot);
    m_writer->WriteDouble(43, ent->tolcontrol);
    // @bug warning check if nknots are correct and ncontrol
    for (int i = 0; i < ent->nknots; i++) { m_writer->WriteDouble(40, ent->knotslist.at(i)); }
    for (int i = 0; i < ent->ncontrol; i++) {
      DRW_Coord* crd = ent->controllist.at(i);
      m_writer->WriteDouble(10, crd->x);
      m_writer->WriteDouble(20, crd->y);
      m_writer->WriteDouble(30, crd->z);
    }
  } else {
    // @todo convert spline in polyline (not exist in acad 12)
  }
  return true;
}

bool dxfRW::WriteHatch(DRW_Hatch* ent) {
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "HATCH");
    WriteEntity(ent);
    m_writer->WriteString(100, "AcDbHatch");
    m_writer->WriteDouble(10, 0.0);
    m_writer->WriteDouble(20, 0.0);
    m_writer->WriteDouble(30, ent->basePoint.z);
    m_writer->WriteDouble(210, ent->extPoint.x);
    m_writer->WriteDouble(220, ent->extPoint.y);
    m_writer->WriteDouble(230, ent->extPoint.z);
    m_writer->WriteString(2, ent->name);
    m_writer->WriteInt16(70, ent->solid);
    m_writer->WriteInt16(71, ent->associative);
    ent->loopsnum = static_cast<int>(ent->looplist.size());
    m_writer->WriteInt16(91, ent->loopsnum);
    // write paths data
    for (int i = 0; i < ent->loopsnum; i++) {
      DRW_HatchLoop* loop = ent->looplist.at(i);
      m_writer->WriteInt16(92, loop->type);
      if ((loop->type & 2) == 2) {
      } else {
        // boundary path
        loop->update();
        m_writer->WriteInt16(93, loop->numedges);
        for (int j = 0; j < loop->numedges; ++j) {
          switch ((loop->objlist.at(j))->eType) {
            case DRW::LINE: {
              m_writer->WriteInt16(72, 1);
              DRW_Line* l = (DRW_Line*)loop->objlist.at(j);
              m_writer->WriteDouble(10, l->basePoint.x);
              m_writer->WriteDouble(20, l->basePoint.y);
              m_writer->WriteDouble(11, l->secPoint.x);
              m_writer->WriteDouble(21, l->secPoint.y);
              break;
            }
            case DRW::ARC: {
              m_writer->WriteInt16(72, 2);
              DRW_Arc* a = (DRW_Arc*)loop->objlist.at(j);
              m_writer->WriteDouble(10, a->basePoint.x);
              m_writer->WriteDouble(20, a->basePoint.y);
              m_writer->WriteDouble(40, a->radious);
              m_writer->WriteDouble(50, a->staangle * DRW::ARAD);
              m_writer->WriteDouble(51, a->endangle * DRW::ARAD);
              m_writer->WriteInt16(73, a->isccw);
              break;
            }
            case DRW::ELLIPSE: {
              m_writer->WriteInt16(72, 3);
              DRW_Ellipse* a = (DRW_Ellipse*)loop->objlist.at(j);
              a->correctAxis();
              m_writer->WriteDouble(10, a->basePoint.x);
              m_writer->WriteDouble(20, a->basePoint.y);
              m_writer->WriteDouble(11, a->secPoint.x);
              m_writer->WriteDouble(21, a->secPoint.y);
              m_writer->WriteDouble(40, a->ratio);
              m_writer->WriteDouble(50, a->staparam * DRW::ARAD);
              m_writer->WriteDouble(51, a->endparam * DRW::ARAD);
              m_writer->WriteInt16(73, a->isccw);
              break;
            }
            case DRW::SPLINE:
              break;
            default:
              break;
          }
        }
        m_writer->WriteInt16(97, 0);
      }
    }
    m_writer->WriteInt16(75, ent->hstyle);
    m_writer->WriteInt16(76, ent->hpattern);
    if (!ent->solid) {
      m_writer->WriteDouble(52, ent->angle);
      m_writer->WriteDouble(41, ent->scale);
      m_writer->WriteInt16(77, ent->doubleflag);
      m_writer->WriteInt16(78, ent->deflines);
    }
    /*        if (ent->deflines > 0){
                m_writer->WriteInt16(78, ent->deflines);
            }*/
    m_writer->WriteInt32(98, 0);
  } else {
  }
  return true;
}

bool dxfRW::WriteLeader(DRW_Leader* ent) {
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "LEADER");
    WriteEntity(ent);
    m_writer->WriteString(100, "AcDbLeader");
    m_writer->WriteUtf8String(3, ent->style);
    m_writer->WriteInt16(71, ent->arrow);
    m_writer->WriteInt16(72, ent->leadertype);
    m_writer->WriteInt16(73, ent->flag);
    m_writer->WriteInt16(74, ent->hookline);
    m_writer->WriteInt16(75, ent->hookflag);
    m_writer->WriteDouble(40, ent->textheight);
    m_writer->WriteDouble(41, ent->textwidth);
    m_writer->WriteDouble(76, ent->vertnum);
    m_writer->WriteDouble(76, static_cast<double>(ent->vertexlist.size()));
    for (unsigned int i = 0; i < ent->vertexlist.size(); i++) {
      DRW_Coord* vert = ent->vertexlist.at(i);
      m_writer->WriteDouble(10, vert->x);
      m_writer->WriteDouble(20, vert->y);
      m_writer->WriteDouble(30, vert->z);
    }
  } else {
    // @todo not supported by acad 12 saved as unnamed block
  }
  return true;
}
bool dxfRW::WriteDimension(DRW_Dimension* ent) {
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "DIMENSION");
    WriteEntity(ent);
    m_writer->WriteString(100, "AcDbDimension");
    if (!ent->getName().empty()) { m_writer->WriteString(2, ent->getName()); }
    m_writer->WriteDouble(10, ent->getDefPoint().x);
    m_writer->WriteDouble(20, ent->getDefPoint().y);
    m_writer->WriteDouble(30, ent->getDefPoint().z);
    m_writer->WriteDouble(11, ent->getTextPoint().x);
    m_writer->WriteDouble(21, ent->getTextPoint().y);
    m_writer->WriteDouble(31, ent->getTextPoint().z);
    if (!(ent->type & 32)) ent->type = ent->type + 32;
    m_writer->WriteInt16(70, ent->type);
    if (!(ent->getText().empty())) m_writer->WriteUtf8String(1, ent->getText());
    m_writer->WriteInt16(71, ent->getAlign());
    if (ent->getTextLineStyle() != 1) m_writer->WriteInt16(72, ent->getTextLineStyle());
    if (ent->getTextLineFactor() != 1) m_writer->WriteDouble(41, ent->getTextLineFactor());
    m_writer->WriteUtf8String(3, ent->getStyle());
    if (ent->getTextLineFactor() != 0) m_writer->WriteDouble(53, ent->getDir());
    m_writer->WriteDouble(210, ent->getExtrusion().x);
    m_writer->WriteDouble(220, ent->getExtrusion().y);
    m_writer->WriteDouble(230, ent->getExtrusion().z);

    switch (ent->eType) {
      case DRW::DIMALIGNED:
      case DRW::DIMLINEAR: {
        DRW_DimAligned* dd = (DRW_DimAligned*)ent;
        m_writer->WriteString(100, "AcDbAlignedDimension");
        DRW_Coord crd = dd->getClonepoint();
        if (crd.x != 0 || crd.y != 0 || crd.z != 0) {
          m_writer->WriteDouble(12, crd.x);
          m_writer->WriteDouble(22, crd.y);
          m_writer->WriteDouble(32, crd.z);
        }
        m_writer->WriteDouble(13, dd->getDef1Point().x);
        m_writer->WriteDouble(23, dd->getDef1Point().y);
        m_writer->WriteDouble(33, dd->getDef1Point().z);
        m_writer->WriteDouble(14, dd->getDef2Point().x);
        m_writer->WriteDouble(24, dd->getDef2Point().y);
        m_writer->WriteDouble(34, dd->getDef2Point().z);
        if (ent->eType == DRW::DIMLINEAR) {
          DRW_DimLinear* dl = (DRW_DimLinear*)ent;
          if (dl->getAngle() != 0) m_writer->WriteDouble(50, dl->getAngle());
          if (dl->getOblique() != 0) m_writer->WriteDouble(52, dl->getOblique());
          m_writer->WriteString(100, "AcDbRotatedDimension");
        }
        break;
      }
      case DRW::DIMRADIAL: {
        DRW_DimRadial* dd = (DRW_DimRadial*)ent;
        m_writer->WriteString(100, "AcDbRadialDimension");
        m_writer->WriteDouble(15, dd->getDiameterPoint().x);
        m_writer->WriteDouble(25, dd->getDiameterPoint().y);
        m_writer->WriteDouble(35, dd->getDiameterPoint().z);
        m_writer->WriteDouble(40, dd->getLeaderLength());
        break;
      }
      case DRW::DIMDIAMETRIC: {
        DRW_DimDiametric* dd = (DRW_DimDiametric*)ent;
        m_writer->WriteString(100, "AcDbDiametricDimension");
        m_writer->WriteDouble(15, dd->getDiameter1Point().x);
        m_writer->WriteDouble(25, dd->getDiameter1Point().y);
        m_writer->WriteDouble(35, dd->getDiameter1Point().z);
        m_writer->WriteDouble(40, dd->getLeaderLength());
        break;
      }
      case DRW::DIMANGULAR: {
        DRW_DimAngular* dd = (DRW_DimAngular*)ent;
        m_writer->WriteString(100, "AcDb2LineAngularDimension");
        m_writer->WriteDouble(13, dd->getFirstLine1().x);
        m_writer->WriteDouble(23, dd->getFirstLine1().y);
        m_writer->WriteDouble(33, dd->getFirstLine1().z);
        m_writer->WriteDouble(14, dd->getFirstLine2().x);
        m_writer->WriteDouble(24, dd->getFirstLine2().y);
        m_writer->WriteDouble(34, dd->getFirstLine2().z);
        m_writer->WriteDouble(15, dd->getSecondLine1().x);
        m_writer->WriteDouble(25, dd->getSecondLine1().y);
        m_writer->WriteDouble(35, dd->getSecondLine1().z);
        m_writer->WriteDouble(16, dd->getDimPoint().x);
        m_writer->WriteDouble(26, dd->getDimPoint().y);
        m_writer->WriteDouble(36, dd->getDimPoint().z);
        break;
      }
      case DRW::DIMANGULAR3P: {
        DRW_DimAngular3p* dd = (DRW_DimAngular3p*)ent;
        m_writer->WriteDouble(13, dd->getFirstLine().x);
        m_writer->WriteDouble(23, dd->getFirstLine().y);
        m_writer->WriteDouble(33, dd->getFirstLine().z);
        m_writer->WriteDouble(14, dd->getSecondLine().x);
        m_writer->WriteDouble(24, dd->getSecondLine().y);
        m_writer->WriteDouble(34, dd->getSecondLine().z);
        m_writer->WriteDouble(15, dd->getVertexPoint().x);
        m_writer->WriteDouble(25, dd->getVertexPoint().y);
        m_writer->WriteDouble(35, dd->getVertexPoint().z);
        break;
      }
      case DRW::DIMORDINATE: {
        DRW_DimOrdinate* dd = (DRW_DimOrdinate*)ent;
        m_writer->WriteString(100, "AcDbOrdinateDimension");
        m_writer->WriteDouble(13, dd->getFirstLine().x);
        m_writer->WriteDouble(23, dd->getFirstLine().y);
        m_writer->WriteDouble(33, dd->getFirstLine().z);
        m_writer->WriteDouble(14, dd->getSecondLine().x);
        m_writer->WriteDouble(24, dd->getSecondLine().y);
        m_writer->WriteDouble(34, dd->getSecondLine().z);
        break;
      }
      default:
        break;
    }
  } else {
    // @todo not supported by acad 12 saved as unnamed block
  }
  return true;
}

bool dxfRW::WriteInsert(DRW_Insert* ent) {
  m_writer->WriteString(0, "INSERT");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(100, "AcDbBlockReference");
    m_writer->WriteUtf8String(2, ent->name);
  } else
    m_writer->WriteUtf8Caps(2, ent->name);
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  m_writer->WriteDouble(30, ent->basePoint.z);
  m_writer->WriteDouble(41, ent->xscale);
  m_writer->WriteDouble(42, ent->yscale);
  m_writer->WriteDouble(43, ent->zscale);
  m_writer->WriteDouble(50, (ent->angle) * DRW::ARAD);  // in dxf angle is written in degrees
  m_writer->WriteInt16(70, ent->colcount);
  m_writer->WriteInt16(71, ent->rowcount);
  m_writer->WriteDouble(44, ent->colspace);
  m_writer->WriteDouble(45, ent->rowspace);
  return true;
}

bool dxfRW::WriteText(DRW_Text* ent) {
  m_writer->WriteString(0, "TEXT");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbText"); }
  //    m_writer->WriteDouble(39, ent->thickness);
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  m_writer->WriteDouble(30, ent->basePoint.z);
  m_writer->WriteDouble(40, ent->height);
  m_writer->WriteUtf8String(1, ent->text);
  m_writer->WriteDouble(50, ent->angle);
  m_writer->WriteDouble(41, ent->widthscale);
  m_writer->WriteDouble(51, ent->oblique);
  if (m_version > DRW::Version::AC1009)
    m_writer->WriteUtf8String(7, ent->style);
  else
    m_writer->WriteUtf8Caps(7, ent->style);
  m_writer->WriteInt16(71, ent->textgen);
  if (ent->alignH != DRW_Text::HLeft) { m_writer->WriteInt16(72, ent->alignH); }
  if (ent->alignH != DRW_Text::HLeft || ent->alignV != DRW_Text::VBaseLine) {
    m_writer->WriteDouble(11, ent->secPoint.x);
    m_writer->WriteDouble(21, ent->secPoint.y);
    m_writer->WriteDouble(31, ent->secPoint.z);
  }
  m_writer->WriteDouble(210, ent->extPoint.x);
  m_writer->WriteDouble(220, ent->extPoint.y);
  m_writer->WriteDouble(230, ent->extPoint.z);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbText"); }
  if (ent->alignV != DRW_Text::VBaseLine) { m_writer->WriteInt16(73, ent->alignV); }
  return true;
}

bool dxfRW::WriteMText(DRW_MText* ent) {
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "MTEXT");
    WriteEntity(ent);
    m_writer->WriteString(100, "AcDbMText");
    m_writer->WriteDouble(10, ent->basePoint.x);
    m_writer->WriteDouble(20, ent->basePoint.y);
    m_writer->WriteDouble(30, ent->basePoint.z);
    m_writer->WriteDouble(40, ent->height);
    m_writer->WriteDouble(41, ent->widthscale);
    m_writer->WriteInt16(71, ent->textgen);
    m_writer->WriteInt16(72, ent->alignH);
    std::string text = m_writer->FromUtf8String(ent->text);

    int i;
    for (i = 0; (text.size() - i) > 250;) {
      m_writer->WriteString(3, text.substr(i, 250));
      i += 250;
    }
    m_writer->WriteString(1, text.substr(i));
    m_writer->WriteString(7, ent->style);
    m_writer->WriteDouble(210, ent->extPoint.x);
    m_writer->WriteDouble(220, ent->extPoint.y);
    m_writer->WriteDouble(230, ent->extPoint.z);
    m_writer->WriteDouble(50, ent->angle);
    m_writer->WriteInt16(73, ent->alignV);
    m_writer->WriteDouble(44, ent->interlin);
  } else {
    // @todo convert mtext in text lines (not exist in acad 12)
  }
  return true;
}

bool dxfRW::WriteViewport(DRW_Viewport* ent) {
  m_writer->WriteString(0, "VIEWPORT");
  WriteEntity(ent);
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbViewport"); }
  m_writer->WriteDouble(10, ent->basePoint.x);
  m_writer->WriteDouble(20, ent->basePoint.y);
  if (ent->basePoint.z != 0.0) m_writer->WriteDouble(30, ent->basePoint.z);
  m_writer->WriteDouble(40, ent->pswidth);
  m_writer->WriteDouble(41, ent->psheight);
  m_writer->WriteInt16(68, ent->vpstatus);
  m_writer->WriteInt16(69, ent->vpID);
  m_writer->WriteDouble(12, ent->centerPX);
  m_writer->WriteDouble(22, ent->centerPY);
  return true;
}

DRW_ImageDef* dxfRW::WriteImage(DRW_Image* ent, std::string name) {
  if (m_version > DRW::Version::AC1009) {
    // search if exist imagedef with this mane (image inserted more than 1 time)
    DRW_ImageDef* id = nullptr;
    for (unsigned int i = 0; i < m_imageDef.size(); i++) {
      if (m_imageDef.at(i)->name == name) {
        id = m_imageDef.at(i);
        continue;
      }
    }
    if (id == nullptr) {
      id = new DRW_ImageDef();
      m_imageDef.push_back(id);
      id->handle = ++m_entityCount;
    }
    id->name = name;
    std::string idReactor = ToHexString(++m_entityCount);

    m_writer->WriteString(0, "IMAGE");
    WriteEntity(ent);
    m_writer->WriteString(100, "AcDbRasterImage");
    m_writer->WriteDouble(10, ent->basePoint.x);
    m_writer->WriteDouble(20, ent->basePoint.y);
    m_writer->WriteDouble(30, ent->basePoint.z);
    m_writer->WriteDouble(11, ent->secPoint.x);
    m_writer->WriteDouble(21, ent->secPoint.y);
    m_writer->WriteDouble(31, ent->secPoint.z);
    m_writer->WriteDouble(12, ent->vVector.x);
    m_writer->WriteDouble(22, ent->vVector.y);
    m_writer->WriteDouble(32, ent->vVector.z);
    m_writer->WriteDouble(13, ent->sizeu);
    m_writer->WriteDouble(23, ent->sizev);
    m_writer->WriteString(340, ToHexString(id->handle));
    m_writer->WriteInt16(70, 1);
    m_writer->WriteInt16(280, ent->clip);
    m_writer->WriteInt16(281, ent->brightness);
    m_writer->WriteInt16(282, ent->contrast);
    m_writer->WriteInt16(283, ent->fade);
    m_writer->WriteString(360, idReactor);
    id->reactors[idReactor] = ToHexString(ent->handle);
    return id;
  }
  return nullptr;  // not exist in acad 12
}

bool dxfRW::WriteBlockRecord(std::string name) {
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "BLOCK_RECORD");
    m_writer->WriteString(5, ToHexString(++m_entityCount));

    m_blockMap[name] = m_entityCount;
    m_entityCount = 2 + m_entityCount;  // reserve 2 for BLOCK & ENDBLOCK
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "1"); }
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbBlockTableRecord");
    m_writer->WriteUtf8String(2, name);
    if (m_version > DRW::Version::AC1018) {
      //    m_writer->WriteInt16(340, 22);
      m_writer->WriteInt16(70, 0);
      m_writer->WriteInt16(280, 1);
      m_writer->WriteInt16(281, 0);
    }
  }
  return true;
}

bool dxfRW::WriteBlock(DRW_Block* bk) {
  if (m_writingBlock) {
    m_writer->WriteString(0, "ENDBLK");
    if (m_version > DRW::Version::AC1009) {
      m_writer->WriteString(5, ToHexString(m_currentHandle + 2));
      if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, ToHexString(m_currentHandle)); }
      m_writer->WriteString(100, "AcDbEntity");
    }
    m_writer->WriteString(8, "0");
    if (m_version > DRW::Version::AC1009) { m_writer->WriteString(100, "AcDbBlockEnd"); }
  }
  m_writingBlock = true;
  m_writer->WriteString(0, "BLOCK");
  if (m_version > DRW::Version::AC1009) {
    m_currentHandle = (*(m_blockMap.find(bk->name))).second;
    m_writer->WriteString(5, ToHexString(m_currentHandle + 1));
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, ToHexString(m_currentHandle)); }
    m_writer->WriteString(100, "AcDbEntity");
  }
  m_writer->WriteString(8, "0");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(100, "AcDbBlockBegin");
    m_writer->WriteUtf8String(2, bk->name);
  } else
    m_writer->WriteUtf8Caps(2, bk->name);
  m_writer->WriteInt16(70, bk->flags);
  m_writer->WriteDouble(10, bk->basePoint.x);
  m_writer->WriteDouble(20, bk->basePoint.y);
  if (bk->basePoint.z != 0.0) { m_writer->WriteDouble(30, bk->basePoint.z); }
  if (m_version > DRW::Version::AC1009)
    m_writer->WriteUtf8String(3, bk->name);
  else
    m_writer->WriteUtf8Caps(3, bk->name);
  m_writer->WriteString(1, "");

  return true;
}

bool dxfRW::WriteTables() {
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "VPORT");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "8");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "0"); }
    m_writer->WriteString(100, "AcDbSymbolTable");
  }
  m_writer->WriteInt16(70, 1);  // end table def
  /*** VPORT ***/
  m_standardDimensionStyle = false;
  m_interface->writeVports();
  if (!m_standardDimensionStyle) {
    DRW_Vport portact;
    portact.name = "*ACTIVE";
    WriteVport(&portact);
  }
  m_writer->WriteString(0, "ENDTAB");
  /*** LTYPE ***/
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "LTYPE");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "5");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "0"); }
    m_writer->WriteString(100, "AcDbSymbolTable");
  }
  m_writer->WriteInt16(70, 4);  // end table def
  // Mandatory linetypes
  m_writer->WriteString(0, "LTYPE");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "14");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "5"); }
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbLinetypeTableRecord");
    m_writer->WriteString(2, "ByBlock");
  } else
    m_writer->WriteString(2, "BYBLOCK");
  m_writer->WriteInt16(70, 0);
  m_writer->WriteString(3, "");
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, 0);
  m_writer->WriteDouble(40, 0.0);

  m_writer->WriteString(0, "LTYPE");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "15");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "5"); }
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbLinetypeTableRecord");
    m_writer->WriteString(2, "ByLayer");
  } else
    m_writer->WriteString(2, "BYLAYER");
  m_writer->WriteInt16(70, 0);
  m_writer->WriteString(3, "");
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, 0);
  m_writer->WriteDouble(40, 0.0);

  m_writer->WriteString(0, "LTYPE");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "16");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "5"); }
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbLinetypeTableRecord");
    m_writer->WriteString(2, "Continuous");
  } else {
    m_writer->WriteString(2, "CONTINUOUS");
  }
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
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "2");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "0"); }
    m_writer->WriteString(100, "AcDbSymbolTable");
  }
  m_writer->WriteInt16(70, 1);  // end table def
  wlayer0 = false;
  m_interface->writeLayers();
  if (!wlayer0) {
    DRW_Layer lay0;
    lay0.name = "0";
    WriteLayer(&lay0);
  }
  m_writer->WriteString(0, "ENDTAB");
  /*** STYLE ***/
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "STYLE");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "3");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "0"); }
    m_writer->WriteString(100, "AcDbSymbolTable");
  }
  m_writer->WriteInt16(70, 3);  // end table def
  m_standardDimensionStyle = false;
  m_interface->writeTextstyles();
  if (!m_standardDimensionStyle) {
    DRW_Textstyle tsty;
    tsty.name = "Standard";
    WriteTextstyle(&tsty);
  }
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "VIEW");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "6");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "0"); }
    m_writer->WriteString(100, "AcDbSymbolTable");
  }
  m_writer->WriteInt16(70, 0);  // end table def
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "UCS");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "7");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "0"); }
    m_writer->WriteString(100, "AcDbSymbolTable");
  }
  m_writer->WriteInt16(70, 0);  // end table def
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "APPID");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "9");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "0"); }
    m_writer->WriteString(100, "AcDbSymbolTable");
  }
  m_writer->WriteInt16(70, 1);  // end table def
  m_writer->WriteString(0, "APPID");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "12");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "9"); }
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbRegAppTableRecord");
  }
  m_writer->WriteString(2, "ACAD");
  m_writer->WriteInt16(70, 0);
  m_interface->writeAppId();
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "DIMSTYLE");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "A");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "0"); }
    m_writer->WriteString(100, "AcDbSymbolTable");
  }
  m_writer->WriteInt16(70, 1);  // end table def
  if (m_version > DRW::Version::AC1014) {
    m_writer->WriteString(100, "AcDbDimStyleTable");
    m_writer->WriteInt16(71, 1);  // end table def
  }
  m_standardDimensionStyle = false;
  m_interface->writeDimstyles();
  if (!m_standardDimensionStyle) {
    DRW_Dimstyle dsty;
    dsty.name = "Standard";
    WriteDimstyle(&dsty);
  }
  m_writer->WriteString(0, "ENDTAB");

  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(0, "TABLE");
    m_writer->WriteString(2, "BLOCK_RECORD");
    m_writer->WriteString(5, "1");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "0"); }
    m_writer->WriteString(100, "AcDbSymbolTable");
    m_writer->WriteInt16(70, 2);  // end table def
    m_writer->WriteString(0, "BLOCK_RECORD");
    m_writer->WriteString(5, "1F");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "1"); }
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbBlockTableRecord");
    m_writer->WriteString(2, "*Model_Space");
    if (m_version > DRW::Version::AC1018) {
      //    m_writer->WriteInt16(340, 22);
      m_writer->WriteInt16(70, 0);
      m_writer->WriteInt16(280, 1);
      m_writer->WriteInt16(281, 0);
    }
    m_writer->WriteString(0, "BLOCK_RECORD");
    m_writer->WriteString(5, "1E");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "1"); }
    m_writer->WriteString(100, "AcDbSymbolTableRecord");
    m_writer->WriteString(100, "AcDbBlockTableRecord");
    m_writer->WriteString(2, "*Paper_Space");
    if (m_version > DRW::Version::AC1018) {
      //    m_writer->WriteInt16(340, 22);
      m_writer->WriteInt16(70, 0);
      m_writer->WriteInt16(280, 1);
      m_writer->WriteInt16(281, 0);
    }
  }
  /* allways call writeBlockRecords to interface for prepare unnamed blocks */
  m_interface->writeBlockRecords();
  if (m_version > DRW::Version::AC1009) { m_writer->WriteString(0, "ENDTAB"); }
  return true;
}

bool dxfRW::WriteBlocks() {
  m_writer->WriteString(0, "BLOCK");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "20");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "1F"); }
    m_writer->WriteString(100, "AcDbEntity");
  }
  m_writer->WriteString(8, "0");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(100, "AcDbBlockBegin");
    m_writer->WriteString(2, "*Model_Space");
  } else
    m_writer->WriteString(2, "$MODEL_SPACE");
  m_writer->WriteInt16(70, 0);
  m_writer->WriteDouble(10, 0.0);
  m_writer->WriteDouble(20, 0.0);
  m_writer->WriteDouble(30, 0.0);
  if (m_version > DRW::Version::AC1009)
    m_writer->WriteString(3, "*Model_Space");
  else
    m_writer->WriteString(3, "$MODEL_SPACE");
  m_writer->WriteString(1, "");
  m_writer->WriteString(0, "ENDBLK");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "21");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "1F"); }
    m_writer->WriteString(100, "AcDbEntity");
  }
  m_writer->WriteString(8, "0");
  if (m_version > DRW::Version::AC1009) m_writer->WriteString(100, "AcDbBlockEnd");
  m_writer->WriteString(0, "BLOCK");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "1C");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "1B"); }
    m_writer->WriteString(100, "AcDbEntity");
  }
  m_writer->WriteString(8, "0");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(100, "AcDbBlockBegin");
    m_writer->WriteString(2, "*Paper_Space");
  } else
    m_writer->WriteString(2, "$PAPER_SPACE");
  m_writer->WriteInt16(70, 0);
  m_writer->WriteDouble(10, 0.0);
  m_writer->WriteDouble(20, 0.0);
  m_writer->WriteDouble(30, 0.0);
  if (m_version > DRW::Version::AC1009)
    m_writer->WriteString(3, "*Paper_Space");
  else
    m_writer->WriteString(3, "$PAPER_SPACE");
  m_writer->WriteString(1, "");
  m_writer->WriteString(0, "ENDBLK");
  if (m_version > DRW::Version::AC1009) {
    m_writer->WriteString(5, "1D");
    if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "1F"); }
    m_writer->WriteString(100, "AcDbEntity");
  }
  m_writer->WriteString(8, "0");
  if (m_version > DRW::Version::AC1009) m_writer->WriteString(100, "AcDbBlockEnd");
  m_writingBlock = false;
  m_interface->writeBlocks();
  if (m_writingBlock) {
    m_writingBlock = false;
    m_writer->WriteString(0, "ENDBLK");
    if (m_version > DRW::Version::AC1009) {
      m_writer->WriteString(5, ToHexString(m_currentHandle + 2));
      //            m_writer->WriteString(5, "1D");
      if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, ToHexString(m_currentHandle)); }
      m_writer->WriteString(100, "AcDbEntity");
    }
    m_writer->WriteString(8, "0");
    if (m_version > DRW::Version::AC1009) m_writer->WriteString(100, "AcDbBlockEnd");
  }
  return true;
}

bool dxfRW::WriteObjects() {
  m_writer->WriteString(0, "DICTIONARY");
  std::string imgDictH;
  m_writer->WriteString(5, "C");
  if (m_version > DRW::Version::AC1014) { m_writer->WriteString(330, "0"); }
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
    DRW_ImageDef* id = m_imageDef.at(i);
    std::map<std::string, std::string>::iterator it;
    for (it = id->reactors.begin(); it != id->reactors.end(); ++it) {
      m_writer->WriteString(0, "IMAGEDEF_REACTOR");
      m_writer->WriteString(5, (*it).first);
      m_writer->WriteString(330, (*it).second);
      m_writer->WriteString(100, "AcDbRasterImageDefReactor");
      m_writer->WriteInt16(90, 2);  // version 2=R14 to v2010
      m_writer->WriteString(330, (*it).second);
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
      f1 = m_imageDef.at(i)->name.find_last_of("/\\");
      f2 = m_imageDef.at(i)->name.find_last_of('.');
      ++f1;
      m_writer->WriteString(3, m_imageDef.at(i)->name.substr(f1, f2 - f1));
      m_writer->WriteString(350, ToHexString(m_imageDef.at(i)->handle));
    }
  }
  for (unsigned int i = 0; i < m_imageDef.size(); i++) {
    DRW_ImageDef* id = m_imageDef.at(i);
    m_writer->WriteString(0, "IMAGEDEF");
    m_writer->WriteString(5, ToHexString(id->handle));
    if (m_version > DRW::Version::AC1014) {
      //            m_writer->WriteString(330, "0"); handle to DICTIONARY
    }
    m_writer->WriteString(102, "{ACAD_REACTORS");
    std::map<std::string, std::string>::iterator it;
    for (it = id->reactors.begin(); it != id->reactors.end(); ++it) { m_writer->WriteString(330, (*it).first); }
    m_writer->WriteString(102, "}");
    m_writer->WriteString(100, "AcDbRasterImageDef");
    m_writer->WriteInt16(90, 0);  // version 0=R14 to v2010
    m_writer->WriteUtf8String(1, id->name);
    m_writer->WriteDouble(10, id->u);
    m_writer->WriteDouble(20, id->v);
    m_writer->WriteDouble(11, id->up);
    m_writer->WriteDouble(21, id->vp);
    m_writer->WriteInt16(280, id->loaded);
    m_writer->WriteInt16(281, id->resolution);
  }
  // no more needed imageDef, delete it
  while (!m_imageDef.empty()) { m_imageDef.pop_back(); }

  return true;
}

bool dxfRW::WriteExtData(const std::vector<DRW_Variant*>& ed) {
  for (std::vector<DRW_Variant*>::const_iterator it = ed.begin(); it != ed.end(); ++it) {
    switch ((*it)->code()) {
      case 1000:
      case 1001:
      case 1002:
      case 1003:
      case 1004:
      case 1005: {
        int cc = (*it)->code();
        if ((*it)->type() == DRW_Variant::Type::String) { m_writer->WriteUtf8String(cc, *(*it)->content.s); }
        //            m_writer->WriteUtf8String((*it)->code, (*it)->content.s);
        break;
      }
      case 1010:
      case 1011:
      case 1012:
      case 1013:
        if ((*it)->type() == DRW_Variant::Type::Coord) {
          m_writer->WriteDouble((*it)->code(), (*it)->content.v->x);
          m_writer->WriteDouble((*it)->code() + 10, (*it)->content.v->y);
          m_writer->WriteDouble((*it)->code() + 20, (*it)->content.v->z);
        }
        break;
      case 1040:
      case 1041:
      case 1042:
        if ((*it)->type() == DRW_Variant::Type::Double) { m_writer->WriteDouble((*it)->code(), (*it)->content.d); }
        break;
      case 1070:
        if ((*it)->type() == DRW_Variant::Type::Integer) { m_writer->WriteInt16((*it)->code(), (*it)->content.i); }
        break;
      case 1071:
        if ((*it)->type() == DRW_Variant::Type::Integer) { m_writer->WriteInt32((*it)->code(), (*it)->content.i); }
        break;
      default:
        break;
    }
  }
  return true;
}

/********* Reader Process *********/

bool dxfRW::ProcessDxf() {
  DRW_DBG("<entered dxfRW::ProcessDxf()>\n");
  int code;
  bool more = true;
  std::string sectionstr;
  //    section = secUnknown;
  while (m_reader->ReadRec(&code)) {
    if (code == 999) {
      m_header.addComment(m_reader->GetString());
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
  DRW_DBG("<entered dxfRW::ProcessHeader>\n");
  std::string zeroGroupTag;
  int code{};
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      zeroGroupTag = m_reader->GetString();
      if (zeroGroupTag == "ENDSEC") {
        m_interface->addHeader(&m_header);
        return true;
      }
    } else
      m_header.parseCode(code, m_reader);
  }
  DRW_DBG("<leaving dxfRW::ProcessHeader>\n");
  return true;
}

/********* Classes Section *********/

bool dxfRW::ProcessClasses() {
  DRW_DBG("<entered dxfRW::ProcessClasses>\n");
  std::string zeroGroupTag;
  int code{};
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    if (code == 0) {
      zeroGroupTag = m_reader->GetString();
      if (zeroGroupTag == "CLASS") {
        DRW_Class class_;
        while (m_reader->ReadRec(&code)) {
          if (code == 0) {
            zeroGroupTag = m_reader->GetString();
            if (zeroGroupTag == "CLASS") {
              m_interface->addClass(class_);
              class_.clear();
            } else if (zeroGroupTag == "ENDSEC") {
              m_interface->addClass(class_);
              DRW_DBG("<leaving dxfRW::ProcessClasses>\n");
              return true;
            }
          } else
            class_.parseCode(code, m_reader);
        }
      } else if (zeroGroupTag == "ENDSEC") {
        DRW_DBG("<leaving dxfRW::ProcessClasses>\n");
        return true;
      }
    }
  }
  return false;
}

/********* Tables Section *********/

bool dxfRW::ProcessTables() {
  DRW_DBG("<entered dxfRW::ProcessTables>\n");
  int code;
  std::string sectionstr;
  bool more = true;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
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
        DRW_DBG("<leaving dxfRW::ProcessTables>\n");
        return true;
      }
    }
  }
  return true;
}

bool dxfRW::ProcessLType() {
  DRW_DBG("<entering dxfRW::ProcessLType>\n");
  int code;
  std::string sectionstr;
  bool reading = false;
  DRW_LType ltype;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) {
        ltype.update();
        m_interface->addLType(ltype);
      }
      sectionstr = m_reader->GetString();
      if (sectionstr == "LTYPE") {
        reading = true;
        ltype.reset();
      } else if (sectionstr == "ENDTAB") {
        DRW_DBG("<leaving dxfRW::ProcessLType>\n");
        return true;
      }
    } else if (reading)
      ltype.parseCode(code, m_reader);
  }
  return true;
}

bool dxfRW::ProcessLayer() {
  DRW_DBG("<entering dxfRW::ProcessLayer>\n");
  int code;
  std::string sectionstr;
  bool reading = false;
  DRW_Layer layer;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) m_interface->addLayer(layer);
      sectionstr = m_reader->GetString();
      if (sectionstr == "LAYER") {
        reading = true;
        layer.reset();
      } else if (sectionstr == "ENDTAB") {
        DRW_DBG("<leaving dxfRW::ProcessLayer>\n");
        return true;
      }
    } else if (reading)
      layer.parseCode(code, m_reader);
  }
  return true;
}

bool dxfRW::ProcessDimStyle() {
  DRW_DBG("<entering dxfRW::ProcessDimStyle>");
  int code;
  std::string sectionstr;
  bool reading = false;
  DRW_Dimstyle dimSty;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) m_interface->addDimStyle(dimSty);
      sectionstr = m_reader->GetString();
      if (sectionstr == "DIMSTYLE") {
        reading = true;
        dimSty.reset();
      } else if (sectionstr == "ENDTAB") {
        DRW_DBG("<leaving dxfRW::ProcessDimStyle>\n");
        return true;
      }
    } else if (reading)
      dimSty.parseCode(code, m_reader);
  }
  return true;
}

bool dxfRW::ProcessTextStyle() {
  DRW_DBG("entering dxfRW::ProcessTextStyle");
  int code;
  std::string sectionstr;
  bool reading = false;
  DRW_Textstyle TxtSty;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) m_interface->addTextStyle(TxtSty);
      sectionstr = m_reader->GetString();
      if (sectionstr == "STYLE") {
        reading = true;
        TxtSty.reset();
      } else if (sectionstr == "ENDTAB") {
        DRW_DBG("<leaving dxfRW::ProcessTextStyle>\n");
        return true;
      }
    } else if (reading)
      TxtSty.parseCode(code, m_reader);
  }
  return true;
}

bool dxfRW::ProcessVports() {
  DRW_DBG("entering dxfRW::ProcessVports");
  int code;
  std::string sectionstr;
  bool reading = false;
  DRW_Vport vp;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) m_interface->addVport(vp);
      sectionstr = m_reader->GetString();
      if (sectionstr == "VPORT") {
        reading = true;
        vp.reset();
      } else if (sectionstr == "ENDTAB") {
        DRW_DBG("<leaving dxfRW::ProcessVports>\n");
        return true;
      }
    } else if (reading)
      vp.parseCode(code, m_reader);
  }
  return true;
}

bool dxfRW::ProcessAppId() {
  DRW_DBG("<entering dxfRW::ProcessAppId>");
  int code;
  std::string sectionstr;
  bool reading = false;
  DRW_AppId vp;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) m_interface->addAppId(vp);
      sectionstr = m_reader->GetString();
      if (sectionstr == "APPID") {
        reading = true;
        vp.reset();
      } else if (sectionstr == "ENDTAB") {
        DRW_DBG("<leaving dxfRW::ProcessAppId>\n");
        return true;
      }
    } else if (reading)
      vp.parseCode(code, m_reader);
  }
  return true;
}

/********* Block Section *********/

bool dxfRW::ProcessBlocks() {
  DRW_DBG("entering dxfRW::ProcessBlocks\n");
  int code;
  std::string sectionstr;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    if (code == 0) {
      sectionstr = m_reader->GetString();
      DRW_DBG(sectionstr);
      DRW_DBG("\n");
      if (sectionstr == "BLOCK") {
        ProcessBlock();
      } else if (sectionstr == "ENDSEC") {
        DRW_DBG("<leaving dxfRW::ProcessBlocks>\n");
        return true;
      }
    }
  }
  return true;
}

bool dxfRW::ProcessBlock() {
  DRW_DBG("entering dxfRW::ProcessBlock");
  int code;
  DRW_Block block;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
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
        block.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

/********* Entities Section *********/

bool dxfRW::ProcessEntities(bool isblock) {
  DRW_DBG("entering dxfRW::ProcessEntities\n");
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
      DRW_DBG("<leaving dxfRW::ProcessEntities>\n");
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
      Process3dface();
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
        if (code == 0) m_nextEntity = m_reader->GetString();
      } else
        return false;  // end of file without ENDSEC
    }

  } while (next);
  return true;
}

bool dxfRW::ProcessEllipse() {
  DRW_DBG("dxfRW::ProcessEllipse\n");
  int code;
  DRW_Ellipse ellipse;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        if (m_applyExtrusion) { ellipse.applyExtrusion(); }
        m_interface->addEllipse(ellipse);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        ellipse.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessTrace() {
  DRW_DBG("dxfRW::ProcessTrace\n");
  int code;
  DRW_Trace trace;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        if (m_applyExtrusion) { trace.applyExtrusion(); }
        m_interface->addTrace(trace);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        trace.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessSolid() {
  DRW_DBG("dxfRW::ProcessSolid\n");
  int code;
  DRW_Solid solid;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        if (m_applyExtrusion) { solid.applyExtrusion(); }
        m_interface->addSolid(solid);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        solid.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::Process3dface() {
  DRW_DBG("dxfRW::Process3dface\n");
  int code;
  DRW_3Dface face;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->add3dFace(face);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        face.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessViewport() {
  DRW_DBG("dxfRW::ProcessViewport\n");
  int code;
  DRW_Viewport vp;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->addViewport(vp);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        vp.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessPoint() {
  DRW_DBG("entering dxfRW::ProcessPoint\n");
  int code;
  DRW_Point point;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addPoint(point);
        DRW_DBG("leaving dxfRW::ProcessPoint\n");
        return true;
      }
      default:
        point.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessLine() {
  DRW_DBG("entering dxfRW::ProcessLine\n");
  int code;
  DRW_Line line;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        m_interface->addLine(line);
        DRW_DBG("leaving dxfRW::ProcessLine\n");
        return true;
      }
      default:
        line.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessRay() {
  DRW_DBG("dxfRW::ProcessRay\n");
  int code;
  DRW_Ray line;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->addRay(line);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        line.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessXline() {
  DRW_DBG("dxfRW::ProcessXline\n");
  int code;
  DRW_Xline line;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->addXline(line);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        line.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessCircle() {
  DRW_DBG("dxfRW::ProcessPoint\n");
  int code;
  DRW_Circle circle;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        if (m_applyExtrusion) { circle.applyExtrusion(); }
        m_interface->addCircle(circle);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        circle.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessArc() {
  DRW_DBG("dxfRW::ProcessArc\n");
  int code;
  DRW_Arc arc;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        if (m_applyExtrusion) { arc.applyExtrusion(); }
        m_interface->addArc(arc);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        arc.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessInsert() {
  DRW_DBG("dxfRW::ProcessInsert\n");
  int code;
  DRW_Insert insert;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->addInsert(insert);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        insert.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessLWPolyline() {
  DRW_DBG("dxfRW::ProcessLWPolyline\n");
  int code;
  DRW_LWPolyline pl;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        if (m_applyExtrusion) { pl.applyExtrusion(); }
        m_interface->addLWPolyline(pl);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        pl.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessPolyline() {
  DRW_DBG("dxfRW::ProcessPolyline\n");
  int code;
  DRW_Polyline pl;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        if (m_nextEntity != "VERTEX") {
          m_interface->addPolyline(pl);
          return true;  // found new entity or ENDSEC, terminate
        } else {
          ProcessVertex(&pl);
        }
      }
        [[fallthrough]];  // in case there is no vertex
      default:
        pl.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessVertex(DRW_Polyline* pl) {
  DRW_DBG("dxfRW::ProcessVertex\n");
  int code;
  DRW_Vertex* v = new DRW_Vertex();
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        pl->appendVertex(v);
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        if (m_nextEntity == "SEQEND") {
          return true;  // found SEQEND no more vertex, terminate
        } else if (m_nextEntity == "VERTEX") {
          v = new DRW_Vertex();  // another vertex
        }
      }
      default:
        v->parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessText() {
  DRW_DBG("dxfRW::ProcessText\n");
  int code;
  DRW_Text txt;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->addText(txt);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        txt.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessMText() {
  DRW_DBG("dxfRW::ProcessMText\n");
  int code;
  DRW_MText txt;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        txt.updateAngle();
        m_interface->addMText(txt);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        txt.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessHatch() {
  DRW_DBG("dxfRW::ProcessHatch\n");
  int code;
  DRW_Hatch hatch;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->addHatch(&hatch);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        hatch.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessSpline() {
  DRW_DBG("dxfRW::ProcessSpline\n");
  int code;
  DRW_Spline sp;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->addSpline(&sp);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        sp.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessImage() {
  DRW_DBG("dxfRW::ProcessImage\n");
  int code;
  DRW_Image img;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->addImage(&img);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        img.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessDimension() {
  DRW_DBG("dxfRW::ProcessDimension\n");
  int code;
  DRW_Dimension dim;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        int type = dim.type & 0x0F;
        switch (type) {
          case 0: {
            DRW_DimLinear d(dim);
            m_interface->addDimLinear(&d);
            break;
          }
          case 1: {
            DRW_DimAligned d(dim);
            m_interface->addDimAlign(&d);
            break;
          }
          case 2: {
            DRW_DimAngular d(dim);
            m_interface->addDimAngular(&d);
            break;
          }
          case 3: {
            DRW_DimDiametric d(dim);
            m_interface->addDimDiametric(&d);
            break;
          }
          case 4: {
            DRW_DimRadial d(dim);
            m_interface->addDimRadial(&d);
            break;
          }
          case 5: {
            DRW_DimAngular3p d(dim);
            m_interface->addDimAngular3P(&d);
            break;
          }
          case 6: {
            DRW_DimOrdinate d(dim);
            m_interface->addDimOrdinate(&d);
            break;
          }
        }
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        dim.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool dxfRW::ProcessLeader() {
  DRW_DBG("dxfRW::ProcessLeader\n");
  int code;
  DRW_Leader leader;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->addLeader(&leader);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        leader.parseCode(code, m_reader);
        break;
    }
  }
  return true;
}

/********* Objects Section *********/

bool dxfRW::ProcessObjects() {
  DRW_DBG("dxfRW::ProcessObjects\n");
  int code;
  if (!m_reader->ReadRec(&code)) { return false; }
  bool next = true;
  if (code == 0) {
    m_nextEntity = m_reader->GetString();
  } else {
    return false;  // first record in objects is 0
  }
  do {
    if (m_nextEntity == "ENDSEC") {
      return true;  // found ENDSEC terminate
    } else if (m_nextEntity == "IMAGEDEF") {
      ProcessImageDef();
    } else {
      if (m_reader->ReadRec(&code)) {
        if (code == 0) m_nextEntity = m_reader->GetString();
      } else
        return false;  // end of file without ENDSEC
    }

  } while (next);
  return true;
}

bool dxfRW::ProcessImageDef() {
  DRW_DBG("dxfRW::ProcessImageDef\n");
  int code;
  DRW_ImageDef img;
  while (m_reader->ReadRec(&code)) {
    DRW_DBG(code);
    DRW_DBG("\n");
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetString();
        DRW_DBG(m_nextEntity);
        DRW_DBG("\n");
        m_interface->linkImage(&img);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        img.parseCode(code, m_reader);
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
