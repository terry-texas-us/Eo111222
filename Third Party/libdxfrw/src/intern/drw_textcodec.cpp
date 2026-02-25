#include <algorithm>
#include <cctype>
#include <cstring>
#include <iconv.h>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

#include "../drw_base.h"
#include "drw_cptables.h"
#include "drw_textcodec.h"

namespace {
constexpr auto CPOFFSET{0x80};  //first entry in table 0x80
constexpr auto CPLENGHTCOMMON{128};
constexpr auto DBCS_REPLACEMENT_CHAR{0x003F};
}  // namespace

DRW_TextCodec::DRW_TextCodec() {
  version = DRW::Version::AC1021;
  conv = new DRW_Converter(nullptr, 0);
}

DRW_TextCodec::~DRW_TextCodec() { delete conv; }

void DRW_TextCodec::setVersion(int v, bool dxfFormat) {
  if (v == DRW::Version::AC1009 || v == DRW::Version::AC1006) {
    version = DRW::Version::AC1009;
    cp = "ANSI_1252";
    setCodePage(&cp, dxfFormat);
  } else if (v == DRW::Version::AC1012 || v == DRW::Version::AC1014 || v == DRW::Version::AC1015 ||
             v == DRW::Version::AC1018) {
    version = DRW::Version::AC1015;
    //        if (cp.empty()) { //codepage not set, initialize
    cp = "ANSI_1252";
    setCodePage(&cp, dxfFormat);
    //        }
  } else {
    version = DRW::Version::AC1021;
    if (dxfFormat)
      cp = "UTF-8";  //RLZ: can be UCS2 or UTF-16 16bits per char
    else
      cp = "UTF-16";  //RLZ: can be UCS2 or UTF-16 16bits per char
    setCodePage(&cp, dxfFormat);
  }
}
/** @brief Sets the version for the text codec based on the provided string and format.
 *  The function checks the version string against known versions and sets the internal version accordingly.
 *  If the version string does not match any known versions, it defaults to DRW::Version::AC1021.
 *  @param v Pointer to a string representing the version to set
 *  @param dxfFormat Boolean indicating whether the format is DXF or not
 */
void DRW_TextCodec::setVersion(std::string* v, bool dxfFormat) {
  std::string versionStr = *v;
  if (versionStr == "AC1009" || versionStr == "AC1006") {
    setVersion(DRW::Version::AC1009, dxfFormat);
  } else if (versionStr == "AC1012" || versionStr == "AC1014" || versionStr == "AC1015" || versionStr == "AC1018") {
    setVersion(DRW::Version::AC1015, dxfFormat);
  } else {
    setVersion(DRW::Version::AC1021, dxfFormat);
  }
}

/** @brief Sets the code page for the text codec based on the provided string and format.
 *  The function first corrects the code page string, then deletes any existing converter.
 *  Depending on the version and format, it initializes a new converter with the appropriate table or encoding.
 *  @param c Pointer to a string representing the code page to set
 *  @param dxfFormat Boolean indicating whether the format is DXF or not
 */
void DRW_TextCodec::setCodePage(std::string* c, bool dxfFormat) {
  cp = correctCodePage(*c);  // always canonical now

  delete conv;

  if (version == DRW::Version::AC1009 || version == DRW::Version::AC1015) {
    // TAS: Only one legacy table left
    conv = new DRW_ConvTable(DRW_Table1252, CPLENGHTCOMMON);
  } else {
    if (dxfFormat)
      conv = new DRW_Converter(nullptr, 0);  // UTF-16 → UTF-8
    else
      conv = new DRW_ConvUTF16();
  }
}

std::string DRW_TextCodec::toUtf8(std::string s) { return conv->toUtf8(&s); }

std::string DRW_TextCodec::fromUtf8(std::string s) { return conv->fromUtf8(&s); }

std::string DRW_Converter::toUtf8(std::string* s) {
  std::string result;
  int j = 0;
  unsigned int i = 0;
  for (i = 0; i < s->length(); i++) {
    unsigned char c = s->at(i);
    if (c < 0x80) {  //ascii check for /U+????
      if (c == '\\' && i + 6 < s->length() && s->at(i + 1) == 'U' && s->at(i + 2) == '+') {
        result += s->substr(j, i - j);
        result += encodeText(s->substr(i, 7));
        i += 6;
        j = i + 1;
      }
    } else if (c < 0xE0) {  //2 bits
      i++;
    } else if (c < 0xF0) {  //3 bits
      i += 2;
    } else if (c < 0xF8) {  //4 bits
      i += 3;
    }
  }
  result += s->substr(j);

  return result;
}

