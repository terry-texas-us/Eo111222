#include <cwctype>

#include "DdeStringUtil.h"

wchar_t* dde::SkipWhiteSpace(wchar_t* inputLine) {
  while (inputLine && *inputLine && iswspace(*inputLine)) { inputLine++; }

  return inputLine;
}

wchar_t* dde::ScanForChar(wchar_t character, wchar_t** lineBuffer) {
  auto position = SkipWhiteSpace(*lineBuffer);

  if (!position || !*position) { return nullptr; }

  if (*position == character) {
    *lineBuffer = position + 1;
    return position;
  }
  return nullptr;  // not found
}

wchar_t* dde::ScanForString(wchar_t** ppStr, wchar_t* pszTerm, wchar_t** ppArgBuf) {
  wchar_t* pIn = SkipWhiteSpace(*ppStr);
  wchar_t* pStart = *ppArgBuf;
  wchar_t* pOut = pStart;

  bool bInQuotes = *pIn == L'"';

  if (bInQuotes) { pIn++; }

  // If not inside a quoted string and the next character cannot start a token (delimiter, closing paren, comma, or
  // end-of-string), report the terminator and return nullptr so the caller sees zero arguments rather than an empty
  // string token. Quoted-empty "" still reaches the loop above and returns a valid (zero-length) arg, preserving the
  // distinction between no-arg and explicit-empty-string.
  if (!bInQuotes) {
    const bool isTokenStart = *pIn != L'\0'
        && (iswalnum(*pIn) || *pIn == L'_' || *pIn == L'$' || *pIn == L'.' || *pIn == L'-' || *pIn == L':'
            || *pIn == L'\\');
    if (!isTokenStart) {
      *pszTerm = *pIn;
      *ppStr = *pIn ? pIn + 1 : pIn;
      return nullptr;
    }
  }

  do {
    if (bInQuotes) {
      if ((*pIn == L'"') && (*(pIn + 1) != L'"')) {  // End of quoted string
        pIn++;
        break;
      }
    } else if (iswalnum(*pIn)) {
      ;  // accepted as-is below
    } else {  // allow path/coordinate specials in unquoted tokens
      if (!(*pIn == L'_' || *pIn == L'$' || *pIn == L'.' || *pIn == L'-' || *pIn == L':' || *pIn == L'\\')) { break; }
    }
    if ((*pIn == L'"') && (*(pIn + 1) == L'"')) {
      pIn++;  // Skip the escaping first quote of a doubled-quote sequence
    }

    if (*pIn == L'\\' && *(pIn + 1) == L'\\') {
      pIn++;  // Skip the escaping backslash of a doubled-backslash sequence
    }

    *pOut++ = *pIn++;  // copy char to arg buffer

  } while (*pIn);

  *pOut++ = L'\0';  // null-terminate and update scan pointer
  *pszTerm = *pIn;
  if (*pIn) {
    *ppStr = pIn + 1;
  } else {
    *ppStr = pIn;
  }

  *ppArgBuf = pOut;  // advance arg buffer to next free slot

  return pStart;
}
