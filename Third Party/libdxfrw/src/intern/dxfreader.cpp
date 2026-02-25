#include "dxfreader.h"

#include <cstdlib>
#include <ios>
#include <istream>
#include <sstream>
#include <string>

#include "drw_dbg.h"

namespace {
constexpr std::int16_t legacyAcGroupCode90Threshhold{2000};
}

bool dxfReader::ReadRec(int* codeData) {
  int code;

  if (!ReadCode(&code)) { return false; }
  *codeData = code;

  if (code <= 9) {
    // String (with the introduction of extended symbol names in AutoCAD 2000,
    // the 255-character limit has been increased to 2049 single-byte characters not including the newline at the end of the line.)
    ReadString();
  } else if (code < 60) {  // Double precision 3D point value (10-39); Double-precision floating-point value (40-59)
    ReadDouble();
  } else if (code < 80) {  // 16-bit integer value
    ReadInt16();
  } else if (code > 89 && code < 100) {  // 32-bit integer value (this is where group code 90 lives)
    ReadInt32();
  } else if (code == 100 || code == 102) {  // String (255-character maximum, less for Unicode strings)
    ReadString();
  } else if (code == 105) {  // String representing hexadecimal (hex) handle value
    ReadString();
  } else if (code > 109 && code < 150) {  // double precision floating-point value (140-149 scalar values)
    ReadDouble();
  } else if (code > 159 && code < 170) {  // 64-bit integer value
    ReadInt64();
  } else if (code < 180) {
    ReadInt16();
  } else if (code > 209 && code < 240) {  // double-precision floating-point value
    ReadDouble();
  } else if (code > 269 && code < 290) {  // 16-bit integer value
    ReadInt16();
  } else if (code < 300) {  // boolean flag value (290-299)
    ReadBool();
  } else if (code < 370) {
    ReadString();
  } else if (code < 390) {
    ReadInt16();
  } else if (code < 400) {
    ReadString();
  } else if (code < 410) {
    ReadInt16();
  } else if (code < 420) {
    ReadString();
  } else if (code < 430) {  // 32-bit integer value (420-429)
    ReadInt32();
  } else if (code < 440) {
    ReadString();
  } else if (code < 450) {  // 32-bit integer value (440-449)
    ReadInt32();
  } else if (code < 460) {  // long (450-459)
    ReadInt32();
  } else if (code < 470) {  // Double-precision floating-point value (460-469)
    ReadDouble();
  } else if (code < 481) {
    ReadString();
  } else if (code == 999) {  // is used for comment strings in DXF files
    ReadString();
  } else if (code >= 1000 && code <= 1009) {  // String (same limits as indicated with 0-9 code range)
    ReadString();
  } else if (code >= 1010 && code <= 1059) {  // Double-precision floating-point value
    ReadDouble();
  } else if (code >= 1060 && code <= 1070) {  // 16-bit integer value
    ReadInt16();
  } else if (code == 1071) {  // 32-bit integer value
    ReadInt32();
  } else if (m_isAsciiFile) {  // skip code in ascii files because the behavior is predictable, but not in binary files
    ReadString();
  } else {  // break in binary files because the conduct is unpredictable
    return false;
  }
  return (m_fileStream->good());
}

int dxfReader::GetHandleString() const {
  int res;
  std::istringstream Convert(m_string);
  if (!(Convert >> std::hex >> res)) res = 0;
  return res;
}

// --- Binary reader ---

bool dxfReaderBinary::ReadCode(int* code) {
  // Always read exactly 2 bytes as little-endian uint16_t (official binary DXF rule)
  uint16_t raw = readLE<uint16_t>(*m_fileStream);

  // Some older AutoCAD versions write 32-bit values for code 90 using only 2 bytes when the value fits in 16 bits.
  // This leaves the next "group code" bytes actually being the high 16 bits of the previous 32-bit value → raw > 2000.
  // We detect it by looking at the PREVIOUS code (*code == 90) and rewind.
  if (*code == 90 && raw > legacyAcGroupCode90Threshhold) {
    // rewind past the bogus 2 bytes + the 2 bytes that were mistakenly read by the previous ReadInt32()
    m_fileStream->seekg(-4, std::ios_base::cur);
    raw = readLE<uint16_t>(*m_fileStream);  // now read the real next group code
  }
  *code = static_cast<int>(raw);
  DRW_DBG(*code);
  DRW_DBG("\n");

  return m_fileStream->good();
}

