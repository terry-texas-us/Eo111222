#include <cctype>

#include "DdeStringUtil.h"

wchar_t* dde::SkipWhiteSpace(wchar_t* inputLine) {
  while (inputLine && *inputLine && isspace(*inputLine)) { inputLine++; }

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

  bool bInQuotes = *pIn == '"';

  if (bInQuotes) { pIn++; }

  do {
    if (bInQuotes) {
      if ((*pIn == '"') && (*(pIn + 1) != '"')) {  // Skip over the quote
        pIn++;
        break;
      }
    } else if (isalnum(*pIn)) {
      ;
    } else {  // allow some peg specials
      if (!(*pIn == '_' || *pIn == '$' || *pIn == '.' || *pIn == '-' || *pIn == ':' || *pIn == '\\')) { break; }
    }
    if ((*pIn == '"') && (*(pIn + 1) == '"')) {
      // Skip the escaping first quote
      pIn++;
    }

    if (*pIn == '\\' && *(pIn + 1) == '\\') {
      // Skip the escaping backslash
      pIn++;
    }

    *pOut++ = *pIn++;  // the char to the arg buffer

  } while (*pIn);

  *pOut++ = '\0';  // Set up the terminating char and update the scan pointer
  *pszTerm = *pIn;
  if (*pIn) {
    *ppStr = pIn + 1;
  } else {
    *ppStr = pIn;
  }

  *ppArgBuf = pOut;  // Update the arg buffer to the next free bit

  return pStart;
}
