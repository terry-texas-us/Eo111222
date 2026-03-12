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

[[nodiscard]] std::wstring Utf8ToWide(const std::string_view text) {
  if (text.empty()) { return {}; }

  const auto inputLength = static_cast<int>(text.size());
  const auto requiredLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), inputLength, nullptr, 0);
  if (requiredLength <= 0) { return {}; }

  std::wstring wideText(static_cast<std::size_t>(requiredLength), L'\0');
  const auto convertedLength =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), inputLength, wideText.data(), requiredLength);
  if (convertedLength != requiredLength) { return {}; }

  return wideText;
}

[[nodiscard]] std::string WideToUtf8(const std::wstring_view text) {
  if (text.empty()) { return {}; }

  const auto inputLength = static_cast<int>(text.size());
  const auto requiredLength = WideCharToMultiByte(CP_UTF8, 0, text.data(), inputLength, nullptr, 0, nullptr, nullptr);
  if (requiredLength <= 0) { return {}; }

  std::string utf8Text(static_cast<std::size_t>(requiredLength), '\0');
  const auto convertedLength =
      WideCharToMultiByte(CP_UTF8, 0, text.data(), inputLength, utf8Text.data(), requiredLength, nullptr, nullptr);
  if (convertedLength != requiredLength) { return {}; }

  return utf8Text;
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

  std::wstring wideText;
  wideText.reserve((text.size() - offset) / 2);
  for (auto index = offset; index + 1 < text.size(); index += 2) {
    const auto firstByte = static_cast<unsigned char>(text[index]);
    const auto secondByte = static_cast<unsigned char>(text[index + 1]);
    const auto codeUnit = littleEndian ? static_cast<std::uint16_t>(firstByte | (secondByte << 8))
                                       : static_cast<std::uint16_t>((firstByte << 8) | secondByte);
    wideText.push_back(static_cast<wchar_t>(codeUnit));
  }

  return wideText;
}

[[nodiscard]] std::string EncodeUtf16(const std::wstring_view text) {
  std::string utf16Text;
  utf16Text.reserve(text.size() * 2);

  for (const auto wideChar : text) {
    const auto codeUnit = static_cast<std::uint16_t>(wideChar);
    utf16Text.push_back(static_cast<char>(codeUnit & 0xFF));
    utf16Text.push_back(static_cast<char>((codeUnit >> 8) & 0xFF));
  }

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

  if (normalizedCodePage == L"UTF8") { return L"UTF-8"; }
  if (normalizedCodePage == L"UTF16" || normalizedCodePage == L"UTF16LE") { return L"UTF-16"; }
  if (normalizedCodePage == L"ANSI1252" || normalizedCodePage == L"CP1252" || normalizedCodePage == L"WINDOWS1252" ||
      normalizedCodePage == L"1252") {
    return L"ANSI_1252";
  }

  return L"ANSI_1252";
}

void EoTcTextCodec::SetCodePage(std::wstring_view codePage) {
  const std::wstring normalizedCodePage = NormalizeCodePage(codePage);
  m_converter = CreateConverter(normalizedCodePage);
  m_codePage = normalizedCodePage;
}

std::string EoTcConvertUtf8::EncodeText(std::wstring_view text) const {
  return WideToUtf8(text);
}

std::wstring EoTcConvertUtf8::DecodeText(std::string_view encodedText) const {
  return Utf8ToWide(encodedText);
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
