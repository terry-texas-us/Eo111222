#pragma once

/** @class EoDbFontDefinition represents the font definition of a text primitive, including the font name, character spacing, precision, path, and horizontal and vertical alignment. */
class EoDbFontDefinition {
  CString m_fontName;
  double m_characterSpacing;
  EoUInt16 m_precision;
  EoUInt16 m_path;
  EoUInt16 m_horizontalAlignment;
  EoUInt16 m_verticalAlignment;

 public:
  EoDbFontDefinition();

  EoDbFontDefinition(EoUInt16 precision, const CString& fontName, EoUInt16 path, EoUInt16 horizontalAlignment,
                     EoUInt16 verticalAlignment, double characterSpacing);

  EoDbFontDefinition(const EoDbFontDefinition& other) = default;
  EoDbFontDefinition& operator=(const EoDbFontDefinition& other) = default;

  [[nodiscard]] CString FormatHorizontalAlignment() const;
  [[nodiscard]] CString FormatPath() const;
  [[nodiscard]] CString FormatPrecision() const;
  [[nodiscard]] CString FormatVerticalAlignment() const;

  [[nodiscard]] double CharacterSpacing() const noexcept { return m_characterSpacing; }
  [[nodiscard]] EoUInt16 HorizontalAlignment() const noexcept { return m_horizontalAlignment; }
  [[nodiscard]] const CString& FontName() const noexcept { return m_fontName; }
  [[nodiscard]] EoUInt16 Precision() const noexcept { return m_precision; }
  [[nodiscard]] EoUInt16 Path() const noexcept { return m_path; }
  [[nodiscard]] EoUInt16 VerticalAlignment() const noexcept { return m_verticalAlignment; }

  void SetAlignment(EoUInt16 horizontalAlignment, EoUInt16 verticalAlignment) {
    m_horizontalAlignment = horizontalAlignment;
    m_verticalAlignment = verticalAlignment;
  }
  void SetCharacterSpacing(double characterSpacing) { m_characterSpacing = characterSpacing; }
  void SetFontName(const CString& fontName) { m_fontName = fontName; }
  void SetHorizontalAlignment(EoUInt16 horizontalAlignment) { m_horizontalAlignment = horizontalAlignment; }
  void SetPath(EoUInt16 path) { m_path = path; }
  void SetPrecision(EoUInt16 precision) { m_precision = precision; }
  void SetVerticalAlignment(EoUInt16 verticalAlignment) { m_verticalAlignment = verticalAlignment; }

  void Read(CFile& file);
  void Write(CFile& file) const;
};
