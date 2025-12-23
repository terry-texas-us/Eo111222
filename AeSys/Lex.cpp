#include "Stdafx.h"

#include "AeSys.h"
#include "Lex.h"
#include "LexTable.h"
#include "Resource.h"
#include <algorithm>
#include <atltrace.h>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <corecrt.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <wchar.h>

namespace {

/** @brief Convert a lex::Operand into the legacy buffer representation.
 *
 * Converts the provided `op` into the legacy layout used by older code paths,
 * writing raw bytes into `operandBuffer` and returning the legacy definition
 * (LOWORD=dimension or scalar, HIWORD=number of longs) and token type.
 *
 * @param[in] op The lex::Operand to convert.
 * @param[out] operandBuffer Buffer that receives the converted data. Caller must
 *                           ensure it is large enough for the representation
 *                           (for strings: at least (dim+1)*sizeof(wchar_t); for
 *                           double: sizeof(double); for int: sizeof(long)).
 * @param[out] outDef Receives the legacy definition (LOWORD = dimension/length, HIWORD = longs).
 * @param[out] outType Receives the token type identifier (e.g. lex::StringToken).
 *
 * @pre For string operands the caller must provide an `operandBuffer` sized to
 *      hold the string characters plus a terminating wchar_t.
 * @throws std::logic_error if `op` contains an unexpected/unsupported variant.
 * @note The function preserves the legacy `meta` encoding in `outDef` for
 *       compatibility with older consumers.
 */
void OperandToLegacyBuffer(const lex::Operand& operand, void* operandBuffer, long& outDef, int& outType) {
  if (auto ps = std::get_if<std::wstring>(&operand.v)) {
    outType = lex::StringToken;
    const std::wstring& s = *ps;
    int dim = static_cast<int>(s.size());
    int len = 1 + (dim - 1) / 2;
    outDef = MAKELONG(dim, len);
    // copy characters into operandBuffer (as wchar_t)
    wchar_t* destination = reinterpret_cast<wchar_t*>(operandBuffer);
    wcsncpy_s(destination, static_cast<rsize_t>(dim + 1), s.c_str(), static_cast<rsize_t>(dim));
    destination[dim] = L'\0';
  } else if (auto pi = std::get_if<std::int64_t>(&operand.v)) {
    outType = lex::IntegerToken;
    outDef = MAKELONG(1, 1);
    // legacy stores 32-bit long; cast/truncate
    long* destination = reinterpret_cast<long*>(operandBuffer);
    destination[0] = static_cast<long>(*pi);
  } else if (auto pd = std::get_if<double>(&operand.v)) {
    outType = lex::RealToken;
    outDef = MAKELONG(1, 2);
    memcpy(operandBuffer, pd, sizeof(double));
  } else {
    throw std::logic_error("Unknown operand variant type");
  }
}

}  // anonymous namespace

