#pragma once

#include <cstdint>
#include <string>

/// @brief Stores a DXF APPID table entry for round-trip passthrough.
///
/// AeSys does not use application IDs internally but preserves them so that
/// DXF export can rewrite the same set of APPID entries that were imported.
struct EoDbAppIdEntry {
  std::wstring m_name;
  std::int16_t m_flagValues{};
  std::uint64_t m_handle{};
  std::uint64_t m_ownerHandle{};
};
