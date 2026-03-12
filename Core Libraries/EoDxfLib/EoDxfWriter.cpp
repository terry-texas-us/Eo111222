#include <ostream>
#include <string>
#include <string_view>

#include "EoDxfWriter.h"

bool EoDxfWriter::WriteWideString(int code, std::wstring_view text) {
  return WriteEncodedText(code, m_encoder.EncodeText(text));
}

bool EoDxfWriterBinary::WriteEncodedText(int code, std::string_view encodedText) {
  char bufcode[2]{};
  bufcode[0] = code & 0xFF;
  bufcode[1] = static_cast<char>(code >> 8);
  m_fileStream->write(bufcode, 2);
  m_fileStream->write(encodedText.data(), static_cast<std::streamsize>(encodedText.size()));
  const char terminator = '\0';
  m_fileStream->write(&terminator, 1);

  return (m_fileStream->good());
}

bool EoDxfWriterBinary::WriteInt16(int code, std::int16_t data) {
  char bufcode[2]{};
  char buffer[2]{};
  bufcode[0] = code & 0xFF;
  bufcode[1] = static_cast<char>(code >> 8);
  buffer[0] = data & 0xFF;
  buffer[1] = static_cast<char>(data >> 8);
  m_fileStream->write(bufcode, 2);
  m_fileStream->write(buffer, 2);
  return (m_fileStream->good());
}

bool EoDxfWriterBinary::WriteInt32(int code, std::int32_t data) {
  char buffer[4]{};
  buffer[0] = code & 0xFF;
  buffer[1] = static_cast<char>(code >> 8);
  m_fileStream->write(buffer, 2);

  buffer[0] = data & 0xFF;
  buffer[1] = static_cast<char>(data >> 8);
  buffer[2] = static_cast<char>(data >> 16);
  buffer[3] = data >> 24;
  m_fileStream->write(buffer, 4);
  return (m_fileStream->good());
}

bool EoDxfWriterBinary::WriteInt64(int code, std::int64_t data) {
  char buffer[8]{};
  buffer[0] = code & 0xFF;
  buffer[1] = static_cast<char>(code >> 8);
  m_fileStream->write(buffer, 2);

  buffer[0] = data & 0xFF;
  buffer[1] = static_cast<char>(data >> 8);
  buffer[2] = static_cast<char>(data >> 16);
  buffer[3] = static_cast<char>(data >> 24);
  buffer[4] = static_cast<char>(data >> 32);
  buffer[5] = static_cast<char>(data >> 40);
  buffer[6] = static_cast<char>(data >> 48);
  buffer[7] = static_cast<char>(data >> 56);
  m_fileStream->write(buffer, 8);
  return (m_fileStream->good());
}

bool EoDxfWriterBinary::WriteDouble(int code, double data) {
  char bufcode[2]{};
  char buffer[8]{};
  bufcode[0] = code & 0xFF;
  bufcode[1] = static_cast<char>(code >> 8);
  m_fileStream->write(bufcode, 2);

  unsigned char* val;
  val = (unsigned char*)&data;
  for (int i = 0; i < 8; i++) { buffer[i] = val[i]; }
  m_fileStream->write(buffer, 8);
  return (m_fileStream->good());
}

// saved as int or add a bool member??
bool EoDxfWriterBinary::WriteBool(int code, bool data) {
  char buffer[1]{};
  char bufcode[2]{};
  bufcode[0] = code & 0xFF;
  bufcode[1] = static_cast<char>(code >> 8);
  m_fileStream->write(bufcode, 2);
  buffer[0] = data;
  m_fileStream->write(buffer, 1);
  return (m_fileStream->good());
}

bool EoDxfWriterAscii::WriteEncodedText(int code, std::string_view encodedText) {
  m_fileStream->width(3);
  *m_fileStream << std::right << code << std::endl;
  m_fileStream->width(0);
  *m_fileStream << std::left << encodedText << std::endl;
  return (m_fileStream->good());
}

bool EoDxfWriterAscii::WriteInt16(int code, std::int16_t data) {
  m_fileStream->width(3);
  *m_fileStream << std::right << code << std::endl;
  m_fileStream->width(5);
  *m_fileStream << data << std::endl;
  return (m_fileStream->good());
}

bool EoDxfWriterAscii::WriteInt32(int code, std::int32_t data) {
  m_fileStream->width(3);
  *m_fileStream << std::right << code << std::endl;
  m_fileStream->width(5);
  *m_fileStream << data << std::endl;
  return (m_fileStream->good());
}

bool EoDxfWriterAscii::WriteInt64(int code, std::int64_t data) {
  m_fileStream->width(3);
  *m_fileStream << std::right << code << std::endl;
  m_fileStream->width(5);
  *m_fileStream << data << std::endl;
  return (m_fileStream->good());
}

bool EoDxfWriterAscii::WriteDouble(int code, double data) {
  m_fileStream->width(3);
  *m_fileStream << std::right << code << std::endl;
  *m_fileStream << data << std::endl;
  return (m_fileStream->good());
}

// saved as int or add a bool member??
bool EoDxfWriterAscii::WriteBool(int code, bool data) {
  *m_fileStream << code << std::endl << static_cast<int>(data) << std::endl;
  return (m_fileStream->good());
}
