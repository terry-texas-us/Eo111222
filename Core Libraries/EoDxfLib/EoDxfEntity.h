#pragma once

#include <cstdint>
#include <list>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfReader.h"

class EoDxfEntity {
 public:
  EoDxfEntity() = default;
  explicit EoDxfEntity(EoDxf::ETYPE entityType) noexcept : m_entityType{entityType} {}
  EoDxfEntity(const EoDxfEntity& other);
  EoDxfEntity& operator=(const EoDxfEntity& other);

  EoDxfEntity(EoDxfEntity&&) noexcept = default;
  EoDxfEntity& operator=(EoDxfEntity&&) noexcept = default;

  virtual ~EoDxfEntity();

 protected:
  /** @brief Parses application-defined group (code 102) and its associated data until the closing tag is reached.
   *
   *  Recognizes `{ACAD_REACTORS` and `{ACAD_XDICTIONARY` groups and extracts their handles into the structured
   *  fields m_reactorHandles and m_extensionDictionaryHandle respectively. All other 102 groups are stored
   *  opaquely in m_appData for round-trip fidelity.
   *  @param reader pointer to EoDxfReader to read value
   *  @return true if group is successfully parsed, false if group is not recognized or an error occurs
   */
  bool ParseAppDataGroup(EoDxfReader* reader);

  void ParseCode(int code, EoDxfReader* reader);

  void clearExtendedData() noexcept;

 public:
  // Database identity — common to all DXF database-resident objects (entities, table entries, objects).
  std::uint64_t m_handle{EoDxf::NoHandle};  // Group code 5
  std::uint64_t m_ownerHandle{EoDxf::NoHandle};  // Group code 330 (soft-pointer to owner)
  enum EoDxf::ETYPE m_entityType{EoDxf::UNKNOWN};  // Group code 0

  // Reactor handles (soft-pointer IDs within {ACAD_REACTORS} group, code 330)
  std::vector<std::uint64_t> m_reactorHandles;

  // Extension dictionary handle (hard-owner ID within {ACAD_XDICTIONARY} group, code 360)
  std::uint64_t m_extensionDictionaryHandle{EoDxf::NoHandle};

  // Extended data (group codes 1000 to 1071)
  std::vector<EoDxfGroupCodeValuesVariant*> m_extendedData{};

  // Application-defined groups (group code 102), excluding ACAD_REACTORS and ACAD_XDICTIONARY
  std::list<std::list<EoDxfGroupCodeValuesVariant>> m_appData{};

 protected:
  EoDxfGroupCodeValuesVariant* m_currentVariant{};
};
