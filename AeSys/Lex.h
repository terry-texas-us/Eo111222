#pragma once

#include <cwctype>
#include <variant>
#include <string>
#include <cstdint>

enum TokenClass {
  Other,
  Constant,
  Identifier,
  BinaryArithmeticOperator,
  BinaryRelationalOperator,
  BinaryLogicOperator,
  UnaryLogicOperator,
  AssignOp,
  OpenParentheses,
  CloseParentheses
};

struct ColumnDefinition {
  long dataDefinition;
  long dataType;
};

struct TokenProperties {
  int inComingPriority;
  int inStackPriority;
  TokenClass tokenClass;
};

namespace lex {

using ValueVariant = std::variant<std::wstring, std::int64_t, double>;

/// Operand is a typed value container used by the expression evaluator.
/// `meta` preserves legacy dimension/length encoding (LOWORD/HIWORD) for staged migration.
struct Operand {
  ValueVariant v;
  long meta; // legacy value definition: LOWORD = dimension, HIWORD = length

  Operand() : v(std::int64_t(0)), meta(0) {}
};

constexpr int MaxTokens = 128;
constexpr int MaxValues = 256;

constexpr int AbsOperator = 1;
constexpr int AcosOperator = 2;
constexpr int AsinOperator = 3;
constexpr int AtanOperator = 4;
constexpr int StringUnaryOperator = 5;
constexpr int CosOperator = 6;
constexpr int ExpOperator = 7;
constexpr int IntUnaryOperator = 8;
constexpr int LnOperator = 9;
constexpr int LogOperator = 10;
constexpr int SinOperator = 11;
constexpr int SqrtOperator = 12;
constexpr int TanOperator = 13;
constexpr int RealUnaryOperator = 14;
constexpr int UnaryPlusOperator = 15;
constexpr int UnaryMinusOperator = 16;
/** Constants */
constexpr int IntegerToken = 20;
constexpr int RealToken = 21;
constexpr int LengthToken = 22;
constexpr int AreaToken = 23;
constexpr int StringToken = 24;
constexpr int IdentifierToken = 25;
/** Binary arithmetic operators */
constexpr int ExponentiationOperator = 26;
constexpr int MultiplicationOperator = 27;
constexpr int DivisionOperator = 28;
constexpr int AdditionOperator = 29;
constexpr int SubtractionOperator = 30;
/** Binary relational operators are not used */
constexpr int EqualOperator = 31;
constexpr int NotEqualOperator = 32;
constexpr int GreaterThanOperator = 33;
constexpr int GreaterThanOrEqualOperator = 34;
constexpr int LessThanOperator = 35;
constexpr int LessThanOrEqualOperator = 36;
/** Binary logical operators */
constexpr int AndOperator = 37;
constexpr int OrOperator = 38;
/** Unary logical operator */
constexpr int NotOperator = 39;
/** Parentheses */
constexpr int LeftParenthesis = 40;
constexpr int RightParenthesis = 41;

/** Static array of TokenProperties structures that defines a mapping of various tokens, their precedence, and types used in a lexical analysis context, with a total of 42 entries. */
static TokenProperties TokenPropertiesTable[] = {
    {0, 0, Other},                       // unused
    {110, 85, Other},                    // abs
    {110, 85, Other},                    // acos
    {110, 85, Other},                    // asin
    {110, 85, Other},                    // atan
    {110, 85, Other},                    // string
    {110, 85, Other},                    // cos
    {110, 85, Other},                    // exp
    {110, 85, Other},                    // int
    {110, 85, Other},                    // ln
    {110, 85, Other},                    // log
    {110, 85, Other},                    // sin
    {110, 85, Other},                    // sqrt
    {110, 85, Other},                    // tan
    {110, 85, Other},                    // real
    {110, 85, Other},                    // unary+
    {110, 85, Other},                    // unary-
    {0, 0, Other},                       // unused
    {0, 0, Other},                       // unused
    {0, 0, Other},                       // unused
    {0, 0, Constant},                    // integer
    {0, 0, Constant},                    // real
    {0, 0, Constant},                    // length
    {0, 0, Constant},                    // area
    {0, 0, Constant},                    // string
    {0, 0, Identifier},                  // identifier
    {80, 79, BinaryArithmeticOperator},  // **
    {70, 71, BinaryArithmeticOperator},  // *
    {70, 71, BinaryArithmeticOperator},  // /
    {60, 61, BinaryArithmeticOperator},  // +
    {60, 61, BinaryArithmeticOperator},  // -
    {40, 41, BinaryRelationalOperator},  // ==
    {40, 41, BinaryRelationalOperator},  // !=
    {40, 41, BinaryRelationalOperator},  // >
    {40, 41, BinaryRelationalOperator},  // >=
    {40, 41, BinaryRelationalOperator},  // <
    {40, 41, BinaryRelationalOperator},  // <=
    {20, 21, BinaryLogicOperator},       // &
    {10, 11, BinaryLogicOperator},       // |
    {30, 31, UnaryLogicOperator},        // !
    {110, 1, OpenParentheses},           // (
    {0, 0, CloseParentheses}             // )
};

///////////////// Token Processing /////////////////////

/** @brief Processes a stream of tokens, evaluating expressions and performing operations based on their types, while managing an operand stack and handling various token classes such as identifiers, constants, and operators.
 *
 * @param aiTokId Array of token IDs representing the expression.
 * @param operandDefinition Pointer to store the definition (dimension and length) of the result.
 * @param operandType Pointer to store the type of the resulting value.
 * @param operandBuffer Buffer to store the resulting value.
 */
void EvalTokenStream(int* aiTokId, long* operandDefinition, int* operandType, void* operandBuffer);

/** @brief Processes a sequence of tokens starting from a specified location, categorizing them into types and handling operator precedence, while managing parentheses and throwing errors for unbalanced expressions or syntax issues.
* @param firstTokenLocation (in)  location of first token in stream to consider. (out) location of first token not part of expression
* @param numberOfTokens (out) number of tokens on stack
* @param typeOfTokens (out) type of tokens on stack
* @param locationOfTokens (out) location of tokens on stack
*/
void BreakExpression(int& firstTokenLocation, int& numberOfTokens, int* typeOfTokens, int* locationOfTokens);

/** @brief Converts a string representation of a value to its internal representation.
* @param valueType (in) type of value
* @param valueDefinition (in) dimension (lo word) and length (hi word) of string
* @param valueText (in) string to convert
* @param resultDefinition (out) dimension (lo word) and length (hi word) of result
* @param resultValue (out) result
* @note Assumes that a valid literal user units string is passed with no suffix characters evaluated.
*/
void ConvertStringToVal(int tokenType, long tokenDefinition, LPWSTR token, long* resultDefinition, void* resultValue);

/** @brief Performs a unary operation on a value based on the specified operator type.
 *
 * @param operatorType The type of unary operator to apply.
 * @param resultType Pointer to store the resulting type after the operation.
 * @param valueMetaData Pointer to the metadata of the value (dimension and length).
 * @tparam value Pointer to the value to be processed.
 */
template <typename T>
void UnaryOp(int operatorType, int* resultType, long* valueMetaData, T* value);

/** @brief Retrieves the token type identifier for a given token type.
 *
 * @param tokenType The token type to look up.
 * @return The corresponding token type identifier, or -1 if the token type is invalid.
 */
int TokType(int tokenType);

/** @brief Does value type conversion
 * @param currentType current type of value
 * @param requiredType required type of value
 * @param valueDefinition definition of value (dimension and length)
 * @param valueBuffer buffer containing value to convert; on return contains converted value
 */
void ConvertValTyp(int currentType, int requiredType, long* valueDefinition, void* valueBuffer);

/** @brief Converts an internal representation of a value to its string representation.
 * @param valueBuffer buffer containing value to convert
 * @param columnDefinition definition of value (dimension and length)
 * @param valueOfString output buffer for string representation
 * @param lengthOfString (out) length of resulting string
 */
void ConvertValToString(void* valueBuffer, ColumnDefinition* columnDefinition, wchar_t* valueAsString,
                        int* lengthOfString);

///////////////// Parse and dependencies /////////////////////

/** @brief Processes a wide-character input line to tokenize it, categorizing each token and storing relevant values in predefined arrays while managing the number of tokens and values encountered.
 * @param inputLine The wide-character input line to parse.
 */
void Parse(const wchar_t* inputLine);

/** @brief Scans the input line for the next token starting at linePosition.
 *
 * @param token The scanned token.
 * @param inputLine The input line to scan.
 * @param linePosition The current position in the input line; updated to the position after the scanned token.
 * @return The token ID of the scanned token, or -1 if no valid token is found.
 */
int Scan(wchar_t* token, const wchar_t* inputLine, int& linePosition);

void ParseStringOperand(wchar_t* token);

/** @brief Skips leading whitespace characters in the input line.
 *
 * @param inputLine The input line to process.
 * @return A pointer to the first non-whitespace character in the input line.
 */
inline wchar_t* SkipWhiteSpace(wchar_t* inputLine) noexcept {
  if (!inputLine) return inputLine;
  while (*inputLine && std::iswspace(static_cast<wint_t>(*inputLine))) { ++inputLine; }
  return inputLine;
}

/** @brief Scans the line buffer for a specific character, skipping whitespace.
  * @param character The character to scan for.
 * @param lineBuffer A pointer to the lineBuffer to scan; updated to point after the found character.
 * @return A pointer to the found character in the string, or nullptr if not found.
 */
inline wchar_t* ScanForChar(wchar_t character, wchar_t** lineBuffer) noexcept {
  if (!lineBuffer || !*lineBuffer) return nullptr;
  wchar_t* position = SkipWhiteSpace(*lineBuffer);
  if (!position) return nullptr;
  if (*position == character) {
    *lineBuffer = position + 1;
    return position;
  }
  return nullptr;  // not found
}

/** @brief Scans a wide character string for a sequence of characters, handling quoted strings and special characters, and updates the provided pointers to reflect the new positions in the input and output buffers.
 *
 * @param ppStr Pointer to the current scan pointer; updated to the character after the scanned token.
 * @param pszTerm Output character that terminated the scan (e.g. ',', ')' ).
 * @param ppArgBuf Pointer to an argument buffer pointer; scanned characters are written into *ppArgBuf and the pointer is updated to the end of the written data. Returns pointer to the start of the stored string inside the argument buffer, or nullptr on failure.
 */
wchar_t* ScanForString(wchar_t** ppStr, wchar_t* pszTerm, wchar_t** ppArgBuf);

extern long lValues[lex::MaxValues];

}  // namespace lex
