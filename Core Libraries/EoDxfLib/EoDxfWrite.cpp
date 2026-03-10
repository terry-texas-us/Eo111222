#include <fstream>
#include <ios>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "EoDxfWrite.h"

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfHeader.h"
#include "EoDxfInterface.h"
#include "EoDxfObjects.h"
#include "EoDxfWriter.h"

namespace {
constexpr auto EODXFLIB_VERSION = "0.1";
constexpr auto FIRSTHANDLE{48};
}  // namespace

EoDxfWrite::EoDxfWrite(const char* name) {
  m_fileName = name;
  m_writer = nullptr;
}

EoDxfWrite::~EoDxfWrite() {
  if (m_writer != nullptr) { delete m_writer; }
  for (std::vector<EoDxfImageDefinition*>::iterator it = m_imageDef.begin(); it != m_imageDef.end(); ++it) {
    delete *it;
  }

  m_imageDef.clear();
}

bool EoDxfWrite::Write(EoDxfInterface* interface_, EoDxf::Version version, bool binaryFile) {
  bool isOk = false;
  std::ofstream filestr;
  m_version = version;
  m_binaryFile = binaryFile;
  m_interface = interface_;
  if (m_binaryFile) {
    filestr.open(m_fileName.c_str(), std::ios_base::out | std::ios::binary | std::ios::trunc);
    // write sentinel
    filestr << "AutoCAD Binary DXF\r\n" << (char)26 << '\0';
    m_writer = new EoDxfWriterBinary(&filestr);
  } else {
    filestr.open(m_fileName.c_str(), std::ios_base::out | std::ios::trunc);
    m_writer = new EoDxfWriterAscii(&filestr);
    std::string comm = std::string("EoDxf ") + std::string(EODXFLIB_VERSION);
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

bool EoDxfWrite::WriteEntity(EoDxfEntity* entity) {
  entity->m_handle = ++m_entityCount;
  m_writer->WriteString(5, ToHexString(entity->m_handle));
  m_writer->WriteString(100, "AcDbEntity");
  if (entity->m_space == EoDxf::Space::PaperSpace) { m_writer->WriteInt16(67, 1); }

  m_writer->WriteUtf8String(8, entity->m_layer);
  m_writer->WriteUtf8String(6, entity->m_lineType);

  m_writer->WriteInt16(62, entity->m_color);
  if (m_version > EoDxf::Version::AC1015 && entity->m_color24 >= 0) { m_writer->WriteInt32(420, entity->m_color24); }
  if (m_version > EoDxf::Version::AC1014) {
    m_writer->WriteInt16(370, EoDxfLineWidths::lineWidth2dxfInt(entity->m_lineWeight));
  }
  return true;
}

bool EoDxfWrite::WriteInsert(EoDxfInsert* blockReference) {
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
  m_writer->WriteDouble(
      50, (blockReference->m_rotationAngle) * EoDxf::RadiansToDegrees);  // in dxf angle is written in degrees
  m_writer->WriteInt16(70, blockReference->m_columnCount);
  m_writer->WriteInt16(71, blockReference->m_rowCount);
  m_writer->WriteDouble(44, blockReference->m_columnSpacing);
  m_writer->WriteDouble(45, blockReference->m_rowSpacing);
  return true;
}

bool EoDxfWrite::WriteViewport(EoDxfViewport* viewport) {
  m_writer->WriteString(0, "VIEWPORT");
  WriteEntity(viewport);
  m_writer->WriteString(100, "AcDbViewport");
  m_writer->WriteDouble(10, viewport->m_firstPoint.x);
  m_writer->WriteDouble(20, viewport->m_firstPoint.y);
  if (viewport->m_firstPoint.z != 0.0) { m_writer->WriteDouble(30, viewport->m_firstPoint.z); }
  m_writer->WriteDouble(40, viewport->m_width);
  m_writer->WriteDouble(41, viewport->m_height);
  if (viewport->m_viewportStatus != 0) { m_writer->WriteInt16(68, viewport->m_viewportStatus); }
  if (viewport->m_viewportId != 0) { m_writer->WriteInt16(69, viewport->m_viewportId); }
  
  if (!viewport->m_viewCenter.IsZero()) {
    m_writer->WriteDouble(12, viewport->m_viewCenter.x);
    m_writer->WriteDouble(22, viewport->m_viewCenter.y);
  }
  if (!viewport->m_snapBasePoint.IsZero()) {
    m_writer->WriteDouble(13, viewport->m_snapBasePoint.x);
    m_writer->WriteDouble(23, viewport->m_snapBasePoint.y);
  }
  if (!viewport->m_snapSpacing.IsZero()) {
    m_writer->WriteDouble(14, viewport->m_snapSpacing.x);
    m_writer->WriteDouble(24, viewport->m_snapSpacing.y);
  }
  if (!viewport->m_gridSpacing.IsZero()) {
    m_writer->WriteDouble(15, viewport->m_gridSpacing.x);
    m_writer->WriteDouble(25, viewport->m_gridSpacing.y);
  }
  if (!viewport->m_viewDirection.IsDefaultNormal()) {
    m_writer->WriteDouble(16, viewport->m_viewDirection.x);
    m_writer->WriteDouble(26, viewport->m_viewDirection.y);
    m_writer->WriteDouble(36, viewport->m_viewDirection.z);
  }
  if (!viewport->m_viewTargetPoint.IsZero()) {
    m_writer->WriteDouble(17, viewport->m_viewTargetPoint.x);
    m_writer->WriteDouble(27, viewport->m_viewTargetPoint.y);
    m_writer->WriteDouble(37, viewport->m_viewTargetPoint.z);
  }
  if (viewport->m_lensLength != 0.0) { m_writer->WriteDouble(42, viewport->m_lensLength); }
  if (viewport->m_frontClipPlane != 0.0) { m_writer->WriteDouble(43, viewport->m_frontClipPlane); }
  if (viewport->m_backClipPlane != 0.0) { m_writer->WriteDouble(44, viewport->m_backClipPlane); }
  if (viewport->m_viewHeight != 0.0) { m_writer->WriteDouble(45, viewport->m_viewHeight); }
  if (viewport->m_snapAngle != 0.0) { m_writer->WriteDouble(50, viewport->m_snapAngle); }
  if (viewport->m_twistAngle != 0.0) { m_writer->WriteDouble(51, viewport->m_twistAngle); }
  return true;
}

EoDxfImageDefinition* EoDxfWrite::WriteImage(EoDxfImage* rasterImage, std::string name) {
  // search if exist imagedef with this name (image inserted more than 1 time)
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

bool EoDxfWrite::WriteBlockRecord(std::string name) {
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

bool EoDxfWrite::WriteBlock(EoDxfBlock* block) {
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

bool EoDxfWrite::WriteBlocks() {
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

bool EoDxfWrite::WriteObjects() {
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

bool EoDxfWrite::WriteExtData(const std::vector<EoDxfGroupCodeValuesVariant*>& ed) {
  for (std::vector<EoDxfGroupCodeValuesVariant*>::const_iterator it = ed.begin(); it != ed.end(); ++it) {
    switch ((*it)->Code()) {
      case 1000:
      case 1001:
      case 1002:
      case 1003:
      case 1004:
      case 1005: {
        int cc = (*it)->Code();
        if ((*it)->GetType() == EoDxfGroupCodeValuesVariant::Type::String) {
          m_writer->WriteUtf8String(cc, *(*it)->m_content.s);
        }
        //            m_writer->WriteUtf8String((*it)->code, (*it)->content.s);
        break;
      }
      case 1010:
      case 1011:
      case 1012:
      case 1013:
        if ((*it)->GetType() == EoDxfGroupCodeValuesVariant::Type::GeometryBase) {
          m_writer->WriteDouble((*it)->Code(), (*it)->m_content.v->x);
          m_writer->WriteDouble((*it)->Code() + 10, (*it)->m_content.v->y);
          m_writer->WriteDouble((*it)->Code() + 20, (*it)->m_content.v->z);
        }
        break;
      case 1040:
      case 1041:
      case 1042:
        if ((*it)->GetType() == EoDxfGroupCodeValuesVariant::Type::Double) {
          m_writer->WriteDouble((*it)->Code(), (*it)->GetDouble());
        }
        break;
      case 1070:
        if ((*it)->GetType() == EoDxfGroupCodeValuesVariant::Type::Integer) {
          m_writer->WriteInt16((*it)->Code(), (*it)->GetInteger());
        }
        break;
      case 1071:
        if ((*it)->GetType() == EoDxfGroupCodeValuesVariant::Type::Integer) {
          m_writer->WriteInt32((*it)->Code(), (*it)->GetInteger());
        }
        break;
      default:
        break;
    }
  }
  return true;
}

std::string EoDxfWrite::ToHexString(uint64_t hexValue) {
  std::ostringstream convert;
  convert << std::uppercase << std::hex << hexValue;
  return convert.str();
}
