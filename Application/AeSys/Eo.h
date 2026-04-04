#pragma once
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

namespace Eo {

/// @brief Identifies the active UI color scheme for the drawing views.
enum class ColorScheme { Dark, Light };

/// @brief Holds all scheme-dependent colors used by the rendering pipeline and UI chrome.
struct ColorSchemeColors {
  COLORREF modelSpaceBackground;  ///< Background for model-space views
  COLORREF paperSpaceBackground;  ///< Background for paper-space layout views
  COLORREF rubberband;  ///< Rubberband / selection feedback color
  COLORREF gridDot;  ///< Grid dot/line color
  // Docking pane / property grid colors
  COLORREF paneBackground;  ///< Background for property grid and output list boxes
  COLORREF paneText;  ///< Text color for property grid and output list boxes
  COLORREF paneGroupBackground;  ///< Property grid group-row background
  COLORREF paneGroupText;  ///< Property grid group-row text
  COLORREF paneLine;  ///< Property grid separator line color
  COLORREF paneDescriptionBackground;  ///< Property grid description area background
  COLORREF paneDescriptionText;  ///< Property grid description area text
  // Chrome colors (visual manager)
  COLORREF captionBackground;  ///< Docking pane caption bar background
  COLORREF captionText;  ///< Docking pane caption bar text
  COLORREF captionActiveBackground;  ///< Active docking pane caption bar background
  COLORREF captionActiveText;  ///< Active docking pane caption bar text
  COLORREF toolbarBackground;  ///< Toolbar / command bar fill
  COLORREF menuBackground;  ///< Menu bar and dropdown background
  COLORREF menuText;  ///< Menu bar and dropdown text
  COLORREF menuHighlightBackground;  ///< Menu item highlight/hover background
  COLORREF menuHighlightBorder;  ///< Menu item highlight/hover border
  COLORREF statusBarBackground;  ///< Status bar background
  COLORREF statusBarText;  ///< Status bar text
  COLORREF tabActiveBackground;  ///< Active MDI/pane tab background
  COLORREF tabActiveText;  ///< Active MDI/pane tab text
  COLORREF tabInactiveBackground;  ///< Inactive MDI/pane tab background
  COLORREF tabInactiveText;  ///< Inactive MDI/pane tab text
  COLORREF separatorColor;  ///< Toolbar separators and divider lines
  COLORREF borderColor;  ///< Pane and toolbar borders
};

/// @brief Dark scheme — warm charcoal optimized for ACI color palette visibility.
/// Foundation: RGB(40, 40, 36) — warm dark gray at AutoCAD luminance (~39).
/// Warm bias (R ≥ G > B) improves Blue ACI #5 contrast while remaining perceptually neutral.
inline constexpr ColorSchemeColors darkSchemeColors{
    RGB(40, 40, 36),  // modelSpaceBackground — warm charcoal, ACI-optimized
    RGB(255, 255, 255),  // paperSpaceBackground (always white for print fidelity)
    RGB(120, 118, 112),  // rubberband — warm mid-gray
    RGB(68, 68, 62),  // gridDot — warm, subtle grid overlay
    // Docking pane / property grid colors – sits comfortably between model space and chrome
    RGB(55, 55, 50),  // paneBackground — warm dark panel
    RGB(214, 212, 207),  // paneText — warm primary text
    RGB(66, 65, 59),  // paneGroupBackground
    RGB(176, 174, 169),  // paneGroupText — warm secondary text
    RGB(56, 56, 51),  // paneLine — warm structural divider
    RGB(66, 65, 59),  // paneDescriptionBackground — elevated within panel
    RGB(176, 174, 169),  // paneDescriptionText — warm secondary text
    // Chrome-colors – three-tier - all brighter than main view RGB(40,40,36))
    RGB(66, 65, 59),  // captionBackground – Tier 2 base chrome
    RGB(150, 148, 143),  // captionText – warm tertiary text
    RGB(0, 122, 204),  // captionActiveBackground (VS accent blue)
    RGB(255, 255, 255),  // captionActiveText
    RGB(66, 65, 59),  // toolbarBackground – Tier 2 base chrome
    RGB(52, 52, 47),  // menuBackground – Tier 1 recessed
    RGB(230, 228, 222),  // menuText – warm bright text
    RGB(79, 77, 70),  // menuHighlightBackground – Tier 3 elevated hover
    RGB(0, 122, 204),  // menuHighlightBorder (VS accent blue)
    RGB(66, 65, 59),  // statusBarBackground – Tier 2 base chrome
    RGB(214, 212, 207),  // statusBarText – warm primary text
    RGB(79, 77, 70),  // tabActiveBackground – Tier 3 elevated for clear distinction
    RGB(214, 212, 207),  // tabActiveText – warm primary text
    RGB(52, 52, 47),  // tabInactiveBackground – Tier 1 recessed
    RGB(150, 148, 143),  // tabInactiveText – warm tertiary text
    RGB(56, 56, 51),  // separatorColor – warm divider
    RGB(56, 56, 51),  // borderColor – warm structural border
};

/// @brief Light scheme — warm white optimized for ACI color palette visibility.
/// Model-space background stays pure white for maximum ACI clarity.
/// Chrome surfaces carry a subtle warm tint (R ≥ G > B) for visual harmony.
inline constexpr ColorSchemeColors lightSchemeColors{
    RGB(255, 255, 255),  // modelSpaceBackground — pure white for ACI clarity
    RGB(255, 255, 255),  // paperSpaceBackground
    RGB(142, 140, 136),  // rubberband — warm mid-gray
    RGB(202, 200, 196),  // gridDot — warm subtle grid
    // Docking pane / property grid colors
    RGB(253, 252, 249),  // paneBackground — warm near-white
    RGB(34, 33, 30),  // paneText — warm near-black
    RGB(242, 241, 238),  // paneGroupBackground — warm light panel
    RGB(120, 118, 114),  // paneGroupText — warm secondary text
    RGB(214, 213, 209),  // paneLine — warm light divider
    RGB(246, 245, 242),  // paneDescriptionBackground — warm light
    RGB(84, 82, 78),  // paneDescriptionText — warm dark gray
    // Chrome colors
    RGB(240, 239, 236),  // captionBackground — warm light chrome
    RGB(72, 70, 66),  // captionText — warm dark text
    RGB(0, 122, 204),  // captionActiveBackground (VS accent blue)
    RGB(255, 255, 255),  // captionActiveText
    RGB(240, 239, 236),  // toolbarBackground — warm light chrome
    RGB(248, 247, 244),  // menuBackground — warm lightest tier
    RGB(34, 33, 30),  // menuText — warm near-black
    RGB(220, 230, 240),  // menuHighlightBackground — soft warm-blue hover
    RGB(0, 122, 204),  // menuHighlightBorder (VS accent blue)
    RGB(240, 239, 236),  // statusBarBackground — warm chrome
    RGB(34, 33, 30),  // statusBarText — warm near-black
    RGB(255, 254, 251),  // tabActiveBackground — warm white
    RGB(34, 33, 30),  // tabActiveText — warm near-black
    RGB(246, 245, 242),  // tabInactiveBackground — warm off-white
    RGB(72, 70, 66),  // tabInactiveText — warm dark text
    RGB(240, 239, 236),  // separatorColor — warm chrome tier
    RGB(214, 213, 209),  // borderColor — warm light border
};

/// @brief Returns the color set for the given scheme.
[[nodiscard]] inline constexpr const ColorSchemeColors& SchemeColors(ColorScheme scheme) noexcept {
  return (scheme == ColorScheme::Light) ? lightSchemeColors : darkSchemeColors;
}

/// @brief The application-wide active color scheme, persisted via EoApOptions.
inline ColorScheme activeColorScheme = ColorScheme::Dark;

/// @brief Convenience accessor — returns the active scheme's model-space background.
[[nodiscard]] inline COLORREF ModelSpaceBackgroundColor() noexcept {
  return SchemeColors(activeColorScheme).modelSpaceBackground;
}

/// @brief Convenience accessor — returns the active scheme's paper-space background.
[[nodiscard]] inline COLORREF PaperSpaceBackgroundColor() noexcept {
  return SchemeColors(activeColorScheme).paperSpaceBackground;
}

/// @brief Returns the appropriate view background color for the given drawing space.
/// @param isPaperSpace true for paper-space layout views, false for model-space.
[[nodiscard]] inline COLORREF ViewBackgroundColorForSpace(bool isPaperSpace) noexcept {
  const auto& colors = SchemeColors(activeColorScheme);
  return isPaperSpace ? colors.paperSpaceBackground : colors.modelSpaceBackground;
}

/// @brief Returns the rubberband color for the active scheme.
[[nodiscard]] inline COLORREF RubberbandColor() noexcept { return SchemeColors(activeColorScheme).rubberband; }

/// @brief Legacy mutable alias — kept for callers that read the background color
/// without knowing the active space. Synchronized via SyncViewBackgroundColor().
inline COLORREF ViewBackgroundColor = RGB(40, 40, 36);

/// @brief Synchronizes the legacy ViewBackgroundColor global with the active scheme's model-space background.
/// Must be called after changing activeColorScheme.
inline void SyncViewBackgroundColor() noexcept {
  ViewBackgroundColor = SchemeColors(activeColorScheme).modelSpaceBackground;
}

inline COLORREF GrayPalette[16] = {RGB(255, 255, 255), RGB(140, 140, 140), RGB(0xbe, 0xbe, 0xbe), RGB(0xdc, 0xdc, 0xdc),
    RGB(0xf0, 0xf0, 0xf0), RGB(0x8d, 0x8d, 0x8d), RGB(191, 191, 191), RGB(0xdd, 0xdd, 0xdd), RGB(0xf1, 0xf1, 0xf1),
    RGB(0x8f, 0x8f, 0x8f), RGB(0xc1, 0xc1, 0xc1), RGB(223, 223, 223), RGB(0xc1, 0xc1, 0xc1), RGB(0x90, 0x90, 0x90),
    RGB(0xc2, 0xc2, 0xc2), RGB(0x64, 0x64, 0x64)};

inline COLORREF ColorPalette[256] = {RGB(0, 0, 0), RGB(255, 0, 0), RGB(255, 255, 0), RGB(0, 255, 0), RGB(0, 255, 255),
    RGB(0, 0, 255), RGB(255, 0, 255), RGB(255, 255, 255), RGB(128, 128, 128), RGB(192, 192, 192), RGB(255, 0, 0),
    RGB(255, 127, 127), RGB(165, 0, 0), RGB(165, 82, 82), RGB(127, 0, 0), RGB(127, 63, 63), RGB(76, 0, 0),
    RGB(76, 38, 38), RGB(38, 0, 0), RGB(38, 19, 19), RGB(255, 63, 0), RGB(255, 159, 127), RGB(165, 41, 0),
    RGB(165, 103, 82), RGB(127, 31, 0), RGB(127, 79, 63), RGB(76, 19, 0), RGB(76, 47, 38), RGB(38, 9, 0),
    RGB(38, 23, 19), RGB(255, 127, 0), RGB(255, 191, 127), RGB(165, 82, 0), RGB(165, 124, 82), RGB(127, 63, 0),
    RGB(127, 95, 63), RGB(76, 38, 0), RGB(76, 57, 38), RGB(38, 19, 0), RGB(38, 28, 19), RGB(255, 191, 0),
    RGB(255, 223, 127), RGB(165, 124, 0), RGB(165, 145, 82), RGB(127, 95, 0), RGB(127, 111, 63), RGB(76, 57, 0),
    RGB(76, 66, 38), RGB(38, 28, 0), RGB(38, 33, 19), RGB(255, 255, 0), RGB(255, 255, 127), RGB(165, 165, 0),
    RGB(165, 165, 82), RGB(127, 127, 0), RGB(127, 127, 63), RGB(76, 76, 0), RGB(76, 76, 38), RGB(38, 38, 0),
    RGB(38, 38, 19), RGB(191, 255, 0), RGB(223, 255, 127), RGB(124, 165, 0), RGB(145, 165, 82), RGB(95, 127, 0),
    RGB(111, 127, 63), RGB(57, 76, 0), RGB(66, 76, 38), RGB(28, 38, 0), RGB(33, 38, 19), RGB(127, 255, 0),
    RGB(191, 255, 127), RGB(82, 165, 0), RGB(124, 165, 82), RGB(63, 127, 0), RGB(95, 127, 63), RGB(38, 76, 0),
    RGB(57, 76, 38), RGB(19, 38, 0), RGB(28, 38, 19), RGB(63, 255, 0), RGB(159, 255, 127), RGB(41, 165, 0),
    RGB(103, 165, 82), RGB(31, 127, 0), RGB(79, 127, 63), RGB(19, 76, 0), RGB(47, 76, 38), RGB(9, 38, 0),
    RGB(23, 38, 19), RGB(0, 255, 0), RGB(127, 255, 127), RGB(0, 165, 0), RGB(82, 165, 82), RGB(0, 127, 0),
    RGB(63, 127, 63), RGB(0, 76, 0), RGB(38, 76, 38), RGB(0, 38, 0), RGB(19, 38, 19), RGB(0, 255, 63),
    RGB(127, 255, 159), RGB(0, 165, 41), RGB(82, 165, 103), RGB(0, 127, 31), RGB(63, 127, 79), RGB(0, 76, 19),
    RGB(38, 76, 47), RGB(0, 38, 9), /* RGB( 19,  88,  23), */ RGB(19, 38, 23), RGB(0, 255, 127), RGB(127, 255, 191),
    RGB(0, 165, 82), RGB(82, 165, 124), RGB(0, 127, 63), RGB(63, 127, 95), RGB(0, 76, 38), RGB(38, 76, 57),
    RGB(0, 38, 19), /* RGB( 19,  88,  28), */ RGB(19, 38, 28), RGB(0, 255, 191), RGB(127, 255, 223), RGB(0, 165, 124),
    RGB(82, 165, 145), RGB(0, 127, 95), RGB(63, 127, 111), RGB(0, 76, 57), RGB(38, 76, 66), RGB(0, 38, 28),
    RGB(19, 38, 33), RGB(0, 255, 255), RGB(127, 255, 255), RGB(0, 165, 165), RGB(82, 165, 165), RGB(0, 127, 127),
    RGB(63, 127, 127), RGB(0, 76, 76), RGB(38, 76, 76), RGB(0, 38, 38), RGB(19, 38, 38), RGB(0, 191, 255),
    RGB(127, 223, 255), RGB(0, 124, 165), RGB(82, 145, 165), RGB(0, 95, 127), RGB(63, 111, 127), RGB(0, 57, 76),
    RGB(38, 66, 126), RGB(0, 28, 38), RGB(19, 33, 38), RGB(0, 127, 255), RGB(127, 191, 255), RGB(0, 82, 165),
    RGB(82, 124, 165), RGB(0, 63, 127), RGB(63, 95, 127), RGB(0, 38, 76), RGB(38, 57, 126), RGB(0, 19, 38),
    RGB(19, 28, 38), RGB(0, 63, 255), RGB(127, 159, 255), RGB(0, 41, 165), RGB(82, 103, 165), RGB(0, 31, 127),
    RGB(63, 79, 127), RGB(0, 19, 76), RGB(38, 47, 126), RGB(0, 9, 38), RGB(19, 23, 38), RGB(0, 0, 255),
    RGB(127, 127, 255), RGB(0, 0, 165), RGB(82, 82, 165), RGB(0, 0, 127), RGB(63, 63, 127), RGB(0, 0, 76),
    RGB(38, 38, 126), RGB(0, 0, 38), RGB(19, 19, 38), RGB(63, 0, 255), RGB(159, 127, 255), RGB(41, 0, 165),
    RGB(103, 82, 165), RGB(31, 0, 127), RGB(79, 63, 127), RGB(19, 0, 76), RGB(47, 38, 126), RGB(9, 0, 38),
    RGB(23, 19, 38), RGB(127, 0, 255), RGB(191, 127, 255), RGB(82, 0, 165), RGB(124, 82, 165), RGB(63, 0, 127),
    RGB(95, 63, 127), RGB(38, 0, 76), RGB(57, 38, 126), RGB(19, 0, 38), RGB(28, 19, 38), RGB(191, 0, 255),
    RGB(223, 127, 255), RGB(124, 0, 165), RGB(145, 82, 165), RGB(95, 0, 127), RGB(111, 63, 127), RGB(57, 0, 76),
    RGB(66, 38, 76), RGB(28, 0, 38), RGB(33, 19, 38), RGB(255, 0, 255), RGB(255, 127, 255), RGB(165, 0, 165),
    RGB(165, 82, 165), RGB(127, 0, 127), RGB(127, 63, 127), RGB(76, 0, 76), RGB(76, 38, 76), RGB(38, 0, 38),
    RGB(38, 19, 38), RGB(255, 0, 191), RGB(255, 127, 223), RGB(165, 0, 124), RGB(165, 82, 145), RGB(127, 0, 95),
    RGB(127, 63, 111), RGB(76, 0, 57), RGB(76, 38, 66), RGB(38, 0, 28), RGB(38, 19, 33), RGB(255, 0, 127),
    RGB(255, 127, 191), RGB(165, 0, 82), RGB(165, 82, 124), RGB(127, 0, 63), RGB(127, 63, 95), RGB(76, 0, 38),
    RGB(76, 38, 57), RGB(38, 0, 19), RGB(38, 19, 28), RGB(255, 0, 63), RGB(255, 127, 159), RGB(165, 0, 41),
    RGB(165, 82, 103), RGB(127, 0, 31), RGB(127, 63, 79), RGB(76, 0, 19), RGB(76, 38, 47), RGB(38, 0, 9),
    RGB(38, 19, 23), RGB(0, 0, 0), RGB(101, 101, 101), RGB(102, 102, 102), RGB(153, 153, 153), RGB(204, 204, 204),
    RGB(255, 255, 255)};

// One and only stroke font supported by AeSys, used for all stroke text rendering
constexpr const wchar_t* defaultStrokeFont = L"Simplex.psf";
constexpr double defaultStrokeFontHeight = 0.125;
constexpr double defaultCharacterCellAspectRatio = 0.6;  // Width to height ratio of the character cell

/// Stroke font file format constants.
/// v1: 96 offset entries (chars 32-126 + sentinel), stroke data at int32[96], 16384 bytes total.
/// v2: magic at int32[0], 225 offset entries at int32[1], 224 advance widths at int32[226],
///    224 left bearings at int32[450], stroke data at int32[674].
constexpr int strokeFontFileSizeInBytes = 16384;
constexpr int strokeFontV1OffsetTableSize = 96;
constexpr int strokeFontV1MaxCharacterCode = 126;
constexpr int strokeFontV2MagicNumber = -2;
constexpr int strokeFontV2OffsetTableStart = 1;  // offset table begins at int32[1]
constexpr int strokeFontV2OffsetTableEntries = 225;  // 224 characters (32-255) + 1 sentinel
constexpr int strokeFontV2AdvanceWidthTableStart = 226;  // 224 advance widths (raw stroke X-units)
constexpr int strokeFontV2LeftBearingTableStart = 450;  // 224 left bearings (raw stroke X-units)
constexpr int strokeFontV2StrokeDataStart = 674;  // stroke data follows left bearing table
constexpr int strokeFontV2MaxCharacterCode = 255;

constexpr std::int16_t continuousLineTypeIndex{1};

constexpr COLORREF colorBlack = RGB(0, 0, 0);
constexpr COLORREF colorRed = RGB(255, 0, 0);  // ACI 1
constexpr COLORREF colorWhite = RGB(255, 255, 255);  // ACI 7
constexpr COLORREF colorGray = RGB(128, 128, 128);  // ACI 8
constexpr COLORREF colorNavy = RGB(0, 0, 128);

constexpr COLORREF colorRubberband = RGB(120, 118, 112);
constexpr COLORREF colorViewBackground = RGB(40, 40, 36);

constexpr std::int16_t defaultColor = 7;

/// @brief Synchronizes ACI 7 and ACI 0 palette entries with the model-space background brightness.
/// ACI 7 displays as white on dark backgrounds and black on light backgrounds (AutoCAD convention).
/// ACI 0 is swapped complementarily for XOR drawing mode correctness (see SetROP2).
/// Must be called after changing activeColorScheme, typically alongside SyncViewBackgroundColor().
inline void SyncAci7WithBackground() noexcept {
  const auto& colors = SchemeColors(activeColorScheme);
  const int luminance = GetRValue(colors.modelSpaceBackground) + GetGValue(colors.modelSpaceBackground) +
      GetBValue(colors.modelSpaceBackground);
  if (luminance > 384) {
    // Light background — ACI 7 = black, ACI 0 = white
    ColorPalette[7] = colorBlack;
    ColorPalette[0] = colorWhite;
    GrayPalette[7] = RGB(0x22, 0x22, 0x22);
    GrayPalette[0] = RGB(0xdd, 0xdd, 0xdd);
  } else {
    // Dark background — ACI 7 = white, ACI 0 = black (factory default)
    ColorPalette[7] = colorWhite;
    ColorPalette[0] = colorBlack;
    GrayPalette[7] = RGB(0xdd, 0xdd, 0xdd);
    GrayPalette[0] = RGB(0xff, 0xff, 0xff);
  }
}

constexpr double MmPerInch = 25.4;

constexpr double Pi = std::numbers::pi;
constexpr double QuarterPi = std::numbers::pi / 4.0;
constexpr double HalfPi = std::numbers::pi / 2.0;
constexpr double ThreeQuartersPi = 3.0 * std::numbers::pi / 4.0;
constexpr double TwoPi = std::numbers::pi + std::numbers::pi;
constexpr double Radian = std::numbers::pi / 180.0;
constexpr double DegreeToRadian(double angleInDegrees) noexcept { return angleInDegrees * Radian; }
constexpr double RadianToDegree(double angleInRadians) noexcept { return angleInRadians * (180.0 / std::numbers::pi); }

// Absolute tolerances for geometric calculations
constexpr double generalTolerance = 1e-6;  // For general UI, display rounding (device space)
constexpr double geometricTolerance = 1e-9;  // For geometric calculations, point coincidence etc. (modelspace)

/** @brief Checks if a geometric value is effectively non-zero considering the defined geometric tolerance.
 * @param geometricValue The value to check.
 * @return true if the absolute value of geometricValue is greater than geometricTolerance, false otherwise.
 */
inline bool IsGeometricallyNonZero(double geometricValue) { return std::abs(geometricValue) > geometricTolerance; }

/** @brief Checks if a geometric value is effectively zero considering the defined geometric tolerance.
 * @param geometricValue The value to check.
 * @return true if the absolute value of geometricValue is less than or equal to geometricTolerance, false otherwise.
 */
inline bool IsGeometricallyZero(double geometricValue) { return std::abs(geometricValue) <= geometricTolerance; }

// Arc/conic tessellation — base number of line segments used to approximate a full 360° circle.
// Partial arcs scale proportionally: segments = ceil(|sweep| / 2π × arcTessellationSegmentsPerFullCircle).
constexpr int arcTessellationSegmentsPerFullCircle = 72;
constexpr int arcTessellationMinimumSegments = 2;
constexpr int arcTessellationMaximumSegments = 128;

// Machine epsilon for dimensionless comparisons
constexpr double numericEpsilon = std::numeric_limits<double>::epsilon();

// Practical bounds based on geometric tolerance - beyond these values, calculations may become unreliable
constexpr double boundsMax = 1.0 / geometricTolerance;
constexpr double boundsMin = -boundsMax;

enum class Units {
  ArchitecturalS = -1,  // Embedded S format
  Architectural,
  Engineering,
  Feet,
  Inches,
  Meters,
  Millimeters,
  Centimeters,
  Decimeters,
  Kilometers
};

/** @brief Returns the sign of a number: -1 if negative, 1 if positive, 0 if zero. */
constexpr int Signum(double number) { return (number < 0.0 ? -1 : (number > 0.0 ? 1 : 0)); }

/** @brief Returns a value with the magnitude of 'a' and the sign of 'b'. */
constexpr double CopySign(double a, double b) { return (b >= 0.0 ? std::abs(a) : -std::abs(a)); }

/** @brief Rounds a double to the nearest integer using std::round, which rounds half away from zero.
 * @param number The double value to round.
 * @return The rounded integer value.
 * @note This function uses std::round for rounding, which follows the "round half away from zero" rule. Legacy
 * implementation had a different "round half toward infinity" rule.
 */
inline int Round(double number) { return static_cast<int>(std::round(number)); }

std::wstring MultiByteToWString(const char* multiByte);
std::string WStringToMultiByte(const std::wstring& wideString);

}  // namespace Eo
