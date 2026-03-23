#pragma once

#include <cstdint>
#include <string>

/// @brief Represents a single CLASS section entry for DXF class definition round-trip.
///
/// Stores the DXF CLASS section fields needed to preserve custom class definitions
/// across DXF import/export. The field names and semantics match the DXF CLASS
/// record group codes.
///
/// AeSys does not interpret class definitions — this is a passthrough container that
/// ensures CLASSES section entries survive a DXF read/write cycle intact.
class EoDbClassEntry {
 public:
  EoDbClassEntry() = default;

  /// @brief Class DXF record name (DXF group code 1). E.g., "ACDBDICTIONARYWDFLT".
  std::wstring m_classDxfRecordName;

  /// @brief C++ class name (DXF group code 2). E.g., "AcDbDictionaryWithDefault".
  std::wstring m_cppClassName;

  /// @brief Application name (DXF group code 3). E.g., "ObjectDBX Classes".
  std::wstring m_applicationName;

  /// @brief Proxy capabilities flag (DXF group code 90).
  std::int32_t m_proxyCapabilitiesFlag{};

  /// @brief Instance count (DXF group code 91, AC1018+).
  std::int32_t m_instanceCount{};

  /// @brief Was-a-proxy flag (DXF group code 280).
  std::int16_t m_wasAProxyFlag{};

  /// @brief Is-an-entity flag (DXF group code 281). 0 = object, 1 = entity.
  std::int16_t m_isAnEntityFlag{};
};
