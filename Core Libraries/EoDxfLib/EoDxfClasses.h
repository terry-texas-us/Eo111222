#pragma once

#include <string>

#include "EoDxfBase.h"
class EoDxfReader;
class EoDxfWriter;

/**
 * @brief Class to handle classes table entries introduced in R13 (AC1012) and used in later versions of the DXF format.
 * @note The CLASSES section of a DXF file contains definitions of custom classes used in the file, including their
 * names, application names, and proxy flags. This class represents an entry in that section and provides methods to
 * parse from a DXF reader and write to a DXF writer.
 */
class EoDxfClass {
 public:
  EoDxfClass() = default;
  ~EoDxfClass() = default;

  void ParseCode(int code, EoDxfReader* reader) noexcept;
  void clear() noexcept;
  void write(EoDxfWriter* writer, EoDxf::Version version) const noexcept;

  std::string m_classDxfRecordName;  // Group code 1
  std::string m_cppClassName;  // Group code 2
  std::string m_applicationName;  // Group code 3
  std::int32_t m_proxyCapabilitiesFlag{};  // Group code 90
  std::int32_t m_instanceCount{};  // Group code 91
  std::int16_t m_wasAProxyFlag{};  // Group code 280
  std::int16_t m_isAnEntityFlag{};  // Group code 281 (0 object, 1 entity)
};