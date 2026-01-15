#include "stdafx.h"
#include <algorithm>
#include <atltrace.h>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <corecrt.h>
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <wchar.h>

#include "AeSys.h"
#include "Eo.h"
#include "Lex.h"
#include "Resource.h"
#pragma warning(push)
#pragma warning(disable : 4003 4242 4244 4263 4264 4365 4800)
#include "lex.yy.h"
#pragma warning(pop)

namespace {
constexpr size_t TokenBufferSize = 64;
constexpr int MaxTokens = 128;
constexpr int MaxValues = 256;

int numberOfValues = 0;
int numberOfTokensInStream = 0;

/**
 * @brief Zero-initialized buffer for operand value storage, sized for MaxValues long.
 *
 * This base long buffer supports type punning for mixed-use in expression parsing (e.g.,  long's, double's or wchar_t's per grammar).
 * Each operand entry used first a packed long containing dimension and length, followed by the actual value(s).
 * Use with reinterpret_cast to view as double or wchar_t arrays.
 * Alignment ensures no undefined behavior on access; size calculation adapts to platform-specific wchar_t sizes.
 *
 * @note For modernization, explore std::variant or unions for operands and this buffer.
 */
long lValues[MaxValues]{0};
int valueLocation[MaxTokens];
int tokenTypeIdentifiers[MaxTokens];
}  // namespace

