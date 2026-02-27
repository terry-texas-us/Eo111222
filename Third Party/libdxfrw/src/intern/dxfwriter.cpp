#include <algorithm>
#include <cctype>
#include <fstream>
#include <ostream>
#include <string>

#include "dxfwriter.h"

bool dxfWriter::WriteUtf8String(int code, std::string text) {
  std::string t = m_encoder.FromUtf8(text);
  return WriteString(code, t);
}

bool dxfWriter::WriteUtf8Caps(int code, std::string text) {
  std::string strname = text;
  std::transform(strname.begin(), strname.end(), strname.begin(), ::toupper);
  std::string t = m_encoder.FromUtf8(strname);
  return WriteString(code, t);
}

bool dxfWriterBinary::WriteString(int code, std::string_view text) {
  char bufcode[2]{};
  bufcode[0] = code & 0xFF;
  bufcode[1] = static_cast<char>(code >> 8);
  m_fileStream->write(bufcode, 2);
  m_fileStream->write(text.data(), static_cast<std::streamsize>(text.size()));
  const char terminator = '\0';
  m_fileStream->write(&terminator, 1);

  return (m_fileStream->good());
}

bool dxfWriterBinary::WriteInt16(int code, int data) {
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

bool dxfWriterBinary::WriteInt32(int code, int data) {
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

bool dxfWriterBinary::WriteInt64(int code, unsigned long long int data) {
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

bool dxfWriterBinary::WriteDouble(int code, double data) {
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

//saved as int or add a bool member??
bool dxfWriterBinary::WriteBool(int code, bool data) {
  char buffer[1]{};
  char bufcode[2]{};
  bufcode[0] = code & 0xFF;
  bufcode[1] = static_cast<char>(code >> 8);
  m_fileStream->write(bufcode, 2);
  buffer[0] = data;
  m_fileStream->write(buffer, 1);
  return (m_fileStream->good());
}

bool dxfWriterAscii::WriteString(int code, std::string_view text) {
  m_fileStream->width(3);
  *m_fileStream << std::right << code << std::endl;
  m_fileStream->width(0);
  *m_fileStream << std::left << text << std::endl;
  return (m_fileStream->good());
}

bool dxfWriterAscii::WriteInt16(int code, int data) {
  m_fileStream->width(3);
  *m_fileStream << std::right << code << std::endl;
  m_fileStream->width(5);
  *m_fileStream << data << std::endl;
  return (m_fileStream->good());
}

bool dxfWriterAscii::WriteInt32(int code, int data) { return WriteInt16(code, data); }

bool dxfWriterAscii::WriteInt64(int code, unsigned long long int data) {
  m_fileStream->width(3);
  *m_fileStream << std::right << code << std::endl;
  m_fileStream->width(5);
  *m_fileStream << data << std::endl;
  return (m_fileStream->good());
}

bool dxfWriterAscii::WriteDouble(int code, double data) {
  m_fileStream->width(3);
  *m_fileStream << std::right << code << std::endl;
  *m_fileStream << data << std::endl;
  return (m_fileStream->good());
}

//saved as int or add a bool member??
bool dxfWriterAscii::WriteBool(int code, bool data) {
  *m_fileStream << code << std::endl << data << std::endl;
  return (m_fileStream->good());
}
