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
  delete m_writer;
  m_writer = nullptr;

  std::ofstream filestr;
  m_version = version;
  m_binaryFile = binaryFile;
  m_interface = interface_;
  m_writeOk = (m_interface != nullptr);
  if (!m_writeOk) { return false; }

  if (m_binaryFile) {
    filestr.open(m_fileName.c_str(), std::ios_base::out | std::ios::binary | std::ios::trunc);
    TrackStreamState(filestr);
    if (m_writeOk) {
      filestr << "AutoCAD Binary DXF\r\n" << (char)26 << '\0';
      TrackStreamState(filestr);
      m_writer = new EoDxfWriterBinary(&filestr);
    }
  } else {
    filestr.open(m_fileName.c_str(), std::ios_base::out | std::ios::trunc);
    TrackStreamState(filestr);
    if (m_writeOk) {
      m_writer = new EoDxfWriterAscii(&filestr);
      const std::string comm = std::string("EoDxf ") + EODXFLIB_VERSION;
      WriteCodeString(999, comm);
    }
  }
  if (m_writer == nullptr) {
    if (filestr.is_open()) { filestr.close(); }
    return false;
  }

  EoDxfHeader header;
  m_interface->writeHeader(header);
  m_entityCount = FIRSTHANDLE;
  WriteCodeString(0, "SECTION");
  header.Write(m_writer, m_version);
  TrackStreamState(filestr);
  WriteCodeString(0, "ENDSEC");

  WriteCodeString(0, "SECTION");
  WriteCodeString(2, "CLASSES");
  WriteCodeString(0, "ENDSEC");

  WriteCodeString(0, "SECTION");
  WriteCodeString(2, "TABLES");
  TrackWriteResult(WriteTables());
  WriteCodeString(0, "ENDSEC");
  WriteCodeString(0, "SECTION");
  WriteCodeString(2, "BLOCKS");
  TrackWriteResult(WriteBlocks());
  WriteCodeString(0, "ENDSEC");

  WriteCodeString(0, "SECTION");
  WriteCodeString(2, "ENTITIES");
  m_interface->writeEntities();
  TrackStreamState(filestr);
  WriteCodeString(0, "ENDSEC");

  WriteCodeString(0, "SECTION");
  WriteCodeString(2, "OBJECTS");
  TrackWriteResult(WriteObjects());
  WriteCodeString(0, "ENDSEC");

  WriteCodeString(0, "EOF");
  filestr.flush();
  TrackStreamState(filestr);
  filestr.close();
  if (filestr.fail()) { TrackWriteResult(false); }

  delete m_writer;
  m_writer = nullptr;
  return m_writeOk;
}

bool EoDxfWrite::WriteEntity(EoDxfEntity* entity) {
  entity->m_handle = ++m_entityCount;
  WriteCodeString(5, ToHexString(entity->m_handle));
  WriteCodeString(100, "AcDbEntity");
  if (entity->m_space == EoDxf::Space::PaperSpace) { WriteCodeInt16(67, 1); }

  WriteCodeUtf8String(8, entity->m_layer);
  WriteCodeUtf8String(6, entity->m_lineType);

  WriteCodeInt16(62, entity->m_color);
  if (m_version > EoDxf::Version::AC1015 && entity->m_color24 >= 0) { WriteCodeInt32(420, entity->m_color24); }
  if (m_version > EoDxf::Version::AC1014) {
    WriteCodeInt16(370, EoDxfLineWidths::lineWidth2dxfInt(entity->m_lineWeight));
  }
  return m_writeOk;
}

bool EoDxfWrite::WriteInsert(EoDxfInsert* blockReference) {
  WriteCodeString(0, "INSERT");
  WriteEntity(blockReference);

  WriteCodeString(100, "AcDbBlockReference");
  WriteCodeUtf8String(2, blockReference->m_blockName);

  WriteCodeDouble(10, blockReference->m_firstPoint.x);
  WriteCodeDouble(20, blockReference->m_firstPoint.y);
  WriteCodeDouble(30, blockReference->m_firstPoint.z);
  WriteCodeDouble(41, blockReference->m_xScaleFactor);
  WriteCodeDouble(42, blockReference->m_yScaleFactor);
  WriteCodeDouble(43, blockReference->m_zScaleFactor);
  WriteCodeDouble(
      50, (blockReference->m_rotationAngle) * EoDxf::RadiansToDegrees);  // in dxf angle is written in degrees
  WriteCodeInt16(70, blockReference->m_columnCount);
  WriteCodeInt16(71, blockReference->m_rowCount);
  WriteCodeDouble(44, blockReference->m_columnSpacing);
  WriteCodeDouble(45, blockReference->m_rowSpacing);
  return m_writeOk;
}