bool dxfReaderBinary::ReadString() {
  m_type = Type::String;
  std::getline(*m_fileStream, m_string, '\0');
  DRW_DBG(m_string);
  DRW_DBG(" `group value (string)`\n");
  return (m_fileStream->good());
}

bool dxfReaderBinary::ReadString(std::string* text) {
  m_type = Type::String;
  std::getline(*m_fileStream, *text, '\0');
  DRW_DBG(*text);
  DRW_DBG(" `group value (string)`\n");
  return (m_fileStream->good());
}

bool dxfReaderBinary::ReadInt16() {
  m_type = Type::Int32;
  m_int16Data = readLE<std::int16_t>(*m_fileStream);  // or uint16_t depending on semantics
  m_intData = m_int16Data;                            // keep your existing storage
  DRW_DBG(m_intData);
  DRW_DBG(" `group value (int16)`\n");
  return m_fileStream->good();
}

bool dxfReaderBinary::ReadInt32() {
  m_type = Type::Int32;
  m_intData = readLE<std::int32_t>(*m_fileStream);
  DRW_DBG(m_intData);
  DRW_DBG(" `group value (int32)`\n");
  return m_fileStream->good();
}

bool dxfReaderBinary::ReadInt64() {
  m_type = Type::Int64;
  m_int64 = readLE<std::uint64_t>(*m_fileStream);
  DRW_DBG(m_int64);
  DRW_DBG(" `group value (int64)`\n");
  return m_fileStream->good();
}

bool dxfReaderBinary::ReadDouble() {
  m_type = Type::Double;
  m_double = readLE<double>(*m_fileStream);
  DRW_DBG(m_double);
  DRW_DBG(" `group value (double)`\n");
  return m_fileStream->good();
}

bool dxfReaderBinary::ReadBool() {
  m_type = Type::Bool;
  m_intData = readLE<uint8_t>(*m_fileStream);  // bool is stored as single byte (0 or 1)
  DRW_DBG(m_intData);
  DRW_DBG(" `group value (bool)`\n");
  return m_fileStream->good();
}

// --- Ascii reader ---

bool dxfReaderAscii::ReadCode(int* code) {
  std::string text;
  std::getline(*m_fileStream, text);
  *code = atoi(text.c_str());
  DRW_DBG(*code);
  DRW_DBG(" `group code`\n");
  return (m_fileStream->good());
}
bool dxfReaderAscii::ReadString(std::string* text) {
  m_type = Type::String;
  std::getline(*m_fileStream, *text);
  if (!text->empty() && text->at(text->size() - 1) == '\r') { text->erase(text->size() - 1); }
  return (m_fileStream->good());
}

bool dxfReaderAscii::ReadString() {
  m_type = Type::String;
  std::getline(*m_fileStream, m_string);
  if (!m_string.empty() && m_string.at(m_string.size() - 1) == '\r') { m_string.erase(m_string.size() - 1); }
  DRW_DBG(m_string);
  DRW_DBG(" `group value (string)`\n");
  return (m_fileStream->good());
}

bool dxfReaderAscii::ReadInt16() {
  m_type = Type::Int32;
  std::string text;
  if (!ReadString(&text)) { return false; }
  m_intData = atoi(text.c_str());
  DRW_DBG(m_intData);
  DRW_DBG(" `group value (int16)`\n");
  return true;
}

bool dxfReaderAscii::ReadInt32() {
  m_type = Type::Int32;
  return ReadInt16();
}

bool dxfReaderAscii::ReadInt64() {
  m_type = Type::Int64;
  return ReadInt16();
}

bool dxfReaderAscii::ReadDouble() {
  m_type = Type::Double;
  std::string text;
  if (!ReadString(&text)) { return false; }
  std::istringstream sd(text);
  sd >> m_double;
  DRW_DBG(m_double);
  DRW_DBG(" `group value (double)`\n");
  return true;
}

// saved as int or add a bool member??
bool dxfReaderAscii::ReadBool() {
  m_type = Type::Bool;
  std::string text;
  if (!ReadString(&text)) { return false; }
  m_intData = atoi(text.c_str());
  DRW_DBG(m_intData);
  DRW_DBG(" `group value (bool)`\n");
  return true;
}
