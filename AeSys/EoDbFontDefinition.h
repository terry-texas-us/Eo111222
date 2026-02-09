#pragma once

#include "Eo.h"
#include "EoDb.h"

/** @class EoDbFontDefinition represents the font definition of a text primitive, including the font name, character spacing, precision, path, and horizontal and vertical alignment. */
class EoDbFontDefinition {
  CString m_fontName{Eo::defaultStrokeFont};
  double m_characterSpacing{};
  EoDb::Precision m_precision{EoDb::Precision::StrokeType};
  EoDb::Path m_path{EoDb::Path::Right};
  EoDb::HorizontalAlignment m_horizontalAlignment{EoDb::HorizontalAlignment::Left};
  EoDb::VerticalAlignment m_verticalAlignment{EoDb::VerticalAlignment::Bottom};

 public:
  [[nodiscard]] CString FormatPrecision() const;
  [[nodiscard]] CString FormatPath() const;
  [[nodiscard]] CString FormatHorizontalAlignment() const;
  [[nodiscard]] CString FormatVerticalAlignment() const;

  [[nodiscard]] double CharacterSpacing() const noexcept { return m_characterSpacing; }
  [[nodiscard]] const CString& FontName() const noexcept { return m_fontName; }
  [[nodiscard]] EoDb::HorizontalAlignment HorizontalAlignment() const noexcept { return m_horizontalAlignment; }
  [[nodiscard]] EoDb::Path Path() const noexcept { return m_path; }
  [[nodiscard]] EoDb::Precision Precision() const noexcept { return m_precision; }
  [[nodiscard]] EoDb::VerticalAlignment VerticalAlignment() const noexcept { return m_verticalAlignment; }

  /** * @brief Sets the font alignment properties.
   * @param horizontalAlignment The horizontal alignment of the font.
   * @param verticalAlignment The vertical alignment of the font.
   */
  void SetAlignment(EoDb::HorizontalAlignment horizontalAlignment, EoDb::VerticalAlignment verticalAlignment);
  void SetCharacterSpacing(double characterSpacing);
  void SetFontName(const CString& fontName);
  void SetHorizontalAlignment(EoDb::HorizontalAlignment horizontalAlignment);
  void SetPath(EoDb::Path path);
  void SetPrecision(EoDb::Precision precision);
  void SetVerticalAlignment(EoDb::VerticalAlignment verticalAlignment);

  void Read(CFile& file);
  void Write(CFile& file) const;
};
