#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>

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

  [[nodiscard]] constexpr std::int16_t GetInt16() const noexcept { return m_int16Data; }
  [[nodiscard]] constexpr std::int32_t GetInt32() const noexcept { return m_int32Data; }
  [[nodiscard]] constexpr std::int64_t GetInt64() const noexcept { return m_int64; }
  [[nodiscard]] constexpr bool GetBool() const noexcept { return m_boolData; }
  [[nodiscard]] constexpr int GetVersion() const noexcept { return m_decoder.GetVersion(); }
  void SetVersion(const std::string& version, bool dxfFormat) { m_decoder.SetVersion(version, dxfFormat); }
  void SetCodePage(const std::string& codePage) { m_decoder.SetCodePage(codePage, true); }
  [[nodiscard]] std::string GetCodePage() const noexcept { return m_decoder.GetCodePage(); }

 protected:
  virtual bool ReadCode(int* code) = 0;  // return true if successful (not EOF)
  virtual bool ReadString(std::string* text) = 0;
  virtual bool ReadString() = 0;
  virtual bool ReadInt16() = 0;
  virtual bool ReadInt32() = 0;
  virtual bool ReadInt64() = 0;
  virtual bool ReadDouble() = 0;
  virtual bool ReadBool() = 0;

  std::string m_string;
  EoTcTextCodec m_decoder;
  std::ifstream* m_fileStream;
  double m_double{};
  std::int64_t m_int64{};
  std::int32_t m_int32Data{};
  std::int16_t m_int16Data{};
  bool m_boolData{};
  bool m_isAsciiFile{};  // set to true for ascii reader, false for binary reader
};

class EoDxfReaderBinary : public EoDxfReader {
 public:
  EoDxfReaderBinary(std::ifstream* stream) : EoDxfReader(stream) { m_isAsciiFile = false; }
  ~EoDxfReaderBinary() = default;
  bool ReadCode(int* code) override;
  bool ReadString(std::string* text) override;
  bool ReadString() override;
  bool ReadInt16() override;
  bool ReadInt32() override;
  bool ReadInt64() override;
  bool ReadDouble() override;

  /** @brief The method reads a single byte from the input stream, interprets it as a boolean value (non-zero is true,
   * zero is false), and stores the result in the m_boolData member variable. Additionally, it sets the m_int16Data
   * member to 1 if the boolean value is true, or 0 if it is false, for reference. The method returns true if the read
   * operation was successful and the stream is still in a good state, or false if an error occurred during reading.
   */
  bool ReadBool() override;

 private:
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
 public:
  EoDxfReaderAscii(std::ifstream* stream) : EoDxfReader(stream) { m_isAsciiFile = true; }
  ~EoDxfReaderAscii() = default;
  bool ReadCode(int* code) override;
  bool ReadString(std::string* text) override;
  bool ReadString() override;
  bool ReadInt16() override;
  bool ReadDouble() override;
  bool ReadInt32() override;
  bool ReadInt64() override;

  /** @brief The method reads the string representation of the boolean value, converts it to an integer using atoi,
   * and then sets the m_boolData member to true if the integer is non-zero, or false if it is zero.
   * @note The integer value is also retained in m_int16Data for reference.
   */
  bool ReadBool() override;
};