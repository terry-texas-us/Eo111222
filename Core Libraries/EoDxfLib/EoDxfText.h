#pragma once

#include <cstdint>
#include <string>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"

/** @brief Class to handle text entity
 *
 *  A text entity represents a single line of text in a drawing.
 *  It is defined by its insertion point (code 10, 20, 30), height (code 40), and the text string itself (code 1).
 *  The text entity can also include properties such as rotation angle (code 50), width scale factor (code 41), oblique
 * angle (code 51), and text style name (code 7), which can affect how the text is rendered in the drawing.
 */
class EoDxfText : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  enum class VerticalAlignment : std::int16_t { BaseLine, Bottom, Middle, Top };

  enum class HorizontalAlignment : std::int16_t {
    Left,
    Center,
    Right,
    AlignedIfBaseLine,
    MiddleIfBaseLine,
    FitIfBaseLine
  };

  explicit EoDxfText(EoDxf::ETYPE entityType = EoDxf::TEXT) noexcept : EoDxfGraphic{entityType} {}

  void ApplyExtrusion() noexcept override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase3d m_firstAlignmentPoint;  // Group codes 10, 20 & 30
  EoDxfGeometryBase3d m_secondAlignmentPoint;  // Group codes 11, 21 & 31
  double m_textHeight{};  // Group code 40
  std::wstring m_string;  // Group code 1
  double m_textRotation{};  // Group code 50
  double m_scaleFactorWidth{1.0};  // Group code 41
  double m_obliqueAngle{};  // Group code 51
  std::wstring m_textStyleName{L"STANDARD"};  // Group code 7
  std::int16_t m_textGenerationFlags{};  // Group code 71 (optional)
  HorizontalAlignment m_horizontalAlignment{HorizontalAlignment::Left};  // Group code 72 (optional)
  VerticalAlignment m_verticalAlignment{VerticalAlignment::BaseLine};  // Group code 73 (optional)

  [[nodiscard]] bool IsAlignedOrFit() const noexcept {
    return m_horizontalAlignment == HorizontalAlignment::AlignedIfBaseLine ||
           m_horizontalAlignment == HorizontalAlignment::FitIfBaseLine;
  }
  [[nodiscard]] bool HasSecondAlignmentPoint() const noexcept { return m_hasSecondAlignmentPoint; }

 private:
  bool m_hasSecondAlignmentPoint{};  // Hassecond alignment point, so baseline can determine text rotation
};
