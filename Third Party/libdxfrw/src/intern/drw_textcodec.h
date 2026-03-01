#pragma once
#include <string>

class DRW_Converter;

class DRW_TextCodec {
 public:
  DRW_TextCodec();
  ~DRW_TextCodec();

  std::string FromUtf8(std::string s);
  
  std::string ToUtf8(std::string s) const;
  
  [[nodiscard]] constexpr int GetVersion() const noexcept { return m_version; }
  
  /** @brief Sets the version for the text codec based on the provided string and format.
   * The function checks the version string against known versions and sets the internal version accordingly.
   * If the version string does not match any known versions, it defaults to DRW::Version::AC1021.
   * @param version Pointer to a string representing the version to set
   * @param dxfFormat Boolean indicating whether the format is DXF or not
   */
  void SetVersion(std::string* version, bool dxfFormat);
  
  void SetVersion(int version, bool dxfFormat);
  
  /** @brief Sets the code page for the text codec based on the provided string and format.
   * The function first corrects the code page string, then deletes any existing converter.
   * Depending on the version and format, it initializes a new converter with the appropriate table or encoding.
   * @param c Pointer to a string representing the code page to set
   * @param dxfFormat Boolean indicating whether the format is DXF or not
   */
  void SetCodePage(std::string* c, bool dxfFormat);
  
  void SetCodePage(std::string c, bool dxfFormat) { SetCodePage(&c, dxfFormat); }
  
  [[nodiscard]] const std::string& GetCodePage() const noexcept { return m_codePage; }

 private:
  [[nodiscard]] std::string NormalizeCodePage(const std::string_view& codePage) noexcept;

  std::string m_codePage;
  DRW_Converter* m_converter;
  int m_version;
};

/** @brief Base class for text conversion. It can handle character encoding and decoding,
 * providing methods to convert between UTF-8 strings and other formats using a specified character table.
 */
class DRW_Converter {
 public:
  DRW_Converter(const int* table, int length) : m_table{table}, m_cpLength{length} {}
  virtual ~DRW_Converter() = default;
  
  virtual std::string FromUtf8(std::string* s) { return *s; }
  virtual std::string ToUtf8(std::string* s) { return *s; }

  const int* m_table;
  int m_cpLength;
};

/** @brief Converter class for UTF-16 encoding. This class provides methods to convert between UTF-8 strings and UTF-16 encoded strings.
 * It overrides the FromUtf8 and ToUtf8 methods to perform the necessary conversions using UTF-16 encoding rules.
 */
class DRW_ConvUTF16 : public DRW_Converter {
 public:
  DRW_ConvUTF16() : DRW_Converter(nullptr, 0) {}
  std::string FromUtf8(std::string* s) { return *s; }
  std::string ToUtf8(std::string* s) { return *s; }
};

/** @brief Converter class for character tables. This class provides methods to convert between UTF-8 strings and strings encoded using a specified character table.
 * It uses the character table provided in the constructor to perform the necessary conversions in the FromUtf8 and ToUtf8 methods.
 */
class DRW_ConvTable : public DRW_Converter {
 public:
  DRW_ConvTable(const int* table, int length) : DRW_Converter(table, length) {}
  std::string FromUtf8(std::string* s) { return *s; }
  std::string ToUtf8(std::string* s) { return *s; }
};

/** @brief Converter class for DBCS (Double-Byte Character Set) tables.
 * This class provides methods to convert between UTF-8 strings and strings encoded using a specified DBCS character table.
 * It uses the lead byte table and double-byte character table provided in the constructor to perform the necessary
 * conversions in the FromUtf8 and ToUtf8 methods.
 */
class DRW_ConvDBCSTable : public DRW_Converter {
 public:
  DRW_ConvDBCSTable(const int* table, const int* leadTable, const int doubleTable[][2], int length)
      : DRW_Converter(table, length) {
    m_leadTable = leadTable;
    m_doubleTable = doubleTable;
  }
  std::string FromUtf8(std::string* s) { return *s; }
  std::string ToUtf8(std::string* s) { return *s; }

 private:
  const int* m_leadTable;
  const int (*m_doubleTable)[2];
};