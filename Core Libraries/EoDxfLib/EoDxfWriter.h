#pragma once

#include "EoDxfTextCodec.h"

#include <fstream>
#include <string>

class EoDxfWriter {
 public:
  explicit EoDxfWriter(std::ofstream* stream) noexcept : m_fileStream{stream} {}
  virtual ~EoDxfWriter() = default;

  virtual bool WriteEncodedText(int code, std::string_view encodedText) = 0;
  virtual bool WriteInt16(int code, std::int16_t data) = 0;
  virtual bool WriteInt32(int code, std::int32_t data) = 0;
  virtual bool WriteInt64(int code, std::int64_t data) = 0;
  virtual bool WriteDouble(int code, double data) = 0;
  virtual bool WriteBool(int code, bool data) = 0;

  void SetVersion(std::wstring_view version) { m_encoder.SetVersion(version); }
  void SetCodePage(std::wstring_view codePage) { m_encoder.SetCodePage(codePage); }
  [[nodiscard]] const std::wstring& GetCodePage() const { return m_encoder.GetCodePage(); }

  bool WriteWideString(int code, std::wstring_view text);

 protected:
  std::ofstream* m_fileStream;

 private:
  EoTcTextCodec m_encoder;
};

class EoDxfWriterBinary : public EoDxfWriter {
 public:
  EoDxfWriterBinary(std::ofstream* stream) : EoDxfWriter(stream) {}
  ~EoDxfWriterBinary() = default;

  bool WriteEncodedText(int code, std::string_view encodedText) override;
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

  bool WriteEncodedText(int code, std::string_view encodedText) override;
  bool WriteInt16(int code, std::int16_t data) override;
  bool WriteInt32(int code, std::int32_t data) override;
  bool WriteInt64(int code, std::int64_t data) override;
  bool WriteDouble(int code, double data) override;
  bool WriteBool(int code, bool data) override;
};