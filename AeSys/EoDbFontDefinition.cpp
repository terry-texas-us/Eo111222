#include "Stdafx.h"

#include "EoDb.h"
#include "EoDbFontDefinition.h"

namespace {
constexpr const wchar_t* horizontalAlignmentText[] = {L"Left", L"Center", L"Right"};
constexpr const wchar_t* pathText[] = {L"Right", L"Left", L"Up", L"Down"};
constexpr const wchar_t* precisionText[] = {L"True Type", L"Stroke"};
constexpr const wchar_t* verticalAlignmentText[] = {L"Top", L"Middle", L"Bottom"};
constexpr const wchar_t* invalidText = L"Invalid!";
}  // namespace

EoDbFontDefinition::EoDbFontDefinition(EoUInt16 precision, const CString& fontName, EoUInt16 path,
                                       EoUInt16 horizontalAlignment, EoUInt16 verticalAlignment,
                                       double characterSpacing)
    : m_fontName{fontName},
      m_characterSpacing{characterSpacing},
      m_precision{precision},
      m_path{path},
      m_horizontalAlignment{horizontalAlignment},
      m_verticalAlignment{verticalAlignment} {}

[[nodiscard]] CString EoDbFontDefinition::FormatHorizontalAlignment() const {
  return (m_horizontalAlignment >= EoDb::HorizontalAlignment::Left &&
          m_horizontalAlignment <= EoDb::HorizontalAlignment::Right)
             ? CString{horizontalAlignmentText[static_cast<int>(m_horizontalAlignment) - 1]}
             : CString{invalidText};
}

[[nodiscard]] CString EoDbFontDefinition::FormatPath() const {
  return (m_path >= EoDb::Path::Right && m_path <= EoDb::Path::Down)
             ? CString{pathText[static_cast<int>(m_path)]}
             : CString{L"Invalid!"};
}
[[nodiscard]] CString EoDbFontDefinition::FormatPrecision() const {
  return (m_precision >= EoDb::EoTrueType && m_precision <= EoDb::StrokeType) ? CString{precisionText[m_precision - 1]}
                                                                              : CString{invalidText};
}

[[nodiscard]] CString EoDbFontDefinition::FormatVerticalAlignment() const {
  return (m_verticalAlignment >= EoDb::AlignTop && m_verticalAlignment <= EoDb::AlignBottom)
             ? CString{verticalAlignmentText[m_verticalAlignment - 2]}
             : CString{invalidText};
}

void EoDbFontDefinition::Read(CFile& file) {
  EoDb::Read(file, m_precision);
  EoDb::Read(file, m_fontName);

  EoDb::Read(file, m_path);
  EoDb::Read(file, m_horizontalAlignment);
  
  EoDb::Read(file, m_verticalAlignment);
  EoDb::Read(file, m_characterSpacing);
}

void EoDbFontDefinition::Write(CFile& file) const {
  EoDb::Write(file, m_precision);
  EoDb::Write(file, m_fontName);
  EoDb::Write(file, static_cast<EoUInt16>(m_path));
  EoDb::Write(file, m_horizontalAlignment);
  EoDb::Write(file, m_verticalAlignment);
  EoDb::Write(file, m_characterSpacing);
}
