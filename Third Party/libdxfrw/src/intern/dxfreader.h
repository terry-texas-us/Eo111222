#pragma once

#include <fstream>
#include <string>

#include "drw_textcodec.h"

class dxfReader {
 public:
  enum class Type : std::uint8_t { String, Int16, Int32, Int64, Double, Bool, Invalid };

  dxfReader(std::ifstream* stream) {
    filestr = stream;
    type = Type::Invalid;
  }
  virtual ~dxfReader() {}
  bool readRec(int* code);

  std::string getString() { return strData; }
  int getHandleString() const;  //Convert hex string to int
  std::string toUtf8String(std::string t) { return decoder.toUtf8(t); }
  std::string getUtf8String() { return decoder.toUtf8(strData); }
  double getDouble() const { return doubleData; }
  int getInt32() const { return intData; }
  unsigned long long int getInt64() const { return int64; }
  bool getBool() const { return (intData == 0) ? false : true; }
  int getVersion() { return decoder.getVersion(); }
  void setVersion(std::string* v, bool dxfFormat) { decoder.setVersion(v, dxfFormat); }
  void setCodePage(std::string* c) { decoder.setCodePage(c, true); }
  std::string getCodePage() { return decoder.getCodePage(); }
  [[nodiscard]] constexpr Type GetType() const noexcept { return type; }

 protected:
  virtual bool readCode(int* code) = 0;  //return true if sucesful (not EOF)
  virtual bool readString(std::string* text) = 0;
  virtual bool readString() = 0;
  virtual bool readInt16() = 0;
  virtual bool readInt32() = 0;
  virtual bool readInt64() = 0;
  virtual bool readDouble() = 0;
  virtual bool readBool() = 0;

  std::string strData;
  DRW_TextCodec decoder;  // moved here for alignment (was private)
  std::ifstream* filestr;
  double doubleData{};
  unsigned long long int int64{};  //64 bits integer
  signed int intData{};            //32 bits integer
  std::int16_t int16Data{};        //16 bits integer
  Type type{Type::Invalid};
  bool skip{};  //set to true for ascii dxf, false for binary
};

class dxfReaderBinary : public dxfReader {
 public:
  dxfReaderBinary(std::ifstream* stream) : dxfReader(stream) { skip = false; }
  virtual ~dxfReaderBinary() {}
  virtual bool readCode(int* code);
  virtual bool readString(std::string* text);
  virtual bool readString();
  virtual bool readInt16();
  virtual bool readInt32();
  virtual bool readInt64();
  virtual bool readDouble();
  virtual bool readBool();
};

class dxfReaderAscii : public dxfReader {
 public:
  dxfReaderAscii(std::ifstream* stream) : dxfReader(stream) { skip = true; }
  virtual ~dxfReaderAscii() {}
  virtual bool readCode(int* code);
  virtual bool readString(std::string* text);
  virtual bool readString();
  virtual bool readInt16();
  virtual bool readDouble();
  virtual bool readInt32();
  virtual bool readInt64();
  virtual bool readBool();
};