std::string DRW_ConvTable::fromUtf8(std::string* s) {
  std::string result;
  bool notFound;
  int code;

  int j = 0;
  for (unsigned int i = 0; i < s->length(); i++) {
    unsigned char c = s->at(i);
    if (c > 0x7F) {  //need to decode
      result += s->substr(j, i - j);
      std::string part1 = s->substr(i, 4);
      int l;
      code = decodeNum(part1, &l);
      j = i + l;
      i = j - 1;
      notFound = true;
      for (int k = 0; k < cpLenght; k++) {
        if (table[k] == code) {
          result += CPOFFSET + k;  //translate from table
          notFound = false;
          break;
        }
      }
      if (notFound) result += decodeText(code);
    }
  }
  result += s->substr(j);

  return result;
}

std::string DRW_ConvTable::toUtf8(std::string* s) {
  std::string res;
  std::string::iterator it;
  for (it = s->begin(); it < s->end(); ++it) {
    unsigned char c = *it;
    if (c < 0x80) {
      //check for \U+ encoded text
      if (c == '\\') {
        if (it + 6 < s->end() && *(it + 1) == 'U' && *(it + 2) == '+') {
          res += encodeText(std::string(it, it + 7));
          it += 6;
        } else {
          res += c;  //no \U+ encoded text write
        }
      } else
        res += c;                         //c!='\' ascii char write
    } else {                              //end c < 0x80
      res += encodeNum(table[c - 0x80]);  //translate from table
    }
  }  //end for

  return res;
}

std::string DRW_Converter::encodeText(std::string stmp) {
  int code;
  std::istringstream sd(stmp.substr(3, 4));
  sd >> std::hex >> code;
  return encodeNum(code);
}

std::string DRW_Converter::decodeText(int c) {
  std::string res = "\\U+";
  std::string num;
  std::stringstream ss;
  ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << c;
  ss >> num;
  res += num;
  return res;
}

std::string DRW_Converter::encodeNum(int c) {
  unsigned char ret[5]{};
  if (c < 128) {  // 0-7F US-ASCII 7 bits
    ret[0] = static_cast<unsigned char>(c);
    ret[1] = 0;
  } else if (c < 0x800) {  //80-07FF 2 bytes
    ret[0] = static_cast<unsigned char>(0xC0 | (c >> 6));
    ret[1] = 0x80 | (c & 0x3f);
    ret[2] = 0;
  } else if (c < 0x10000) {  //800-FFFF 3 bytes
    ret[0] = static_cast<unsigned char>(0xe0 | (c >> 12));
    ret[1] = 0x80 | ((c >> 6) & 0x3f);
    ret[2] = 0x80 | (c & 0x3f);
    ret[3] = 0;
  } else {  //10000-10FFFF 4 bytes
    ret[0] = static_cast<unsigned char>(0xf0 | (c >> 18));
    ret[1] = 0x80 | ((c >> 12) & 0x3f);
    ret[2] = 0x80 | ((c >> 6) & 0x3f);
    ret[3] = 0x80 | (c & 0x3f);
    ret[4] = 0;
  }
  return std::string((char*)ret);
}

/** 's' is a string with at least 4 bytes lenght
** returned 'b' is byte lenght of encoded char: 2,3 or 4
**/
int DRW_Converter::decodeNum(std::string s, int* b) {
  int code = 0;
  unsigned char c = s.at(0);
  if ((c & 0xE0) == 0xC0) {  //2 bytes
    code = (c & 0x1F) << 6;
    code = (s.at(1) & 0x3F) | code;
    *b = 2;
  } else if ((c & 0xF0) == 0xE0) {  //3 bytes
    code = (c & 0x0F) << 12;
    code = ((s.at(1) & 0x3F) << 6) | code;
    code = (s.at(2) & 0x3F) | code;
    *b = 3;
  } else if ((c & 0xF8) == 0xF0) {  //4 bytes
    code = (c & 0x07) << 18;
    code = ((s.at(1) & 0x3F) << 12) | code;
    code = ((s.at(2) & 0x3F) << 6) | code;
    code = (s.at(3) & 0x3F) | code;
    *b = 4;
  }

  return code;
}

std::string DRW_ConvDBCSTable::fromUtf8(std::string* s) {
  std::string result;
  bool notFound;
  int code;

  int j = 0;
  for (unsigned int i = 0; i < s->length(); i++) {
    unsigned char c = s->at(i);
    if (c > 0x7F) {  //need to decode
      result += s->substr(j, i - j);
      std::string part1 = s->substr(i, 4);
      int l;
      code = decodeNum(part1, &l);
      j = i + l;
      i = j - 1;
      notFound = true;
      for (int k = 0; k < cpLenght; k++) {
        if (doubleTable[k][1] == code) {
          int data = doubleTable[k][0];
          char d[3]{};
          d[0] = static_cast<char>(data >> 8);
          d[1] = data & 0xFF;
          d[2] = '\0';
          result += d;  //translate from table
          notFound = false;
          break;
        }
      }
      if (notFound) result += decodeText(code);
    }  //direct conversion
  }
  result += s->substr(j);

  return result;
}

