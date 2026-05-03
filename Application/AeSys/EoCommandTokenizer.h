#pragma once

#include <string>
#include <vector>

/// @brief Helpers for tokenizing and normalizing command-line input.
namespace EoCommandTokenizer {

/// @brief Parsed coordinate token from the command line.
///
/// Accepted forms (whitespace within the token is not allowed):
///   x,y             — absolute 2D world point (z = 0).
///   x,y,z           — absolute 3D world point.
///   @x,y            — relative cartesian offset from the current cursor world point.
///   @x,y,z          — relative 3D cartesian offset.
///   distance<angle  — absolute polar (angle in degrees, CCW from +X). z = 0.
///   @distance<angle — relative polar offset from the current cursor world point.
///
/// Polar tokens are converted to cartesian (x, y) at parse time; consumers see
/// only x/y/z fields. The isRelative flag still indicates @-prefixed input.
struct EoCoordinate {
  bool isRelative{false};
  bool hasZ{false};
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

/// @brief Returns true if @p token would parse as an EoCoordinate. Used to
///        detect coordinate-only command lines (continuation of the active
///        gesture without a leading verb).
[[nodiscard]] bool IsCoordinateToken(const std::wstring& token);

/// @brief Trims leading/trailing whitespace and upper-cases ASCII letters.
[[nodiscard]] std::wstring NormalizeCommand(const std::wstring& input);

/// @brief Splits @p input on whitespace into successive non-empty tokens.
/// @details Tokens are returned in source order; no upper-casing is applied so
///          coordinate strings (e.g. "1.5,2") survive intact for later parsing.
[[nodiscard]] std::vector<std::wstring> SplitTokens(const std::wstring& input);

/// @brief Attempts to parse @p token as an EoCoordinate. Returns false on any
///        malformed component.
[[nodiscard]] bool TryParseCoordinate(const std::wstring& token, EoCoordinate& out);

/// @brief Returns true and sets @p out when @p token is a bare positive number
///        with no comma, '@', or '<' — the Direct Distance Entry format.
///        Used to detect "5" as "draw 5 units in the cursor direction."
[[nodiscard]] bool TryParseDistance(const std::wstring& token, double& out);

/// @brief Splits @p input on whitespace, but treats double-quoted spans as a
///        single token (quotes stripped).  Single-word strings need not be
///        quoted.  A lone closing quote without an opening quote is treated as
///        a regular character.  Coordinate tokens ("1,2" etc.) are never
///        confused with string tokens because they contain no whitespace.
///
/// Example:
///   TEXT 0,0 "Hello World"  ->  {"TEXT", "0,0", "Hello World"}
///   TEXT 0,0 Hello          ->  {"TEXT", "0,0", "Hello"}
[[nodiscard]] std::vector<std::wstring> SplitTokensWithStrings(const std::wstring& input);

/// @brief Returns true if @p token is a string argument — i.e. it is neither a
///        coordinate nor a bare number.  Used by ExecuteCommand to distinguish
///        text arguments from coordinate arguments in the post-verb token list.
[[nodiscard]] bool IsStringArgument(const std::wstring& token);

}  // namespace EoCommandTokenizer