bool EoDxfWrite::WriteViewport(EoDxfViewport* viewport) {
  WriteCodeString(0, "VIEWPORT");
  WriteEntity(viewport);
  WriteCodeString(100, "AcDbViewport");
  WriteCodeDouble(10, viewport->m_firstPoint.x);
  WriteCodeDouble(20, viewport->m_firstPoint.y);
  if (viewport->m_firstPoint.z != 0.0) { WriteCodeDouble(30, viewport->m_firstPoint.z); }
  WriteCodeDouble(40, viewport->m_width);
  WriteCodeDouble(41, viewport->m_height);
  if (viewport->m_viewportStatus != 0) { WriteCodeInt16(68, viewport->m_viewportStatus); }
  if (viewport->m_viewportId != 0) { WriteCodeInt16(69, viewport->m_viewportId); }

  if (!viewport->m_viewCenter.IsZero()) {
    WriteCodeDouble(12, viewport->m_viewCenter.x);
    WriteCodeDouble(22, viewport->m_viewCenter.y);
  }
  if (!viewport->m_snapBasePoint.IsZero()) {
    WriteCodeDouble(13, viewport->m_snapBasePoint.x);
    WriteCodeDouble(23, viewport->m_snapBasePoint.y);
  }
  if (!viewport->m_snapSpacing.IsZero()) {
    WriteCodeDouble(14, viewport->m_snapSpacing.x);
    WriteCodeDouble(24, viewport->m_snapSpacing.y);
  }
  if (!viewport->m_gridSpacing.IsZero()) {
    WriteCodeDouble(15, viewport->m_gridSpacing.x);
    WriteCodeDouble(25, viewport->m_gridSpacing.y);
  }
  if (!viewport->m_viewDirection.IsDefaultNormal()) {
    WriteCodeDouble(16, viewport->m_viewDirection.x);
    WriteCodeDouble(26, viewport->m_viewDirection.y);
    WriteCodeDouble(36, viewport->m_viewDirection.z);
  }
  if (!viewport->m_viewTargetPoint.IsZero()) {
    WriteCodeDouble(17, viewport->m_viewTargetPoint.x);
    WriteCodeDouble(27, viewport->m_viewTargetPoint.y);
    WriteCodeDouble(37, viewport->m_viewTargetPoint.z);
  }
  if (viewport->m_lensLength != 0.0) { WriteCodeDouble(42, viewport->m_lensLength); }
  if (viewport->m_frontClipPlane != 0.0) { WriteCodeDouble(43, viewport->m_frontClipPlane); }
  if (viewport->m_backClipPlane != 0.0) { WriteCodeDouble(44, viewport->m_backClipPlane); }
  if (viewport->m_viewHeight != 0.0) { WriteCodeDouble(45, viewport->m_viewHeight); }
  if (viewport->m_snapAngle != 0.0) { WriteCodeDouble(50, viewport->m_snapAngle); }
  if (viewport->m_twistAngle != 0.0) { WriteCodeDouble(51, viewport->m_twistAngle); }
  return m_writeOk;
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

  WriteCodeString(0, "IMAGE");
  WriteEntity(rasterImage);
  WriteCodeString(100, "AcDbRasterImage");
  WriteCodeDouble(10, rasterImage->m_firstPoint.x);
  WriteCodeDouble(20, rasterImage->m_firstPoint.y);
  WriteCodeDouble(30, rasterImage->m_firstPoint.z);
  WriteCodeDouble(11, rasterImage->m_secondPoint.x);
  WriteCodeDouble(21, rasterImage->m_secondPoint.y);
  WriteCodeDouble(31, rasterImage->m_secondPoint.z);
  WriteCodeDouble(12, rasterImage->vVector.x);
  WriteCodeDouble(22, rasterImage->vVector.y);
  WriteCodeDouble(32, rasterImage->vVector.z);
  WriteCodeDouble(13, rasterImage->sizeu);
  WriteCodeDouble(23, rasterImage->sizev);
  WriteCodeString(340, ToHexString(id->m_handle));
  WriteCodeInt16(70, 1);
  WriteCodeInt16(280, rasterImage->clip);
  WriteCodeInt16(281, rasterImage->brightness);
  WriteCodeInt16(282, rasterImage->contrast);
  WriteCodeInt16(283, rasterImage->fade);
  WriteCodeString(360, idReactor);
  id->reactors[idReactor] = ToHexString(rasterImage->m_handle);
  return id;
}

