#pragma once

namespace dde {

/// <summary>Skips whitespace characters in the input string.</summary>
/// <param name="inputLine">The input string to process.</param>
/// <returns>A pointer to the first non-whitespace character in the string.</returns>
wchar_t* SkipWhiteSpace(wchar_t* inputLine);

/// <summary>Scans the line buffer for a specific character, skipping whitespace.</summary>
/// <param name="character">The character to scan for.</param>
/// <param name="lineBuffer">A pointer to the lineBuffer to scan; updated to point after the found character.</param>
/// <returns>A pointer to the found character in the string, or nullptr if not found.</returns>
wchar_t* ScanForChar(wchar_t character, wchar_t** lineBuffer);

/// <summary>Scans the line buffer for a string token, handling quotes and escapes.</summary>
/// <param name="ppStr">A pointer to the current position in the line buffer; updated to point after the scanned string.</param>
/// <param name="pszTerm">A pointer to store the terminating character after the string.</param>
/// <param name="ppArgBuf">A pointer to the argument buffer to store the scanned string; updated to point to the next free position.</param>
/// <returns>A pointer to the start of the scanned string in the argument buffer.</returns>
wchar_t* ScanForString(wchar_t** ppStr, wchar_t* pszTerm, wchar_t** ppArgBuf);

}  // namespace dde
