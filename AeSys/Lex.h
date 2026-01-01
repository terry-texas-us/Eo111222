#pragma once
#include <Windows.h>

enum TokenClass {
  PlaceHolderForZero,
  Other,
  Constant,
  Identifier,
  BinaryArithmeticOperator,
  OpenParentheses,
  CloseParentheses
};

/** @brief Structure defining a value's definition (int16_t dimension and int_16_t length packed into a long) and type.
 */
struct ValueMetaInformation {
  long definition;
  long type;

  constexpr int GetDimension() const noexcept { return LOWORD(definition); }
  constexpr int GetLength() const noexcept { return HIWORD(definition); }
};

/** @brief Structure defining token properties such as precedence and type.
 */
struct TokenProperties {
  int inComingPriority;
  int inStackPriority;
  TokenClass tokenClass;
};

namespace lex {

constexpr int MaxTokens = 128;
constexpr int MaxValues = 256;

constexpr int AbsoluteValue = 1;
constexpr int ArcCosine = 2;
constexpr int ArcSine = 3;
constexpr int ArcTangent = 4;
constexpr int ToString = 5;
constexpr int Cosine = 6;
constexpr int ExponentialValue = 7;
constexpr int ToInt = 8;
constexpr int NaturalLogarithm = 9;
constexpr int Base10Logarithm = 10;
constexpr int Sine = 11;
constexpr int SquareRoot = 12;
constexpr int Tangent = 13;
constexpr int ToReal = 14;
constexpr int UnaryPlus = 15;
constexpr int UnaryMinus = 16;
constexpr int IntegerToken = 17;
constexpr int RealToken = 18;
constexpr int ArchitecturalUnitsLengthToken = 19;
constexpr int EngineeringUnitsLengthToken = 20;
constexpr int SimpleUnitsLengthToken = 21;
constexpr int StringToken = 22;
constexpr int IdentifierToken = 23;
constexpr int ExponentiateToken = 24;
constexpr int MultiplyToken = 25;
constexpr int DivideToken = 26;
constexpr int BinaryAddToken = 27;
constexpr int BinarySubtractToken = 28;
constexpr int OpenParenthesesToken = 29;
constexpr int CloseParenthesesToken = 30;

//* Static array of TokenProperties structures that defines a mapping of various tokens, their precedence, and types used in a lexical analysis context. */
static TokenProperties TokenPropertiesTable[] = {
    {0, 0, PlaceHolderForZero},          // unused
    {110, 85, Other},                    // AbsoluteValue
    {110, 85, Other},                    // ArcCosine
    {110, 85, Other},                    // ArcSine
    {110, 85, Other},                    // ArcTangent
    {110, 85, Other},                    // ToString
    {110, 85, Other},                    // Cosine
    {110, 85, Other},                    // ExponentialValue
    {110, 85, Other},                    // ToInt
    {110, 85, Other},                    // NaturalLogarithm
    {110, 85, Other},                    // Base10Logarithm
    {110, 85, Other},                    // Sine
    {110, 85, Other},                    // SquareRoot
    {110, 85, Other},                    // Tangent
    {110, 85, Other},                    // ToReal
    {110, 85, Other},                    // UnaryPlus
    {110, 85, Other},                    // UnaryMinus
    {0, 0, Constant},                    // IntegerToken
    {0, 0, Constant},                    // RealToken
    {0, 0, Constant},                    // ArchitecturalUnitsLengthToken
    {0, 0, Constant},                    // EngineeringUnitsLengthToken
    {0, 0, Constant},                    // SimpleUnitsLengthToken
    {0, 0, Constant},                    // StringToken
    {0, 0, Identifier},                  // IdentifierToken
    {80, 79, BinaryArithmeticOperator},  // ExponentiateToken
    {70, 71, BinaryArithmeticOperator},  // MultiplyToken
    {70, 71, BinaryArithmeticOperator},  // DivideToken
    {60, 61, BinaryArithmeticOperator},  // BinaryAddToken
    {60, 61, BinaryArithmeticOperator},  // BinarySubtractToken
    {110, 1, OpenParentheses},           // OpenParenthesesToken
    {0, 0, CloseParentheses}             // CloseParenthesesToken
};

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

/** @brief Converts an internal representation of a value to its string representation.
 * @param valueBuffer buffer containing value to convert
 * @param valueMetaInformation definition of value (dimension and length)
 * @param[out] stringBuffer output buffer for string representation
 * @param[out] stringLength length of resulting string
 */
void ConvertValToString(void* valueBuffer, ValueMetaInformation* valueMetaInformation, wchar_t* stringbuffer,
                        int* stringLength);

/** @brief Does value type conversion
 * @param currentType current type of value
 * @param requiredType required type of value
 * @param valueDefinition definition of value (dimension and length)
 * @param valueBuffer buffer containing value to convert; on return contains converted value
 */
void ConvertValTyp(int currentType, int requiredType, long* valueDefinition, void* valueBuffer);

/** @brief Processes a stream of tokens, evaluating expressions and performing operations based on their types, while managing an operand stack and handling various token classes such as identifiers, constants, and operators.
 *
 * @param aiTokId Array of token IDs representing the expression.
 * @param operandDefinition Pointer to store the definition (dimension and length) of the result.
 * @param operandType Pointer to store the type of the resulting value.
 * @param operandBuffer Buffer to store the resulting value.
 */
void EvalTokenStream(int* aiTokId, long* operandDefinition, int* operandType, void* operandBuffer);

/** @brief Processes a wide-character input line to tokenize it, categorizing each token and storing relevant values in predefined arrays while managing the number of tokens and values encountered.
 * @param inputLine The wide-character input line to parse.
 */
void Parse(const wchar_t* inputLine);

void ParseStringOperand(wchar_t* token);

/** @brief Scans the line buffer for a specific character, skipping whitespace.
  * @param character The character to scan for.
 * @param lineBuffer A pointer to the lineBuffer to scan; updated to point after the found character.
 * @return A pointer to the found character in the string, or nullptr if not found.
 */
wchar_t* ScanForChar(wchar_t character, wchar_t** lineBuffer);

/** * @brief Scans the line buffer for a string token, handling quotes and escapes.
 *
 * @param ppStr A pointer to the current position in the line buffer; updated to point after the scanned string.
 * @param pszTerm A pointer to store the terminating character after the string.
 * @param ppArgBuf A pointer to the argument buffer to store the scanned string; updated to point to the next free position.
 * @return A pointer to the start of the scanned string in the argument buffer.
 */
wchar_t* ScanForString(wchar_t** ppStr, wchar_t* pszTerm, wchar_t** ppArgBuf);

/** @brief Scans the input line for the next token, skipping whitespace and returning the token ID and updating the line position.
 *
 * @param token A buffer to store the scanned token.
 * @param inputLine The input line to scan.
 * @param linePosition A reference to the current position in the line; updated to point after the scanned token.
 * @return The token ID of the scanned token, or -1 if no token is found.
 */
int Scan(wchar_t* token, const wchar_t* inputLine, int& linePosition);

/** @brief Skips whitespace characters in the input string.
 *
 * @param inputLine The input string to process.
 * @return A pointer to the first non-whitespace character in the string.
 */
wchar_t* SkipWhiteSpace(wchar_t* inputLine);

int TokType(int tokenType);

void UnaryOp(int, int*, long*, double*);

void UnaryOp(int, int*, long*, long*);
}  // namespace lex
