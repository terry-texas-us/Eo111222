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

CString EoDbFontDefinition::FormatPrecision() const {
  return (m_precision >= EoDb::Precision::TrueType && m_precision <= EoDb::Precision::StrokeType)
             ? CString{precisionText[static_cast<int>(m_precision) - static_cast<int>(EoDb::Precision::TrueType)]}
             : CString{invalidText};
}

CString EoDbFontDefinition::FormatPath() const {
  return (m_path >= EoDb::Path::Right && m_path <= EoDb::Path::Down) ? CString{pathText[static_cast<int>(m_path)]}
                                                                     : CString{L"Invalid!"};
}

CString EoDbFontDefinition::FormatHorizontalAlignment() const {
  return (m_horizontalAlignment >= EoDb::HorizontalAlignment::Left &&
             m_horizontalAlignment <= EoDb::HorizontalAlignment::Right)
             ? CString{horizontalAlignmentText[static_cast<int>(m_horizontalAlignment) -
                                               static_cast<int>(EoDb::HorizontalAlignment::Left)]}
             : CString{invalidText};
}

CString EoDbFontDefinition::FormatVerticalAlignment() const {
  return (m_verticalAlignment >= EoDb::VerticalAlignment::Top && m_verticalAlignment <= EoDb::VerticalAlignment::Bottom)
             ? CString{verticalAlignmentText[static_cast<int>(m_verticalAlignment) -
                                             static_cast<int>(EoDb::VerticalAlignment::Top)]}
             : CString{invalidText};
}

void EoDbFontDefinition::SetAlignment(
    EoDb::HorizontalAlignment horizontalAlignment, EoDb::VerticalAlignment verticalAlignment) {
  m_horizontalAlignment = horizontalAlignment;
  m_verticalAlignment = verticalAlignment;
}
void EoDbFontDefinition::SetCharacterSpacing(double characterSpacing) { m_characterSpacing = characterSpacing; }
void EoDbFontDefinition::SetFontName(const CString& fontName) { m_fontName = fontName; }
void EoDbFontDefinition::SetHorizontalAlignment(EoDb::HorizontalAlignment horizontalAlignment) {
  m_horizontalAlignment = horizontalAlignment;
}
void EoDbFontDefinition::SetPath(EoDb::Path path) { m_path = path; }
void EoDbFontDefinition::SetPrecision(EoDb::Precision precision) { m_precision = precision; }
void EoDbFontDefinition::SetVerticalAlignment(EoDb::VerticalAlignment verticalAlignment) {
  m_verticalAlignment = verticalAlignment;
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
  EoDb::Write(file, m_path);
  EoDb::Write(file, m_horizontalAlignment);
  EoDb::Write(file, m_verticalAlignment);
  EoDb::Write(file, m_characterSpacing);
}
