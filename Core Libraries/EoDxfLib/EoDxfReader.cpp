#include "EoDxfReader.h"

#include <charconv>
#include <cstdlib>
#include <ios>
#include <istream>
#include <sstream>
#include <string>

namespace {
constexpr std::int16_t legacyAcGroupCode90Threshhold{2000};
}

bool EoDxfReader::ReadRec(int* codeData) {
  int code;
  bool readSucceeded{};

  if (!ReadCode(&code)) { return false; }
  *codeData = code;

  if (code <= 9) {
    // String (with the introduction of extended symbol names in AutoCAD 2000,
    // the 255-character limit has been increased to 2049 single-byte characters not including the newline at the end of
    // the line.)
    readSucceeded = ReadString();
  } else if (code < 60) {  // Double precision 3D point value (10-39); Double-precision floating-point value (40-59)
    readSucceeded = ReadDouble();
  } else if (code < 80) {  // 16-bit integer value
    readSucceeded = ReadInt16();
  } else if (code > 89 && code < 100) {  // 32-bit integer value (this is where group code 90 lives)
    readSucceeded = ReadInt32();
  } else if (code == 100 || code == 102) {  // String (255-character maximum, less for Unicode strings)
    readSucceeded = ReadString();
  } else if (code == 105) {  // String representing hexadecimal (hex) handle value
    readSucceeded = ReadString();
  } else if (code > 109 && code < 150) {  // double precision floating-point value (140-149 scalar values)
    readSucceeded = ReadDouble();
  } else if (code > 159 && code < 170) {  // 64-bit integer value
    readSucceeded = ReadInt64();
  } else if (code < 180) {
    readSucceeded = ReadInt16();
  } else if (code > 209 && code < 240) {  // double-precision floating-point value
    readSucceeded = ReadDouble();
  } else if (code > 269 && code < 290) {  // 16-bit integer value
    readSucceeded = ReadInt16();
  } else if (code < 300) {  // boolean flag value (290-299)
    readSucceeded = ReadBool();
  } else if (code < 370) {
    readSucceeded = ReadString();
  } else if (code < 390) {
    readSucceeded = ReadInt16();
  } else if (code < 400) {
    readSucceeded = ReadString();
  } else if (code < 410) {
    readSucceeded = ReadInt16();
  } else if (code < 420) {
    readSucceeded = ReadString();
  } else if (code < 430) {  // 32-bit integer value (420-429)
    readSucceeded = ReadInt32();
  } else if (code < 440) {
    readSucceeded = ReadString();
  } else if (code < 450) {  // 32-bit integer value (440-449)
    readSucceeded = ReadInt32();
  } else if (code < 460) {  // long (450-459)
    readSucceeded = ReadInt32();
  } else if (code < 470) {  // Double-precision floating-point value (460-469)
    readSucceeded = ReadDouble();
  } else if (code < 481) {
    readSucceeded = ReadString();
  } else if (code == 999) {  // is used for comment strings in DXF files
    readSucceeded = ReadString();
  } else if (code >= 1000 && code <= 1009) {  // String (same limits as indicated with 0-9 code range)
    readSucceeded = ReadString();
  } else if (code >= 1010 && code <= 1059) {  // Double-precision floating-point value
    readSucceeded = ReadDouble();
  } else if (code >= 1060 && code <= 1070) {  // 16-bit integer value
    readSucceeded = ReadInt16();
  } else if (code == 1071) {  // 32-bit integer value
    readSucceeded = ReadInt32();
  } else if (m_isAsciiFile) {  // skip code in ascii files because the behavior is predictable, but not in binary files
    readSucceeded = ReadString();
  } else {  // break in binary files because the conduct is unpredictable
    return false;
  }
  return readSucceeded;
}

std::uint64_t EoDxfReader::GetHandleString() const {
  std::uint64_t handleValue;
  std::istringstream Convert(m_string);
  if (!(Convert >> std::hex >> handleValue)) { handleValue = 0; }
  return handleValue;
}

// --- Binary reader ---

bool EoDxfReaderBinary::ReadCode(int* code) {
  // Always read exactly 2 bytes as little-endian uint16_t (official binary DXF rule)
  uint16_t raw = readLE<uint16_t>(*m_fileStream);

  // Some older AutoCAD versions write 32-bit values for code 90 using only 2 bytes when the value fits in 16 bits.
  // This leaves the next "group code" bytes actually being the high 16 bits of the previous 32-bit value → raw > 2000.
  // We detect it by looking at the previous group code and rewind.
  if (m_previousCode == 90 && raw > legacyAcGroupCode90Threshhold) {
    // rewind past the bogus 2 bytes + the 2 bytes that were mistakenly read by the previous ReadInt32()
    m_fileStream->seekg(-4, std::ios_base::cur);
    raw = readLE<uint16_t>(*m_fileStream);  // now read the real next group code
  }
  *code = static_cast<int>(raw);
  m_previousCode = *code;

  return m_fileStream->good();
}

bool EoDxfReaderBinary::ReadString() {
  std::getline(*m_fileStream, m_string, '\0');
  return (m_fileStream->good());
}

bool EoDxfReaderBinary::ReadString(std::string* text) {
  std::getline(*m_fileStream, *text, '\0');
  return (m_fileStream->good());
}

bool EoDxfReaderBinary::ReadInt16() {
  m_int16 = readLE<std::int16_t>(*m_fileStream);
  return m_fileStream->good();
}

bool EoDxfReaderBinary::ReadInt32() {
  m_int32 = readLE<std::int32_t>(*m_fileStream);
  return m_fileStream->good();
}

bool EoDxfReaderBinary::ReadInt64() {
  m_int64 = readLE<std::int64_t>(*m_fileStream);
  return m_fileStream->good();
}

bool EoDxfReaderBinary::ReadDouble() {
  m_double = readLE<double>(*m_fileStream);
  return m_fileStream->good();
}

bool EoDxfReaderBinary::ReadBool() {
  m_boolData = readLE<uint8_t>(*m_fileStream) != 0;
  m_int16 = m_boolData ? 1 : 0;
  return m_fileStream->good();
}

// --- Ascii reader ---

bool EoDxfReaderAscii::ReadCode(int* code) {
  std::string text;
  std::getline(*m_fileStream, text);
  *code = atoi(text.c_str());
  return (m_fileStream->good());
}
bool EoDxfReaderAscii::ReadString(std::string* text) {
  std::getline(*m_fileStream, *text);
  if (!text->empty() && text->at(text->size() - 1) == '\r') { text->erase(text->size() - 1); }
  return (m_fileStream->good());
}

bool EoDxfReaderAscii::ReadString() {
  std::getline(*m_fileStream, m_string);
  if (!m_string.empty() && m_string.at(m_string.size() - 1) == '\r') { m_string.erase(m_string.size() - 1); }
  return (m_fileStream->good());
}

bool EoDxfReaderAscii::ReadInt16() { return ParseInteger(m_int16); }
bool EoDxfReaderAscii::ReadInt32() { return ParseInteger(m_int32); }
bool EoDxfReaderAscii::ReadInt64() { return ParseInteger(m_int64); }


bool EoDxfReaderAscii::ReadDouble() {
  std::string text;
  if (!ReadString(&text)) { return false; }
  std::istringstream sd(text);
  sd >> m_double;
  return true;
}

bool EoDxfReaderAscii::ReadBool() {
  std::string text;
  if (!ReadString(&text)) { return false; }
  m_int16 = atoi(text.c_str());
  m_boolData = (m_int16 != 0);
  return true;
}
