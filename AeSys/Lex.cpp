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
/**
   * @brief Promotes an integer value to a real (double) in-place.
   *
   * This function converts the internal representation of an integer value to a real (double) value
   * directly within the provided value buffer. It also updates the associated metadata to reflect
   * the new type.
   *
   * @param valueMetaData Pointer to the long integer defining the value's metadata (dimension and length).
   * @param valueBuffer Pointer to the buffer containing the value to be promoted.
   * @param resultType Pointer to an integer that will be updated to indicate the new type (RealToken).
   */
inline void PromoteIntegerToReal(long* valueMetaData, void* valueBuffer, int* resultType) noexcept {
  // Convert internal integer representation to real in-place and update metadata
  lex::ConvertValTyp(lex::IntegerToken, lex::RealToken, valueMetaData, valueBuffer);
  *resultType = lex::RealToken;
}

}  // anonymous namespace

namespace lex {
int tokenTypeIdentifiers[lex::MaxTokens];
int valueLocation[lex::MaxTokens];
int numberOfTokensInStream;
int numberOfValues;
long lValues[lex::MaxValues];

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
      if (!(*pIn == L'_' || *pIn == L'$' || *pIn == L'.' || *pIn == L'-' || *pIn == L':' || *pIn == L'\\'))
        break;
    }

    // handle escaped double-quote inside quoted string
    if ((*pIn == L'"') && (*(pIn + 1) == L'"')) {
      ++pIn;  // skip the escaping first quote
    }

    // handle escaped backslash
    if (*pIn == L'\\' && *(pIn + 1) == L'\\') {
      ++pIn;
    }

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

} // namespace lex

