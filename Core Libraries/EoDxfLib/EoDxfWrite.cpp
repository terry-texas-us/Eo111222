#include <fstream>
#include <filesystem>
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
constexpr std::wstring_view EODXFLIB_VERSION{L"0.1"};
constexpr auto FIRSTHANDLE{48};

[[nodiscard]] std::wstring GetRequestedCodePage(const EoDxfHeader& header) {
  if (!header.GetCodePageToken().empty()) { return header.GetCodePageToken(); }
  return L"ANSI_1252";
}

[[nodiscard]] bool IsUtf16CodePage(std::wstring_view codePage) {
  EoTcTextCodec textCodec;
  textCodec.SetCodePage(codePage);
  return textCodec.GetCodePage() == L"UTF-16";
}

[[nodiscard]] std::wstring GetActiveWriteCodePage(std::wstring_view codePage, bool binaryFile) {
  EoTcTextCodec textCodec;
  textCodec.SetCodePage(codePage);
  if (!binaryFile && textCodec.GetCodePage() == L"UTF-16") { return L"ANSI_1252"; }
  return textCodec.GetCodePage();
}
}  // namespace

EoDxfWrite::EoDxfWrite(const std::filesystem::path& fileName) {
  m_fileName = fileName.wstring();
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

  EoDxfHeader header;
  if (interface_ != nullptr) { interface_->writeHeader(header); }

  std::ofstream filestr;
  m_version = version;
  m_binaryFile = binaryFile;
  m_interface = interface_;
  m_writeOk = (m_interface != nullptr);
  if (!m_writeOk) { return false; }
  if (m_binaryFile && IsUtf16CodePage(GetRequestedCodePage(header))) {
    m_writeOk = false;
    return false;
  }

  if (m_binaryFile) {
      filestr.open(std::filesystem::path{m_fileName}, std::ios_base::out | std::ios::binary | std::ios::trunc);
    TrackStreamState(filestr);
    if (m_writeOk) {
      filestr << "AutoCAD Binary DXF\r\n" << (char)26 << '\0';
      TrackStreamState(filestr);
      m_writer = new EoDxfWriterBinary(&filestr);
    }
  } else {
    filestr.open(std::filesystem::path{m_fileName}, std::ios_base::out | std::ios::trunc);
    TrackStreamState(filestr);
    if (m_writeOk) {
      m_writer = new EoDxfWriterAscii(&filestr);
    }
  }
  if (m_writer == nullptr) {
    if (filestr.is_open()) { filestr.close(); }
    return false;
  }

  m_writer->SetCodePage(GetActiveWriteCodePage(GetRequestedCodePage(header), m_binaryFile));

  const auto preservedComments = header.GetComments();
  if (!preservedComments.empty()) {
    std::size_t lineStart{};
    while (lineStart <= preservedComments.size()) {
      const auto lineEnd = preservedComments.find(L'\n', lineStart);
      auto commentLine =
          lineEnd == std::wstring::npos ? preservedComments.substr(lineStart) : preservedComments.substr(lineStart, lineEnd - lineStart);
      if (!commentLine.empty() && commentLine.back() == L'\r') { commentLine.pop_back(); }
      WriteCodeString(999, commentLine);
      if (lineEnd == std::wstring::npos) { break; }
      lineStart = lineEnd + 1;
    }
  }
  if (!m_binaryFile) {
    const std::wstring comment = std::wstring{L"EoDxf "} + std::wstring{EODXFLIB_VERSION};
    WriteCodeString(999, comment);
  }

  m_entityCount = FIRSTHANDLE;
   WriteCodeString(0, L"SECTION");
  header.Write(m_writer, m_version);
  TrackStreamState(filestr);
   WriteCodeString(0, L"ENDSEC");

   WriteCodeString(0, L"SECTION");
   WriteCodeString(2, L"CLASSES");
   WriteCodeString(0, L"ENDSEC");

   WriteCodeString(0, L"SECTION");
   WriteCodeString(2, L"TABLES");
  TrackWriteResult(WriteTables());
   WriteCodeString(0, L"ENDSEC");
   WriteCodeString(0, L"SECTION");
   WriteCodeString(2, L"BLOCKS");
  TrackWriteResult(WriteBlocks());
   WriteCodeString(0, L"ENDSEC");

   WriteCodeString(0, L"SECTION");
   WriteCodeString(2, L"ENTITIES");
  m_interface->writeEntities();
  TrackStreamState(filestr);
   WriteCodeString(0, L"ENDSEC");

   WriteCodeString(0, L"SECTION");
   WriteCodeString(2, L"OBJECTS");
  TrackWriteResult(WriteObjects());
   WriteCodeString(0, L"ENDSEC");

   WriteCodeString(0, L"EOF");
  filestr.flush();
  TrackStreamState(filestr);
  filestr.close();
  if (filestr.fail()) { TrackWriteResult(false); }

  delete m_writer;
  m_writer = nullptr;
  return m_writeOk;
}

