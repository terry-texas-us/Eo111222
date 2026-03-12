#pragma once

#include <string_view>

#include <memory>
#include <string>

#include "EoDxfBase.h"

class EoTcConverter;

class EoTcTextCodec {
 public:
  EoTcTextCodec();
  ~EoTcTextCodec();
  EoTcTextCodec(const EoTcTextCodec& other);
  EoTcTextCodec& operator=(const EoTcTextCodec& other);
  EoTcTextCodec(EoTcTextCodec&& other) noexcept;
  EoTcTextCodec& operator=(EoTcTextCodec&& other) noexcept;

  [[nodiscard]] std::string EncodeText(std::wstring_view text) const;
  [[nodiscard]] std::wstring DecodeText(std::string_view encodedText) const;

  [[nodiscard]] std::wstring GetVersion() const;
  [[nodiscard]] constexpr EoDxf::Version GetVersionEnum() const noexcept { return m_version; }

  /** @brief Sets the version for the text codec. This method allows you to specify the version of the DXF format that the text
   * codec should use when encoding and decoding text. The version can affect how certain characters are handled, as different
   * versions of the DXF format may have different rules for character encoding. By setting the version correctly, you can
   * ensure that the text codec behaves in a way that is compatible with the specific DXF version you are working with.
   *
   * @param version The version to set, specified as a string (e.g., "AC1015", "AC1021") or as an integer corresponding to
   * the EoDxf::Version enumeration.
   */
  void SetVersion(std::wstring_view version);
  void SetVersion(EoDxf::Version version) noexcept;

  /** @brief Gets the current code page used by the text codec.
   * The code page is a character encoding standard that defines how characters are represented in bytes.
   * @return The current code page as a string.
   */
  [[nodiscard]] const std::wstring& GetCodePage() const noexcept { return m_codePage; }

  /** @brief Sets the code page for the text codec. This method allows you to specify the character encoding standard that
   * the text codec should use when encoding and decoding text. The code page can affect how characters are represented
   * in bytes, and it is important to set it correctly to ensure proper handling of text data.
   *
   * @param codePage The code page to set, specified as a string (e.g., "UTF-8", "UTF-16", "ANSI_1252").
   */
  void SetCodePage(std::wstring_view codePage);

 private:
  [[nodiscard]] static std::unique_ptr<EoTcConverter> CreateConverter(std::wstring_view normalizedCodePage);

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
  [[nodiscard]] static std::wstring NormalizeToken(std::wstring_view value);
  [[nodiscard]] std::wstring NormalizeCodePage(std::wstring_view codePage);

  std::wstring m_codePage;
  std::unique_ptr<EoTcConverter> m_converter;
  EoDxf::Version m_version;
};

/** @brief Base converter class for text encoding conversions. This class provides a common interface for converting between
 * UTF-8 strings and other encodings. It holds a character table and its length, which can be used by derived classes to
 * perform specific conversions based on the encoding rules defined in the character table.
 */
class EoTcConverter {
 public:
  EoTcConverter(const int* table, int length) : m_table{table}, m_codePageLength{length} {}
  virtual ~EoTcConverter() = default;

  virtual [[nodiscard]] std::string EncodeText(std::wstring_view text) const = 0;
  virtual [[nodiscard]] std::wstring DecodeText(std::string_view encodedText) const = 0;

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
  [[nodiscard]] std::string EncodeText(std::wstring_view text) const override;
  [[nodiscard]] std::wstring DecodeText(std::string_view encodedText) const override;
};

/** @brief Converter class for UTF-16 encoding. This class provides methods to convert between UTF-8 strings and UTF-16
 * encoded strings. The FromUtf8 method converts a UTF-8 string to a UTF-16 encoded string, while the ToUtf8 method
 * converts a UTF-16 encoded string back to a UTF-8 string. The actual conversion logic would need to be implemented in
 * these methods, as UTF-8 and UTF-16 are different character encodings with distinct byte representations.
 */
class EoTcConvertUtf16 : public EoTcConverter {
 public:
  EoTcConvertUtf16() : EoTcConverter(nullptr, 0) {}
  [[nodiscard]] std::string EncodeText(std::wstring_view text) const override;
  [[nodiscard]] std::wstring DecodeText(std::string_view encodedText) const override;
};

/** @brief Converter class for character tables. This class provides methods to convert between UTF-8 strings and strings
 * encoded using a specified character table. The FromUtf8 and ToUtf8 methods would need to be implemented to perform the
 * necessary conversions based on the character table provided in the constructor.
 */
class EoTcConvertTable : public EoTcConverter {
 public:
  EoTcConvertTable(const int* table, int length) : EoTcConverter(table, length) {}
  [[nodiscard]] std::string EncodeText(std::wstring_view text) const override;
  [[nodiscard]] std::wstring DecodeText(std::string_view encodedText) const override;
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
  [[nodiscard]] std::string EncodeText(std::wstring_view text) const override;
  [[nodiscard]] std::wstring DecodeText(std::string_view encodedText) const override;

 private:
  const int* m_leadTable;
  const int (*m_doubleTable)[2];
};