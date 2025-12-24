#pragma once

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

const int TOK_ABS = 1;
const int TOK_ACOS = 2;
const int TOK_ASIN = 3;
const int TOK_ATAN = 4;
const int TOK_TOSTRING = 5;
const int TOK_COS = 6;
const int TOK_EXP = 7;
const int TOK_TOINTEGER = 8;
const int TOK_LN = 9;
const int TOK_LOG = 10;
const int TOK_SIN = 11;
const int TOK_SQRT = 12;
const int TOK_TAN = 13;
const int TOK_TOREAL = 14;
const int TOK_UNARY_PLUS = 15;
const int TOK_UNARY_MINUS = 16;

constexpr int IntegerToken = 20;
constexpr int RealToken = 21;
constexpr int LengthToken = 22;
constexpr int AreaToken = 23;
constexpr int StringToken = 24;
constexpr int IdentifierToken = 25;
const int TOK_EXPONENTIATE = 26;
const int TOK_MULTIPLY = 27;
const int TOK_DIVIDE = 28;
const int TOK_BINARY_PLUS = 29;
const int TOK_BINARY_MINUS = 30;
const int TOK_EQ = 31;
const int TOK_NE = 32;
const int TOK_GT = 33;
const int TOK_GE = 34;
const int TOK_LT = 35;
const int TOK_LE = 36;
const int TOK_AND = 37;
const int TOK_OR = 38;
const int TOK_NOT = 39;
const int TOK_LPAREN = 40;
const int TOK_RPAREN = 41;

//* Static array of TokenProperties structures that defines a mapping of various tokens, their precedence, and types used in a lexical analysis context, with a total of 42 entries. */
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

/** @brief Scans the input line for the next token starting at linePosition.
 *
 * @param token The scanned token.
 * @param inputLine The input line to scan.
 * @param linePosition The current position in the input line; updated to the position after the scanned token.
 * @return The token ID of the scanned token, or -1 if no valid token is found.
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
