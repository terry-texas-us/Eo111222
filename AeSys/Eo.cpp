#include "Stdafx.h"

#include < Windows.h>
#include <string>

namespace Eo {

std::wstring MultiByteToWString(const char* multiByte) {
  if (!multiByte) return {L""};
  int size = ::MultiByteToWideChar(CP_UTF8, 0, multiByte, -1, nullptr, 0);
  if (size == 0) return {L""};
  std::wstring string;
  string.resize(static_cast<size_t>(size) - 1);
  ::MultiByteToWideChar(CP_UTF8, 0, multiByte, -1, &string[0], size - 1);
  return string;
}

std::string WStringToMultiByte(const std::wstring& wideString) {
  if (wideString.empty()) return std::string();
  int size = ::WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, nullptr, 0, nullptr, nullptr);
  if (size == 0) return std::string();
  std::string multiByte;
  multiByte.resize(static_cast<size_t>(size) - 1);
  ::WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, &multiByte[0], size - 1, nullptr, nullptr);
  return multiByte;
}

}  // namespace Eo