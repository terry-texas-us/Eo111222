#include "EoDxfCodePageTables.h"
#include "EoDxfTextCodec.h"

EoTcTextCodec::EoTcTextCodec() : m_codePage{"ANSI_1252"}, m_converter{nullptr}, m_version{} {}

EoTcTextCodec::~EoTcTextCodec() { delete m_converter; }

std::string EoTcTextCodec::FromUtf8(std::string s) {
  if (m_converter) { return m_converter->FromUtf8(&s); }
  return s;
}

std::string EoTcTextCodec::ToUtf8(std::string s) const {
  if (m_converter) { return m_converter->ToUtf8(&s); }
  return s;
}

void EoTcTextCodec::SetVersion(const std::string& version, bool dxfFormat) {
  m_version = 0;
  if (!version.empty()) {}
}

void EoTcTextCodec::SetVersion(int version, bool dxfFormat) { m_version = version; }

/** @brief Standardizes a DXF code page string to one of the standard internal forms.
 *
 * - Empty string → "ANSI_1252"
 * - "UTF-8" or "UTF8" → "UTF-8"
 * - "UTF-16" or "UTF16" → "UTF-16"
 * - Everything else (including all ANSI-1252 variants and unknown values) → "ANSI_1252"
 *
 * @param codePage Input code page identifier (from DXF $DWGCODEPAGE or similar).
 * @return Standardized code page string.
 */
std::string EoTcTextCodec::NormalizeCodePage(const std::string_view codePage) noexcept {
  if (codePage.empty()) { return "ANSI_1252"; }

  if (codePage == "UTF-8" || codePage == "UTF8") { return "UTF-8"; }
  if (codePage == "UTF-16" || codePage == "UTF16") { return "UTF-16"; }

  return "ANSI_1252";
}

void EoTcTextCodec::SetCodePage(const std::string& codePage, bool dxfFormat) {
  delete m_converter;
  m_converter = nullptr;

  std::string normalizedCodePage = NormalizeCodePage(codePage);
  m_codePage = normalizedCodePage;

  if (normalizedCodePage == "UTF-16") {
    m_converter = new EoTcConvertUtf16();
  } else if (normalizedCodePage == "UTF-8") {
    m_converter = new EoTcConvertTable(nullptr, 0);
  } else {
    m_converter = new EoTcConvertTable(DRW_Table1252, 256);
  }
}
