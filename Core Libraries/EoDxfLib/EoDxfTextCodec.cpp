#include "EoDxfCodePageTables.h"
#include "EoDxfTextCodec.h"

#include <Windows.h>

#include <cctype>
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

[[nodiscard]] std::string NormalizeToken(const std::string_view value) {
  std::string normalizedValue;
  normalizedValue.reserve(value.size());

  for (const auto character : value) {
    const auto unsignedCharacter = static_cast<unsigned char>(character);
    if (std::isalnum(unsignedCharacter) != 0) {
      normalizedValue.push_back(static_cast<char>(std::toupper(unsignedCharacter)));
    }
  }

  return normalizedValue;
}

}  // namespace

EoTcTextCodec::EoTcTextCodec()
    : m_codePage{"ANSI_1252"}, m_converter{new EoTcConvertTable(EoTcTable1252, cp1252TableLength)},
      m_version{static_cast<int>(EoDxf::Version::UNKNOWN)} {}

EoTcTextCodec::~EoTcTextCodec() { delete m_converter; }

std::string EoTcTextCodec::FromUtf8(std::string s) {
  if (m_converter) { return m_converter->FromUtf8(&s); }
  return s;
}

std::string EoTcTextCodec::ToUtf8(std::string s) const {
  if (m_converter) { return m_converter->ToUtf8(&s); }
  return s;
}

void EoTcTextCodec::SetVersion(const std::string& version) {
  const auto normalizedVersion = NormalizeToken(version);
  if (normalizedVersion == "AC1006") {
    m_version = static_cast<int>(EoDxf::Version::AC1006);
  } else if (normalizedVersion == "AC1009") {
    m_version = static_cast<int>(EoDxf::Version::AC1009);
  } else if (normalizedVersion == "AC1012") {
    m_version = static_cast<int>(EoDxf::Version::AC1012);
  } else if (normalizedVersion == "AC1014") {
    m_version = static_cast<int>(EoDxf::Version::AC1014);
  } else if (normalizedVersion == "AC1015") {
    m_version = static_cast<int>(EoDxf::Version::AC1015);
  } else if (normalizedVersion == "AC1018") {
    m_version = static_cast<int>(EoDxf::Version::AC1018);
  } else if (normalizedVersion == "AC1021") {
    m_version = static_cast<int>(EoDxf::Version::AC1021);
  } else if (normalizedVersion == "AC1024") {
    m_version = static_cast<int>(EoDxf::Version::AC1024);
  } else if (normalizedVersion == "AC1027") {
    m_version = static_cast<int>(EoDxf::Version::AC1027);
  } else if (normalizedVersion == "AC1032") {
    m_version = static_cast<int>(EoDxf::Version::AC1032);
  } else if (normalizedVersion.empty()) {
    m_version = static_cast<int>(EoDxf::Version::UNKNOWN);
  } else {
    m_version = static_cast<int>(EoDxf::Version::AC1021);
  }
}

void EoTcTextCodec::SetVersion(int version) { m_version = version; }

std::string EoTcTextCodec::NormalizeCodePage(const std::string_view codePage) noexcept {
  const auto normalizedCodePage = NormalizeToken(codePage);
  if (normalizedCodePage.empty()) { return "ANSI_1252"; }

  if (normalizedCodePage == "UTF8") { return "UTF-8"; }
  if (normalizedCodePage == "UTF16" || normalizedCodePage == "UTF16LE") { return "UTF-16"; }
  if (normalizedCodePage == "ANSI1252" || normalizedCodePage == "CP1252" || normalizedCodePage == "WINDOWS1252" ||
      normalizedCodePage == "1252") {
    return "ANSI_1252";
  }

  return "ANSI_1252";
}

void EoTcTextCodec::SetCodePage(const std::string& codePage) {
  delete m_converter;
  m_converter = nullptr;

  std::string normalizedCodePage = NormalizeCodePage(codePage);
  m_codePage = normalizedCodePage;

  if (normalizedCodePage == "UTF-16") {
    m_converter = new EoTcConvertUtf16();
  } else if (normalizedCodePage == "UTF-8") {
    m_converter = new EoTcConvertUtf8();
  } else {
    m_converter = new EoTcConvertTable(EoTcTable1252, cp1252TableLength);
  }
}

std::string EoTcConvertUtf16::FromUtf8(std::string* s) {
  return EncodeUtf16(Utf8ToWide(*s));
}

std::string EoTcConvertUtf16::ToUtf8(std::string* s) {
  return WideToUtf8(DecodeUtf16(*s));
}

std::string EoTcConvertTable::FromUtf8(std::string* s) {
  return EncodeAnsiTable(Utf8ToWide(*s), m_table, m_codePageLength);
}

std::string EoTcConvertTable::ToUtf8(std::string* s) {
  return WideToUtf8(DecodeAnsiTable(*s, m_table, m_codePageLength));
}