bool EoDxfWrite::WriteClass(EoDxfClass* class_) {
  if (class_ == nullptr || m_writer == nullptr) { return false; }
  class_->write(m_writer, m_version);
  return m_writeOk;
}

void EoDxfWrite::AddImageDefinition(const EoDxfImageDefinition& imageDefinition) {
  auto* preservedImageDefinition = new EoDxfImageDefinition(imageDefinition);
  m_imageDef.push_back(preservedImageDefinition);
}

bool EoDxfWrite::WriteUnsupportedObject(const EoDxfUnsupportedObject& objectData) {
  objectData.Write(m_writer);
  return m_writeOk;
}

bool EoDxfWrite::WriteEntity(EoDxfGraphic* entity) {
  entity->m_handle = ++m_entityCount;
  WriteCodeString(5, ToHexString(entity->m_handle));
  WriteCodeString(100, L"AcDbEntity");
  if (entity->m_space == EoDxf::Space::PaperSpace) { WriteCodeInt16(67, 1); }

  WriteCodeWideString(8, entity->m_layer);
  WriteCodeWideString(6, entity->m_lineType);

  WriteCodeInt16(62, entity->m_color);
  if (m_version > EoDxf::Version::AC1015 && entity->m_color24 >= 0) { WriteCodeInt32(420, entity->m_color24); }
  if (m_version > EoDxf::Version::AC1015 && !entity->m_colorName.empty()) { WriteCodeWideString(430, entity->m_colorName); }
  if (m_version > EoDxf::Version::AC1014) {
    WriteCodeInt16(370, EoDxfLineWidths::LineWidthToDxfIndex(entity->m_lineWeight));
  }
  if (!entity->m_appData.empty()) { WriteAppData(entity->m_appData); }
  if (!entity->m_extendedData.empty()) { WriteExtData(entity->m_extendedData); }
  return m_writeOk;
}

bool EoDxfWrite::WriteInsert(EoDxfInsert* blockReference) {
  WriteCodeString(0, L"INSERT");
  WriteEntity(blockReference);

  WriteCodeString(100, L"AcDbBlockReference");
  WriteCodeWideString(2, blockReference->m_blockName);

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
  WriteCodeString(0, L"VIEWPORT");
  WriteEntity(viewport);
  WriteCodeString(100, L"AcDbViewport");
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

EoDxfImageDefinition* EoDxfWrite::WriteImage(EoDxfImage* rasterImage, std::wstring_view name) {
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
  std::wstring idReactor = ToHexString(++m_entityCount);

  WriteCodeString(0, L"IMAGE");
  WriteEntity(rasterImage);
  WriteCodeString(100, L"AcDbRasterImage");
  WriteCodeDouble(10, rasterImage->m_startPoint.x);
  WriteCodeDouble(20, rasterImage->m_startPoint.y);
  WriteCodeDouble(30, rasterImage->m_startPoint.z);
  WriteCodeDouble(11, rasterImage->m_endPoint.x);
  WriteCodeDouble(21, rasterImage->m_endPoint.y);
  WriteCodeDouble(31, rasterImage->m_endPoint.z);
  WriteCodeDouble(12, rasterImage->vVector.x);
  WriteCodeDouble(22, rasterImage->vVector.y);
  WriteCodeDouble(32, rasterImage->vVector.z);
  WriteCodeDouble(13, rasterImage->sizeu);
  WriteCodeDouble(23, rasterImage->sizev);
  WriteCodeString(340, ToHexString(id->m_handle));
  WriteCodeInt16(70, 1);
  WriteCodeInt16(280, rasterImage->m_clippingState);
  WriteCodeInt16(281, rasterImage->m_brightnessValue);
  WriteCodeInt16(282, rasterImage->m_contrastValue);
  WriteCodeInt16(283, rasterImage->m_fadeValue);
  WriteCodeString(360, idReactor);
  id->reactors[idReactor] = ToHexString(rasterImage->m_handle);
  return id;
}

bool EoDxfWrite::WriteBlockRecord(std::wstring_view name) {
  WriteCodeString(0, L"BLOCK_RECORD");
  WriteCodeString(5, ToHexString(++m_entityCount));

  m_blockMap.emplace(name, m_entityCount);
  m_entityCount = 2 + m_entityCount;  // reserve 2 for BLOCK & ENDBLOCK
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"1"); }
  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbBlockTableRecord");
  WriteCodeWideString(2, name);
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
    WriteCodeString(0, L"ENDBLK");

    WriteCodeString(5, ToHexString(m_currentHandle + 2));
    if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, ToHexString(m_currentHandle)); }
    WriteCodeString(100, L"AcDbEntity");

    WriteCodeString(8, L"0");
    WriteCodeString(100, L"AcDbBlockEnd");
  }
  m_writingBlock = true;
  WriteCodeString(0, L"BLOCK");

  m_currentHandle = (*(m_blockMap.find(block->m_blockName))).second;
  WriteCodeString(5, ToHexString(m_currentHandle + 1));
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, ToHexString(m_currentHandle)); }
  WriteCodeString(100, L"AcDbEntity");

  WriteCodeString(8, L"0");

  WriteCodeString(100, L"AcDbBlockBegin");
  WriteCodeWideString(2, block->m_blockName);

  WriteCodeInt16(70, block->m_blockTypeFlags);
  WriteCodeDouble(10, block->m_basePoint.x);
  WriteCodeDouble(20, block->m_basePoint.y);
  if (std::abs(block->m_basePoint.z) > EoDxf::geometricTolerance) { WriteCodeDouble(30, block->m_basePoint.z); }

  WriteCodeWideString(3, block->m_blockName);

  WriteCodeString(1, L"");

  return m_writeOk;
}