namespace lex {

/** Count of tokens stored in tokenTypeIdentifiers and valueLocation arrays.
 */
int numberOfTokensInStream;

/** Storage for token types and their associated value locations in the expression parser.
 * tokenTypeIdentifiers: Array storing the type identifier for each token.
 * valueLocation: Array storing the index into tokenValues array for each token's associated value.
 */

int tokenTypeIdentifiers[lex::MaxTokens];
/** Storage for value locations corresponding to each token in the expression parser.
 */
int valueLocation[lex::MaxTokens];

/** Count of values stored in tokenValues array.
 */
int numberOfValues;

/** Storage for values associated with tokens in the expression parser.
 * The storage is organized as follows:
 * - For string values: The first long contains the dimension and length (LOWORD = dimension, HIWORD = length),
 *   followed by the actual characters stored as wchar_t.
 * - For integer values: The first long contains the integer value (stored as a 32-bit int).
 * - For real values: The first two longs contain the double value (8 bytes).
 */
long tokenValues[lex::MaxValues];

//////// Parse and dependencies ///////////////////////////////////////////////

void lex::Parse(const wchar_t* inputLine) {
  numberOfTokensInStream = 0;
  numberOfValues = 0;

  wchar_t token[256]{};

  int linePosition{0};
  int lineLength = static_cast<int>(wcslen(inputLine));

  while (linePosition < lineLength) {
    int tokenId = lex::Scan(token, inputLine, linePosition);

    if (tokenId == -1) { return; }
    if (lex::numberOfTokensInStream == lex::MaxTokens) { return; }

    lex::tokenTypeIdentifiers[lex::numberOfTokensInStream] = tokenId;
    int iLen = static_cast<int>(wcslen(token));
    int iDim{0};

    switch (tokenId) {
      case lex::IdentifierToken:
        iDim = static_cast<int>(wcslen(token));
        iLen = 1 + (iDim - 1) / 2;

        lex::valueLocation[lex::numberOfTokensInStream] = lex::numberOfValues + 1;
        lex::tokenValues[lex::numberOfValues + 1] = MAKELONG(iDim, iLen);
        memcpy(&lex::tokenValues[lex::numberOfValues + 2], token, static_cast<size_t>(iDim));
        lex::numberOfValues = lex::numberOfValues + 1 + iLen;
        break;

      case lex::StringToken: {
        if (wcslen(token) < 3) {
          app.AddStringToMessageList(IDS_MSG_ZERO_LENGTH_STRING);
          break;
        }
        auto value = reinterpret_cast<wchar_t*>(&lex::tokenValues[lex::numberOfValues + 2]);

        int next{1};
        while (token[next] != '\0') {
          if (token[next] == '"' && token[next + 1] == '"') { next++; }
          value[iDim++] = token[next++];
        }
        value[--iDim] = '\0';
        iLen = 1 + (iDim - 1) / 2;
        lex::valueLocation[lex::numberOfTokensInStream] = ++lex::numberOfValues;
        lex::tokenValues[lex::numberOfValues] = MAKELONG(iDim, iLen);
        lex::numberOfValues += iLen;
      } break;

      case lex::IntegerToken:
        lex::valueLocation[lex::numberOfTokensInStream] = lex::numberOfValues;
        lex::tokenValues[lex::numberOfValues++] = MAKELONG(1, 1);
        lex::tokenValues[lex::numberOfValues++] = _wtoi(token);
        break;

      case lex::RealToken:
      case lex::LengthToken: {
        double doubleValue = (tokenId == lex::RealToken) ? _wtof(token) : app.ParseLength(token);

        lex::valueLocation[lex::numberOfTokensInStream] = lex::numberOfValues;
        lex::tokenValues[lex::numberOfValues++] = MAKELONG(1, 2);
        memcpy(&lex::tokenValues[lex::numberOfValues++], &doubleValue, sizeof(double));
        lex::numberOfValues++;
        break;
      }
    }
    lex::numberOfTokensInStream++;
  }
}

int lex::Scan(wchar_t* token, const wchar_t* inputLine, int& linePosition) {
  while (inputLine[linePosition] == ' ') { linePosition++; }

  int beginPosition = linePosition;
  int tokenPosition = linePosition;
  int tokenId{-1};
  int scanPosition{1};

  bool done{false};
  while (!done) {
    int address = iBase[scanPosition] + inputLine[linePosition];

    if (iCheck[address] == scanPosition) {
      scanPosition = iNext[address];
      if (iTokVal[scanPosition] != 0) {
        tokenId = iTokVal[scanPosition];
        tokenPosition = linePosition;
      }
      linePosition++;
    } else if (iDefault[scanPosition] != 0) {
      scanPosition = iDefault[scanPosition];
    } else {
      done = true;
    }
  }

  int tokenLength = tokenPosition - beginPosition + 1;
  wcsncpy_s(token, static_cast<rsize_t>(tokenLength + 1), &inputLine[beginPosition], static_cast<rsize_t>(tokenLength));
  token[tokenLength] = '\0';
  if (tokenId == -1) { linePosition = beginPosition + 1; }
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Token `%s` LinePosition = %d, TokenID = %d\n", token, linePosition,
            tokenId);
  return (tokenId);
}


