#pragma once
#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN  // Exclude rarely-used stuff from Windows headers
#endif

#define NOMINMAX

#include "TargetVer.h"

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

#include <atltrace.h>
#ifdef TRACE
#undef TRACE
#endif

UINT AFXAPI HashKey(CString& str);

using EoByte = unsigned char;
using EoSbyte = char;
using EoInt16 = short;
using EoUInt16 = unsigned short;

#if defined(USING_DDE)
#include <ddeml.h>
#endif  // USING_DDE
