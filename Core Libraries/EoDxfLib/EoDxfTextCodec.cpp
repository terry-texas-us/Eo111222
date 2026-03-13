#include "EoDxfCodePageTables.h"
#include "EoDxfTextCodec.h"

#include <Windows.h>

#include <cctype>
#include <cwctype>
#include <memory>
#include <string_view>

#include "EoDxfBase.h"

namespace {

constexpr int cp1252TableLength{128};
constexpr DWORD strictUtf8DecodeFlags{MB_ERR_INVALID_CHARS};
constexpr DWORD strictWindowsCodePageEncodeFlags{WC_NO_BEST_FIT_CHARS};

struct WindowsCodePageMapping {
  std::wstring_view normalizedToken;
  std::wstring_view canonicalToken;
  unsigned int codePage;
};

constexpr WindowsCodePageMapping windowsCodePageMappings[] = {
    {L"ANSI874", L"ANSI_874", 874},
    {L"CP874", L"ANSI_874", 874},
    {L"WINDOWS874", L"ANSI_874", 874},
    {L"874", L"ANSI_874", 874},
    {L"ANSI932", L"ANSI_932", 932},
    {L"CP932", L"ANSI_932", 932},
    {L"WINDOWS932", L"ANSI_932", 932},
    {L"SHIFTJIS", L"ANSI_932", 932},
    {L"SHIFTJISX0213", L"ANSI_932", 932},
    {L"WINDOWS31J", L"ANSI_932", 932},
    {L"MSKANJI", L"ANSI_932", 932},
    {L"932", L"ANSI_932", 932},
    {L"ANSI936", L"ANSI_936", 936},
    {L"CP936", L"ANSI_936", 936},
    {L"WINDOWS936", L"ANSI_936", 936},
    {L"GBK", L"ANSI_936", 936},
    {L"GB2312", L"ANSI_936", 936},
    {L"936", L"ANSI_936", 936},
    {L"ANSI949", L"ANSI_949", 949},
    {L"CP949", L"ANSI_949", 949},
    {L"WINDOWS949", L"ANSI_949", 949},
    {L"UHC", L"ANSI_949", 949},
    {L"949", L"ANSI_949", 949},
    {L"ANSI950", L"ANSI_950", 950},
    {L"CP950", L"ANSI_950", 950},
    {L"WINDOWS950", L"ANSI_950", 950},
    {L"BIG5", L"ANSI_950", 950},
    {L"950", L"ANSI_950", 950},
    {L"ANSI1250", L"ANSI_1250", 1250},
    {L"CP1250", L"ANSI_1250", 1250},
    {L"WINDOWS1250", L"ANSI_1250", 1250},
    {L"1250", L"ANSI_1250", 1250},
    {L"ANSI1251", L"ANSI_1251", 1251},
    {L"CP1251", L"ANSI_1251", 1251},
    {L"WINDOWS1251", L"ANSI_1251", 1251},
    {L"1251", L"ANSI_1251", 1251},
    {L"ANSI1252", L"ANSI_1252", 1252},
    {L"CP1252", L"ANSI_1252", 1252},
    {L"WINDOWS1252", L"ANSI_1252", 1252},
    {L"LATIN1", L"ANSI_1252", 1252},
    {L"ISO88591", L"ANSI_1252", 1252},
    {L"1252", L"ANSI_1252", 1252},
    {L"ANSI1253", L"ANSI_1253", 1253},
    {L"CP1253", L"ANSI_1253", 1253},
    {L"WINDOWS1253", L"ANSI_1253", 1253},
    {L"1253", L"ANSI_1253", 1253},
    {L"ANSI1254", L"ANSI_1254", 1254},
    {L"CP1254", L"ANSI_1254", 1254},
    {L"WINDOWS1254", L"ANSI_1254", 1254},
    {L"1254", L"ANSI_1254", 1254},
    {L"ANSI1255", L"ANSI_1255", 1255},
    {L"CP1255", L"ANSI_1255", 1255},
    {L"WINDOWS1255", L"ANSI_1255", 1255},
    {L"1255", L"ANSI_1255", 1255},
    {L"ANSI1256", L"ANSI_1256", 1256},
    {L"CP1256", L"ANSI_1256", 1256},
    {L"WINDOWS1256", L"ANSI_1256", 1256},
    {L"1256", L"ANSI_1256", 1256},
    {L"ANSI1257", L"ANSI_1257", 1257},
    {L"CP1257", L"ANSI_1257", 1257},
    {L"WINDOWS1257", L"ANSI_1257", 1257},
    {L"1257", L"ANSI_1257", 1257},
    {L"ANSI1258", L"ANSI_1258", 1258},
    {L"CP1258", L"ANSI_1258", 1258},
    {L"WINDOWS1258", L"ANSI_1258", 1258},
    {L"1258", L"ANSI_1258", 1258},
    {L"DOS437", L"DOS437", 437},
    {L"CP437", L"DOS437", 437},
    {L"IBM437", L"DOS437", 437},
    {L"OEM437", L"DOS437", 437},
    {L"DOS720", L"DOS720", 720},
    {L"CP720", L"DOS720", 720},
    {L"IBM720", L"DOS720", 720},
    {L"DOS737", L"DOS737", 737},
    {L"CP737", L"DOS737", 737},
    {L"IBM737", L"DOS737", 737},
    {L"DOS775", L"DOS775", 775},
    {L"CP775", L"DOS775", 775},
    {L"IBM775", L"DOS775", 775},
    {L"DOS850", L"DOS850", 850},
    {L"CP850", L"DOS850", 850},
    {L"IBM850", L"DOS850", 850},
    {L"DOS852", L"DOS852", 852},
    {L"CP852", L"DOS852", 852},
    {L"IBM852", L"DOS852", 852},
    {L"DOS855", L"DOS855", 855},
    {L"CP855", L"DOS855", 855},
    {L"IBM855", L"DOS855", 855},
    {L"DOS857", L"DOS857", 857},
    {L"CP857", L"DOS857", 857},
    {L"IBM857", L"DOS857", 857},
    {L"DOS858", L"DOS858", 858},
    {L"CP858", L"DOS858", 858},
    {L"IBM858", L"DOS858", 858},
    {L"DOS860", L"DOS860", 860},
    {L"CP860", L"DOS860", 860},
    {L"IBM860", L"DOS860", 860},
    {L"DOS861", L"DOS861", 861},
    {L"CP861", L"DOS861", 861},
    {L"IBM861", L"DOS861", 861},
    {L"DOS862", L"DOS862", 862},
    {L"CP862", L"DOS862", 862},
    {L"IBM862", L"DOS862", 862},
    {L"DOS863", L"DOS863", 863},
    {L"CP863", L"DOS863", 863},
    {L"IBM863", L"DOS863", 863},
    {L"DOS864", L"DOS864", 864},
    {L"CP864", L"DOS864", 864},
    {L"IBM864", L"DOS864", 864},
    {L"DOS865", L"DOS865", 865},
    {L"CP865", L"DOS865", 865},
    {L"IBM865", L"DOS865", 865},
    {L"DOS866", L"DOS866", 866},
    {L"CP866", L"DOS866", 866},
    {L"IBM866", L"DOS866", 866},
    {L"DOS869", L"DOS869", 869},
    {L"CP869", L"DOS869", 869},
    {L"IBM869", L"DOS869", 869},
};

[[nodiscard]] bool TryGetWindowsCodePageMapping(
    std::wstring_view normalizedToken, std::wstring* canonicalToken, unsigned int* codePage) {
  for (const auto& mapping : windowsCodePageMappings) {
    if (mapping.normalizedToken == normalizedToken) {
      if (canonicalToken != nullptr) { *canonicalToken = mapping.canonicalToken; }
      if (codePage != nullptr) { *codePage = mapping.codePage; }
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::wstring DecodeMultiByteText(const std::string_view text, const unsigned int codePage, const DWORD flags) {
  if (text.empty()) { return {}; }

  const auto inputLength = static_cast<int>(text.size());
  const auto requiredLength = MultiByteToWideChar(codePage, flags, text.data(), inputLength, nullptr, 0);
  if (requiredLength <= 0) { return {}; }

  std::wstring wideText(static_cast<std::size_t>(requiredLength), L'\0');
  const auto convertedLength = MultiByteToWideChar(codePage, flags, text.data(), inputLength, wideText.data(), requiredLength);
  if (convertedLength != requiredLength) { return {}; }

  return wideText;
}

[[nodiscard]] std::string EncodeWideText(
    const std::wstring_view text, const unsigned int codePage, const DWORD flags, const bool failOnDefaultCharacter = false) {
  if (text.empty()) { return {}; }

  const auto inputLength = static_cast<int>(text.size());
  BOOL usedDefaultCharacter = FALSE;
  auto* usedDefaultCharacterPointer = failOnDefaultCharacter ? &usedDefaultCharacter : nullptr;
  const auto requiredLength = WideCharToMultiByte(
      codePage, flags, text.data(), inputLength, nullptr, 0, nullptr, usedDefaultCharacterPointer);
  if (requiredLength <= 0) { return {}; }
  if (failOnDefaultCharacter && usedDefaultCharacter != FALSE) { return {}; }

  std::string encodedText(static_cast<std::size_t>(requiredLength), '\0');
  usedDefaultCharacter = FALSE;
  const auto convertedLength = WideCharToMultiByte(
      codePage, flags, text.data(), inputLength, encodedText.data(), requiredLength, nullptr, usedDefaultCharacterPointer);
  if (convertedLength != requiredLength) { return {}; }
  if (failOnDefaultCharacter && usedDefaultCharacter != FALSE) { return {}; }
  if (failOnDefaultCharacter && DecodeMultiByteText(encodedText, codePage, 0) != text) { return {}; }

  return encodedText;
}

[[nodiscard]] bool IsHighSurrogate(const std::uint16_t codeUnit) noexcept {
  return codeUnit >= 0xD800 && codeUnit <= 0xDBFF;
}

[[nodiscard]] bool IsLowSurrogate(const std::uint16_t codeUnit) noexcept {
  return codeUnit >= 0xDC00 && codeUnit <= 0xDFFF;
}

[[nodiscard]] std::wstring DecodeUtf8(const std::string_view text) {
  return DecodeMultiByteText(text, CP_UTF8, strictUtf8DecodeFlags);
}

[[nodiscard]] std::string EncodeUtf8(const std::wstring_view text) {
  return EncodeWideText(text, CP_UTF8, 0);
}

[[nodiscard]] std::wstring DecodeWindowsCodePage(const std::string_view text, const unsigned int codePage) {
  return DecodeMultiByteText(text, codePage, 0);
}

[[nodiscard]] std::string EncodeWindowsCodePage(const std::wstring_view text, const unsigned int codePage) {
  return EncodeWideText(text, codePage, strictWindowsCodePageEncodeFlags, true);
}

[[nodiscard]] std::wstring DecodeAnsiTable(const std::string_view text, const int* table, const int length) {
  std::wstring wideText;
  wideText.reserve(text.size());

  for (const auto byteValue : text) {
    const auto unsignedByte = static_cast<unsigned char>(byteValue);
    if (unsignedByte < 0x80 || unsignedByte >= 0xA0) {
      wideText.push_back(static_cast<wchar_t>(unsignedByte));
      continue;
    }

    const auto tableIndex = static_cast<int>(unsignedByte - 0x80);
    if (table != nullptr && tableIndex >= 0 && tableIndex < length && table[tableIndex] != 0) {
      wideText.push_back(static_cast<wchar_t>(table[tableIndex]));
    } else {
      wideText.push_back(static_cast<wchar_t>(unsignedByte));
    }
  }

  return wideText;
}

[[nodiscard]] std::string EncodeAnsiTable(const std::wstring_view text, const int* table, const int length) {
  std::string encodedText;
  encodedText.reserve(text.size());

  for (const auto wideChar : text) {
    const auto codeUnit = static_cast<unsigned int>(wideChar);
    if (codeUnit < 0x80 || (codeUnit >= 0xA0 && codeUnit <= 0xFF)) {
      encodedText.push_back(static_cast<char>(codeUnit));
      continue;
    }

    bool mapped{};
    if (codeUnit >= 0x80 && codeUnit <= 0x9F) {
      const auto tableIndex = static_cast<int>(codeUnit - 0x80);
      if (table != nullptr && tableIndex >= 0 && tableIndex < length && table[tableIndex] == 0) {
        encodedText.push_back(static_cast<char>(codeUnit));
        mapped = true;
      }
    }

    if (!mapped && table != nullptr) {
      for (auto tableIndex = 0; tableIndex < length; ++tableIndex) {
        if (table[tableIndex] == static_cast<int>(codeUnit)) {
          encodedText.push_back(static_cast<char>(tableIndex + 0x80));
          mapped = true;
          break;
        }
      }
    }

    if (!mapped) { encodedText.push_back('?'); }
  }

  return encodedText;
}

[[nodiscard]] std::wstring DecodeUtf16(const std::string_view text) {
  if (text.empty()) { return {}; }

  bool littleEndian{true};
  std::size_t offset{};
  if (text.size() >= 2) {
    const auto firstByte = static_cast<unsigned char>(text[0]);
    const auto secondByte = static_cast<unsigned char>(text[1]);
    if (firstByte == 0xFF && secondByte == 0xFE) {
      offset = 2;
    } else if (firstByte == 0xFE && secondByte == 0xFF) {
      littleEndian = false;
      offset = 2;
    }
  }

  if (((text.size() - offset) % 2) != 0) { return {}; }

  std::wstring wideText;
  wideText.reserve((text.size() - offset) / 2);
  bool expectLowSurrogate{};
  for (auto index = offset; index + 1 < text.size(); index += 2) {
    const auto firstByte = static_cast<unsigned char>(text[index]);
    const auto secondByte = static_cast<unsigned char>(text[index + 1]);
    const auto codeUnit = littleEndian ? static_cast<std::uint16_t>(firstByte | (secondByte << 8))
                                       : static_cast<std::uint16_t>((firstByte << 8) | secondByte);

    if (expectLowSurrogate) {
      if (!IsLowSurrogate(codeUnit)) { return {}; }
      expectLowSurrogate = false;
    } else if (IsLowSurrogate(codeUnit)) {
      return {};
    } else if (IsHighSurrogate(codeUnit)) {
      expectLowSurrogate = true;
    }

    wideText.push_back(static_cast<wchar_t>(codeUnit));
  }

  if (expectLowSurrogate) { return {}; }

  return wideText;
}

[[nodiscard]] std::string EncodeUtf16(const std::wstring_view text) {
  std::string utf16Text;
  utf16Text.reserve(text.size() * 2);

  bool expectLowSurrogate{};
  for (const auto wideChar : text) {
    const auto codeUnit = static_cast<std::uint16_t>(wideChar);

    if (expectLowSurrogate) {
      if (!IsLowSurrogate(codeUnit)) { return {}; }
      expectLowSurrogate = false;
    } else if (IsLowSurrogate(codeUnit)) {
      return {};
    } else if (IsHighSurrogate(codeUnit)) {
      expectLowSurrogate = true;
    }

    utf16Text.push_back(static_cast<char>(codeUnit & 0xFF));
    utf16Text.push_back(static_cast<char>((codeUnit >> 8) & 0xFF));
  }

  if (expectLowSurrogate) { return {}; }

  return utf16Text;
}

[[nodiscard]] std::wstring NormalizeTokenImpl(const std::wstring_view value) {
  std::wstring normalizedValue;
  normalizedValue.reserve(value.size());

  for (const auto character : value) {
    if (std::iswalnum(character) != 0) {
      normalizedValue.push_back(static_cast<wchar_t>(std::towupper(character)));
    }
  }

  return normalizedValue;
}

}  // namespace

EoTcTextCodec::EoTcTextCodec()
    : m_codePage{L"ANSI_1252"},
      m_converter{CreateConverter(L"ANSI_1252")},
      m_version{static_cast<int>(EoDxf::Version::UNKNOWN)} {}

EoTcTextCodec::~EoTcTextCodec() = default;

EoTcTextCodec::EoTcTextCodec(const EoTcTextCodec& other)
    : m_codePage{other.m_codePage}, m_converter{CreateConverter(other.m_codePage)}, m_version{other.m_version} {}

EoTcTextCodec& EoTcTextCodec::operator=(const EoTcTextCodec& other) {
  if (this != &other) {
    m_codePage = other.m_codePage;
    m_converter = CreateConverter(other.m_codePage);
    m_version = other.m_version;
  }
  return *this;
}

EoTcTextCodec::EoTcTextCodec(EoTcTextCodec&& other) noexcept = default;

EoTcTextCodec& EoTcTextCodec::operator=(EoTcTextCodec&& other) noexcept = default;

std::string EoTcTextCodec::EncodeText(std::wstring_view text) const {
  if (m_converter) { return m_converter->EncodeText(text); }
  return {};
}

std::wstring EoTcTextCodec::DecodeText(std::string_view encodedText) const {
  if (m_converter) { return m_converter->DecodeText(encodedText); }
  return {};
}

std::unique_ptr<EoTcConverter> EoTcTextCodec::CreateConverter(std::wstring_view normalizedCodePage) {
  if (normalizedCodePage == L"UTF-16") { return std::make_unique<EoTcConvertUtf16>(); }
  if (normalizedCodePage == L"UTF-8") { return std::make_unique<EoTcConvertUtf8>(); }
  if (normalizedCodePage == L"ANSI_1252") { return std::make_unique<EoTcConvertTable>(EoTcTable1252, cp1252TableLength); }

  unsigned int windowsCodePage{};
  if (TryGetWindowsCodePageMapping(normalizedCodePage, nullptr, &windowsCodePage)) {
    return std::make_unique<EoTcConvertWindowsCodePage>(windowsCodePage);
  }

  return std::make_unique<EoTcConvertTable>(EoTcTable1252, cp1252TableLength);
}

std::wstring EoTcTextCodec::GetVersion() const {
  switch (m_version) {
    case EoDxf::Version::AC1006:
      return L"AC1006";
    case EoDxf::Version::AC1009:
      return L"AC1009";
    case EoDxf::Version::AC1012:
      return L"AC1012";
    case EoDxf::Version::AC1014:
      return L"AC1014";
    case EoDxf::Version::AC1015:
      return L"AC1015";
    case EoDxf::Version::AC1018:
      return L"AC1018";
    case EoDxf::Version::AC1021:
      return L"AC1021";
    case EoDxf::Version::AC1024:
      return L"AC1024";
    case EoDxf::Version::AC1027:
      return L"AC1027";
    case EoDxf::Version::AC1032:
      return L"AC1032";
    default:
      return {};
  }
}

void EoTcTextCodec::SetVersion(std::wstring_view version) {
  const auto normalizedVersion = NormalizeToken(version);
  if (normalizedVersion == L"AC1006") {
    m_version = EoDxf::Version::AC1006;
  } else if (normalizedVersion == L"AC1009") {
    m_version = EoDxf::Version::AC1009;
  } else if (normalizedVersion == L"AC1012") {
    m_version = EoDxf::Version::AC1012;
  } else if (normalizedVersion == L"AC1014") {
    m_version = EoDxf::Version::AC1014;
  } else if (normalizedVersion == L"AC1015") {
    m_version = EoDxf::Version::AC1015;
  } else if (normalizedVersion == L"AC1018") {
    m_version = EoDxf::Version::AC1018;
  } else if (normalizedVersion == L"AC1021") {
    m_version = EoDxf::Version::AC1021;
  } else if (normalizedVersion == L"AC1024") {
    m_version = EoDxf::Version::AC1024;
  } else if (normalizedVersion == L"AC1027") {
    m_version = EoDxf::Version::AC1027;
  } else if (normalizedVersion == L"AC1032") {
    m_version = EoDxf::Version::AC1032;
  } else if (normalizedVersion.empty()) {
    m_version = EoDxf::Version::UNKNOWN;
  } else {
    m_version = EoDxf::Version::AC1021;
  }
}

void EoTcTextCodec::SetVersion(EoDxf::Version version) noexcept { m_version = version; }

std::wstring EoTcTextCodec::NormalizeToken(std::wstring_view value) { return NormalizeTokenImpl(value); }

std::wstring EoTcTextCodec::NormalizeCodePage(std::wstring_view codePage) {
  const auto normalizedCodePage = NormalizeToken(codePage);
  if (normalizedCodePage.empty()) { return L"ANSI_1252"; }

  if (normalizedCodePage == L"UTF8" || normalizedCodePage == L"UTF8NOBOM") { return L"UTF-8"; }
  if (normalizedCodePage == L"UTF16" || normalizedCodePage == L"UTF16LE" || normalizedCodePage == L"UNICODE") {
    return L"UTF-16";
  }

  std::wstring canonicalCodePage;
  if (TryGetWindowsCodePageMapping(normalizedCodePage, &canonicalCodePage, nullptr)) { return canonicalCodePage; }

  return L"ANSI_1252";
}

void EoTcTextCodec::SetCodePage(std::wstring_view codePage) {
  const std::wstring normalizedCodePage = NormalizeCodePage(codePage);
  m_converter = CreateConverter(normalizedCodePage);
  m_codePage = normalizedCodePage;
}

std::string EoTcConvertUtf8::EncodeText(std::wstring_view text) const {
  return EncodeUtf8(text);
}

std::wstring EoTcConvertUtf8::DecodeText(std::string_view encodedText) const {
  return DecodeUtf8(encodedText);
}

std::string EoTcConvertUtf16::EncodeText(std::wstring_view text) const {
  return EncodeUtf16(text);
}

std::wstring EoTcConvertUtf16::DecodeText(std::string_view encodedText) const {
  return DecodeUtf16(encodedText);
}

std::string EoTcConvertTable::EncodeText(std::wstring_view text) const {
  return EncodeAnsiTable(text, m_table, m_codePageLength);
}

std::wstring EoTcConvertTable::DecodeText(std::string_view encodedText) const {
  return DecodeAnsiTable(encodedText, m_table, m_codePageLength);
}

std::string EoTcConvertDBCSTable::EncodeText(std::wstring_view text) const {
  return EncodeAnsiTable(text, m_table, m_codePageLength);
}

std::wstring EoTcConvertDBCSTable::DecodeText(std::string_view encodedText) const {
  return DecodeAnsiTable(encodedText, m_table, m_codePageLength);
}

std::string EoTcConvertWindowsCodePage::EncodeText(std::wstring_view text) const {
  return EncodeWindowsCodePage(text, m_codePage);
}

std::wstring EoTcConvertWindowsCodePage::DecodeText(std::string_view encodedText) const {
  return DecodeWindowsCodePage(encodedText, m_codePage);
}
