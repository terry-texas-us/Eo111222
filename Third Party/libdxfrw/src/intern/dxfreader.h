#pragma once

#include <bit>
#include <cstring>
#include <fstream>
#include <string>

#include "drw_textcodec.h"

class dxfReader {
 public:
  enum class Type : std::uint8_t { String, Int16, Int32, Int64, Double, Bool, Invalid };

  explicit dxfReader(std::ifstream* stream) noexcept : m_fileStream{stream}, m_type{Type::Invalid} {}

  virtual ~dxfReader() = default;
  bool ReadRec(int* code);

  [[nodiscard]] const std::string& GetString() const { return m_string; }
  [[nodiscard]] int GetHandleString() const;
  [[nodiscard]] std::string ToUtf8String(const std::string& t) { return m_decoder.ToUtf8(t); }
  [[nodiscard]] std::string GetUtf8String() const { return m_decoder.ToUtf8(m_string); }
  [[nodiscard]] constexpr double GetDouble() const noexcept { return m_double; }
  [[nodiscard]] constexpr int GetInt32() const noexcept { return m_intData; }
  [[nodiscard]] constexpr unsigned long long int GetInt64() const noexcept { return m_int64; }
  [[nodiscard]] constexpr bool GetBool() const noexcept { return (m_intData == 0) ? false : true; }
  [[nodiscard]] constexpr int GetVersion() const noexcept { return m_decoder.GetVersion(); }
  void SetVersion(std::string* v, bool dxfFormat) { m_decoder.SetVersion(v, dxfFormat); }
  void SetCodePage(std::string* c) { m_decoder.SetCodePage(c, true); }
  [[nodiscard]] std::string GetCodePage() const noexcept { return m_decoder.GetCodePage(); }
  [[nodiscard]] constexpr Type GetType() const noexcept { return m_type; }

 protected:
  virtual bool ReadCode(int* code) = 0;  //return true if successful (not EOF)
  virtual bool ReadString(std::string* text) = 0;
  virtual bool ReadString() = 0;
  virtual bool ReadInt16() = 0;
  virtual bool ReadInt32() = 0;
  virtual bool ReadInt64() = 0;
  virtual bool ReadDouble() = 0;
  virtual bool ReadBool() = 0;

  std::string m_string;
  DRW_TextCodec m_decoder;
  std::ifstream* m_fileStream;
  double m_double{};
  unsigned long long int m_int64{};  //64 bits integer
  signed int m_intData{};            //32 bits integer
  std::int16_t m_int16Data{};        //16 bits integer
  Type m_type{Type::Invalid};
  bool m_isAsciiFile{};  //set to true for ascii reader, false for binary reader
};

class dxfReaderBinary : public dxfReader {
 public:
  dxfReaderBinary(std::ifstream* stream) : dxfReader(stream) { m_isAsciiFile = false; }
  ~dxfReaderBinary() = default;
  bool ReadCode(int* code) override;
  bool ReadString(std::string* text) override;
  bool ReadString() override;
  bool ReadInt16() override;
  bool ReadInt32() override;
  bool ReadInt64() override;
  bool ReadDouble() override;
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
    
    if (!is.read(reinterpret_cast<char*>(buf), sizeof(T))) return T{};
    
    if constexpr (std::endian::native == std::endian::big) { std::reverse(buf, buf + sizeof(T)); }
    
    T value{};
    std::memcpy(&value, buf, sizeof(T));
    return value;
  }
};

class dxfReaderAscii : public dxfReader {
 public:
  dxfReaderAscii(std::ifstream* stream) : dxfReader(stream) { m_isAsciiFile = true; }
  ~dxfReaderAscii() = default;
  bool ReadCode(int* code) override;
  bool ReadString(std::string* text) override;
  bool ReadString() override;
  bool ReadInt16() override;
  bool ReadDouble() override;
  bool ReadInt32() override;
  bool ReadInt64() override;
  bool ReadBool() override;
};