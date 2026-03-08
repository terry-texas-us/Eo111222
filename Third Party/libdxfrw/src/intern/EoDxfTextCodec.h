#pragma once
#include <string>

class EoTcConverter;

class EoTcTextCodec {
 public:
  EoTcTextCodec();
  ~EoTcTextCodec();

  [[nodiscard]] std::string FromUtf8(std::string s);

  [[nodiscard]] std::string ToUtf8(std::string s) const;

  [[nodiscard]] constexpr int GetVersion() const noexcept { return m_version; }

  /** @brief Sets the version for the text codec based on the provided string and format.
   * The function checks the version string against known versions and sets the internal version accordingly.
   * If the version string does not match any known versions, it defaults to EoDxf::Version::AC1021.
   * @param version Pointer to a string representing the version to set
   * @param dxfFormat Boolean indicating whether the format is DXF or not
   */
  void SetVersion(const std::string& version, bool dxfFormat);

  void SetVersion(int version, bool dxfFormat);

  [[nodiscard]] const std::string& GetCodePage() const noexcept { return m_codePage; }

  void SetCodePage(const std::string& codePage, bool dxfFormat);

 private:
  [[nodiscard]] std::string NormalizeCodePage(const std::string_view codePage) noexcept;

  std::string m_codePage;
  EoTcConverter* m_converter;
  int m_version;
};

/** @brief Base class for text conversion. It can handle character encoding and decoding,
 * providing methods to convert between UTF-8 strings and other formats using a specified character table.
 */
class EoTcConverter {
 public:
  EoTcConverter(const int* table, int length) : m_table{table}, m_codePageLength{length} {}
  virtual ~EoTcConverter() = default;

  virtual [[nodiscard]] std::string FromUtf8(std::string* s) { return *s; }
  virtual [[nodiscard]] std::string ToUtf8(std::string* s) { return *s; }

 protected:
  const int* m_table;
  int m_codePageLength;
};

/** @brief Converter class for UTF-16 encoding. This class provides methods to convert between UTF-8 strings and UTF-16
 * encoded strings. It overrides the FromUtf8 and ToUtf8 methods to perform the necessary conversions using UTF-16
 * encoding rules.
 */
class EoTcConvertUtf16 : public EoTcConverter {
 public:
  EoTcConvertUtf16() : EoTcConverter(nullptr, 0) {}
  [[nodiscard]] std::string FromUtf8(std::string* s) override { return *s; }
  [[nodiscard]] std::string ToUtf8(std::string* s) override { return *s; }
};

/** @brief Converter class for character tables. This class provides methods to convert between UTF-8 strings and
 * strings encoded using a specified character table. It uses the character table provided in the constructor to perform
 * the necessary conversions in the FromUtf8 and ToUtf8 methods.
 */
class EoTcConvertTable : public EoTcConverter {
 public:
  EoTcConvertTable(const int* table, int length) : EoTcConverter(table, length) {}
  [[nodiscard]] std::string FromUtf8(std::string* s) override { return *s; }
  [[nodiscard]] std::string ToUtf8(std::string* s) override { return *s; }
};

/** @brief Converter class for DBCS (Double-Byte Character Set) tables.
 * This class provides methods to convert between UTF-8 strings and strings encoded using a specified DBCS character
 * table. It uses the lead byte table and double-byte character table provided in the constructor to perform the
 * necessary conversions in the FromUtf8 and ToUtf8 methods.
 */
class EoTcConvertDBCSTable : public EoTcConverter {
 public:
  EoTcConvertDBCSTable(const int* table, const int* leadTable, const int doubleTable[][2], int length)
      : EoTcConverter(table, length) {
    m_leadTable = leadTable;
    m_doubleTable = doubleTable;
  }
  [[nodiscard]] std::string FromUtf8(std::string* s) override { return *s; }
  [[nodiscard]] std::string ToUtf8(std::string* s) override { return *s; }

 private:
  const int* m_leadTable;
  const int (*m_doubleTable)[2];
};