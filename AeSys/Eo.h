#pragma once
#include <cmath>
#include <numbers>
#include <string>

namespace Eo {
constexpr double MmPerInch = 25.4;

constexpr double Pi = std::numbers::pi;
constexpr double QuarterPi = std::numbers::pi / 4.0;
constexpr double HalfPi = std::numbers::pi / 2.0;
constexpr double ThreeQuartersPi = 3.0 * std::numbers::pi / 4.0;
constexpr double TwoPi = std::numbers::pi + std::numbers::pi;
constexpr double Radian = std::numbers::pi / 180.0;
constexpr double DegreeToRadian(double angleInDegrees) noexcept { return angleInDegrees * Radian; }
constexpr double RadianToDegree(double angleInRadians) noexcept { return angleInRadians * (180.0 / std::numbers::pi); }

constexpr double generalTolerance = 1e-6;    // For general UI
constexpr double geometricTolerance = 1e-9;  // For geometric calculations
constexpr double preciseTolerance = 1e-12;   // For precise calculations

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