// Scans a command argument for a string or identifier, handling quotes and escape sequences.
wchar_t* ScanForString(wchar_t** ppStr, wchar_t* pszTerm, wchar_t** ppArgBuf) {
  if (!ppStr || !*ppStr || !ppArgBuf || !*ppArgBuf) return nullptr;

  wchar_t* pIn = SkipWhiteSpace(*ppStr);
  wchar_t* pStart = *ppArgBuf;
  wchar_t* pOut = pStart;

  bool bInQuotes = (*pIn == L'"');
  if (bInQuotes) { ++pIn; }

  while (*pIn) {
    if (bInQuotes) {
      if ((*pIn == L'"') && (*(pIn + 1) != L'"')) {  // closing quote (not an escaped quote)
        ++pIn;
        break;
      }
    } else if (iswalnum(*pIn)) {
      // ok
    } else {
      // allow some peg specials when not quoted
      if (!(*pIn == L'_' || *pIn == L'$' || *pIn == L'.' || *pIn == L'-' || *pIn == L':' || *pIn == L'\\')) break;
    }

    // handle escaped double-quote inside quoted string
    if ((*pIn == L'"') && (*(pIn + 1) == L'"')) {
      ++pIn;  // skip the escaping first quote
    }

    // handle escaped backslash
    if (*pIn == L'\\' && *(pIn + 1) == L'\\') { ++pIn; }

    *pOut++ = *pIn++;
  }

  *pOut++ = L'\0';

  *pszTerm = *pIn;
  if (*pIn) {
    *ppStr = pIn + 1;
  } else {
    *ppStr = pIn;
  }

  *ppArgBuf = pOut;  // Update the arg buffer to the next free bit

  return pStart;
}

}  // namespace lex

void lex::BreakExpression(int& firstTokenLocation, int& numberOfTokens, int* typeOfTokens, int* locationOfTokens) {
  int NumberOfOpenParentheses{0};
  int PreviousTokenType{0};

  int OperatorStack[32]{};
  int TopOfOperatorStack{1}; // Operator stack is not zero-based

  OperatorStack[TopOfOperatorStack] = lex::IdentifierToken;

  numberOfTokens = 0;

  int CurrentTokenType = TokType(firstTokenLocation);
  while (CurrentTokenType != -1) {
    switch (TokenPropertiesTable[CurrentTokenType].tokenClass) {
      case Constant:
        typeOfTokens[numberOfTokens] = CurrentTokenType;
        locationOfTokens[numberOfTokens++] = firstTokenLocation;
        break;

      case OpenParentheses:
        OperatorStack[++TopOfOperatorStack] = CurrentTokenType;  // Push to operator stack
        NumberOfOpenParentheses++;
        break;

      case CloseParentheses:
        if (NumberOfOpenParentheses == 0) { break; }

        while (OperatorStack[TopOfOperatorStack] != lex::LeftParenthesis) {  // Move operator to token stack
          typeOfTokens[numberOfTokens++] = OperatorStack[TopOfOperatorStack--];
        }
        TopOfOperatorStack--;       // Discard open parentheses
        NumberOfOpenParentheses--;  // One less open parentheses
        break;

      case BinaryArithmeticOperator:
      case Other:
        if (CurrentTokenType == lex::AdditionOperator || CurrentTokenType == lex::SubtractionOperator) {
          TokenClass eClassPrv = TokenPropertiesTable[PreviousTokenType].tokenClass;
          if (eClassPrv != Constant && eClassPrv != Identifier && eClassPrv != CloseParentheses) {
            CurrentTokenType =
                (CurrentTokenType == lex::AdditionOperator) ? lex::UnaryPlusOperator : lex::UnaryMinusOperator;
          }
        }
        // Pop higher priority operators from stack
        while (TopOfOperatorStack > 0 && TokenPropertiesTable[OperatorStack[TopOfOperatorStack]].inStackPriority >=
                                             TokenPropertiesTable[CurrentTokenType].inComingPriority) {
          typeOfTokens[numberOfTokens++] = OperatorStack[TopOfOperatorStack--];
        }
        // Push new operator onto stack
        OperatorStack[++TopOfOperatorStack] = CurrentTokenType;
        break;

      // TODO .. classes of tokens which might be implemented
      case Identifier:
      case BinaryRelationalOperator:
      case BinaryLogicOperator:
      case UnaryLogicOperator:
      case AssignOp:

      default:
        break;
    }
    PreviousTokenType = CurrentTokenType;
    CurrentTokenType = TokType(++firstTokenLocation);
  }
  if (NumberOfOpenParentheses > 0) { throw L"Unbalanced parentheses"; }

  while (TopOfOperatorStack > 1) { typeOfTokens[numberOfTokens++] = OperatorStack[TopOfOperatorStack--]; }

  if (numberOfTokens == 0) { throw L"Syntax error"; }
}