void lex::BreakExpression(int& firstTokenLocation, int& numberOfTokens, int* typeOfTokens, int* locationOfTokens) {
  int NumberOfOpenParentheses{0};
  int PreviousTokenType{0};

  int OperatorStack[32]{};
  int TopOfOperatorStack{1};

  OperatorStack[TopOfOperatorStack] = IdentifierToken;

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

        while (OperatorStack[TopOfOperatorStack] != OpenParenthesesToken) {  // Move operator to token stack
          typeOfTokens[numberOfTokens++] = OperatorStack[TopOfOperatorStack--];
        }
        TopOfOperatorStack--;       // Discard open parentheses
        NumberOfOpenParentheses--;  // One less open parentheses
        break;

      case BinaryArithmeticOperator:
      case Other:
        if (CurrentTokenType == BinaryAddToken || CurrentTokenType == BinarySubtractToken) {
          TokenClass eClassPrv = TokenPropertiesTable[PreviousTokenType].tokenClass;
          if (eClassPrv != Constant && eClassPrv != Identifier && eClassPrv != CloseParentheses) {
            CurrentTokenType = (CurrentTokenType == BinaryAddToken) ? UnaryPlus : UnaryMinus;
          }
        }
        // Pop higher priority operators from stack
        while (TokenPropertiesTable[OperatorStack[TopOfOperatorStack]].inStackPriority >=
               TokenPropertiesTable[CurrentTokenType].inComingPriority) {
          typeOfTokens[numberOfTokens++] = OperatorStack[TopOfOperatorStack--];
        }
        // Push new operator onto stack
        OperatorStack[++TopOfOperatorStack] = CurrentTokenType;
        break;

      // TODO .. classes of tokens which might be implemented
      // (AssignmentOperator, BinaryRelationalOperator, BinaryLogicOperator and UnaryLogicOperator) should go here
      case Identifier:
      case PlaceHolderForZero:

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

void lex::ConvertValToString(void* valueBuffer, ValueMetaInformation* valueMetaInformation, wchar_t* stringBuffer,
                             int* stringLength) {
  int iDim = valueMetaInformation->GetDimension();

  if (valueMetaInformation->type == StringToken) {
    *stringLength = iDim;
    stringBuffer[0] = '\'';
    memmove(&stringBuffer[1], valueBuffer, static_cast<size_t>(*stringLength));
    stringBuffer[++*stringLength] = '\'';
    stringBuffer[++*stringLength] = '\0';
  } else {
    wchar_t cVal[32]{};
    long* lVal = (long*)cVal;
    double* dVal = (double*)cVal;

    wchar_t* szpVal{nullptr};
    int iLoc;

    int iVLen = 0;
    int byteOfset = 0;
    int iLnLoc = 0;
    int iLen = valueMetaInformation->GetLength();

    if (valueMetaInformation->type != IntegerToken) { iLen = iLen / 2; }

    if (iDim != iLen) {  // Matrix
      stringBuffer[0] = '[';
      iLnLoc++;
    }
    for (int i1 = 0; i1 < iLen; i1++) {
      iLnLoc++;
      if (iLen != 1 && (i1 % iDim) == 0) { stringBuffer[iLnLoc++] = '['; }
      if (valueMetaInformation->type == IntegerToken) {
        memcpy(lVal, reinterpret_cast<const std::byte*>(valueBuffer) + byteOfset, 4);
        byteOfset += 4;
        _ltow_s(*lVal, &stringBuffer[iLnLoc], static_cast<size_t>(32 - iLnLoc), 10);
        iVLen = (int)wcslen(&stringBuffer[iLnLoc]);
        iLnLoc += iVLen;
      } else {
        memcpy(dVal, reinterpret_cast<const std::byte*>(valueBuffer) + byteOfset, 8);
        byteOfset += 8;
        if (valueMetaInformation->type == RealToken) {
          iLoc = 1;
          wchar_t* NextToken{nullptr};
          szpVal = wcstok_s(cVal, L" ", &NextToken);
          wcscpy_s(&stringBuffer[iLnLoc], static_cast<size_t>(32 - iLnLoc), szpVal);
          iLnLoc += (int)wcslen(szpVal);
        } else if (valueMetaInformation->type == ArchitecturalUnitsLengthToken) {
          iLnLoc += iVLen;
        } else if (valueMetaInformation->type == EngineeringUnitsLengthToken) {
          iLnLoc += iVLen;
        } else if (valueMetaInformation->type == SimpleUnitsLengthToken) {
          iLnLoc += iVLen;
        }
      }
      if (iLen != 1 && (i1 % iDim) == iDim - 1) { stringBuffer[iLnLoc++] = ']'; }
    }
    if (iDim == iLen) {
      *stringLength = iLnLoc - 1;
    } else {
      stringBuffer[iLnLoc] = ']';
      *stringLength = iLnLoc;
    }
  }
}

void lex::ConvertValTyp(int currentType, int requiredType, long* valueDefinition, void* buffer) {
  if (currentType == requiredType) { return; }

  double* doubleInterpretedBuffer = reinterpret_cast<double*>(buffer);
  long* longInterpretedBuffer = reinterpret_cast<long*>(buffer);

  if (currentType == StringToken) {
    if (requiredType == IntegerToken) {
      longInterpretedBuffer[0] = _wtoi(reinterpret_cast<wchar_t*>(buffer));
      *valueDefinition = MAKELONG(1, 1);
    } else {
      doubleInterpretedBuffer[0] = _wtof(reinterpret_cast<wchar_t*>(buffer));
      *valueDefinition = MAKELONG(1, 2);
    }
  } else if (currentType == IntegerToken) {
    if (requiredType == StringToken) {
    } else {
      doubleInterpretedBuffer[0] = static_cast<double>(longInterpretedBuffer[0]);
      *valueDefinition = MAKELONG(1, 2);
    }
  } else {  // currentType is double
    if (requiredType == StringToken) {
    } else if (requiredType == IntegerToken) {
    }
  }
}

void lex::ConvertStringToVal(int desiredType, long tokenDefinition, const wchar_t* inputLine, long* resultDefinition,
                             void* resultValue) {
  if (LOWORD(tokenDefinition) <= 0) { throw std::invalid_argument("Empty string"); }

  wchar_t token[TokenBufferSize]{0};
  int linePosition = 0;

  int tokenType = Scan(token, inputLine, linePosition);

  auto throwConversionError = []() { throw std::invalid_argument("String format conversion error"); };

  if (desiredType == IntegerToken) {
    long* longInterpretedValue = reinterpret_cast<long*>(resultValue);

    if (tokenType == IntegerToken) {
      *longInterpretedValue = _wtol(token);
    } else if (tokenType == RealToken) {
      *longInterpretedValue = static_cast<long>(_wtof(token));
    } else {
      throwConversionError();
    }
    *resultDefinition = MAKELONG(1, 1);
  } else {
    double* doubleInterpretedValue = reinterpret_cast<double*>(resultValue);

    if (tokenType == IntegerToken) {
      *doubleInterpretedValue = static_cast<double>(_wtoi(token));
    } else if (tokenType == RealToken) {
      *doubleInterpretedValue = _wtof(token);
    } else {
      throwConversionError();
    }
    *resultDefinition = MAKELONG(1, 2);
  }
}

void lex::EvalTokenStream(int* aiTokId, long* operandDefinition, int* operandType, void* operandBuffer) {
  wchar_t szTok[256]{};

  int iDim{0};
  int iTyp{0};

  long lDef1 = MAKELONG(1, 1);
  int iDim1{0};
  int iLen1{0};
  int iTyp1{IntegerToken};

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
      lDef1 = lValues[valueLocation[iTokLoc]];
      memcpy(operandBuffer, &lValues[valueLocation[iTokLoc] + 1], static_cast<size_t>(HIWORD(lDef1) * 4));
    } else {  // Token is an operator .. Pop an operand from operand stack
      if (operandStackTop == 0) { throw L"Operand stack is empty"; }

      iTyp1 = operandStack[operandStackTop];
      lDef1 = lOpStkDef[operandStackTop];
      iLen1 = HIWORD(lDef1);
      memcpy(operandBuffer, &lOpStk[operandStackTop--][0], static_cast<size_t>(iLen1 * 4));

      if (TokenPropertiesTable[tokenType].tokenClass == Other) {  // intrinsics and oddball unary minus/plus
        if (iTyp1 == StringToken) {
          iDim1 = LOWORD(lDef1);
          wcscpy_s(szTok, 256, reinterpret_cast<wchar_t*>(operandBuffer));
          if (tokenType == Int) {
            iTyp1 = IntegerToken;
            ConvertStringToVal(IntegerToken, lDef1, szTok, &lDef1, operandBuffer);
          } else if (tokenType == RealToken) {
            iTyp1 = RealToken;
            ConvertStringToVal(RealToken, lDef1, szTok, &lDef1, operandBuffer);
          } else if (tokenType == StringToken) {
            ;
          } else {
            throw L"String operand conversions error: unknown";
          }
        } else if (iTyp1 == IntegerToken) {
          UnaryOp(tokenType, &iTyp1, &lDef1, lOp1);
        } else {
          UnaryOp(tokenType, &iTyp1, &lDef1, dOp1);
        }
      } else if (TokenPropertiesTable[tokenType].tokenClass == BinaryArithmeticOperator) {
        if (operandStackTop == 0) { throw L"Binary Arithmetic: Only one operand."; }
        iTyp2 = operandStack[operandStackTop];  // Pop second operand from operand stack
        lDef2 = lOpStkDef[operandStackTop];
        iLen2 = HIWORD(lDef2);
        memcpy(secondOperandBuffer, &lOpStk[operandStackTop--][0], static_cast<size_t>(iLen2 * 4));
        iTyp = std::min(iTyp2, RealToken);
        if (iTyp1 < iTyp) {  // Convert first operand
          ConvertValTyp(iTyp1, iTyp, &lDef1, lOp1);
          iTyp1 = iTyp;
          iLen1 = HIWORD(lDef1);
        } else {
          iTyp = std::min(iTyp1, RealToken);
          if (iTyp2 < iTyp) {  // Convert second operand
            ConvertValTyp(iTyp2, iTyp, &lDef2, lOp2);
            iTyp2 = iTyp;
            iLen2 = HIWORD(lDef2);
          }
        }
        if (tokenType == BinaryAddToken) {
          if (iTyp1 == StringToken) {
            iDim1 = LOWORD(lDef1);
            iDim2 = LOWORD(lDef2);
            iDim = iDim2 + iDim1;

            errno_t err = wcscat_s(cOp2, 256, reinterpret_cast<wchar_t*>(operandBuffer));
            if (err != 0) { throw L"String concatenation overflow!"; }

            wcscpy_s(reinterpret_cast<wchar_t*>(operandBuffer), static_cast<size_t>(HIWORD(lDef1) * 4), cOp2);
            iLen1 = 1 + (iDim - 1) / 4;
            lDef1 = MAKELONG(iDim, iLen1);
          } else {
            if (iTyp1 == IntegerToken) {
              lOp1[0] += lOp2[0];
            } else {
              dOp1[0] += dOp2[0];
            }
          }
        } else if (tokenType == BinarySubtractToken) {
          if (iTyp1 == StringToken) { throw L"Can not subtract strings"; }
          if (iTyp1 == IntegerToken) {
            lOp1[0] = lOp2[0] - lOp1[0];
          } else {
            dOp1[0] = dOp2[0] - dOp1[0];
          }
        } else if (tokenType == MultiplyToken) {
          if (iTyp1 == StringToken) { throw L"Can not mutiply strings"; }
          if (iTyp1 == IntegerToken) {
            lOp1[0] *= lOp2[0];
          } else {
            if (iTyp1 == RealToken) {
              iTyp1 = iTyp2;
            } else if (iTyp2 == RealToken) {
              ;
            } else {
              throw L"Invalid mix of multiplicands";
            }
            dOp1[0] *= dOp2[0];
          }
        } else if (tokenType == DivideToken) {
          if (iTyp1 == StringToken) { throw L"Can not divide strings"; }
          if (iTyp1 == IntegerToken) {
            if (lOp1[0] == 0) { throw std::domain_error("Attempting to divide by 0"); }
            lOp1[0] = lOp2[0] / lOp1[0];
          } else if (iTyp1 <= iTyp2) {
            if (dOp1[0] == 0.0) { throw std::domain_error("Attempting to divide by 0."); }
            if (iTyp1 == iTyp2) {
              iTyp1 = RealToken;
            } else if (iTyp1 == RealToken) {
              iTyp1 = iTyp2;
            } else if (iTyp1 == ArchitecturalUnitsLengthToken) {
              iTyp1 = ArchitecturalUnitsLengthToken;
            } else if (iTyp1 == EngineeringUnitsLengthToken) {
              iTyp1 = EngineeringUnitsLengthToken;
            } else if (iTyp1 == SimpleUnitsLengthToken) {
              iTyp1 = SimpleUnitsLengthToken;
            }
            dOp1[0] = dOp2[0] / dOp1[0];
          } else {
            throw L"Division type error";
          }
        } else if (tokenType == ExponentiateToken) {
          if (iTyp1 == IntegerToken) {
            if ((lOp1[0] >= 0 && lOp1[0] > DBL_MAX_10_EXP) || (lOp1[0] < 0 && lOp1[0] < DBL_MIN_10_EXP)) {
              throw L"Exponentiation error";
            }

            lOp1[0] = (int)pow((double)lOp2[0], lOp1[0]);
          } else if (iTyp1 == RealToken) {
            int iExp = (int)dOp1[0];

            if ((iExp >= 0 && iExp > DBL_MAX_10_EXP) || (iExp < 0 && iExp < DBL_MIN_10_EXP)) {
              throw L"Exponentiation error";
            }
            dOp1[0] = pow(dOp2[0], dOp1[0]);
          }
        }
        // TODO: Support for BinaryRelationalOperator (== != > >= < <=) should go here
        //       Support for BinaryLogicOperator (& |) should go here
        //       Support for UnaryLogicOperator (!) should go here
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
    if (numberOfTokensInStream == MaxTokens) { return; }

    tokenTypeIdentifiers[numberOfTokensInStream] = tokenId;
    int iLen = static_cast<int>(wcslen(token));
    int iDim{0};
    double dVal{0.0};

    switch (tokenId) {
      case IdentifierToken:
        iDim = static_cast<int>(wcslen(token));
        iLen = 1 + (iDim - 1) / 2;

        valueLocation[numberOfTokensInStream] = numberOfValues + 1;
        lValues[numberOfValues + 1] = iDim + iLen * 65536;
        memcpy(&lValues[numberOfValues + 2], token, static_cast<size_t>(iDim));
        numberOfValues = numberOfValues + 1 + iLen;
        break;

      case StringToken:
        ParseStringOperand(token);
        break;

      case IntegerToken:
        valueLocation[numberOfTokensInStream] = numberOfValues;
        lValues[numberOfValues++] = MAKELONG(1, 1);
        lValues[numberOfValues++] = _wtoi(token);
        break;

      case RealToken:
      case ArchitecturalUnitsLengthToken:
      case EngineeringUnitsLengthToken:
      case SimpleUnitsLengthToken:
        dVal = (tokenId == RealToken) ? _wtof(token) : app.ParseLength(token);

        valueLocation[numberOfTokensInStream] = numberOfValues;
        lValues[numberOfValues++] = MAKELONG(1, 2);
        memcpy(&lValues[numberOfValues++], &dVal, sizeof(double));
        numberOfValues++;
        break;
    }
    numberOfTokensInStream++;
  }
}

void lex::ParseStringOperand(wchar_t* token) {
  if (wcslen(token) < 3) {
    app.AddStringToMessageList(IDS_MSG_ZERO_LENGTH_STRING);
    return;
  }

  auto values = (wchar_t*)&lValues[numberOfValues + 2];

  int iDim{0};
  int next{1};
  while (token[next] != '\0') {
    if (token[next] == '"' && token[next + 1] == '"') { next++; }
    values[iDim++] = token[next++];
  }
  values[--iDim] = '\0';
  int iLen = 1 + (iDim - 1) / 2;
  valueLocation[numberOfTokensInStream] = ++numberOfValues;
  lValues[numberOfValues] = MAKELONG(iDim, iLen);
  numberOfValues += iLen;
}

int lex::Scan(wchar_t* token, const wchar_t* inputLine, int& linePosition) {
  while (inputLine[linePosition] == L' ') { linePosition++; }

  if (inputLine[linePosition] == L'\0') { return -1; }  // End of line, no token

  Lexer lexer(reflex::Input(inputLine + linePosition));

  int tokenId = lexer.lex();

  if (tokenId == 0) {  // Unmatched input (error or unexpected char)
    token[0] = inputLine[linePosition];
    token[1] = L'\0';
    linePosition++;  // Advance one char
    tokenId = -1;    // Signal error
  } else {           // Copy matched text
    std::wstring matched = lexer.wstr();
    size_t length = lexer.wsize();
    wcsncpy_s(token, length + 1, matched.c_str(), length);
    token[length] = L'\0';
    linePosition += static_cast<int>(length);
  }
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Token `%s` TokenID = %d\n", token, tokenId);
  return tokenId;
}

int lex::TokType(int tokenType) {
  return (tokenType >= 0 && tokenType < numberOfTokensInStream) ? tokenTypeIdentifiers[tokenType] : -1;
}

void lex::UnaryOp(int aiTokTyp, int* aiTyp, long* alDef, double* adOp) {
  ValueMetaInformation valueMetaInformation{};

  switch (aiTokTyp) {
    case UnaryMinus:
      for (int i = 0; i < HIWORD(*alDef) / 2; i++) { adOp[i] = -adOp[i]; }
      break;

    case UnaryPlus:
      break;

    case AbsoluteValue:
      adOp[0] = fabs(adOp[0]);
      break;

    case ArcCosine:
      if (fabs(adOp[0]) > 1.0) {
        throw std::domain_error("acos of a value greater than 1. or less than -1.");
      } else {
        adOp[0] = acos(Eo::RadianToDegree(adOp[0]));
      }
      break;

    case ArcSine:
      if (fabs(adOp[0]) > 1.0) {
        throw std::domain_error("asin of a value greater than 1. or less than -1.");
      } else {
        adOp[0] = asin(Eo::RadianToDegree(adOp[0]));
      }
      break;

    case ArcTangent:
      adOp[0] = atan(Eo::RadianToDegree(adOp[0]));
      break;

    case Cosine:
      adOp[0] = cos(Eo::DegreeToRadian(adOp[0]));
      break;

    case Real:
      break;

    case ExponentialValue:
      adOp[0] = exp(adOp[0]);
      break;

    case Int:  // Conversion to integer
      ConvertValTyp(RealToken, IntegerToken, alDef, (void*)adOp);
      *aiTyp = IntegerToken;
      break;

    case NaturalLogarithm:
      if (adOp[0] <= 0.0) {
        throw std::domain_error("ln of a non-positive number");
      } else {
        adOp[0] = log(adOp[0]);
      }
      break;

    case Base10Logarithm:
      if (adOp[0] <= 0.0) {
        throw std::domain_error("log of a non-positive number");
      } else {
        adOp[0] = log10(adOp[0]);
      }
      break;

    case Sine:
      adOp[0] = sin(Eo::DegreeToRadian(adOp[0]));
      break;

    case SquareRoot:
      if (adOp[0] < 0.0) {
        throw std::domain_error("sqrt of a negative number");
      } else {
        adOp[0] = sqrt(adOp[0]);
      }
      break;

    case Tangent:
      adOp[0] = tan(Eo::DegreeToRadian(adOp[0]));
      break;

    case String: {  // Conversion to string
      wchar_t stringBuffer[32]{};
      int iDim = LOWORD(*alDef);
      *aiTyp = StringToken;
      valueMetaInformation.type = RealToken;
      valueMetaInformation.definition = *alDef;
      ConvertValToString(static_cast<void*>(adOp), &valueMetaInformation, stringBuffer, &iDim);
      int iLen = 1 + (iDim - 1) / 4;
      wcscpy_s(reinterpret_cast<wchar_t*>(adOp), 32, stringBuffer);
      *alDef = MAKELONG(iDim, iLen);
    } break;

    default:
      throw "Unknown operation";
  }
}

void lex::UnaryOp(int aiTokTyp, int* tokenType, long* alDef, long* alOp) {
  ValueMetaInformation valueMetaInformation{};

  switch (aiTokTyp) {
    case UnaryMinus:
      alOp[0] = -alOp[0];
      break;

    case UnaryPlus:
      break;

    case AbsoluteValue:
      alOp[0] = labs(alOp[0]);
      break;

    case Int:  // Conversion to integer
      break;

    case Real:
      ConvertValTyp(IntegerToken, RealToken, alDef, (void*)alOp);
      *tokenType = RealToken;
      break;

    case String: {  // Conversion to string
      wchar_t stringBuffer[32]{};
      int iDim = LOWORD(*alDef);
      int iLen = HIWORD(*alDef);

      *tokenType = StringToken;
      valueMetaInformation.type = IntegerToken;
      valueMetaInformation.definition = *alDef;
      ConvertValToString(static_cast<void*>(alOp), &valueMetaInformation, stringBuffer, &iDim);
      iLen = 1 + (iDim - 1) / 4;
      wcscpy_s((wchar_t*)alOp, 32, stringBuffer);
      *alDef = MAKELONG(iDim, iLen);
    } break;

    default:
      throw L"Unknown operation";
  }
}

wchar_t* lex::ScanForChar(wchar_t character, wchar_t** lineBuffer) {
  auto position = SkipWhiteSpace(*lineBuffer);

  if (*position == character) {
    *lineBuffer = position + 1;
    return position;
  }
  return nullptr;  // not found
}

wchar_t* lex::SkipWhiteSpace(wchar_t* inputLine) {
  while (inputLine && *inputLine && isspace(*inputLine)) { inputLine++; }

  return inputLine;
}

wchar_t* lex::ScanForString(wchar_t** ppStr, wchar_t* pszTerm, wchar_t** ppArgBuf) {
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
