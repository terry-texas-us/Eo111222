#pragma once

#include <string_view>

#include <string>

class EoTcConverter;

class EoTcTextCodec {
 public:
  EoTcTextCodec();
  ~EoTcTextCodec();

  [[nodiscard]] std::string FromUtf8(std::string s);

  [[nodiscard]] std::string ToUtf8(std::string s) const;

  [[nodiscard]] constexpr int GetVersion() const noexcept { return m_version; }

  /** @brief Sets the version of the text codec. This method allows you to specify the version of the text codec being
   * used, which can affect how text is encoded and decoded. The version can be set using either a string representation
   * or an integer value, and the dxfFormat parameter indicates whether the version is in DXF format.
   *
   * @param version The version to set, either as a string or an integer.
   * @param dxfFormat A boolean indicating whether the version is in DXF format.
   */
  void SetVersion(const std::string& version, bool dxfFormat);
  void SetVersion(int version, bool dxfFormat);

  /** @brief Gets the current code page used by the text codec.
   * The code page is a character encoding standard that defines how characters are represented in bytes.
   * @return The current code page as a string.
   */
  [[nodiscard]] const std::string& GetCodePage() const noexcept { return m_codePage; }

  /** @brief Sets the code page for the text codec. This method allows you to specify the character encoding standard that
   * the text codec should use when encoding and decoding text. The code page can affect how characters are represented
   * in bytes, and it is important to set it correctly to ensure proper handling of text data.
   *
   * @param codePage The code page to set, specified as a string (e.g., "UTF-8", "UTF-16", "ANSI_1252").
   */
  void SetCodePage(const std::string& codePage);

 private:

  /** @brief Standardizes a DXF code page string to one of the standard internal forms.
   *
   * - Empty string → "ANSI_1252"
   * - "UTF-8" or "UTF8" → "UTF-8"
   * - "UTF-16" or "UTF16" → "UTF-16"
   * - Everything else (including all ANSI-1252 variants and unknown values) → "ANSI_1252"
   *
   * @param codePage Input code page identifier (from DXF $DWGCODEPAGE or similar).
   * @return Standardized code page string.
   */
  [[nodiscard]] std::string NormalizeCodePage(const std::string_view codePage) noexcept;

  std::string m_codePage;
  EoTcConverter* m_converter;
  int m_version;
};

/** @brief Base converter class for text encoding conversions. This class provides a common interface for converting between
 * UTF-8 strings and other encodings. It holds a character table and its length, which can be used by derived classes to
 * perform specific conversions based on the encoding rules defined in the character table.
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

/** @brief Converter class for UTF-8 encoding. This class provides methods to convert between UTF-8 strings and UTF-8
 * encoded strings. Since UTF-8 is a variable-width character encoding, the FromUtf8 and ToUtf8 methods simply return
 * the input string without any modifications, as UTF-8 strings are already in the desired format.
 */
class EoTcConvertUtf8 : public EoTcConverter {
 public:
  EoTcConvertUtf8() : EoTcConverter(nullptr, 0) {}
  [[nodiscard]] std::string FromUtf8(std::string* s) override { return *s; }
  [[nodiscard]] std::string ToUtf8(std::string* s) override { return *s; }
};

/** @brief Converter class for UTF-16 encoding. This class provides methods to convert between UTF-8 strings and UTF-16
 * encoded strings. The FromUtf8 method converts a UTF-8 string to a UTF-16 encoded string, while the ToUtf8 method
 * converts a UTF-16 encoded string back to a UTF-8 string. The actual conversion logic would need to be implemented in
 * these methods, as UTF-8 and UTF-16 are different character encodings with distinct byte representations.
 */
class EoTcConvertUtf16 : public EoTcConverter {
 public:
  EoTcConvertUtf16() : EoTcConverter(nullptr, 0) {}
  [[nodiscard]] std::string FromUtf8(std::string* s) override;
  [[nodiscard]] std::string ToUtf8(std::string* s) override;
};

/** @brief Converter class for character tables. This class provides methods to convert between UTF-8 strings and strings
 * encoded using a specified character table. The FromUtf8 and ToUtf8 methods would need to be implemented to perform the
 * necessary conversions based on the character table provided in the constructor.
 */
class EoTcConvertTable : public EoTcConverter {
 public:
  EoTcConvertTable(const int* table, int length) : EoTcConverter(table, length) {}
  [[nodiscard]] std::string FromUtf8(std::string* s) override;
  [[nodiscard]] std::string ToUtf8(std::string* s) override;
};

/** @brief Converter class for DBCS (Double-Byte Character Set) tables. This class provides methods to convert between UTF-8
 * strings and strings encoded using a specified DBCS character table. The FromUtf8 and ToUtf8 methods would need to be
 * implemented to perform the necessary conversions based on the lead byte table and double-byte character table provided
 * in the constructor.
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