#include <list>
#include <memory>
#include <string>
#include <utility>

#include "EoDxfEntity.h"
#include "EoDxfGeometry.h"
#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfReader.h"

namespace {

/** @brief Helper function to parse application-defined group (code 102) and its associated data until the closing tag
 * is reached.
 *  @param reader pointer to EoDxfReader to read value
 *  @return true if group is successfully parsed, false if group is not recognized or an error occurs
 *
 *  This function reads the application-defined group code (102) and its associated data,
 *  which can include various types of values based on the DXF specification.
 *  It continues reading until it encounters the closing tag for the group.
 *  The parsed values are stored in an EoDxfGroupCodeValuesVariant, which can hold different types of data
 *  based on the group code ranges defined in the DXF format.
 */
bool AddAppDataValue(EoDxfGroupCodeValuesVariant& variant, int code, EoDxfReader& reader) {
  if (code == 330 || code == 360) {
    variant.AddHandle(code, reader.GetHandleString());
    return true;
  }
  if (code <= 9 || code == 100 || code == 102 || code == 105 || (code >= 300 && code < 370) ||
      (code >= 390 && code < 400) || (code >= 410 && code < 420) || (code >= 430 && code < 440) ||
      (code >= 470 && code < 481) || code == 999 || (code >= 1000 && code <= 1009)) {
    variant.AddWideString(code, reader.GetWideString());
    return true;
  }
  if ((code >= 10 && code < 60) || (code >= 110 && code < 150) || (code >= 210 && code < 240) ||
      (code >= 460 && code < 470) || (code >= 1010 && code <= 1059)) {
    variant.AddDouble(code, reader.GetDouble());
    return true;
  }
  if ((code >= 60 && code < 80) || (code >= 170 && code < 180) || (code >= 270 && code < 290) ||
      (code >= 370 && code < 390) || (code >= 400 && code < 410) || (code >= 1060 && code <= 1070)) {
    variant.AddInt16(code, reader.GetInt16());
    return true;
  }
  if ((code >= 90 && code < 100) || (code >= 420 && code < 430) || (code >= 440 && code < 460) || code == 1071) {
    variant.AddInt32(code, reader.GetInt32());
    return true;
  }
  if (code >= 160 && code < 170) {
    variant.AddInt64(code, reader.GetInt64());
    return true;
  }
  if (code >= 290 && code < 300) {
    variant.AddBoolean(code, reader.GetBool());
    return true;
  }
  return false;
}
}  // namespace

EoDxfEntity::EoDxfEntity(const EoDxfEntity& other)
    : m_handle{other.m_handle},
      m_ownerHandle{other.m_ownerHandle},
      m_entityType{other.m_entityType},
      m_reactorHandles{other.m_reactorHandles},
      m_extensionDictionaryHandle{other.m_extensionDictionaryHandle},
      m_appData{other.m_appData} {
  m_extendedData.reserve(other.m_extendedData.size());
  for (const auto& variant : other.m_extendedData) {
    m_extendedData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(*variant));
  }
}

EoDxfEntity& EoDxfEntity::operator=(const EoDxfEntity& other) {
  if (this != &other) {
    std::vector<std::unique_ptr<EoDxfGroupCodeValuesVariant>> extendedData;
    extendedData.reserve(other.m_extendedData.size());
    for (const auto& variant : other.m_extendedData) {
      extendedData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(*variant));
    }

    m_handle = other.m_handle;
    m_ownerHandle = other.m_ownerHandle;
    m_entityType = other.m_entityType;
    m_reactorHandles = other.m_reactorHandles;
    m_extensionDictionaryHandle = other.m_extensionDictionaryHandle;
    m_appData = other.m_appData;
    m_extendedData = std::move(extendedData);
    m_currentVariant = nullptr;
  }
  return *this;
}

EoDxfEntity::EoDxfEntity(EoDxfEntity&& other) noexcept
    : m_handle{other.m_handle},
      m_ownerHandle{other.m_ownerHandle},
      m_entityType{other.m_entityType},
      m_reactorHandles{std::move(other.m_reactorHandles)},
      m_extensionDictionaryHandle{other.m_extensionDictionaryHandle},
      m_appData{std::move(other.m_appData)},
      m_extendedData{std::move(other.m_extendedData)},
      m_currentVariant{std::exchange(other.m_currentVariant, nullptr)} {}