void lex::ConvertValToString(void* valueBuffer, ColumnDefinition* columnDefinition, wchar_t* valueAsString,
                             int* lengthOfString) {
  long tokenType = columnDefinition->dataType;

  if (tokenType == lex::StringToken) {
    *lengthOfString = LOWORD(columnDefinition->dataDefinition);  // length of string
    valueAsString[0] = '\'';
    memmove(&valueAsString[1], valueBuffer, static_cast<size_t>(*lengthOfString));
    valueAsString[++*lengthOfString] = '\'';
    valueAsString[++*lengthOfString] = '\0';
  } else {
    wchar_t cVal[32]{};
    long* lVal = (long*)cVal;
    double* dVal = (double*)cVal;

    if (tokenType == lex::IntegerToken) {
      memcpy(lVal, reinterpret_cast<const std::byte*>(valueBuffer), sizeof(std::int32_t));
      _ltow_s(*lVal, valueAsString, 32, 10);
    } else {
      memcpy(dVal, reinterpret_cast<const std::byte*>(valueBuffer), sizeof(double));
      if (tokenType == lex::RealToken) {
        std::wstring str = std::to_wstring(*dVal);
        memcpy(valueAsString, str.c_str(), str.size() * sizeof(wchar_t));
      } else if (tokenType == lex::LengthToken) {
        //TODO: Length to length string
      } else if (tokenType == lex::AreaToken) {
        // TODO: Area to area string;
      }
    }
    *lengthOfString = static_cast<int>(wcslen(valueAsString));
  }
}

void lex::ConvertValTyp(int currentType, int requiredType, long* valueDefinition, void* buffer) {
  if (currentType == requiredType) { return; }

  double* doubleInterpretedBuffer = reinterpret_cast<double*>(buffer);
  long* longInterpretedBuffer = reinterpret_cast<long*>(buffer);

  if (currentType == lex::StringToken) {
    if (requiredType == lex::IntegerToken) {
      longInterpretedBuffer[0] = static_cast<int64_t>(_wtoi(reinterpret_cast<wchar_t*>(buffer)));
      *valueDefinition = MAKELONG(1, 2);
    } else {
      doubleInterpretedBuffer[0] = _wtof(reinterpret_cast<wchar_t*>(buffer));
      *valueDefinition = MAKELONG(1, 2);
    }
  } else if (currentType == lex::IntegerToken) {
    if (requiredType == lex::StringToken) {
    } else {
      doubleInterpretedBuffer[0] = static_cast<double>(longInterpretedBuffer[0]);
      *valueDefinition = MAKELONG(1, 2);
    }
  } else {  // currentType is double
    if (requiredType == lex::StringToken) {
    } else if (requiredType == lex::IntegerToken) {
    }
  }
}

void lex::ConvertStringToVal(int tokenType, long tokenDefinition, LPWSTR token, long* resultDefinition,
                             void* resultValue) {
  if (LOWORD(tokenDefinition) <= 0) { throw L"Empty string"; }

  WCHAR szTok[64];
  int iNxt = 0;

  int iTyp = Scan(szTok, token, iNxt);
  if (tokenType == lex::IntegerToken) {
    long* pVal = (long*)resultValue;

    if (iTyp == lex::IntegerToken) {
      *pVal = _wtol(szTok);
    } else if (iTyp == lex::RealToken) {
      *pVal = (long)_wtof(szTok);
    } else {
      throw L"String format conversion error";
    }
    *resultDefinition = MAKELONG(1, 1);
  } else {
    double* pVal = (double*)resultValue;

    if (iTyp == lex::IntegerToken) {
      *pVal = (double)_wtoi(szTok);
    } else if (iTyp == lex::RealToken) {
      *pVal = _wtof(szTok);
    } else {
      throw L"String format conversion error";
    }
    *resultDefinition = MAKELONG(1, 2);
  }
}

