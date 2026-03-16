#include <string>

#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfObjects.h"
#include "EoDxfReader.h"
#include "EoDxfWriter.h"

void EoDxfObjectEntry::ParseCode(int code, EoDxfReader& reader) {
  EoDxfEntity::ParseCode(code, reader);
}

void EoDxfObjectEntry::Reset() {
  m_handle = EoDxf::NoHandle;
  m_ownerHandle = EoDxf::NoHandle;
  m_extensionDictionaryHandle = EoDxf::NoHandle;
  m_reactorHandles.clear();
  m_extendedData.clear();
  m_currentVariant = nullptr;
  m_appData.clear();
}

void EoDxfImageDefinition::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 1:
      m_fileNameOfImage = reader.GetWideString();
      break;
    case 90:
      m_classVersion = reader.GetInt32();
      break;
    case 10:
      m_uImageSizeInPixels = reader.GetDouble();
      break;
    case 20:
      m_vImageSizeInPixels = reader.GetDouble();
      break;
    case 11:
      m_uSizeOfOnePixel = reader.GetDouble();
      break;
    case 12:
      [[fallthrough]];  // Group code 12 is used for the v size of one pixel in the DXF documentation.
    case 21:  // However, we will always write group code 21 for the v size of one pixel.
      m_vSizeOfOnePixel = reader.GetDouble();
      break;
    case 280:
      m_imageIsLoadedFlag = reader.GetInt16();
      break;
    case 281:
      m_resolutionUnits = reader.GetInt16();
      break;
    default:
      EoDxfObjectEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfImageDefinition::Reset() {
  m_classVersion = 0;
  m_uImageSizeInPixels = 0.0;
  m_vImageSizeInPixels = 0.0;
  m_uSizeOfOnePixel = 0.0;
  m_vSizeOfOnePixel = 0.0;
  m_imageIsLoadedFlag = 0;
  m_resolutionUnits = 0;
  EoDxfObjectEntry::Reset();
}

void EoDxfUnsupportedObject::Write(EoDxfWriter* writer) const {
  if (writer == nullptr || m_objectType.empty()) { return; }

  writer->WriteWideString(0, m_objectType);
  for (const auto& value : m_values) {
    const auto code = value.Code();
    if (const auto* wideStringValue = value.GetIf<std::wstring>()) {
      writer->WriteWideString(code, *wideStringValue);
    } else if (const auto* int16Value = value.GetIf<std::int16_t>()) {
      writer->WriteInt16(code, *int16Value);
    } else if (const auto* int32Value = value.GetIf<std::int32_t>()) {
      writer->WriteInt32(code, *int32Value);
    } else if (const auto* int64Value = value.GetIf<std::int64_t>()) {
      writer->WriteInt64(code, *int64Value);
    } else if (const auto* boolValue = value.GetIf<bool>()) {
      writer->WriteBool(code, *boolValue);
    } else if (const auto* doubleValue = value.GetIf<double>()) {
      writer->WriteDouble(code, *doubleValue);
    }
  }
}
