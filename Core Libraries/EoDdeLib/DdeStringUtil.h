#pragma once

namespace dde {

/// @brief Skips whitespace characters in the input string.
/// @param inputLine The input string to process.
/// @return A pointer to the first non-whitespace character in the string.
wchar_t* SkipWhiteSpace(wchar_t* inputLine);

/// @brief Scans the line buffer for a specific character, skipping whitespace.
/// @param character The character to scan for.
/// @param lineBuffer A pointer to the lineBuffer to scan; updated to point after the found character.
/// @return A pointer to the found character in the string, or nullptr if not found.
wchar_t* ScanForChar(wchar_t character, wchar_t** lineBuffer);

/// @brief Scans the line buffer for a string token, handling quotes and escapes.
/// @param ppStr A pointer to the current position in the line buffer; updated to point after the scanned string.
/// @param pszTerm A pointer to store the terminating character after the string.
/// @param ppArgBuf A pointer to the argument buffer to store the scanned string; updated to point to the next free position.
/// @return A pointer to the start of the scanned string in the argument buffer.
wchar_t* ScanForString(wchar_t** ppStr, wchar_t* pszTerm, wchar_t** ppArgBuf);

}  // namespace dde