bool EoDxfWrite::WriteBlocks() {
  WriteCodeString(0, L"BLOCK");

  WriteCodeString(5, L"20");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"1F"); }
  WriteCodeString(100, L"AcDbEntity");

  WriteCodeString(8, L"0");

  WriteCodeString(100, L"AcDbBlockBegin");
  WriteCodeString(2, L"*Model_Space");

  WriteCodeInt16(70, 0);
  WriteCodeDouble(10, 0.0);
  WriteCodeDouble(20, 0.0);
  WriteCodeDouble(30, 0.0);

  WriteCodeString(3, L"*Model_Space");

  WriteCodeString(1, L"");
  WriteCodeString(0, L"ENDBLK");

  WriteCodeString(5, L"21");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"1F"); }
  WriteCodeString(100, L"AcDbEntity");

  WriteCodeString(8, L"0");
  WriteCodeString(100, L"AcDbBlockEnd");
  WriteCodeString(0, L"BLOCK");

  WriteCodeString(5, L"1C");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"1B"); }
  WriteCodeString(100, L"AcDbEntity");

  WriteCodeString(8, L"0");

  WriteCodeString(100, L"AcDbBlockBegin");
  WriteCodeString(2, L"*Paper_Space");

  WriteCodeInt16(70, 0);
  WriteCodeDouble(10, 0.0);
  WriteCodeDouble(20, 0.0);
  WriteCodeDouble(30, 0.0);

  WriteCodeString(3, L"*Paper_Space");

  WriteCodeString(1, L"");
  WriteCodeString(0, L"ENDBLK");

  WriteCodeString(5, L"1D");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"1F"); }
  WriteCodeString(100, L"AcDbEntity");

  WriteCodeString(8, L"0");
  WriteCodeString(100, L"AcDbBlockEnd");
  m_writingBlock = false;
  m_interface->writeBlocks();
  if (m_writingBlock) {
    m_writingBlock = false;
    WriteCodeString(0, L"ENDBLK");

    WriteCodeString(5, ToHexString(m_currentHandle + 2));
    //            m_writer->WriteString(5, "1D");
    if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, ToHexString(m_currentHandle)); }
    WriteCodeString(100, L"AcDbEntity");

    WriteCodeString(8, L"0");
    WriteCodeString(100, L"AcDbBlockEnd");
  }
  return m_writeOk;
}

