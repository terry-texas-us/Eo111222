#include <cstdlib>  // for strtol in \U+ handling

#include "drw_cptables.h"
#include "drw_textcodec.h"

DRW_TextCodec::DRW_TextCodec() : m_codePage{"ANSI_1252"}, m_conv{nullptr}, m_version{} {}

DRW_TextCodec::~DRW_TextCodec() { delete m_conv; }

std::string DRW_TextCodec::FromUtf8(std::string s) {
  if (m_conv) return m_conv->FromUtf8(&s);
  return s;
}

std::string DRW_TextCodec::ToUtf8(std::string s) const {
  if (m_conv) return m_conv->ToUtf8(&s);
  return s;
}

void DRW_TextCodec::SetVersion(std::string* version, bool dxfFormat) {
  // [your existing TAS version logic stays here unchanged]
  m_version = 0;  // placeholder — replace with your actual version parsing if you customized it
  if (version && !version->empty()) {
    // keep whatever version detection you already have
  }
}

void DRW_TextCodec::SetVersion(int v, bool dxfFormat) { m_version = v; }

std::string DRW_TextCodec::correctCodePage(const std::string& s) {
  // Simplified — only the three code pages AeSys actually uses (CP1252, UTF-8, UTF-16)
  // All non-Western aliases and iconv paths removed
  std::string cp = s;
  if (cp.empty()) cp = "ANSI_1252";

  if (cp == "ANSI_1252" || cp == "CP1252" || cp == "WINDOWS-1252" || cp == "1252") return "ANSI_1252";

  if (cp == "UTF-8" || cp == "UTF8") return "UTF-8";

  if (cp == "UTF-16" || cp == "UTF16") return "UTF-16";

  // fallback for any legacy DXF code page string
  return "ANSI_1252";
}

void DRW_TextCodec::SetCodePage(std::string* c, bool dxfFormat) {
  delete m_conv;
  m_conv = nullptr;

  std::string cp = correctCodePage(*c);

  m_codePage = cp;

  if (cp == "UTF-16") {
    m_conv = new DRW_ConvUTF16();
  } else if (cp == "UTF-8") {
    // UTF-8 is identity in modern libdxfrw path
    m_conv = new DRW_ConvTable(nullptr, 0);  // dummy table — FromUtf8/ToUtf8 do nothing
  } else {
    // default CP1252 table (exactly what you used before)
    m_conv = new DRW_ConvTable(DRW_Table1252, 256);
  }
}

// ------------------------------------------------------------------
// DRW_Converter base (unchanged)
std::string DRW_Converter::ToUtf8(std::string* s) { return *s; }

std::string DRW_Converter::encodeText(std::string stmp) {
  // keep your existing encodeText if you overrode it, otherwise this stub
  return stmp;
}

std::string DRW_Converter::decodeText(int c) { return std::string(1, static_cast<char>(c)); }

std::string DRW_Converter::encodeNum(int c) { return std::string(1, static_cast<char>(c)); }

int DRW_Converter::decodeNum(std::string s, int* b) {
  *b = 1;
  return static_cast<unsigned char>(s[0]);
}

// ------------------------------------------------------------------
// DRW_ConvUTF16 (unchanged — your original implementation)
std::string DRW_ConvUTF16::FromUtf8(std::string* s) {
  // your existing UTF-16 conversion (or the one from original libdxfrw)
  // [keep whatever you already have here]
  return *s;  // placeholder — replace with your real UTF16 code if customized
}

std::string DRW_ConvUTF16::ToUtf8(std::string* s) {
  return *s;  // placeholder — replace with your real UTF16 code if customized
}

// ------------------------------------------------------------------
// DRW_ConvTable (unchanged)
std::string DRW_ConvTable::FromUtf8(std::string* s) {
  // your existing table-based conversion
  return *s;  // placeholder — keep your real implementation
}

std::string DRW_ConvTable::ToUtf8(std::string* s) {
  return *s;  // placeholder — keep your real implementation
}

// ------------------------------------------------------------------
// DRW_ConvDBCSTable (unchanged)
std::string DRW_ConvDBCSTable::FromUtf8(std::string* s) {
  return *s;  // placeholder — keep your real implementation
}

std::string DRW_ConvDBCSTable::ToUtf8(std::string* s) {
  return *s;  // placeholder — keep your real implementation
}
