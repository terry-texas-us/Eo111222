#include "stdafx.h"

EoDbFontDefinition::EoDbFontDefinition() {
  Set(EoDb::kStrokeType, L"Simplex.psf", EoDb::kPathRight, EoDb::kAlignLeft, EoDb::kAlignBottom, 0.0);
}
EoDbFontDefinition::EoDbFontDefinition(EoUInt16 precision, const CString& fontName, EoUInt16 path,
                                       EoUInt16 horizontalAlignment, EoUInt16 verticalAlignment, double spacing) {
  Set(precision, fontName, path, horizontalAlignment, verticalAlignment, spacing);
}
EoDbFontDefinition::EoDbFontDefinition(const EoDbFontDefinition& fd) {
  m_Precision = fd.m_Precision;
  m_FontName = fd.m_FontName;
  m_Path = fd.m_Path;
  m_HorizontalAlignment = fd.m_HorizontalAlignment;
  m_VerticalAlignment = fd.m_VerticalAlignment;
  m_CharacterSpacing = fd.m_CharacterSpacing;
}
EoDbFontDefinition& EoDbFontDefinition::operator=(const EoDbFontDefinition& fd) {
  m_Precision = fd.m_Precision;
  m_FontName = fd.m_FontName;
  m_Path = fd.m_Path;
  m_HorizontalAlignment = fd.m_HorizontalAlignment;
  m_VerticalAlignment = fd.m_VerticalAlignment;
  m_CharacterSpacing = fd.m_CharacterSpacing;

  return (*this);
}
CString EoDbFontDefinition::FormatHorizonatlAlignment() {
  CString strAlign[] = {L"Left", L"Center", L"Right"};
  return (m_HorizontalAlignment >= EoDb::kAlignLeft && m_HorizontalAlignment <= EoDb::kAlignRight)
             ? strAlign[m_HorizontalAlignment - 1]
             : L"Invalid!";
}
CString EoDbFontDefinition::FormatPath() {
  CString strPath[] = {L"Right", L"Left", L"Up", L"Down"};
  return (m_Path >= EoDb::kPathRight && m_Path <= EoDb::kPathDown) ? strPath[m_Path] : L"Invalid!";
}
CString EoDbFontDefinition::FormatPrecision() {
  CString strPrec[] = {L"True Type", L"Stroke"};
  return (m_Precision >= EoDb::kEoTrueType && m_Precision <= EoDb::kStrokeType) ? strPrec[m_Precision - 1]
                                                                                : L"Invalid!";
}
CString EoDbFontDefinition::FormatVerticalAlignment() {
  CString strAlign[] = {L"Top", L"Middle", L"Bottom"};
  return (m_VerticalAlignment >= EoDb::kAlignTop && m_VerticalAlignment <= EoDb::kAlignBottom)
             ? strAlign[m_VerticalAlignment - 2]
             : L"Invalid!";
}
void EoDbFontDefinition::Read(CFile& file) {
  EoDb::Read(file, m_Precision);
  EoDb::Read(file, m_FontName);
  EoDb::Read(file, m_Path);
  EoDb::Read(file, m_HorizontalAlignment);
  EoDb::Read(file, m_VerticalAlignment);
  EoDb::Read(file, m_CharacterSpacing);
}
void EoDbFontDefinition::Set(EoUInt16 precision, const CString& fontName, EoUInt16 path, EoUInt16 horizontalAlignment,
                             EoUInt16 verticalAlignment, double spacing) {
  m_Precision = precision;
  m_FontName = fontName;
  m_Path = path;
  m_HorizontalAlignment = horizontalAlignment;
  m_VerticalAlignment = verticalAlignment;
  m_CharacterSpacing = spacing;
}
void EoDbFontDefinition::Write(CFile& file) {
  EoDb::Write(file, m_Precision);
  EoDb::Write(file, m_FontName);
  EoDb::Write(file, m_Path);
  EoDb::Write(file, m_HorizontalAlignment);
  EoDb::Write(file, m_VerticalAlignment);
  EoDb::Write(file, m_CharacterSpacing);
}