EoDxfEntity& EoDxfEntity::operator=(EoDxfEntity&& other) noexcept {
  if (this != &other) {
    m_handle = other.m_handle;
    m_ownerHandle = other.m_ownerHandle;
    m_entityType = other.m_entityType;
    m_reactorHandles = std::move(other.m_reactorHandles);
    m_extensionDictionaryHandle = other.m_extensionDictionaryHandle;
    m_appData = std::move(other.m_appData);
    m_extendedData = std::move(other.m_extendedData);
    m_currentVariant = std::exchange(other.m_currentVariant, nullptr);
  }
  return *this;
}

EoDxfEntity::~EoDxfEntity() = default;

void EoDxfEntity::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 5:
      m_handle = reader.GetHandleString();
      break;
    case 330:
      m_ownerHandle = reader.GetHandleString();
      break;
    case 102:
      ParseAppDataGroup(reader);
      break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
      m_extendedData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(code, reader.GetWideString()));
      break;
    case 1010:
    case 1011:
    case 1012:
    case 1013: {
      m_currentVariant = nullptr;  // Reset in case a previous triplet was incomplete
      auto variant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, EoDxfGeometryBase3d(reader.GetDouble(), 0.0, 0.0));
      m_currentVariant = variant.get();
      m_extendedData.push_back(std::move(variant));
      break;
    }
    case 1020:
    case 1021:
    case 1022:
    case 1023:
      if (m_currentVariant) { m_currentVariant->SetGeometryBaseY(reader.GetDouble()); }
      break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
      if (m_currentVariant) { m_currentVariant->SetGeometryBaseZ(reader.GetDouble()); }
      m_currentVariant = nullptr;
      break;
    case 1040:
    case 1041:
    case 1042:
      m_extendedData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(code, reader.GetDouble()));
      break;
    case 1070:
      m_extendedData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(code, reader.GetInt16()));
      break;
    case 1071:
      m_extendedData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(code, reader.GetInt32()));
      break;
    default:
      break;
  }
}

bool EoDxfEntity::ParseAppDataGroup(EoDxfReader& reader) {
  auto appName = reader.GetWideString();
  if (appName.empty() || appName[0] != L'{') { return false; }

  // Structurally extract ACAD_REACTORS: collect soft-pointer handles (code 330) into m_reactorHandles.
  if (appName == L"{ACAD_REACTORS") {
    while (true) {
      int nextCode{};
      if (!reader.ReadRec(&nextCode)) { break; }
      if (nextCode == 102) {
        auto value = reader.GetWideString();
        if (!value.empty() && value[0] == L'}') { break; }
      } else if (nextCode == 330) {
        m_reactorHandles.push_back(reader.GetHandleString());
      } else {
        // Unexpected code inside ACAD_REACTORS — value already consumed by ReadRec, discard silently
      }
    }
    return true;
  }

  // Structurally extract ACAD_XDICTIONARY: store hard-owner handle (code 360) in m_extensionDictionaryHandle.
  if (appName == L"{ACAD_XDICTIONARY") {
    while (true) {
      int nextCode{};
      if (!reader.ReadRec(&nextCode)) { break; }
      if (nextCode == 102) {
        auto value = reader.GetWideString();
        if (!value.empty() && value[0] == L'}') { break; }
      } else if (nextCode == 360) {
        m_extensionDictionaryHandle = reader.GetHandleString();
      } else {
        // Unexpected code inside ACAD_XDICTIONARY — value already consumed by ReadRec, discard silently
      }
    }
    return true;
  }

  // All other application-defined groups: store opaquely in m_appData for round-trip fidelity.
  std::list<EoDxfGroupCodeValuesVariant> groupList;
  EoDxfGroupCodeValuesVariant currentVariant;

  // opening line: store without the leading '{'
  currentVariant.AddWideString(102, appName.substr(1));
  groupList.push_back(currentVariant);

  while (true) {
    int nextCode{};
    if (!reader.ReadRec(&nextCode)) { break; }  // EOF or read error
    bool hasValue{};

    if (nextCode == 102) {
      std::wstring val = reader.GetWideString();
      if (!val.empty() && val[0] == L'}') { break; }  // closing 102 } — do not store the closing tag

      // rare nested control string
      currentVariant = EoDxfGroupCodeValuesVariant{};
      currentVariant.AddWideString(102, val);
      hasValue = true;
    } else {
      currentVariant = EoDxfGroupCodeValuesVariant{};
      hasValue = AddAppDataValue(currentVariant, nextCode, reader);
    }
    if (hasValue) { groupList.push_back(currentVariant); }
  }

  m_appData.push_back(std::move(groupList));  // avoid copy
  return true;
}
