#pragma once

#include "drw_base.h"

class dxfReader;
class dxfWriter;

/**
 * @brief Class to handle classes table entries introduced in R13 (AC1012) and used in later versions of the DXF format.
 * @note The CLASSES section of a DXF file contains definitions of custom classes used in the file, including their
 * names, application names, and proxy flags. This class represents an entry in that section and provides methods to
 * parse from a DXF reader and write to a DXF writer.
 */
class DRW_Class {
 public:
  DRW_Class() = default;
  ~DRW_Class() = default;

  void parseCode(int code, dxfReader* reader) noexcept;
  void clear() noexcept;
  void write(dxfWriter* writer, DRW::Version version) const noexcept;

  UTF8STRING recName;             // group 1 - dxf record name
  UTF8STRING className;           // group 2 - C++ class name
  UTF8STRING appName;             // group 3 - application name
  int proxyCapabilities{};        // group 90 - Proxy capabilities flag
  int instanceCount{};            // group 91 - instance count for a custom class
  std::int16_t wasAProxyFlag{};   // group 280 - was-a-proxy flag (app loaded on save)
  std::int16_t isAnEntityFlag{};  // group 281 - is-an-entity flag (0 object, 1 entity)
};