void lex::BreakExpression(int& firstTokenLocation, int& numberOfTokens, int* typeOfTokens, int* locationOfTokens) {
  int NumberOfOpenParentheses{0};
  int PreviousTokenType{0};

  int OperatorStack[32]{};
  int TopOfOperatorStack{1};

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
      longInterpretedBuffer[0] = _wtoi(reinterpret_cast<wchar_t*>(buffer));
      *valueDefinition = MAKELONG(1, 1);
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
  WCHAR szTok[256]{};

  int iDim{0};
  int iTyp{0};

  long lDef1 = MAKELONG(1, 1);
  int iDim1{0};
  int iLen1{0};
  int iTyp1{lex::IntegerToken};

  long lDef2;
  int iDim2{0};
  int iLen2{0};
  int iTyp2{0};

  int numberOfTokens{0};
  int iExprTokTyp[32]{};
  int iExprTokLoc[32]{};

  BreakExpression(*aiTokId, numberOfTokens, iExprTokTyp, iExprTokLoc);

  int operandStack[32]{};
  long lOpStk[32][32]{};
  long lOpStkDef[32]{};

  double* dOp1 = reinterpret_cast<double*>(operandBuffer);
  long* lOp1 = reinterpret_cast<long*>(operandBuffer);

  /**
   * @brief Zero-initialized buffer for operand storage, sized for 256 wchar_t but aligned for numeric reinterpretation.
   *
   * This buffer supports type punning for mixed-use in expression parsing (e.g., strings, reals, or ints per grammar).
   * It uses a byte array for flexibility, with reinterpret_cast to view as double or long arrays.
   * Alignment ensures no undefined behavior on access; size calculation adapts to platform-specific wchar_t sizes.
   *
   * @note if not needing char-specific operations. For even safer modernization, explore std::variant or unions for operands.
   */
  alignas(double) std::byte secondOperandBuffer[256 * sizeof(wchar_t)]{};
  wchar_t* cOp2 = reinterpret_cast<wchar_t*>(secondOperandBuffer);
  double* dOp2 = reinterpret_cast<double*>(secondOperandBuffer);
  long* lOp2 = reinterpret_cast<long*>(secondOperandBuffer);

  int operandStackTop{0};  // Empty operand stack
  int iTokStkId{0};        // Start with first token

  while (iTokStkId < numberOfTokens) {
    int tokenType = iExprTokTyp[iTokStkId];
    int iTokLoc = iExprTokLoc[iTokStkId];
    if (TokenPropertiesTable[tokenType].tokenClass == Identifier) {
      // symbol table stuff if desired
      throw L"Identifier token class not implemented";
    } else if (TokenPropertiesTable[tokenType].tokenClass == Constant) {
      iTyp1 = tokenType;
      lDef1 = lex::lValues[lex::valueLocation[iTokLoc]];
      memcpy(operandBuffer, &lex::lValues[lex::valueLocation[iTokLoc] + 1], static_cast<size_t>(HIWORD(lDef1) * 4));
    } else {  // Token is an operator .. Pop an operand from operand stack
      if (operandStackTop == 0) { throw L"Operand stack is empty"; }

      iTyp1 = operandStack[operandStackTop];
      lDef1 = lOpStkDef[operandStackTop];
      iLen1 = HIWORD(lDef1);
      memcpy(operandBuffer, &lOpStk[operandStackTop--][0], static_cast<size_t>(iLen1 * 4));

      if (TokenPropertiesTable[tokenType].tokenClass == Other) {  // intrinsics and oddball unary minus/plus
        if (iTyp1 == lex::StringToken) {
          iDim1 = LOWORD(lDef1);
          wcscpy_s(szTok, 256, reinterpret_cast<wchar_t*>(operandBuffer));
          if (tokenType == lex::IntUnaryOperator) {
            iTyp1 = lex::IntegerToken;
            ConvertStringToVal(lex::IntegerToken, lDef1, szTok, &lDef1, operandBuffer);
          } else if (tokenType == lex::RealToken) {
            iTyp1 = lex::RealToken;
            ConvertStringToVal(lex::RealToken, lDef1, szTok, &lDef1, operandBuffer);
          } else if (tokenType == lex::StringToken) {
            ;
          } else {
            throw L"String operand conversions error: unknown";
          }
        } else if (iTyp1 == lex::IntegerToken) {
          UnaryOp(tokenType, &iTyp1, &lDef1, lOp1);
        } else {
          UnaryOp(tokenType, &iTyp1, &lDef1, dOp1);
        }
      } else if (TokenPropertiesTable[tokenType].tokenClass ==
                 BinaryArithmeticOperator) {  // Binary arithmetic operator
        if (operandStackTop == 0) { throw L"Binary Arithmetic: Only one operand."; }
        iTyp2 = operandStack[operandStackTop];  // Pop second operand from operand stack
        lDef2 = lOpStkDef[operandStackTop];
        iLen2 = HIWORD(lDef2);
        memcpy(secondOperandBuffer, &lOpStk[operandStackTop--][0], static_cast<size_t>(iLen2 * 4));
        iTyp = std::min(iTyp2, lex::RealToken);
        if (iTyp1 < iTyp) {  // Convert first operand
          ConvertValTyp(iTyp1, iTyp, &lDef1, lOp1);
          iTyp1 = iTyp;
          iLen1 = HIWORD(lDef1);
        } else {
          iTyp = std::min(iTyp1, lex::RealToken);
          if (iTyp2 < iTyp) {  // Convert second operand
            ConvertValTyp(iTyp2, iTyp, &lDef2, lOp2);
            iTyp2 = iTyp;
            iLen2 = HIWORD(lDef2);
          }
        }
        if (tokenType == lex::AdditionOperator) {
          if (iTyp1 == lex::StringToken) {
            iDim1 = LOWORD(lDef1);
            iDim2 = LOWORD(lDef2);
            iDim = iDim2 + iDim1;

            errno_t err = wcscat_s(cOp2, 256, reinterpret_cast<wchar_t*>(operandBuffer));
            if (err != 0) { throw L"String concatenation overflow!"; }

            wcscpy_s(reinterpret_cast<wchar_t*>(operandBuffer), static_cast<size_t>(HIWORD(lDef1) * 4), cOp2);
            iLen1 = 1 + (iDim - 1) / 4;
            lDef1 = MAKELONG(iDim, iLen1);
          } else {
            if (iTyp1 == lex::IntegerToken) {
              lOp1[0] += lOp2[0];
            } else {
              dOp1[0] += dOp2[0];
            }
          }
        } else if (tokenType == lex::SubtractionOperator) {
          if (iTyp1 == lex::StringToken) { throw L"Can not subtract strings"; }
          if (iTyp1 == lex::IntegerToken) {
            lOp1[0] = lOp2[0] - lOp1[0];
          } else {
            dOp1[0] = dOp2[0] - dOp1[0];
          }
        } else if (tokenType == lex::MultiplicationOperator) {
          if (iTyp1 == lex::StringToken) { throw L"Can not mutiply strings"; }
          if (iTyp1 == lex::IntegerToken) {
            lOp1[0] *= lOp2[0];
          } else {
            if (iTyp1 == lex::RealToken) {
              iTyp1 = iTyp2;
            } else if (iTyp2 == lex::RealToken) {
              ;
            } else if (iTyp1 == lex::LengthToken && iTyp2 == lex::LengthToken) {
              iTyp1 = lex::AreaToken;
            } else {
              throw L"Invalid mix of multiplicands";
            }

            dOp1[0] *= dOp2[0];
          }
        } else if (tokenType == lex::DivisionOperator) {
          if (iTyp1 == lex::StringToken) { throw L"Can not divide strings"; }
          if (iTyp1 == lex::IntegerToken) {
            if (lOp1[0] == 0) { throw L"Attempting to divide by 0"; }
            lOp1[0] = lOp2[0] / lOp1[0];
          } else if (iTyp1 <= iTyp2) {
            if (dOp1[0] == 0.0) { throw L"Attempting to divide by 0."; }
            if (iTyp1 == iTyp2) {
              iTyp1 = lex::RealToken;
            } else if (iTyp1 == lex::RealToken) {
              iTyp1 = iTyp2;
            } else {
              iTyp1 = lex::LengthToken;
            }
            dOp1[0] = dOp2[0] / dOp1[0];
          } else {
            throw L"Division type error";
          }
        } else if (tokenType == lex::ExponentiationOperator) {
          if (iTyp1 == lex::IntegerToken) {
            if ((lOp1[0] >= 0 && lOp1[0] > DBL_MAX_10_EXP) || (lOp1[0] < 0 && lOp1[0] < DBL_MIN_10_EXP)) {
              throw L"Exponentiation error";
            }

            lOp1[0] = (int)pow((double)lOp2[0], lOp1[0]);
          } else if (iTyp1 == lex::RealToken) {
            int iExp = (int)dOp1[0];

            if ((iExp >= 0 && iExp > DBL_MAX_10_EXP) || (iExp < 0 && iExp < DBL_MIN_10_EXP)) {
              throw L"Exponentiation error";
            }
            dOp1[0] = pow(dOp2[0], dOp1[0]);
          }
        }
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
    operandStackTop++;                      // Increment opernad stack pointer
    operandStack[operandStackTop] = iTyp1;  // Push operand onto operand stack
    lOpStkDef[operandStackTop] = lDef1;
    memcpy(&lOpStk[operandStackTop][0], operandBuffer, static_cast<size_t>(HIWORD(lDef1) * 4));
    iTokStkId++;
  }
  *operandType = iTyp1;
  *operandDefinition = lDef1;
}

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
    double dVal{0.0};

    switch (tokenId) {
      case lex::IdentifierToken:
        iDim = static_cast<int>(wcslen(token));
        iLen = 1 + (iDim - 1) / 4;

        lex::valueLocation[lex::numberOfTokensInStream] = lex::numberOfValues + 1;
        lex::lValues[lex::numberOfValues + 1] = iDim + iLen * 65536;
        memcpy(&lex::lValues[lex::numberOfValues + 2], token, static_cast<size_t>(iDim));
        lex::numberOfValues = lex::numberOfValues + 1 + iLen;
        break;

      case lex::StringToken:
        ParseStringOperand(token);
        break;

      case lex::IntegerToken:
        lex::valueLocation[lex::numberOfTokensInStream] = lex::numberOfValues;
        lex::lValues[lex::numberOfValues++] = MAKELONG(1, 1);
        lex::lValues[lex::numberOfValues++] = _wtoi(token);
        break;

      case lex::RealToken:
      case lex::LengthToken:
        dVal = (tokenId == lex::RealToken) ? _wtof(token) : app.ParseLength(token);

        lex::valueLocation[lex::numberOfTokensInStream] = lex::numberOfValues;
        lex::lValues[lex::numberOfValues++] = MAKELONG(1, 2);
        memcpy(&lex::lValues[lex::numberOfValues++], &dVal, sizeof(double));
        lex::numberOfValues++;
        break;
    }
    lex::numberOfTokensInStream++;
  }
}