bool EoDxfWrite::WriteObjects() {
  m_interface->writeObjects();
  WriteCodeString(0, L"DICTIONARY");
  std::wstring imgDictH;
  WriteCodeString(5, L"C");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"0"); }
  WriteCodeString(100, L"AcDbDictionary");
  WriteCodeInt16(281, 1);
  WriteCodeString(3, L"ACAD_GROUP");
  WriteCodeString(350, L"D");
  if (m_imageDef.size() != 0) {
    WriteCodeString(3, L"ACAD_IMAGE_DICT");
    imgDictH = ToHexString(++m_entityCount);
    WriteCodeString(350, imgDictH);
  }
  WriteCodeString(0, L"DICTIONARY");
  WriteCodeString(5, L"D");
  WriteCodeString(330, L"C");
  WriteCodeString(100, L"AcDbDictionary");
  WriteCodeInt16(281, 1);
  // write IMAGEDEF_REACTOR
  for (unsigned int i = 0; i < m_imageDef.size(); i++) {
    EoDxfImageDefinition* id = m_imageDef.at(i);
    std::map<std::wstring, std::wstring>::iterator it;
    for (it = id->reactors.begin(); it != id->reactors.end(); ++it) {
      WriteCodeString(0, L"IMAGEDEF_REACTOR");
      WriteCodeString(5, it->first);
      WriteCodeString(330, it->second);
      WriteCodeString(100, L"AcDbRasterImageDefReactor");
      WriteCodeInt32(90, 2);  // version 2=R14 to v2010
      WriteCodeString(330, it->second);
    }
  }
  if (m_imageDef.size() != 0) {
    WriteCodeString(0, L"DICTIONARY");
    WriteCodeString(5, imgDictH);
    WriteCodeString(330, L"C");
    WriteCodeString(100, L"AcDbDictionary");
    WriteCodeInt16(281, 1);
    for (unsigned int i = 0; i < m_imageDef.size(); i++) {
      size_t f1, f2;
      f1 = m_imageDef.at(i)->m_fileNameOfImage.find_last_of(L"/\\");
      f2 = m_imageDef.at(i)->m_fileNameOfImage.find_last_of(L'.');
      ++f1;
      WriteCodeWideString(3, m_imageDef.at(i)->m_fileNameOfImage.substr(f1, f2 - f1));
      WriteCodeString(350, ToHexString(m_imageDef.at(i)->m_handle));
    }
  }
  for (unsigned int i = 0; i < m_imageDef.size(); i++) {
    EoDxfImageDefinition* id = m_imageDef.at(i);
    WriteCodeString(0, L"IMAGEDEF");
    WriteCodeString(5, ToHexString(id->m_handle));
    if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, imgDictH); }
    WriteCodeString(102, L"{ACAD_REACTORS");
    std::map<std::wstring, std::wstring>::iterator it;
    for (it = id->reactors.begin(); it != id->reactors.end(); ++it) { WriteCodeString(330, it->first); }
    WriteCodeString(102, L"}");
    WriteCodeString(100, L"AcDbRasterImageDef");
    WriteCodeInt32(90, 0);  // version 0=R14 to v2010
    WriteCodeWideString(1, id->m_fileNameOfImage);
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

  m_interface->writeUnsupportedObjects();

  return m_writeOk;
}

bool EoDxfWrite::WriteAppData(const std::list<std::list<EoDxfGroupCodeValuesVariant>>& appData) {
  for (const auto& group : appData) {
    if (group.empty()) { continue; }

    auto variant = group.begin();
    if (variant->Code() != 102) { continue; }

    if (const auto* value = variant->GetIf<std::wstring>()) {
      WriteCodeWideString(102, std::wstring{L"{"} + *value);
    } else {
      continue;
    }

    ++variant;
    for (; variant != group.end(); ++variant) { WriteVariantValue(*variant); }
    WriteCodeString(102, L"}");
  }

  return m_writeOk;
}

bool EoDxfWrite::WriteExtData(const std::vector<EoDxfGroupCodeValuesVariant*>& ed) {
  for (std::vector<EoDxfGroupCodeValuesVariant*>::const_iterator it = ed.begin(); it != ed.end(); ++it) {
    if (*it != nullptr) { WriteVariantValue(*(*it)); }
  }
  return m_writeOk;
}

bool EoDxfWrite::WriteVariantValue(const EoDxfGroupCodeValuesVariant& value) {
  const auto code = value.Code();

  if (const auto* wideStringValue = value.GetIf<std::wstring>()) {
    WriteCodeWideString(code, *wideStringValue);
  } else if (const auto* handleValue = value.GetIf<std::uint64_t>()) {
    WriteCodeString(code, ToHexString(*handleValue));
  } else if (const auto* geometryBase = value.GetIf<EoDxfGeometryBase3d>()) {
    WriteCodeDouble(code, geometryBase->x);
    WriteCodeDouble(code + 10, geometryBase->y);
    WriteCodeDouble(code + 20, geometryBase->z);
  } else if (const auto* int16Value = value.GetIf<std::int16_t>()) {
    WriteCodeInt16(code, *int16Value);
  } else if (const auto* int32Value = value.GetIf<std::int32_t>()) {
    WriteCodeInt32(code, *int32Value);
  } else if (const auto* int64Value = value.GetIf<std::int64_t>()) {
    WriteCodeInt64(code, *int64Value);
  } else if (const auto* boolValue = value.GetIf<bool>()) {
    if (code == 1070) {
      WriteCodeInt16(code, *boolValue ? 1 : 0);
    } else {
      WriteCodeBool(code, *boolValue);
    }
  } else if (const auto* doubleValue = value.GetIf<double>()) {
    WriteCodeDouble(code, *doubleValue);
  }

  return m_writeOk;
}

std::wstring EoDxfWrite::ToHexString(uint64_t hexValue) {
  std::wostringstream convert;
  convert << std::uppercase << std::hex << hexValue;
  return convert.str();
}
