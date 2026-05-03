#include "Stdafx.h"

#include "EoCommandTokenizer.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cwchar>
#include <cwctype>

namespace {
constexpr double kDegToRad = 0.01745329251994329577;  // pi / 180
}  // namespace

namespace EoCommandTokenizer {

std::wstring NormalizeCommand(const std::wstring& input) {
  auto begin = input.begin();
  auto end = input.end();
  while (begin != end && std::iswspace(*begin) != 0) { ++begin; }
  while (end != begin && std::iswspace(*(end - 1)) != 0) { --end; }
  std::wstring result(begin, end);
  std::transform(result.begin(), result.end(), result.begin(), [](wchar_t ch) {
    return static_cast<wchar_t>(std::towupper(ch));
  });
  return result;
}

std::vector<std::wstring> SplitTokens(const std::wstring& input) {
  std::vector<std::wstring> tokens;
  std::size_t i = 0;
  const std::size_t n = input.size();
  while (i < n) {
    while (i < n && std::iswspace(input[i]) != 0) { ++i; }
    const std::size_t start = i;
    while (i < n && std::iswspace(input[i]) == 0) { ++i; }
    if (i > start) { tokens.emplace_back(input.substr(start, i - start)); }
  }
  return tokens;
}

namespace {

[[nodiscard]] bool TryParseDouble(const std::wstring& text, double& out) {
  if (text.empty()) { return false; }
  wchar_t* end = nullptr;
  errno = 0;
  const double value = std::wcstod(text.c_str(), &end);
  if (end == text.c_str() || *end != L'\0') { return false; }
  out = value;
  return true;
}

}  // namespace

bool TryParseCoordinate(const std::wstring& token, EoCoordinate& out) {
  if (token.empty()) { return false; }

  std::size_t pos = 0;
  out = EoCoordinate{};
  if (token[0] == L'@') {
    out.isRelative = true;
    pos = 1;
  }

  // Polar form: distance<angleDegrees (angle measured CCW from +X axis).
  const std::size_t polarPos = token.find(L'<', pos);
  if (polarPos != std::wstring::npos) {
    const std::wstring distText = token.substr(pos, polarPos - pos);
    const std::wstring angText = token.substr(polarPos + 1);
    double distance = 0.0;
    double angleDegrees = 0.0;
    if (!TryParseDouble(distText, distance)) { return false; }
    if (!TryParseDouble(angText, angleDegrees)) { return false; }
    const double angleRadians = angleDegrees * kDegToRad;
    out.x = distance * std::cos(angleRadians);
    out.y = distance * std::sin(angleRadians);
    out.z = 0.0;
    out.hasZ = false;
    return true;
  }

  std::vector<std::wstring> parts;
  std::size_t start = pos;
  for (std::size_t i = pos; i <= token.size(); ++i) {
    if (i == token.size() || token[i] == L',') {
      parts.emplace_back(token.substr(start, i - start));
      start = i + 1;
    }
  }
  if (parts.size() < 2 || parts.size() > 3) { return false; }

  if (!TryParseDouble(parts[0], out.x)) { return false; }
  if (!TryParseDouble(parts[1], out.y)) { return false; }
  if (parts.size() == 3) {
    if (!TryParseDouble(parts[2], out.z)) { return false; }
    out.hasZ = true;
  }
  return true;
}

bool IsCoordinateToken(const std::wstring& token) {
  EoCoordinate scratch{};
  return TryParseCoordinate(token, scratch);
}

}  // namespace EoCommandTokenizer
