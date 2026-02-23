#include <algorithm>
#include <cctype>
#include <fstream>
#include <ostream>
#include <string>

#include "dxfwriter.h"

bool dxfWriter::writeUtf8String(int code, std::string text) {
  std::string t = encoder.fromUtf8(text);
  return writeString(code, t);
}

bool dxfWriter::writeUtf8Caps(int code, std::string text) {
  std::string strname = text;
  std::transform(strname.begin(), strname.end(), strname.begin(), ::toupper);
  std::string t = encoder.fromUtf8(strname);
  return writeString(code, t);
}

bool dxfWriterBinary::writeString(int code, std::string text) {
  char bufcode[2]{ 0 };
  bufcode[0] = code & 0xFF;
  bufcode[1] = static_cast<char>(code >> 8);
  filestr->write(bufcode, 2);
  *filestr << text << '\0';
  return (filestr->good());
}

bool dxfWriterBinary::writeInt16(int code, int data) {
  char bufcode[2]{ 0 };
  char buffer[2]{ 0 };
  bufcode[0] = code & 0xFF;
  bufcode[1] = static_cast<char>(code >> 8);
  buffer[0] = data & 0xFF;
  buffer[1] = static_cast<char>(data >> 8);
  filestr->write(bufcode, 2);
  filestr->write(buffer, 2);
  return (filestr->good());
}

bool dxfWriterBinary::writeInt32(int code, int data) {
  char buffer[4]{ 0 };
  buffer[0] = code & 0xFF;
  buffer[1] = static_cast<char>(code >> 8);
  filestr->write(buffer, 2);

  buffer[0] = data & 0xFF;
  buffer[1] = static_cast<char>(data >> 8);
  buffer[2] = static_cast<char>(data >> 16);
  buffer[3] = data >> 24;
  filestr->write(buffer, 4);
  return (filestr->good());
}

bool dxfWriterBinary::writeInt64(int code, unsigned long long int data) {
  char buffer[8]{ 0 };
  buffer[0] = code & 0xFF;
  buffer[1] = static_cast<char>(code >> 8);
  filestr->write(buffer, 2);

  buffer[0] = data & 0xFF;
  buffer[1] = static_cast<char>(data >> 8);
  buffer[2] = static_cast<char>(data >> 16);
  buffer[3] = static_cast<char>(data >> 24);
  buffer[4] = static_cast<char>(data >> 32);
  buffer[5] = static_cast<char>(data >> 40);
  buffer[6] = static_cast<char>(data >> 48);
  buffer[7] = static_cast<char>(data >> 56);
  filestr->write(buffer, 8);
  return (filestr->good());
}

bool dxfWriterBinary::writeDouble(int code, double data) {
  char bufcode[2]{ 0 };
  char buffer[8]{ 0 };
  bufcode[0] = code & 0xFF;
  bufcode[1] = static_cast<char>(code >> 8);
  filestr->write(bufcode, 2);

  unsigned char* val;
  val = (unsigned char*)&data;
  for (int i = 0; i < 8; i++) {
    buffer[i] = val[i];
  }
  filestr->write(buffer, 8);
  return (filestr->good());
}

//saved as int or add a bool member??
bool dxfWriterBinary::writeBool(int code, bool data) {
  char buffer[1]{ 0 };
  char bufcode[2]{ 0 };
  bufcode[0] = code & 0xFF;
  bufcode[1] = static_cast<char>(code >> 8);
  filestr->write(bufcode, 2);
  buffer[0] = data;
  filestr->write(buffer, 1);
  return (filestr->good());
}

dxfWriterAscii::dxfWriterAscii(std::ofstream* stream) :dxfWriter(stream) {
  filestr->precision(16);
}

bool dxfWriterAscii::writeString(int code, std::string text) {
  //    *filestr << code << std::endl << text << std::endl ;
  filestr->width(3);
  *filestr << std::right << code << std::endl;
  filestr->width(0);
  *filestr << std::left << text << std::endl;
  /*    std::getline(*filestr, strData, '\0');
  DBG(strData); DBG("\n");*/
  return (filestr->good());
}

bool dxfWriterAscii::writeInt16(int code, int data) {
  //    *filestr << std::right << code << std::endl << data << std::endl;
  filestr->width(3);
  *filestr << std::right << code << std::endl;
  filestr->width(5);
  *filestr << data << std::endl;
  return (filestr->good());
}

bool dxfWriterAscii::writeInt32(int code, int data) {
  return writeInt16(code, data);
}

bool dxfWriterAscii::writeInt64(int code, unsigned long long int data) {
  //    *filestr << code << std::endl << data << std::endl;
  filestr->width(3);
  *filestr << std::right << code << std::endl;
  filestr->width(5);
  *filestr << data << std::endl;
  return (filestr->good());
}

bool dxfWriterAscii::writeDouble(int code, double data) {
  //    std::streamsize prec = filestr->precision();
  //    filestr->precision(12);
  //    *filestr << code << std::endl << data << std::endl;
  filestr->width(3);
  *filestr << std::right << code << std::endl;
  *filestr << data << std::endl;
  //    filestr->precision(prec);
  return (filestr->good());
}

//saved as int or add a bool member??
bool dxfWriterAscii::writeBool(int code, bool data) {
  *filestr << code << std::endl << data << std::endl;
  return (filestr->good());
}