void lex::ParseStringOperand(wchar_t* token) {
  if (wcslen(token) < 3) {
    app.AddStringToMessageList(IDS_MSG_ZERO_LENGTH_STRING);
    return;
  }

  auto values = (LPWSTR)&lex::lValues[lex::numberOfValues + 2];

  int iDim{0};
  int next{1};
  while (token[next] != '\0') {
    if (token[next] == '"' && token[next + 1] == '"') { next++; }
    values[iDim++] = token[next++];
  }
  values[--iDim] = '\0';
  int iLen = 1 + (iDim - 1) / 4;
  lex::valueLocation[lex::numberOfTokensInStream] = ++lex::numberOfValues;
  lex::lValues[lex::numberOfValues] = MAKELONG(iDim, iLen);
  lex::numberOfValues += iLen;
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
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"LinePosition = %d, TokenID = %d\n", linePosition, tokenId);
  if (tokenId == -1) { linePosition = beginPosition + 1; }
  return (tokenId);
}

int lex::TokType(int tokenType) {
  return (tokenType >= 0 && tokenType < numberOfTokensInStream) ? tokenTypeIdentifiers[tokenType] : -1;
}

template <typename T>
void lex::UnaryOp(int operatorType, int* resultType, long* valueMetaData, T* value) {
  ColumnDefinition columnDefinition{};

  int iDim = LOWORD(*valueMetaData);
  int iLen = HIWORD(*valueMetaData);

  double* doubleValue = reinterpret_cast<double*>(value);

  switch (operatorType) {
    case lex::UnaryMinusOperator:
      value[0] = -value[0];
      break;

    case lex::UnaryPlusOperator:
      break;

    case lex::AbsOperator:
      if constexpr (std::is_same_v<T, double>) {
        value[0] = fabs(value[0]);
      } else if constexpr (std::is_same_v<T, long>) {
        value[0] = labs(value[0]);
      }
      break;

    case lex::AcosOperator:
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      if (fabs(value[0]) > 1.0) {
        throw L"Math error: acos value < -1. or > 1.";
      } else {
        // compute acos in radians then convert result to degrees
        doubleValue[0] = Eo::RadianToDegree(acos(doubleValue[0]));
      }
      break;

    case lex::AsinOperator:
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      if (fabs(value[0]) > 1.0) {
        throw L"Math error: asin value < -1. or > 1.";
      } else {
        // compute asin in radians then convert result to degrees
        doubleValue[0] = Eo::RadianToDegree(asin(doubleValue[0]));
      }
      break;

    case lex::AtanOperator:
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      // compute atan in radians then convert result to degrees
      doubleValue[0] = Eo::RadianToDegree(atan(doubleValue[0]));
      break;

    case lex::CosOperator:
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      doubleValue[0] = cos(Eo::DegreeToRadian(doubleValue[0]));
      break;

    case lex::ExpOperator:  // exponential function (e^x)
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      doubleValue[0] = exp(doubleValue[0]);
      break;

    case lex::IntUnaryOperator:  // Conversion to integer
      if constexpr (std::is_same_v<T, double>) {
        ConvertValTyp(lex::RealToken, lex::IntegerToken, valueMetaData, reinterpret_cast<void*>(value));
        *resultType = lex::IntegerToken;
      }
      break;

    case lex::LnOperator:
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      if (value[0] <= 0.0) {
        throw L"Math error: ln of a non-positive number";
      } else {
        doubleValue[0] = log(doubleValue[0]);
      }
      break;

    case lex::LogOperator:
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      if (value[0] <= 0.0) {
        throw L"Math error: log of a non-positive number";
      } else {
        doubleValue[0] = log10(doubleValue[0]);
      }
      break;

    case lex::RealUnaryOperator:  // Conversion to real (double)
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      break;

    case lex::SinOperator:
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      doubleValue[0] = sin(Eo::DegreeToRadian(doubleValue[0]));
      break;

    case lex::SqrtOperator:
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      if (value[0] < 0.0) {
        throw L"Math error: sqrt of a negative number";
      } else {
        doubleValue[0] = sqrt(doubleValue[0]);
      }
      break;

    case lex::TanOperator:
      if constexpr (std::is_same_v<T, long>) {
        PromoteIntegerToReal(valueMetaData, reinterpret_cast<void*>(value), resultType);
      }
      doubleValue[0] = tan(Eo::DegreeToRadian(doubleValue[0]));
      break;

    case lex::StringUnaryOperator: {  // Conversion to string
      wchar_t valueAsString[32]{};
      *resultType = lex::StringToken;
      columnDefinition.dataDefinition = *valueMetaData;
      if constexpr (std::is_same_v<T, long>) {
        columnDefinition.dataType = lex::IntegerToken;
      } else if constexpr (std::is_same_v<T, double>) {
        columnDefinition.dataType = lex::RealToken;
      }
      ConvertValToString((wchar_t*)value, &columnDefinition, valueAsString, &iDim);
      iLen = 1 + (iDim - 1) / 4;
      wcscpy_s((wchar_t*)value, 32, valueAsString);
      *valueMetaData = MAKELONG(iDim, iLen);
      break;
    }
    default:
      throw std::logic_error("Unknown operation");
  }
}