std::string DRW_ConvDBCSTable::toUtf8(std::string* s) {
  std::string res;
  std::string::iterator it;
  for (it = s->begin(); it < s->end(); ++it) {
    bool notFound = true;
    unsigned char c = *it;
    if (c < 0x80) {
      notFound = false;
      //check for \U+ encoded text
      if (c == '\\') {
        if (it + 6 < s->end() && *(it + 1) == 'U' && *(it + 2) == '+') {
          res += encodeText(std::string(it, it + 7));
          it += 6;
        } else {
          res += c;  //no \U+ encoded text write
        }
      } else
        res += c;            //c!='\' ascii char write
    } else if (c == 0x80) {  //1 byte table
      notFound = false;
      res += encodeNum(0x20AC);  //euro sign
    } else {                     //2 bytes
      ++it;
      int code = (c << 8) | (unsigned char)(*it);
      int sta = leadTable[c - 0x81];
      int end = leadTable[c - 0x80];
      for (int k = sta; k < end; k++) {
        if (doubleTable[k][0] == code) {
          res += encodeNum(doubleTable[k][1]);  //translate from table
          notFound = false;
          break;
        }
      }
    }
    //not found
    if (notFound) { res += encodeNum(DBCS_REPLACEMENT_CHAR); }
  }
  return res;
}

std::string DRW_ConvUTF16::fromUtf8(std::string* s) {
  DRW_UNUSED(s);
  return std::string();
}

std::string DRW_ConvUTF16::toUtf8(std::string* s) {  //RLZ: pending to write
  std::string res;
  std::string::iterator it;
  for (it = s->begin(); it < s->end(); ++it) {
    unsigned char c1 = *it;
    unsigned char c2 = *(++it);
    std::uint16_t ch = (c2 << 8) | c1;
    res += encodeNum(ch);
  }  //end for

  return res;
}

std::string DRW_ExtConverter::convertByiconv(const char* in_encode, const char* out_encode, const std::string* s) {
  const int BUF_SIZE = 1000;
  static char in_buf[BUF_SIZE], out_buf[BUF_SIZE];

  char* in_ptr = in_buf;
  char* out_ptr = out_buf;
  strncpy(in_buf, s->c_str(), BUF_SIZE);

  iconv_t ic;
  ic = iconv_open(out_encode, in_encode);
  size_t il = BUF_SIZE - 1, ol = BUF_SIZE - 1;

  // TAS: Error C2664 fix: iconv expects 'const char **' for the second argument, but in_ptr is 'char *'.
  // Cast 'char**' to 'const char**' is not safe, so use a temporary 'const char*' pointer.
  // iconv(ic, (char**)&in_ptr, &il, &out_ptr, &ol);

  const char* temp_in_ptr = in_ptr;
  iconv(ic, &temp_in_ptr, &il, &out_ptr, &ol);

  iconv_close(ic);

  return std::string(out_buf);
}

std::string DRW_ExtConverter::fromUtf8(std::string* s) { return convertByiconv("UTF8", this->encoding, s); }

std::string DRW_ExtConverter::toUtf8(std::string* s) { return convertByiconv(this->encoding, "UTF8", s); }

std::string DRW_TextCodec::correctCodePage(const std::string& s) {
  std::string cp = s;
  std::transform(cp.begin(), cp.end(), cp.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

  // Western European (our only supported legacy codepage)
  if (cp == "ANSI_1252" || cp == "CP1252" || cp == "LATIN1" || cp == "ISO-8859-1" || cp == "CP819" ||
      cp == "ISO8859-1" || cp == "ISO8859-15" || cp == "ISO-IR-100" || cp == "L1" || cp == "IBM 850" ||
      cp == "APPLE ROMAN" || cp == "ISO_8859-1") {
    return "ANSI_1252";
  }

  // UTF paths (modern DXF)
  else if (cp == "UTF-8" || cp == "UTF8" || cp == "UTF8-BIT") {
    return "UTF-8";
  } else if (cp == "UTF-16" || cp == "UTF16" || cp == "UTF16-BIT") {
    return "UTF-16";
  }

  // Everything else (including all remaining European, Thai, Arabic, etc.)
  // maps to our safe Western default. This matches the Eastern pruning you already did.
  return "ANSI_1252";
}
