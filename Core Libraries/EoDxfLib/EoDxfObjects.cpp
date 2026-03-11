#include <string>

#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfObjects.h"
#include "EoDxfReader.h"

void EoDxfObjectEntry::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 5:
      m_handle = reader->GetHandleString();
      break;
    case 102: {
      std::string value = reader->GetString();
      if (value == "{ACAD_REACTORS") {
        m_inReactors = true;
      } else if (value == "{ACAD_XDICTIONARY") {
        m_inXDictionary = true;
      } else if (value == "}") {
        m_inReactors = false;
        m_inXDictionary = false;
      }
      break;
    }
    case 330:
      if (m_inReactors) {
        m_reactorHandles.push_back(reader->GetHandleString());
      } else {
        m_ownerHandle = reader->GetHandleString();
      }
      break;
    case 360:
      if (m_inXDictionary) { m_extensionDictionaryHandle = reader->GetHandleString(); }
      break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
      m_extensionData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetString()));
      break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
      m_currentVariant = new EoDxfGroupCodeValuesVariant(code, EoDxfGeometryBase3d(reader->GetDouble(), 0.0, 0.0));
      m_extensionData.push_back(m_currentVariant);
      break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
      if (m_currentVariant) { m_currentVariant->SetGeometryBaseY(reader->GetDouble()); }
      break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
      if (m_currentVariant) { m_currentVariant->SetGeometryBaseZ(reader->GetDouble()); }
      m_currentVariant = nullptr;
      break;
    case 1040:
    case 1041:
    case 1042:
      m_extensionData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetDouble()));
      break;
    case 1070:
      m_extensionData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetInt16()));
      break;
    case 1071:
      m_extensionData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetInt32()));
      break;
    default:
      break;
  }
}

void EoDxfObjectEntry::Reset() {
  m_ownerHandle = 0;
  m_extensionDictionaryHandle = 0;
  m_reactorHandles.clear();
  for (auto* variant : m_extensionData) { delete variant; }
  m_extensionData.clear();
  m_currentVariant = nullptr;
  m_inReactors = false;
  m_inXDictionary = false;
}

void EoDxfImageDefinition::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 1:
      m_fileNameOfImage = reader->GetUtf8String();
      break;
    case 90:
      imgVersion = reader->GetInt32();
      break;
    case 10:
      m_uImageSizeInPixels = reader->GetDouble();
      break;
    case 20:
      m_vImageSizeInPixels = reader->GetDouble();
      break;
    case 11:
      m_uSizeOfOnePixel = reader->GetDouble();
      break;
    case 12:
      [[fallthrough]];  // Group code 12 is used for the v size of one pixel in the DXF documentation.
    case 21:  // However, we will always write group code 21 for the v size of one pixel.
      m_vSizeOfOnePixel = reader->GetDouble();
      break;
    case 280:
      m_imageIsLoadedFlag = reader->GetInt16();
      break;
    case 281:
      m_resolutionUnits = reader->GetInt16();
      break;
    default:
      EoDxfObjectEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfImageDefinition::Reset() {
  imgVersion = 0;
  m_uImageSizeInPixels = 0.0;
  m_vImageSizeInPixels = 0.0;
  m_uSizeOfOnePixel = 0.0;
  m_vSizeOfOnePixel = 0.0;
  m_imageIsLoadedFlag = 0;
  m_resolutionUnits = 0;
  EoDxfObjectEntry::Reset();
}
