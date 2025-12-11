#include "stdafx.h"

#include "AeSysView.h"

#include "EoDlgActiveViewKeyplan.h"
CPoint	pnt;
bool	bPtInRect = FALSE;

LRESULT CALLBACK WndProcKeyPlan(HWND, UINT, WPARAM, LPARAM);

void WndProcKeyPlanOnDraw(HWND);
void WndProcKeyPlanOnMouseMove(HWND, UINT, LPARAM);
void WndProcKeyPlanOnNewRatio(HWND, LPARAM);

ATOM WINAPI RegisterKeyPlanWindowClass(HINSTANCE instance) {
	WNDCLASS  Class;

	Class.style	= CS_HREDRAW | CS_VREDRAW;
	Class.lpfnWndProc = WndProcKeyPlan;
	Class.cbClsExtra = 0;
	Class.cbWndExtra = 0;
	Class.hInstance = instance;
	Class.hIcon = 0;
	Class.hCursor = (HCURSOR) ::LoadImage(HINSTANCE(NULL), MAKEINTRESOURCE(IDC_CROSS), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);
	Class.hbrBackground	= (HBRUSH) ::GetStockObject(BLACK_BRUSH);
	Class.lpszMenuName = 0;
	Class.lpszClassName	= L"KeyPlanWindow";

	return ::RegisterClass(&Class);
}

LRESULT CALLBACK WndProcKeyPlan(HWND hwnd, UINT message, UINT nParam, LPARAM lParam) {
	switch (message) {
	case WM_USER_ON_NEW_RATIO:
		WndProcKeyPlanOnNewRatio(hwnd, lParam);
		break;

	case WM_PAINT:
		WndProcKeyPlanOnDraw(hwnd);
		return (FALSE);

	case WM_LBUTTONDOWN:
		SetFocus(hwnd);
		pnt.x = LOWORD(lParam); pnt.y = HIWORD(lParam);
		bPtInRect = ::PtInRect(&EoDlgActiveViewKeyplan::m_rcWnd, pnt) != 0;
		return (FALSE);

	case WM_MOUSEMOVE:
		WndProcKeyPlanOnMouseMove(hwnd, nParam, lParam);
		return (FALSE);
	}
	return DefWindowProc(hwnd, message, nParam, lParam);
}

void WndProcKeyPlanOnDraw(HWND hwnd) {
	PAINTSTRUCT ps;

	CDC dc;
	dc.Attach(::BeginPaint(hwnd, &ps));

	CDC dcMem;
	dcMem.CreateCompatibleDC(NULL);

	dcMem.SelectObject(CBitmap::FromHandle(EoDlgActiveViewKeyplan::m_hbmKeyplan));
	BITMAP bitmap;
	::GetObject(EoDlgActiveViewKeyplan::m_hbmKeyplan, sizeof(BITMAP), (LPTSTR) &bitmap);

	dc.BitBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, &dcMem, 0, 0, SRCCOPY);

	CBrush* pBrush = (CBrush*) dc.SelectStockObject(NULL_BRUSH);
	dc.Rectangle(0, 0, bitmap.bmWidth, bitmap.bmHeight);

	//Note: Need to use the CWnd associated with Keyplan and not the active app view

	AeSysView* ActiveView = AeSysView::GetActiveView();

	EoGePoint3d Target = ActiveView->CameraTarget();

	float UMin = static_cast<float>(Target.x) + ActiveView->UMin();
	float UMax = static_cast<float>(Target.x) + ActiveView->UMax();
	float VMin = static_cast<float>(Target.y) + ActiveView->VMin();
	float VMax = static_cast<float>(Target.y) + ActiveView->VMax();
	float UMinOverview = static_cast<float>(Target.x) + ActiveView->OverviewUMin();
	float VMinOverview = static_cast<float>(Target.y) + ActiveView->OverviewVMin();

	CRect rc;
	rc.left = EoRound((UMin - UMinOverview) / ActiveView->OverviewUExt() * bitmap.bmWidth);
	rc.right = EoRound((UMax - UMinOverview) / ActiveView->OverviewUExt() * bitmap.bmWidth);
	rc.top = EoRound((1.0f - (VMax - VMinOverview) / ActiveView->OverviewVExt()) * bitmap.bmHeight);
	rc.bottom = EoRound((1.0f - (VMin - VMinOverview) / ActiveView->OverviewVExt()) * bitmap.bmHeight);

	int DrawMode = dc.SetROP2(R2_XORPEN);

	// Show current window as light gray rectangle with no outline
	dc.SelectStockObject(LTGRAY_BRUSH);
	CPen* pPen = (CPen*) dc.SelectStockObject(NULL_PEN);
	dc.Rectangle(rc.left, rc.top, rc.right, rc.bottom);

	// Show defining window as hollow rectangle with dark gray outline
	dc.SelectStockObject(NULL_BRUSH);
	CPen penGray(PS_SOLID, 2, RGB(0x80, 0x80, 0x80));
	dc.SelectObject(&penGray);
	dc.Rectangle(EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top, EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

	// Restore device context
	dc.SelectObject(pPen);
	dc.SelectObject(pBrush);
	dc.SetROP2(DrawMode);

	dc.Detach();

	::EndPaint(hwnd, &ps);
}