void lex::EvalTokenStream(int* aiTokId, long* operandDefinition, int* operandType, void* operandBuffer) {
  int numberOfTokens{0};
  int iExprTokTyp[32]{};
  int iExprTokLoc[32]{};

  BreakExpression(*aiTokId, numberOfTokens, iExprTokTyp, iExprTokLoc);

  Operand operandStack[32];
  int operandStackTop{0};  // Empty operand stack (index of top)

  int iTokStkId{0};        // Start with first token

  while (iTokStkId < numberOfTokens) {
    int tokenType = iExprTokTyp[iTokStkId];
    int iTokLoc = iExprTokLoc[iTokStkId];
    if (TokenPropertiesTable[tokenType].tokenClass == Identifier) {
      // symbol table stuff if desired
      throw L"Identifier token class not implemented";
    } else if (TokenPropertiesTable[tokenType].tokenClass == Constant) {
      // Push constant as Operand
      auto valueIndex = lex::valueLocation[iTokLoc];
     
     /**Construct an operand from values in the tokenValues array based on the provided value index and token type. */

     lex::Operand operand;
      operand.meta = lex::tokenValues[valueIndex];
      switch (tokenType) {
        case lex::StringToken: {
          // characters stored starting at tokenValues[valueIndex + 1]
          auto stringBuffer = reinterpret_cast<const wchar_t*>(&lex::tokenValues[valueIndex + 1]);
          operand.v = std::wstring(stringBuffer, static_cast<size_t>(LOWORD(operand.meta)));
          break;
        }
        case lex::IntegerToken: {
          // integer stored in next long (32-bit)
          std::int32_t integerValue = static_cast<std::int32_t>(lex::tokenValues[valueIndex + 1]);
          operand.v = static_cast<std::int64_t>(integerValue);
          break;
        }
        case lex::RealToken:
        case lex::LengthToken:
        default: {
          double doubleBuffer{};
          // double occupies next two longs
          memcpy(&doubleBuffer, &lex::tokenValues[valueIndex + 1], sizeof(double));
          operand.v = doubleBuffer;
          break;
        }
      }
      operandStackTop++;
      operandStack[operandStackTop] = operand;
    } else {  // Token is an operator .. Pop an operand from operand stack
      if (operandStackTop == 0) { throw L"Operand stack is empty"; }

      // Pop right operand
      Operand right = operandStack[operandStackTop];
      operandStackTop--;

      if (TokenPropertiesTable[tokenType].tokenClass == Other) {  // intrinsics and oddball unary minus/plus
        // Handle string conversions first
        if (std::holds_alternative<std::wstring>(right.v)) {
          std::wstring s = std::get<std::wstring>(right.v);
          if (tokenType == lex::IntUnaryOperator || tokenType == lex::RealToken ||
              tokenType == lex::RealUnaryOperator) {
            // convert string to numeric using existing helper
            long newDef{};
            if (tokenType == lex::IntUnaryOperator) {
              long tmpInt{};
              ConvertStringToVal(lex::IntegerToken, right.meta, const_cast<LPWSTR>(s.c_str()), &newDef, &tmpInt);
              Operand res;
              res.v = static_cast<std::int64_t>(tmpInt);
              res.meta = newDef;
              operandStackTop++;
              operandStack[operandStackTop] = res;
            } else {
              double tmpD{};
              ConvertStringToVal(lex::RealToken, right.meta, const_cast<LPWSTR>(s.c_str()), &newDef, &tmpD);
              Operand res;
              res.v = tmpD;
              res.meta = newDef;
              operandStackTop++;
              operandStack[operandStackTop] = res;
            }
          } else if (tokenType == lex::StringToken) {
            // noop
            operandStackTop++;
            operandStack[operandStackTop] = right;
          } else {
            throw L"String operand conversions error: unknown";
          }
        } else {
          long tmpDef = right.meta;
          // use 32-byte local buffer as before
          unsigned char localBuf[256]{};
          void* bufPtr = localBuf;
          int callType = 0;
          OperandToLegacyBuffer(right, bufPtr, tmpDef, callType);

          if (std::holds_alternative<std::int64_t>(right.v)) {
            long tmpLong = static_cast<long>(std::get<std::int64_t>(right.v));
            lex::UnaryOp(tokenType, &right);
            callType = GetTokenType(right.v);

            // pack result back into operand and push
            Operand res;
            res.v = static_cast<std::int64_t>(tmpLong);
            res.meta = tmpDef;
            operandStackTop++;
            operandStack[operandStackTop] = res;
          } else {
            double tmpDouble = std::get<double>(right.v);
            lex::UnaryOp(tokenType, &right);
            callType = GetTokenType(right.v);
            Operand res;
            res.v = tmpDouble;
            res.meta = tmpDef;
            operandStackTop++;
            operandStack[operandStackTop] = res;
          }
        }
      } else if (TokenPropertiesTable[tokenType].tokenClass ==
                 BinaryArithmeticOperator) {  // Binary arithmetic operator
        if (operandStackTop == 0) { throw L"Binary Arithmetic: Only one operand."; }
        // Pop left operand
        Operand left = operandStack[operandStackTop];
        operandStackTop--;

        // Determine operand kinds
        bool leftIsStr = std::holds_alternative<std::wstring>(left.v);
        bool rightIsStr = std::holds_alternative<std::wstring>(right.v);
        bool leftIsInt = std::holds_alternative<std::int64_t>(left.v);
        bool rightIsInt = std::holds_alternative<std::int64_t>(right.v);
        bool leftIsReal = std::holds_alternative<double>(left.v);
        bool rightIsReal = std::holds_alternative<double>(right.v);

        // Result operand
        Operand result;
        int resultType = 0;
        long resultDef = 0;

        if (tokenType == lex::AdditionOperator) {
          if (leftIsStr && rightIsStr) {
            const std::wstring& ls = std::get<std::wstring>(left.v);
            const std::wstring& rs = std::get<std::wstring>(right.v);
            std::wstring concat = ls + rs;
            result.v = concat;
            int dim = static_cast<int>(concat.size());
            int len = 1 + (dim - 1) / 2;
            result.meta = MAKELONG(dim, len);
            resultType = lex::StringToken;
            resultDef = result.meta;
          } else if (leftIsInt && rightIsInt) {
            std::int64_t lv = std::get<std::int64_t>(left.v);
            std::int64_t rv = std::get<std::int64_t>(right.v);
            result.v = lv + rv;
            resultType = lex::IntegerToken;
            resultDef = MAKELONG(1, 2);
            result.meta = resultDef;
          } else {
            // numeric promotion to double
            double ld = leftIsReal ? std::get<double>(left.v) : static_cast<double>(std::get<std::int64_t>(left.v));
            double rd = rightIsReal ? std::get<double>(right.v) : static_cast<double>(std::get<std::int64_t>(right.v));
            result.v = ld + rd;
            resultType = lex::RealToken;
            resultDef = MAKELONG(1, 2);
            result.meta = resultDef;
          }
        } else if (tokenType == lex::SubtractionOperator) {
          if (leftIsStr || rightIsStr) { throw L"Can not subtract strings"; }
          if (leftIsInt && rightIsInt) {
            std::int64_t lv = std::get<std::int64_t>(left.v);
            std::int64_t rv = std::get<std::int64_t>(right.v);
            result.v = lv - rv;
            resultType = lex::IntegerToken;
            resultDef = MAKELONG(1, 2);
            result.meta = resultDef;
          } else {
            double ld = leftIsReal ? std::get<double>(left.v) : static_cast<double>(std::get<std::int64_t>(left.v));
            double rd = rightIsReal ? std::get<double>(right.v) : static_cast<double>(std::get<std::int64_t>(right.v));
            result.v = ld - rd;
            resultType = lex::RealToken;
            resultDef = MAKELONG(1, 2);
            result.meta = resultDef;
          }
        } else if (tokenType == lex::MultiplicationOperator) {
          if (leftIsStr || rightIsStr) { throw L"Can not mutiply strings"; }
          if (leftIsInt && rightIsInt) {
            std::int64_t lv = std::get<std::int64_t>(left.v);
            std::int64_t rv = std::get<std::int64_t>(right.v);
            result.v = lv * rv;
            resultType = lex::IntegerToken;
            resultDef = MAKELONG(1, 2);
            result.meta = resultDef;
          } else {
            double ld = leftIsReal ? std::get<double>(left.v) : static_cast<double>(std::get<std::int64_t>(left.v));
            double rd = rightIsReal ? std::get<double>(right.v) : static_cast<double>(std::get<std::int64_t>(right.v));
            result.v = ld * rd;
            resultType = lex::RealToken;
            resultDef = MAKELONG(1, 2);
            result.meta = resultDef;
          }
        } else if (tokenType == lex::DivisionOperator) {
          if (leftIsStr || rightIsStr) { throw L"Can not divide strings"; }
          if (leftIsInt && rightIsInt) {
            std::int64_t lv = std::get<std::int64_t>(left.v);
            std::int64_t rv = std::get<std::int64_t>(right.v);
            if (rv == 0) { throw L"Attempting to divide by 0"; }
            result.v = lv / rv;
            resultType = lex::IntegerToken;
            resultDef = MAKELONG(1, 2);
            result.meta = resultDef;
          } else {
            double ld = leftIsReal ? std::get<double>(left.v) : static_cast<double>(std::get<std::int64_t>(left.v));
            double rd = rightIsReal ? std::get<double>(right.v) : static_cast<double>(std::get<std::int64_t>(right.v));
            if (rd == 0.0) { throw L"Attempting to divide by 0."; }
            result.v = ld / rd;
            resultType = lex::RealToken;
            resultDef = MAKELONG(1, 2);
            result.meta = resultDef;
          }
        } else if (tokenType == lex::ExponentiationOperator) {
          if (leftIsInt && rightIsInt) {
            std::int64_t exp = std::get<std::int64_t>(right.v);
            double base = static_cast<double>(std::get<std::int64_t>(left.v));
            // be conservative using pow
            double val = pow(base, static_cast<double>(exp));
            result.v = static_cast<std::int64_t>(val);
            resultType = lex::IntegerToken;
            resultDef = MAKELONG(1, 2);
            result.meta = resultDef;
          } else {
            double ld = leftIsReal ? std::get<double>(left.v) : static_cast<double>(std::get<std::int64_t>(left.v));
            double rd = rightIsReal ? std::get<double>(right.v) : static_cast<double>(std::get<std::int64_t>(right.v));
            double val = pow(ld, rd);
            result.v = val;
            resultType = lex::RealToken;
            resultDef = MAKELONG(1, 2);
            result.meta = resultDef;
          }
        }

        // push result
        operandStackTop++;
        operandStack[operandStackTop] = result;

      } else if (TokenPropertiesTable[tokenType].tokenClass == BinaryRelationalOperator) {
        // if support for binary relational operators desired (== != > >= < <=)
        throw L"Binary relational operators not implemented";
      } else if (TokenPropertiesTable[tokenType].tokenClass == BinaryLogicOperator) {
        // if support for binary logical operators desired (& |)
        throw L"Binary logical operators not implemented";
      } else if (TokenPropertiesTable[tokenType].tokenClass == UnaryLogicOperator) {
        // if support for unary logical operator desired (!)
        throw L"Unary logical operator not implemented";
      }
    }
    iTokStkId++;
  }

  if (operandStackTop == 0) { throw L"No result on operand stack"; }
  // Final result is on top
  Operand finalOp = operandStack[operandStackTop];
  long outDef{};
  int outType{};
  OperandToLegacyBuffer(finalOp, operandBuffer, outDef, outType);

  *operandType = outType;
  *operandDefinition = outDef;
}


