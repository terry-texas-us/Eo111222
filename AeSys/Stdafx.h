#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h> // MFC core and standard components
#include <afxext.h> // MFC extensions

#include <afxdisp.h> // MFC Automation classes

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h> // MFC support for ribbons and control bars

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#include <cfloat>
#include <math.h>
#include <memory>

#if defined(USING_Direct2D)
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease) {
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}

#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) do {if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}} while(0)
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif
#endif // USING_Direct2D

#include "Resource.h"

#if defined(USING_ODA)
#include <xnamath.h>
#include "OdaCommon.h"
#include "Ge/GeVector3d.h"
#include "Ge/GePoint3d.h"
#include "Ge/GeScale3d.h"
#else // !USING_ODA
#include <DirectXMath.h>
using namespace DirectX;

#endif // USING_ODA

typedef unsigned char EoByte;
typedef char EoSbyte;
typedef short EoInt16;
typedef unsigned short EoUInt16;

UINT AFXAPI HashKey(CString& str);

const double PI = 3.14159265358979323846;
const double HALF_PI = PI / 2.;
const double QUARTER_PI = PI / 4.;
const double RADIAN = PI / 180.;
const double TWOPI = PI + PI;

const float EoMmPerInch = 25.4f;

#include "SafeMath.h"

#include "EoGeVector3d.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeMatrix.h"
#include "EoGeTransformMatrix.h"
#include "EoGeReferenceSystem.h"
#include "EoGeLine.h"
#include "EoGePolyline.h"
#include "EoGeUniquePoint.h"

#include "EoGsViewport.h"
#include "EoGsModelTransform.h"
#include "EoGsViewTransform.h"

#include "EoDb.h"
#include "EoDbLineType.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"

#include "EoDbPrimitive.h"
#include "EoDbBlockReference.h"
#include "EoDbDimension.h"
#include "EoDbEllipse.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPolyline.h"
#include "EoDbSpline.h"
#include "EoDbText.h"
#include "EoDbMaskedPrimitive.h"
#include "EoDbGroup.h"
#include "EoDbBlock.h"
#include "EoDbGroupList.h"
#include "EoDbLayer.h"

#include "PrimState.h"
#include "Section.h"

#if defined(USING_DDE)
#include <ddeml.h>
#endif // USING_DDE
