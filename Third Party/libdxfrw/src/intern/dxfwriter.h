#pragma once

#include "drw_textcodec.h"

#include <fstream>
#include <string>

class dxfWriter {
 public:
  explicit dxfWriter(std::ofstream* stream) noexcept : m_fileStream{stream} {}
  virtual ~dxfWriter() = default;
  
  virtual bool WriteString(int code, std::string_view text) = 0;
  virtual bool WriteInt16(int code, int data) = 0;
  virtual bool WriteInt32(int code, int data) = 0;
  virtual bool WriteInt64(int code, unsigned long long int data) = 0;
  virtual bool WriteDouble(int code, double data) = 0;
  virtual bool WriteBool(int code, bool data) = 0;
  
  void SetVersion(std::string* version, bool dxfFormat) { m_encoder.SetVersion(version, dxfFormat); }
  void SetCodePage(std::string* codePage) { m_encoder.SetCodePage(codePage, true); }
  [[nodiscard]] const std::string& GetCodePage() const { return m_encoder.GetCodePage(); }
  
  std::string FromUtf8String(std::string t) { return m_encoder.FromUtf8(t); }

  bool WriteUtf8String(int code, std::string text);
  bool WriteUtf8Caps(int code, std::string text);
  
 protected:
  std::ofstream* m_fileStream;

 private:
  DRW_TextCodec m_encoder;
};

class dxfWriterBinary : public dxfWriter {
 public:
  dxfWriterBinary(std::ofstream* stream) : dxfWriter(stream) {}
  ~dxfWriterBinary() = default;
  
  bool WriteString(int code, std::string_view text) override;
  bool WriteInt16(int code, int data) override;
  bool WriteInt32(int code, int data) override;
  bool WriteInt64(int code, unsigned long long int data) override;
  bool WriteDouble(int code, double data) override;
  bool WriteBool(int code, bool data) override;
};

class dxfWriterAscii : public dxfWriter {
 public:
  dxfWriterAscii(std::ofstream* stream) : dxfWriter(stream) { m_fileStream->precision(16); }
  ~dxfWriterAscii() = default;
  
  bool WriteString(int code, std::string_view text) override;
  bool WriteInt16(int code, int data) override;
  bool WriteInt32(int code, int data) override;
  bool WriteInt64(int code, unsigned long long int data) override;
  bool WriteDouble(int code, double data) override;
  bool WriteBool(int code, bool data) override;
};