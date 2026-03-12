#pragma once

#include "EoDxfTextCodec.h"

#include <fstream>
#include <string>

class EoDxfWriter {
 public:
  explicit EoDxfWriter(std::ofstream* stream) noexcept : m_fileStream{stream} {}
  virtual ~EoDxfWriter() = default;

  virtual bool WriteString(int code, std::string_view text) = 0;
  virtual bool WriteInt16(int code, std::int16_t data) = 0;
  virtual bool WriteInt32(int code, std::int32_t data) = 0;
  virtual bool WriteInt64(int code, std::int64_t data) = 0;
  virtual bool WriteDouble(int code, double data) = 0;
  virtual bool WriteBool(int code, bool data) = 0;

  void SetVersion(const std::string& version, bool dxfFormat) { m_encoder.SetVersion(version, dxfFormat); }
  void SetCodePage(const std::string& codePage) { m_encoder.SetCodePage(codePage); }
  [[nodiscard]] const std::string& GetCodePage() const { return m_encoder.GetCodePage(); }

  std::string FromUtf8String(std::string t) { return m_encoder.FromUtf8(t); }

  bool WriteUtf8String(int code, std::string text);

 protected:
  std::ofstream* m_fileStream;

 private:
  EoTcTextCodec m_encoder;
};

class EoDxfWriterBinary : public EoDxfWriter {
 public:
  EoDxfWriterBinary(std::ofstream* stream) : EoDxfWriter(stream) {}
  ~EoDxfWriterBinary() = default;

  bool WriteString(int code, std::string_view text) override;
  bool WriteInt16(int code, std::int16_t data) override;
  bool WriteInt32(int code, std::int32_t data) override;
  bool WriteInt64(int code, std::int64_t data) override;
  bool WriteDouble(int code, double data) override;
  bool WriteBool(int code, bool data) override;
};

class EoDxfWriterAscii : public EoDxfWriter {
 public:
  EoDxfWriterAscii(std::ofstream* stream) : EoDxfWriter(stream) { m_fileStream->precision(16); }
  ~EoDxfWriterAscii() = default;

  bool WriteString(int code, std::string_view text) override;
  bool WriteInt16(int code, std::int16_t data) override;
  bool WriteInt32(int code, std::int32_t data) override;
  bool WriteInt64(int code, std::int64_t data) override;
  bool WriteDouble(int code, double data) override;
  bool WriteBool(int code, bool data) override;
};