bool EoDxfWrite::WriteBlockRecord(std::string name) {
  WriteCodeString(0, "BLOCK_RECORD");
  WriteCodeString(5, ToHexString(++m_entityCount));

  m_blockMap[name] = m_entityCount;
  m_entityCount = 2 + m_entityCount;  // reserve 2 for BLOCK & ENDBLOCK
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "1"); }
  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbBlockTableRecord");
  WriteCodeUtf8String(2, name);
  if (m_version > EoDxf::Version::AC1018) {
    //    m_writer->WriteInt16(340, 22);
    WriteCodeInt16(70, 0);
    WriteCodeInt16(280, 1);
    WriteCodeInt16(281, 0);
  }

  return m_writeOk;
}

bool EoDxfWrite::WriteBlock(EoDxfBlock* block) {
  if (m_writingBlock) {
    WriteCodeString(0, "ENDBLK");

    WriteCodeString(5, ToHexString(m_currentHandle + 2));
    if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, ToHexString(m_currentHandle)); }
    WriteCodeString(100, "AcDbEntity");

    WriteCodeString(8, "0");
    WriteCodeString(100, "AcDbBlockEnd");
  }
  m_writingBlock = true;
  WriteCodeString(0, "BLOCK");

  m_currentHandle = (*(m_blockMap.find(block->name))).second;
  WriteCodeString(5, ToHexString(m_currentHandle + 1));
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, ToHexString(m_currentHandle)); }
  WriteCodeString(100, "AcDbEntity");

  WriteCodeString(8, "0");

  WriteCodeString(100, "AcDbBlockBegin");
  WriteCodeUtf8String(2, block->name);

  WriteCodeInt16(70, block->m_flags);
  WriteCodeDouble(10, block->m_firstPoint.x);
  WriteCodeDouble(20, block->m_firstPoint.y);
  if (block->m_firstPoint.z != 0.0) { WriteCodeDouble(30, block->m_firstPoint.z); }

  WriteCodeUtf8String(3, block->name);

  WriteCodeString(1, "");

  return m_writeOk;
}

bool EoDxfWrite::WriteBlocks() {
  WriteCodeString(0, "BLOCK");

  WriteCodeString(5, "20");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "1F"); }
  WriteCodeString(100, "AcDbEntity");

  WriteCodeString(8, "0");

  WriteCodeString(100, "AcDbBlockBegin");
  WriteCodeString(2, "*Model_Space");

  WriteCodeInt16(70, 0);
  WriteCodeDouble(10, 0.0);
  WriteCodeDouble(20, 0.0);
  WriteCodeDouble(30, 0.0);

  WriteCodeString(3, "*Model_Space");

  WriteCodeString(1, "");
  WriteCodeString(0, "ENDBLK");

  WriteCodeString(5, "21");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "1F"); }
  WriteCodeString(100, "AcDbEntity");

  WriteCodeString(8, "0");
  WriteCodeString(100, "AcDbBlockEnd");
  WriteCodeString(0, "BLOCK");

  WriteCodeString(5, "1C");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "1B"); }
  WriteCodeString(100, "AcDbEntity");

  WriteCodeString(8, "0");

  WriteCodeString(100, "AcDbBlockBegin");
  WriteCodeString(2, "*Paper_Space");

  WriteCodeInt16(70, 0);
  WriteCodeDouble(10, 0.0);
  WriteCodeDouble(20, 0.0);
  WriteCodeDouble(30, 0.0);

  WriteCodeString(3, "*Paper_Space");

  WriteCodeString(1, "");
  WriteCodeString(0, "ENDBLK");

  WriteCodeString(5, "1D");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "1F"); }
  WriteCodeString(100, "AcDbEntity");

  WriteCodeString(8, "0");
  WriteCodeString(100, "AcDbBlockEnd");
  m_writingBlock = false;
  m_interface->writeBlocks();
  if (m_writingBlock) {
    m_writingBlock = false;
    WriteCodeString(0, "ENDBLK");

    WriteCodeString(5, ToHexString(m_currentHandle + 2));
    //            m_writer->WriteString(5, "1D");
    if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, ToHexString(m_currentHandle)); }
    WriteCodeString(100, "AcDbEntity");

    WriteCodeString(8, "0");
    WriteCodeString(100, "AcDbBlockEnd");
  }
  return m_writeOk;
}

