#pragma once
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

namespace Eo {

// One and only stroke font supported by AeSys, used for all stroke text rendering
constexpr const wchar_t* defaultStrokeFont = L"Simplex.psf";
constexpr double defaultStrokeFontHeight = 0.125;
constexpr double defaultCharacterCellAspectRatio = 0.6;  // Width to height ratio of the character cell

constexpr std::int16_t continuousLineTypeIndex{1};

constexpr COLORREF colorBlack = RGB(0, 0, 0);
constexpr COLORREF colorRed = RGB(255, 0, 0);        // ACI 1
constexpr COLORREF colorWhite = RGB(255, 255, 255);  // ACI 7
constexpr COLORREF colorGray = RGB(128, 128, 128);   // ACI 8
constexpr COLORREF colorNavy = RGB(0, 0, 128);

constexpr COLORREF colorRubberband = RGB(102, 102, 102);
constexpr COLORREF colorViewBackground = RGB(33, 40, 47);

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
constexpr double generalTolerance = 1e-6;    // For general UI, display rounding (device space)
constexpr double geometricTolerance = 1e-9;  // For geometric calculations, point coincidence etc. (modelspace)

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
constexpr int Signum(const double number) { return (number < 0.0 ? -1 : (number > 0.0 ? 1 : 0)); }

/** @brief Returns a value with the magnitude of 'a' and the sign of 'b'. */
constexpr double CopySign(const double a, const double b) { return (b >= 0.0 ? fabs(a) : -fabs(a)); }

/** @brief Rounds a double to the nearest integer using std::round, which rounds half away from zero.
* @param number The double value to round.
* @return The rounded integer value.
* @note This function uses std::round for rounding, which follows the "round half away from zero" rule. Legacy implementation had a different "round half toward infinity" rule.
*/
inline int Round(const double number) { return static_cast<int>(std::round(number)); }

std::wstring MultiByteToWString(const char* multiByte);
std::string WStringToMultiByte(const std::wstring& wideString);

}  // namespace Eo
