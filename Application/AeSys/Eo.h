#pragma once
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

namespace Eo {

/// @brief Identifies the drawing view background preference (model space only).
/// Paper space is always white. Chrome is always light.
enum class ViewBackground { Dark, White };

/// @brief Holds all chrome colors used by the visual manager and docking panes.
/// Single light-chrome palette. View-dependent colors (model-space background,
/// rubberband, grid) are handled separately via ViewBackground and standalone
/// constexpr values below.
struct ChromeColors {
  // Docking pane / property grid colors — content panels between model-space and chrome
  COLORREF paneBackground = RGB(247, 246, 243);  ///< Background for property grid and output list boxes
  COLORREF paneText = RGB(28, 27, 24);  ///< Text color for property grid and output list boxes
  COLORREF paneGroupBackground = RGB(237, 236, 233);  ///< Property grid group-row background
  COLORREF paneGroupText = RGB(100, 98, 94);  ///< Property grid group-row text
  COLORREF paneLine = RGB(214, 213, 209);  ///< Property grid separator line color
  COLORREF paneDescriptionBackground = RGB(241, 240, 237);  ///< Property grid description area background
  COLORREF paneDescriptionText = RGB(78, 76, 72);  ///< Property grid description area text
  // Chrome colors (visual manager) — three-tier warm light, darkened for depth contrast
  COLORREF captionBackground = RGB(229, 228, 224);  ///< Docking pane caption bar background (Tier 2)
  COLORREF captionText = RGB(64, 62, 58);  ///< Docking pane caption bar text
  COLORREF captionActiveBackground = RGB(0, 122, 204);  ///< Active caption bar background (VS accent blue)
  COLORREF captionActiveText = RGB(255, 255, 255);  ///< Active caption bar text
  COLORREF toolbarBackground = RGB(229, 228, 224);  ///< Toolbar / command bar fill (Tier 2)
  COLORREF menuBackground = RGB(238, 237, 233);  ///< Menu bar and dropdown background (Tier 1)
  COLORREF menuText = RGB(28, 27, 24);  ///< Menu bar and dropdown text
  COLORREF menuHighlightBackground = RGB(220, 230, 240);  ///< Menu item highlight/hover background
  COLORREF menuHighlightBorder = RGB(0, 122, 204);  ///< Menu item highlight/hover border (VS accent blue)
  COLORREF statusBarBackground = RGB(229, 228, 224);  ///< Status bar background (Tier 2)
  COLORREF statusBarText = RGB(28, 27, 24);  ///< Status bar text
  COLORREF tabActiveBackground = RGB(247, 246, 243);  ///< Active MDI/pane tab background
  COLORREF tabActiveText = RGB(28, 27, 24);  ///< Active MDI/pane tab text
  COLORREF tabInactiveBackground = RGB(238, 237, 233);  ///< Inactive MDI/pane tab background (Tier 1)
  COLORREF tabInactiveText = RGB(64, 62, 58);  ///< Inactive MDI/pane tab text
  COLORREF separatorColor = RGB(229, 228, 224);  ///< Toolbar separators and divider lines (Tier 2)
  COLORREF borderColor = RGB(214, 213, 209);  ///< Pane and toolbar borders
};

inline constexpr ChromeColors chromeColors;

/// @brief The application-wide view background preference, persisted via EoApOptions.
inline ViewBackground activeViewBackground = ViewBackground::Dark;

constexpr COLORREF colorBlack = RGB(0, 0, 0);
constexpr COLORREF colorRed = RGB(255, 0, 0);  // ACI 1
constexpr COLORREF colorWhite = RGB(255, 255, 255);  // ACI 7
constexpr COLORREF colorGray = RGB(128, 128, 128);  // ACI 8
constexpr COLORREF colorNavy = RGB(0, 0, 128);

/// @brief Standard mask/transparency-key color for all bitmaps.
/// For CImageList: pass as the mask color in CImageList::Add().
/// For CMFCToolBar: set as the bottom-left pixel of the BMP (auto-detected as transparency key).
constexpr COLORREF bitmapMaskColor = chromeColors.paneBackground;

constexpr COLORREF colorRubberband = RGB(120, 118, 112);
constexpr COLORREF colorViewBackground = RGB(40, 40, 36);

/// @brief Returns the model-space background color for the active view background setting.
[[nodiscard]] inline COLORREF ModelSpaceBackgroundColor() noexcept {
  return (activeViewBackground == ViewBackground::Dark) ? colorViewBackground : colorWhite;
}

/// @brief Returns the paper-space "table" background color (the area behind the white sheet).
/// Dark view: warm dark gray; Light view: warm light gray. The sheet itself is always white.
[[nodiscard]] inline COLORREF PaperSpaceBackgroundColor() noexcept {
  return (activeViewBackground == ViewBackground::Dark) ? RGB(96, 95, 90) : RGB(192, 191, 187);
}

/// @brief The paper-space sheet fill color — always white, matching physical paper.
[[nodiscard]] inline constexpr COLORREF PaperSpaceSheetColor() noexcept { return colorWhite; }

/// @brief Returns the block-edit view background color — same warm gray as the paper-space table.
/// Visually signals "special view of a model component" and avoids ACI 7 white-on-white ambiguity.
[[nodiscard]] inline COLORREF BlockEditBackgroundColor() noexcept {
  return PaperSpaceBackgroundColor();
}

/// @brief Returns the appropriate view background color for the given drawing space.
/// @param isPaperSpace true for paper-space layout views, false for model-space.
[[nodiscard]] inline COLORREF ViewBackgroundColorForSpace(bool isPaperSpace) noexcept {
  return isPaperSpace ? PaperSpaceBackgroundColor() : ModelSpaceBackgroundColor();
}

/// @brief Returns the rubberband color appropriate for the active view background.
[[nodiscard]] inline COLORREF RubberbandColor() noexcept {
  return (activeViewBackground == ViewBackground::Dark) ? colorRubberband : RGB(130, 128, 124);
}

/// @brief Returns the grid dot color appropriate for the active view background.
[[nodiscard]] inline COLORREF GridDotColor() noexcept {
  return (activeViewBackground == ViewBackground::Dark) ? RGB(68, 68, 62) : RGB(202, 200, 196);
}

/// @brief Legacy mutable alias — kept for callers that read the background color
/// without knowing the active space. Synchronized via SyncViewBackgroundColor().
inline COLORREF ViewBackgroundColor = RGB(40, 40, 36);

/// @brief Synchronizes the legacy ViewBackgroundColor global with the active view background setting.
/// Must be called after changing activeViewBackground.
inline void SyncViewBackgroundColor() noexcept { ViewBackgroundColor = ModelSpaceBackgroundColor(); }

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

constexpr std::int16_t defaultColor = 7;

/// @brief Synchronizes ACI 7 and ACI 0 palette entries with the model-space background brightness.
/// ACI 7 displays as white on dark backgrounds and black on light backgrounds (AutoCAD convention).
/// ACI 0 is swapped complementarily for XOR drawing mode correctness (see SetROP2).
/// Must be called after changing activeViewBackground, typically alongside SyncViewBackgroundColor().
inline void SyncAci7WithBackground() noexcept {
  const COLORREF background = ModelSpaceBackgroundColor();
  const int luminance = GetRValue(background) + GetGValue(background) + GetBValue(background);
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