void WndProcKeyPlanOnMouseMove(HWND hwnd, UINT nParam, LPARAM lParam) {
	if (LOWORD(nParam) == MK_LBUTTON) {
		CPoint pntCur;

		pntCur.x = LOWORD(lParam);
		pntCur.y = HIWORD(lParam);

		CDC dcKeyPlan;
		HDC hDCKeyplan = ::GetDC(hwnd);
		int DrawMode = ::SetROP2(hDCKeyplan, R2_XORPEN);

		// Show defining window as hollow rectangle with dark gray outline
		HBRUSH hBrush = (HBRUSH) ::SelectObject(hDCKeyplan, ::GetStockObject(NULL_BRUSH));
		HPEN hPenGray = ::CreatePen(PS_SOLID, 2, RGB(0x80, 0x80, 0x80));
		HPEN hPen = (HPEN) ::SelectObject(hDCKeyplan, hPenGray);
		::Rectangle(hDCKeyplan, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top, EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

		if (bPtInRect)
			::OffsetRect(&EoDlgActiveViewKeyplan::m_rcWnd, (pntCur.x - pnt.x), (pntCur.y - pnt.y));
		else {
			if (pntCur.x > EoDlgActiveViewKeyplan::m_rcWnd.right)
				EoDlgActiveViewKeyplan::m_rcWnd.right += (pntCur.x - pnt.x);
			else if (pntCur.x < EoDlgActiveViewKeyplan::m_rcWnd.left)
				EoDlgActiveViewKeyplan::m_rcWnd.left += (pntCur.x - pnt.x);
			if (pntCur.y > EoDlgActiveViewKeyplan::m_rcWnd.bottom)
				EoDlgActiveViewKeyplan::m_rcWnd.bottom += (pntCur.y - pnt.y);
			else if (pntCur.y < EoDlgActiveViewKeyplan::m_rcWnd.top)
				EoDlgActiveViewKeyplan::m_rcWnd.top += (pntCur.y - pnt.y);
			::SendMessage(::GetParent(hwnd), WM_COMMAND, (WPARAM) ::GetWindowLong(hwnd, GWL_ID), (LPARAM) hwnd);
		}
		pnt = pntCur;
		::Rectangle(hDCKeyplan, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top, EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

		::SelectObject(hDCKeyplan, hPen);
		::SelectObject(hDCKeyplan, hBrush);
		::DeleteObject(hPenGray);
		::SetROP2(hDCKeyplan, DrawMode);
		::ReleaseDC(hwnd, hDCKeyplan);
	}
}

void WndProcKeyPlanOnNewRatio(HWND hwnd, LPARAM lParam) {
	double Ratio = *(double*) (LPDWORD) lParam;

	AeSysView* ActiveView = AeSysView::GetActiveView();

	EoGePoint3d Target = ActiveView->CameraTarget();

	float UExtent = ActiveView->WidthInInches() / static_cast<float>(Ratio);
	float UMin = static_cast<float>(Target.x) - (UExtent * 0.5f);
	float UMax = UMin + UExtent;
	float VExtent = ActiveView->HeightInInches() / static_cast<float>(Ratio);
	float VMin = static_cast<float>(Target.y) - (VExtent * 0.5f);
	float VMax = VMin + VExtent;

	HDC hDCKeyplan = ::GetDC(hwnd);
	int DrawMode = ::SetROP2(hDCKeyplan, R2_XORPEN);

	HBRUSH hBrush = (HBRUSH) ::SelectObject(hDCKeyplan, ::GetStockObject(NULL_BRUSH));
	HPEN hPenGray = ::CreatePen(PS_SOLID, 2, RGB(0x80, 0x80, 0x80));
	HPEN hPen = (HPEN) ::SelectObject(hDCKeyplan, hPenGray);
	::Rectangle(hDCKeyplan, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top, EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);

	CDC dcMem;
	dcMem.CreateCompatibleDC(NULL);

	dcMem.SelectObject(CBitmap::FromHandle(EoDlgActiveViewKeyplan::m_hbmKeyplan));
	BITMAP bitmap; ::GetObject(EoDlgActiveViewKeyplan::m_hbmKeyplan, sizeof(BITMAP), (LPSTR) &bitmap);

	float dUMinOverview = static_cast<float>(Target.x) + ActiveView->OverviewUMin();
	float dVMinOverview = static_cast<float>(Target.y) + ActiveView->OverviewVMin();

	EoDlgActiveViewKeyplan::m_rcWnd.left = EoRound((UMin - dUMinOverview) / ActiveView->OverviewUExt() * bitmap.bmWidth);
	EoDlgActiveViewKeyplan::m_rcWnd.top = EoRound((1.0f - (VMax - dVMinOverview) / ActiveView->OverviewVExt()) * bitmap.bmHeight);
	EoDlgActiveViewKeyplan::m_rcWnd.right = EoRound((UMax - dUMinOverview) / ActiveView->OverviewUExt() * bitmap.bmWidth);
	EoDlgActiveViewKeyplan::m_rcWnd.bottom = EoRound((1.0f - (VMin - dVMinOverview) / ActiveView->OverviewVExt()) * bitmap.bmHeight);

	::Rectangle(hDCKeyplan, EoDlgActiveViewKeyplan::m_rcWnd.left, EoDlgActiveViewKeyplan::m_rcWnd.top, EoDlgActiveViewKeyplan::m_rcWnd.right, EoDlgActiveViewKeyplan::m_rcWnd.bottom);
	::SelectObject(hDCKeyplan, hPen);
	::SelectObject(hDCKeyplan, hBrush);
	::DeleteObject(hPenGray);

	::SetROP2(hDCKeyplan, DrawMode);
	::ReleaseDC(hwnd, hDCKeyplan);
}