bool EoDxfWrite::WriteObjects() {
  WriteCodeString(0, "DICTIONARY");
  std::string imgDictH;
  WriteCodeString(5, "C");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "0"); }
  WriteCodeString(100, "AcDbDictionary");
  WriteCodeInt16(281, 1);
  WriteCodeString(3, "ACAD_GROUP");
  WriteCodeString(350, "D");
  if (m_imageDef.size() != 0) {
    WriteCodeString(3, "ACAD_IMAGE_DICT");
    imgDictH = ToHexString(++m_entityCount);
    WriteCodeString(350, imgDictH);
  }
  WriteCodeString(0, "DICTIONARY");
  WriteCodeString(5, "D");
  WriteCodeString(330, "C");
  WriteCodeString(100, "AcDbDictionary");
  WriteCodeInt16(281, 1);
  // write IMAGEDEF_REACTOR
  for (unsigned int i = 0; i < m_imageDef.size(); i++) {
    EoDxfImageDefinition* id = m_imageDef.at(i);
    std::map<std::string, std::string>::iterator it;
    for (it = id->reactors.begin(); it != id->reactors.end(); ++it) {
      WriteCodeString(0, "IMAGEDEF_REACTOR");
      WriteCodeString(5, it->first);
      WriteCodeString(330, it->second);
      WriteCodeString(100, "AcDbRasterImageDefReactor");
      WriteCodeInt32(90, 2);  // version 2=R14 to v2010
      WriteCodeString(330, it->second);
    }
  }
  if (m_imageDef.size() != 0) {
    WriteCodeString(0, "DICTIONARY");
    WriteCodeString(5, imgDictH);
    WriteCodeString(330, "C");
    WriteCodeString(100, "AcDbDictionary");
    WriteCodeInt16(281, 1);
    for (unsigned int i = 0; i < m_imageDef.size(); i++) {
      size_t f1, f2;
      f1 = m_imageDef.at(i)->m_fileNameOfImage.find_last_of("/\\");
      f2 = m_imageDef.at(i)->m_fileNameOfImage.find_last_of('.');
      ++f1;
      WriteCodeString(3, m_imageDef.at(i)->m_fileNameOfImage.substr(f1, f2 - f1));
      WriteCodeString(350, ToHexString(m_imageDef.at(i)->m_handle));
    }
  }
  for (unsigned int i = 0; i < m_imageDef.size(); i++) {
    EoDxfImageDefinition* id = m_imageDef.at(i);
    WriteCodeString(0, "IMAGEDEF");
    WriteCodeString(5, ToHexString(id->m_handle));
    if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, imgDictH); }
    WriteCodeString(102, "{ACAD_REACTORS");
    std::map<std::string, std::string>::iterator it;
    for (it = id->reactors.begin(); it != id->reactors.end(); ++it) { WriteCodeString(330, it->first); }
    WriteCodeString(102, "}");
    WriteCodeString(100, "AcDbRasterImageDef");
    WriteCodeInt32(90, 0);  // version 0=R14 to v2010
    WriteCodeUtf8String(1, id->m_fileNameOfImage);
    WriteCodeDouble(10, id->m_uImageSizeInPixels);
    WriteCodeDouble(20, id->m_vImageSizeInPixels);
    WriteCodeDouble(11, id->m_uSizeOfOnePixel);
    WriteCodeDouble(21, id->m_vSizeOfOnePixel);
    WriteCodeInt16(280, id->m_imageIsLoadedFlag);
    WriteCodeInt16(281, id->m_resolutionUnits);
  }
  // no more needed imageDef, delete it
  for (auto* id_ : m_imageDef) { delete id_; }
  m_imageDef.clear();

  return m_writeOk;
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
        if (const auto* value = (*it)->GetIf<std::wstring>()) {
          WriteCodeWideString(cc, *value);
        } else if (const auto* value = (*it)->GetIf<std::string>()) {
          WriteCodeUtf8String(cc, *value);
        }
        //            m_writer->WriteUtf8String((*it)->code, (*it)->content.s);
        break;
      }
      case 1010:
      case 1011:
      case 1012:
      case 1013:
        if (const auto* geometryBase = (*it)->GetIf<EoDxfGeometryBase3d>()) {
          WriteCodeDouble((*it)->Code(), geometryBase->x);
          WriteCodeDouble((*it)->Code() + 10, geometryBase->y);
          WriteCodeDouble((*it)->Code() + 20, geometryBase->z);
        }
        break;
      case 1040:
      case 1041:
      case 1042:
        if (const auto* value = (*it)->GetIf<double>()) { WriteCodeDouble((*it)->Code(), *value); }
        break;
      case 1070:
        if (const auto* value = (*it)->GetIf<std::int16_t>()) {
          WriteCodeInt16((*it)->Code(), *value);
        } else if (const auto* value = (*it)->GetIf<bool>()) {
          WriteCodeInt16((*it)->Code(), *value ? 1 : 0);
        }
        break;
      case 1071:
        if (const auto* value = (*it)->GetIf<std::int32_t>()) { WriteCodeInt32((*it)->Code(), *value); }
        break;
      default:
        break;
    }
  }
  return m_writeOk;
}

std::string EoDxfWrite::ToHexString(uint64_t hexValue) {
  std::ostringstream convert;
  convert << std::uppercase << std::hex << hexValue;
  return convert.str();
}
