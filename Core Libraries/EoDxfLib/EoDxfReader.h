#pragma once

#include <bit>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>

#include "EoDxfBase.h"
#include "EoDxfTextCodec.h"

class EoDxfReader {
 public:
  explicit EoDxfReader(std::ifstream* stream) noexcept : m_fileStream{stream} {}
  virtual ~EoDxfReader() = default;
  bool ReadRec(int* code);

  [[nodiscard]] const std::string& GetString() const { return m_string; }
  [[nodiscard]] std::uint64_t GetHandleString() const;
  [[nodiscard]] std::string ToUtf8String(const std::string& t) { return m_decoder.ToUtf8(t); }
  [[nodiscard]] std::string GetUtf8String() const { return m_decoder.ToUtf8(m_string); }
  [[nodiscard]] constexpr double GetDouble() const noexcept { return m_double; }

  [[nodiscard]] constexpr std::int16_t GetInt16() const noexcept { return m_int16; }
  [[nodiscard]] constexpr std::int32_t GetInt32() const noexcept { return m_int32; }
  [[nodiscard]] constexpr std::int64_t GetInt64() const noexcept { return m_int64; }
  [[nodiscard]] constexpr bool GetBool() const noexcept { return m_boolData; }
  [[nodiscard]] constexpr int GetVersion() const noexcept { return m_decoder.GetVersion(); }
  void SetVersion(const std::string& version) { m_decoder.SetVersion(version); }
  void SetCodePage(const std::string& codePage) { m_decoder.SetCodePage(codePage); }
  [[nodiscard]] std::string GetCodePage() const noexcept { return m_decoder.GetCodePage(); }

 protected:
  virtual bool ReadCode(int* code) = 0;  // return true if successful (not EOF)
  virtual bool ReadString() = 0;
  virtual bool ReadString(std::string* text) = 0;
  virtual bool ReadInt16() = 0;
  virtual bool ReadInt32() = 0;
  virtual bool ReadInt64() = 0;
  virtual bool ReadBool() = 0;
  virtual bool ReadDouble() = 0;

  std::string m_string;
  EoTcTextCodec m_decoder;
  std::ifstream* m_fileStream;
  double m_double{};
  std::int64_t m_int64{};
  std::int32_t m_int32{};
  std::int16_t m_int16{};
  bool m_boolData{};
  bool m_isAsciiFile{};  // set to true for ascii reader, false for binary reader
};

class EoDxfReaderBinary : public EoDxfReader {
 public:
  EoDxfReaderBinary(std::ifstream* stream) : EoDxfReader(stream) { m_isAsciiFile = false; }
  ~EoDxfReaderBinary() = default;

  [[nodiscard]] bool ReadCode(int* code) override;
  [[nodiscard]] bool ReadString() override;
  [[nodiscard]] bool ReadString(std::string* text) override;
  [[nodiscard]] bool ReadInt16() override;
  [[nodiscard]] bool ReadInt32() override;
  [[nodiscard]] bool ReadInt64() override;

  /** @brief The method reads a single byte from the input stream, interprets it as a boolean value (non-zero is true,
   * zero is false), and stores the result in the m_boolData member variable. Additionally, it sets the m_int16
   * member to 1 if the boolean value is true, or 0 if it is false, for reference. The method returns true if the read
   * operation was successful and the stream is still in a good state, or false if an error occurred during reading.
   */
  bool ReadBool() override;

  bool ReadDouble() override;

 private:
  /** @brief Threshold for detecting legacy AutoCAD versions that write 32-bit values for code 90 using only 2 bytes
   * when the value fits in 16 bits.*/
  int m_previousCode{};

  /** @brief Reads a value of type T from the input stream in little-endian format.
   *  The function reads sizeof(T) bytes from the stream, reverses the byte order if the system is big-endian,
   *  and then copies the bytes into a variable of type T to return the value.
   *  @tparam T The type of value to read (must be trivially copyable).
   *  @param is The input stream to read from.
   *  @return The value read from the stream, or a default-constructed T if reading fails.
   */
  template <typename T>
  T readLE(std::istream& is) {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable (int, double, uint64_t, etc.)");

    alignas(T) std::byte buf[sizeof(T)]{};

    if (!is.read(reinterpret_cast<char*>(buf), sizeof(T))) { return T{}; }

    if constexpr (std::endian::native == std::endian::big) { std::reverse(buf, buf + sizeof(T)); }

    T value{};
    std::memcpy(&value, buf, sizeof(T));
    return value;
  }
};

class EoDxfReaderAscii : public EoDxfReader {
  /** @brief Parses an integer value of type T from the current string buffer.
   *  The function reads a string using ReadString(), trims leading and trailing whitespace, and then attempts to
   *  convert the trimmed string to an integer of type T using std::from_chars. If the conversion is successful and
   *  consumes the entire trimmed string, the resulting value is stored in the output parameter 'out' and the function
   *  returns true. If any step fails (e.g., reading the string, trimming results in an empty string, conversion fails,
   *  or extra characters remain after conversion), the function returns false.
   *
   *  @tparam T The integer type to parse (e.g., int16_t, int32_t, int64_t).
   *  @param out Reference to a variable where the parsed integer value will be stored if successful.
   *  @return true if parsing was successful and 'out' contains a valid integer; false otherwise.
   */
  template <typename T>
  [[nodiscard]] bool ParseInteger(T& out) {
    std::string text;
    if (!ReadString(&text)) { return false; }

    std::string_view trimmedText = EoDxf::Detail::Trim(text);
    if (trimmedText.empty()) { return false; }

    T value{};
    auto [ptr, ec] = std::from_chars(trimmedText.data(), trimmedText.data() + trimmedText.size(), value);
    if (ec != std::errc{} || ptr != trimmedText.data() + trimmedText.size()) { return false; }

    out = value;
    return true;
  }

  [[nodiscard]] bool ParseDouble(double& out) {
    std::string text;
    if (!ReadString(&text)) { return false; }

    std::string_view trimmedText = EoDxf::Detail::Trim(text);
    if (trimmedText.empty()) { return false; }

    double value{};
    auto [ptr, ec] = std::from_chars(
        trimmedText.data(), trimmedText.data() + trimmedText.size(), value, std::chars_format::general);
    if (ec != std::errc{} || ptr != trimmedText.data() + trimmedText.size()) { return false; }

    out = value;
    return true;
  }

 public:
  EoDxfReaderAscii(std::ifstream* stream) : EoDxfReader(stream) { m_isAsciiFile = true; }
  ~EoDxfReaderAscii() = default;

  [[nodiscard]] bool ReadCode(int* code) override;
  [[nodiscard]] bool ReadString() override;
  [[nodiscard]] bool ReadString(std::string* text) override;
  [[nodiscard]] bool ReadInt16() override;
  [[nodiscard]] bool ReadInt32() override;
  [[nodiscard]] bool ReadInt64() override;

  /** @brief The method reads the string representation of the boolean value, converts it to an integer using
   * std::from_chars, and then sets the m_boolData member to true if the integer is non-zero, or false if it is zero.
   * @note The integer value is also retained in m_int16 for reference.
   */
  bool ReadBool() override;

  bool ReadDouble() override;
};