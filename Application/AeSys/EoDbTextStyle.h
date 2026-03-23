#pragma once

#include <cstdint>
#include <string>

/// @brief Represents a single STYLE table entry for DXF text style round-trip.
///
/// Stores the DXF STYLE table fields needed to preserve text style definitions
/// across DXF import/export. The field names and semantics match the DXF STYLE
/// table entry (AcDbTextStyleTableRecord) group codes.
///
/// A style table item is also used to record shape file LOAD command requests.
/// In that case bit 0x01 is set in @c m_flagValues and only @c m_font (group 3)
/// is meaningful.
class EoDbTextStyle {
 public:
  EoDbTextStyle() = default;

  /// @brief Style name (DXF group 2). Typically "Standard" for the default style.
  std::wstring m_name{L"Standard"};

  /// @brief Fixed text height; 0.0 if not fixed (DXF group 40).
  double m_height{};

  /// @brief Width factor (DXF group 41). Default 1.0.
  double m_widthFactor{1.0};

  /// @brief Oblique angle in radians (DXF group 50 is in degrees; converted at import).
  double m_obliqueAngle{};

  /// @brief Text generation flags (DXF group 71). 0x02 = mirrored in X, 0x04 = mirrored in Y.
  std::int16_t m_textGenerationFlags{};

  /// @brief Last height used (DXF group 42). Informational; preserved for round-trip.
  double m_lastHeight{1.0};

  /// @brief Primary font file name (DXF group 3). Default "txt".
  std::wstring m_font{L"txt"};

  /// @brief Bigfont file name; empty if none (DXF group 4).
  std::wstring m_bigFont{};

  /// @brief TrueType font pitch/family, charset, italic and bold flags (DXF group 1071).
  int m_fontFamily{};

  /// @brief Standard DXF flag values (DXF group 70). Bit 0x01 = shape file entry.
  std::int16_t m_flagValues{};

  /// @brief Entity handle from the DXF STYLE table (DXF group 5).
  std::uint64_t m_handle{};

  /// @brief Owner handle — typically the STYLE table handle (DXF group 330).
  std::uint64_t m_ownerHandle{};
};