int lex::TokType(int tokenType) {
  return (tokenType >= 0 && tokenType < numberOfTokensInStream) ? tokenTypeIdentifiers[tokenType] : -1;
}

int lex::GetTokenType(const ValueVariant& v) {
  if (std::holds_alternative<std::int64_t>(v)) return IntegerToken;
  if (std::holds_alternative<double>(v)) return RealToken;
  if (std::holds_alternative<std::wstring>(v)) return StringToken;
  throw std::runtime_error("Unknown variant type");
}

void lex::UnaryOp(int operatorType, Operand* operand) {
  auto new_variant = std::visit(
      [operatorType](auto&& val) -> ValueVariant {
        using U = std::decay_t<decltype(val)>;

        switch (operatorType) {
          case UnaryPlusOperator:
            return val;  // No-op

          case UnaryMinusOperator:
            if constexpr (std::is_same_v<U, std::int64_t>) {
              return -val;
            } else if constexpr (std::is_same_v<U, double>) {
              return -val;
            } else {
              throw std::runtime_error("Unary minus not supported on string");
            }

          case NotOperator:
            if constexpr (std::is_same_v<U, std::int64_t>) {
              return std::int64_t(!val);
            } else if constexpr (std::is_same_v<U, double>) {
              return double(!val);
            } else {
              throw std::runtime_error("Logical not not supported on string");
            }

          case AbsOperator:
            if constexpr (std::is_same_v<U, std::int64_t>) {
              return std::abs(val);
            } else if constexpr (std::is_same_v<U, double>) {
              return std::fabs(val);  // Use fabs for double
            } else {
              throw std::runtime_error("Abs not supported on string");
            }

          case AcosOperator:
            if constexpr (std::is_same_v<U, double>) {
              return std::acos(val);
            } else {
              throw std::runtime_error("Acos requires double");
            }

          case AsinOperator:
            if constexpr (std::is_same_v<U, double>) {
              return std::asin(val);
            } else {
              throw std::runtime_error("Asin requires double");
            }

          case AtanOperator:
            if constexpr (std::is_same_v<U, double>) {
              return std::atan(val);
            } else {
              throw std::runtime_error("Atan requires double");
            }

          case CosOperator:
            if constexpr (std::is_same_v<U, double>) {
              return std::cos(val);
            } else {
              throw std::runtime_error("Cos requires double");
            }

          case ExpOperator:
            if constexpr (std::is_same_v<U, double>) {
              return std::exp(val);
            } else {
              throw std::runtime_error("Exp requires double");
            }

          case IntUnaryOperator:
            if constexpr (std::is_same_v<U, std::int64_t>) {
              return val;
            } else if constexpr (std::is_same_v<U, double>) {
              return static_cast<std::int64_t>(std::floor(val));  // Or trunc/round as per legacy
            } else if constexpr (std::is_same_v<U, std::wstring>) {
              return std::wcstoll(val.c_str(), nullptr, 10);
            }

          case LnOperator:
            if constexpr (std::is_same_v<U, double>) {
              return std::log(val);  // Natural log
            } else {
              throw std::runtime_error("Ln requires double");
            }

          case LogOperator:
            if constexpr (std::is_same_v<U, double>) {
              return std::log10(val);  // Assuming base-10; confirm if natural
            } else {
              throw std::runtime_error("Log requires double");
            }

          case SinOperator:
            if constexpr (std::is_same_v<U, double>) {
              return std::sin(val);
            } else {
              throw std::runtime_error("Sin requires double");
            }

          case SqrtOperator:
            if constexpr (std::is_same_v<U, double>) {
              return std::sqrt(val);
            } else {
              throw std::runtime_error("Sqrt requires double");
            }

          case TanOperator:
            if constexpr (std::is_same_v<U, double>) {
              return std::tan(val);
            } else {
              throw std::runtime_error("Tan requires double");
            }

          case RealUnaryOperator:
            if constexpr (std::is_same_v<U, std::int64_t>) {
              return static_cast<double>(val);
            } else if constexpr (std::is_same_v<U, double>) {
              return val;
            } else if constexpr (std::is_same_v<U, std::wstring>) {
              return std::wcstod(val.c_str(), nullptr);
            }

          case StringUnaryOperator:
            if constexpr (std::is_same_v<U, std::int64_t>) {
              return std::to_wstring(val);
            } else if constexpr (std::is_same_v<U, double>) {
              return std::to_wstring(val);
            } else {
              return val;
            }

          default:
            throw std::runtime_error("Unknown unary operator");
        }
      },
      operand->v);

  operand->v = std::move(new_variant);

  // Update meta based on new type
  if (std::holds_alternative<std::int64_t>(operand->v)) {
    operand->meta = MAKELONG(1, 1);
  } else if (std::holds_alternative<double>(operand->v)) {
    operand->meta = MAKELONG(1, 2);
  } else if (std::holds_alternative<std::wstring>(operand->v)) {
    const auto& str = std::get<std::wstring>(operand->v);
    size_t char_len = str.length();
    size_t byte_len = char_len * sizeof(wchar_t);
    size_t long_len = (byte_len + 3) / 4;  // Padded to longs
    operand->meta = MAKELONG(static_cast<long>(char_len), static_cast<long>(long_len));
  }
}
