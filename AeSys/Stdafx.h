#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN  // Exclude rarely-used stuff from Windows headers
#endif

#define NOMINMAX

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS  // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

//#include <afxwin.h>  // MFC core and standard components
//#include <afxext.h>  // MFC extensions

//#include <afxdisp.h>  // MFC Automation classes

#ifndef _AFX_NO_AFXCMN_SUPPORT
//#include <afxcmn.h>  // MFC support for Windows Common Controls
#endif  // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>  // MFC support for ribbons and control bars

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment( \
    linker,      \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment( \
    linker,      \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment( \
    linker,      \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#if defined(USING_Direct2D)
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

template <class Interface>
inline void SafeRelease(Interface** ppInterfaceToRelease) {
  if (*ppInterfaceToRelease != nullptr) {
    (*ppInterfaceToRelease)->Release();
    (*ppInterfaceToRelease) = nullptr;
  }
}

#ifndef Assert
#if defined(DEBUG) || defined(_DEBUG)
#define Assert(b)                                         \
  do {                                                    \
    if (!(b)) { OutputDebugStringA("Assert: " #b "\n"); } \
  } while (0)
#else
#define Assert(b)
#endif  //DEBUG || _DEBUG
#endif

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE) & __ImageBase)
#endif
#endif  // USING_Direct2D

#include "Resource.h"

#include <atltrace.h>
#ifdef TRACE
#undef TRACE
#endif

UINT AFXAPI HashKey(CString& str);

using EoByte = unsigned char;
using EoSbyte = char;
using EoInt16 = short;
using EoUInt16 = unsigned short;

#include <cmath>
#include <numbers>

namespace Eo {
constexpr double MmPerInch = 25.4;

constexpr double Pi = std::numbers::pi;
constexpr double QuarterPi = std::numbers::pi / 4.0;
constexpr double HalfPi = std::numbers::pi / 2.0;
constexpr double ThreeQuartersPi = 3.0 * std::numbers::pi / 4.0;
constexpr double TwoPi = std::numbers::pi + std::numbers::pi;
constexpr double Radian = std::numbers::pi / 180.0;
constexpr double DegreeToRadian(double angleInDegrees) noexcept { return angleInDegrees * Radian; }
constexpr double RadianToDegree(double angleInRadians) noexcept { return angleInRadians * (180.0 / std::numbers::pi); }

/** @brief Returns the sign of a number: -1 if negative, 1 if positive, 0 if zero. */
constexpr int Signum(const double number) { return (number < 0.0 ? -1 : (number > 0.0 ? 1 : 0)); }

/** @brief Returns a value with the magnitude of 'a' and the sign of 'b'. */
constexpr double CopySign(const double a, const double b) { return (b >= 0.0 ? fabs(a) : -fabs(a)); }

/** @brief Rounds a double to the nearest integer using std::round, which rounds half away from zero.
* @param number The double value to round.
* @return The rounded integer value.
* @note This function uses std::round for rounding, which follows the "round half away from zero" rule. Legacy implementation had a different "round half toward infinity" rule.
*/
inline int Round(const double number) { return static_cast<int>(std::round(number)); }
}  // namespace Eo

#if defined(USING_DDE)
#include <ddeml.h>
#endif  // USING_DDE
